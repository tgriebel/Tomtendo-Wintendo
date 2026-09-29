/*
* MIT License
*
* Copyright( c ) 2017-2021 Thomas Griebel
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this softwareand associated documentation files( the "Software" ), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions :
*
* The above copyright noticeand this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/

#include <windows.h>
#include <xaudio2.h>
#include "stdafx.h"
#include "wintendoApp.h"
#include "renderer_d3d12.h"
#include "audioEngine.h"
#include <tomtendo\command.h>

#ifdef NDEBUG
#undef assert
#define assert(x) if( !(x) ) { PrintLog(#x##"\n"); }
#endif

HINSTANCE hInst;                                // current instance
const WCHAR* szTitle = L"TomTendo!";
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

#if _DEBUG
static const uint32_t MaxSpinTime = 1;
#else
static const uint32_t MaxSpinTime = 100;
#endif

std::wstring nesFilePath( L"Games/Legend of Zelda.nes" );

wtAppInterface		app;
wtRenderer			r;
wtAudioEngine		audio;
Tomtendo::Emulator	sys;

static const bool singleRenderThread = false; // Temp test variable

// Forward declare message handler from imgui_impl_win32.cpp
#ifdef IMGUI_ENABLE
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );
#endif

void WaitOnThread( const HANDLE& threadSemaphore, const uint32_t spinTime, const uint32_t waitMs )
{
	while ( true )
	{
		const DWORD result = WaitForSingleObject( threadSemaphore, spinTime );
		if ( ( result == WAIT_OBJECT_0 ) || ( waitMs == 0 ) ) {
			break;
		}
		Sleep( waitMs );
	}
}


void SignalThread( HANDLE threadSemaphore, const uint32_t releaseCount )
{
	ReleaseSemaphore( threadSemaphore, releaseCount, NULL );
}


DWORD WINAPI EmulatorThread( LPVOID lpParameter )
{
	using namespace Tomtendo;

	Emulator&		nes				= *app.system;
	config_t&		systemConfig	= app.systemConfig;
	wtAppDebug_t&	debugData		= app.debugData;

	static timePoint_t previousTime;

	while( app.IsEmulatorRunning() )
	{
		const uint32_t waitFrame = app.frameIx;
		if ( app.IsResetPending() )
		{
			WaitOnThread( app.frameSubmitReadLock, MaxSpinTime, 1 );

			for( uint32_t i = 0; i < FrameResultCount; ++i ) {			
				WaitOnThread( app.audioReadLock[ i ], MaxSpinTime, 1 );
			}
			WaitOnThread( app.workerLock, MaxSpinTime, 0 );

			app.systemConfig = DefaultConfig();

			Boot( &nes, nesFilePath.c_str() );
			SetConfig( &nes, systemConfig );
			GenerateRomDissambly( &nes, debugData.prgRomAsm );
			GenerateChrRomTables( &nes, debugData.chrRom );

			app.AcknowledgeReset();
			app.EnableEmulation();
			app.frameIx = 0;
			r.frameResultIx = 0;
			r.lastFrameDrawn = 0;
			audio.emulatorFrame = 0;

			previousTime = std::chrono::steady_clock::now();

			for ( uint32_t i = 0; i < FrameResultCount; ++i )
			{
				SignalThread( app.frameSubmitReadLock, 1 );
				SignalThread( app.audioReadLock[ i ], 1 );
			}
			SignalThread( app.workerLock, 1 );

			PrintLog( "Reset" );
		}
		else
		{
			WaitOnThread( app.frameSubmitReadLock, MaxSpinTime, 0 );
			WaitOnThread( app.audioReadLock[ waitFrame ], MaxSpinTime, 0 );
		}

		if ( app.IsEmulatorEnabled() )
		{
			SetConfig( &nes, systemConfig );

			const timePoint_t currentTime = std::chrono::steady_clock::now();
			const std::chrono::nanoseconds elapsed = ( currentTime - previousTime );
			previousTime = currentTime;

			if( !RunEpoch( &nes, elapsed ) ) {
				app.DisableEmulation();
			}

			app.frameIx = ( app.frameIx + 1 ) % FrameCount;
		}

		wtFrameResult& fr = app.frameResult[ waitFrame ];
			
		app.t.copyTimer.Start();
		GetFrameResult( &nes, fr );
		app.t.copyTime = app.t.copyTimer.GetElapsedMs();

		if( singleRenderThread )
		{
			r.IssueTextureCopyCommands( waitFrame, r.currentFrameIx );
			r.SubmitFrame();
		}

		SignalThread( app.frameSubmitWriteLock, 1 );
		SignalThread( app.audioWriteLock[ waitFrame ], 1 );

		app.t.elapsedCopyTime = app.t.elapsedCopyTimer.GetElapsedMs();
		app.t.elapsedCopyTimer.Start();
	}

	return 0;
}


DWORD WINAPI RenderThread( LPVOID lpParameter )
{
	using namespace Tomtendo;

	r.app = &app;
	r.InitD3D12();

	while ( app.IsEmulatorRunning() )
	{
		const uint32_t waitFrame = r.frameResultIx;	
		wtFrameResult* fr = &app.frameResult[ waitFrame ];
		if( ( fr->currentFrame > 1 ) && ( fr->frameBuffer != nullptr ) )
		{
			if ( !singleRenderThread )
			{
				WaitOnThread( app.frameSubmitWriteLock, MaxSpinTime, 0 );
				if( r.NeedsResize( app.clientWidth, app.clientHeight ) )
				{
					app.DisableEmulation();
					r.RecreateSwapChain( app.clientWidth, app.clientHeight );
					InvalidateRect( r.hWnd, NULL, TRUE );
					app.EnableEmulation();
				}
				r.IssueTextureCopyCommands( waitFrame, r.currentFrameIx );
				SignalThread( app.frameSubmitReadLock, 1 );

				r.SubmitFrame();
			}
			r.lastFrameDrawn = fr->dbgFrameBufferIx;
			r.frameResultIx = ( r.frameResultIx + 1 ) % FrameResultCount;
		}		
	}
	r.DestroyD3D12();

	return 0;
}


DWORD WINAPI AudioThread( LPVOID lpParameter )
{
	using namespace Tomtendo;

	audio.Init();

	while ( app.IsEmulatorRunning() )
	{
		if( !audio.enableSound || !app.IsEmulatorEnabled() )
		{
			if( audio.pSourceVoice != nullptr )
			{
				audio.pSourceVoice->FlushSourceBuffers();
				audio.pSourceVoice->Stop();
				audio.audioStopped = true;
				voiceCallback.totalQueues = 0;
				voiceCallback.totalDuration = 0;
				voiceCallback.processedQueues = 0;
			}
			Sleep( 1000 );
		}
		else if( audio.audioStopped )
		{
			audio.pSourceVoice->Start( 0, XAUDIO2_COMMIT_ALL );
			audio.audioStopped = false;
		}

		const uint32_t waitFrame = app.frameIx;
		wtFrameResult& fr = app.frameResult[ waitFrame ];

		const bool log = app.audio->logSnd && !( waitFrame % 60 );
		if ( log ) {
			LogApu( fr );
		}

		WaitOnThread( app.audioWriteLock[ waitFrame ], MaxSpinTime, 0 );
		if( fr.soundOutput != nullptr )
		{
			audio.EncodeSamples( fr.soundOutput->mixed );
			++audio.emulatorFrame;
		}
		audio.frameResultIx = ( audio.frameResultIx + 1 ) % FrameResultCount;

		SignalThread( app.audioReadLock[ waitFrame ], 1 );
		
		if( audio.AudioSubmit() )
		{
			app.t.audioSubmitTime = app.t.audioSubmitTimer.GetElapsedMs();
			app.t.audioSubmitTimer.Start();
		}
	}

	return 0;
}


DWORD WINAPI WorkerThread( LPVOID lpParameter )
{
	using namespace Tomtendo;

	Emulator& nes = *app.system;

	while( true )
	{
		WaitOnThread( app.workerLock, MaxSpinTime, 0 );

		wtFrameResult& fr = app.frameResult[ r.frameResultIx ];
		if ( ( fr.dbgLog != nullptr ) && !app.logUnpacked && fr.dbgLog->IsFinished() )
		{
			app.traceLog.resize( 0 );
			app.traceLog.reserve( 400 * fr.dbgLog->GetRecordCount() ); // Assume 400 characters per log line
			fr.dbgLog->ToString( app.traceLog, 0, fr.dbgLog->GetRecordCount(), true );
			app.logUnpacked = true;
		}

		if( ( fr.currentFrame > 0 ) && app.IsEmulatorEnabled() )
		{
			UpdateDebugImages( &nes );

			if ( app.refreshChrRomTables )
			{
				wtAppDebug_t& debugData = app.debugData;
				GenerateChrRomTables( &nes, debugData.chrRom );
				app.refreshChrRomTables = false;
			}
		}

		SignalThread( app.workerLock, 1 );
		
		Sleep( 1000 );
	}
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

	app.r				= &r;
	app.audio			= &audio;
	app.system			= &sys;

	app.t.runTime.Start();

	const uint32_t threadCount = 4;
	DWORD threadIDs[ threadCount ];
	HANDLE threadHdls[ threadCount ];
	threadHdls[ 0 ] = CreateThread( 0, 0, EmulatorThread,	&sharedData, 0, &threadIDs[ 0 ] );
	threadHdls[ 1 ] = CreateThread( 0, 0, RenderThread,		&sharedData, 0, &threadIDs[ 1 ] );
	threadHdls[ 2 ] = CreateThread( 0, 0, AudioThread,		&sharedData, 0, &threadIDs[ 2 ] );
	threadHdls[ 3 ] = CreateThread( 0, 0, WorkerThread,		&sharedData, 0, &threadIDs[ 3 ] );

	for( uint32_t i = 0; i < threadCount; ++i )
	{
		if ( threadHdls[ i ] == nullptr )
		{
			PrintLog( "Thread creation failed.\n" );
			return 0;
		}
	}

    LoadStringW( hInstance, IDC_WINTENDOAPP, szWindowClass, MAX_LOADSTRING );
	RegisterWindow( hInstance, szWindowClass );

    if ( !InitAppInstance ( hInstance, nCmdShow ) )
    {
		PrintLog( "Window creation failed.\n" );
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators( hInstance, MAKEINTRESOURCE( IDC_WINTENDOAPP ) );

    MSG msg;
	while ( GetMessage( &msg, nullptr, 0, 0 ) )
	{
		if ( !TranslateAccelerator( msg.hwnd, hAccelTable, &msg ) )
		{
			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}
	}

	Shutdown( app.system );
	app.TerminateEmulator();
	for ( uint32_t i = 0; i < threadCount; ++i )
	{
		DWORD exitCode;
		WaitForSingleObject( threadHdls[ i ], 1000 );
		TerminateThread( threadHdls[ i ], GetExitCodeThread( threadHdls[ i ], &exitCode ) );
		CloseHandle( threadHdls[ i ] );
	}

    return (int)msg.wParam;
}


void InitKeyBindings()
{
	using namespace Tomtendo;

	Input& input = app.system->input;

	BindKey( &input, 'A', ControllerId::CONTROLLER_0, ButtonFlags::BUTTON_LEFT );
	BindKey( &input, 'D', ControllerId::CONTROLLER_0, ButtonFlags::BUTTON_RIGHT );
	BindKey( &input, 'W', ControllerId::CONTROLLER_0, ButtonFlags::BUTTON_UP );
	BindKey( &input, 'S', ControllerId::CONTROLLER_0, ButtonFlags::BUTTON_DOWN );
	BindKey( &input, 'G', ControllerId::CONTROLLER_0, ButtonFlags::BUTTON_SELECT );
	BindKey( &input, 'H', ControllerId::CONTROLLER_0, ButtonFlags::BUTTON_START );
	BindKey( &input, 'J', ControllerId::CONTROLLER_0, ButtonFlags::BUTTON_B );
	BindKey( &input, 'K', ControllerId::CONTROLLER_0, ButtonFlags::BUTTON_A );

	BindKey( &input, '1', ControllerId::CONTROLLER_1, ButtonFlags::BUTTON_LEFT );
	BindKey( &input, '2', ControllerId::CONTROLLER_1, ButtonFlags::BUTTON_RIGHT );
	BindKey( &input, '3', ControllerId::CONTROLLER_1, ButtonFlags::BUTTON_UP );
	BindKey( &input, '4', ControllerId::CONTROLLER_1, ButtonFlags::BUTTON_DOWN );
	BindKey( &input, '5', ControllerId::CONTROLLER_1, ButtonFlags::BUTTON_SELECT );
	BindKey( &input, '6', ControllerId::CONTROLLER_1, ButtonFlags::BUTTON_START );
	BindKey( &input, '7', ControllerId::CONTROLLER_1, ButtonFlags::BUTTON_B );
	BindKey( &input, '8', ControllerId::CONTROLLER_1, ButtonFlags::BUTTON_A );
}


BOOL InitAppInstance( HINSTANCE hInstance, int nCmdShow )
{
	hInst = hInstance;

	InitKeyBindings();

	int dwStyle = ( WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME );

	RECT wr = { 0, 0, r.view.DefaultWidth(), r.view.DefaultHeight() };
	AdjustWindowRect( &wr, dwStyle, TRUE );

	r.hWnd = CreateWindowW( szWindowClass, szTitle, dwStyle,
		CW_USEDEFAULT, CW_USEDEFAULT, ( wr.right - wr.left ), ( wr.bottom - wr.top ), nullptr, nullptr, hInstance, nullptr );

	if ( !r.hWnd )
	{
		return FALSE;
	}

	ShowWindow( r.hWnd, nCmdShow );
	UpdateWindow( r.hWnd );

	return TRUE;
}


LRESULT CALLBACK WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	using namespace Tomtendo;
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
			OpenNesGame( nesFilePath );
			app.RequestReset();
			app.DisableEmulation();
			break;
		case ID_FILE_RESET:
			app.RequestReset();
			break;
		case ID_FILE_LOADSTATE:
			{
				sysCmd_t traceCmd;
				traceCmd.type = sysCmdType_t::LOAD_STATE;
				SubmitCommand( app.system, traceCmd );
			}
			break;
		case ID_FILE_SAVESTATE:
			{
				sysCmd_t traceCmd;
				traceCmd.type = sysCmdType_t::SAVE_STATE;
				SubmitCommand( app.system, traceCmd );
			}
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
		GetClientRect( r.hWnd, &rc );

		const int32_t clientWidth = ( rc.right - rc.left );
		const int32_t clientHeight = ( rc.bottom - rc.top );
		const int32_t displayAreaX = r.view.displayScalar * ScreenWidth();
		const int32_t displayAreaY = clientHeight; // height minus title bar

		if( ( p.x >= 0 ) && ( p.x < displayAreaX ) && (p.y >= 0 ) && ( p.y < displayAreaY ) )
		{
			p.x /= r.view.displayScalar;
			p.y /= r.view.displayScalar;
			StoreMouseClick( &app.system->input, p.x, p.y );
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
		const int32_t width = LOWORD( lParam );
		const int32_t height = HIWORD( lParam );
		const int32_t sizeType = static_cast< int32_t >( wParam );

		if( sizeType != SIZE_MINIMIZED )
		{
			app.clientWidth = width;
			app.clientHeight = height;
		}
	}
	break;

	case WM_KEYDOWN:
	{
		const uint32_t capKey = toupper( (int)wParam );
		StoreKey( &app.system->input, capKey );
	}
	break;

	case WM_KEYUP:
	{
		const uint32_t capKey = toupper( (int)wParam );
		if ( capKey == 'T' ) {
			app.ToggleEmulation();
		}

		ReleaseKey( &app.system->input, capKey );
	}
	break;

	case WM_DESTROY:
	{
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