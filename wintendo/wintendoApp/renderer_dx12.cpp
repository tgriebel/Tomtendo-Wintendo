#include "renderer_d3d12.h"
#include "audioEngine.h"

void wtRenderer::WaitForGpu()
{
	ThrowIfFailed( cmd.d3d12CommandQueue->Signal( sync.fence.Get(), sync.fenceValues[ currentFrameIx ] ) );

	ThrowIfFailed( sync.fence->SetEventOnCompletion( sync.fenceValues[ currentFrameIx ], sync.fenceEvent ) );
	WaitForSingleObjectEx( sync.fenceEvent, INFINITE, FALSE );

	sync.fenceValues[ currentFrameIx ]++;
}


void wtRenderer::AdvanceNextFrame()
{
	const UINT64 currentFence = sync.fenceValues[ currentFrameIx ];
	ThrowIfFailed( cmd.d3d12CommandQueue->Signal( sync.fence.Get(), currentFence ) );

	currentFrameIx = swapChain.dxgi->GetCurrentBackBufferIndex();

	if ( sync.fence->GetCompletedValue() < sync.fenceValues[ currentFrameIx ] )
	{
		ThrowIfFailed( sync.fence->SetEventOnCompletion( sync.fenceValues[ currentFrameIx ], sync.fenceEvent ) );
		WaitForSingleObject( sync.fenceEvent, INFINITE );
	//	PrintLog( "GPU WAIT.\n" );
	}

	sync.fenceValues[ currentFrameIx ] = currentFence + 1;
	frameNumber = sync.fenceValues[ currentFrameIx ];
}


uint32_t wtRenderer::InitD3D12()
{
	view.width = view.defaultWidth;
	view.height = view.defaultHeight;
	view.viewport = CD3DX12_VIEWPORT( 0.0f, 0.0f, static_cast<float>( view.width ), static_cast<float>( view.height ) );
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

	cmd.d3d12CopyQueue->SetName( L"Copy Queue" );
	cmd.d3d12CommandQueue->SetName( L"Draw Queue" );

	CreateFrameBuffers();

	D3D12_DESCRIPTOR_HEAP_DESC cbvSrvUavHeapDesc = {};
	cbvSrvUavHeapDesc.NumDescriptors = SHADER_RESOURES_COUNT;
	cbvSrvUavHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbvSrvUavHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed( d3d12device->CreateDescriptorHeap( &cbvSrvUavHeapDesc, IID_PPV_ARGS( &pipeline.cbvSrvUavHeap ) ) );
	pipeline.cbvSrvUavDescStride = d3d12device->GetDescriptorHandleIncrementSize( D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV );

	for ( uint32_t i = 0; i < FrameCount; ++i )
	{
		ThrowIfFailed( d3d12device->CreateCommandAllocator( D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS( &cmd.commandAllocator[ i ] ) ) );
		ThrowIfFailed( d3d12device->CreateCommandAllocator( D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS( &cmd.cpyCommandAllocator[ i ] ) ) );
		ThrowIfFailed( d3d12device->CreateCommandAllocator( D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS( &cmd.imguiCommandAllocator[ i ] ) ) );
	}

	InitImgui();
	CreateD3D12Pipeline();
	initD3D12 = true;

	return 0;
}


void wtRenderer::CreatePSO()
{
	D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
	featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

	if ( FAILED( d3d12device->CheckFeatureSupport( D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof( featureData ) ) ) )
	{
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}

	const uint32_t srvCnt = 1; // Only 1 texture is visible to ImGui at a time
	CD3DX12_DESCRIPTOR_RANGE1 ranges[ 2 ] = {};
	ranges[ 0 ].Init( D3D12_DESCRIPTOR_RANGE_TYPE_SRV, srvCnt, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE );
	ranges[ 1 ].Init( D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE );

	CD3DX12_ROOT_PARAMETER1 rootParameters[ 2 ] = {};
	rootParameters[ 0 ].InitAsDescriptorTable( 1, &ranges[ 0 ], D3D12_SHADER_VISIBILITY_PIXEL );
	rootParameters[ 1 ].InitAsDescriptorTable( 1, &ranges[ 1 ], D3D12_SHADER_VISIBILITY_PIXEL );

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
	psoDesc.InputLayout = { inputElementDescs, _countof( inputElementDescs ) };
	psoDesc.pRootSignature = pipeline.rootSig.Get();
	psoDesc.VS = CD3DX12_SHADER_BYTECODE( vertexShader.Get() );
	psoDesc.PS = CD3DX12_SHADER_BYTECODE( pixelShader.Get() );
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC( D3D12_DEFAULT );
	psoDesc.BlendState = CD3DX12_BLEND_DESC( D3D12_DEFAULT );
	psoDesc.DepthStencilState.DepthEnable = FALSE;
	psoDesc.DepthStencilState.StencilEnable = FALSE;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[ 0 ] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleDesc.Count = 1;
	ThrowIfFailed( d3d12device->CreateGraphicsPipelineState( &psoDesc, IID_PPV_ARGS( &pipeline.pso ) ) );
}


