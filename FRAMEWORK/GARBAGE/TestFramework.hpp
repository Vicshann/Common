
#pragma once

#ifndef _TSTFRAMEWORK_    // These macro are used to check which header file included
#define _TSTFRAMEWORK_

#include "Framework.hpp"
#include <intrin.h> 
#include <Windows.h>
//---------------------------------------------------------------------------
struct TSTFRM
{
static int Test_CMemHeapBlk(void)
{
  NFRM::SMemPage<>* ppg = (NFRM::SMemPage<>*)5677; 
/* const int TryNum  = 100;
 const int ArrSize = 1000;
 NFRM::CMemPoolBlkS<>* PBlk = NFRM::CMemPoolBlkS<>::Create(1024);
 void* PtrArr[ArrSize];
 int   LenArr[ArrSize];
 for(int ctr=0;ctr < TryNum;ctr++)
  {
   int AllNum = 0;
//   __cpuid(); 
   HANDLE hHeap = GetProcessHeap();
   unsigned __int64 tBase = __rdtsc(); 
   for(AllNum=0;AllNum < ArrSize;AllNum++)
    {
//       if(AllNum == 0x2e)
//            AllNum = AllNum;
     LenArr[AllNum] = ((AllNum+1) * ((AllNum >> 2)+2)) & 0xFF;
     PtrArr[AllNum] = PBlk->Alloc(LenArr[AllNum]);   //   HeapAlloc(hHeap,0,LenArr[AllNum]);  //
     if(!PtrArr[AllNum])break;   // Failed to allocate more
    }
   unsigned __int64 tDelta = __rdtsc() - tBase; 
   for(int idx=0;idx < AllNum;idx++)
    {
     PBlk->Free(PtrArr[idx], LenArr[idx]);
    }
   PBlk->Trim();
   if(PBlk->BlkSize());//return 1;     // Not all mem is released to the block!
  }
 PBlk->Delete(); */
 return 0;
}
//---------------------------------------------------------------------------

};
//---------------------------------------------------------------------------
#endif
