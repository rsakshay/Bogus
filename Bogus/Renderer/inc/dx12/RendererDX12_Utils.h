#ifndef RENDERERDX12_UTILS_H
#define RENDERERDX12_UTILS_H
#include "Globals.h"
#include "d3d12.h"
#include "windows.h"

namespace Bogus::Renderer
{
bool ASSERT_HROK( HRESULT hr, char const* szMsg );
void WaitForFenceValue( ID3D12Fence* pFence, uint64 uiFenceValue, HANDLE hFenceEvent );

template <typename T> static void DXRelease( T** ppObj )
{
    ( *ppObj )->Release();
    *ppObj = nullptr;
}

} // namespace Bogus::Renderer
#endif