void wtRenderer::CreateCommandLists()
{
	for ( uint32_t i = 0; i < FrameCount; ++i )
	{
		ThrowIfFailed( d3d12device->CreateCommandList( 0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmd.commandAllocator[ i ].Get(), pipeline.pso.Get(), IID_PPV_ARGS( &cmd.commandList[ i ] ) ) );
		ThrowIfFailed( d3d12device->CreateCommandList( 0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmd.imguiCommandAllocator[ i ].Get(), pipeline.pso.Get(), IID_PPV_ARGS( &cmd.imguiCommandList[ i ] ) ) );
		ThrowIfFailed( d3d12device->CreateCommandList( 0, D3D12_COMMAND_LIST_TYPE_COPY, cmd.cpyCommandAllocator[ i ].Get(), pipeline.pso.Get(), IID_PPV_ARGS( &cmd.cpyCommandList[ i ] ) ) );

		ThrowIfFailed( cmd.commandList[ i ]->Close() );
		ThrowIfFailed( cmd.imguiCommandList[ i ]->Close() );
		ThrowIfFailed( cmd.cpyCommandList[ i ]->Close() );
	}
}


void wtRenderer::CreateConstantBuffers()
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

	pipeline.shaderData.hardPix = -3.0f;
	pipeline.shaderData.hardScan = -8.0f;
	pipeline.shaderData.maskDark = 0.9f;
	pipeline.shaderData.maskLight = 1.1f;
	pipeline.shaderData.warp = { 0.0f, 0.002f };
	pipeline.shaderData.imageDim = { 0.0f, 0.0f, view.nesWidth, view.nesHeight };
	pipeline.shaderData.destImageDim = { 0.0f, 0.0f, view.displayScalar * view.nesWidth, view.defaultHeight };
	pipeline.shaderData.enable = false;

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


void wtRenderer::CreateSyncObjects()
{
	ThrowIfFailed( d3d12device->CreateFence( 0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS( &sync.fence ) ) );
	ThrowIfFailed( d3d12device->CreateFence( 0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS( &sync.cpyFence ) ) );
	sync.fenceValues[ currentFrameIx ]++;

	sync.fenceEvent = CreateEvent( nullptr, FALSE, FALSE, nullptr );
	if ( sync.fenceEvent == nullptr )
	{
		ThrowIfFailed( HRESULT_FROM_WIN32( GetLastError() ) );
	}
}


void wtRenderer::CreateD3D12Pipeline()
{
	CreatePSO();
	CreateCommandLists();
	CreateVertexBuffers();
	for ( uint32_t i = 0; i < FrameCount; ++i ) {
		CreateTextureResources( i );
	}
	CreateConstantBuffers();
	CreateSyncObjects();
	WaitForGpu();
}


