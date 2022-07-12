#ifndef CORE_STRING_H
#define CORE_STRING_H
#include "Globals.h"
#include "assert.h"


namespace ASR
{

namespace Core
{

namespace String
{

// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
template<uint32 t_uiCapacity>
struct Buffer
{
public:
	enum { eCapacity = t_uiCapacity };

	template<uint32 t_uiStringLen>
	Buffer& operator=( char const ( &szString )[ t_uiStringLen ] )
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

	template<uint32 t_uiStringLen>
	Buffer& operator+=( char const ( &szString )[ t_uiStringLen ] )
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

	char const* c_str() { return m_pData; }

	void Terminate()
	{
		assert( m_uiLen < eCapacity );
		m_pData[ m_uiLen ] = 0;
	}

private:
	void append( char const* const pString, uint32 const uiLen )
	{
		assert( m_uiLen + uiLen <= eCapacity );
		uint32 uiAppendLen = MIN( uiLen, eCapacity );
		memcpy( &m_pData[ m_uiLen ], pString, uiAppendLen );
		m_uiLen += uiAppendLen;
	}

	uint32 m_uiLen;
	char   m_pData[ eCapacity ];
};

}

}

}
#endif