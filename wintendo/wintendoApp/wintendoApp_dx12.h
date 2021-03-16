#pragma once

#include <d2d1.h>
#include <dxgi.h>
#include <dxgi1_2.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>

// D3D12 extension library.
#include "d3dx12.h" // https://github.com/Microsoft/DirectX-Graphics-Samples/tree/master/Libraries/D3DX12

#pragma comment(lib, "d2d1")
#pragma comment(lib, "DXGI")
#pragma comment(lib, "D3D12")
#pragma comment(lib, "D3DCompiler")

static inline void SetName( ID3D12Object* pObject, LPCWSTR name )
{
	pObject->SetName( name );
}

#define NAME_D3D12_OBJECT(x) SetName((x).Get(), L#x)
#define NAME_D3D12_OBJECT_INDEXED(x, n) SetNameIndexed((x)[n].Get(), L#x, n)

static inline  std::string HrToString( HRESULT hr )
{
	char s_str[64] = {};
	sprintf_s( s_str, "HRESULT of 0x%08X", static_cast<UINT>( hr ) );
	return std::string( s_str );
}


class HrException : public std::runtime_error
{
public:
	HrException( HRESULT hr ) : std::runtime_error( HrToString( hr ) ), m_hr( hr ) {}
	HRESULT Error() const { return m_hr; }
private:
	const HRESULT m_hr;
};


static inline void ThrowIfFailed( HRESULT hr )
{
	if ( FAILED( hr ) )
	{
		throw HrException( hr );
	}
}


template <class T>
static inline void SafeRelease( T** ppT )
{
    if ( *ppT )
    {
        ( *ppT )->Release();
        *ppT = nullptr;
    }
}

// https://docs.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-d3d12createdevice
static inline void GetHardwareAdapter( IDXGIFactory4* pFactory, IDXGIAdapter1** ppAdapter )
{
    *ppAdapter = nullptr;
    for ( UINT adapterIndex = 0; ; ++adapterIndex )
    {
        IDXGIAdapter1* pAdapter = nullptr;
        if ( DXGI_ERROR_NOT_FOUND == pFactory->EnumAdapters1( adapterIndex, &pAdapter ) )
        {
            // No more adapters to enumerate.
            break;
        }

        // Check to see if the adapter supports Direct3D 12, but don't create the
        // actual device yet.
        if ( SUCCEEDED( D3D12CreateDevice( pAdapter, D3D_FEATURE_LEVEL_11_0, _uuidof( ID3D12Device ), nullptr ) ) )
        {
            *ppAdapter = pAdapter;
            return;
        }
        pAdapter->Release();
    }
}