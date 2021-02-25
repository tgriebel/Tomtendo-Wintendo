#pragma once

#include <windows.h>
#include <direct.h>
#include <ole2.h>
#include <ObjBase.h>
#include <comdef.h>
#include <queue>
#include <algorithm>
#include "stdafx.h"
#include "wintendoApp.h"
#include "wintendoApp_dx12.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

static const uint32_t				FrameCount = 2;
static const uint32_t				FrameResultCount = FrameCount;//( FrameCount + 1 );

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

	const wtRawImageInterface* srcImage;
	D3D12_RESOURCE_ALLOCATION_INFO	allocInfo;
	D3D12_RESOURCE_DESC				desc;
};


struct wtAppDisplay
{
	HWND hWnd;
};


struct swapChain_t
{
	ComPtr<IDXGISwapChain3>			dxgi;
	ComPtr<ID3D12Resource>			renderTargets[ FrameCount ];
	ComPtr<ID3D12DescriptorHeap>	rtvHeap;
	uint32_t						rtvDescriptorSize;
};


struct view_t
{
	static const int32_t			displayScalar = 2;
	static const int32_t			nesWidth = PPU::ScreenWidth;
	static const int32_t			nesHeight = PPU::ScreenHeight;
	static const int32_t			overscanY0 = displayScalar * 8;
	static const int32_t			overscanY1 = displayScalar * ( PPU::ScreenHeight - 8 );
	static const int32_t			debugAreaX = 1024;
	static const int32_t			debugAreaY = 0;
	static const int32_t			defaultWidth = displayScalar * nesWidth + debugAreaX;
	static const int32_t			defaultHeight = displayScalar * nesHeight + debugAreaY;
	CD3DX12_VIEWPORT				viewport;
	CD3DX12_RECT					scissorRect;
};


struct pipeline_t
{
	ComPtr<ID3D12Resource>			cb;
	DisplayConstantBuffer			shaderData;
	ComPtr<ID3D12Resource>			vb;
	D3D12_VERTEX_BUFFER_VIEW		vbView;
	UINT8* pCbvDataBegin;
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
	HANDLE								frameCopySemaphore[ FrameCount ];
	HANDLE								audioCopySemaphore[ FrameCount ];
};


enum ShaderResources
{
	SHADER_RESOURES_FRAMEBUFFER,
	SHADER_RESOURES_NAMETABLE,
	SHADER_RESOURES_PALETTE,
	SHADER_RESOURES_PATTERN0,
	SHADER_RESOURES_PATTERN1,
	SHADER_RESOURES_OBJECT,
	SHADER_RESOURES_CHRBANK0,
	SHADER_RESOURES_CHRBANK_CNT = 32,

	SHADER_RESOURES_TEXTURE_CNT = SHADER_RESOURES_CHRBANK0 + SHADER_RESOURES_CHRBANK_CNT,
	SHADER_RESOURES_CBV0 = SHADER_RESOURES_TEXTURE_CNT,
	SHADER_RESOURES_COUNT,
};