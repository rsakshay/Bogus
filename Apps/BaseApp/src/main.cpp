// Bogus_Map.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "Core_String.h"
#include "Core_Vector.h"
#include "Core_Utility.h"

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
    //myVec[ 1 ] = 420;

    std::cout << "\n";

    printf( "\nSize: %d", myVec.size() );
    printf( "\nCapacity: %d", myVec.capacity() );
    uint32 uiIndex = 0;
    for( uint32 uiItem : myVec )
        printf( "\n[%u]: %u", uiIndex++, uiItem );

    using Uint32MapPair       = ASR::Core::VectorMapPair<uint32, uint32>;
    using Uint32MapPairVector = ASR::Core::Vector<Uint32MapPair>;
    using Uint32Map           = ASR::Core::VectorMap<Uint32MapPairVector>;

    Uint32Map myMap;
    myMap.add( 0, 69   );
    myMap.add( 1, 420  );
    myMap.add( 2, 911  );
    myMap.add( Hash32( "Key1", 4 ), 1234 );

    for( Uint32MapPair const& Pair : myMap )
    {
        printf( "\n[%u]: %u", Pair.m_Key, Pair.m_Element );
    }

    getchar();
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
