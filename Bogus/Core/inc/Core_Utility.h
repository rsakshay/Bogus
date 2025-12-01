#ifndef CORE_UTILITY_H
#define CORE_UTILITY_H
#include "Globals.h"
#include "MurmurHash3.h"

namespace Bogus
{
namespace Core
{

static uint32 Hash32( void const* pData, int const len )
{
    uint32 uiHash = 0;
    MurmurHash3_x86_32( pData, len, 0, &uiHash );
    return uiHash;
}

static uint64 AlignSize( uint64 uiSize, uint64 uiAlignment )
{
    uint64 uiAlignedSize = uiSize;
    uiAlignedSize += uiAlignment - 1;
    uiAlignedSize -= uiAlignedSize % uiAlignment;
    return uiAlignedSize;
}

} // namespace Core

} // namespace Bogus
#endif
