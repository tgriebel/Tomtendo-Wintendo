#define IMGUI_ENABLE
#define MAX_LOADSTRING ( 100 )

#include <windows.h>
#include <direct.h>
#include <combaseapi.h>
#include <comdef.h>
#include <wrl/client.h>
#include <shobjidl.h> 
#include <ole2.h>
#include <ObjBase.h>
#include <comdef.h>
#include <wincodec.h>
#include <xaudio2.h>
#include <queue>
#include <algorithm>
#include "stdafx.h"
#include "wintendoApp.h"
#include "wintendoApp_dx12.h"
#ifdef IMGUI_ENABLE
#pragma warning(push, 0)        
#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx12.h"
#include "imgui/imgui_memory_editor/imgui_memory_editor.h"
#pragma warning(pop)
#endif

using namespace DirectX;
using Microsoft::WRL::ComPtr;

HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

ATOM                RegisterWindow(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

std::wstring nesFilePath( L"Games/Skate or Die.nes" );
std::wstring shaderPath( L"Shaders/" );

struct DisplayConstantBuffer
{
	// Hardness of scanline.
	//  -8.0 = soft
	// -16.0 = medium
	float		hardScan;
	// Hardness of pixels in scanline.
	// -2.0 = soft
	// -4.0 = hard
	float		hardPix;
	// Display warp.
	// 0.0 = none
	// 1.0/8.0 = extreme
	XMFLOAT2	warp;
	XMFLOAT4	imageDim;
	XMFLOAT4	destImageDim;
	// Amount of shadow mask.
	float		maskDark;
	float		maskLight;
	bool		enable;
};

struct Vertex
{
	XMFLOAT3 position;
	XMFLOAT4 color;
	XMFLOAT2 uv;
};

struct wtApp_D3D12TextureResource
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE	cpuHandle;
	CD3DX12_GPU_DESCRIPTOR_HANDLE	gpuHandle;

	ComPtr<ID3D12Resource>			srv;
	ComPtr<ID3D12Resource>			uploadBuffer;

	const wtRawImageInterface*		srcImage;
	D3D12_RESOURCE_ALLOCATION_INFO	allocInfo;
	D3D12_RESOURCE_DESC				desc;
};

struct wtAppDisplay
{
	HWND hWnd;
};

template<uint32_t T>
struct wtImguiGraphBuffer
{
	static const uint32_t bufferSize = T;
	float samples[T];
	uint32_t start;

	wtImguiGraphBuffer()
	{
		start = 0;
	}

	void Record( const float sample )
	{
		samples[start] = sample;
		start = ( start + 1 ) % bufferSize;
	}

	float* GetSamplesBuffer()
	{
		return samples;
	}

	uint32_t GetBufferStart()
	{
		return start;
	}

	uint32_t GetBufferSize()
	{
		return bufferSize;
	}

	float GetSample( const int32_t index )
	{
		return GetSample( samples, index % bufferSize );
	}

	static float GetSample( void* data, int32_t idx )
	{
		return ( (float*)data )[idx];
	}
};

ComPtr<IDXGIAdapter1>				dxgiAdapter;
ComPtr<IDXGIFactory4>				dxgiFactory;
ComPtr<ID3D12Device>				d3d12device;

static const uint32_t				FrameCount = 3;
static const uint32_t				FrameResultCount = 1;//( FrameCount + 1 );

struct swapChain_t
{
	ComPtr<IDXGISwapChain3>			dxgi;
	ComPtr<ID3D12Resource>			renderTargets[ FrameCount ];
	ComPtr<ID3D12DescriptorHeap>	rtvHeap;
	uint32_t						rtvDescriptorSize;
};


struct view_t
{
	static const int32_t			displayScalar	= 2;
	static const int32_t			nesWidth		= PPU::ScreenWidth;
	static const int32_t			nesHeight		= PPU::ScreenHeight;
	static const int32_t			overscanY0		= displayScalar * 8;
	static const int32_t			overscanY1		= displayScalar * ( PPU::ScreenHeight - 8 );
	static const int32_t			debugAreaX		= 1024;
	static const int32_t			debugAreaY		= 0;
	static const int32_t			defaultWidth	= displayScalar * nesWidth + debugAreaX;
	static const int32_t			defaultHeight	= displayScalar * nesHeight + debugAreaY;
	CD3DX12_VIEWPORT				viewport;
	CD3DX12_RECT					scissorRect;
};


struct pipeline_t
{
	ComPtr<ID3D12Resource>			cb;
	DisplayConstantBuffer			shaderData;
	ComPtr<ID3D12Resource>			vb;
	D3D12_VERTEX_BUFFER_VIEW		vbView;
	UINT8*							pCbvDataBegin;
	UINT							cbvSrvUavDescStride;
	ComPtr<ID3D12DescriptorHeap>	cbvSrvUavHeap;
	CD3DX12_CPU_DESCRIPTOR_HANDLE	cbvSrvCpuHandle;
	CD3DX12_GPU_DESCRIPTOR_HANDLE	cbvSrvGpuHandle;
	ComPtr<ID3D12RootSignature>		rootSig;
	ComPtr<ID3D12PipelineState>		pso;
};


struct command_t
{
	ComPtr<ID3D12CommandQueue>			d3d12CommandQueue;
	ComPtr<ID3D12CommandQueue>			d3d12CopyQueue;
	ComPtr<ID3D12CommandAllocator>		commandAllocator[ FrameCount ];
	ComPtr<ID3D12CommandAllocator>		cpyCommandAllocator[ FrameCount ];
	ComPtr<ID3D12CommandAllocator>		imguiCommandAllocator[ FrameCount ];
	ComPtr<ID3D12GraphicsCommandList>	commandList[ FrameCount ];
	ComPtr<ID3D12GraphicsCommandList>	cpyCommandList[ FrameCount ];
	ComPtr<ID3D12GraphicsCommandList>	imguiCommandList[ FrameCount ];
};


struct sync_t
{
	HANDLE								fenceEvent;
	ComPtr<ID3D12Fence>					fence;
	ComPtr<ID3D12Fence>					cpyFence;
	UINT64								fenceValues[ FrameCount ];
	volatile uint64_t					frameBufferCpyLock;
	HANDLE								frameCopySemaphore;
};

swapChain_t							swapChain;
view_t								view;
pipeline_t							pipeline;
command_t							cmd;
sync_t								sync = { 0 };
uint32_t							currentFrame = 0;
uint64_t							frameNumber = 0;

static bool reset = true;
static bool pause = false;
static bool emulatorRunning = true;

wtAppDisplay		appDisplay;
wtFrameResult		frameResult[ FrameResultCount ];
uint32_t			prevFrNum = 0;
uint32_t			frNum = 0;
wtConfig			systemConfig;
string				prgRomAsm[128];
wtPatternTableImage chrRom[32];
bool				refreshChrRomTables;
wtSampleQueue		currentSoundBuffer;

using frameSampleBuffer = wtImguiGraphBuffer<500>;
frameSampleBuffer frameTimePlot;

std::vector<wtApp_D3D12TextureResource> textureResources;

wtAudioEngine						audio;
wtSystem							nesSystem;

Timer								elapsedCopyTimer;
double								elapsedCopyTime;

Timer								copyTimer;
double								copyTime;

Timer								audioSubmitTimer;
double								audioSubmitTime;

Timer								runTime;


enum ShaderResources
{
	SHADER_RESOURES_IMGUI,
	SHADER_RESOURES_SRV0,
	SHADER_RESOURES_SRV1,
	SHADER_RESOURES_SRV2,
	SHADER_RESOURES_SRV3,
	SHADER_RESOURES_SRV4,
	SHADER_RESOURES_SRV5,
	SHADER_RESOURES_SRV6,
	SHADER_RESOURES_SRV7,
	SHADER_RESOURES_SRV8,
	SHADER_RESOURES_SRV9,
	SHADER_RESOURES_SRV10,
	SHADER_RESOURES_SRV11,
	SHADER_RESOURES_SRV12,
	SHADER_RESOURES_SRV13,
	SHADER_RESOURES_SRV14,
	SHADER_RESOURES_SRV15,
	SHADER_RESOURES_SRV16,
	SHADER_RESOURES_SRV17,
	SHADER_RESOURES_SRV18,
	SHADER_RESOURES_SRV19,
	SHADER_RESOURES_SRV20,
	SHADER_RESOURES_SRV21,
	SHADER_RESOURES_SRV22,

	SHADER_RESOURES_SRV_CNT,
	SHADER_RESOURES_CBV0 = SHADER_RESOURES_SRV_CNT,
	SHADER_RESOURES_COUNT,
};

// Forward declare message handler from imgui_impl_win32.cpp
#ifdef IMGUI_ENABLE
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );
#endif
void LogApu( wtFrameResult& frameResult );


static float GetQueueSample( void* data, int idx )
{
	const wtSampleQueue* quque = reinterpret_cast<wtSampleQueue*>( data );
	return quque->Peek( idx );
}

static inline const float NormalizeCoordinate( const uint32_t x, const uint32_t length )
{
	return ( 2.0f * ( x / static_cast<float>( length ) ) - 1.0f );
}


