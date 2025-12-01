#include "Core_Arena.h"
#include "Core_String.h"
#include "Core_Vector.h"
#include "stdio.h"

int main()
{
    using namespace Bogus::Core;
    Bogus::Core::String::Buffer<128> buffer;
    buffer = "MyBuffer";
    buffer.Terminate();
    buffer += " is amazing!";
    buffer.Terminate();
    printf( "%s\n\n", buffer.c_str() );

    Arena* pArena = NEW_ARENA(.tokName = "Thunderdome!!" );
    printf( "Arena Created with name: %.*s\n", pArena->initParams.tokName.m_uiLen,
            pArena->initParams.tokName.m_pData );

    constexpr uint32 MAX_CAPACITY = 10;
    Bogus::Core::VectorStatic<uint32> myVec = ArenaPushVector<uint32>( pArena, MAX_CAPACITY );
    myVec.push_back( 69 );
    myVec.push_back( 55 );
    myVec.push_back( 420 );
    myVec.push_back( 67 );

    uint32 uiCount = myVec.size();
    printf( "\nArenaVector: %d", uiCount );
    printf( "\nSize: %d", uiCount );
    printf( "\nCapacity: %d", MAX_CAPACITY );
    for( uint32 i = 0; i < uiCount; ++i )
    {
        uint32 uiItem = myVec[i];
        printf( "\n[%u]: %u", i, uiItem );
    }

    using Uint32MapPair = Bogus::Core::VectorMapPair<uint32, uint32>;
    using Uint32MapPairVector = Bogus::Core::Vector<Uint32MapPair>;
    using Uint32Map = Bogus::Core::VectorMap<Uint32MapPairVector>;

    Uint32Map myMap;
    myMap.add( 0, 69 );
    myMap.add( 1, 420 );
    myMap.add( 2, 911 );
    myMap.add( Bogus::Core::String::CalcHash( "Key1" ), 1234 );

    printf( "\n\nUint32Map: %d", uiCount );
    for( Uint32MapPair const& Pair : myMap )
    {
        printf( "\n[%u]: %u", Pair.m_Key, Pair.m_Element );
    }

    getchar();
    ArenaRelease( pArena );
}
