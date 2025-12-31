#include "Globals.h"
#include "Renderer.h"
#include "dx12/RendererDX12_CommandQueue.h"
#include "dx12/RendererDX12_Utils.h"

#include "App_Windows.h"
#include "Core_Assert.h"

#include "DirectXMath.h"
#include "d3d12.h"
#include "dxgi1_5.h"
#include <d3dcompiler.h> // D3DCompile
#include <iostream>
#pragma comment( lib, "d3dcompiler.lib" )

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

static ID3D12RootSignature* g_RootSignature;
static ID3D12PipelineState* g_PSO;

// Depth
static ID3D12DescriptorHeap* g_DSVDescriptorHeap;
static ID3D12Resource* g_DepthBuffer;

// Viewport/scissor
static D3D12_VIEWPORT g_Viewport;
static D3D12_RECT g_ScissorRect;

// Simple constant buffer (one MVP)
struct alignas( 256 ) CB_MVP
{
    DirectX::XMFLOAT4X4 MVP;
};

static ID3D12Resource* g_ConstantBuffer[MAX_FRAMES];
static CB_MVP* g_pCBMapped[MAX_FRAMES];

static ID3D12Resource* g_VertexBuffer = nullptr;
static ID3D12Resource* g_VertexUploadBuffer = nullptr;
static D3D12_VERTEX_BUFFER_VIEW g_VBView = {};

static ID3D12Resource* g_IndexBuffer = nullptr;
static ID3D12Resource* g_IndexUploadBuffer = nullptr;
static D3D12_INDEX_BUFFER_VIEW g_IBView = {};

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
static void CreateDXBuffer( ID3D12Device2* pDevice, uint64 uiSize, D3D12_HEAP_TYPE heapType,
                            D3D12_RESOURCE_STATES initialState, ID3D12Resource** ppOutBuffer );
static void UploadToDXBuffer( ID3D12Resource* pUploadBuffer, void const* pData, uint64 uiSize );
static ID3DBlob* CompileShader( char const* pSource, char const* pEntry, char const* pTarget );
static void CreateRootSignature( ID3D12Device2* pDevice, ID3D12RootSignature** ppOutRootSig );
static void CreatePipelineState( ID3D12Device2* pDevice, ID3D12RootSignature* pRootSig,
                                 ID3D12PipelineState** ppOutPSO );
static void CreateDSVHeap( ID3D12Device2* pDevice, ID3D12DescriptorHeap** ppOutHeap );
static void CreateDepthBuffer( ID3D12Device2* pDevice, uint32 w, uint32 h,
                               ID3D12DescriptorHeap* pDSVHeap, ID3D12Resource** ppOutDepth );
