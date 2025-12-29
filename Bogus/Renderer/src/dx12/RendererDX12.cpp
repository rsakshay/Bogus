#include "Globals.h"
#include "Renderer.h"
#include "dx12/RendererDX12_CommandQueue.h"
#include "dx12/RendererDX12_Utils.h"

#include "App_Windows.h"
#include "Core_Assert.h"

#include "DirectXMath.h"
#include "d3d12.h"
#include "dxgi1_5.h"
#include <iostream>

namespace Bogus::Renderer
{
static constexpr bool VSYNC_ENABLED = true;

enum
{
    eRenderer_Initialized = 1 << 0,
    eRenderer_VSyncEnabled = 1 << 1,
    eRenderer_TearingSupported = 1 << 2,
};
static uint32 g_uiRendererFlags = 0;

static ID3D12Device2* g_Device;
static IDXGISwapChain4* g_SwapChain;
static ID3D12DescriptorHeap* g_RTVDescriptorHeap;
static uint32 g_uiRTVDescriptorSize;
static ID3D12Resource* g_BackBuffers[MAX_FRAMES];

static CommandQueue g_DirectQueue;
static CommandQueue g_CopyQueue;
static CommandQueue g_ComputeQueue;

static uint32 g_uiCurrentBackBufferIndex = 0;
uint64 g_uiFenceValue = 0;
uint64 g_uiFrameFenceValues[MAX_FRAMES] = {};

// Vertex data for a colored cube.
struct VertexPosColor
{
    DirectX::XMFLOAT3 Position;
    DirectX::XMFLOAT3 Color;
};

static VertexPosColor g_Vertices[8] = {
    { DirectX::XMFLOAT3( -1.0f, -1.0f, -1.0f ), DirectX::XMFLOAT3( 0.0f, 0.0f, 0.0f ) }, // 0
    { DirectX::XMFLOAT3( -1.0f, 1.0f, -1.0f ), DirectX::XMFLOAT3( 0.0f, 1.0f, 0.0f ) },  // 1
    { DirectX::XMFLOAT3( 1.0f, 1.0f, -1.0f ), DirectX::XMFLOAT3( 1.0f, 1.0f, 0.0f ) },   // 2
    { DirectX::XMFLOAT3( 1.0f, -1.0f, -1.0f ), DirectX::XMFLOAT3( 1.0f, 0.0f, 0.0f ) },  // 3
    { DirectX::XMFLOAT3( -1.0f, -1.0f, 1.0f ), DirectX::XMFLOAT3( 0.0f, 0.0f, 1.0f ) },  // 4
    { DirectX::XMFLOAT3( -1.0f, 1.0f, 1.0f ), DirectX::XMFLOAT3( 0.0f, 1.0f, 1.0f ) },   // 5
    { DirectX::XMFLOAT3( 1.0f, 1.0f, 1.0f ), DirectX::XMFLOAT3( 1.0f, 1.0f, 1.0f ) },    // 6
    { DirectX::XMFLOAT3( 1.0f, -1.0f, 1.0f ), DirectX::XMFLOAT3( 1.0f, 0.0f, 1.0f ) }    // 7
};

static uint16 g_Indicies[36] = { 0, 1, 2, 0, 2, 3, 4, 6, 5, 4, 7, 6, 4, 5, 1, 4, 1, 0,
                                 3, 2, 6, 3, 6, 7, 1, 5, 6, 1, 6, 2, 4, 0, 3, 4, 3, 7 };

// static internals decls
static void GetHardwareAdapter( IDXGIFactory4* pFactory, IDXGIAdapter1** ppOutAdapter );
static void CreateDevice( IDXGIAdapter1* pAdapter, ID3D12Device2** ppOutDevice );
static void CreateSwapChain( HWND hWnd, IDXGIFactory4* pFactory, ID3D12CommandQueue* pQueue,
                             uint32 uiWidth, uint32 uiHeight, uint32 uiBufferCount,
                             IDXGISwapChain4** ppOutSwapChain );
static void CreateDescriptorHeap( ID3D12Device2* pDevice, D3D12_DESCRIPTOR_HEAP_TYPE type,
                                  uint32 uiNumDescriptors,
                                  ID3D12DescriptorHeap** ppOutDescriptorHeap );
static void UpdateRenderTargetViews( ID3D12Device2* pDevice, IDXGISwapChain4* pSwapChain,
                                     ID3D12DescriptorHeap* pDescriptorHeap );
static void LoadCube();
static void WaitForGPU();
// static internals decls

void Initialize()
{
    IDXGIFactory4* pFactory;
    if( !ASSERT_HROK( CreateDXGIFactory2( 0, IID_PPV_ARGS( &pFactory ) ),
                      "Failed to create pFactory" ) )
    {
        return;
    }

    IDXGIAdapter1* pAdapter;
    GetHardwareAdapter( pFactory, &pAdapter );

    CreateDevice( pAdapter, &g_Device );

    g_DirectQueue.Initialize( g_Device, D3D12_COMMAND_LIST_TYPE_DIRECT );
    g_CopyQueue.Initialize( g_Device, D3D12_COMMAND_LIST_TYPE_COPY );
    g_ComputeQueue.Initialize( g_Device, D3D12_COMMAND_LIST_TYPE_COMPUTE );

    CreateSwapChain( Bogus::App::g_pAppWindows->m_hWnd, pFactory, g_DirectQueue.m_pQueue,
                     Bogus::App::g_pAppWindows->m_uiClientWidth,
                     Bogus::App::g_pAppWindows->m_uiClientHeight, MAX_FRAMES, &g_SwapChain );
    g_uiCurrentBackBufferIndex = g_SwapChain->GetCurrentBackBufferIndex();

    CreateDescriptorHeap( g_Device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, MAX_FRAMES,
                          &g_RTVDescriptorHeap );
    g_uiRTVDescriptorSize =
        g_Device->GetDescriptorHandleIncrementSize( D3D12_DESCRIPTOR_HEAP_TYPE_RTV );

    UpdateRenderTargetViews( g_Device, g_SwapChain, g_RTVDescriptorHeap );

    if constexpr( VSYNC_ENABLED )
    {
        g_uiRendererFlags |= eRenderer_VSyncEnabled;
    }
    g_uiRendererFlags |= eRenderer_Initialized;

    LoadCube();
}

void Render()
{
    uint64 const uiBackBufferFence = g_uiFrameFenceValues[g_uiCurrentBackBufferIndex];
    g_DirectQueue.WaitForFence( uiBackBufferFence );

    CommandList* pCmdList = NULL;
    g_DirectQueue.GetCommandList( &pCmdList );

    ID3D12Resource* pBackBuffer = g_BackBuffers[g_uiCurrentBackBufferIndex];

    { // Clear render target
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition = { pBackBuffer, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
                               D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET };
        pCmdList->pList->ResourceBarrier( 1, &barrier );

        float clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };

        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle =
            g_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
        rtvHandle.ptr += g_uiCurrentBackBufferIndex * g_uiRTVDescriptorSize;

        pCmdList->pList->ClearRenderTargetView( rtvHandle, clearColor, 0, nullptr );
    }

