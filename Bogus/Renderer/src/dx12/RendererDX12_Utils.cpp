#include "dx12/RendererDX12_Utils.h"
#include "Core_Assert.h"

namespace Bogus::Renderer
{
// -------------------------------------------------------------------------------------------------
bool ASSERT_HROK( HRESULT hr, char const* szMsg )
{
    bool const bFailedHR = FAILED( hr );
    BGASSERT( bFailedHR, szMsg );
    return !bFailedHR;
}

// -------------------------------------------------------------------------------------------------
void WaitForFenceValue( ID3D12Fence* pFence, uint64 uiFenceValue, HANDLE hFenceEvent )
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

} // namespace Bogus::Renderer