static void CreateConstantBuffers( ID3D12Device2* pDevice );

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

    CreateDSVHeap( g_Device, &g_DSVDescriptorHeap );
    CreateDepthBuffer( g_Device, Bogus::App::g_pAppWindows->m_uiClientWidth,
                       Bogus::App::g_pAppWindows->m_uiClientHeight, g_DSVDescriptorHeap,
                       &g_DepthBuffer );

    CreateRootSignature( g_Device, &g_RootSignature );
    CreatePipelineState( g_Device, g_RootSignature, &g_PSO );
    CreateConstantBuffers( g_Device );

    // viewport / scissor
    g_Viewport = { 0.0f,
                   0.0f,
                   (float)Bogus::App::g_pAppWindows->m_uiClientWidth,
                   (float)Bogus::App::g_pAppWindows->m_uiClientHeight,
                   0.0f,
                   1.0f };

    g_ScissorRect = { 0, 0, (int32)Bogus::App::g_pAppWindows->m_uiClientWidth,
                      (int32)Bogus::App::g_pAppWindows->m_uiClientHeight };

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

        // RTV
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle =
            g_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
        rtvHandle.ptr += g_uiCurrentBackBufferIndex * g_uiRTVDescriptorSize;

        pCmdList->pList->ClearRenderTargetView( rtvHandle, clearColor, 0, nullptr );

        // DSV
        D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle =
            g_DSVDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
        pCmdList->pList->ClearDepthStencilView( dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0,
                                                nullptr );

        pCmdList->pList->OMSetRenderTargets( 1, &rtvHandle, FALSE, &dsvHandle );
        pCmdList->pList->RSSetViewports( 1, &g_Viewport );
        pCmdList->pList->RSSetScissorRects( 1, &g_ScissorRect );

        pCmdList->pList->SetPipelineState( g_PSO );
        pCmdList->pList->SetGraphicsRootSignature( g_RootSignature );

        // Update CB (simple rotating MVP)
        static uint64 s_TickCount = 0;
        auto GetTickCount64 = []() { return s_TickCount++; };
        using namespace DirectX;
        XMMATRIX m = XMMatrixRotationY( (float)GetTickCount64() * 0.001f );
        XMMATRIX v = XMMatrixLookAtLH( XMVectorSet( 0, 0, -5, 0 ), XMVectorZero(),
                                       XMVectorSet( 0, 1, 0, 0 ) );
        XMMATRIX p = XMMatrixPerspectiveFovLH( XM_PIDIV4, g_Viewport.Width / g_Viewport.Height,
                                               0.1f, 100.0f );
        XMMATRIX mvp = XMMatrixTranspose( m * v * p );
        XMStoreFloat4x4( &g_pCBMapped[g_uiCurrentBackBufferIndex]->MVP, mvp );

        // Root CBV
        pCmdList->pList->SetGraphicsRootConstantBufferView(
            0, g_ConstantBuffer[g_uiCurrentBackBufferIndex]->GetGPUVirtualAddress() );

        // IA + Draw
        pCmdList->pList->IASetPrimitiveTopology( D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
        pCmdList->pList->IASetVertexBuffers( 0, 1, &g_VBView );
        pCmdList->pList->IASetIndexBuffer( &g_IBView );
        pCmdList->pList->DrawIndexedInstanced( 36, 1, 0, 0, 0 );
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

    DXRelease( &g_DepthBuffer );
    CreateDepthBuffer( g_Device, uiWidth, uiHeight, g_DSVDescriptorHeap, &g_DepthBuffer );

    g_Viewport.Width = (float)uiWidth;
    g_Viewport.Height = (float)uiHeight;

    g_ScissorRect.right = (int32)uiWidth;
    g_ScissorRect.bottom = (int32)uiHeight;

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

    for( uint32 i = 0; i < MAX_FRAMES; ++i )
    {
        if( g_ConstantBuffer[i] )
        {
            g_ConstantBuffer[i]->Unmap( 0, nullptr );
            g_pCBMapped[i] = nullptr;
        }
        DXRelease( &g_ConstantBuffer[i] );
    }

    DXRelease( &g_PSO );
    DXRelease( &g_RootSignature );

    DXRelease( &g_DepthBuffer );
    DXRelease( &g_DSVDescriptorHeap );

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

static void CreateDXBuffer( ID3D12Device2* pDevice, uint64 uiSize, D3D12_HEAP_TYPE heapType,
                            D3D12_RESOURCE_STATES initialState, ID3D12Resource** ppOutBuffer )
{
    BGASSERT( pDevice && ppOutBuffer, "Invalid args to CreateDXBuffer" );
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = heapType;
    heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProps.CreationNodeMask = 1;
    heapProps.VisibleNodeMask = 1;

    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Alignment = 0;
    desc.Width = uiSize;
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.SampleDesc = { 1, 0 };
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.Flags = D3D12_RESOURCE_FLAG_NONE;

    ASSERT_HROK( pDevice->CreateCommittedResource( &heapProps, D3D12_HEAP_FLAG_NONE, &desc,
                                                   initialState, nullptr,
                                                   IID_PPV_ARGS( ppOutBuffer ) ),
                 "Failed to create committed buffer resource" );
}

static void UploadToDXBuffer( ID3D12Resource* pUploadBuffer, void const* pData, uint64 uiSize )
{
    BGASSERT( pUploadBuffer && pData && uiSize > 0, "UploadToBuffer invalid args" );

    void* pMapped = nullptr;
    D3D12_RANGE readRange = { 0, 0 }; // CPU will not read back.
    ASSERT_HROK( pUploadBuffer->Map( 0, &readRange, &pMapped ), "Failed to map upload buffer" );

    memcpy( pMapped, pData, uiSize );

    // We wrote the entire range.
    D3D12_RANGE writtenRange = { 0, uiSize };
    pUploadBuffer->Unmap( 0, &writtenRange );
}

static ID3DBlob* CompileShader( const char* pSource, const char* pEntry, const char* pTarget )
{
    UINT uiFlags = 0;
#if defined( _DEBUG )
    uiFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    ID3DBlob* pBytecode = nullptr;
    ID3DBlob* pErrors = nullptr;

    HRESULT hr = D3DCompile( pSource, strlen( pSource ), nullptr, nullptr, nullptr, pEntry, pTarget,
                             uiFlags, 0, &pBytecode, &pErrors );

    if( FAILED( hr ) )
    {
        if( pErrors )
        {
            std::cerr << (const char*)pErrors->GetBufferPointer() << "\n";
            pErrors->Release();
        }
        DXRelease( &pBytecode );
        return nullptr;
    }

    if( pErrors )
        pErrors->Release();
    return pBytecode;
}

static void CreateRootSignature( ID3D12Device2* pDevice, ID3D12RootSignature** ppOutRootSig )
{
    // Root parameter: CBV(b0)
    D3D12_ROOT_PARAMETER param = {};
    param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    param.Descriptor.ShaderRegister = 0; // b0
    param.Descriptor.RegisterSpace = 0;
    param.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

    D3D12_ROOT_SIGNATURE_DESC desc = {};
    desc.NumParameters = 1;
    desc.pParameters = &param;
    desc.NumStaticSamplers = 0;
    desc.pStaticSamplers = nullptr;
    desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ID3DBlob* pSigBlob = nullptr;
    ID3DBlob* pErrorBlob = nullptr;

    ASSERT_HROK(
        D3D12SerializeRootSignature( &desc, D3D_ROOT_SIGNATURE_VERSION_1, &pSigBlob, &pErrorBlob ),
        "Failed to serialize root signature" );

    if( pErrorBlob )
    {
        std::cerr << (const char*)pErrorBlob->GetBufferPointer() << "\n";
        pErrorBlob->Release();
    }

    ASSERT_HROK( pDevice->CreateRootSignature( 0, pSigBlob->GetBufferPointer(),
                                               pSigBlob->GetBufferSize(),
                                               IID_PPV_ARGS( ppOutRootSig ) ),
                 "Failed to create root signature" );

    pSigBlob->Release();
}

static void DefaultBlendDesc( D3D12_BLEND_DESC* pDesc )
{
    pDesc->AlphaToCoverageEnable = FALSE;
    pDesc->IndependentBlendEnable = FALSE;

    D3D12_RENDER_TARGET_BLEND_DESC rt = {};
    rt.BlendEnable = FALSE;
    rt.LogicOpEnable = FALSE;
    rt.SrcBlend = D3D12_BLEND_ONE;
    rt.DestBlend = D3D12_BLEND_ZERO;
    rt.BlendOp = D3D12_BLEND_OP_ADD;
    rt.SrcBlendAlpha = D3D12_BLEND_ONE;
    rt.DestBlendAlpha = D3D12_BLEND_ZERO;
    rt.BlendOpAlpha = D3D12_BLEND_OP_ADD;
    rt.LogicOp = D3D12_LOGIC_OP_NOOP;
    rt.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    for( uint32 i = 0; i < 8; ++i )
        pDesc->RenderTarget[i] = rt;
}

static void DefaultDepthStencilDesc( D3D12_DEPTH_STENCIL_DESC* pDesc )
{
    pDesc->DepthEnable = TRUE;
    pDesc->DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    pDesc->DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    pDesc->StencilEnable = FALSE;
    pDesc->StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
    pDesc->StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;

    pDesc->FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
    pDesc->FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
    pDesc->FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
    pDesc->FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    pDesc->BackFace = pDesc->FrontFace;
}

static void DefaultRasterizerDesc( D3D12_RASTERIZER_DESC* pDesc )
{
    pDesc->FillMode = D3D12_FILL_MODE_SOLID;
    pDesc->CullMode = D3D12_CULL_MODE_BACK;
    pDesc->FrontCounterClockwise = FALSE;
    pDesc->DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
    pDesc->DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    pDesc->SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    pDesc->DepthClipEnable = TRUE;
    pDesc->MultisampleEnable = FALSE;
    pDesc->AntialiasedLineEnable = FALSE;
    pDesc->ForcedSampleCount = 0;
    pDesc->ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
}

static void CreatePipelineState( ID3D12Device2* pDevice, ID3D12RootSignature* pRootSig,
                                 ID3D12PipelineState** ppOutPSO )
{
    char const* vsSrc = R"(
cbuffer MVP : register(b0)
{
    float4x4 g_MVP;
};
struct VSIn
{
    float3 pos   : POSITION;
    float3 color : COLOR;
};
struct VSOut
{
    float4 pos   : SV_Position;
    float3 color : COLOR;
};
VSOut main(VSIn v)
{
    VSOut o;
    o.pos = mul(float4(v.pos, 1.0f), g_MVP);
    o.color = v.color;
    return o;
}
)";

    char const* psSrc = R"(
struct PSIn
{
    float4 pos   : SV_Position;
    float3 color : COLOR;
};
float4 main(PSIn i) : SV_Target
{
    return float4(i.color, 1.0f);
}
)";

    ID3DBlob* vs = CompileShader( vsSrc, "main", "vs_5_0" );
    ID3DBlob* ps = CompileShader( psSrc, "main", "ps_5_0" );
    BGASSERT( vs, "VS blob was null" );
    BGASSERT( ps, "PS blob was null" );

    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
          D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12,
          D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC pso = {};
    pso.pRootSignature = pRootSig;
    pso.VS = { vs->GetBufferPointer(), vs->GetBufferSize() };
    pso.PS = { ps->GetBufferPointer(), ps->GetBufferSize() };
    pso.SampleMask = UINT_MAX;
    pso.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pso.NumRenderTargets = 1;
    pso.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    pso.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    pso.SampleDesc = { 1, 0 };
    pso.InputLayout = { inputLayout, ARRAYSIZE( inputLayout ) };
    DefaultBlendDesc( &pso.BlendState );
    DefaultRasterizerDesc( &pso.RasterizerState );
    DefaultDepthStencilDesc( &pso.DepthStencilState );

    ASSERT_HROK( pDevice->CreateGraphicsPipelineState( &pso, IID_PPV_ARGS( ppOutPSO ) ),
                 "Failed to create PSO" );

    vs->Release();
    ps->Release();
}