void wtRenderer::CreateVertexBuffers()
{
	const float x0 = NormalizeCoordinate( 0, view.defaultWidth );
	const float x1 = NormalizeCoordinate( view.displayScalar * PPU::ScreenWidth, view.defaultWidth );
	const float y0 = NormalizeCoordinate( 0, view.defaultHeight );
	const float y1 = NormalizeCoordinate( view.displayScalar * PPU::ScreenHeight, view.defaultHeight );

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


void wtRenderer::BuildDrawCommandList()
{
	ThrowIfFailed( cmd.commandAllocator[ currentFrameIx ]->Reset() );
	ThrowIfFailed( cmd.commandList[ currentFrameIx ]->Reset( cmd.commandAllocator[ currentFrameIx ].Get(), pipeline.pso.Get() ) );

	cmd.commandList[ currentFrameIx ]->SetGraphicsRootSignature( pipeline.rootSig.Get() );

	ID3D12DescriptorHeap* ppHeaps[] = { pipeline.cbvSrvUavHeap.Get() };
	cmd.commandList[ currentFrameIx ]->SetDescriptorHeaps( _countof( ppHeaps ), ppHeaps );
	cmd.commandList[ currentFrameIx ]->SetGraphicsRootDescriptorTable( 0, textureResources[ currentFrameIx ][ 0 ].gpuHandle );
	cmd.commandList[ currentFrameIx ]->SetGraphicsRootDescriptorTable( 1, pipeline.cbvSrvGpuHandle );

	cmd.commandList[ currentFrameIx ]->RSSetViewports( 1, &view.viewport );
	cmd.commandList[ currentFrameIx ]->RSSetScissorRects( 1, &view.scissorRect );

	D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition( swapChain.renderTargets[ currentFrameIx ].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET );
	cmd.commandList[ currentFrameIx ]->ResourceBarrier( 1, &barrier );

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle( swapChain.rtvHeap->GetCPUDescriptorHandleForHeapStart(), currentFrameIx, swapChain.rtvDescriptorSize );
	cmd.commandList[ currentFrameIx ]->OMSetRenderTargets( 1, &rtvHandle, FALSE, nullptr );

	const float clearColor[] = { 0.1f, 0.1f, 0.1f, 1.0f };
	cmd.commandList[ currentFrameIx ]->ClearRenderTargetView( rtvHandle, clearColor, 0, nullptr );
	cmd.commandList[ currentFrameIx ]->IASetPrimitiveTopology( D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
	cmd.commandList[ currentFrameIx ]->IASetVertexBuffers( 0, 1, &pipeline.vbView );
	cmd.commandList[ currentFrameIx ]->DrawInstanced( 6, 1, 0, 0 );

	barrier = CD3DX12_RESOURCE_BARRIER::Transition( swapChain.renderTargets[ currentFrameIx ].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT );
	cmd.commandList[ currentFrameIx ]->ResourceBarrier( 1, &barrier );

	ThrowIfFailed( cmd.commandList[ currentFrameIx ]->Close() );
}


void wtRenderer::ExecuteDrawCommands()
{
	ID3D12CommandList* ppCommandLists[] = { cmd.commandList[ currentFrameIx ].Get(), cmd.imguiCommandList[ currentFrameIx ].Get() };
	cmd.d3d12CommandQueue->Wait( sync.cpyFence.Get(), sync.fenceValues[ currentFrameIx ] );
	cmd.d3d12CommandQueue->ExecuteCommandLists( _countof( ppCommandLists ), ppCommandLists );
}


void wtRenderer::UpdateD3D12()
{
	memcpy( pipeline.pCbvDataBegin, &pipeline.shaderData, sizeof( pipeline.shaderData ) );
}


void wtRenderer::SubmitFrame()
{
	UpdateD3D12();

	BuildDrawCommandList();
	BuildImguiCommandList();
	ExecuteDrawCommands();

	ThrowIfFailed( swapChain.dxgi->Present( 1, 0 ) );
	AdvanceNextFrame();
}


void wtRenderer::CreateFrameBuffers()
{
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = { 0 };
	swapChainDesc.Width					= view.defaultWidth;
	swapChainDesc.Height				= view.defaultHeight;
	swapChainDesc.BufferCount			= FrameCount;
	swapChainDesc.Format				= DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.Stereo				= false;
	swapChainDesc.SampleDesc.Count		= 1;
	swapChainDesc.SampleDesc.Quality	= 0;
	swapChainDesc.BufferUsage			= DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.Scaling				= DXGI_SCALING_NONE;
	swapChainDesc.SwapEffect			= DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.Flags					= DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

	DXGI_SWAP_CHAIN_FULLSCREEN_DESC swapChainFullDesc;
	swapChainFullDesc.Windowed			= true;
	swapChainFullDesc.RefreshRate		= { 60, 1 };
	swapChainFullDesc.ScanlineOrdering	= DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapChainFullDesc.Scaling			= DXGI_MODE_SCALING_CENTERED;

	ComPtr<IDXGISwapChain1> tmpSwapChain;
	ThrowIfFailed( dxgiFactory->CreateSwapChainForHwnd( cmd.d3d12CommandQueue.Get(), hWnd, &swapChainDesc, &swapChainFullDesc, nullptr, &tmpSwapChain ) );

	ThrowIfFailed( tmpSwapChain.As( &swapChain.dxgi ) );
	currentFrameIx = swapChain.dxgi->GetCurrentBackBufferIndex();

	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = FrameCount;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ThrowIfFailed( d3d12device->CreateDescriptorHeap( &rtvHeapDesc, IID_PPV_ARGS( &swapChain.rtvHeap ) ) );

	swapChain.rtvDescriptorSize = d3d12device->GetDescriptorHandleIncrementSize( D3D12_DESCRIPTOR_HEAP_TYPE_RTV );

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle( swapChain.rtvHeap->GetCPUDescriptorHandleForHeapStart() );
	for ( uint32_t i = 0; i < FrameCount; ++i )
	{
		ThrowIfFailed( swapChain.dxgi->GetBuffer( i, IID_PPV_ARGS( &swapChain.renderTargets[ i ] ) ) );
		d3d12device->CreateRenderTargetView( swapChain.renderTargets[ i ].Get(), nullptr, rtvHandle );
		rtvHandle.Offset( 1, swapChain.rtvDescriptorSize );
	}
}


void wtRenderer::CreateTextureResources( const uint32_t frameIx )
{
	int dbgImageIx = 0;
	wtPoint sourceImages[ SHADER_RESOURES_TEXTURE_CNT ];
	sourceImages[ dbgImageIx++ ] = { PPU::ScreenWidth, PPU::ScreenHeight };
	sourceImages[ dbgImageIx++ ] = { 2 * PPU::NameTableWidthPixels, 2 * PPU::NameTableHeightPixels };
	sourceImages[ dbgImageIx++ ] = { 16, 2 };
	sourceImages[ dbgImageIx++ ] = { 128, 128 };
	sourceImages[ dbgImageIx++ ] = { 128, 128 };
	sourceImages[ dbgImageIx++ ] = { 8, 16 };

	for ( int i = 0; i < SHADER_RESOURES_CHRBANK_CNT; ++i )
	{
		sourceImages[ dbgImageIx + i ] = { 128, 128 };
	}

	const uint32_t textureCount = _countof( sourceImages );
	assert( textureCount <= SHADER_RESOURES_COUNT );

	textureResources[ frameIx ].clear();
	textureResources[ frameIx ].resize( SHADER_RESOURES_TEXTURE_CNT );

	const UINT cbvSrvUavHeapIncrement = d3d12device->GetDescriptorHandleIncrementSize( D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV );
	const CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHeapStart( pipeline.cbvSrvUavHeap->GetCPUDescriptorHandleForHeapStart() );
	const CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHeapStart( pipeline.cbvSrvUavHeap->GetGPUDescriptorHandleForHeapStart() );

	for ( uint32_t i = 0; i < textureCount; ++i )
	{
		textureResources[ frameIx ][ i ].desc.MipLevels = 1;
		textureResources[ frameIx ][ i ].desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		textureResources[ frameIx ][ i ].desc.Alignment = 0;
		textureResources[ frameIx ][ i ].desc.Width = sourceImages[ i ].x;
		textureResources[ frameIx ][ i ].desc.Height = sourceImages[ i ].y;
		textureResources[ frameIx ][ i ].desc.Flags = D3D12_RESOURCE_FLAG_NONE;
		textureResources[ frameIx ][ i ].desc.DepthOrArraySize = 1;
		textureResources[ frameIx ][ i ].desc.SampleDesc.Count = 1;
		textureResources[ frameIx ][ i ].desc.SampleDesc.Quality = 0;
		textureResources[ frameIx ][ i ].desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

		textureResources[ frameIx ][ i ].allocInfo = d3d12device->GetResourceAllocationInfo( 0, 1, &textureResources[ frameIx ][ i ].desc );

		D3D12_HEAP_PROPERTIES props = CD3DX12_HEAP_PROPERTIES( D3D12_HEAP_TYPE_DEFAULT );

		ThrowIfFailed( d3d12device->CreateCommittedResource(
			&props,
			D3D12_HEAP_FLAG_NONE,
			&textureResources[ frameIx ][ i ].desc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			IID_PPV_ARGS( &textureResources[ frameIx ][ i ].srv ) ) );

		wchar_t texName[ 128 ];
		swprintf_s( texName, 128, L"Texture #%i(%i)", i, frameIx );
		textureResources[ frameIx ][ i ].srv->SetName( texName );

		const UINT64 uploadBufferSize = GetRequiredIntermediateSize( textureResources[ frameIx ][ i ].srv.Get(), 0, 1 );

		props = CD3DX12_HEAP_PROPERTIES( D3D12_HEAP_TYPE_UPLOAD );
		D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer( uploadBufferSize );

		ThrowIfFailed( d3d12device->CreateCommittedResource(
			&props,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS( &textureResources[ frameIx ][ i ].uploadBuffer ) ) );

		swprintf_s( texName, 128, L"Upload Texture #%i(%i)", i, frameIx );
		textureResources[ frameIx ][ i ].srv->SetName( texName );

		textureResources[ frameIx ][ i ].cpuHandle.InitOffsetted( cpuHeapStart, i + SHADER_RESOURES_NAMETABLE, cbvSrvUavHeapIncrement );
		textureResources[ frameIx ][ i ].gpuHandle.InitOffsetted( gpuHeapStart, i + SHADER_RESOURES_NAMETABLE, cbvSrvUavHeapIncrement );
		textureResources[ frameIx ][ i ].width = sourceImages[ i ].x;
		textureResources[ frameIx ][ i ].height = sourceImages[ i ].y;

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = textureResources[ frameIx ][ i ].desc.Format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;

		d3d12device->CreateShaderResourceView( textureResources[ frameIx ][ i ].srv.Get(), &srvDesc, textureResources[ frameIx ][ i ].cpuHandle );
	}
}


void wtRenderer::InitImgui()
{
#ifdef IMGUI_ENABLE
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;

	ImGui::StyleColorsDark();

	ImGui_ImplWin32_Init( hWnd );
	ImGui_ImplDX12_Init( d3d12device.Get(), FrameCount,
		DXGI_FORMAT_R8G8B8A8_UNORM, pipeline.cbvSrvUavHeap.Get(),
		pipeline.cbvSrvUavHeap.Get()->GetCPUDescriptorHandleForHeapStart(),
		pipeline.cbvSrvUavHeap.Get()->GetGPUDescriptorHandleForHeapStart() );
#endif
}


void wtRenderer::DestroyD3D12()
{
	WaitForGpu();

#ifdef IMGUI_ENABLE
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
#endif

	CloseHandle( sync.fenceEvent );

	initD3D12 = false;
}


bool wtRenderer::NeedsResize( const uint32_t width, const uint32_t height )
{
	if ( !initD3D12 ) {
		return false;
	}

	return ( width != view.width || height != view.height );
}


void wtRenderer::RecreateSwapChain( const uint32_t width, const uint32_t height )
{
	if( !initD3D12 ) {
		return;
	}

	// Flush all current GPU commands.
	WaitForGpu();

	for ( uint32_t i = 0; i < FrameCount; i++ )
	{
		swapChain.renderTargets[ i ].Reset();
		sync.fenceValues[ i ] = sync.fenceValues[ currentFrameIx ];
	}

	// Resize the swap chain to the desired dimensions.
	DXGI_SWAP_CHAIN_DESC desc = {};
	swapChain.dxgi->GetDesc( &desc );
	ThrowIfFailed( swapChain.dxgi->ResizeBuffers( FrameCount, width, height, desc.BufferDesc.Format, desc.Flags ) );

	// Reset the frame index to the current back buffer index.
	currentFrameIx = swapChain.dxgi->GetCurrentBackBufferIndex();

	view.width = width;
	view.height = height;
	view.viewport = CD3DX12_VIEWPORT( 0.0f, 0.0f, static_cast<float>( width ), static_cast<float>( height ) );
	view.scissorRect = CD3DX12_RECT( 0, view.overscanY0, width, height );

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle( swapChain.rtvHeap->GetCPUDescriptorHandleForHeapStart() );
	for ( uint32_t i = 0; i < FrameCount; ++i )
	{
		ThrowIfFailed( swapChain.dxgi->GetBuffer( i, IID_PPV_ARGS( &swapChain.renderTargets[ i ] ) ) );
		d3d12device->CreateRenderTargetView( swapChain.renderTargets[ i ].Get(), nullptr, rtvHandle );
		rtvHandle.Offset( 1, swapChain.rtvDescriptorSize );
	}
}


void wtRenderer::IssueTextureCopyCommands( const uint32_t srcFrameIx, const uint32_t renderFrameIx )
{
	ThrowIfFailed( cmd.cpyCommandAllocator[ renderFrameIx ]->Reset() );
	ThrowIfFailed( cmd.cpyCommandList[ renderFrameIx ]->Reset( cmd.cpyCommandAllocator[ renderFrameIx ].Get(), pipeline.pso.Get() ) );

	const uint32_t textureCount = static_cast<uint32_t>( textureResources[ renderFrameIx ].size() );
	std::vector<D3D12_SUBRESOURCE_DATA> textureData( textureCount );

	wtFrameResult* fr = &app->frameResult[ srcFrameIx ];

	uint32_t imageIx = 0;
	const wtRawImageInterface* sourceImages[ SHADER_RESOURES_TEXTURE_CNT ];
	// All that this needs right now are the dimensions.
	sourceImages[ imageIx++ ] = fr->frameBuffer;
	sourceImages[ imageIx++ ] = fr->nameTableSheet;
	sourceImages[ imageIx++ ] = fr->paletteDebug;
	sourceImages[ imageIx++ ] = fr->patternTable0;
	sourceImages[ imageIx++ ] = fr->patternTable1;
	sourceImages[ imageIx++ ] = fr->pickedObj8x16;

	for ( int i = 0; i < SHADER_RESOURES_CHRBANK_CNT; ++i )
	{
		sourceImages[ imageIx + i ] = &app->debugData.chrRom[ i ];
	}

	const uint32_t texturePixelSize = 4;

	for ( uint32_t i = 0; i < textureCount; ++i )
	{
		D3D12_SUBRESOURCE_DATA textureData;
		textureData.pData = sourceImages[ i ]->GetRawBuffer();
		textureData.RowPitch = textureResources[ renderFrameIx ][ i ].width * static_cast<uint64_t>( texturePixelSize );
		textureData.SlicePitch = textureData.RowPitch * textureResources[ renderFrameIx ][ i ].height;

		D3D12_PLACED_SUBRESOURCE_FOOTPRINT pLayouts = {};
		pLayouts.Offset = 0;
		pLayouts.Footprint.Depth = 1;
		pLayouts.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		pLayouts.Footprint.Width = textureResources[ renderFrameIx ][ i ].width;
		pLayouts.Footprint.Height = textureResources[ renderFrameIx ][ i ].height;
		pLayouts.Footprint.RowPitch = static_cast<uint32_t>( textureData.RowPitch );

		D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition( textureResources[ renderFrameIx ][ i ].srv.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST );
		cmd.cpyCommandList[ renderFrameIx ]->ResourceBarrier( 1, &barrier );

		// TODO: use only one intermediate buffer
		UpdateSubresources( cmd.cpyCommandList[ renderFrameIx ].Get(), textureResources[ renderFrameIx ][ i ].srv.Get(), textureResources[ renderFrameIx ][ i ].uploadBuffer.Get(), 0, 0, 1, &textureData );
	
		barrier = CD3DX12_RESOURCE_BARRIER::Transition( textureResources[ renderFrameIx ][ i ].srv.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON );
		cmd.cpyCommandList[ renderFrameIx ]->ResourceBarrier( 1, &barrier );
	}

	ThrowIfFailed( cmd.cpyCommandList[ renderFrameIx ]->Close() );

	ID3D12CommandList* ppCommandLists[] = { cmd.cpyCommandList[ renderFrameIx ].Get() };
	cmd.d3d12CopyQueue->ExecuteCommandLists( 1, ppCommandLists );
	cmd.d3d12CopyQueue->Signal( sync.cpyFence.Get(), sync.fenceValues[ renderFrameIx ] );
}