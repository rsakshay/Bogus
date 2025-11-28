#include "Globals.h"
#include "Renderer.h"

#include "App_Windows.h"

#include "d3d12.h"
#include "dxgi1_6.h"
#include <iostream>

namespace Bogus::Renderer
{
static constexpr uint32 MAX_FRAMES = 3;
static constexpr bool VSYNC_ENABLED = true;

enum
{
    eRenderer_Initialized = 1 << 0,
    eRenderer_VSyncEnabled = 1 << 1,
    eRenderer_TearingSupported = 1 << 2,
};
static uint32 g_uiRendererFlags = 0;

static ID3D12Device2* g_Device;
static ID3D12CommandQueue* g_CommandQueue;
static IDXGISwapChain4* g_SwapChain;
static ID3D12DescriptorHeap* g_RTVDescriptorHeap;
static uint32 g_uiRTVDescriptorSize;
static ID3D12Resource* g_BackBuffers[MAX_FRAMES];
static ID3D12CommandAllocator* g_CommandAllocators[MAX_FRAMES];
static ID3D12GraphicsCommandList* g_CommandList;

static uint32 g_uiCurrentBackBufferIndex = 0;
static ID3D12Fence* g_Fence;
static HANDLE g_FenceEvent;
uint64 g_uiFenceValue = 0;
uint64 g_uiFrameFenceValues[MAX_FRAMES] = {};

template <typename T> static void DXRelease( T** ppObj )
{
    ( *ppObj )->Release();
    *ppObj = nullptr;
}

// static internals decls
static bool ASSERT_HROK( HRESULT hr, char const* szMsg );
static void GetHardwareAdapter( IDXGIFactory4* pFactory, IDXGIAdapter1** ppOutAdapter );
static void CreateDevice( IDXGIAdapter1* pAdapter, ID3D12Device2** ppOutDevice );
static void CreateCommandQueue( ID3D12Device2* pDevice, D3D12_COMMAND_LIST_TYPE type,
                                ID3D12CommandQueue** ppOutQueue );
static void CreateSwapChain( HWND hWnd, IDXGIFactory4* pFactory, ID3D12CommandQueue* pQueue,
                             uint32 uiWidth, uint32 uiHeight, uint32 uiBufferCount,
                             IDXGISwapChain4** ppOutSwapChain );
static void CreateFence( ID3D12Device2* pDevice, ID3D12Fence** ppOutFence );
static void CreateDescriptorHeap( ID3D12Device2* pDevice, D3D12_DESCRIPTOR_HEAP_TYPE type,
                                  uint32 uiNumDescriptors,
                                  ID3D12DescriptorHeap** ppOutDescriptorHeap );
static void CreateCommandAllocator( ID3D12Device2* pDevice, D3D12_COMMAND_LIST_TYPE type,
                                    ID3D12CommandAllocator** ppOutCommandAllocator );
static void CreateCommandList( ID3D12Device2* pDevice, ID3D12CommandAllocator* pCommandAllocator,
                               D3D12_COMMAND_LIST_TYPE type,
                               ID3D12GraphicsCommandList** ppOutCommandList );
static void UpdateRenderTargetViews( ID3D12Device2* pDevice, IDXGISwapChain4* pSwapChain,
                                     ID3D12DescriptorHeap* pDescriptorHeap );
static void WaitForFenceValue( ID3D12Fence* pFence, uint64 uiFenceValue, HANDLE hFenceEvent );
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
    CreateCommandQueue( g_Device, D3D12_COMMAND_LIST_TYPE_DIRECT, &g_CommandQueue );
    CreateSwapChain( Bogus::App::g_pAppWindows->m_hWnd, pFactory, g_CommandQueue,
                     Bogus::App::g_pAppWindows->m_uiClientWidth,
                     Bogus::App::g_pAppWindows->m_uiClientHeight, MAX_FRAMES, &g_SwapChain );
    g_uiCurrentBackBufferIndex = g_SwapChain->GetCurrentBackBufferIndex();

    CreateDescriptorHeap( g_Device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, MAX_FRAMES,
                          &g_RTVDescriptorHeap );
    g_uiRTVDescriptorSize =
        g_Device->GetDescriptorHandleIncrementSize( D3D12_DESCRIPTOR_HEAP_TYPE_RTV );

    UpdateRenderTargetViews( g_Device, g_SwapChain, g_RTVDescriptorHeap );

    for( uint32 i = 0; i < MAX_FRAMES; ++i )
    {
        CreateCommandAllocator( g_Device, D3D12_COMMAND_LIST_TYPE_DIRECT, &g_CommandAllocators[i] );
    }
    CreateCommandList( g_Device, g_CommandAllocators[g_uiCurrentBackBufferIndex],
                       D3D12_COMMAND_LIST_TYPE_DIRECT, &g_CommandList );

    CreateFence( g_Device, &g_Fence );
    g_FenceEvent = CreateEvent( nullptr, FALSE, FALSE, nullptr );

    if constexpr( VSYNC_ENABLED )
    {
        g_uiRendererFlags |= eRenderer_VSyncEnabled;
    }
    g_uiRendererFlags |= eRenderer_Initialized;
}