static void CreateDSVHeap( ID3D12Device2* pDevice, ID3D12DescriptorHeap** ppOutHeap )
{
    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.NumDescriptors = 1;
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    ASSERT_HROK( pDevice->CreateDescriptorHeap( &desc, IID_PPV_ARGS( ppOutHeap ) ),
                 "Failed to create DSV heap" );
}

static void CreateDepthBuffer( ID3D12Device2* pDevice, uint32 w, uint32 h,
                               ID3D12DescriptorHeap* pDSVHeap, ID3D12Resource** ppOutDepth )
{
    D3D12_CLEAR_VALUE clear = {};
    clear.Format = DXGI_FORMAT_D32_FLOAT;
    clear.DepthStencil.Depth = 1.0f;
    clear.DepthStencil.Stencil = 0;

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Alignment = 0;
    desc.Width = w;
    desc.Height = h;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_D32_FLOAT;
    desc.SampleDesc = { 1, 0 };
    desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    ASSERT_HROK( pDevice->CreateCommittedResource( &heapProps, D3D12_HEAP_FLAG_NONE, &desc,
                                                   D3D12_RESOURCE_STATE_DEPTH_WRITE, &clear,
                                                   IID_PPV_ARGS( ppOutDepth ) ),
                 "Failed to create depth buffer" );

    D3D12_DEPTH_STENCIL_VIEW_DESC dsv = {};
    dsv.Format = DXGI_FORMAT_D32_FLOAT;
    dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsv.Flags = D3D12_DSV_FLAG_NONE;

    pDevice->CreateDepthStencilView( *ppOutDepth, &dsv,
                                     pDSVHeap->GetCPUDescriptorHandleForHeapStart() );
}

