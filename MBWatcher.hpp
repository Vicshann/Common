
#pragma once
/*
  Copyright (c) 2021 Victor Sheinmann, Vicshann@gmail.com

  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), 
  to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
  and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. 
*/

//---------------------------------------------------------------------------
#include "Crypto\Crc32.hpp"

class CMBWatcher
{
struct SMemBlkRec
{
 PVOID  Addr;
 PVOID  RAddr;
 SIZE_T Size;
 SIZE_T Crc32;
};

 CRITICAL_SECTION csec;
 SMemBlkRec* WatchedMemBlocks;
 HANDLE hWMemBlocksDump;
 UINT32 MaxWMemBlocks;
 UINT32 TotalWMemBlocks;

//---------------------------------------------------------------------------
int GetFreeRecIndex(void)
{
 for(UINT idx=0;idx < this->MaxWMemBlocks;idx++)
  {
   SMemBlkRec* CurRec = &this->WatchedMemBlocks[idx];
   if(!CurRec->Addr)return idx;
  }
 return -1;
}
//---------------------------------------------------------------------------
int FindRexIdxForAddr(PVOID Addr)
{
 for(UINT idx=0;idx < this->TotalWMemBlocks;idx++)
  {
   SMemBlkRec* CurRec = &this->WatchedMemBlocks[idx];
   if(CurRec->Addr == Addr)return idx;
  }
 return -1;
}
//---------------------------------------------------------------------------
int DumpBlkIfChanged(SMemBlkRec* Blk)
{
 if(IsBadReadPtr(Blk->Addr, Blk->Size))return -3;
 SIZE_T crc = CCRC32::DoCRC32((UINT8*)Blk->Addr, Blk->Size);
 if(crc == Blk->Crc32)return -1;  // Not changed
 Blk->Crc32 = crc;    // Update
 DWORD Result;
// DBGMSG("Addr=%p, Size=%p",Blk->Addr,Blk->Size);
 if(this->pBlkCbkProc)this->pBlkCbkProc(Blk->Addr, Blk->Size);
 if((this->hWMemBlocksDump != INVALID_HANDLE_VALUE) && !WriteFile(this->hWMemBlocksDump, Blk->Addr, Blk->Size, &Result, NULL))return -2;
 return 0;
}
//---------------------------------------------------------------------------


public:
 void (_fastcall *pBlkCbkProc)(PVOID Data, SIZE_T Size);
//---------------------------------------------------------------------------
CMBWatcher(UINT MaxBlk=1024)
{
 InitializeCriticalSection(&this->csec);
 SIZE_T  FullSize = NCMN::AlignP2Frwd(MaxBlk * sizeof(SMemBlkRec), 0x10000);
 this->pBlkCbkProc = nullptr;
 this->MaxWMemBlocks = FullSize / sizeof(SMemBlkRec);
 this->TotalWMemBlocks = 0;
 this->hWMemBlocksDump = INVALID_HANDLE_VALUE;
 WatchedMemBlocks = (SMemBlkRec*)VirtualAlloc(NULL,FullSize,MEM_COMMIT,PAGE_READWRITE);
}
//---------------------------------------------------------------------------
~CMBWatcher()
{
 DeleteCriticalSection(&this->csec);
 if(this->hWMemBlocksDump)VirtualFree(this->hWMemBlocksDump,0,MEM_RELEASE);   
}
//---------------------------------------------------------------------------
bool SetDumpFile(PWSTR Path)
{
 this->hWMemBlocksDump = CreateFileW(Path,GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL|FILE_FLAG_SEQUENTIAL_SCAN,NULL);
 return this->hWMemBlocksDump != INVALID_HANDLE_VALUE;
}
//---------------------------------------------------------------------------
int AddMemBlock(PVOID RAddr, PVOID Addr, SIZE_T Size)
{
 EnterCriticalSection(&this->csec);
 int idx = this->GetFreeRecIndex();
 if(idx >= 0)
  {
   if(idx >= this->TotalWMemBlocks)this->TotalWMemBlocks++;
   SMemBlkRec* CurRec = &this->WatchedMemBlocks[idx];
   CurRec->Addr  = Addr;
   CurRec->RAddr = RAddr;
   CurRec->Size  = Size; 
   CurRec->Crc32 = CCRC32::DoCRC32((UINT8*)Addr, Size);
//   DBGMSG("Addr=%p, Size=%p, Crc32=%08X, BlkIdx=%i",Addr,Size,CurRec->Crc32,idx);
  }
 LeaveCriticalSection(&this->csec);
 return idx;
}
//---------------------------------------------------------------------------
int RemoveMemBlock(PVOID Addr)
{
 EnterCriticalSection(&this->csec);
 int idx = this->FindRexIdxForAddr(Addr);
 if(idx >= 0)
  {   
   SMemBlkRec* CurRec = &this->WatchedMemBlocks[idx];
//   DBGMSG("Addr=%p, Removing=%i",CurRec->Addr,idx);
   CurRec->Addr = nullptr;
   if(idx == (this->TotalWMemBlocks-1))this->TotalWMemBlocks--;
  }
 LeaveCriticalSection(&this->csec);
 return idx;
}
//---------------------------------------------------------------------------
int DumpChangedBlocks(void)
{
 int Dumped = 0;
 if(!this->pBlkCbkProc && (this->hWMemBlocksDump == INVALID_HANDLE_VALUE))return -1;
 EnterCriticalSection(&this->csec);
 for(UINT idx=0;idx < this->TotalWMemBlocks;idx++)
  {  
   if(0 == DumpBlkIfChanged(&this->WatchedMemBlocks[idx]))Dumped++;
  }
 LeaveCriticalSection(&this->csec);
 return Dumped;
}
//---------------------------------------------------------------------------

};
//---------------------------------------------------------------------------
