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

void RunTest_VectorStatic( Bogus::Core::Arena* pArena )
{
    printf( "\n\nTesting Static Vector..." );
    using namespace Bogus::Core;

    constexpr uint32 MAX_CAPACITY = 10;
    uint32* pData = ArenaPushArray<uint32>( pArena, MAX_CAPACITY );
    Vector<VectorPolicyStatic<uint32>> myVec( { pData, MAX_CAPACITY } );

    myVec.push( 69 );
    myVec.push( 55 );
    myVec.push( 420 );
    myVec.push( 67 );

    uint32 uiCount = myVec.size();
    printf( "\nSize: %d", uiCount );
    printf( "\nCapacity: %d", myVec.capacity() );

    for( uint32 i = 0; i < uiCount; ++i )
    {
        uint32 uiItem = myVec[i];
        printf( "\n[%u]: %u", i, uiItem );
    }

    printf( "\nRunning fill test..." );
    for( uint32 i = uiCount; i < MAX_CAPACITY; ++i )
    {
        myVec.push( 0xBEEF );
        printf( "\n[%u]: %x", i, myVec[i] );
    }
    myVec.push( 0xDEAD );
}

void RunTest_QueueStatic( Bogus::Core::Arena* pArena )
{
    printf( "\n\nTesting Static Queue..." );
    using namespace Bogus::Core;

    constexpr uint32 MAX_CAPACITY = 8;
    uint32* pData = ArenaPushArray<uint32>( pArena, MAX_CAPACITY );
    Queue<VectorPolicyStatic<uint32>> queue( { pData, MAX_CAPACITY } );

    queue.push( 69 );
    queue.push( 55 );
    queue.push( 420 );
    queue.push( 67 );
    for( uint32 i = 0; i < 4; ++i )
    {
        queue.push( 0xBEEF );
    }

    uint32 uiCount = queue.count();
    printf( "\nCount: %d", uiCount );
    printf( "\nCapacity: %d", queue.capacity() );

    for( uint32 i = 0; i < uiCount; ++i )
    {
        uint32 uiItem = queue[i];
        printf( "\n[%u]: %u", i, uiItem );
    }

    uint32 const uiElementsToPop = ( uiCount / 2 ) - 1;
    for( uint32 i = 0; i < uiElementsToPop; ++i )
    {
        queue.pop();
    }

    printf( "\nPopped first %d elements.", uiElementsToPop );

    for( uint32 i = 0; i < queue.count(); ++i )
    {
        uint32 uiItem = queue[i];
        printf( "\n[%u]: %u", i, uiItem );
    }
}

void RunTest_VectorArena()
{
    printf( "\n\nTesting Arena Vector..." );
    using namespace Bogus::Core;

    Vector<VectorPolicyArena<uint32>> myVec;

    myVec.push( 69 );
    myVec.push( 55 );
    myVec.push( 420 );
    myVec.push( 67 );

    uint32 uiCount = myVec.size();
    printf( "\nSize: %d", uiCount );
    printf( "\nCapacity: %d", myVec.capacity() );

    for( uint32 i = 0; i < uiCount; ++i )
    {
        uint32 uiItem = myVec[i];
        printf( "\n[%u]: %u", i, uiItem );
    }

    printf( "\nRunning fill test..." );
    for( uint32 i = uiCount; i < myVec.capacity(); ++i )
    {
        myVec.push( 0xBEEF );
    }
    myVec.push( 0xDEAD );
}

void RunTest_QueueArena()
{
    printf( "\n\nTesting Arena Queue..." );
    using namespace Bogus::Core;

    Queue<VectorPolicyArena<uint32>> queue;
    uint32 const size_committed = queue.size_committed();
    uint32 const uiElementsToPop = size_committed / 2;

    printf( "\nFilling up to size_committed: %d", size_committed );
    for( uint32 i = 0; i < uiElementsToPop; ++i )
    {
        queue.push( 69 );
    }

    for( uint32 i = 0; i < uiElementsToPop; ++i )
    {
        queue.push( 420 );
    }

    printf( "\nPopping up to size_committed / 2: %d", uiElementsToPop );
    for( uint32 i = 0; i < uiElementsToPop; ++i )
    {
        queue.pop();
    }

    printf( "\nPopped first %d elements.", uiElementsToPop );

    for( uint32 i = 0; i < queue.count(); ++i )
    {
        uint32 uiItem = queue[i];
        printf( "\n[%u]: %u", i, uiItem );
    }
}

void RunTest_ElementPool()
{
    constexpr uint32 MAX_CAPACITY = 100;
    using namespace Bogus::Core;

    struct TestElem
    {
        uint32 uiID = 0;
        String::Buffer<128> name;
    };

    ElementPool<TestElem> pool;
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
    Arena* pArena = NEW_ARENA(.name = "Thunderdome!!" );
    printf( "\n\nArena Created with name: %.*s\n", pArena->initParams.name.m_uiLen,
            pArena->initParams.name.m_pData );

    RunTest_VectorStatic( pArena );
    RunTest_QueueStatic( pArena );
    ArenaRelease( pArena );
}

void RunTest_VectorMap()
{
    using namespace Bogus::Core;
    using Uint32MapPair = Bogus::Core::VectorMapPair<uint32, uint32>;
    using Uint32MapPairVector = Bogus::Core::Vector<Bogus::Core::VectorPolicyArena<Uint32MapPair>>;
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
    RunTest_VectorMap();
    RunTest_Arena();
    RunTest_VectorArena();
    RunTest_QueueArena();
    RunTest_ElementPool();
    getchar();
}