static void CreateConstantBuffers( ID3D12Device2* pDevice )
{
    for( uint32 i = 0; i < MAX_FRAMES; ++i )
    {
        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

        D3D12_RESOURCE_DESC desc = {};
        desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Width = sizeof( CB_MVP );
        desc.Height = 1;
        desc.DepthOrArraySize = 1;
        desc.MipLevels = 1;
        desc.SampleDesc = { 1, 0 };
        desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        ASSERT_HROK( pDevice->CreateCommittedResource( &heapProps, D3D12_HEAP_FLAG_NONE, &desc,
                                                       D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                                       IID_PPV_ARGS( &g_ConstantBuffer[i] ) ),
                     "Failed to create constant buffer" );

        ASSERT_HROK( g_ConstantBuffer[i]->Map( 0, nullptr, (void**)&g_pCBMapped[i] ),
                     "Failed to map constant buffer" );
    }
}

static void LoadCube_DirectUpload()
{
    // Vertex buffer directly in UPLOAD heap
    {
        uint64 const uiVBSize = sizeof( g_Vertices );

        CreateDXBuffer( g_Device, uiVBSize, D3D12_HEAP_TYPE_UPLOAD,
                        D3D12_RESOURCE_STATE_GENERIC_READ, &g_VertexBuffer );

        UploadToDXBuffer( g_VertexBuffer, g_Vertices, uiVBSize );

        g_VBView.BufferLocation = g_VertexBuffer->GetGPUVirtualAddress();
        g_VBView.StrideInBytes = sizeof( VertexPosColor );
        g_VBView.SizeInBytes = uiVBSize;
    }

    // Index buffer directly in UPLOAD heap
    {
        uint64 const uiIBSize = sizeof( g_Indicies );

        CreateDXBuffer( g_Device, uiIBSize, D3D12_HEAP_TYPE_UPLOAD,
                        D3D12_RESOURCE_STATE_GENERIC_READ, &g_IndexBuffer );

        UploadToDXBuffer( g_IndexBuffer, g_Indicies, uiIBSize );

        g_IBView.BufferLocation = g_IndexBuffer->GetGPUVirtualAddress();
        g_IBView.SizeInBytes = uiIBSize;
        g_IBView.Format = DXGI_FORMAT_R16_UINT;
    }
}

