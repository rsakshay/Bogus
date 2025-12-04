#ifndef CORE_STRING_H
#define CORE_STRING_H
#include "Core_Utility.h"
#include "Globals.h"
#include "assert.h"
#include "string.h"

namespace Bogus
{

namespace Core
{

namespace String
{

// -----------------------------------------------------------------------
template <uint32 t_uiStrLen> uint32 CalcHash( char const ( &szString )[t_uiStrLen] )
{
    return Bogus::Core::Hash32( szString, t_uiStrLen );
}

// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
struct HashToken
{
    HashToken() {}
    HashToken( char const* pData, uint32 uiLen ) : m_pData( pData ), m_uiLen( uiLen )
    {
        m_uiHash = Bogus::Core::Hash32( pData, uiLen );
    }
    HashToken( char const* pData, uint32 uiLen, uint32 uiHash )
        : m_pData( pData ), m_uiLen( uiLen ), m_uiHash( uiHash )
    {
    }

    template <uint32 t_uiStrLen>
    HashToken( char const ( &szString )[t_uiStrLen] ) : HashToken( szString, t_uiStrLen )
    {
    }

    void set( char* pData, uint32 uiLen, uint32 uiHash )
    {
        m_pData = pData;
        m_uiLen = uiLen;
        m_uiHash = uiHash;
    }

    char const* m_pData = nullptr;
    uint32 m_uiLen = 0;
    uint32 m_uiHash = 0;
};

// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
template <uint32 t_uiCapacity> struct Buffer
{
  public:
    enum
    {
        eCapacity = t_uiCapacity
    };

    Buffer() = default;
    Buffer( Buffer const& other ) = default;

    template <uint32 t_uiStringLen> Buffer( char const ( &szString )[t_uiStringLen] )
    {
        // Note(asr): Account for null termination hence "-1"
        m_uiLen = MIN( t_uiStringLen - 1, eCapacity );
        memcpy( m_pData, szString, m_uiLen );
    }

    template <uint32 t_uiStringLen> Buffer& operator=( char const ( &szString )[t_uiStringLen] )
    {
        // Note(asr): Account for null termination hence "-1"
        m_uiLen = MIN( t_uiStringLen - 1, eCapacity );
        memcpy( m_pData, szString, m_uiLen );
        return *this;
    }

    Buffer& operator=( Buffer const& rhs )
    {
        m_uiLen = rhs.m_uiLen;
        memcpy( m_pData, rhs.m_pData, rhs.m_uiLen );
        return *this;
    }

    template <uint32 t_uiStringLen> Buffer& operator+=( char const ( &szString )[t_uiStringLen] )
    {
        // Note(asr): Account for null termination hence "-1"
        append( szString, t_uiStringLen - 1 );
        return *this;
    }

    Buffer& operator+=( Buffer& rhs )
    {
        append( rhs.m_pData, rhs.m_uiLen );
        return *this;
    }

    Buffer& operator=( Buffer&& rhs ) = default;

    char const* c_str()
    {
        assert( m_pData[m_uiLen] == 0 );
        return m_pData;
    }

    void Terminate()
    {
        assert( m_uiLen < eCapacity );
        m_pData[m_uiLen] = 0;
    }

    void append( char const* const pString, uint32 const uiLen )
    {
        assert( m_uiLen + uiLen <= eCapacity );
        uint32 uiAppendLen = MIN( uiLen, eCapacity );
        memcpy( &m_pData[m_uiLen], pString, uiAppendLen );
        m_uiLen += uiAppendLen;
    }

    uint32 m_uiLen;
    char m_pData[eCapacity];
};

} // namespace String

} // namespace Core

} // namespace Bogus
#endif
