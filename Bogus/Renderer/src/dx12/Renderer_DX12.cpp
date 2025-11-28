#include "Renderer.h"
#include "assert.h"
#include "d3d12.h"
#include "dxgi1_4.h"

namespace Bogus::Renderer
{

static ID3D12Device2* g_Device;
static ID3D12CommandQueue* g_CommandQueue;

// static internals decls
static void GetHardwareAdapter( IDXGIFactory4* pFactory, IDXGIAdapter1** ppOutAdapter );
static void CreateDevice( IDXGIAdapter1* pAdapter, ID3D12Device2** ppOutDevice );
static void CreateCommandQueue( ID3D12Device2* pDevice, D3D12_COMMAND_LIST_TYPE type,
                                ID3D12CommandQueue** ppOutQueue );
// static internals decls

void Initialize()
{
    IDXGIFactory4* pFactory;
    HRESULT hr = CreateDXGIFactory2( 0, IID_PPV_ARGS( &pFactory ) );
    if( FAILED( hr ) )
    {
        assert( 0 && "Failed to create pFactory" );
        return;
    }

    IDXGIAdapter1* pAdapter;
    GetHardwareAdapter( pFactory, &pAdapter );

    CreateDevice( pAdapter, &g_Device );
    CreateCommandQueue( g_Device, D3D12_COMMAND_LIST_TYPE_DIRECT, &g_CommandQueue );
}

void Draw() {}

static void GetHardwareAdapter( IDXGIFactory4* pFactory, IDXGIAdapter1** ppOutAdapter )
{
    *ppOutAdapter = nullptr;
    for( UINT adapterIndex = 0;; ++adapterIndex )
    {
        IDXGIAdapter1* pAdapter = nullptr;
        if( DXGI_ERROR_NOT_FOUND == pFactory->EnumAdapters1( adapterIndex, &pAdapter ) )
        {
            assert( 0 && "No more adapters to enumerate." );
            break;
        }

        // Check to see if the adapter supports Direct3D 12, but don't create the
        // actual device yet.
        if( SUCCEEDED( D3D12CreateDevice( pAdapter, D3D_FEATURE_LEVEL_11_0, IID_ID3D12Device2,
                                          nullptr ) ) )
        {
            *ppOutAdapter = pAdapter;
            return;
        }
        pAdapter->Release();
    }
}

static void CreateDevice( IDXGIAdapter1* pAdapter, ID3D12Device2** ppOutDevice )
{
    HRESULT hr = D3D12CreateDevice( pAdapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS( ppOutDevice ) );
    if( FAILED( hr ) )
    {
        assert( 0 && "Failed to create device" );
    }
}

static void CreateCommandQueue( ID3D12Device2* pDevice, D3D12_COMMAND_LIST_TYPE type,
                                ID3D12CommandQueue** ppOutQueue )
{
    D3D12_COMMAND_QUEUE_DESC desc = {};
    desc.Type = type;
    desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    desc.NodeMask = 0;

    HRESULT hr = pDevice->CreateCommandQueue( &desc, IID_PPV_ARGS( ppOutQueue ) );
    if( FAILED( hr ) )
    {
        assert( 0 && "Failed to create command queue" );
    }
}

} // namespace Bogus::Renderer