DWORD WINAPI EmulatorThread( LPVOID lpParameter )
{
	while( emulatorRunning )
	{
		if ( reset )
		{
			nesSystem.Init( nesFilePath );
			nesSystem.GenerateRomDissambly( prgRomAsm );
			nesSystem.GenerateChrRomTables( chrRom );
			reset = false;
			pause = false;
			nesSystem.GetConfig( systemConfig );
		}

		if( !pause )
		{
			nesSystem.SyncConfig( systemConfig );
			if( refreshChrRomTables )
			{
				nesSystem.GenerateChrRomTables( chrRom );
				refreshChrRomTables = false;
			}

			emulatorRunning = emulatorRunning && nesSystem.RunFrame();
		}

		wtFrameResult& fr = frameResult[ 0 ];

		while ( InterlockedCompareExchange( &audio.soundBufferCpyLock, 1, 0 ) == 1 );
		copyTimer.Start();
		nesSystem.GetFrameResult( fr ); // FIXME: Timing issue, can be writing while doing GPU upload buffer copy
		copyTimer.Stop();
		copyTime = copyTimer.GetElapsedMs();
		InterlockedExchange( &audio.soundBufferCpyLock, 0 );

		elapsedCopyTimer.Stop();
		elapsedCopyTime = elapsedCopyTimer.GetElapsedMs();
		elapsedCopyTimer.Start();

		const bool log = audio.logSnd && !( fr.currentFrame % 60 );
		if( log )
		{
			LogApu( fr );
		}

		RedrawWindow( appDisplay.hWnd, NULL, NULL, RDW_INVALIDATE );
	}

	return 0;
}


DWORD WINAPI AudioThread( LPVOID lpParameter )
{
	audio.Init();

	while ( emulatorRunning )
	{
		if( !audio.enableSound )
		{
			if( audio.pSourceVoice != nullptr )
			{
				audio.pSourceVoice->FlushSourceBuffers();
				audio.pSourceVoice->Stop();
				audio.audioStopped = true;
			}
			Sleep( 1000 );
		}
		else if( audio.audioStopped )
		{
			audio.pSourceVoice->Start( 0, XAUDIO2_COMMIT_ALL );
			audio.audioStopped = false;
		}

		wtFrameResult& fr = frameResult[ 0 ];

		while ( InterlockedCompareExchange( &audio.soundBufferCpyLock, 1, 0 ) == 1 );
		audio.EncodeSamples( fr.soundOutput.master );
		assert( fr.soundOutput.master.IsEmpty() );		
		InterlockedExchange( &audio.soundBufferCpyLock, 0 );
		
		const bool submitted = audio.AudioSubmit();
		
		if( submitted )
		{
			audioSubmitTimer.Stop();
			audioSubmitTime = audioSubmitTimer.GetElapsedMs();
			audioSubmitTimer.Start();
		}

		static uint32_t prevMsgFrame = 0;
		if( submitted/*currentFrame != prevMsgFrame*/ )
		{
			prevMsgFrame = currentFrame;
			//char msg[256];
			//sprintf_s( msg , 256, "Last buffer took %4.2f\n", audioDuration );
			//sprintf_s( msg, 256, "Last submit: %4.2f\n", audioSubmitTime );
			//OutputDebugStringA( msg );
			if( audio.audioState.BuffersQueued == 0 )
			{
			//	OutputDebugStringA( "Audio thread starved.\n" );
			}
			else if ( audio.audioState.BuffersQueued > 10 )
			{
			//	OutputDebugStringA( "Audio thread overcommitted.\n" );
			}
		}
	}

	return 0;
}


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
	{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

	using namespace std;

	unsigned int sharedData = 0;

	runTime.Start();

	sync.frameCopySemaphore = CreateSemaphore( NULL, 0, 1, NULL );

	DWORD emuThreadID;
	DWORD audioThreadID;
	HANDLE emulatorThreadHandle = CreateThread( 0, 0, EmulatorThread, &sharedData, 0, &emuThreadID );
	HANDLE audioThreadHandle = CreateThread( 0, 0, AudioThread, &sharedData, 0, &audioThreadID );

	if ( ( emulatorThreadHandle <= 0 ) || ( audioThreadHandle <= 0 ) )
		return 0;

    LoadStringW( hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING );
    LoadStringW( hInstance, IDC_WINTENDOAPP, szWindowClass, MAX_LOADSTRING );
	RegisterWindow( hInstance );

    if ( !InitInstance ( hInstance, nCmdShow ) )
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators( hInstance, MAKEINTRESOURCE( IDC_WINTENDOAPP ) );

    MSG msg;

	// Main message loop:
	while ( GetMessage( &msg, nullptr, 0, 0 ) )
	{
		if ( !TranslateAccelerator( msg.hwnd, hAccelTable, &msg ) )
		{
			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}
	}

	runTime.Stop();

	emulatorRunning = false;
	WaitForSingleObject( emulatorThreadHandle, 1000 );
	WaitForSingleObject( audioThreadHandle, 1000 );

	DWORD emuThreadExitCode;
	DWORD audioThreadExitCode;

	TerminateThread( emulatorThreadHandle, GetExitCodeThread( emulatorThreadHandle, &emuThreadExitCode ) );
	TerminateThread( audioThreadHandle, GetExitCodeThread( audioThreadHandle, &audioThreadExitCode ) );

	CloseHandle( emulatorThreadHandle );
	CloseHandle( audioThreadHandle );

    return (int)msg.wParam;
}


ATOM RegisterWindow(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof( WNDCLASSEX );

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon( hInstance, MAKEINTRESOURCE( IDI_WINTENDOAPP ) );
    wcex.hCursor        = LoadCursor( nullptr, IDC_ARROW );
    wcex.hbrBackground  = (HBRUSH)( COLOR_WINDOW + 1 );
    wcex.lpszMenuName   = MAKEINTRESOURCEW( IDC_WINTENDOAPP );
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon( wcex.hInstance, MAKEINTRESOURCE( IDI_SMALL ) );

    return RegisterClassExW( &wcex );
}


std::wstring GetAssetFullPath( LPCWSTR assetName )
{
	return shaderPath + assetName;
}


// http://www.cplusplus.com/forum/beginner/14349/
void ToClipboard( const std::string& s )
{
	OpenClipboard( 0 );
	EmptyClipboard();
	HGLOBAL hg = GlobalAlloc( GMEM_MOVEABLE, s.size() );
	if ( !hg )
	{
		CloseClipboard();
		return;
	}
	memcpy( GlobalLock( hg ), s.c_str(), s.size() );
	GlobalUnlock( hg );
	SetClipboardData( CF_TEXT, hg );
	CloseClipboard();
	GlobalFree( hg );
}


void WaitForGpu()
{
	ThrowIfFailed( cmd.d3d12CommandQueue->Signal( sync.fence.Get(), sync.fenceValues[currentFrame] ) );

	ThrowIfFailed( sync.fence->SetEventOnCompletion( sync.fenceValues[currentFrame], sync.fenceEvent ) );
	WaitForSingleObjectEx( sync.fenceEvent, INFINITE, FALSE );

	sync.fenceValues[currentFrame]++;
}


void AdvanceNextFrame()
{
	const UINT64 currentFence = sync.fenceValues[currentFrame];
	ThrowIfFailed( cmd.d3d12CommandQueue->Signal( sync.fence.Get(), currentFence ) );

	currentFrame = swapChain.dxgi->GetCurrentBackBufferIndex();

	if ( sync.fence->GetCompletedValue() < sync.fenceValues[currentFrame] )
	{
		ThrowIfFailed( sync.fence->SetEventOnCompletion( sync.fenceValues[currentFrame], sync.fenceEvent ) );
		WaitForSingleObject( sync.fenceEvent, INFINITE );
	}

	sync.fenceValues[currentFrame] = currentFence + 1;
	frameNumber = sync.fenceValues[ currentFrame ];
}


void CreateFrameBuffers()
{
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = { 0 };
	swapChainDesc.Width = view.defaultWidth;
	swapChainDesc.Height = view.defaultHeight;
	swapChainDesc.BufferCount = FrameCount;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.Stereo = false;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.Scaling = DXGI_SCALING_NONE;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.Flags = 0;

	DXGI_SWAP_CHAIN_FULLSCREEN_DESC swapChainFullDesc;
	swapChainFullDesc.Windowed = true;
	swapChainFullDesc.RefreshRate = { 60, 1 };
	swapChainFullDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapChainFullDesc.Scaling = DXGI_MODE_SCALING_CENTERED;

	ComPtr<IDXGISwapChain1> tmpSwapChain;
	ThrowIfFailed( dxgiFactory->CreateSwapChainForHwnd( cmd.d3d12CommandQueue.Get(), appDisplay.hWnd, &swapChainDesc, nullptr/*&swapChainFullDesc*/, nullptr, &tmpSwapChain ) );

	ThrowIfFailed( tmpSwapChain.As( &swapChain.dxgi ) );
	currentFrame = swapChain.dxgi->GetCurrentBackBufferIndex();

	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = FrameCount;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ThrowIfFailed( d3d12device->CreateDescriptorHeap( &rtvHeapDesc, IID_PPV_ARGS( &swapChain.rtvHeap ) ) );

	swapChain.rtvDescriptorSize = d3d12device->GetDescriptorHandleIncrementSize( D3D12_DESCRIPTOR_HEAP_TYPE_RTV );

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle( swapChain.rtvHeap->GetCPUDescriptorHandleForHeapStart() );
	for ( uint32_t i = 0; i < FrameCount; ++i )
	{
		ThrowIfFailed( swapChain.dxgi->GetBuffer( i, IID_PPV_ARGS( &swapChain.renderTargets[i] ) ) );
		d3d12device->CreateRenderTargetView( swapChain.renderTargets[i].Get(), nullptr, rtvHandle );
		rtvHandle.Offset( 1, swapChain.rtvDescriptorSize );
	}
}


void InitImgui()
{
#ifdef IMGUI_ENABLE
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;

	ImGui::StyleColorsDark();

	ImGui_ImplWin32_Init( appDisplay.hWnd );
	ImGui_ImplDX12_Init( d3d12device.Get(), FrameCount,
		DXGI_FORMAT_R8G8B8A8_UNORM, pipeline.cbvSrvUavHeap.Get(),
		pipeline.cbvSrvUavHeap.Get()->GetCPUDescriptorHandleForHeapStart(),
		pipeline.cbvSrvUavHeap.Get()->GetGPUDescriptorHandleForHeapStart() );
#endif
}


