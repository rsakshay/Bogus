#include "dx12/RendererDX12_CommandQueue.h"
#include "Core_Assert.h"
#include "Core_Vector.h"
#include "dx12/RendererDX12_Utils.h"

#include "d3d12.h"
#include "dxgi1_5.h"

namespace Bogus::Renderer
{
static Core::HeapVector<CommandAllocator, MAX_FRAMES * 2> g_CommandAllocators;
static Core::HeapVector<CommandList, MAX_FRAMES * 2> g_CommandLists;
// INTERNALS (DECLS) -------------------------------------------------------------------------------
static void CreateCommandAllocator( ID3D12Device2* pDevice, D3D12_COMMAND_LIST_TYPE type,
                                    ID3D12CommandAllocator** pOut );
static void CreateCommandList( ID3D12Device2* pDevice, ID3D12CommandAllocator* pCommandAllocator,
                               D3D12_COMMAND_LIST_TYPE type,
                               ID3D12GraphicsCommandList2** ppOutCommandList );
static bool TestFence( ID3D12Fence* pFence, CommandAllocator const& alloc );
// INTERNALS (DECLS) -------------------------------------------------------------------------------

// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------
void CommandQueue::Initialize( ID3D12Device2* pDevice, D3D12_COMMAND_LIST_TYPE type )
{
    m_Type = type;
    m_pDevice = pDevice;

    D3D12_COMMAND_QUEUE_DESC desc = {};
    desc.Type = type;
    desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    desc.NodeMask = 0;

    ASSERT_HROK( pDevice->CreateCommandQueue( &desc, IID_PPV_ARGS( &m_pQueue ) ),
                 "Failed to create command queue" );

    ASSERT_HROK( pDevice->CreateFence( 0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS( &m_pFence ) ),
                 "Failed to create fence" );

    m_FenceEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
    BGASSERT( m_FenceEvent, "Failed to create fence event handle." );
}

// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------
void CommandQueue::Terminate()
{
    // NOTE(asr): wait for current inflight allocators to be done
    if( m_AllocatorQueue.count() )
    {
        CommandAllocator* const& pBack = m_AllocatorQueue.back();
        if( !TestFence( m_pFence, *pBack ) )
        {
            WaitForFenceValue( m_pFence, pBack->uiFence, m_FenceEvent );
        }
    }

    // Release all allocators
    for( CommandAllocator*& allocator : m_AllocatorQueue )
    {
        DXRelease( &allocator->pCmdAlloc );
    }

    // Release all lists
    for( CommandList*& list : m_ListsQueue )
    {
        DXRelease( &list->pList );
    }

    DXRelease( &m_pFence );
    CloseHandle( m_FenceEvent );
    m_FenceEvent = nullptr;

    // Release the queue
    DXRelease( &m_pQueue );
}

// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------
void CommandQueue::GetCommandList( CommandList** ppOutCommandList )
{
    CommandAllocator* pAllocator = nullptr;
    if( m_AllocatorQueue.count() )
    {
        CommandAllocator*& pFront = m_AllocatorQueue.front();
        if( TestFence( m_pFence, *pFront ) )
        {
            ASSERT_HROK( pFront->pCmdAlloc->Reset(),
                         "Failed to reset CommandAllocator from queue." );
            pAllocator = pFront;
            m_AllocatorQueue.pop();
        }
    }

    if( !pAllocator )
    {
        pAllocator = g_CommandAllocators.push_new();
        CreateCommandAllocator( m_pDevice, m_Type, &pAllocator->pCmdAlloc );
    }

    if( m_ListsQueue.count() )
    {
        *ppOutCommandList = m_ListsQueue.front();
        m_ListsQueue.pop();

        ASSERT_HROK( ( *ppOutCommandList )->pList->Reset( pAllocator->pCmdAlloc, NULL ),
                     "Failed to reset CommandList from queue." );
    }
    else
    {
        *ppOutCommandList = g_CommandLists.push_new();
        CreateCommandList( m_pDevice, pAllocator->pCmdAlloc, m_Type,
                           &( *ppOutCommandList )->pList );
    }

    ( *ppOutCommandList )->pAllocator = pAllocator;
}

// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------
uint64 CommandQueue::ExecuteCommandList( CommandList* pCommandList )
{
    ASSERT_HROK( pCommandList->pList->Close(), "Failed to close command list" );

    ID3D12CommandList* const commandLists[] = { pCommandList->pList };
    m_pQueue->ExecuteCommandLists( ARRAYSIZE( commandLists ), commandLists );

    uint64 const uiFenceValue = Signal();
    pCommandList->pAllocator->uiFence = uiFenceValue;
    m_AllocatorQueue.push( pCommandList->pAllocator );
    m_ListsQueue.push( pCommandList );

    return uiFenceValue;
}

// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------
uint64 CommandQueue::Signal()
{
    uint64 const uiSignalFenceValue = ++m_uiFenceValue;
    ASSERT_HROK( m_pQueue->Signal( m_pFence, uiSignalFenceValue ),
                 "Failed to signal command queue" );
    return uiSignalFenceValue;
}

// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------
void CommandQueue::WaitForFence( uint64 uiFenceValue )
{
    WaitForFenceValue( m_pFence, uiFenceValue, m_FenceEvent );
}

// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------
void CommandQueue::Flush()
{
    WaitForFence( Signal() );
}

// INTERNALS (IMPLS) -------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------
static void CreateCommandAllocator( ID3D12Device2* pDevice, D3D12_COMMAND_LIST_TYPE type,
                                    ID3D12CommandAllocator** pOut )
{
    ASSERT_HROK( pDevice->CreateCommandAllocator( type, IID_PPV_ARGS( pOut ) ),
                 "Failed to create CommandAllocator" );
}

// -------------------------------------------------------------------------------------------------
static void CreateCommandList( ID3D12Device2* pDevice, ID3D12CommandAllocator* pCommandAllocator,
                               D3D12_COMMAND_LIST_TYPE type,
                               ID3D12GraphicsCommandList2** ppOutCommandList )
{
    ASSERT_HROK( pDevice->CreateCommandList( 0, type, pCommandAllocator, nullptr,
                                             IID_PPV_ARGS( ppOutCommandList ) ),
                 "Failed to create CommandList" );
    ASSERT_HROK( ( *ppOutCommandList )->Close(), "Failed to close command list" );
}

// -------------------------------------------------------------------------------------------------
static bool TestFence( ID3D12Fence* pFence, CommandAllocator const& alloc )
{
    return pFence->GetCompletedValue() >= alloc.uiFence;
}
// INTERNALS (IMPLS) -------------------------------------------------------------------------------

} // namespace Bogus::Renderer
