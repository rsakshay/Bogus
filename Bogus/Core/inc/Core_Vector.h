#ifndef CORE_VECTOR_H
#define CORE_VECTOR_H
#include "Core_Arena.h"
#include "Core_Assert.h"
#include "Globals.h"
#include <new.h>
#include <stdlib.h>

namespace Bogus
{

namespace Core
{

template <typename tElemType, uint32 uiGrowthSize = 16,
          uint64 uiDesiredCapacity = ARENA_DEFAULT_RESERVE_SIZE / sizeof( tElemType )>
struct VectorPolicyArena
{
    using ELEMTYPE = tElemType;

    VectorPolicyArena()
    {
        m_pArena = ArenaAlloc( { ( uiDesiredCapacity * sizeof( ELEMTYPE ) ) + ARENA_HEADER_SIZE,
                                 uiGrowthSize * sizeof( ELEMTYPE ), "VectorArena" } );
        m_uiCapacity = ( m_pArena->uiReservedSize - m_pArena->uiBasePos ) / sizeof( ELEMTYPE );
    }

    ~VectorPolicyArena() { ArenaRelease( m_pArena ); }

    ELEMTYPE* pData() { return (ELEMTYPE*)ArenaGetBegin( m_pArena ); }
    ELEMTYPE const* pData() const { return (ELEMTYPE*)ArenaGetBegin( m_pArena ); }
    uint32 const size() const { return m_uiSize; }
    uint32 const capacity() const { return m_uiCapacity; }

    ELEMTYPE* push_new()
    {
        ELEMTYPE* pData = (ELEMTYPE*)ArenaPush( m_pArena, sizeof( ELEMTYPE ), 1 );
        if( pData )
        {
            new( pData ) ELEMTYPE();
            ++m_uiSize;
        }
        BGASSERT( pData, "Failed to add new element. Arena Allocation ran out of memory." );
        return pData;
    }

    void pop()
    {
        if( m_uiSize == 0 )
        {
            BGASSERT( 0, "Failed to pop element. Vector size is 0." );
            return;
        }

        --m_uiSize;
        ArenaPop( m_pArena, sizeof( ELEMTYPE ) );
    }

