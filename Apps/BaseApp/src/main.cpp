#include "Core_String.h"
#include "Core_Utility.h"
#include "Core_Vector.h"
#include <iostream>

int main()
{
    using namespace ASR::Core;
    ASR::Core::String::Buffer<128> buffer;
    buffer = "MyBuffer";
    buffer.Terminate();
    buffer += " is amazing!";
    buffer.Terminate();
    std::cout << buffer.c_str();

    ASR::Core::Vector<uint32> myVec;
    myVec.push_back( 69 );
    myVec.push_back( 420 );
    myVec.reserve( 20 );
    // myVec[ 1 ] = 420;

    std::cout << "\n";

    printf( "\nSize: %d", myVec.size() );
    printf( "\nCapacity: %d", myVec.capacity() );
    uint32 uiIndex = 0;
    for( uint32 uiItem : myVec )
        printf( "\n[%u]: %u", uiIndex++, uiItem );

    using Uint32MapPair = ASR::Core::VectorMapPair<uint32, uint32>;
    using Uint32MapPairVector = ASR::Core::Vector<Uint32MapPair>;
    using Uint32Map = ASR::Core::VectorMap<Uint32MapPairVector>;

    Uint32Map myMap;
    myMap.add( 0, 69 );
    myMap.add( 1, 420 );
    myMap.add( 2, 911 );
    myMap.add( ASR::Core::String::CalcHash( "Key1" ), 1234 );

    for( Uint32MapPair const &Pair : myMap )
    {
        printf( "\n[%u]: %u", Pair.m_Key, Pair.m_Element );
    }

    getchar();
}
