#include "Core_Arena.h"
#include "Core_String.h"
#include "Core_Vector.h"
#include "stdio.h"

void RunTest_StringBuffer()
{
    using namespace Bogus::Core;
    Bogus::Core::String::Buffer<128> buffer;
    buffer = "MyBuffer";
    buffer.Terminate();
    buffer += " is amazing!";
    buffer.Terminate();
    printf( "%s\n\n", buffer.c_str() );
}

void RunTest_Vector( Bogus::Core::Arena* pArena )
{
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
}

void RunTest_ElementPool( Bogus::Core::Arena* pArena )
{
    constexpr uint32 MAX_CAPACITY = 100;
    using namespace Bogus::Core;

    struct TestElem
    {
        uint32 uiID = 0;
        String::Buffer<128> name;
    };

    ElementPool<TestElem> pool = ArenaPushPool<TestElem>( pArena, MAX_CAPACITY );
    printf( "\nArenaElementPool created with capacity: %d", MAX_CAPACITY );

    auto CreateElem = [&]( TestElem elem, bool bSilent )
    {
        uint32 uiHandle = pool.Create();
        if( uiHandle == pool.INVALID )
        {
            printf( "\nArenaElementPool handle creation failed." );
            printf( "\n                             ID: %d", elem.uiID );
            printf( "\n                           Name: %.*s", elem.name.m_uiLen,
                    elem.name.m_pData );
        }
        else
        {
            if( !bSilent )
            {
                printf( "\nArenaElementPool handle created: %d", uiHandle );
                printf( "\n                             ID: %d", elem.uiID );
                printf( "\n                           Name: %.*s", elem.name.m_uiLen,
                        elem.name.m_pData );
            }
            pool[uiHandle] = elem;
        }
        return uiHandle;
    };

    auto DestroyElem = [&]( uint32 uiHandle )
    {
        if( pool.Destroy( uiHandle ) )
        {
            printf( "\nArenaElementPool handle destroyed: %d", uiHandle );
        }
        else
        {
            printf( "\nArenaElementPool failed to destroy handle: %d", uiHandle );
        }
    };

    uint32 uiNiceHandle = CreateElem( { 69, "Nice" }, false );
    CreateElem( { 55, "Wow" }, false );
    CreateElem( { 420, "Smokin" }, false );
    CreateElem( { 67, "Okay" }, false );

    DestroyElem( uiNiceHandle );
    if( pool.TryGet( uiNiceHandle ) )
    {
        BGASSERT( 0, "Handle was destroyed but we were still successful in getting the ptr." );
    }
    else
    {
        printf( "\nArenaElementPool TryGet failed for handle: %d", uiNiceHandle );
    }

    uint32 uiNewNiceHandle = CreateElem( { 6969, "New Nice" }, false );

    printf( "\nArenaElementPool creating test handles upto capacity: %d", MAX_CAPACITY );
    while( pool.count() < MAX_CAPACITY )
    {
        CreateElem( { 69, "Cloned Nice" }, true );
    }

    uint32 uiFailedHandle = CreateElem( { 101, "Failure" }, false );
    DestroyElem( 101 );
    DestroyElem( 69 );
    DestroyElem( 69 );
    uint32 uiNewTry = CreateElem( { 696969, "Nicest!" }, false );
}

void RunTest_Arena()
{
    using namespace Bogus::Core;
    Arena* pArena = NEW_ARENA(.tokName = "Thunderdome!!" );
    printf( "Arena Created with name: %.*s\n", pArena->initParams.tokName.m_uiLen,
            pArena->initParams.tokName.m_pData );

    RunTest_Vector( pArena );
    RunTest_ElementPool( pArena );
    ArenaRelease( pArena );
}

void RunTest_VectorMap()
{
    using namespace Bogus::Core;
    using Uint32MapPair = Bogus::Core::VectorMapPair<uint32, uint32>;
    using Uint32MapPairVector = Bogus::Core::Vector<Uint32MapPair>;
    using Uint32Map = Bogus::Core::VectorMap<Uint32MapPairVector>;

    Uint32Map myMap;
    myMap.add( 0, 69 );
    myMap.add( 1, 420 );
    myMap.add( 2, 911 );
    myMap.add( Bogus::Core::String::CalcHash( "Key1" ), 1234 );

    uint32 uiCount = 4;
    printf( "\n\nUint32Map: %d", uiCount );
    for( Uint32MapPair const& Pair : myMap )
    {
        printf( "\n[%u]: %u", Pair.m_Key, Pair.m_Element );
    }
}

int main()
{
    RunTest_StringBuffer();
    RunTest_Arena();
    RunTest_VectorMap();
    getchar();
}
