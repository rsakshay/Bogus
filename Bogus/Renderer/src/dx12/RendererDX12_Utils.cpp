#include "dx12/RendererDX12_Utils.h"
#include "Core_Assert.h"

namespace Bogus::Renderer
{
bool ASSERT_HROK( HRESULT hr, char const* szMsg )
{
    bool const bFailedHR = FAILED( hr );
    BGASSERT( bFailedHR, szMsg );
    return !bFailedHR;
}
} // namespace Bogus::Renderer
