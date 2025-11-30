#ifndef RENDERERDX12_COMMANDLIST_H
#define RENDERERDX12_COMMANDLIST_H
#include "d3d12.h"

namespace Bogus::Renderer::CommandList
{
void GetAvailableCommandList( ID3D12CommandList** ppOutCommandList );
} // namespace Bogus::Renderer::CommandList
#endif