uint32_t InitD3D12()
{
	view.viewport = CD3DX12_VIEWPORT( 0.0f, 0.0f, static_cast<float>( view.defaultWidth ), static_cast<float>( view.defaultHeight ) );
	view.scissorRect = CD3DX12_RECT( 0, view.overscanY0, view.defaultWidth, view.overscanY1 );

	UINT dxgiFactoryFlags = 0;

#if defined(_DEBUG)
	ComPtr<ID3D12Debug> debugController;
	if ( SUCCEEDED( D3D12GetDebugInterface( IID_PPV_ARGS( &debugController ) ) ) )
	{
		debugController->EnableDebugLayer();
		dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
	}
#endif

	ThrowIfFailed( CreateDXGIFactory2( dxgiFactoryFlags, IID_PPV_ARGS( &dxgiFactory ) ) );
	GetHardwareAdapter( dxgiFactory.Get(), &dxgiAdapter );
	ThrowIfFailed( D3D12CreateDevice( dxgiAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS( &d3d12device ) ) );

	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	ThrowIfFailed( d3d12device->CreateCommandQueue( &queueDesc, IID_PPV_ARGS( &cmd.d3d12CommandQueue ) ) );

	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
	ThrowIfFailed( d3d12device->CreateCommandQueue( &queueDesc, IID_PPV_ARGS( &cmd.d3d12CopyQueue ) ) );

	CreateFrameBuffers();

	D3D12_DESCRIPTOR_HEAP_DESC cbvSrvUavHeapDesc = {};
	cbvSrvUavHeapDesc.NumDescriptors = SHADER_RESOURES_COUNT;
	cbvSrvUavHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbvSrvUavHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed( d3d12device->CreateDescriptorHeap( &cbvSrvUavHeapDesc, IID_PPV_ARGS( &pipeline.cbvSrvUavHeap ) ) );
	pipeline.cbvSrvUavDescStride = d3d12device->GetDescriptorHandleIncrementSize( D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV );

	for ( uint32_t i = 0; i < FrameCount; ++i )
	{
		ThrowIfFailed( d3d12device->CreateCommandAllocator( D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS( &cmd.commandAllocator[i] ) ) );
		ThrowIfFailed( d3d12device->CreateCommandAllocator( D3D12_COMMAND_LIST_TYPE_COPY,	IID_PPV_ARGS( &cmd.cpyCommandAllocator[i] ) ) );
		ThrowIfFailed( d3d12device->CreateCommandAllocator( D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS( &cmd.imguiCommandAllocator[i] ) ) );
	}

	return 0;
}


void LogApu( wtFrameResult& frameResult )
{
	stringstream sndLogName;
	sndLogName << "apuLogs/" << "snd" << frameResult.currentFrame << ".csv";

	ofstream sndLog;
	sndLog.open( sndLogName.str(), ios::out | ios::binary );
/*
	const wtSoundBuffer& pulse1 = frameResult.soundOutput.pulse1;
	const wtSoundBuffer& pulse2 = frameResult.soundOutput.pulse2;
	const wtSoundBuffer& master = frameResult.soundOutput.master;

	sndLog << "ix,pulse1,pulse2,master\n";

	const uint32_t sampleCnt = master.GetSampleCnt();
	for( uint32_t i = 0; i < sampleCnt; ++i )
	{
		sndLog << i << "," << pulse1.Read( i ) << "," << pulse2.Read( i ) << "," << master.Read( i ) << "\n";
	}
*/
	sndLog.close();
}


void CreatePSO()
{
	D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
	featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

	if ( FAILED( d3d12device->CheckFeatureSupport( D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof( featureData ) ) ) )
	{
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}

	const uint32_t srvCnt = 1; // Only 1 texture is visible to ImGui at a time
	CD3DX12_DESCRIPTOR_RANGE1 ranges[2] = {};
	ranges[0].Init( D3D12_DESCRIPTOR_RANGE_TYPE_SRV, srvCnt, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE );
	ranges[1].Init( D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE );

	CD3DX12_ROOT_PARAMETER1 rootParameters[2] = {};
	rootParameters[0].InitAsDescriptorTable( 1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL );
	rootParameters[1].InitAsDescriptorTable( 1, &ranges[1], D3D12_SHADER_VISIBILITY_PIXEL );

	D3D12_STATIC_SAMPLER_DESC sampler = {};
	sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	sampler.MipLODBias = 0;
	sampler.MaxAnisotropy = 0;
	sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	sampler.MinLOD = 0.0f;
	sampler.MaxLOD = D3D12_FLOAT32_MAX;
	sampler.ShaderRegister = 0;
	sampler.RegisterSpace = 0;
	sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init_1_1( _countof( rootParameters ), rootParameters, 1, &sampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT );

	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;
	ThrowIfFailed( D3DX12SerializeVersionedRootSignature( &rootSignatureDesc, featureData.HighestVersion, &signature, &error ) );
	ThrowIfFailed( d3d12device->CreateRootSignature( 0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS( &pipeline.rootSig ) ) );

	ComPtr<ID3DBlob> vertexShader;
	ComPtr<ID3DBlob> pixelShader;

#if defined(_DEBUG)
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0;
#endif

	ThrowIfFailed( D3DCompileFromFile( GetAssetFullPath( L"crt.hlsl" ).c_str(), nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, nullptr ) );
	ThrowIfFailed( D3DCompileFromFile( GetAssetFullPath( L"crt.hlsl" ).c_str(), nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, nullptr ) );

	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
	{
		{ "POSITION",	0,	DXGI_FORMAT_R32G32B32_FLOAT,	0,	D3D12_APPEND_ALIGNED_ELEMENT,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,	0 },
		{ "COLOR",		0,	DXGI_FORMAT_R32G32B32A32_FLOAT,	0,	D3D12_APPEND_ALIGNED_ELEMENT,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,	0 },
		{ "TEXCOORD",	0,	DXGI_FORMAT_R32G32_FLOAT,		0,	D3D12_APPEND_ALIGNED_ELEMENT,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,	0 }
	};

	// Describe and create the graphics pipeline state object (PSO).
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout						= { inputElementDescs, _countof( inputElementDescs ) };
	psoDesc.pRootSignature					= pipeline.rootSig.Get();
	psoDesc.VS								= CD3DX12_SHADER_BYTECODE( vertexShader.Get() );
	psoDesc.PS								= CD3DX12_SHADER_BYTECODE( pixelShader.Get() );
	psoDesc.RasterizerState					= CD3DX12_RASTERIZER_DESC( D3D12_DEFAULT );
	psoDesc.BlendState						= CD3DX12_BLEND_DESC( D3D12_DEFAULT );
	psoDesc.DepthStencilState.DepthEnable	= FALSE;
	psoDesc.DepthStencilState.StencilEnable	= FALSE;
	psoDesc.SampleMask						= UINT_MAX;
	psoDesc.PrimitiveTopologyType			= D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets				= 1;
	psoDesc.RTVFormats[0]					= DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleDesc.Count				= 1;
	ThrowIfFailed( d3d12device->CreateGraphicsPipelineState( &psoDesc, IID_PPV_ARGS( &pipeline.pso ) ) );
}


void CreateCommandLists()
{
	for ( uint32_t i = 0; i < FrameCount; ++i )
	{
		ThrowIfFailed( d3d12device->CreateCommandList( 0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmd.commandAllocator[i].Get(), pipeline.pso.Get(), IID_PPV_ARGS( &cmd.commandList[i] ) ) );
		ThrowIfFailed( d3d12device->CreateCommandList( 0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmd.imguiCommandAllocator[i].Get(), pipeline.pso.Get(), IID_PPV_ARGS( &cmd.imguiCommandList[i] ) ) );
		ThrowIfFailed( d3d12device->CreateCommandList( 0, D3D12_COMMAND_LIST_TYPE_COPY, cmd.cpyCommandAllocator[ i ].Get(), pipeline.pso.Get(), IID_PPV_ARGS( &cmd.cpyCommandList[ i ] ) ) );

		ThrowIfFailed( cmd.commandList[i]->Close() );
		ThrowIfFailed( cmd.imguiCommandList[ i ]->Close() );
		ThrowIfFailed( cmd.cpyCommandList[ i ]->Close() );
	}
}


void CreateVertexBuffers()
{
	const wtFrameResult& fr = frameResult[ 0 ];

	const float x0 = NormalizeCoordinate( 0, view.defaultWidth );
	const float x1 = NormalizeCoordinate( view.displayScalar * fr.frameBuffer.GetWidth(), view.defaultWidth );
	const float y0 = NormalizeCoordinate( 0, view.defaultHeight );
	const float y1 = NormalizeCoordinate( view.displayScalar * fr.frameBuffer.GetHeight(), view.defaultHeight );

	const XMFLOAT4 tintColor = { 1.0f, 0.0f, 1.0f, 1.0f };

	Vertex triangleVertices[] =
	{
		{ { x0, y0, 0.0f }, tintColor, { 0.0f, 1.0f } },
		{ { x0, y1, 0.0f }, tintColor, { 0.0f, 0.0f } },
		{ { x1, y1, 0.0f }, tintColor, { 1.0f, 0.0f } },

		{ { x0, y0, 0.0f }, tintColor, { 0.0f, 1.0f } },
		{ { x1, y1, 0.0f }, tintColor, { 1.0f, 0.0f } },
		{ { x1, y0, 0.0f }, tintColor, { 1.0f, 1.0f } },
	};

	const UINT vertexBufferSize = sizeof( triangleVertices );

	D3D12_HEAP_PROPERTIES props = CD3DX12_HEAP_PROPERTIES( D3D12_HEAP_TYPE_UPLOAD );
	D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer( vertexBufferSize );

	ThrowIfFailed( d3d12device->CreateCommittedResource(
		&props,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS( &pipeline.vb ) ) );

	UINT8* pVertexDataBegin;
	CD3DX12_RANGE readRange( 0, 0 );
	ThrowIfFailed( pipeline.vb->Map( 0, &readRange, reinterpret_cast<void**>( &pVertexDataBegin ) ) );
	memcpy( pVertexDataBegin, triangleVertices, sizeof( triangleVertices ) );
	pipeline.vb->Unmap( 0, nullptr );

	pipeline.vbView.BufferLocation = pipeline.vb->GetGPUVirtualAddress();
	pipeline.vbView.StrideInBytes = sizeof( Vertex );
	pipeline.vbView.SizeInBytes = vertexBufferSize;
}