void Render()
{
    ID3D12CommandAllocator* pCommandAllocator = g_CommandAllocators[g_uiCurrentBackBufferIndex];
    ID3D12Resource* pBackBuffer = g_BackBuffers[g_uiCurrentBackBufferIndex];

    pCommandAllocator->Reset();
    g_CommandList->Reset( pCommandAllocator, nullptr );

    { // Clear render target
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition = { pBackBuffer, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
                               D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET };
        g_CommandList->ResourceBarrier( 1, &barrier );

        float clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };

        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle =
            g_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
        rtvHandle.ptr += g_uiCurrentBackBufferIndex * g_uiRTVDescriptorSize;

        g_CommandList->ClearRenderTargetView( rtvHandle, clearColor, 0, nullptr );
    }

    { // Present
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition = { pBackBuffer, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
                               D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT };
        g_CommandList->ResourceBarrier( 1, &barrier );

        ASSERT_HROK( g_CommandList->Close(), "Failed to close command list" );

        ID3D12CommandList* const commandLists[] = { g_CommandList };
        g_CommandQueue->ExecuteCommandLists( ARRAYSIZE( commandLists ), commandLists );

        uint64 uiSignalFenceValue = ++g_uiFenceValue;
        ASSERT_HROK( g_CommandQueue->Signal( g_Fence, uiSignalFenceValue ),
                     "Failed to signal command queue" );
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

        WaitForFenceValue( g_Fence, uiSignalFenceValue, g_FenceEvent );
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
    if( g_CommandQueue && g_Fence )
    {
        WaitForGPU();
    }

    for( uint32 i = 0; i < MAX_FRAMES; ++i )
    {
        DXRelease( &g_BackBuffers[i] );
        DXRelease( &g_CommandAllocators[i] );
        g_uiFrameFenceValues[i] = 0;
    }

    DXRelease( &g_CommandList );

    DXRelease( &g_RTVDescriptorHeap );

    DXRelease( &g_SwapChain );

    DXRelease( &g_Fence );
    CloseHandle( g_FenceEvent );
    g_FenceEvent = nullptr;

    DXRelease( &g_CommandQueue );

    DXRelease( &g_Device );

    g_uiCurrentBackBufferIndex = 0;
    g_uiRTVDescriptorSize = 0;
    g_uiFenceValue = 0;
}

static bool ASSERT_HROK( HRESULT hr, char const* szMsg )
{
    bool const bFailedHR = FAILED( hr );
    if( bFailedHR )
    {
        std::cerr << printf( "[ERROR]: %s\n", szMsg );
        __debugbreak();
    }
    return !bFailedHR;
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

static void CreateCommandQueue( ID3D12Device2* pDevice, D3D12_COMMAND_LIST_TYPE type,
                                ID3D12CommandQueue** ppOutQueue )
{
    D3D12_COMMAND_QUEUE_DESC desc = {};
    desc.Type = type;
    desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    desc.NodeMask = 0;

    ASSERT_HROK( pDevice->CreateCommandQueue( &desc, IID_PPV_ARGS( ppOutQueue ) ),
                 "Failed to create command queue" );
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

    { // Cast the swap chain to IDXGISwapChain4 since we will be using that for the rest of the app
        ASSERT_HROK( pSwapChain1->QueryInterface( IID_PPV_ARGS( ppOutSwapChain ) ),
                     "Failed to pSwapChain1 to IDXGISwapChain4" );
    }
}

static void CreateFence( ID3D12Device2* pDevice, ID3D12Fence** ppOutFence )
{
    ASSERT_HROK( pDevice->CreateFence( 0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS( ppOutFence ) ),
                 "Failed to create fence" );
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

static void CreateCommandAllocator( ID3D12Device2* pDevice, D3D12_COMMAND_LIST_TYPE type,
                                    ID3D12CommandAllocator** ppOutCommandAllocator )
{
    ASSERT_HROK( pDevice->CreateCommandAllocator( type, IID_PPV_ARGS( ppOutCommandAllocator ) ),
                 "Failed to create CommandAllocator" );
}

static void CreateCommandList( ID3D12Device2* pDevice, ID3D12CommandAllocator* pCommandAllocator,
                               D3D12_COMMAND_LIST_TYPE type,
                               ID3D12GraphicsCommandList** ppOutCommandList )
{
    ASSERT_HROK( pDevice->CreateCommandList( 0, type, pCommandAllocator, nullptr,
                                             IID_PPV_ARGS( ppOutCommandList ) ),
                 "Failed to create CommandList" );
    ASSERT_HROK( ( *ppOutCommandList )->Close(), "Failed to close command list" );
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

static void WaitForFenceValue( ID3D12Fence* pFence, uint64 uiFenceValue, HANDLE hFenceEvent )
{
    if( pFence->GetCompletedValue() < uiFenceValue )
    {
        if( !ASSERT_HROK( pFence->SetEventOnCompletion( uiFenceValue, hFenceEvent ),
                          "Failed to set completion event on Fence" ) )
        {
            return;
        }

        WaitForSingleObject( hFenceEvent, INFINITE );
    }
}

static void WaitForGPU()
{
    uint64 uiWaitFenceValue = ++g_uiFenceValue;

    if( !ASSERT_HROK( g_CommandQueue->Signal( g_Fence, uiWaitFenceValue ),
                      "Failed signaling new fence value" ) )
    {
        return;
    }

    WaitForFenceValue( g_Fence, uiWaitFenceValue, g_FenceEvent );
}

} // namespace Bogus::Renderer
