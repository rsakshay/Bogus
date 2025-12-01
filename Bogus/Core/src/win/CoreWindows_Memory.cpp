#include "Core_Memory.h"
#include "Core_Utility.h"
#include "Globals.h"
#include "windows.h"

namespace Bogus::Core::Memory
{

uint64 GetPageSize()
{
    SYSTEM_INFO info;
    GetSystemInfo( &info );
    return info.dwPageSize;
}

void* Reserve( uint64 uiSize )
{
    uint64 const uiGBSnappedSize = AlignSize( uiSize, GIGABYTES( 1 ) );
    void* pMem = VirtualAlloc( 0, uiGBSnappedSize, MEM_RESERVE, PAGE_NOACCESS );
    return pMem;
}

void Release( void* pMem, uint64 uiSize )
{
    VirtualFree( pMem, 0, MEM_RELEASE );
}

void Commit( void* pMem, uint64 uiSize )
{
    uint64 const uiPageSnappedSize = AlignSize( uiSize, GetPageSize() );
    VirtualAlloc( pMem, uiPageSnappedSize, MEM_COMMIT, PAGE_READWRITE );
}

void Decommit( void* pMem, uint64 uiSize )
{
    VirtualFree( pMem, uiSize, MEM_DECOMMIT );
}

void Abort()
{
    ExitProcess( 1 );
}

} // namespace Bogus::Core::Memory