static void LoadCube()
{
    auto InitBuffers = []( ID3D12Resource** ppCPUBuffer, ID3D12Resource** ppGPUBuffer,
                           void const* pData, uint64 const uiSize )
    {
        CreateDXBuffer( g_Device, uiSize, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_COPY_DEST,
                        ppCPUBuffer );
        CreateDXBuffer( g_Device, uiSize, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ,
                        ppGPUBuffer );
        UploadToDXBuffer( *ppGPUBuffer, pData, uiSize );
    };

    // Create Vertex Buffer
    uint64 const uiVBSize = sizeof( g_Vertices );
    InitBuffers( &g_VertexUploadBuffer, &g_VertexBuffer, g_Vertices, uiVBSize );

    // Create Index Buffer
    uint64 const uiIBSize = sizeof( g_Indicies );
    InitBuffers( &g_IndexUploadBuffer, &g_IndexBuffer, g_Indicies, uiIBSize );

    // Record copy commands on CopyQueue
    CommandList* pCopyCmd = NULL;
    g_CopyQueue.GetCommandList( &pCopyCmd );

    pCopyCmd->pList->CopyBufferRegion( g_VertexBuffer, 0, g_VertexUploadBuffer, 0, uiVBSize );
    pCopyCmd->pList->CopyBufferRegion( g_IndexBuffer, 0, g_IndexUploadBuffer, 0, uiIBSize );

    uint64 const uiCopyFence = g_CopyQueue.ExecuteCommandList( pCopyCmd );
    // -------------------------
    // IMPORTANT: keep upload buffers alive until copy finishes.
    // Since this is load-time, we can just wait here once.
    // -------------------------
    g_CopyQueue.WaitForFence( uiCopyFence );

    DXRelease( &g_VertexUploadBuffer );
    DXRelease( &g_IndexUploadBuffer );

    // Transition ON THE DIRECT QUEUE
    CommandList* pDirectCmd = nullptr;
    g_DirectQueue.GetCommandList( &pDirectCmd );

    D3D12_RESOURCE_BARRIER barriers[2] = {};

    barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barriers[0].Transition.pResource = g_VertexBuffer;
    barriers[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;

    barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barriers[1].Transition.pResource = g_IndexBuffer;
    barriers[1].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_INDEX_BUFFER;

    pDirectCmd->pList->ResourceBarrier( 2, barriers );

    uint64 const uiDirectFence = g_DirectQueue.ExecuteCommandList( pDirectCmd );
    g_DirectQueue.WaitForFence( uiDirectFence );

    // -------------------------
    // Create views
    g_VBView.BufferLocation = g_VertexBuffer->GetGPUVirtualAddress();
    g_VBView.SizeInBytes = uiVBSize;
    g_VBView.StrideInBytes = sizeof( VertexPosColor );

    g_IBView.BufferLocation = g_IndexBuffer->GetGPUVirtualAddress();
    g_IBView.SizeInBytes = uiIBSize;
    g_IBView.Format = DXGI_FORMAT_R16_UINT;
}

static void WaitForGPU()
{
    g_DirectQueue.Flush();
    g_CopyQueue.Flush();
    g_ComputeQueue.Flush();
}

} // namespace Bogus::Renderer