    { // Present
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition = { pBackBuffer, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
                               D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT };
        pCmdList->pList->ResourceBarrier( 1, &barrier );

        uint64 uiSignalFenceValue = g_DirectQueue.ExecuteCommandList( pCmdList );

        g_uiFrameFenceValues[g_uiCurrentBackBufferIndex] = uiSignalFenceValue;

        uint32 uiSyncInterval = g_uiRendererFlags & eRenderer_VSyncEnabled ? 1 : 0;
        uint32 uiPresentFlags =
            ( g_uiRendererFlags & ( eRenderer_TearingSupported | eRenderer_VSyncEnabled ) ) ==
                    eRenderer_TearingSupported
                ? DXGI_PRESENT_ALLOW_TEARING
                : 0;
        ASSERT_HROK( g_SwapChain->Present( uiSyncInterval, uiPresentFlags ),
                     "Failed to present Swap Chain" );

        g_uiCurrentBackBufferIndex = g_SwapChain->GetCurrentBackBufferIndex();
    }
}

void Resize( uint32 uiWidth, uint32 uiHeight )
{
    WaitForGPU();

    for( uint32 i = 0; i < MAX_FRAMES; ++i )
    {
        DXRelease( &g_BackBuffers[i] );
        g_uiFrameFenceValues[i] = g_uiFrameFenceValues[g_uiCurrentBackBufferIndex];
    }

    DXGI_SWAP_CHAIN_DESC desc = {};
    ASSERT_HROK( g_SwapChain->GetDesc( &desc ), "Failed to get swap chain descriptor." );
    ASSERT_HROK( g_SwapChain->ResizeBuffers( MAX_FRAMES, uiWidth, uiHeight, desc.BufferDesc.Format,
                                             desc.Flags ),
                 "Failed to resize back buffers." );

    g_uiCurrentBackBufferIndex = g_SwapChain->GetCurrentBackBufferIndex();

    UpdateRenderTargetViews( g_Device, g_SwapChain, g_RTVDescriptorHeap );
}

