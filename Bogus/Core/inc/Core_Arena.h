#ifndef CORE_ARENA_H
#define CORE_ARENA_H
#include "Core_String.h"
#include "Core_Vector.h"
#include "Globals.h"

namespace Bogus
{
namespace Core
{
static constexpr uint64 ARENA_DEFAULT_RESERVE_SIZE = MEGABYTES( 64 );
static constexpr uint64 ARENA_DEFAULT_COMMIT_SIZE = KILOBYTES( 64 );

// ------------------------------------------------------
struct ArenaAllocParams
{
    uint64 uiReserveSize = ARENA_DEFAULT_RESERVE_SIZE;
    uint64 uiCommitSize = ARENA_DEFAULT_COMMIT_SIZE;
    String::HashToken tokName;
};

// ------------------------------------------------------
struct Arena
{
    ArenaAllocParams initParams;
    uint64 uiPos = 0;
    uint64 uiCommittedSize = 0;
    uint64 uiReservedSize = 0;
};
static constexpr uint32 ARENA_HEADER_SIZE = sizeof( Arena );

Arena* ArenaAlloc( ArenaAllocParams const& params );
#define NEW_ARENA( ... )                                                                           \
    ArenaAlloc( { .uiReserveSize = ARENA_DEFAULT_RESERVE_SIZE,                                     \
                  .uiCommitSize = ARENA_DEFAULT_COMMIT_SIZE,                                       \
                  __VA_ARGS__ } )
void ArenaRelease( Arena* pArena );

uint64 ArenaGetPos( Arena* pArena );

uint8* ArenaPush( Arena* pArena, uint64 uiSize, uint64 uiAlignment );

void ArenaPopTo( Arena* pArena, uint64 uiPos );
void ArenaPop( Arena* pArena, uint64 uiSize );
void ArenaClear( Arena* pArena );

template <typename T>
T* ArenaPushArrayNoZeroAligned( Arena* pArena, uint32 uiCount, uint64 uiAlignment )
{
    return reinterpret_cast<T*>( ArenaPush( pArena, sizeof( T ) * uiCount, uiAlignment ) );
}

template <typename T> T* ArenaPushArrayAligned( Arena* pArena, uint32 uiCount, uint64 uiAlignment )
{
    uint64 const uiTotalSize = sizeof( T ) * uiCount;
    T* pData = reinterpret_cast<T*>( ArenaPush( pArena, uiTotalSize, uiAlignment ) );
    memset( pData, 0, uiTotalSize );
    return pData;
}

template <typename T> T* ArenaPushArrayNoZero( Arena* pArena, uint32 uiCount )
{
    return ArenaPushArrayNoZeroAligned<T>( pArena, uiCount, MAX( 8, ALIGNOF( T ) ) );
}

template <typename T> T* ArenaPushArray( Arena* pArena, uint32 uiCount )
{
    return ArenaPushArrayAligned<T>( pArena, uiCount, MAX( 8, ALIGNOF( T ) ) );
}

template <typename T> VectorStatic<T> ArenaPushVectorNoZero( Arena* pArena, uint32 uiCapacity )
{
    T* pData = ArenaPushArrayNoZero<T>( pArena, uiCapacity );
    return VectorStatic<T>( pData, uiCapacity );
}

template <typename T> VectorStatic<T> ArenaPushVector( Arena* pArena, uint32 uiCapacity )
{
    T* pData = ArenaPushArray<T>( pArena, uiCapacity );
    return VectorStatic<T>( pData, uiCapacity );
}

template <typename T> ElementPool<T> ArenaPushPoolNoZero( Arena* pArena, uint32 uiCapacity )
{
    T* pData = ArenaPushArrayNoZero<T>( pArena, uiCapacity );
    return ElementPool<T>( pData, uiCapacity );
}

template <typename T> ElementPool<T> ArenaPushPool( Arena* pArena, uint32 uiCapacity )
{
    T* pData = ArenaPushArray<T>( pArena, uiCapacity );
    return ElementPool<T>( pData, uiCapacity );
}

} // namespace Core
} // namespace Bogus
#endif
