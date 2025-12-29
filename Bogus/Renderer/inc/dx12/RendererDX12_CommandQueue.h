#ifndef RENDERERDX12_COMMANDQUEUE_H
#define RENDERERDX12_COMMANDQUEUE_H
#include "Core_Vector.h"
#include "Globals.h"
#include "d3d12.h"

namespace Bogus::Renderer
{
static constexpr uint32 MAX_FRAMES = 3;

struct CommandAllocator
{
    uint64 uiFence = 0;
    ID3D12CommandAllocator* pCmdAlloc;
};

struct CommandList
{
    ID3D12GraphicsCommandList2* pList;
    CommandAllocator* pAllocator;
};

class CommandQueue
{
  public:
    void Initialize( ID3D12Device2* pDevice, D3D12_COMMAND_LIST_TYPE type );
    void Terminate();

    void GetCommandList( CommandList** ppOutCommandList );
    uint64 ExecuteCommandList( CommandList* pCommandList );
    uint64 Signal();

    void WaitForFence( uint64 uiFenceValue );
    void Flush();

    ID3D12Device2* m_pDevice;
    ID3D12CommandQueue* m_pQueue;
    D3D12_COMMAND_LIST_TYPE m_Type;
    ID3D12Fence* m_pFence;
    HANDLE m_FenceEvent;
    uint64 m_uiFenceValue = 0;
    Core::HeapQueue<CommandAllocator*, MAX_FRAMES * 2> m_AllocatorQueue;
    Core::HeapQueue<CommandList*, MAX_FRAMES * 2> m_ListsQueue;
};

} // namespace Bogus::Renderer
#endif
