#include "Core_Arena.h"
#include "Core_Assert.h"
#include "Core_Memory.h"
#include "Core_Utility.h"
#include "Globals.h"

namespace Bogus
{
namespace Core
{

// ------------------------------------------------------
// ------------------------------------------------------
Arena* ArenaAlloc( ArenaAllocParams const& params )
{
    uint64 const uiPageSize = Memory::GetPageSize();
    uint64 const uiReserveSize = ALIGNUP_POW2( params.uiReserveSize, uiPageSize );
    uint64 const uiCommitSize = ALIGNUP_POW2( params.uiCommitSize, uiPageSize );

    uint8* pMem = (uint8*)Memory::Reserve( uiReserveSize );
    Memory::Commit( pMem, uiCommitSize );
    if( pMem == 0 )
    {
        BGASSERT( 0, "Failed to Reserve pMemory" );
        return nullptr;
    }

    Arena* pArena = (Arena*)pMem;
    pArena->initParams = params;
    pArena->uiPos = ARENA_HEADER_SIZE;
    pArena->uiBasePos = pArena->uiPos;
    pArena->uiReservedSize = uiReserveSize;
    pArena->uiCommittedSize = uiCommitSize;
    ArenaPush( pArena, 0, ALIGNOF( Arena ) );

    return pArena;
}

// ------------------------------------------------------
// ------------------------------------------------------
void ArenaRelease( Arena* pArena )
{
    Memory::Release( pArena, pArena->uiReservedSize );
}

// ------------------------------------------------------
// ------------------------------------------------------
uint64 ArenaGetPos( Arena* pArena )
{
    return pArena->uiPos;
}

// ------------------------------------------------------
// ------------------------------------------------------
uint8* ArenaGetBegin( Arena* pArena )
{
    return (uint8*)pArena + pArena->uiBasePos;
}

// ------------------------------------------------------
// ------------------------------------------------------
uint8* ArenaPush( Arena* pArena, uint64 uiSize, uint64 uiAlignment )
{
    uint64 uiCurrentPos = ALIGNUP_POW2( pArena->uiPos, uiAlignment );
    uint64 uiNewPos = uiCurrentPos + uiSize;

    // NOTE(asr): Commit new pages if necessary
    if( pArena->uiCommittedSize < uiNewPos )
    {
        uint64 uiNewCommitSizeAligned = AlignSize( uiNewPos, pArena->initParams.uiCommitSize );
        uint64 uiNewCommitSizeClamped = MIN( uiNewCommitSizeAligned, pArena->uiReservedSize );
        uint64 uiCommitSize = uiNewCommitSizeClamped - pArena->uiCommittedSize;
        uint8* pCommitted = (uint8*)pArena + pArena->uiCommittedSize;
        Memory::Commit( pCommitted, uiCommitSize );
        pArena->uiCommittedSize = uiNewCommitSizeClamped;
    }

    uint8* pMem = 0;
    if( pArena->uiCommittedSize >= uiNewPos )
    {
        pMem = (uint8*)pArena + uiCurrentPos;
        pArena->uiPos = uiNewPos;
    }

    if( pMem == 0 )
    {
        BGASSERT( 0, "Failed to allocate memory" );
    }

    return pMem;
}

// ------------------------------------------------------
// ------------------------------------------------------
void ArenaPopTo( Arena* pArena, uint64 uiPos )
{
    BGASSERT( pArena->uiPos >= uiPos, "Attempting to pop memory that is already popped." );
    BGASSERT( uiPos < pArena->uiCommittedSize, "Attempting to pop memory that is not committed" );
    uiPos = MAX( ARENA_HEADER_SIZE, uiPos );
    pArena->uiPos = uiPos;
}

// ------------------------------------------------------
// ------------------------------------------------------
void ArenaPop( Arena* pArena, uint64 uiSize )
{
    uint64 uiCurrentPos = ArenaGetPos( pArena );
    uint64 uiNewPos = uiCurrentPos;
    if( uiSize < uiCurrentPos )
    {
        uiNewPos = uiCurrentPos - uiSize;
    }
    ArenaPopTo( pArena, uiNewPos );
}

// ------------------------------------------------------
// ------------------------------------------------------
void ArenaClear( Arena* pArena )
{
    ArenaPopTo( pArena, 0 );
}

} // namespace Core
} // namespace Bogus
