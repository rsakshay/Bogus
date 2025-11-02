#ifndef CORE_ARENA_H
#define CORE_ARENA_H
#include "Globals.h"

namespace ASR
{
namespace Core
{
// ------------------------------------------------------
struct Arena
{
    uint64 uiSize = 0;
    uint64 uiPos = 0;
    uint8* pMemory;
    uint8* pAllocated; // unused
    uint8* pCommitted; // unused
};

Arena ArenaAlloc( uint64 uiCapacity );
void ArenaRelease( Arena* pArena );
void ArenaSetAutoAlign( Arena* pArena, uint64 uiAlign );

uint64 ArenaGetPos( Arena* pArena );

uint8* ArenaPushNoZero( Arena* pArena, uint64 uiSize );
uint8* ArenaPushAligned( Arena* pArena, uint64 uiAlignment );
uint8* ArenaPush( Arena* pArena, uint64 uiSize );

void ArenaPopTo( Arena* pArena, uint64 uiPos );
void ArenaPop( Arena* pArena, uint64 uiSize );
void ArenaClear( Arena* pArena );

template <typename T> T* ArenaPushArray( Arena* pArena, uint32 uiCount )
{
    return reinterpret_cast<uint32*>( ArenaPush( pArena, sizeof( T ) * uiCount ) );
}

template <typename T> T* ArenaPushArrayNoZero( Arena* pArena, uint32 uiCount )
{
    return reinterpret_cast<uint32*>( ArenaPushNoZero( pArena, sizeof( T ) * uiCount ) );
}

} // namespace Core
} // namespace ASR
#endif