    uint32 m_uiFlags = 0;
    uint32 m_uiSize = 0;
    uint32 m_uiCapacity = 0;
    Arena* m_pArena;
};

template <typename tElemType> struct VectorPolicyStatic
{
    using ELEMTYPE = tElemType;

    VectorPolicyStatic( ELEMTYPE* pData, uint32 uiCapacity )
        : m_pData( pData ), m_uiCapacity( uiCapacity )
    {
    }

    ~VectorPolicyStatic() {}

    ELEMTYPE* pData() { return m_pData; }
    ELEMTYPE const* pData() const { return pData(); }
    uint32 const size() const { return m_uiSize; }
    uint32 const capacity() const { return m_uiCapacity; }

    ELEMTYPE* push_new()
    {
        if( m_uiSize == capacity() )
        {
            BGASSERT( 0, "Failed to add new element. Static Allocation ran out of memory." );
            return nullptr;
        }

        new( &m_pData[m_uiSize] ) ELEMTYPE();
        return &m_pData[m_uiSize++];
    }

    void pop() { --m_uiSize; }

    ELEMTYPE* m_pData;
    uint32 m_uiSize = 0;
    uint32 m_uiCapacity = 0;
};

// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
template <typename tElemAllocator> struct Vector : tElemAllocator
{
    using ELEMTYPE = tElemAllocator::ELEMTYPE;
    using tElemAllocator::capacity;
    using tElemAllocator::pData;
    using tElemAllocator::pop;
    using tElemAllocator::push_new;
    using tElemAllocator::size;
    enum
    {
        eInvalidIndex = max_uint32,
    };

    using iterator = ELEMTYPE*;
    using const_iterator = ELEMTYPE const*;
    iterator begin() { return pData(); }
    iterator end() { return pData() + size(); }
    const_iterator begin() const { return pData(); }
    const_iterator end() const { return pData() + size(); }

    ELEMTYPE& operator[]( uint32 const uiIndex )
    {
        assert( uiIndex < size() );
        return pData()[uiIndex];
    }
    ELEMTYPE const& operator[]( uint32 const uiIndex ) const
    {
        assert( uiIndex < size() );
        return pData()[uiIndex];
    }

    ELEMTYPE const& front() const { return pData()[0]; }
    ELEMTYPE const& back() const { return pData()[size() - 1]; }

    void push( ELEMTYPE const& in_Element )
    {
        ELEMTYPE* pNewElement = push_new();
        if( pNewElement )
        {
            *pNewElement = in_Element;
        }
    }

    uint32 find( ELEMTYPE const& in_data ) const
    {
        for( uint32 i = 0; i < size(); ++i )
        {
            if( pData[i] == in_data )
                return i;
        }
        return eInvalidIndex;
    }

    void remove( uint32 uiIndex )
    {
        if( uiIndex >= size() )
        {
            BGASSERT( 0, "Index OOB" );
            return;
        }

        for( uint32 i = uiIndex; i < size() - 1; ++i )
        {
            pData[i] = pData[i + 1];
        }
        pop();
    }

    void remove_elem( ELEMTYPE const& kElem )
    {
        uint32 const uiIndex = find( kElem );
        if( uiIndex == eInvalidIndex )
        {
            BGASSERT( 0, "Element not found when trying to remove." );
            return;
        }
        remove( uiIndex );
    }
};

// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
template <typename tElemType> struct ElementPool
{
    using ELEMTYPE = tElemType;
    static constexpr uint32 INVALID = max_uint32;
    static constexpr uint32 DEAD_ENTRY = 0xdeadffff;

    struct DeadEntry
    {
        uint32 uiDeadMark = DEAD_ENTRY;
        uint32 uiNextFree = INVALID;
    };
    static_assert( sizeof( ELEMTYPE ) >= sizeof( DeadEntry ),
                   "Element size must be bigger than DeadEntry" );

    ElementPool() = default;
    ElementPool( Arena* pArena ) : m_Vec( { pArena } ) {}

    template <typename tFunc> void ForEachElement( tFunc func )
    {
        for( uint32 i = 0; i < m_Vec.size(); ++i )
        {
            DeadEntry* pEntry = reinterpret_cast<DeadEntry*>( &m_Vec[i] );
            if( pEntry->uiDeadMark == DEAD_ENTRY )
            {
                continue;
            }

            func( i, &m_Vec[i] );
        }
    }

    uint32 Create()
    {
        // Reuse a dead entry if available
        if( m_uiNextFree != INVALID )
        {
            uint32 uiReusedHandle = m_uiNextFree;

            // Patch up m_uiNextFree
            DeadEntry* pEntry = reinterpret_cast<DeadEntry*>( &m_Vec[uiReusedHandle] );
            BGASSERT( pEntry->uiDeadMark == DEAD_ENTRY, "Free handle is not marked dead." );
            m_uiNextFree = pEntry->uiNextFree;

            new( &m_Vec[uiReusedHandle] ) ELEMTYPE();
            ++m_uiCount;
            return uiReusedHandle;
        }

        ELEMTYPE* pNew = m_Vec.push_new();
        if( !pNew )
        {
            return INVALID;
        }

        ++m_uiCount;
        return m_Vec.size() - 1;
    }

    bool Destroy( uint32 uiHandle )
    {
        if( uiHandle >= m_Vec.size() )
        {
            BGASSERT( 0, "Bad Handle passed in to ElementPool::Destroy" );
            return false;
        }

        DeadEntry* pEntry = reinterpret_cast<DeadEntry*>( &m_Vec[uiHandle] );
        if( pEntry->uiDeadMark == DEAD_ENTRY )
        {
            BGASSERT( 0, "Double delete. Handle is already dead." );
            return false;
        }

        pEntry->uiDeadMark = DEAD_ENTRY;
        pEntry->uiNextFree = m_uiNextFree;
        m_uiNextFree = uiHandle;
        --m_uiCount;
        return true;
    }

    ELEMTYPE* Get( uint32 uiHandle )
    {
        if( uiHandle >= m_Vec.size() )
        {
            BGASSERT( 0, "Bad handle when getting data from handle." );
            return nullptr;
        }
        DeadEntry* pEntry = reinterpret_cast<DeadEntry*>( &m_Vec[uiHandle] );
        BGASSERT( pEntry->uiDeadMark != DEAD_ENTRY, "Bad pool access. Getting dead Handle." );
        return &m_Vec[uiHandle];
    }

    ELEMTYPE* TryGet( uint32 uiHandle )
    {
        if( uiHandle >= m_Vec.size() )
        {
            return nullptr;
        }
        DeadEntry* pEntry = reinterpret_cast<DeadEntry*>( &m_Vec[uiHandle] );
        if( pEntry->uiDeadMark == DEAD_ENTRY )
        {
            return nullptr;
        }
        return &m_Vec[uiHandle];
    }

    ELEMTYPE& operator[]( uint32 const uiHandle ) { return *Get( uiHandle ); }
    ELEMTYPE const& operator[]( uint32 const uiHandle ) const { return *Get( uiHandle ); }
    uint32 const count() const { return m_uiCount; }

    Vector<VectorPolicyArena<ELEMTYPE>> m_Vec;
    uint32 m_uiNextFree = INVALID;
    uint32 m_uiCount = 0;
};

// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
template <typename tKey, typename tElement> struct VectorMapPair
{
    using KEY = tKey;
    using ELEMENT = tElement;

    VectorMapPair() {}
    VectorMapPair( KEY const& key, ELEMENT const& element )
    {
        m_Key = key;
        m_Element = element;
    }

    KEY m_Key;
    ELEMENT m_Element;
};

// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
template <typename tPairVector> struct VectorMap
{
    using VEC = tPairVector;
    using PAIR = typename VEC::ELEMTYPE;
    using KEY = typename VEC::ELEMTYPE::KEY;
    using ELEMTYPE = typename VEC::ELEMTYPE::ELEMENT;
    using iterator = typename VEC::iterator;

    static constexpr PAIR* s_InvalidEntry = nullptr;
    enum
    {
        eInvalidIndex = max_uint32
    };

    uint32 add( KEY const& Key, ELEMTYPE const& Element, uint32* pExists = 0 )
    {
        return add( PAIR( Key, Element ), pExists );
    }
    uint32 add( PAIR const& Pair, uint32* pExists = 0 )
    {
        uint32 uiIndex = find( Pair.m_Key );
        if( uiIndex != eInvalidIndex )
        {
            if( pExists )
                *pExists = 1;
            return uiIndex;
        }

        m_Vec.push( Pair );
        return m_Vec.size() - 1;
    }

    // PAIR& add( PAIR const& Pair, uint32* pExists = 0 )
    //{
    //     PAIR* pPair = find( Pair.m_Key );
    //     if( pPair == s_InvalidEntry )
    //     {
    //         m_Vec.push_back( Pair );
    //         pPair = &m_Vec.back();
    //     }
    //     else
    //     {
    //         if( pExists )
    //             *pExists = 1;
    //     }

    //    return *pPair;
    //}

    // PAIR const* find( KEY const& key ) const
    //{
    //     for( uint32 i = 0; i < m_Vec.size(); ++i  )
    //     {
    //         PAIR const& Pair = m_Vec[ i ];
    //         if( Pair.m_Key == key )
    //             return &Pair;
    //     }
    //     return s_InvalidEntry;
    // }

    uint32 find( KEY const& key ) const
    {
        for( uint32 i = 0; i < m_Vec.size(); ++i )
        {
            PAIR const& Pair = m_Vec[i];
            if( Pair.m_Key == key )
                return i;
        }
        return eInvalidIndex;
    }

    ELEMTYPE& get_data( uint32 const uiIndex ) const
    {
        assert( uiIndex < m_Vec.size() );
        return m_Vec[uiIndex].m_Element;
    }

    ELEMTYPE& find_data( KEY const& key ) const { return m_Vec[find( key )].m_Element; }

    iterator begin() { return m_Vec.begin(); }
    iterator end() { return m_Vec.end(); }

  private:
    VEC m_Vec;
};

} // namespace Core

} // namespace Bogus

#endif
