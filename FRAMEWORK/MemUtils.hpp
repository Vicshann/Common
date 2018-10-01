
#pragma once


//---------------------------------------------------------------------------
//  Base class for all memory operations. Do all trackings here if necessary
//---------------------------------------------------------------------------
//struct MemUtil           // No data members, but sizeof(MemUtil) is equal to current compiler`s data alignment value
//{
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
               mpNO  = 1,
               mp4K  = MEMPAGESIZE << 0,   // Anything more than this will waste some memory on other platforms where allocation granularity is 4k
               mp8K  = MEMPAGESIZE << 1,
               mp16K = MEMPAGESIZE << 2,
               mp32K = MEMPAGESIZE << 3,
               mp64K = MEMPAGESIZE << 4
};
//---------------------------------------------------------------------------
#include "MemAllocs.hpp"
#include "MemChain.hpp"

//======================================================================================================================
// TODO: Optional maximum hardware supported size operation (Additional loop with __int128(If platform supports it) before loop with PVOID) 
//
static void CopyMem(PVOID Dst, PVOID Src, UINT Length)   // Allow unaligned memory?      // MoveToLeft  (for Delete operation)
{
 UINT ALen = Length/sizeof(PVOID);
 UINT BLen = Length%sizeof(PVOID);
 for(UINT ctr=0;ctr < ALen;ctr++)                    ((PVOID*)Dst)[ctr] = ((PVOID*)Src)[ctr];
 for(UINT ctr=(ALen*sizeof(PVOID));ctr < Length;ctr++)((PINT8)Dst)[ctr] = ((PINT8)Src)[ctr];
}
//----------------------
static void MoveMem(PVOID Dst, PVOID Src, UINT Length)   // Allow unaligned memory?    Check direction
{
 if((PINT8)Dst <= (PINT8)Src){CopyMem(Dst,Src,Length);return;}
 int ALen = Length/sizeof(PVOID);
 int BLen = Length%sizeof(PVOID);
 for(int ctr=Length-1;BLen > 0;ctr--,BLen--)((PINT8)Dst)[ctr]  = ((PINT8)Src)[ctr];
 for(int ctr=ALen-1;ALen > 0;ctr--,ALen--)  ((PVOID*)Dst)[ctr] = ((PVOID*)Src)[ctr]; 
}
//----------------------
static bool EqualMem(PVOID BlkA, PVOID BlkB, UINT Length)
{
 return (memcmp(BlkA,BlkB,Length) == 0);  // TODO: implement 'memcmp'?
}
//----------------------

//};
//---------------------------------------------------------------------------

