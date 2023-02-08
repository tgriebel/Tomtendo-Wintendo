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

static inline std::wstring GetAssetFullPath( LPCWSTR assetName )
{
	std::wstring shaderPath( L"Shaders/" );
	return shaderPath + assetName;
}

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


struct wtAppTextureD3D12
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE	cpuHandle;
	CD3DX12_GPU_DESCRIPTOR_HANDLE	gpuHandle;

	ComPtr<ID3D12Resource>			srv;
	ComPtr<ID3D12Resource>			uploadBuffer;

	uint32_t						width;
	uint32_t						height;
	D3D12_RESOURCE_ALLOCATION_INFO	allocInfo;
	D3D12_RESOURCE_DESC				desc;
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
	static const int32_t			debugAreaX = 1024;
	static const int32_t			debugAreaY = 0;
	CD3DX12_VIEWPORT				viewport;
	CD3DX12_RECT					scissorRect;
	uint32_t						width;
	uint32_t						height;

	inline int32_t DefaultWidth()
	{
		return view_t::displayScalar * Tomtendo::ScreenWidth() + view_t::debugAreaX;
	}


	inline int32_t DefaultHeight()
	{
		return view_t::displayScalar * Tomtendo::ScreenHeight() + view_t::debugAreaY;
	}


	inline int32_t OverscanY0()
	{
		return view_t::displayScalar * 8;
	}


	inline int32_t OverscanY1()
	{
		return view_t::displayScalar * ( Tomtendo::ScreenHeight() - 8 );
	}
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


class wtRenderer
{
private:
	ComPtr<IDXGIAdapter1>					dxgiAdapter;
	ComPtr<IDXGIFactory4>					dxgiFactory;
	ComPtr<ID3D12Device>					d3d12device;

	swapChain_t								swapChain;
	pipeline_t								pipeline;
	command_t								cmd;

	std::vector<wtAppTextureD3D12>			textureResources[ FrameCount ];

public:

	wtRenderer()
	{
		currentFrameIx = 0;
		frameResultIx = 0;
		frameNumber = 0;
		lastFrameDrawn = 0;
		initD3D12 = false;
	}

	bool									initD3D12;
	uint32_t								currentFrameIx;
	uint32_t								frameResultIx;
	uint64_t								frameNumber;
	uint64_t								lastFrameDrawn;
	view_t									view;
	sync_t									sync = { 0 };
	HWND									hWnd;
	wtAppInterface*							app;

	void									WaitForGpu();
	void									AdvanceNextFrame();

	void									CreateFrameBuffers();
	void									CreatePSO();
	void									CreateCommandLists();
	void									CreateVertexBuffers();
	void									CreateConstantBuffers();
	void									CreateTextureResources( const uint32_t frameIx );
	void									CreateSyncObjects();
	void									CreateD3D12Pipeline();

	uint32_t								InitD3D12();
	void									InitImgui();
	void									UpdateD3D12();
	void									DestroyD3D12();

	bool									NeedsResize( const uint32_t width, const uint32_t height );
	void									RecreateSwapChain( const uint32_t width, const uint32_t height );

	void									IssueTextureCopyCommands( const uint32_t srcFrameIx, const uint32_t renderFrameIx );
	void									BuildImguiCommandList();
	void									BuildDrawCommandList();
	void									ExecuteDrawCommands();
	void									SubmitFrame();
};