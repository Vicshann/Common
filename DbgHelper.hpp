
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
struct NDbgHlp
{
/*          31-16         15-14  13  12-10  9-0
 DR7: ???? ???? ???? ???? ??     ?   ???    ??????????
      LNRW LNRW LNRW LNRW        G   001    GLGLGLGLGL        
      3333 2222 1111 0000        D          EE33221100

    RW: 0=Exe,1=Wr,2=IO,3=RdWr
    LN: 0=1,1=2,2=8,3=4
*/
enum EHBpType {btExe=0,btRead=3,btWrite=1,btIO=2};       // NOTE: btWrite does not triggered by read operations
enum EHBpSize {bsByte=0,bsWord=1,bsDWord=3,bsQWord=2};    
//---------------------------------------------------------------------------
template<typename T> static void SetupHWBP(T& Dr7, UINT BpIdx, EHBpType BpType, EHBpSize BpSize, bool Enable)
{
 DWORD FltMsk = ~((0x0F << (16+(BpIdx*4)))|(1 << (BpIdx*2)));   // Type and size together  
 Dr7 &= FltMsk;   // Zero everything for this BP
 if(Enable)
  {
   DWORD EnbMsk = (((UINT)Enable) << (BpIdx*2));       // NOTE: Windows uses local breakpoints
   DWORD TypMsk = ((DWORD)BpType << (16+(BpIdx*4))); 
   DWORD LenMsk = ((DWORD)BpSize << (18+(BpIdx*4)));
   Dr7 |= EnbMsk|TypMsk|LenMsk;
  }
}
//---------------------------------------------------------------------------
static int _stdcall SetThreadHWBP(DWORD ThId, UINT BpIdx, EHBpType BpType, EHBpSize BpSize, PVOID Addr)   // 0 for current thread
{
 DBGMSG("ThId=%08X, Addr=%p, BpIdx=%u, BpType=%u, BpSize=%u",ThId,Addr,BpIdx,BpType,BpSize); 
 if(BpIdx > 3)return -1;
 CONTEXT Context = {};
 Context.ContextFlags = CONTEXT_DEBUG_REGISTERS;
 NTSTATUS res = NtGetContextThread(NtCurrentThread, &Context);
 DBGMSG("GetRes=%08X, ContextFlags=%08X, Dr7=%p, Dr6=%p, Dr0=%p, Dr1=%p, Dr2=%p, Dr3=%p",res, Context.ContextFlags, Context.Dr7, Context.Dr6, Context.Dr0, Context.Dr1, Context.Dr2, Context.Dr3);
 if(!res)
  {
   SetupHWBP(Context.Dr7, BpIdx, BpType, BpSize, true);
   if(BpIdx==0)Context.Dr0 = (SIZE_T)Addr;
   else if(BpIdx==1)Context.Dr1 = (SIZE_T)Addr;
   else if(BpIdx==2)Context.Dr2 = (SIZE_T)Addr;
   else if(BpIdx==3)Context.Dr3 = (SIZE_T)Addr;
   res = NtSetContextThread(NtCurrentThread, &Context);
   DBGMSG("SetRes=%08X, ContextFlags=%08X, Dr7=%p, Dr6=%p, Dr0=%p, Dr1=%p, Dr2=%p, Dr3=%p",res, Context.ContextFlags, Context.Dr7, Context.Dr6, Context.Dr0, Context.Dr1, Context.Dr2, Context.Dr3);
  }
 return 0;
}
//------------------------------------------------------------------------------------
static int _stdcall DelThreadHWBP(DWORD ThId, UINT BpIdx)
{
 DBGMSG("ThId=%08X, BpIdx=%u",ThId,BpIdx); 
 if(BpIdx > 3)return -1;
 CONTEXT Context = {};
 Context.ContextFlags = CONTEXT_DEBUG_REGISTERS;
 NTSTATUS res = NtGetContextThread(NtCurrentThread, &Context);
 DBGMSG("GetRes=%08X, ContextFlags=%08X, Dr7=%p, Dr6=%p, Dr0=%p, Dr1=%p, Dr2=%p, Dr3=%p",res, Context.ContextFlags, Context.Dr7, Context.Dr6, Context.Dr0, Context.Dr1, Context.Dr2, Context.Dr3);
 if(!res)
  {
   SetupHWBP(Context.Dr7, BpIdx, btExe, bsByte, false);
   if(BpIdx==0)Context.Dr0 = 0;
   else if(BpIdx==1)Context.Dr1 = 0;
   else if(BpIdx==2)Context.Dr2 = 0;
   else if(BpIdx==3)Context.Dr3 = 0;
   res = NtSetContextThread(NtCurrentThread, &Context);
   DBGMSG("SetRes=%08X, ContextFlags=%08X, Dr7=%p, Dr6=%p, Dr0=%p, Dr1=%p, Dr2=%p, Dr3=%p",res, Context.ContextFlags, Context.Dr7, Context.Dr6, Context.Dr0, Context.Dr1, Context.Dr2, Context.Dr3);
  }
 return 0;
}
//---------------------------------------------------------------------------
};
//---------------------------------------------------------------------------