void CreateConstantBuffers()
{
	const UINT constantBufferDataSize = ( sizeof( DisplayConstantBuffer ) + 255 ) & ~255;

	D3D12_HEAP_PROPERTIES props = CD3DX12_HEAP_PROPERTIES( D3D12_HEAP_TYPE_UPLOAD );
	D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer( constantBufferDataSize );

	ThrowIfFailed( d3d12device->CreateCommittedResource(
		&props,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS( &pipeline.cb ) ) );

	NAME_D3D12_OBJECT( pipeline.cb );

	pipeline.shaderData.hardPix			= -3.0f;
	pipeline.shaderData.hardScan		= -8.0f;
	pipeline.shaderData.maskDark		= 0.9f;
	pipeline.shaderData.maskLight		= 1.1f;
	pipeline.shaderData.warp			= { 1.0f / 32.0f, 1.0f / 32.0f };
	pipeline.shaderData.imageDim		= { 0.0f, 0.0f, view.nesWidth, view.nesHeight };
	pipeline.shaderData.destImageDim	= { 0.0f, 0.0f, view.displayScalar * view.nesWidth, view.defaultHeight };
	pipeline.shaderData.enable			= false;

	pipeline.cbvSrvCpuHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE( pipeline.cbvSrvUavHeap->GetCPUDescriptorHandleForHeapStart(), SHADER_RESOURES_CBV0, pipeline.cbvSrvUavDescStride );
	pipeline.cbvSrvGpuHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE( pipeline.cbvSrvUavHeap->GetGPUDescriptorHandleForHeapStart(), SHADER_RESOURES_CBV0, pipeline.cbvSrvUavDescStride );

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = pipeline.cb->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = constantBufferDataSize;
	d3d12device->CreateConstantBufferView( &cbvDesc, pipeline.cbvSrvCpuHandle );

	CD3DX12_RANGE readRange( 0, 0 );
	ThrowIfFailed( pipeline.cb->Map( 0, &readRange, reinterpret_cast<void**>( &pipeline.pCbvDataBegin ) ) );
	memcpy( pipeline.pCbvDataBegin, &pipeline.shaderData, sizeof( pipeline.cb ) );
}


void CreateTextureResources()
{
	int dbgImageIx = 0;
	const wtFrameResult& fr = frameResult[ 0 ];
	const wtRawImageInterface* sourceImages[21];
	sourceImages[ dbgImageIx++ ] = &fr.frameBuffer;
	sourceImages[ dbgImageIx++ ] = &fr.nameTableSheet;
	sourceImages[ dbgImageIx++ ] = &fr.paletteDebug;
	sourceImages[ dbgImageIx++ ] = &fr.patternTable0;
	sourceImages[ dbgImageIx++ ] = &fr.patternTable1;
	sourceImages[ dbgImageIx++ ] = &fr.pickedObj8x16;

	for( int i = 0; i < 16; ++ i )
	{
		sourceImages[ dbgImageIx + i] = &chrRom[i];
	}

	const uint32_t textureCount = _countof( sourceImages );
	assert( textureCount <= SHADER_RESOURES_COUNT );

	textureResources.clear();
	textureResources.resize( textureCount );

	const UINT cbvSrvUavHeapIncrement = d3d12device->GetDescriptorHandleIncrementSize( D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV );
	const CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHeapStart( pipeline.cbvSrvUavHeap->GetCPUDescriptorHandleForHeapStart() );
	const CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHeapStart( pipeline.cbvSrvUavHeap->GetGPUDescriptorHandleForHeapStart() );

	for ( uint32_t i = 0; i < textureCount; ++i )
	{
		textureResources[i].desc.MipLevels = 1;
		textureResources[i].desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		textureResources[i].desc.Alignment = 0;
		textureResources[i].desc.Width = sourceImages[i]->GetWidth();
		textureResources[i].desc.Height = sourceImages[i]->GetHeight();
		textureResources[i].desc.Flags = D3D12_RESOURCE_FLAG_NONE;
		textureResources[i].desc.DepthOrArraySize = 1;
		textureResources[i].desc.SampleDesc.Count = 1;
		textureResources[i].desc.SampleDesc.Quality = 0;
		textureResources[i].desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

		textureResources[i].allocInfo = d3d12device->GetResourceAllocationInfo( 0, 1, &textureResources[i].desc );
		textureResources[i].srcImage = sourceImages[i];

		D3D12_HEAP_PROPERTIES props = CD3DX12_HEAP_PROPERTIES( D3D12_HEAP_TYPE_DEFAULT );

		ThrowIfFailed( d3d12device->CreateCommittedResource(
			&props,
			D3D12_HEAP_FLAG_NONE,
			&textureResources[i].desc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			IID_PPV_ARGS( &textureResources[i].srv ) ) );

		const UINT64 uploadBufferSize = GetRequiredIntermediateSize( textureResources[i].srv.Get(), 0, 1 );

		props = CD3DX12_HEAP_PROPERTIES( D3D12_HEAP_TYPE_UPLOAD );
		D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer( uploadBufferSize );

		ThrowIfFailed( d3d12device->CreateCommittedResource(
			&props,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS( &textureResources[i].uploadBuffer ) ) );

		textureResources[i].cpuHandle.InitOffsetted( cpuHeapStart, i + SHADER_RESOURES_SRV0, cbvSrvUavHeapIncrement );
		textureResources[i].gpuHandle.InitOffsetted( gpuHeapStart, i + SHADER_RESOURES_SRV0, cbvSrvUavHeapIncrement );

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = textureResources[i].desc.Format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;

		d3d12device->CreateShaderResourceView( textureResources[i].srv.Get(), &srvDesc, textureResources[i].cpuHandle );
	}
}


void CreateSyncObjects()
{
	ThrowIfFailed( d3d12device->CreateFence( 0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS( &sync.fence ) ) );
	ThrowIfFailed( d3d12device->CreateFence( 0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS( &sync.cpyFence ) ) );
	sync.fenceValues[currentFrame]++;

	sync.fenceEvent = CreateEvent( nullptr, FALSE, FALSE, nullptr );
	if ( sync.fenceEvent == nullptr )
	{
		ThrowIfFailed( HRESULT_FROM_WIN32( GetLastError() ) );
	}
}


void CreateD3D12Pipeline()
{
	CreatePSO();
	CreateCommandLists();
	CreateVertexBuffers();
	CreateTextureResources();
	CreateConstantBuffers();
	CreateSyncObjects();
	WaitForGpu();
}


void UpdateD3D12()
{
	memcpy( pipeline.pCbvDataBegin, &pipeline.shaderData, sizeof( pipeline.shaderData ) );
}


void DestroyD3D12()
{
	WaitForGpu();
	CloseHandle( sync.fenceEvent );
	CloseHandle( sync.frameCopySemaphore );
	sync.cpyFence->Release();
	sync.fence->Release();
}


void IssueTextureCopyCommands()
{
	ThrowIfFailed( cmd.cpyCommandAllocator[currentFrame]->Reset() );
	ThrowIfFailed( cmd.cpyCommandList[currentFrame]->Reset( cmd.cpyCommandAllocator[currentFrame].Get(), pipeline.pso.Get() ) );
	
	const uint32_t textureCount = static_cast<uint32_t>( textureResources.size() );
	std::vector<D3D12_SUBRESOURCE_DATA> textureData( textureCount );

	for ( uint32_t i = 0; i < textureCount; ++i )
	{
		D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition( textureResources[ i ].srv.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST );
		cmd.cpyCommandList[currentFrame]->ResourceBarrier( 1, &barrier );
	}

	const uint32_t texturePixelSize = 4;

	for ( uint32_t i = 0; i < textureCount; ++i )
	{
		D3D12_SUBRESOURCE_DATA textureData;
		textureData.pData = textureResources[i].srcImage->GetRawBuffer();
		textureData.RowPitch = textureResources[i].srcImage->GetWidth() * static_cast<uint64_t>( texturePixelSize );
		textureData.SlicePitch = textureData.RowPitch * textureResources[i].srcImage->GetHeight();

		D3D12_PLACED_SUBRESOURCE_FOOTPRINT pLayouts = {};
		pLayouts.Offset = 0;
		pLayouts.Footprint.Depth = 1;
		pLayouts.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		pLayouts.Footprint.Width = textureResources[i].srcImage->GetWidth();
		pLayouts.Footprint.Height = textureResources[i].srcImage->GetHeight();
		pLayouts.Footprint.RowPitch = static_cast<uint32_t>( textureData.RowPitch );

		UpdateSubresources( cmd.cpyCommandList[currentFrame].Get(), textureResources[i].srv.Get(), textureResources[i].uploadBuffer.Get(), 0, 0, 1, &textureData );
	}

	InterlockedExchange( &sync.frameBufferCpyLock, 0 );

	for ( uint32_t i = 0; i < textureCount; ++i )
	{
		D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition( textureResources[ i ].srv.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON );
		cmd.cpyCommandList[currentFrame]->ResourceBarrier( 1, &barrier );
	}

	ThrowIfFailed( cmd.cpyCommandList[currentFrame]->Close() );

	ID3D12CommandList* ppCommandLists[] = { cmd.cpyCommandList[currentFrame].Get() };
	cmd.d3d12CopyQueue->ExecuteCommandLists( 1, ppCommandLists );
	cmd.d3d12CopyQueue->Signal( sync.cpyFence.Get(), sync.fenceValues[ currentFrame ] );
}

