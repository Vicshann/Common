
#ifndef MemUtilsH
#define MemUtilsH

#pragma once

#include "Platforms\Platform.h"

//---------------------------------------------------------------------------
//  Base class for all memory operations. Do all trackings here if necessary
//---------------------------------------------------------------------------
struct MemUtil           // No data members, but sizeof(MemUtil) is equal to current compiler`s data alignment value
{
// MEMPAGESIZE (4k) is more or less universal allocation granularity on various platforms
// Windows: PageSize=4k <-> AllocGran=64k
// Unix:    PageSize=4k and need some extra reallocations to make a specified alignment
// If page size is 4k:
// 0 = 4k * 1  = 4096
// 1 = 4k * 2  = 8182
// 2 = 4k * 4  = 16384
// 3 = 4k * 8  = 32768
// 4 = 4k * 16 = 65536
//
enum ESMemPage {
               mp4K  = MEMPAGESIZE << 0,
               mp8K  = MEMPAGESIZE << 1,
               mp16K = MEMPAGESIZE << 2,
               mp32K = MEMPAGESIZE << 3,
               mp64K = MEMPAGESIZE << 4
};
//---------------------------------------------------------------------------
#include "MemUtils\MemAllocs.h"
#include "MemUtils\MemChain.h"

//======================================================================================================================
static void CopyMem(PVOID Dst, PVOID Src, UINT Length)   // Allow unaligned memory?      // MoveToLeft  (for Delete operation)
{
 UINT ALen = Length/sizeof(PVOID);
 UINT BLen = Length%sizeof(PVOID);
 for(UINT ctr=0;ctr < ALen;ctr++)                    ((PVOID*)Dst)[ctr] = ((PVOID*)Src)[ctr];
 for(UINT ctr=(ALen*sizeof(PVOID));ctr < Length;ctr++)((PBYTE)Dst)[ctr] = ((PBYTE)Src)[ctr];
}
//----------------------
static void MoveMem(PVOID Dst, PVOID Src, UINT Length)   // Allow unaligned memory?    Check direction
{
 if((PBYTE)Dst <= (PBYTE)Src){MemUtil::CopyMem(Dst,Src,Length);return;}
 int ALen = Length/sizeof(PVOID);
 int BLen = Length%sizeof(PVOID);
 for(int ctr=Length-1;BLen > 0;ctr--,BLen--)((PBYTE)Dst)[ctr]  = ((PBYTE)Src)[ctr];
 for(int ctr=ALen-1;ALen > 0;ctr--,ALen--)  ((PVOID*)Dst)[ctr] = ((PVOID*)Src)[ctr]; 
}
//----------------------
static bool EqualMem(PVOID BlkA, PVOID BlkB, UINT Length)
{
 return (memcmp(BlkA,BlkB,Length) == 0);  // TODO: implement 'memcmp'?
}
//----------------------

};
//---------------------------------------------------------------------------
#endif
