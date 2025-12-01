#ifndef CORE_OS_H
#define CORE_OS_H
#include "Globals.h"

namespace Bogus::Core::Memory
{
uint64 GetPageSize();
void* Reserve( uint64 uiSize );
void Release( void* pMem, uint64 uiSize );
void Commit( void* pMem, uint64 uiSize );
void Decommit( void* pMem, uint64 uiSize );
void Abort();
} // namespace Bogus::Core::Memory

#endif
