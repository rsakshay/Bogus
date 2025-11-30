#include "Core_Arena.h"
#include "Core_String.h"
#include "Core_Vector.h"
#include <iostream>

int main()
{
    using namespace Bogus::Core;
    Bogus::Core::String::Buffer<128> buffer;
    buffer = "MyBuffer";
    buffer.Terminate();
    buffer += " is amazing!";
    buffer.Terminate();
    std::cout << buffer.c_str();

    Arena arena = ArenaAlloc( 1024 * 1024 );
    constexpr uint32 MAX_ARRAY = 10;
    uint32* pMyArr = ArenaPushArray<uint32>( &arena, MAX_ARRAY * sizeof( uint32 ) );
    uint32 uiCount = 0;
    pMyArr[uiCount++] = 69;
    pMyArr[uiCount++] = 55;
    pMyArr[uiCount++] = 420;
    pMyArr[uiCount++] = 67;

    // Bogus::Core::Vector<uint32> myVec;
    // myVec.push_back( 69 );
    // myVec.push_back( 420 );
    // myVec.reserve( 20 );
    //  myVec[ 1 ] = 420;

    std::cout << "\n";

    printf( "\nArenaArray: %d", uiCount );
    printf( "\nSize: %d", uiCount );
    printf( "\nCapacity: %d", MAX_ARRAY );
    for( uint32 i = 0; i < uiCount; ++i )
    {
        uint32 uiItem = pMyArr[i];
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
    ArenaRelease( &arena );
}