void BuildImguiCommandList()
{
#ifdef IMGUI_ENABLE
	ThrowIfFailed( cmd.imguiCommandAllocator[currentFrame]->Reset() );
	ThrowIfFailed( cmd.imguiCommandList[currentFrame]->Reset( cmd.imguiCommandAllocator[currentFrame].Get(), pipeline.pso.Get() ) );
	
	cmd.imguiCommandList[currentFrame]->SetGraphicsRootSignature( pipeline.rootSig.Get() );

	ID3D12DescriptorHeap* ppHeaps[] = { pipeline.cbvSrvUavHeap.Get() };
	cmd.imguiCommandList[currentFrame]->SetDescriptorHeaps( _countof( ppHeaps ), ppHeaps );
	cmd.imguiCommandList[currentFrame]->SetGraphicsRootDescriptorTable( 0, pipeline.cbvSrvUavHeap->GetGPUDescriptorHandleForHeapStart() );

	cmd.imguiCommandList[currentFrame]->RSSetViewports( 1, &view.viewport );
	cmd.imguiCommandList[currentFrame]->RSSetScissorRects( 1, &view.scissorRect );

	D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition( swapChain.renderTargets[ currentFrame ].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET );
	cmd.imguiCommandList[currentFrame]->ResourceBarrier( 1, &barrier );

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle( swapChain.rtvHeap->GetCPUDescriptorHandleForHeapStart(), currentFrame, swapChain.rtvDescriptorSize );
	cmd.imguiCommandList[currentFrame]->OMSetRenderTargets( 1, &rtvHandle, FALSE, nullptr );

	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	wtFrameResult& fr = frameResult[ 0 ];

	if( fr.loadedState ) {
		systemConfig.sys.requestLoadState = false;
	}

	if( fr.savedState ) {
		systemConfig.sys.requestSaveState  = false;
	}

	if ( fr.replayFinished ) {
		systemConfig.sys.replay = false;
	}

	static bool autoPlayback = false;

	if ( ImGui::BeginTabBar( "Debug Info" ) )
	{
		bool systemTabOpen = true;
		if ( ImGui::BeginTabItem( "System", &systemTabOpen ) )
		{
			if ( ImGui::CollapsingHeader( "Registers", ImGuiTreeNodeFlags_OpenOnArrow ) )
			{
				ImGui::Columns( 2 );
				ImGui::Text( "A: %i", fr.state.A );
				ImGui::Text( "X: %i", fr.state.X );
				ImGui::Text( "Y: %i", fr.state.Y );
				ImGui::Text( "PC: %i", fr.state.PC );
				ImGui::Text( "SP: %i", fr.state.SP );
				ImGui::NextColumn();
				ImGui::Text( "Status Flags" );
				ImGui::Text( "Carry: %i", fr.state.P.bit.c );
				ImGui::Text( "Zero: %i", fr.state.P.bit.z );
				ImGui::Text( "Interrupt: %i", fr.state.P.bit.i );
				ImGui::Text( "Decimal: %i", fr.state.P.bit.d );
				ImGui::Text( "Unused: %i", fr.state.P.bit.u );
				ImGui::Text( "Break: %i", fr.state.P.bit.b );
				ImGui::Text( "Overflow: %i", fr.state.P.bit.v );
				ImGui::Text( "Negative: %i", fr.state.P.bit.n );
				ImGui::Columns( 1 );
			}

			if ( ImGui::CollapsingHeader( "ROM Info", ImGuiTreeNodeFlags_OpenOnArrow ) )
			{
				ImGui::Text( "PRG ROM Banks: %i", fr.romHeader.prgRomBanks );
				ImGui::Text( "CHR ROM Banks: %i", fr.romHeader.chrRomBanks );
				ImGui::Text( "Mirror Mode: %i", fr.mirrorMode );
				ImGui::Text( "Mapper ID: %i", fr.mapperId );
			}

			if ( ImGui::CollapsingHeader( "ASM", ImGuiTreeNodeFlags_OpenOnArrow ) )
			{
				for ( int i = 0; i < fr.romHeader.prgRomBanks; ++i )
				{
					if ( prgRomAsm[ i ].empty() )
						continue;

					char treeNodeName[ 128 ];
					sprintf_s( treeNodeName, 128, "prgBankText_%i", i );
					if ( ImGui::TreeNode( treeNodeName, "PRG Bank %i", i ) )
					{
						if ( ImGui::Button( "Copy" ) ) {
							ToClipboard( prgRomAsm[ i ] );
						}
						ImGui::TextWrapped( prgRomAsm[ i ].c_str() );
						ImGui::TreePop();
					}
				}
			}

			if ( ImGui::CollapsingHeader( "Trace Log", ImGuiTreeNodeFlags_OpenOnArrow ) )
			{
				if ( ImGui::Button( "Start" ) ) {
					systemConfig.cpu.traceFrameCount = 1;
				}
				static std::string log;
				if ( ( systemConfig.cpu.traceFrameCount >= 1 ) && ( fr.dbgMetrics->size() > 0 ) ) {
					log.resize( 0 );
					log.reserve( 400 * fr.dbgMetrics->size() ); // Assume 400 characters per log line
					for ( OpDebugInfo& dbgInfo : *fr.dbgMetrics )
					{
						dbgInfo.ToString( log, true );
						log += "\n";
					}
					systemConfig.cpu.traceFrameCount = 0;
				}

				ImGui::SameLine();
				if ( ImGui::Button( "Copy" ) ) {
					ToClipboard( log );
				}
				if ( ImGui::TreeNode( "Log" ) ) {
					ImGui::TextWrapped( log.c_str() );
					ImGui::TreePop();
				}
			}

			if ( ImGui::CollapsingHeader( "Timing", ImGuiTreeNodeFlags_OpenOnArrow ) )
			{
				ImGui::Text( "Frame Cycle Start: %u", fr.dbgInfo.cycleBegin );
				ImGui::Text( "Frame Cycle End: %u", fr.dbgInfo.cycleEnd );
			}

			if ( ImGui::CollapsingHeader( "Controls", ImGuiTreeNodeFlags_OpenOnArrow ) )
			{
				if ( ImGui::Button( "Load State" ) ) {
					systemConfig.sys.requestLoadState = true;
				}

				ImGui::SameLine();
				if ( ImGui::Button( "Save State" ) ) {
					systemConfig.sys.requestSaveState = true;
				}

				if ( ImGui::Button( "Record" ) ) {
					systemConfig.sys.record = !systemConfig.sys.replay;
				}

				ImGui::SameLine();
				if ( ImGui::Button( "Rewind" ) ) {
					systemConfig.sys.restoreFrame = 1;
					systemConfig.sys.replay = true;
					systemConfig.sys.record = false;
				}

				ImGui::SameLine();
				if ( ImGui::Button( "Play" ) ) {
					autoPlayback = true;
				}

				ImGui::SameLine();
				if ( ImGui::Button( "Stop" ) ) {
					autoPlayback = false;
				}

				ImGui::SameLine();
				if ( ImGui::Button( "Next" ) ) {
					systemConfig.sys.restoreFrame += 1;
					systemConfig.sys.nextScaline = 0;
				}

				/*
				if ( ImGui::Button( "Next Scanline" ) ) {
					systemConfig.sys.nextScaline += 1;
				}
				*/

				ImGui::SliderInt( "", &systemConfig.sys.restoreFrame, 0, 100 );
				ImGui::SameLine();
				ImGui::Text( "%i%%", systemConfig.sys.restoreFrame );
			}

			if ( ImGui::CollapsingHeader( "Physical Memory", ImGuiTreeNodeFlags_OpenOnArrow ) )
			{
				static MemoryEditor cpuMemEdit;
				cpuMemEdit.DrawContents( fr.state.cpuMemory, wtState::CpuMemorySize, 0 );
			}

			ImGui::EndTabItem();
		}

		bool ppuTabOpen = true;
		if ( ImGui::BeginTabItem( "PPU", &ppuTabOpen ) )
		{
			if ( ImGui::CollapsingHeader( "Frame Buffer", ImGuiTreeNodeFlags_OpenOnArrow ) )
			{
				const uint32_t imageId = 0;
				const wtRawImageInterface* srcImage = textureResources[ imageId ].srcImage;
				ImGui::Image( (ImTextureID)textureResources[ imageId ].gpuHandle.ptr, ImVec2( (float)srcImage->GetWidth(), (float)srcImage->GetHeight() ) );
				ImGui::Text( "%.3f ms/frame (%.1f FPS)", fr.dbgInfo.frameTimeUs / 1000.0f, 1000000.0f / fr.dbgInfo.frameTimeUs );
				ImGui::Text( "Display Frame #%i", frameNumber );
				ImGui::Text( "Emulator Frame #%i", fr.dbgInfo.frameNumber );
				ImGui::Text( "Copy time: %4.2f ms", copyTime );
				ImGui::Text( "Time since last copy: %4.2f ms", elapsedCopyTime );
				ImGui::Text( "Copy size: %i MB", sizeof( fr ) / 1024 / 1024 );

				frameTimePlot.Record( 1000000.0f / fr.dbgInfo.frameTimeUs );

				ImGui::PlotLines( "", &frameSampleBuffer::GetSample,
					frameTimePlot.GetSamplesBuffer(),
					frameTimePlot.GetBufferSize(),
					frameTimePlot.GetBufferStart(),
					NULL,
					FLT_MIN,
					FLT_MAX,
					ImVec2( 0, 0 ) );
			}

			if ( ImGui::CollapsingHeader( "Name Table", ImGuiTreeNodeFlags_OpenOnArrow ) )
			{
				static float ntScale = 1.0f;
				ImGui::SliderFloat( "Scale", &ntScale, 0.1f, 10.0f );
				const uint32_t imageId = 1;
				const wtRawImageInterface* srcImage = textureResources[ imageId ].srcImage;
				ImGui::Image( (ImTextureID)textureResources[ imageId ].gpuHandle.ptr, ImVec2( ntScale * srcImage->GetWidth(), ntScale * srcImage->GetHeight() ) );
			}

			if ( ImGui::CollapsingHeader( "Pattern Table", ImGuiTreeNodeFlags_OpenOnArrow ) )
			{
				static float ptScale = 1.0f;
				static int patternTableSelect = 0;
				ImGui::SliderFloat( "Scale", &ptScale, 0.1f, 10.0f );
				if ( ImGui::Button( "Switch Table" ) )
					patternTableSelect ^= 1;
				const uint32_t imageId = ( patternTableSelect == 0 ) ? 3 : 4;
				const wtRawImageInterface* srcImage = textureResources[ imageId ].srcImage;
				ImGui::Image( (ImTextureID)textureResources[ imageId ].gpuHandle.ptr, ImVec2( ptScale * srcImage->GetWidth(), ptScale * srcImage->GetHeight() ) );
				ImGui::Text( "Table=%i", patternTableSelect );
			}

			if ( ImGui::CollapsingHeader( "CHR ROM", ImGuiTreeNodeFlags_OpenOnArrow ) )
			{
				if ( ImGui::InputInt( "ChrPalette", &systemConfig.ppu.chrPalette, 1, 1, ImGuiInputTextFlags_None ) )
				{
					systemConfig.ppu.chrPalette = max( 0, min( 7, systemConfig.ppu.chrPalette ) );
					refreshChrRomTables = true;
				}

				for ( int i = 0; i < fr.romHeader.chrRomBanks; ++i )
				{
					const uint32_t imageId = 5 + i;
					const wtRawImageInterface* srcImage = textureResources[ imageId ].srcImage;
					ImGui::Image( (ImTextureID)textureResources[ imageId ].gpuHandle.ptr, ImVec2( (float)srcImage->GetWidth(), (float)srcImage->GetHeight() ) );
				}
			}

			if ( ImGui::CollapsingHeader( "Palette", ImGuiTreeNodeFlags_OpenOnArrow ) )
			{
				const uint32_t imageId = 2;
				const wtRawImageInterface* srcImage = textureResources[ imageId ].srcImage;
				ImGui::Image( (ImTextureID)textureResources[ imageId ].gpuHandle.ptr, ImVec2( 10.0f * srcImage->GetWidth(), 10.0f * srcImage->GetHeight() ) );
			}

			if ( ImGui::CollapsingHeader( "Controls", ImGuiTreeNodeFlags_OpenOnArrow ) )
			{
				ImGui::Checkbox( "Show BG", &systemConfig.ppu.showBG );
				ImGui::Checkbox( "Show Sprite", &systemConfig.ppu.showSprite );
				ImGui::SliderInt( "Line Sprites", &systemConfig.ppu.spriteLimit, 1, PPU::TotalSprites );
			}

			if ( ImGui::CollapsingHeader( "Picked Object", ImGuiTreeNodeFlags_OpenOnArrow ) )
			{
				const uint32_t imageId = 5;
				const wtRawImageInterface* srcImage = textureResources[ imageId ].srcImage;

				ImGui::Columns( 3 );
				ImGui::Text( "X: %i", fr.ppuDebug.spritePicked.x );
				ImGui::Text( "Y: %i", fr.ppuDebug.spritePicked.y );
				ImGui::Text( "Palette: %i", fr.ppuDebug.spritePicked.palette );
				ImGui::Text( "Priority: %i", fr.ppuDebug.spritePicked.priority >> 2 );
				ImGui::NextColumn();
				ImGui::Text( "Flipped X: %i", fr.ppuDebug.spritePicked.flippedHorizontal );
				ImGui::Text( "Flipped Y: %i", fr.ppuDebug.spritePicked.flippedVertical );
				ImGui::Text( "Tile ID: %i", fr.ppuDebug.spritePicked.tileId );
				ImGui::Text( "OAM Index: %i", fr.ppuDebug.spritePicked.oamIndex );
				ImGui::Text( "2nd OAM Index: %i", fr.ppuDebug.spritePicked.secondaryOamIndex );
				ImGui::NextColumn();
				const float heightScale = fr.ppuDebug.spritePicked.is8x16 ? 2.0f : 1.0f;
				ImGui::Image( (ImTextureID)textureResources[ imageId ].gpuHandle.ptr, ImVec2( 4 * srcImage->GetWidth(), 4 * heightScale * srcImage->GetHeight() ) );
				ImGui::Columns( 1 );
			}

			if ( ImGui::CollapsingHeader( "NT Memory", ImGuiTreeNodeFlags_OpenOnArrow ) )
			{
				static MemoryEditor ppuMemEdit;
				ppuMemEdit.DrawContents( fr.state.ppuMemory, wtState::PpuMemorySize );
			}
			ImGui::EndTabItem();
		}

		bool apuTabOpen = true;
		if ( ImGui::BeginTabItem( "APU", &apuTabOpen ) )
		{
			apuDebug_t& apuDebug = fr.apuDebug;
			ImGui::Checkbox( "Play Sound", &audio.enableSound );
			const float maxVolume = 2 * systemConfig.apu.volume;
			if ( ImGui::CollapsingHeader( "Pulse 1", ImGuiTreeNodeFlags_OpenOnArrow ) )
			{
				ImGui::Checkbox( "Mute", &systemConfig.apu.mutePulse1 );
				ImGui::Columns( 2 );
				ImGui::Text( "$4000 - Duty: %i", apuDebug.pulse1.regCtrl.sem.duty );
				ImGui::Text( "$4000 - Constant: %i", apuDebug.pulse1.regCtrl.sem.isConstant );
				ImGui::Text( "$4000 - Volume: %i", apuDebug.pulse1.regCtrl.sem.volume );
				ImGui::Text( "$4000 - Halt Flag: %i", apuDebug.pulse1.regCtrl.sem.counterHalt );
				ImGui::Text( "$4002 - Timer: %i", apuDebug.pulse1.regTune.sem0.timer );
				ImGui::Text( "$4003 - Counter: %i", apuDebug.pulse1.regTune.sem0.counter );
				ImGui::NextColumn();
				ImGui::Text( "Pulse Period: %i", apuDebug.pulse1.period.Value() );
				ImGui::Text( "Sweep Delta: %i", abs( apuDebug.pulse1.period.Value() - apuDebug.pulse1.regTune.sem0.timer ) );
				ImGui::Text( "$4001 - Sweep Enabled: %i", apuDebug.pulse1.regRamp.sem.enabled );
				ImGui::Text( "$4001 - Sweep Period: %i", apuDebug.pulse1.regRamp.sem.period );
				ImGui::Text( "$4001 - Sweep Shift: %i", apuDebug.pulse1.regRamp.sem.shift );
				ImGui::Text( "$4001 - Sweep Negate: %i", apuDebug.pulse1.regRamp.sem.negate );
				ImGui::Columns( 1 );
				ImGui::PlotLines( "Wave", GetQueueSample, &fr.soundOutput.dbgPulse1, fr.soundOutput.dbgPulse1.GetSampleCnt(), 0, "", -15, 15, ImVec2( 1000.0f, 100.0f ) );
			}
			if ( ImGui::CollapsingHeader( "Pulse 2", ImGuiTreeNodeFlags_OpenOnArrow ) )
			{
				ImGui::Checkbox( "Mute", &systemConfig.apu.mutePulse2 );
				ImGui::Columns( 2 );
				ImGui::Text( "Samples: %i", fr.soundOutput.dbgPulse2.GetSampleCnt() );
				ImGui::Text( "$4004 - Duty: %i", apuDebug.pulse2.regCtrl.sem.duty );
				ImGui::Text( "$4004 - Constant: %i", apuDebug.pulse2.regCtrl.sem.isConstant );
				ImGui::Text( "$4004 - Reg Volume: %i", apuDebug.pulse2.regCtrl.sem.volume );
				ImGui::Text( "$4004 - Halt Flag: %i", apuDebug.pulse2.regCtrl.sem.counterHalt );			
				ImGui::Text( "$4006 - Timer: %i", apuDebug.pulse2.regTune.sem0.timer );
				ImGui::Text( "$4007 - Counter: %i", apuDebug.pulse2.regTune.sem0.counter );
				ImGui::NextColumn();
				ImGui::Text( "Pulse Period: %i", apuDebug.pulse2.period.Value() );
				ImGui::Text( "Sweep Delta: %i", abs( apuDebug.pulse2.period.Value() - apuDebug.pulse2.regTune.sem0.timer ) );
				ImGui::Text( "$4005 - Sweep Enabled: %i", apuDebug.pulse2.regRamp.sem.enabled );
				ImGui::Text( "$4005 - Sweep Period: %i", apuDebug.pulse2.regRamp.sem.period );
				ImGui::Text( "$4005 - Sweep Shift: %i", apuDebug.pulse2.regRamp.sem.shift );
				ImGui::Text( "$4005 - Sweep Negate: %i", apuDebug.pulse2.regRamp.sem.negate );
				ImGui::Columns( 1 );
				ImGui::PlotLines( "Wave", GetQueueSample, &fr.soundOutput.dbgPulse2, fr.soundOutput.dbgPulse2.GetSampleCnt(), 0, "", -15, 15, ImVec2( 1000.0f, 100.0f ) );
			}
			if ( ImGui::CollapsingHeader( "Triangle", ImGuiTreeNodeFlags_OpenOnArrow ) )
			{
				ImGui::Checkbox( "Mute", &systemConfig.apu.muteTri );
				ImGui::Columns( 2 );
				ImGui::Text( "Samples: %i", fr.soundOutput.dbgTri.GetSampleCnt() );
				ImGui::Text( "Length Counter: %i", apuDebug.triangle.lengthCounter );
				ImGui::Text( "Linear Counter: %i", apuDebug.triangle.linearCounter.Value() );
				ImGui::Text( "Timer: %i", apuDebug.triangle.timer.Value() );
				ImGui::NextColumn();
				ImGui::Text( "$4008 - Halt Flag: %i", apuDebug.triangle.regLinear.sem.counterHalt );
				ImGui::Text( "$4008 - Counter Load: %i", apuDebug.triangle.regLinear.sem.counterLoad );
				ImGui::Text( "$400A - Timer: %i", apuDebug.triangle.regTimer.sem0.timer );
				ImGui::Text( "$400B - Counter: %i", apuDebug.triangle.regTimer.sem0.counter );
				ImGui::Columns( 1 );
				ImGui::PlotLines( "Wave", GetQueueSample, &fr.soundOutput.dbgTri, fr.soundOutput.dbgTri.GetSampleCnt(), 0, "", -30, 30, ImVec2( 1000.0f, 100.0f ) );
			}
			if ( ImGui::CollapsingHeader( "Noise", ImGuiTreeNodeFlags_OpenOnArrow ) )
			{
				ImGui::Checkbox( "Mute", &systemConfig.apu.muteNoise );
				ImGui::Columns( 2 );
				ImGui::Text( "Samples: %i", fr.soundOutput.dbgNoise.GetSampleCnt() );
				ImGui::Text( "Shifter: %i", apuDebug.noise.shift.Value() );
				ImGui::Text( "Timer: %i", apuDebug.noise.timer );
				ImGui::NextColumn();
				ImGui::Text( "$400C - Halt Flag", apuDebug.noise.regCtrl.sem.counterHalt );
				ImGui::Text( "$400C - Constant Volume", apuDebug.noise.regCtrl.sem.isConstant );
				ImGui::Text( "$400C - Volume", apuDebug.noise.regCtrl.sem.volume );
				ImGui::Text( "$400E - Mode", apuDebug.noise.regFreq1.sem.mode );
				ImGui::Text( "$400E - Period", apuDebug.noise.regFreq1.sem.period );
				ImGui::Text( "$400F - Length Counter", apuDebug.noise.regFreq2.sem.length );
				ImGui::Columns( 1 );
				ImGui::PlotLines( "Wave", GetQueueSample, &fr.soundOutput.dbgNoise, fr.soundOutput.dbgNoise.GetSampleCnt(), 0, "", -30, 30, ImVec2( 1000.0f, 100.0f ) );
			}
			if ( ImGui::CollapsingHeader( "DMC", ImGuiTreeNodeFlags_OpenOnArrow ) )
			{
				ImGui::Checkbox( "Mute", &systemConfig.apu.muteDMC );
				ImGui::Columns( 2 );
				ImGui::Text( "Samples: %i", fr.soundOutput.dbgDmc.GetSampleCnt() );
				ImGui::Text( "Volume: %i", apuDebug.dmc.outputLevel.Value() );
				ImGui::Text( "Sample Buffer: %i", apuDebug.dmc.sampleBuffer );
				ImGui::Text( "Bit Counter: %i", apuDebug.dmc.byteCnt );
				ImGui::Text( "Period: %i", apuDebug.dmc.period );
				ImGui::Text( "Period Counter: %i", apuDebug.dmc.periodCounter );
				ImGui::NextColumn();
				ImGui::Text( "$4010 - Loop: %i", apuDebug.dmc.regCtrl.sem.loop );
				ImGui::Text( "$4010 - Freq: %i", apuDebug.dmc.regCtrl.sem.freq );
				ImGui::Text( "$4010 - IRQ: %i", apuDebug.dmc.regCtrl.sem.irqEnable );
				ImGui::Text( "$4011 - Load Counter: %i", apuDebug.dmc.regLoad );
				ImGui::Text( "$4012 - Addr: %i", apuDebug.dmc.regAddr );
				ImGui::Text( "$4013 - Length: %i", apuDebug.dmc.regLength );
				ImGui::Columns( 1 );
				ImGui::PlotLines( "Wave", GetQueueSample, &fr.soundOutput.dbgDmc, fr.soundOutput.dbgDmc.GetSampleCnt(), 0, "", -30, 30, ImVec2( 1000.0f, 100.0f ) );
			}
			if ( ImGui::CollapsingHeader( "Frame Counter", ImGuiTreeNodeFlags_OpenOnArrow ) )
			{
				ImGui::Text( "Half Clock Ticks: %i", fr.apuDebug.halfClkTicks );
				ImGui::Text( "Quarter Clock Ticks: %i", fr.apuDebug.quarterClkTicks );
				ImGui::Text( "IRQ Events: %i", fr.apuDebug.irqClkEvents );
				ImGui::Text( "Cycle: %i", fr.apuDebug.cycle.count() );
				ImGui::Text( "Apu Cycle: %i", fr.apuDebug.apuCycle.count() );
				ImGui::Text( "Frame Counter Cycle: %i", fr.apuDebug.frameCounterTicks.count() );
			}
			if ( ImGui::CollapsingHeader( "Controls", ImGuiTreeNodeFlags_OpenOnArrow ) )
			{
				ImGui::InputFloat( "Volume", &systemConfig.apu.volume, 1.0f, 10.0f, 2, ImGuiInputTextFlags_None );
				ImGui::InputFloat( "Frequency", &systemConfig.apu.frequencyScale, 1.0f, 10.0f, 2, ImGuiInputTextFlags_None );
				ImGui::InputInt( "Pulse Wave Shift", &systemConfig.apu.waveShift );
				ImGui::Checkbox( "Disable Sweep", &systemConfig.apu.disableSweep );
				ImGui::Checkbox( "Disable Envelope", &systemConfig.apu.disableEnvelope );
			}
			if ( ImGui::CollapsingHeader( "Filters", ImGuiTreeNodeFlags_OpenOnArrow ) )
			{
				ImGui::InputFloat( "HighPass 1 Q:", &audio.Q1 );
				ImGui::InputFloat( "HighPass 2 Q:", &audio.Q2 );
				ImGui::InputFloat( "LowPass Q:", &audio.Q3 );
				ImGui::InputFloat( "HighPass 1 Freq:", &audio.F1 );
				ImGui::InputFloat( "HighPass 2 Freq:", &audio.F2 );
				ImGui::InputFloat( "LowPass Freq:", &audio.F3 );
			}
			
			ImGui::PlotLines( "Audio Wave", audio.dbgSoundFrameData.GetRawBuffer(), ApuBufferSize, 0, "", -32768.0f, 32768.0f, ImVec2( 1000.0f, 100.0f ) );
			ImGui::Text( "Sample Length: %i", audio.dbgLastSoundSampleLength );
			ImGui::Text( "Target MS: %4.2f", 1000.0f * audio.dbgLastSoundSampleLength / (float)wtAudioEngine::SourceFreqHz );
			ImGui::Text( "Actual MS: %4.2f", audio.audioDuration );
			ImGui::Text( "Queues: %i", audio.audioState.BuffersQueued );
			ImGui::Text( "Queues Started: %i", voiceCallback.totalQueues );
			ImGui::Text( "Time since last submit: %i", (int)audioSubmitTime );
			ImGui::EndTabItem();
		}

		bool displayTabOpen = true;
		if ( ImGui::BeginTabItem( "Display Settings", &displayTabOpen ) )
		{
			ImGui::Checkbox( "Enable", &pipeline.shaderData.enable );
			ImGui::SliderFloat( "Hard Pix", &pipeline.shaderData.hardPix, -4.0f, -2.0f );
			ImGui::SliderFloat( "Hard Scan", &pipeline.shaderData.hardScan, -16.0f, -8.0f );
			ImGui::SliderFloat( "Warp X", &pipeline.shaderData.warp.x, 0.0f, 1.0f / 8.0f );
			ImGui::SliderFloat( "Warp Y", &pipeline.shaderData.warp.y, 0.0f, 1.0f / 8.0f );
			ImGui::SliderFloat( "Mask Light", &pipeline.shaderData.maskLight, 0.0f, 2.0f );
			ImGui::SliderFloat( "Mask Dark", &pipeline.shaderData.maskDark, 0.0f, 2.0f );
			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}

	if ( autoPlayback )
	{
		if( systemConfig.sys.replay && ( ( (int)runTime.GetElapsedMs() % 8 ) == 0 ) )
		{
			++systemConfig.sys.restoreFrame;
			systemConfig.sys.restoreFrame = min( systemConfig.sys.restoreFrame, 100 );
		}
	}

	ImGui::Render();
	ImGui_ImplDX12_RenderDrawData( ImGui::GetDrawData(), cmd.imguiCommandList[currentFrame].Get() );

	barrier = CD3DX12_RESOURCE_BARRIER::Transition( swapChain.renderTargets[ currentFrame ].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT );
	cmd.imguiCommandList[currentFrame]->ResourceBarrier( 1, &barrier );

	ThrowIfFailed( cmd.imguiCommandList[currentFrame]->Close() );
#endif
}


void BuildDrawCommandList()
{
	ThrowIfFailed( cmd.commandAllocator[currentFrame]->Reset() );
	ThrowIfFailed( cmd.commandList[currentFrame]->Reset( cmd.commandAllocator[currentFrame].Get(), pipeline.pso.Get() ) );

	cmd.commandList[currentFrame]->SetGraphicsRootSignature( pipeline.rootSig.Get() );
	
	ID3D12DescriptorHeap* ppHeaps[] = { pipeline.cbvSrvUavHeap.Get() };
	cmd.commandList[currentFrame]->SetDescriptorHeaps( _countof( ppHeaps ), ppHeaps );
	cmd.commandList[currentFrame]->SetGraphicsRootDescriptorTable( 0, textureResources[0].gpuHandle );
	cmd.commandList[currentFrame]->SetGraphicsRootDescriptorTable( 1, pipeline.cbvSrvGpuHandle );

	cmd.commandList[currentFrame]->RSSetViewports( 1, &view.viewport );
	cmd.commandList[currentFrame]->RSSetScissorRects( 1, &view.scissorRect );

	D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition( swapChain.renderTargets[ currentFrame ].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET );
	cmd.commandList[currentFrame]->ResourceBarrier( 1, &barrier );

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle( swapChain.rtvHeap->GetCPUDescriptorHandleForHeapStart(), currentFrame, swapChain.rtvDescriptorSize );
	cmd.commandList[currentFrame]->OMSetRenderTargets( 1, &rtvHandle, FALSE, nullptr );
	
	const float clearColor[] = { 0.1f, 0.2f, 0.2f, 1.0f };
	cmd.commandList[currentFrame]->ClearRenderTargetView( rtvHandle, clearColor, 0, nullptr );
	cmd.commandList[currentFrame]->IASetPrimitiveTopology( D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
	cmd.commandList[currentFrame]->IASetVertexBuffers( 0, 1, &pipeline.vbView );
	cmd.commandList[currentFrame]->DrawInstanced( 6, 1, 0, 0 );
	
	barrier = CD3DX12_RESOURCE_BARRIER::Transition( swapChain.renderTargets[ currentFrame ].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT );
	cmd.commandList[currentFrame]->ResourceBarrier( 1, &barrier );

	ThrowIfFailed( cmd.commandList[currentFrame]->Close() );
}


void ExecuteDrawCommands()
{
	ID3D12CommandList* ppCommandLists[] = { cmd.commandList[ currentFrame ].Get(), cmd.imguiCommandList[ currentFrame ].Get() };
	cmd.d3d12CommandQueue->Wait( sync.cpyFence.Get(), sync.fenceValues[ currentFrame ] );
	cmd.d3d12CommandQueue->ExecuteCommandLists( _countof( ppCommandLists ), ppCommandLists );
}


void InitKeyBindings()
{
	nesSystem.input.BindKey( 'A', ControllerId::CONTROLLER_0, ButtonFlags::BUTTON_LEFT );
	nesSystem.input.BindKey( 'D', ControllerId::CONTROLLER_0, ButtonFlags::BUTTON_RIGHT );
	nesSystem.input.BindKey( 'W', ControllerId::CONTROLLER_0, ButtonFlags::BUTTON_UP );
	nesSystem.input.BindKey( 'S', ControllerId::CONTROLLER_0, ButtonFlags::BUTTON_DOWN );
	nesSystem.input.BindKey( 'G', ControllerId::CONTROLLER_0, ButtonFlags::BUTTON_SELECT );
	nesSystem.input.BindKey( 'H', ControllerId::CONTROLLER_0, ButtonFlags::BUTTON_START );
	nesSystem.input.BindKey( 'J', ControllerId::CONTROLLER_0, ButtonFlags::BUTTON_B );
	nesSystem.input.BindKey( 'K', ControllerId::CONTROLLER_0, ButtonFlags::BUTTON_A );

	nesSystem.input.BindKey( '1', ControllerId::CONTROLLER_1, ButtonFlags::BUTTON_LEFT );
	nesSystem.input.BindKey( '2', ControllerId::CONTROLLER_1, ButtonFlags::BUTTON_RIGHT );
	nesSystem.input.BindKey( '3', ControllerId::CONTROLLER_1, ButtonFlags::BUTTON_UP );
	nesSystem.input.BindKey( '4', ControllerId::CONTROLLER_1, ButtonFlags::BUTTON_DOWN );
	nesSystem.input.BindKey( '5', ControllerId::CONTROLLER_1, ButtonFlags::BUTTON_SELECT );
	nesSystem.input.BindKey( '6', ControllerId::CONTROLLER_1, ButtonFlags::BUTTON_START );
	nesSystem.input.BindKey( '7', ControllerId::CONTROLLER_1, ButtonFlags::BUTTON_B );
	nesSystem.input.BindKey( '8', ControllerId::CONTROLLER_1, ButtonFlags::BUTTON_A );
}


BOOL InitInstance( HINSTANCE hInstance, int nCmdShow )
{
	hInst = hInstance;

	InitKeyBindings();

	int dwStyle = ( WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX /*| WS_THICKFRAME*/ );

	RECT wr = { 0, 0, view.defaultWidth, view.defaultHeight };
	AdjustWindowRect( &wr, dwStyle, TRUE );

	appDisplay.hWnd = CreateWindowW( szWindowClass, szTitle, dwStyle,
		CW_USEDEFAULT, CW_USEDEFAULT, ( wr.right - wr.left ), ( wr.bottom - wr.top ), nullptr, nullptr, hInstance, nullptr );

	if ( !appDisplay.hWnd )
	{
		return FALSE;
	}

	InitD3D12();
	InitImgui();
	CreateD3D12Pipeline();

	ShowWindow( appDisplay.hWnd, nCmdShow );
	UpdateWindow( appDisplay.hWnd );

	return TRUE;
}


void OpenNesGame()
{
	IFileOpenDialog* pFileOpen;

	// Create the FileOpenDialog object.
	HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
		IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen) );

	if ( !SUCCEEDED( hr ) )
		return;

	// Show the Open dialog box.
	hr = pFileOpen->Show(NULL);

	// Get the file name from the dialog box.
	if ( !SUCCEEDED( hr ) )
		return;
	
	IShellItem* pItem;
	hr = pFileOpen->GetResult( &pItem );

	if ( !SUCCEEDED( hr ) )
		return;

	PWSTR filePath = nullptr;
	hr = pItem->GetDisplayName( SIGDN_FILESYSPATH, &filePath );

	// Display the file name to the user.
	if ( !SUCCEEDED( hr ) )
		return;

	if( filePath == nullptr )
		return;
	
	nesFilePath = std::wstring( filePath );
	CoTaskMemFree( filePath );

	reset = true;
	pause = true;

	pItem->Release();

	pFileOpen->Release();
	CoUninitialize();
}


