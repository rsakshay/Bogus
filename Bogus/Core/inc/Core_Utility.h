#ifndef CORE_UTILITY_H
#define CORE_UTILITY_H
#include "Globals.h"
#include "MurmurHash3.h"

namespace ASR
{
namespace Core
{

uint32 Hash32( void const* pData, int const len )
{
    uint32 uiHash = 0;
    MurmurHash3_x86_32( pData, len, 0, &uiHash );
    return uiHash;
}

}

}
#endif
