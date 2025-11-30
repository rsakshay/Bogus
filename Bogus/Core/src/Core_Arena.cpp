#include "Core_Arena.h"
#include <cassert>
#include <cstdlib>
#include <cstring>

namespace Bogus
{
namespace Core
{

// ------------------------------------------------------
// ------------------------------------------------------
Arena ArenaAlloc( uint64 uiCapacity )
{
    Arena arena;
    arena.pMemory = (uint8*)malloc( uiCapacity );
    arena.uiSize = uiCapacity;
    return arena;
}

// ------------------------------------------------------
// ------------------------------------------------------
void ArenaRelease( Arena* pArena )
{
    free( pArena->pMemory );
    memset( pArena, 0, sizeof( Arena ) );
}

// ------------------------------------------------------
// ------------------------------------------------------
void ArenaSetAutoAlign( Arena* pArena, uint64 uiAlign )
{
    assert( 0 && "Not implemented" );
}

// ------------------------------------------------------
// ------------------------------------------------------
uint64 ArenaGetPos( Arena* pArena )
{
    return pArena->uiPos;
}

// ------------------------------------------------------
// ------------------------------------------------------
uint8* ArenaPushNoZero( Arena* pArena, uint64 uiSize )
{
    assert( pArena->uiPos + uiSize < pArena->uiSize );
    uint8* pNew = pArena->pMemory + pArena->uiPos;
    pArena->uiPos += uiSize;
    return pNew;
}

// ------------------------------------------------------
// ------------------------------------------------------
uint8* ArenaPushAligned( Arena* pArena, uint64 uiAlignment )
{
    assert( 0 && "Not implemented" );
    return nullptr;
}

// ------------------------------------------------------
// ------------------------------------------------------
uint8* ArenaPush( Arena* pArena, uint64 uiSize )
{
    uint8* pNew = ArenaPushNoZero( pArena, uiSize );
    memset( pNew, 0, uiSize );
    return pNew;
}

// ------------------------------------------------------
// ------------------------------------------------------
void ArenaPopTo( Arena* pArena, uint64 uiPos )
{
    assert( pArena->uiPos <= uiPos );
    assert( uiPos < pArena->uiSize );
    pArena->uiPos = uiPos;
}

// ------------------------------------------------------
// ------------------------------------------------------
void ArenaPop( Arena* pArena, uint64 uiSize )
{
    assert( pArena->uiPos >= uiSize );
    pArena->uiPos -= uiSize;
}

// ------------------------------------------------------
// ------------------------------------------------------
void ArenaClear( Arena* pArena )
{
    pArena->uiPos = 0;
}

} // namespace Core
} // namespace Bogus
