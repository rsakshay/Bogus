#ifndef RENDERER_H
#define RENDERER_H

#include "Globals.h"

namespace Bogus::App
{
}
namespace Bogus::Renderer
{
void Initialize();
void Render();
void Resize( uint32 uiWidth, uint32 uiHeight );
void Terminate();
} // namespace Bogus::Renderer
#endif