LRESULT CALLBACK WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
#ifdef IMGUI_ENABLE
	if ( ImGui_ImplWin32_WndProcHandler( hWnd, message, wParam, lParam ) )
		return true;
#endif

	switch ( message )
	{
	case WM_CREATE:
	{
		
	}
	break;

	case WM_COMMAND:
	{
		int wmId = LOWORD( wParam );
		// Parse the menu selections:
		switch ( wmId )
		{
		case IDM_ABOUT:
			DialogBox( hInst, MAKEINTRESOURCE( IDD_ABOUTBOX ), hWnd, About );
			break;
		case ID_FILE_OPEN:
			OpenNesGame();
			break;
		case ID_FILE_RESET:
			reset = true;
			break;
		case ID_FILE_LOADSTATE:
			systemConfig.sys.requestLoadState = true;
			break;
		case ID_FILE_SAVESTATE:
			systemConfig.sys.requestSaveState = true;
			break;
		case IDM_EXIT:
			DestroyWindow( hWnd );
			break;
		default:
			return DefWindowProc( hWnd, message, wParam, lParam );
		}
	}
	break;

	case WM_PAINT:
	{
		UpdateD3D12();

		IssueTextureCopyCommands();		
		BuildDrawCommandList();
		BuildImguiCommandList();
		ExecuteDrawCommands();

		ThrowIfFailed( swapChain.dxgi->Present( 1, 0 ) );
		AdvanceNextFrame();
	}
	break;

	case WM_LBUTTONDOWN:
	{
		POINT p;
		if ( GetCursorPos( &p ) )
		{
			//cursor position now in p.x and p.y
		}
		if ( ScreenToClient( hWnd, &p ) )
		{
			//p.x and p.y are now relative to hwnd's client area
		}

		RECT rc;
		GetClientRect( appDisplay.hWnd, &rc );

		const int32_t clientWidth = ( rc.right - rc.left );
		const int32_t clientHeight = ( rc.bottom - rc.top );
		const int32_t displayAreaX = view.displayScalar * view.nesWidth;
		const int32_t displayAreaY = clientHeight; // height minus title bar

		if( ( p.x >= 0 ) && ( p.x < displayAreaX ) && (p.y >= 0 ) && ( p.y < displayAreaY ) )
		{
			p.x /= view.displayScalar;
			p.y /= view.displayScalar;
			nesSystem.input.StoreMouseClick( wtPoint{ p.x, p.y } );
		}
	}
	break;

	case WM_LBUTTONUP:
	{
	//	ClearMouseClick();
	}
	break;

	case WM_SIZE:
	{
		InvalidateRect( hWnd, NULL, FALSE );
	}
	break;

	case WM_KEYDOWN:
	{
		const uint32_t capKey = toupper( (int)wParam );

		if( capKey == 'T' )
		{
			pause = true;
		}

		nesSystem.input.StoreKey( capKey );
	}
	break;

	case WM_KEYUP:
	{
		const uint32_t capKey = toupper( (int)wParam );

		if ( capKey == 'T' )
		{
			pause = false;
		}

		nesSystem.input.ReleaseKey( capKey );
	}
	break;

	case WM_DESTROY:
	{
#ifdef IMGUI_ENABLE
			ImGui_ImplDX12_Shutdown();
			ImGui_ImplWin32_Shutdown();
			ImGui::DestroyContext();
#endif

		DestroyD3D12();

		PostQuitMessage( 0 );
	}
	break;

	default:
		return DefWindowProc( hWnd, message, wParam, lParam );
	}
	return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam )
{
    UNREFERENCED_PARAMETER(lParam);
    switch ( message )
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if ( LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL )
        {
            EndDialog( hDlg, LOWORD(wParam) );
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}