void Terminate()
{
    WaitForGPU();

    for( uint32 i = 0; i < MAX_FRAMES; ++i )
    {
        DXRelease( &g_BackBuffers[i] );
        g_uiFrameFenceValues[i] = 0;
    }

    DXRelease( &g_RTVDescriptorHeap );

    DXRelease( &g_SwapChain );

    g_DirectQueue.Terminate();
    g_CopyQueue.Terminate();
    g_ComputeQueue.Terminate();

    DXRelease( &g_Device );

    g_uiCurrentBackBufferIndex = 0;
    g_uiRTVDescriptorSize = 0;
    g_uiFenceValue = 0;
}

static void GetHardwareAdapter( IDXGIFactory4* pFactory, IDXGIAdapter1** ppOutAdapter )
{
    *ppOutAdapter = nullptr;
    for( UINT adapterIndex = 0;; ++adapterIndex )
    {
        IDXGIAdapter1* pAdapter = nullptr;
        if( DXGI_ERROR_NOT_FOUND == pFactory->EnumAdapters1( adapterIndex, &pAdapter ) )
        {
            std::cerr << "No more adapters to enumerate.";
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
    ASSERT_HROK( D3D12CreateDevice( pAdapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS( ppOutDevice ) ),
                 "Failed to create device" );
}

static void CreateSwapChain( HWND hWnd, IDXGIFactory4* pFactory, ID3D12CommandQueue* pQueue,
                             uint32 uiWidth, uint32 uiHeight, uint32 uiBufferCount,
                             IDXGISwapChain4** ppOutSwapChain )
{

    auto IsTearingSupported = [&pFactory]() -> bool
    {
        BOOL bAllowTearing = FALSE;
        IDXGIFactory5* pFactory5;
        if( SUCCEEDED( pFactory->QueryInterface( IID_PPV_ARGS( &pFactory5 ) ) ) )
        {
            if( FAILED( pFactory5->CheckFeatureSupport( DXGI_FEATURE_PRESENT_ALLOW_TEARING,
                                                        &bAllowTearing,
                                                        sizeof( bAllowTearing ) ) ) )
            {
                bAllowTearing = FALSE;
            }
        }
        return bAllowTearing == TRUE;
    };
    bool const isTearingSupported = IsTearingSupported();

    DXGI_SWAP_CHAIN_DESC1 desc = {};
    desc.Width = uiWidth;
    desc.Height = uiHeight;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.Stereo = FALSE;
    desc.SampleDesc = { 1, 0 };
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount = uiBufferCount;
    desc.Scaling = DXGI_SCALING_STRETCH;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    desc.Flags = isTearingSupported ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
    if( isTearingSupported )
    {
        g_uiRendererFlags |= eRenderer_TearingSupported;
    }

    IDXGISwapChain1* pSwapChain1;
    { // Create the swap chain
        ASSERT_HROK(
            pFactory->CreateSwapChainForHwnd( pQueue, hWnd, &desc, nullptr, nullptr, &pSwapChain1 ),
            "Failed to create swap chain" );
    }

    { // Cast the swap chain to IDXGISwapChain4 since we will be using that for the rest of the
      // app
        ASSERT_HROK( pSwapChain1->QueryInterface( IID_PPV_ARGS( ppOutSwapChain ) ),
                     "Failed to pSwapChain1 to IDXGISwapChain4" );
    }
}

static void CreateDescriptorHeap( ID3D12Device2* pDevice, D3D12_DESCRIPTOR_HEAP_TYPE type,
                                  uint32 uiNumDescriptors,
                                  ID3D12DescriptorHeap** ppOutDescriptorHeap )
{
    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.NumDescriptors = uiNumDescriptors;
    desc.Type = type;

    ASSERT_HROK( pDevice->CreateDescriptorHeap( &desc, IID_PPV_ARGS( ppOutDescriptorHeap ) ),
                 "Failed to create descriptor heap" );
}

static void UpdateRenderTargetViews( ID3D12Device2* pDevice, IDXGISwapChain4* pSwapChain,
                                     ID3D12DescriptorHeap* pDescriptorHeap )
{
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = pDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

    for( uint32 i = 0; i < MAX_FRAMES; ++i )
    {
        ID3D12Resource* pBackBuffer;
        ASSERT_HROK( pSwapChain->GetBuffer( i, IID_PPV_ARGS( &pBackBuffer ) ),
                     "Failed to get back buffer" );

        pDevice->CreateRenderTargetView( pBackBuffer, nullptr, rtvHandle );
        g_BackBuffers[i] = pBackBuffer;
        rtvHandle.ptr += g_uiRTVDescriptorSize;
    }
}

static void LoadCube()
{
    ID3D12Resource* pIntermediateVBuffer;
}

static void WaitForGPU()
{
    g_DirectQueue.Flush();
    g_CopyQueue.Flush();
    g_ComputeQueue.Flush();
}

} // namespace Bogus::Renderer
