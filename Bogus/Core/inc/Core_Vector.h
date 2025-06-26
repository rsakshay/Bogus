#ifndef CORE_VECTOR_H
#define CORE_VECTOR_H
#include "Globals.h"
#include "stdlib.h"

namespace ASR
{

namespace Core
{


// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
template<typename tElemType, uint32 uiGrowthSize = 8>
struct Vector
{
    using ELEMTYPE = tElemType;
    enum
    {
        eGrowthSize   = uiGrowthSize,
        eInvalidIndex = max_uint32,
    };

    struct iterator
    {
        iterator() : m_pElement( 0 ) {}
        explicit iterator( ELEMTYPE* pElem ) : m_pElement( pElem ) {}
        iterator( iterator const& rhs ) : m_pElement( rhs.m_pElement ) {}
        ~iterator() { m_pElement = 0; }

        iterator& operator++()          { ++m_pElement; return *this; }
        iterator& operator--()          { --m_pElement; return *this; }
        iterator& operator++( int )     { iterator temp = *this; ++(*this); return temp; }
        iterator& operator--( int )     { iterator temp = *this; --(*this); return temp; }
        iterator& operator+=( int i )   { m_pElement += i; return *this; }
        iterator& operator-=( int i )   { m_pElement -= i; return *this; }
        ELEMTYPE& operator*()           { return *m_pElement; }
        ELEMTYPE* operator->()          { return m_pElement; }

        friend bool     operator==( iterator const& lhs, iterator const& rhs ) { return lhs.m_pElement == rhs.m_pElement; }
        friend bool     operator!=( iterator const& lhs, iterator const& rhs ) { return lhs.m_pElement != rhs.m_pElement; }
        friend iterator operator+(  iterator& lhs, iterator const& rhs )       { lhs += rhs; return lhs; }
        friend iterator operator-(  iterator& lhs, iterator const& rhs )       { lhs -= rhs; return lhs; }
        friend iterator operator+(  iterator& lhs, int const& rhs )            { lhs += rhs; return lhs; }
        friend iterator operator-(  iterator& lhs, int const& rhs )            { lhs -= rhs; return lhs; }

    private:
        ELEMTYPE* m_pElement;
    };

    iterator begin() { return iterator( m_pData ); }
    iterator end()   { return iterator( m_pData + m_uiSize ); }

    ELEMTYPE&       operator[]( uint32 const uiIndex )     { assert( uiIndex < m_uiSize ); return m_pData[ uiIndex ]; }
    ELEMTYPE const& operator[](uint32 const uiIndex) const { assert( uiIndex < m_uiSize ); return m_pData[ uiIndex ]; }

    ELEMTYPE const& front() const { return m_pData[ 0 ]; }
    ELEMTYPE const& back() const  { return m_pData[ m_uiSize - 1 ]; }

    uint32 const size()     const { return m_uiSize;     }
    uint32 const capacity() const { return m_uiCapacity; }

    ELEMTYPE* push_back_new()
    {
        if( m_uiSize == capacity() )
            reserve( m_uiSize + 1 );

        new( &m_pData[ m_uiSize ] ) ELEMTYPE();
        return &m_pData[ m_uiSize++ ];
    }

    void push_back( ELEMTYPE const& in_Element )
    {
        ELEMTYPE* pNewElement = push_back_new();
        *pNewElement = in_Element;
    }

    void resize( uint32 const uiSize )
    {
        if( m_uiSize == uiSize )
            return;

        if( m_uiCapacity < uiSize )
            move( uiSize );

        if( m_uiSize < uiSize )
        {
            for( uint32 i = m_uiSize; i < uiSize; i++ )
                new( &m_pData[ i ] ) ELEMTYPE();
        }
        else
        {
            for( uint32 i = uiSize; i < m_uiSize; i++ )
                m_pData[ i ].~ELEMTYPE();
        }

        m_uiSize = uiSize;
    }

    void reserve( uint32 const uiCapacity )
    {
        if( m_uiCapacity >= uiCapacity )
            return;

        move( POW2_ROUNDUP( uiCapacity, eGrowthSize ) );
    }

    uint32 find_index( ELEMTYPE const& in_data ) const
    {
        for( uint32 i = 0; i < m_uiSize; ++i )
        {
            if( m_pData[ i ] == in_data )
                return i;
        }
        return eInvalidIndex;
    }

private:
    void move( uint32 const uiNewCapacity )
    {
        ELEMTYPE* pNewData = static_cast<ELEMTYPE*>( malloc( sizeof( ELEMTYPE ) * ( uiNewCapacity ) ) );
        assert( pNewData );
        if( m_pData )
        {
            assert( uiNewCapacity > m_uiSize );
            memcpy( pNewData, m_pData, sizeof( ELEMTYPE ) * m_uiSize );
            free( m_pData );
        }
        m_pData = pNewData;
        m_uiCapacity = uiNewCapacity;
    }

    ELEMTYPE* m_pData     = nullptr;
    uint32   m_uiSize     = 0;
    uint32   m_uiCapacity = 0;
};


// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
template<typename tKey, typename tElement>
struct VectorMapPair
{
    using KEY     = tKey;     
    using ELEMENT = tElement;

    VectorMapPair() {}
    VectorMapPair( KEY const& key, ELEMENT const& element )
    {
        m_Key = key;
        m_Element = element;
    }

    KEY     m_Key;
    ELEMENT m_Element;
};


// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
template<typename tPairVector>
struct VectorMap
{
    using VEC      = tPairVector;
    using PAIR     = typename VEC::ELEMTYPE;
    using KEY      = typename VEC::ELEMTYPE::KEY;
    using ELEMTYPE = typename VEC::ELEMTYPE::ELEMENT;
    using iterator = typename VEC::iterator;

    static constexpr PAIR* s_InvalidEntry = nullptr;
    enum { eInvalidIndex = max_uint32 };

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

        m_Vec.push_back( Pair );
        return m_Vec.size() - 1;
    }

    //PAIR& add( PAIR const& Pair, uint32* pExists = 0 )
    //{
    //    PAIR* pPair = find( Pair.m_Key );
    //    if( pPair == s_InvalidEntry )
    //    {
    //        m_Vec.push_back( Pair );
    //        pPair = &m_Vec.back();
    //    }
    //    else
    //    {
    //        if( pExists )
    //            *pExists = 1;
    //    }

    //    return *pPair;
    //}

    //PAIR const* find( KEY const& key ) const
    //{
    //    for( uint32 i = 0; i < m_Vec.size(); ++i  )
    //    {
    //        PAIR const& Pair = m_Vec[ i ];
    //        if( Pair.m_Key == key )
    //            return &Pair;
    //    }
    //    return s_InvalidEntry;
    //}

    uint32 find( KEY const& key ) const
    {
        for( uint32 i = 0; i < m_Vec.size(); ++i  )
        {
            PAIR const& Pair = m_Vec[ i ];
            if( Pair.m_Key == key )
                return i;
        }
        return eInvalidIndex;
    }

    ELEMTYPE& get_data( uint32 const uiIndex ) const
    {
        assert( uiIndex < m_Vec.size() );
        return m_Vec[ uiIndex ].m_Element;
    }

    ELEMTYPE& find_data( KEY const& key ) const
    {
        return m_Vec[ find( key ) ].m_Element;
    }

    iterator begin() { return m_Vec.begin(); }
    iterator end() { return m_Vec.end(); }

private:
    VEC m_Vec;
};

}

}


#endif