
#pragma once 
/*
  Copyright (c) 2018 Victor Sheinmann, Vicshann@gmail.com

  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), 
  to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
  and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. 
*/

#include "ntdll.h"
#include "FormatPE.h"
#include "ShMemIPC.hpp"

struct GhDbg
{
typedef ShMem SHM;     // Shorter and helps in case of renaming

//====================================================================================
enum EDatType {dtNone,dtNull=0x01,dtBool=0x02,dtBYTE=0x04,dtWORD=0x08,dtDWORD=0x10,dtQWORD=0x20,dtSigned=0x40,dtReal=0x80};
enum EMsgType {mtNone,mtDbgReq=0x01,mtDbgRsp=0x02,mtUsrReq=0x04,mtUsrRsp=0x08,mtInfo=0x10};
enum EMsgId   {
// System
               miNone, 
               miHello,
               miGoodby,
// Debugging 
               miQueryInformationProcess,
               miQueryInformationThread,
               miProtectVirtualMemory,
               miFlushVirtualMemory, 
               miQueryVirtualMemory,
               miDebugActiveProcess,
               miDebugActiveProcessStop,
               miContinueDebugEvent,
               miWaitForDebugEvent,
               miDebugBreakProcess,
               miGetThreadContext,
               miSetThreadContext,
               miReadVirtualMemory,
               miWriteVirtualMemory,
               miTerminateThread,
               miTerminateProcess,
               miSuspendThread,
               miResumeThread,
// User
               miDbgSetConfigs,
               miDbgGetConfigs,
               miDbgDetachNtfy,
               miUserBase       // Base inex for inspecified custom messages 
};

struct DbgEvtEx: public DEBUG_EVENT
{
 UINT PathSize;    // In chars
 wchar_t FilePath[1];
};
//====================================================================================
template<typename T> class CGrowArr    // Can invalidate old pointers after it grows
{
 T* Data;
 size_t Used; 
 size_t Allocated;    // Grow only

void Resize(size_t Cnt)     // All heap functions are Slow!
{
 this->Used = (Cnt*sizeof(T));
 if(this->Used && (this->Used <= this->Allocated))return;   // Already allocated
   else this->Allocated = this->Used + (this->Used / 2);
 if(this->Allocated && this->Data)this->Data = (T*)HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,this->Data,this->Allocated);
   else if(!this->Data)this->Data = (T*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,this->Allocated);
     else if(!this->Allocated && this->Data){HeapFree(GetProcessHeap(),0,this->Data); this->Data=NULL;}
}
//------------------------------------------------------------------------------------

public:
 CGrowArr(void){this->Data = NULL;}
 CGrowArr(size_t Cnt){this->Used = 0; this->Allocated = (Cnt*sizeof(T)); this->Data = (T*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,this->Allocated);}
 ~CGrowArr(){this->Clear();}
 operator  T*() {return this->Data;}
 UINT Count(void){return (this->Size() / sizeof(T));}
 UINT Size(void){return this->Used;}
//------------------------------------------------------------------------------------
T* Add(T* Data, size_t Cnt=1)
{
// DBGMSG("Adding: %p, %u",Data,Cnt);
 size_t OIdx = this->Count();
 this->Resize(OIdx+Cnt);
 if(Data)memcpy(&this->Data[OIdx], Data, Cnt*sizeof(T));
 return &this->Data[OIdx];
}
//------------------------------------------------------------------------------------
bool Remove(size_t Idx, size_t Cnt=1)
{
// DBGMSG("Removing: %u, %u",Idx,Cnt);
 size_t OIdx = this->Count();
 if(Idx >= OIdx)return false;
 size_t LIdx = Idx + Cnt;
 if(LIdx < OIdx)
  {
   memmove(&this->Data[Idx], &this->Data[LIdx], (OIdx-LIdx)*sizeof(T)); 
   this->Resize(OIdx-Cnt); 
  }
  else this->Resize(Idx);       // At the end of list
 return true;
}
//------------------------------------------------------------------------------------
bool Remove(T* Itm, size_t Cnt=1)
{
 return this->Remove(size_t(Itm - this->Data), Cnt);
}
//------------------------------------------------------------------------------------
void Clear(void){this->Resize(0);}

}; 
//====================================================================================
class CThreadList
{
public:
struct SThDesc
{
 enum ThFlags {tfNone=0,tfOpenedHnd=1,tfMarkedForRemove=2};
 BYTE   Flags;
 BYTE   Index;
 bool   TFlag;
 bool   DbgCont;
 WORD   SuspCtr;
 DWORD  ThreadID;  // Not WORD
 HANDLE hThread;
 PVOID  pContext;  // Non NULL only if thread is in Exception Handler (Suspended) 
 TEB*   pThTeb;
 struct SHwBp
  {
   PBYTE Addr;
   ULONG Size;
  } HwBpLst[4]; 
 struct SDbgCtx
  {
   SIZE_T Dr0;
   SIZE_T Dr1;
   SIZE_T Dr2;
   SIZE_T Dr3;
   SIZE_T Dr6;
   SIZE_T Dr7;
   bool   TrFlg;    // Unreliable, because this flag can be accessed with pushf/popf
  } DbgCtx;   
};
private:   
 CGrowArr<SThDesc> ThreadLst; 
 CRITICAL_SECTION csec;

void RemoveThreadFromListByIdx(DWORD Index)
{
// EnterCriticalSection(&this->csec);
 if((this->ThreadLst[Index].Flags & SThDesc::tfOpenedHnd) && this->ThreadLst[Index].hThread)CloseHandle(this->ThreadLst[Index].hThread);
 this->ThreadLst[Index].hThread  = NULL;          
 this->ThreadLst[Index].ThreadID = 0;
// LeaveCriticalSection(&this->csec);
}
//------------------------------------------------------------------------------------

public:
//------------------------------------------------------------------------------------
CThreadList(void): ThreadLst(16)
{
 InitializeCriticalSection(&this->csec);
 this->Clear();
}
//------------------------------------------------------------------------------------
~CThreadList()
{
 DeleteCriticalSection(&this->csec);
}
//------------------------------------------------------------------------------------
int AddThreadToList(TEB* pThTeb, DWORD ThreadID=0, HANDLE hThread=NULL, bool CanOpen=true)
{ 
 EnterCriticalSection(&this->csec);
 int NewThIdx = -1;
 if(!ThreadID && pThTeb)ThreadID = (DWORD)pThTeb->ClientId.UniqueThread;
 for(UINT ctr=0,total=this->ThreadLst.Count();ctr < total;ctr++)
  {
   SThDesc* ThDes = &this->ThreadLst[ctr];
   if(ThreadID && (ThDes->ThreadID == ThreadID)){LeaveCriticalSection(&this->csec); return ctr;}      // Already in list
   if(hThread  && (ThDes->hThread  == hThread )){LeaveCriticalSection(&this->csec); return ctr;}      // Already in list
   if(!ThDes->hThread)NewThIdx = ctr;
  }
 SThDesc* ThDes;
 if(NewThIdx < 0)     // Adding a new slot
  {
   NewThIdx = this->ThreadLst.Count();
   ThDes = this->ThreadLst.Add(NULL);
  }
   else ThDes = &this->ThreadLst[NewThIdx];     // Reusing an empty slot
// DBGMSG("Adding a new thread: %u",NewThIdx);
 ThDes->ThreadID = ThreadID;
 ThDes->Index    = NewThIdx;                                  
 ThDes->SuspCtr  = 0;
 ThDes->pContext = NULL;
 ThDes->pThTeb   = pThTeb;
 if(CanOpen && !hThread)
  {
   ThDes->hThread = OpenThread(THREAD_ALL_ACCESS,FALSE,ThreadID);    // What for?      THREAD_GET_CONTEXT|THREAD_SET_CONTEXT|THREAD_SUSPEND_RESUME
   if(!ThDes->hThread)
    {
     ThDes->ThreadID = NULL;
     LeaveCriticalSection(&this->csec);
     DBGMSG("Failed to open thread %u: %u",ThreadID,GetLastError());
     return -1;
    }
   ThDes->Flags = SThDesc::tfOpenedHnd;
  }
   else
    {
     ThDes->Flags   = SThDesc::tfNone;
     ThDes->hThread = hThread;   
    }
 DBGMSG("ThreadID=%08X(%u), hThread=%08X, Index=%u",ThDes->ThreadID,ThDes->ThreadID,ThDes->hThread,NewThIdx);
 LeaveCriticalSection(&this->csec);
 return NewThIdx;
}
//------------------------------------------------------------------------------------
bool IsThreadInList(DWORD ThreadID, HANDLE hThread=NULL){return (this->FindThreadIdxInList(NULL, ThreadID, hThread) >= 0);}
bool IsThreadIndexExist(UINT Index){return ((Index < this->ThreadLst.Count()) || this->ThreadLst[Index].hThread);}
HANDLE GetHandleByIndex(UINT Index)
{
 HANDLE val = NULL;
 EnterCriticalSection(&this->csec);
 if(Index < this->ThreadLst.Count())val = this->ThreadLst[Index].hThread;  // NULL if the slot is empty
 LeaveCriticalSection(&this->csec);
 return val;
}
//------------------------------------------------------------------------------------
HANDLE GetHandleByID(UINT ThreadID)
{
 EnterCriticalSection(&this->csec);
 int Idx = this->FindThreadIdxInList(NULL, ThreadID, NULL);
 if(Idx < 0){LeaveCriticalSection(&this->csec); return NULL;}
 HANDLE val = NULL;
 if(Idx < this->ThreadLst.Count())val = this->ThreadLst[Idx].hThread;  // NULL if the slot is empty
 LeaveCriticalSection(&this->csec);
 return val;
}
//------------------------------------------------------------------------------------
int FindThreadIdxInList(SThDesc& Res, DWORD ThreadID, HANDLE hThread=NULL)
{
 EnterCriticalSection(&this->csec);
 SThDesc* Ptr;
 int Idx = this->FindThreadIdxInList(&Ptr, ThreadID, hThread);
 if(Idx < 0){LeaveCriticalSection(&this->csec); return -1;}
 memcpy(&Res,Ptr,sizeof(SThDesc));
 LeaveCriticalSection(&this->csec);
 return Idx;
}
//------------------------------------------------------------------------------------
int FindThreadIdxInList(SThDesc** Res, DWORD ThreadID, HANDLE hThread=NULL)
{
 EnterCriticalSection(&this->csec);
 for(UINT ctr=0,total=this->ThreadLst.Count();ctr < total;ctr++)
  {
   if(!this->ThreadLst[ctr].hThread)continue;     // The slot is empty
   if(hThread  && (this->ThreadLst[ctr].hThread  != hThread))continue;
   if(ThreadID && (this->ThreadLst[ctr].ThreadID != ThreadID))continue; 
   if(Res)*Res = &this->ThreadLst[ctr]; 
   LeaveCriticalSection(&this->csec);
   return ctr;
  }
 LeaveCriticalSection(&this->csec);
// DBGMSG("Thread %u:%08X not found!",ThreadID,hThread);
 return -1;
}
//------------------------------------------------------------------------------------
SThDesc* GetThreadDesc(UINT Index)     
{
 if(!this->IsThreadIndexExist(Index))return NULL;
 return &this->ThreadLst[Index];
}
//------------------------------------------------------------------------------------
bool RemoveThreadFromList(DWORD ThreadID, HANDLE hThread=NULL)   // A process can create and delete some thread very frequently so no memory moving here, just invalidate removed entries
{
 EnterCriticalSection(&this->csec);
 int Idx = this->FindThreadIdxInList(NULL, ThreadID, hThread);
 if(Idx < 0){LeaveCriticalSection(&this->csec); return false;}
 this->RemoveThreadFromListByIdx(Idx);
 LeaveCriticalSection(&this->csec);
 return true;
}
//------------------------------------------------------------------------------------
void Clear(void)
{
 EnterCriticalSection(&this->csec);
 for(UINT ctr=0,total=this->ThreadLst.Count();ctr < total;ctr++)
  {
   SThDesc* ThDes = &this->ThreadLst[ctr];
   if((ThDes->Flags & SThDesc::tfOpenedHnd) && ThDes->hThread)CloseHandle(ThDes->hThread);
  }
 this->ThreadLst.Clear(); 
 LeaveCriticalSection(&this->csec);
}
//------------------------------------------------------------------------------------
NTSTATUS Suspend(UINT Idx, PULONG PrevCnt, bool MarkForRemove=false) 
{
 EnterCriticalSection(&this->csec);
 if(!this->IsThreadIndexExist(Idx)){LeaveCriticalSection(&this->csec); return STATUS_NOT_FOUND;}
 if(this->ThreadLst[Idx].SuspCtr != (UINT)-1)
  {
   this->ThreadLst[Idx].SuspCtr++;
   if(MarkForRemove)this->ThreadLst[Idx].Flags |= SThDesc::tfMarkedForRemove;  // Will be removed after next Resume
   HANDLE hTh = this->ThreadLst[Idx].hThread; 
   LeaveCriticalSection(&this->csec);
//   DBGMSG("Handle=%p",hTh);
   if(hTh)return NtSuspendThread(hTh, PrevCnt);  // May be this thread
   return STATUS_SUCCESS;
  }
 LeaveCriticalSection(&this->csec);
 return STATUS_UNSUCCESSFUL;
}
//------------------------------------------------------------------------------------
NTSTATUS Resume(UINT Idx, PULONG PrevCnt, int DbgContinue=-1) 
{
 EnterCriticalSection(&this->csec);
 if(!this->IsThreadIndexExist(Idx)){LeaveCriticalSection(&this->csec); return STATUS_NOT_FOUND;}
 if(this->ThreadLst[Idx].SuspCtr > 0)
  {
   NTSTATUS Status = STATUS_SUCCESS;
   this->ThreadLst[Idx].SuspCtr--; 
   if(DbgContinue >= 0)this->ThreadLst[Idx].DbgCont = DbgContinue;  // ???
   if(this->ThreadLst[Idx].hThread)Status = NtResumeThread(this->ThreadLst[Idx].hThread, PrevCnt); 
   if(this->ThreadLst[Idx].Flags & SThDesc::tfMarkedForRemove)this->RemoveThreadFromListByIdx(Idx);  
   LeaveCriticalSection(&this->csec);   
   return Status;
  }
 LeaveCriticalSection(&this->csec);
 return STATUS_UNSUCCESSFUL;
}
//------------------------------------------------------------------------------------
NTSTATUS SuspendAllThreads(DWORD SingleThreadID, int SingleThreadIdx=-1,  bool MarkForRemove=false)   // Suspends only registered threads so ClientThread is safe
{
 EnterCriticalSection(&this->csec);
 NTSTATUS Status = STATUS_UNSUCCESSFUL;
 if(!SingleThreadID && (SingleThreadIdx > 0))
  {
   SThDesc* ThDesc = GetThreadDesc(SingleThreadIdx);
   if(ThDesc)SingleThreadID = ThDesc->ThreadID;
  }
 SThDesc* CurrThDesc = NULL;
 TEB* CurrTeb = NtCurrentTeb();    
 for(UINT ctr=0,total=this->ThreadLst.Count();ctr < total;ctr++)
  {
   SThDesc* ThDesc = &this->ThreadLst[ctr];
   if(!ThDesc->hThread)continue;     // The slot is empty
   if(MarkForRemove && SingleThreadID && (ThDesc->ThreadID == SingleThreadID))ThDesc->Flags |= SThDesc::tfMarkedForRemove;  // Will be removed after next Resume
   if(ThDesc->SuspCtr == (UINT)-1)continue;     // Suspend counter overflow!
   if(ThDesc->pThTeb == CurrTeb){CurrThDesc = ThDesc; continue;}  // Current thread is in the list - suspend it last
   ThDesc->SuspCtr++;
   if(ThDesc->hThread)Status = NtSuspendThread(ThDesc->hThread, NULL);
  }
 if(CurrThDesc)
  {
   CurrThDesc->SuspCtr++;
   if(CurrThDesc->hThread)Status = NtSuspendThread(CurrThDesc->hThread, NULL);     // This thread
  }
 LeaveCriticalSection(&this->csec);
 return STATUS_SUCCESS;    // Always
}
//------------------------------------------------------------------------------------
NTSTATUS ResumeAllThreads(DWORD SingleThreadID, int SingleThreadIdx=-1, int DbgContinue=-1)
{
 EnterCriticalSection(&this->csec);
 NTSTATUS Status = STATUS_UNSUCCESSFUL;
 if(!SingleThreadID && (SingleThreadIdx > 0))
  {
   SThDesc* ThDesc = GetThreadDesc(SingleThreadIdx);
   if(ThDesc)SingleThreadID = ThDesc->ThreadID;
  }
 TEB* CurrTeb = NtCurrentTeb(); 
 for(UINT ctr=0,total=this->ThreadLst.Count();ctr < total;ctr++)
  {
   SThDesc* ThDesc = &this->ThreadLst[ctr];
   if(!ThDesc->hThread)continue;     // The slot is empty
   if(!ThDesc->SuspCtr)continue;     // Not suspended
   if((ThDesc->ThreadID == SingleThreadID) && (DbgContinue >= 0))ThDesc->DbgCont = DbgContinue;  // ???
   ThDesc->SuspCtr--;
   if(ThDesc->hThread && (ThDesc->pThTeb != CurrTeb))Status = NtResumeThread(ThDesc->hThread, NULL);  // Current thread is already running but suspend count > 0?
   if(ThDesc->Flags & SThDesc::tfMarkedForRemove)this->RemoveThreadFromListByIdx(ctr);   // Remove ALL marked threads
  }
 LeaveCriticalSection(&this->csec);
 return STATUS_SUCCESS;
}
//------------------------------------------------------------------------------------
bool SetContextVal(UINT Idx, PCONTEXT Ctx)
{
 EnterCriticalSection(&this->csec);
 if(!this->IsThreadIndexExist(Idx) || !this->ThreadLst[Idx].pContext){LeaveCriticalSection(&this->csec); return false;}
 PCONTEXT ThCtx = (PCONTEXT)this->ThreadLst[Idx].pContext;
 ThCtx->ContextFlags |= Ctx->ContextFlags;   // Merge groups   
#ifdef _AMD64_ 
 ThCtx->MxCsr = Ctx->MxCsr;        // ?????????????
 if(Ctx->ContextFlags & CONTEXT_DEBUG_REGISTERS)
  {
   ThCtx->Dr0 = Ctx->Dr0;
   ThCtx->Dr1 = Ctx->Dr1;
   ThCtx->Dr2 = Ctx->Dr2;
   ThCtx->Dr3 = Ctx->Dr3;
   ThCtx->Dr6 = Ctx->Dr6;
   ThCtx->Dr7 = Ctx->Dr7;
  }
 if(Ctx->ContextFlags & CONTEXT_CONTROL)
  {
   ThCtx->Rbp    = Ctx->Rbp;
   ThCtx->Rip    = Ctx->Rip;
   ThCtx->SegCs  = Ctx->SegCs;         
   ThCtx->EFlags = Ctx->EFlags;         
   ThCtx->Rsp    = Ctx->Rsp;
   ThCtx->SegSs  = Ctx->SegSs;
  }
 if(Ctx->ContextFlags & CONTEXT_SEGMENTS)
  {
   ThCtx->SegGs = Ctx->SegGs;
   ThCtx->SegFs = Ctx->SegFs;
   ThCtx->SegEs = Ctx->SegEs;
   ThCtx->SegDs = Ctx->SegDs;
  }
 if(Ctx->ContextFlags & CONTEXT_INTEGER)
  {
   ThCtx->Rdi = Ctx->Rdi;
   ThCtx->Rsi = Ctx->Rsi;
   ThCtx->Rbx = Ctx->Rbx;
   ThCtx->Rdx = Ctx->Rdx;
   ThCtx->Rcx = Ctx->Rcx;
   ThCtx->Rax = Ctx->Rax;
   ThCtx->R8  = Ctx->R8;
   ThCtx->R9  = Ctx->R9;
   ThCtx->R10 = Ctx->R10;
   ThCtx->R11 = Ctx->R11;
   ThCtx->R12 = Ctx->R12;
   ThCtx->R13 = Ctx->R13;
   ThCtx->R14 = Ctx->R14;
   ThCtx->R15 = Ctx->R15;
  }
 if(Ctx->ContextFlags & CONTEXT_FLOATING_POINT)
  {
   memcpy(&ThCtx->FltSave,&Ctx->FltSave,sizeof(XMM_SAVE_AREA32));
  }
 if(Ctx->ContextFlags & CONTEXT_XSTATE)     
  {
   memcpy(&ThCtx->VectorRegister,&Ctx->VectorRegister,sizeof(Ctx->VectorRegister));
   ThCtx->VectorControl = Ctx->VectorControl;
  }
#else
 if(Ctx->ContextFlags & CONTEXT_DEBUG_REGISTERS)
  {
   ThCtx->Dr0 = Ctx->Dr0;
   ThCtx->Dr1 = Ctx->Dr1;
   ThCtx->Dr2 = Ctx->Dr2;
   ThCtx->Dr3 = Ctx->Dr3;
   ThCtx->Dr6 = Ctx->Dr6;
   ThCtx->Dr7 = Ctx->Dr7;
  }
 if(Ctx->ContextFlags & CONTEXT_CONTROL)
  {
   ThCtx->Ebp    = Ctx->Ebp;
   ThCtx->Eip    = Ctx->Eip;
   ThCtx->SegCs  = Ctx->SegCs;         
   ThCtx->EFlags = Ctx->EFlags;         
   ThCtx->Esp    = Ctx->Esp;
   ThCtx->SegSs  = Ctx->SegSs;
  }
 if(Ctx->ContextFlags & CONTEXT_SEGMENTS)
  {
   ThCtx->SegGs = Ctx->SegGs;
   ThCtx->SegFs = Ctx->SegFs;
   ThCtx->SegEs = Ctx->SegEs;
   ThCtx->SegDs = Ctx->SegDs;
  }
 if(Ctx->ContextFlags & CONTEXT_INTEGER)
  {
   ThCtx->Edi = Ctx->Edi;
   ThCtx->Esi = Ctx->Esi;
   ThCtx->Ebx = Ctx->Ebx;
   ThCtx->Edx = Ctx->Edx;
   ThCtx->Ecx = Ctx->Ecx;
   ThCtx->Eax = Ctx->Eax;
  }
 if(Ctx->ContextFlags & CONTEXT_FLOATING_POINT)
  {
   memcpy(&ThCtx->FloatSave,&Ctx->FloatSave,sizeof(FLOATING_SAVE_AREA));
  }
 if(Ctx->ContextFlags & CONTEXT_EXTENDED_REGISTERS)
  {   
   memcpy(&ThCtx->ExtendedRegisters,&Ctx->ExtendedRegisters,sizeof(Ctx->ExtendedRegisters));
  }
 if(Ctx->ContextFlags & CONTEXT_XSTATE)   // TODO:
  {

  }
#endif
 LeaveCriticalSection(&this->csec);
 return true;
}
//------------------------------------------------------------------------------------
bool GetContextVal(UINT Idx, PCONTEXT Ctx)
{
 EnterCriticalSection(&this->csec);
 if(!this->IsThreadIndexExist(Idx) || !this->ThreadLst[Idx].pContext){LeaveCriticalSection(&this->csec); return false;}
 PCONTEXT ThCtx = (PCONTEXT)this->ThreadLst[Idx].pContext;
 Ctx->ContextFlags &= ThCtx->ContextFlags;      // Take only present groups
#ifdef _AMD64_   
 Ctx->MxCsr = ThCtx->MxCsr;        // ?????????????
 if(Ctx->ContextFlags & CONTEXT_DEBUG_REGISTERS)
  {
   Ctx->Dr0 = ThCtx->Dr0;
   Ctx->Dr1 = ThCtx->Dr1;
   Ctx->Dr2 = ThCtx->Dr2;
   Ctx->Dr3 = ThCtx->Dr3;
   Ctx->Dr6 = ThCtx->Dr6;
   Ctx->Dr7 = ThCtx->Dr7;
  }
 if(Ctx->ContextFlags & CONTEXT_CONTROL)
  {
   Ctx->Rbp    = ThCtx->Rbp;
   Ctx->Rip    = ThCtx->Rip;
   Ctx->SegCs  = ThCtx->SegCs;         
   Ctx->EFlags = ThCtx->EFlags;         
   Ctx->Rsp    = ThCtx->Rsp;
   Ctx->SegSs  = ThCtx->SegSs;
  }
 if(Ctx->ContextFlags & CONTEXT_SEGMENTS)
  {
   Ctx->SegGs = ThCtx->SegGs;
   Ctx->SegFs = ThCtx->SegFs;
   Ctx->SegEs = ThCtx->SegEs;
   Ctx->SegDs = ThCtx->SegDs;
  }
 if(Ctx->ContextFlags & CONTEXT_INTEGER)
  {
   Ctx->Rdi = ThCtx->Rdi;
   Ctx->Rsi = ThCtx->Rsi;
   Ctx->Rbx = ThCtx->Rbx;
   Ctx->Rdx = ThCtx->Rdx;
   Ctx->Rcx = ThCtx->Rcx;
   Ctx->Rax = ThCtx->Rax;
   Ctx->R8  = ThCtx->R8;
   Ctx->R9  = ThCtx->R9;
   Ctx->R10 = ThCtx->R10;
   Ctx->R11 = ThCtx->R11;
   Ctx->R12 = ThCtx->R12;
   Ctx->R13 = ThCtx->R13;
   Ctx->R14 = ThCtx->R14;
   Ctx->R15 = ThCtx->R15;
  }
 if(Ctx->ContextFlags & CONTEXT_FLOATING_POINT)
  {
   memcpy(&Ctx->FltSave,&ThCtx->FltSave,sizeof(XMM_SAVE_AREA32));
  }
 if(Ctx->ContextFlags & CONTEXT_XSTATE)   
  {
   memcpy(&Ctx->VectorRegister,&ThCtx->VectorRegister,sizeof(Ctx->VectorRegister));
   Ctx->VectorControl = ThCtx->VectorControl;
  }
#else
 if(Ctx->ContextFlags & CONTEXT_DEBUG_REGISTERS)
  {
   Ctx->Dr0 = ThCtx->Dr0;
   Ctx->Dr1 = ThCtx->Dr1;
   Ctx->Dr2 = ThCtx->Dr2;
   Ctx->Dr3 = ThCtx->Dr3;
   Ctx->Dr6 = ThCtx->Dr6;
   Ctx->Dr7 = ThCtx->Dr7;
  }
 if(Ctx->ContextFlags & CONTEXT_CONTROL)
  {
   Ctx->Ebp    = ThCtx->Ebp;
   Ctx->Eip    = ThCtx->Eip;
   Ctx->SegCs  = ThCtx->SegCs;         
   Ctx->EFlags = ThCtx->EFlags;         
   Ctx->Esp    = ThCtx->Esp;
   Ctx->SegSs  = ThCtx->SegSs;
  }
 if(Ctx->ContextFlags & CONTEXT_SEGMENTS)
  {
   Ctx->SegGs = ThCtx->SegGs;
   Ctx->SegFs = ThCtx->SegFs;
   Ctx->SegEs = ThCtx->SegEs;
   Ctx->SegDs = ThCtx->SegDs;
  }
 if(Ctx->ContextFlags & CONTEXT_INTEGER)
  {
   Ctx->Edi = ThCtx->Edi;
   Ctx->Esi = ThCtx->Esi;
   Ctx->Ebx = ThCtx->Ebx;
   Ctx->Edx = ThCtx->Edx;
   Ctx->Ecx = ThCtx->Ecx;
   Ctx->Eax = ThCtx->Eax;
  }
 if(Ctx->ContextFlags & CONTEXT_FLOATING_POINT)
  {
   memcpy(&Ctx->FloatSave,&ThCtx->FloatSave,sizeof(FLOATING_SAVE_AREA));
  }
 if(Ctx->ContextFlags & CONTEXT_EXTENDED_REGISTERS)
  {
   memcpy(&Ctx->ExtendedRegisters,&ThCtx->ExtendedRegisters,sizeof(Ctx->ExtendedRegisters));
  }
 if(Ctx->ContextFlags & CONTEXT_XSTATE)    // TODO:
  {

  }
#endif
 LeaveCriticalSection(&this->csec);
 return true;
}
//------------------------------------------------------------------------------------
bool IsSingleStepping(UINT Idx)
{
 EnterCriticalSection(&this->csec);
 if(!this->IsThreadIndexExist(Idx)){LeaveCriticalSection(&this->csec); return false;}
 SThDesc* ThDes = &this->ThreadLst[Idx];
 bool res = ThDes->TFlag;
 LeaveCriticalSection(&this->csec);
 return res;
}
//------------------------------------------------------------------------------------
bool UpdTraceFlag(UINT Idx, PCONTEXT Ctx)  
{
 EnterCriticalSection(&this->csec);
 if(!(Ctx->ContextFlags & CONTEXT_CONTROL) || !this->IsThreadIndexExist(Idx)){LeaveCriticalSection(&this->csec); return false;}
 SThDesc* ThDes = &this->ThreadLst[Idx];
 ThDes->TFlag = (Ctx->EFlags & 0x0100);    // Trap flag mask
 LeaveCriticalSection(&this->csec);
 return true;
}
//------------------------------------------------------------------------------------
bool UpdHardwareBp(UINT Idx, PCONTEXT Ctx)  
{
 EnterCriticalSection(&this->csec);
 if(!(Ctx->ContextFlags & CONTEXT_DEBUG_REGISTERS) || !this->IsThreadIndexExist(Idx)){LeaveCriticalSection(&this->csec); return false;}
 SThDesc* ThDes = &this->ThreadLst[Idx];
 memset(&ThDes->HwBpLst,0,sizeof(ThDes->HwBpLst));  
 for(UINT ctr=0;ctr < 4;ctr++)       // Nothing special for x64?
  {
   if(!(Ctx->Dr7 & (1 << (ctr*2))))continue;       // Skip any not enabled
   BYTE LCod = (Ctx->Dr7 >> (10+(ctr*4))) & 3;     // Breakpoint size
   if(LCod & 2)ThDes->HwBpLst[ctr].Size = (LCod & 1)?(4):(8);
     else ThDes->HwBpLst[ctr].Size = LCod+1;
   switch(ctr)
    {
     case 0:
      ThDes->HwBpLst[ctr].Addr = (PBYTE)Ctx->Dr0;
      break;
     case 1:
      ThDes->HwBpLst[ctr].Addr = (PBYTE)Ctx->Dr1;
      break;
     case 2:
      ThDes->HwBpLst[ctr].Addr = (PBYTE)Ctx->Dr2;
      break;
     case 3:
      ThDes->HwBpLst[ctr].Addr = (PBYTE)Ctx->Dr3;
      break;
    }
  }
 LeaveCriticalSection(&this->csec);
 return true;
}
//------------------------------------------------------------------------------------
bool IsHardwareBpHit(UINT Idx, PVOID Addr)
{
 EnterCriticalSection(&this->csec);
 if(!this->IsThreadIndexExist(Idx)){LeaveCriticalSection(&this->csec); return false;}
 SThDesc* ThDes = &this->ThreadLst[Idx];
 for(UINT ctr=0;ctr < 4;ctr++)       // Nothing special for x64?
  {
   if(!ThDes->HwBpLst[ctr].Addr)continue;
   if(((PBYTE)Addr >= ThDes->HwBpLst[ctr].Addr)&&((PBYTE)Addr < &ThDes->HwBpLst[ctr].Addr[ThDes->HwBpLst[ctr].Size])){LeaveCriticalSection(&this->csec); return true;}    
  }
 LeaveCriticalSection(&this->csec);
 return false;
}
//------------------------------------------------------------------------------------
bool ReadDbgContext(UINT Idx, PCONTEXT Ctx)    // Remember DRx changes, made by an application 
{
 EnterCriticalSection(&this->csec);
 if(!this->IsThreadIndexExist(Idx)){LeaveCriticalSection(&this->csec); return false;}
 SThDesc* ThDes = &this->ThreadLst[Idx];
 if(Ctx->ContextFlags & CONTEXT_DEBUG_REGISTERS)   // Only if present in Src
  {
   ThDes->DbgCtx.Dr0 = Ctx->Dr0;
   ThDes->DbgCtx.Dr1 = Ctx->Dr1;
   ThDes->DbgCtx.Dr2 = Ctx->Dr2;
   ThDes->DbgCtx.Dr3 = Ctx->Dr3;
   ThDes->DbgCtx.Dr6 = Ctx->Dr6;
   ThDes->DbgCtx.Dr7 = Ctx->Dr7;
  }
 if(Ctx->ContextFlags & CONTEXT_CONTROL)ThDes->DbgCtx.TrFlg = (Ctx->EFlags & 0x0100);    // ResumeFlag?
 LeaveCriticalSection(&this->csec);
 return false;
}
//------------------------------------------------------------------------------------
bool WriteDbgContext(UINT Idx, PCONTEXT Ctx)   // Restore DRx state to show it to an application
{
 EnterCriticalSection(&this->csec);
 if(!this->IsThreadIndexExist(Idx)){LeaveCriticalSection(&this->csec); return false;}
 SThDesc* ThDes = &this->ThreadLst[Idx];
 if(Ctx->ContextFlags & CONTEXT_DEBUG_REGISTERS)   // Only if required by dest
  {
   Ctx->Dr0 = ThDes->DbgCtx.Dr0; 
   Ctx->Dr1 = ThDes->DbgCtx.Dr1;
   Ctx->Dr2 = ThDes->DbgCtx.Dr2; 
   Ctx->Dr3 = ThDes->DbgCtx.Dr3;
   Ctx->Dr6 = ThDes->DbgCtx.Dr6;
   Ctx->Dr7 = ThDes->DbgCtx.Dr7;
  }
 if(Ctx->ContextFlags & CONTEXT_CONTROL)Ctx->EFlags = (Ctx->EFlags & ~0x0100)|((UINT)ThDes->DbgCtx.TrFlg << 8);  
 LeaveCriticalSection(&this->csec);
 return false;
}
//------------------------------------------------------------------------------------

};
//====================================================================================
class CSwBpList             // TODO: PAGE breakpoints
{
 CRITICAL_SECTION csec;
 CGrowArr<PVOID> SwBpLst;

public:
//------------------------------------------------------------------------------------
CSwBpList(void): SwBpLst(32)
{
 InitializeCriticalSection(&this->csec);
}
//------------------------------------------------------------------------------------
~CSwBpList()
{
 DeleteCriticalSection(&this->csec);
}
//------------------------------------------------------------------------------------
int AddBP(PVOID Addr)
{
 EnterCriticalSection(&this->csec);
 int BpIdx = -1;
 for(UINT ctr=0,total=this->SwBpLst.Count();ctr < total;ctr++)
  {
   PVOID Val = this->SwBpLst[ctr];
   if(Val == Addr){LeaveCriticalSection(&this->csec); return ctr;}   // Already in list
   if(!Val)BpIdx = ctr;
  }
 if(BpIdx < 0)
  {
   BpIdx = this->SwBpLst.Count();
   this->SwBpLst.Add(NULL);
  }
 this->SwBpLst[BpIdx] = Addr;
 LeaveCriticalSection(&this->csec);
 DBGMSG("Adding SwBP: %u",BpIdx);
 return BpIdx;
}
//------------------------------------------------------------------------------------
int DelBP(PVOID Addr)
{
 EnterCriticalSection(&this->csec);
 int BpIdx = this->GetBpIdxForAddr(Addr);
 if(BpIdx < 0){LeaveCriticalSection(&this->csec); return -1;}
 this->SwBpLst[BpIdx] = NULL;      /////////////     this->SwBpLst.Remove(BpIdx);  //  [idx] = NULL;
 LeaveCriticalSection(&this->csec);
 DBGMSG("Removing SwBP: %u",BpIdx);
 return BpIdx;
}
//------------------------------------------------------------------------------------
int IsHitBP(PVOID Addr){return (this->GetBpIdxForAddr(Addr) >= 0);}
int GetBpIdxForAddr(PVOID Addr)
{
 EnterCriticalSection(&this->csec);
 for(UINT ctr=0,total=this->SwBpLst.Count();ctr < total;ctr++)
  {
   PVOID Cadr = this->SwBpLst[ctr];
   if(Cadr && (Cadr == Addr)){LeaveCriticalSection(&this->csec); return ctr;}
  }
 LeaveCriticalSection(&this->csec);
 return -1;
}
//------------------------------------------------------------------------------------

};
//====================================================================================
struct SNtDll    // Only for a simple system service redirected functions 
{
 PBYTE   pLibMem;
 HMODULE hNtDll;
 decltype(::NtQueryInformationThread)* NtQueryInformationThread;
 decltype(::NtQueryInformationProcess)* NtQueryInformationProcess;
 decltype(::NtQueryVirtualMemory)* NtQueryVirtualMemory;
 decltype(::NtGetContextThread)* NtGetContextThread;
 decltype(::NtSetContextThread)* NtSetContextThread;
 decltype(::NtReadVirtualMemory)* NtReadVirtualMemory;
 decltype(::NtWriteVirtualMemory)* NtWriteVirtualMemory;
 decltype(::NtProtectVirtualMemory)* NtProtectVirtualMemory;
 decltype(::NtFlushVirtualMemory)* NtFlushVirtualMemory;
 decltype(::NtTerminateThread)* NtTerminateThread;
 decltype(::NtSuspendThread)* NtSuspendThread;
 decltype(::NtResumeThread)* NtResumeThread;
 decltype(::NtQueryObject)* NtQueryObject;

//------------------------------------------------------------------------------------
SNtDll(void)       // NOTE: Better call this before hooking NtMapViewOfSection
{
 LPSTR LibPath[MAX_PATH];
 this->hNtDll = GetModuleHandleA("ntdll.dll");
 if(!this->hNtDll || !GetModuleFileNameA(this->hNtDll,(LPSTR)&LibPath,sizeof(LibPath)))return;    
 HANDLE hFile = CreateFileA((LPSTR)&LibPath,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL|FILE_FLAG_SEQUENTIAL_SCAN,NULL);   // TODO: Receive this from debugger to avoid CreateFile("ntdll.dll") detection
 if(hFile == INVALID_HANDLE_VALUE)return;
 DWORD Result   = 0;
 DWORD FileSize = GetFileSize(hFile,NULL);
 this->pLibMem  = (PBYTE)VirtualAlloc(NULL,FileSize,MEM_COMMIT,PAGE_EXECUTE_READWRITE);  
 if(FileSize)ReadFile(hFile,this->pLibMem,FileSize,&Result,NULL);
 CloseHandle(hFile); 
 TFixRelocations<PECURRENT>(this->pLibMem,true); 

 *(PVOID*)&this->NtQueryObject             = TGetProcedureAddress<PECURRENT,true>(this->pLibMem, "NtQueryObject"); 
 *(PVOID*)&this->NtResumeThread            = TGetProcedureAddress<PECURRENT,true>(this->pLibMem, "NtResumeThread"); 
 *(PVOID*)&this->NtSuspendThread           = TGetProcedureAddress<PECURRENT,true>(this->pLibMem, "NtSuspendThread"); 
 *(PVOID*)&this->NtTerminateThread         = TGetProcedureAddress<PECURRENT,true>(this->pLibMem, "NtTerminateThread"); 
 *(PVOID*)&this->NtFlushVirtualMemory      = TGetProcedureAddress<PECURRENT,true>(this->pLibMem, "NtFlushVirtualMemory"); 
 *(PVOID*)&this->NtProtectVirtualMemory    = TGetProcedureAddress<PECURRENT,true>(this->pLibMem, "NtProtectVirtualMemory"); 
 *(PVOID*)&this->NtWriteVirtualMemory      = TGetProcedureAddress<PECURRENT,true>(this->pLibMem, "NtWriteVirtualMemory"); 
 *(PVOID*)&this->NtReadVirtualMemory       = TGetProcedureAddress<PECURRENT,true>(this->pLibMem, "NtReadVirtualMemory"); 
 *(PVOID*)&this->NtSetContextThread        = TGetProcedureAddress<PECURRENT,true>(this->pLibMem, "NtSetContextThread"); 
 *(PVOID*)&this->NtGetContextThread        = TGetProcedureAddress<PECURRENT,true>(this->pLibMem, "NtGetContextThread"); 
 *(PVOID*)&this->NtQueryVirtualMemory      = TGetProcedureAddress<PECURRENT,true>(this->pLibMem, "NtQueryVirtualMemory"); 
 *(PVOID*)&this->NtQueryInformationProcess = TGetProcedureAddress<PECURRENT,true>(this->pLibMem, "NtQueryInformationProcess"); 
 *(PVOID*)&this->NtQueryInformationThread  = TGetProcedureAddress<PECURRENT,true>(this->pLibMem, "NtQueryInformationThread"); 
}
//------------------------------------------------------------------------------------
~SNtDll()     
{
 PVOID Ptr = this->pLibMem;
 memset(this,0,sizeof(SNtDll));
 if(Ptr)VirtualFree(Ptr,0,MEM_RELEASE);
}
//------------------------------------------------------------------------------------
/*
RtlUserThreadStart (Callback)    // CREATE_SUSPENDED thread`s IP 
  x32  x64
  EAX  RCX = ThreadProc
  EBX  RDX = ThreadParam
*/
static bool ChangeNewSuspThProcAddr(HANDLE hThread, PVOID NewThProc, PVOID* Param)
{
 CONTEXT ctx;
 ctx.ContextFlags = CONTEXT_INTEGER;   // CONTEXT_CONTROL - no check if IP is at RtlUserThreadStart
 if(!GetThreadContext(hThread, &ctx)){LOGMSG("Failed to get CONTEXT"); return false;}
#ifdef _AMD64_ 
 if(NewThProc)ctx.Rcx = (DWORD64)NewThProc;
 if(Param)
  {
   DWORD64 Prv = ctx.Rdx;
   ctx.Rdx = (DWORD64)*Param;
   *Param  = (PVOID)Prv;
  }
#else
 if(NewThProc)ctx.Eax = (DWORD)NewThProc;
 if(Param)
  {
   DWORD Prv = ctx.Ebx;
   ctx.Ebx = (DWORD)*Param;
   *Param  = (PVOID)Prv;
  }
#endif
 if(!SetThreadContext(hThread, &ctx)){LOGMSG("Failed to set CONTEXT"); return false;}
 LOGMSG("New ThProc: %p", NewThProc);
 return true;
}
//------------------------------------------------------------------------------------

};
//====================================================================================
//
//
//
//====================================================================================
class CDbgClient: public SHM::CMessageIPC        
{
 bool    BreakWrk;
 DWORD   ClientThID;
 DWORD   MainThID;
 DWORD   CurProcID;    
 HMODULE hClientDll;
 HANDLE  hIPCThread;
 UINT    IPCSize;
 UINT    UrsOpts;
 SNtDll  NtDll;         // Used to avoid a target application`s hooks
 CSwBpList   BpList;
 CThreadList ThList;

//------------------------------------------------------------------------------------
_declspec(noinline) static void DoDbgBreak(PVOID This){__debugbreak();}       // <<<<<<<<<<<<<<<< Very fishy!!!!!!!!!!!!!!!!!!!!!
static bool IsAddrInDbgBreak(PVOID Addr){return (((PBYTE)Addr >= (PBYTE)&DoDbgBreak)&&((PBYTE)Addr < ((PBYTE)&DoDbgBreak + 64)));} 
static DWORD WINAPI DbgBreakThread(LPVOID lpThreadParameter)
{
 CDbgClient* DbgIPC = (CDbgClient*)lpThreadParameter;
 DoDbgBreak(DbgIPC);  // x64Dbg will continue normally from this  // Any DebugBreak functions may be tracked   
 return 0;
}
//------------------------------------------------------------------------------------
int DoSuspendThreads(DWORD SingleThreadID, int SingleThreadIdx=-1,  bool MarkForRemove=false, bool SingleOnly=false)   // Suspends only registered threads so ClientThread is safe
{
 if(this->EvtSuspAllTh && !SingleOnly)return (int)this->ThList.SuspendAllThreads(SingleThreadID, SingleThreadIdx=-1, MarkForRemove) - 1;
 if(SingleThreadIdx < 0)SingleThreadIdx = this->ThList.FindThreadIdxInList(nullptr, SingleThreadID);
 if(SingleThreadIdx < 0){DBGMSG("Thread ID not found: ThId=%u",SingleThreadID); return -2;}
 return this->ThList.Suspend(SingleThreadIdx, NULL, MarkForRemove);
}
//------------------------------------------------------------------------------------
int DoResumeThreads(DWORD SingleThreadID, int SingleThreadIdx=-1, int DbgContinue=-1, bool SingleOnly=false)
{
 if(this->EvtSuspAllTh && !SingleOnly)return (int)this->ThList.ResumeAllThreads(SingleThreadID, SingleThreadIdx, DbgContinue) - 1;
 if(SingleThreadIdx < 0)SingleThreadIdx = this->ThList.FindThreadIdxInList(nullptr, SingleThreadID);
 if(SingleThreadIdx < 0){DBGMSG("Thread ID not found: ThId=%u",SingleThreadID); return -2;}
 return this->ThList.Resume(SingleThreadIdx, NULL, DbgContinue);
}
//------------------------------------------------------------------------------------
// NOTE: Buffer is locked by this thread in message enumeration
//
int _stdcall ProcessRequestDbg(SMsgHdr* Req)
{
 static UINT LastMsg = 0;
 DBGMSG("MsgID=%u, MsgSeqID=%u",Req->MsgID, Req->GetBlk()->MsgSeqID);
 if(Req->GetBlk()->MsgSeqID <= LastMsg){ DBGMSG("HelpME!"); }   // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 LastMsg = Req->GetBlk()->MsgSeqID;

 switch(Req->MsgID)
  {   
   case miQueryInformationProcess:
    {
     SHM::CArgPack<> api;
     SHM::CArgPack<1024> apo;
     HANDLE ProcessHandle; 
     PROCESSINFOCLASS ProcessInformationClass; 
     ULONG ProcessInformationLength; 
     ULONG   RetLen = 0; 
     HRESULT Status = STATUS_UNSUCCESSFUL;
   
     api.Assign((PBYTE)&Req->Data,Req->DataSize);
     api.PopArg(ProcessHandle);
     api.PopArg(ProcessInformationClass);
     api.PopArg(ProcessInformationLength);  
     if(ProcessHandle != UintToFakeHandle(this->CurProcID)){DBGMSG("Process HANDLE mismatch: %08X:%08X",ProcessHandle,UintToFakeHandle(this->CurProcID)); apo.PushBlk(ProcessInformationLength);}                                                                           
       else Status = this->NtDll.NtQueryInformationProcess(NtCurrentProcess,ProcessInformationClass,apo.PushBlk(ProcessInformationLength),ProcessInformationLength,&RetLen);   // ULONG or SIZE_T ?
     apo.PushArg(RetLen);
     apo.PushArg(Status);
     DBGMSG("miQueryInformationProcess PutMsg: Status=%08X, Size=%u",Status,apo.GetLen());
     this->PutMsg(mtDbgRsp, miQueryInformationProcess, Req->Sequence, apo.GetPtr(), apo.GetLen());      
    }
   break;
   case miQueryInformationThread:
    {
     SHM::CArgPack<> api;
     SHM::CArgPack<1024> apo;
     THREADINFOCLASS ThreadInformationClass; 
     ULONG ThreadInformationLength; 
     ULONG RetLen = 0;        
     HRESULT Status = STATUS_UNSUCCESSFUL;

     api.Assign((PBYTE)&Req->Data,Req->DataSize);
     UINT    ThIdx  = FakeHandleToUint(api.PopArg<HANDLE>());
     HANDLE ThreadHandle = this->ThList.GetHandleByIndex(ThIdx);
     api.PopArg(ThreadInformationClass);
     api.PopArg(ThreadInformationLength);                                           
     if(!ThreadHandle){DBGMSG("Thread handle not found: ThIdx=%i",ThIdx); apo.PushBlk(ThreadInformationLength);}
       else Status = this->NtDll.NtQueryInformationThread(ThreadHandle,ThreadInformationClass,apo.PushBlk(ThreadInformationLength),ThreadInformationLength,&RetLen);    // ULONG or SIZE_T ?
     apo.PushArg(RetLen);
     apo.PushArg(Status);
     DBGMSG("miQueryInformationThread PutMsg: Status=%08X, Size=%u",Status,apo.GetLen());
     this->PutMsg(mtDbgRsp, miQueryInformationThread, Req->Sequence, apo.GetPtr(), apo.GetLen());      
    }
   break;     
   case miQueryVirtualMemory:     // NOTE: x64Dbg will call this for every memory block and avery request/response will move forward in the buffer (In a small buffer some old messages(Thread reports) will be lost)
    {
     SHM::CArgPack<> api;
     SHM::CArgPack<1024> apo;
     PVOID BaseAddress;
     HANDLE ProcessHandle; 
     MEMORY_INFORMATION_CLASS MemoryInformationClass; 
     SIZE_T MemoryInformationLength; 
     SIZE_T RetLen = 0; 
     HRESULT Status = STATUS_UNSUCCESSFUL;
        
     api.Assign((PBYTE)&Req->Data,Req->DataSize);
     api.PopArg(ProcessHandle);
     api.PopArg(BaseAddress);
     api.PopArg(MemoryInformationClass);
     api.PopArg(MemoryInformationLength);  
     if(ProcessHandle != UintToFakeHandle(this->CurProcID)){DBGMSG("Process HANDLE mismatch: %08X:%08X",ProcessHandle,UintToFakeHandle(this->CurProcID)); apo.PushBlk(MemoryInformationLength);}                                                                              
       else Status = this->NtDll.NtQueryVirtualMemory(NtCurrentProcess,BaseAddress,MemoryInformationClass,apo.PushBlk(MemoryInformationLength),MemoryInformationLength,&RetLen);
     apo.PushArg(RetLen);
     apo.PushArg(Status);
//     DBGMSG("miQueryVirtualMemory PutMsg: Status=%08X, InfoClass=%u, BaseAddress=%p, Size=%u",Status,MemoryInformationClass,BaseAddress,apo.GetLen()); 
     this->PutMsg(mtDbgRsp, miQueryVirtualMemory, Req->Sequence, apo.GetPtr(), apo.GetLen());      
    }
   break;
   case miDebugActiveProcess:
    {
     SHM::CArgPack<> api;
     SHM::CArgPack<> apo;
     BOOL  Reslt = TRUE;
     DWORD dwProcessId = 0;
     HRESULT Status = STATUS_UNSUCCESSFUL;

     api.Assign((PBYTE)&Req->Data,Req->DataSize);
     api.PopArg(dwProcessId);
     if(dwProcessId != this->CurProcID){DBGMSG("Process ID mismatch: %08X:%08X",dwProcessId,this->CurProcID); return -1;}  // Allow to fail here?
     apo.PushArg(Reslt);
     DBGMSG("miDebugActiveProcess PutMsg: Status=%08X, Size=%u",apo.GetLen());
     this->PutMsg(mtDbgRsp, miDebugActiveProcess, Req->Sequence, apo.GetPtr(), apo.GetLen());   // <<<<<<<<< Response not received!  

     CGrowArr<TEB*>  TebArr(32);
     CGrowArr<PBYTE> ModArr(64);
     MEMORY_BASIC_INFORMATION meminfo;
     SIZE_T RetLen = 0;
     SIZE_T MemoryInformationLength = sizeof(MEMORY_BASIC_INFORMATION);      
     PBYTE BaseAddress = NULL;
     PVOID LastABase = NULL;
     TEB* CurTeb = NtCurrentTeb();      // ThreadID is DbgIPC->ClientThID and should not be reported to a debugger
     TEB* MainThTeb = NULL;
     for(;!this->NtDll.NtQueryVirtualMemory(NtCurrentProcess,BaseAddress,MemoryBasicInformation,&meminfo,MemoryInformationLength,&RetLen); BaseAddress += meminfo.RegionSize)
      {
       if(LastABase == meminfo.AllocationBase)continue;
       LastABase = meminfo.AllocationBase; 
       if((meminfo.Type == MEM_IMAGE) && (meminfo.State == MEM_COMMIT) && IsValidPEHeader(BaseAddress))
        {
         DBGMSG("Found PE image at %p",BaseAddress);
         *ModArr.Add(NULL) = BaseAddress;
        }
       else if((meminfo.Type == MEM_PRIVATE) && (meminfo.State == MEM_COMMIT) && (meminfo.Protect == meminfo.AllocationProtect) && (meminfo.Protect == PAGE_READWRITE) && (meminfo.RegionSize > 0x1000) && (meminfo.RegionSize < 0x4000))   // WinXP TEB >= 0x2000 ?
        {
         TEB* MayBeTEB = (TEB*)BaseAddress;
         if((MayBeTEB != CurTeb) && (MayBeTEB->ProcessEnvironmentBlock == CurTeb->ProcessEnvironmentBlock) && (MayBeTEB->CurrentLocale == CurTeb->CurrentLocale))
          {
           DBGMSG("Found TEB at %p",BaseAddress);
           *TebArr.Add(NULL) = MayBeTEB;    // Includes ClientThread
           if(MainThTeb)   // TODO: Request the thread creation time and find a first thread?     // Add and suspend threads here?
            {

            }
             else if((UINT)MayBeTEB->ClientId.UniqueThread != this->ClientThID)MainThTeb = MayBeTEB;    // INCORRECT: Need sorting by creation time!
          }
        }       
      }
     this->MainThID = (UINT)MainThTeb->ClientId.UniqueThread;  // MainThTeb should be present!!!   // Report this only in CREATE_PROCESS_DEBUG_EVENT    // Not guranteed to return a real main(first) thread
     DBGMSG("Modules=%u, Threads=%u, MainThreadID=%u, ClientThreadID=%u",ModArr.Count(),TebArr.Count(),this->MainThID,this->ClientThID);
     this->Report_CREATE_PROCESS_DEBUG_EVENT(MainThTeb, true);   // Report process creation    // Suspend Main Thread
     for(UINT Ctr=0,Total=ModArr.Count();Ctr < Total;Ctr++)  // TODO: Report ntdll.dll first as it is done originally?
      {
       PVOID hModule = ModArr[Ctr];
       if((hModule == this->hClientDll)||(hModule == CurTeb->ProcessEnvironmentBlock->ImageBaseAddress))continue;                  // Do not report Client DLL and main module
       this->Report_LOAD_DLL_DEBUG_INFO(hModule, false);     // No suspending here (Only a main thread is registered anyway)
      }
     for(UINT Ctr=0,Total=TebArr.Count();Ctr < Total;Ctr++)   // Originally all threads suspended by DebugActiveProcess before any of debug events reported but we do not have any handles yet
      {
       TEB* pTeb = TebArr[Ctr];
       if(((UINT)pTeb->ClientId.UniqueThread == this->ClientThID)||((UINT)pTeb->ClientId.UniqueThread == this->MainThID))continue;   // Do not report Client thread and Main thread(again)
       DBGMSG("miDebugActiveProcess: OwnerProcessID=%u, ThreadID=%u",(UINT)pTeb->ClientId.UniqueProcess,(UINT)pTeb->ClientId.UniqueThread);
       this->Report_CREATE_THREAD_DEBUG_EVENT(pTeb, true);   // Open a thread`s handle, report it and suspend
      }
    }
   break;
   case miDebugActiveProcessStop:
    {
     DBGMSG("miDebugActiveProcessStop");
    }
   break;
   case miContinueDebugEvent:
    {
     SHM::CArgPack<> api;
     SHM::CArgPack<> apo;
     BOOL  Reslt = TRUE;
     DWORD dwProcessId = 0;
     DWORD dwThreadId; 
     DWORD dwContinueStatus;

     api.Assign((PBYTE)&Req->Data,Req->DataSize);
     api.PopArg(dwProcessId);
     api.PopArg(dwThreadId);
     api.PopArg(dwContinueStatus);
     if(dwProcessId != this->CurProcID){DBGMSG("Process ID mismatch: %08X:%08X",dwProcessId,this->CurProcID);}
     int thidx = this->ThList.FindThreadIdxInList(NULL, dwThreadId);
     if(thidx >= 0)this->ThList.Resume(thidx, NULL, (dwContinueStatus == DBG_CONTINUE)); 
       else {DBGMSG("miContinueDebugEvent: Thread ID not found: ThId=%u",dwThreadId);}   // Can exit before resuming?    
     apo.PushArg(Reslt);
     DBGMSG("miContinueDebugEvent PutMsg: Size=%u",apo.GetLen());
     this->PutMsg(mtDbgRsp, miContinueDebugEvent, Req->Sequence, apo.GetPtr(), apo.GetLen());     
    }
   break;

/*   case miWaitForDebugEvent:     // This is a response only          
    {

    }
   break; */

   case miDebugBreakProcess:  // Unsafe and unstealthy   // Some debuggers will not read any information about a debuggee unless it breaks somwhere (i.e. WinDbg)
    {                      // All of process`s threads may be in some waiting state and because of that a new thread required
     SHM::CArgPack<> api;
     SHM::CArgPack<> apo;
     BOOL   Reslt = TRUE;
     UINT   uExitCode;
     HANDLE hProcess;
     api.Assign((PBYTE)&Req->Data,Req->DataSize);
     api.PopArg(hProcess);
     api.PopArg(uExitCode);
     if(hProcess != UintToFakeHandle(this->CurProcID)){DBGMSG("Process HANDLE mismatch: %08X:%08X",hProcess,UintToFakeHandle(this->CurProcID));}
     CloseHandle(CreateThread(NULL,0,&CDbgClient::DbgBreakThread,this,0,NULL));     // Should wait for it?   // Do not report this thread to debugger? Originally it is reported(DebugBreakProcess)
     apo.PushArg(Reslt);
     DBGMSG("miDebugBreakProcess PutMsg: Size=%u",apo.GetLen());
     this->PutMsg(mtDbgRsp, miDebugBreakProcess, Req->Sequence, apo.GetPtr(), apo.GetLen()); 
    }
   break;
   case miGetThreadContext:
    {                        
     CONTEXT ctx;
     SHM::CArgPack<sizeof(CONTEXT)+sizeof(HANDLE)> api;                
     SHM::CArgPack<sizeof(CONTEXT)+sizeof(HANDLE)> apo;
     api.Assign((PBYTE)&Req->Data,Req->DataSize);
     UINT    ThIdx  = FakeHandleToUint(api.PopArg<HANDLE>());
     HANDLE ThreadHandle = this->ThList.GetHandleByIndex(ThIdx);
     if(!ThreadHandle){DBGMSG("Thread handle not found: ThIdx=%i",ThIdx);}
     api.PopArg(ctx);
     HRESULT Status = STATUS_SUCCESS;
     if(!this->ThList.GetContextVal(ThIdx, &ctx))Status = this->NtDll.NtGetContextThread(ThreadHandle, &ctx);  
       else {DBGMSG("GetContext read locally: %u",ThIdx);}     
     apo.PushArg(ctx);
     apo.PushArg(Status); 
     if(!Status && ThreadHandle){if(this->OnlyOwnHwBP)this->ThList.UpdHardwareBp(ThIdx, &ctx); if(this->OnlyOwnTF)this->ThList.UpdTraceFlag(ThIdx, &ctx);}    // Need update here?
     DBGMSG("miGetThreadContext PutMsg: Status=%08X, Size=%u",Status,apo.GetLen());
     this->PutMsg(mtDbgRsp, miGetThreadContext, Req->Sequence, apo.GetPtr(), apo.GetLen());     
    }
   break;
   case miSetThreadContext:
    {
     CONTEXT ctx;
     SHM::CArgPack<sizeof(CONTEXT)+sizeof(HANDLE)> api;                
     SHM::CArgPack<> apo;
     api.Assign((PBYTE)&Req->Data,Req->DataSize);
     UINT    ThIdx  = FakeHandleToUint(api.PopArg<HANDLE>());
     HANDLE ThreadHandle = this->ThList.GetHandleByIndex(ThIdx);
     if(!ThreadHandle){DBGMSG("Thread handle not found: ThIdx=%i",ThIdx);}   
     api.PopArg(ctx);
     HRESULT Status = STATUS_SUCCESS;
     if(!this->ThList.SetContextVal(ThIdx, &ctx))Status = this->NtDll.NtSetContextThread(ThreadHandle, &ctx);
       else {DBGMSG("SetContext stored locally: %u",ThIdx);}
     DBGMSG("SetContext %u: DbgRegs=%u, Dr7=%p, Dr0=%p, Dr1=%p, Dr2=%p, Dr3=%p",ThIdx,bool(ctx.ContextFlags & CONTEXT_DEBUG_REGISTERS),ctx.Dr7,ctx.Dr0,ctx.Dr1,ctx.Dr2,ctx.Dr3);
     apo.PushArg(Status);
     if(!Status && ThreadHandle){if(this->OnlyOwnHwBP)this->ThList.UpdHardwareBp(ThIdx, &ctx); if(this->OnlyOwnTF)this->ThList.UpdTraceFlag(ThIdx, &ctx);}
     DBGMSG("miSetThreadContext PutMsg: Status=%08X, FLAGS=%08X, Size=%u",Status,ctx.ContextFlags, apo.GetLen());
     this->PutMsg(mtDbgRsp, miSetThreadContext, Req->Sequence, apo.GetPtr(), apo.GetLen());     
    }
   break;
   case miReadVirtualMemory:
    {
     SHM::CArgPack<> api;
     SHM::CArgPack<1024> apo;
     HANDLE ProcessHandle; 
     PVOID BaseAddress;
     SIZE_T BufferLength; 
     SIZE_T  RetLen = 0; 
     HRESULT Status = STATUS_UNSUCCESSFUL;
   
     api.Assign((PBYTE)&Req->Data,Req->DataSize);
     api.PopArg(ProcessHandle);
     api.PopArg(BaseAddress);
     api.PopArg(BufferLength);
     if(ProcessHandle != UintToFakeHandle(this->CurProcID)){DBGMSG("Process HANDLE mismatch: %08X:%08X",ProcessHandle,UintToFakeHandle(this->CurProcID)); apo.PushBlk(BufferLength);}                                        
       else Status = this->NtDll.NtReadVirtualMemory(NtCurrentProcess, BaseAddress, apo.PushBlk(BufferLength), BufferLength, &RetLen); 
     apo.PushArg(RetLen);
     apo.PushArg(Status);
//     DBGMSG("miReadVirtualMemory PutMsg: Status=%08X, BaseAddress=%p, BufferLength=%08X, Size=%u",Status,BaseAddress,BufferLength,apo.GetLen());     
     this->PutMsg(mtDbgRsp, miReadVirtualMemory, Req->Sequence, apo.GetPtr(), apo.GetLen());      
    }
   break;
   case miWriteVirtualMemory:
    {
     SHM::CArgPack<1024> api;
     SHM::CArgPack<> apo;
     HANDLE ProcessHandle; 
     PVOID BaseAddress;
     SIZE_T BufferLength; 
     SIZE_T  RetLen = 0; 
     HRESULT Status = STATUS_UNSUCCESSFUL;   

     api.Assign((PBYTE)&Req->Data,Req->DataSize);
     api.PopArg(ProcessHandle);
     api.PopArg(BaseAddress);
     api.PopArg(BufferLength);
     PBYTE BufPtr = api.PopBlk(BufferLength);
     if(ProcessHandle != UintToFakeHandle(this->CurProcID)){DBGMSG("Process HANDLE mismatch: %08X:%08X",ProcessHandle,UintToFakeHandle(this->CurProcID));}                           
       else 
        {
         if(BufferLength == 1)
          {
           bool IsBP = (*BufPtr == this->SwBpVal);
           if(IsBP)this->BpList.AddBP(BaseAddress);
             else this->BpList.DelBP(BaseAddress);
           DBGMSG("miWriteVirtualMemory: Addr=%p, BYTE=%02X, IsBP=%u",BaseAddress,*BufPtr,IsBP);
           Status = this->NtDll.NtWriteVirtualMemory(NtCurrentProcess, BaseAddress, BufPtr, BufferLength, &RetLen);
           if(Status && IsBP)this->BpList.DelBP(BaseAddress);  // Remove a failed BP  // Case when removing a BP is failed are left undefined
          }
           else Status = this->NtDll.NtWriteVirtualMemory(NtCurrentProcess, BaseAddress, BufPtr, BufferLength, &RetLen);
        }
     apo.PushArg(RetLen);
     apo.PushArg(Status);
     DBGMSG("miWriteVirtualMemory PutMsg: Status=%08X, Size=%u",Status,apo.GetLen());
     this->PutMsg(mtDbgRsp, miWriteVirtualMemory, Req->Sequence, apo.GetPtr(), apo.GetLen());     
    }
   break;
   case miProtectVirtualMemory:       
    {
     SHM::CArgPack<> api;
     SHM::CArgPack<> apo;
     HANDLE ProcessHandle; 
     PVOID BaseAddress;
     SIZE_T RegionSize;
     ULONG NewProtect;
     ULONG OldProtect;
     HRESULT Status = STATUS_UNSUCCESSFUL;
   
     api.Assign((PBYTE)&Req->Data,Req->DataSize);
     api.PopArg(ProcessHandle);
     api.PopArg(BaseAddress);
     api.PopArg(RegionSize);  
     api.PopArg(NewProtect);
     if(ProcessHandle != UintToFakeHandle(this->CurProcID)){DBGMSG("Process HANDLE mismatch: %08X:%08X",ProcessHandle,UintToFakeHandle(this->CurProcID));}                                                                           
       else Status = this->NtDll.NtProtectVirtualMemory(NtCurrentProcess, &BaseAddress, &RegionSize, NewProtect, &OldProtect);
     apo.PushArg(OldProtect);
     apo.PushArg(BaseAddress);
     apo.PushArg(RegionSize);
     apo.PushArg(Status);
     DBGMSG("miProtectVirtualMemory PutMsg: Status=%08X, Size=%u",Status,apo.GetLen());
     this->PutMsg(mtDbgRsp, miProtectVirtualMemory, Req->Sequence, apo.GetPtr(), apo.GetLen());      
    }
   break;
   case miFlushVirtualMemory:       
    {
     SHM::CArgPack<> api;
     SHM::CArgPack<> apo;
     HANDLE ProcessHandle; 
     PVOID BaseAddress;
     SIZE_T RegionSize;
     IO_STATUS_BLOCK IoStatus;
     HRESULT Status = STATUS_UNSUCCESSFUL;

     api.Assign((PBYTE)&Req->Data,Req->DataSize);
     api.PopArg(ProcessHandle);
     api.PopArg(BaseAddress);
     api.PopArg(RegionSize);  
     if(ProcessHandle != UintToFakeHandle(this->CurProcID)){DBGMSG("Process HANDLE mismatch: %08X:%08X",ProcessHandle,UintToFakeHandle(this->CurProcID));}                                                                           
       else Status = this->NtDll.NtFlushVirtualMemory(NtCurrentProcess, &BaseAddress, &RegionSize, &IoStatus);
     apo.PushArg(IoStatus);
     apo.PushArg(BaseAddress);
     apo.PushArg(RegionSize);
     apo.PushArg(Status);
     DBGMSG("miFlushVirtualMemory PutMsg: Status=%08X, Size=%u",Status,apo.GetLen());
     this->PutMsg(mtDbgRsp, miFlushVirtualMemory, Req->Sequence, apo.GetPtr(), apo.GetLen());      
    }
   break;
   case miTerminateThread:
    {
     SHM::CArgPack<> api;
     SHM::CArgPack<> apo;
     NTSTATUS ExitStatus;
     HRESULT Status = STATUS_UNSUCCESSFUL;

     api.Assign((PBYTE)&Req->Data,Req->DataSize);
     UINT    ThIdx = FakeHandleToUint(api.PopArg<HANDLE>());
     HANDLE ThreadHandle = this->ThList.GetHandleByIndex(ThIdx);
     apo.PopArg(ExitStatus);
     if(!ThreadHandle){DBGMSG("Thread handle not found: ThIdx=%i",ThIdx);}
       else Status = (this->AllowThTerm)?(this->NtDll.NtTerminateThread(ThreadHandle, ExitStatus)):(STATUS_SUCCESS);
     apo.PushArg(Status);
     DBGMSG("miTerminateThread PutMsg: Status=%08X, Size=%u",Status,apo.GetLen());
     this->PutMsg(mtDbgRsp, miTerminateThread, Req->Sequence, apo.GetPtr(), apo.GetLen());     
    }
   break;
   case miTerminateProcess:
    {
     SHM::CArgPack<> api;
     SHM::CArgPack<> apo;
     BOOL   Reslt = TRUE;
     UINT   uExitCode;
     HANDLE hProcess;
     api.Assign((PBYTE)&Req->Data,Req->DataSize);
     api.PopArg(hProcess);
     api.PopArg(uExitCode);
     if(hProcess != UintToFakeHandle(this->CurProcID)){DBGMSG("Process HANDLE mismatch: %08X:%08X",hProcess,UintToFakeHandle(this->CurProcID));}
     apo.PushArg(Reslt);
     DBGMSG("miTerminateProcess PutMsg: Size=%u",apo.GetLen());
     this->PutMsg(mtDbgRsp, miTerminateProcess, Req->Sequence, apo.GetPtr(), apo.GetLen()); 
     if(this->AllowPrTerm && (hProcess == UintToFakeHandle(this->CurProcID)))
      {
       this->EndMsg();
       DBGMSG("Terminating self with code %u",uExitCode);
       TerminateProcess(GetCurrentProcess(),uExitCode); 
      }
    }
   break;
   case miSuspendThread:
    {
     SHM::CArgPack<> api;
     SHM::CArgPack<> apo;
     ULONG  PrevCnt = 0;

     api.Assign((PBYTE)&Req->Data,Req->DataSize);
     UINT    ThIdx  = FakeHandleToUint(api.PopArg<HANDLE>());
     HRESULT Status = this->ThList.Suspend(ThIdx, &PrevCnt);
     apo.PushArg(PrevCnt);
     apo.PushArg(Status);
     DBGMSG("miSuspendThread PutMsg: Status=%08X, Index=%u, Size=%u",Status,ThIdx,apo.GetLen());
     this->PutMsg(mtDbgRsp, miSuspendThread, Req->Sequence, apo.GetPtr(), apo.GetLen());     
    }
   break;
   case miResumeThread:
    {
     SHM::CArgPack<> api;
     SHM::CArgPack<> apo;
     ULONG  PrevCnt = 0;

     api.Assign((PBYTE)&Req->Data,Req->DataSize);
     UINT    ThIdx  = FakeHandleToUint(api.PopArg<HANDLE>());
     HRESULT Status = this->ThList.Resume(ThIdx, &PrevCnt);
     apo.PushArg(PrevCnt);
     apo.PushArg(Status);
     DBGMSG("miResumeThread PutMsg: Status=%08X, Index=%u, Size=%u",Status,ThIdx,apo.GetLen());
     this->PutMsg(mtDbgRsp, miResumeThread, Req->Sequence, apo.GetPtr(), apo.GetLen());     
    }
   break;

   default: return -9;
  }
 return 0;
}
//------------------------------------------------------------------------------------
int _stdcall ProcessRequestUsr(SMsgHdr* Req)
{
 switch(Req->MsgID)
  {   
   case miDbgGetConfigs:     // Must be called before irst call to miDbgSetConfigs
    {
     SHM::CArgPack<> apo;
     BYTE Sep = '-';
     if(this->UsrReqCallback)this->UrsOpts = this->UsrReqCallback(Req,&apo,0);   // Some extra configs
     if(this->UrsOpts)apo.PushArgEx(Sep, NULL, dtNull);    // Separator for a custom config records
     UINT OptCtr = this->UrsOpts;
     apo.PushArgEx(this->SwBpVal,      "Value of SwBP",             MakeCfgItemID(++OptCtr,dtBYTE));    
     apo.PushArgEx(this->OnlyOwnTF,    "Report Only Own TF",        MakeCfgItemID(++OptCtr,dtBool));   
     apo.PushArgEx(this->OnlyOwnHwBP,  "Report Only Own HwBP",      MakeCfgItemID(++OptCtr,dtBool));   
     apo.PushArgEx(this->OnlyOwnSwBP,  "Report Only Own SwBP",      MakeCfgItemID(++OptCtr,dtBool));
     apo.PushArgEx(this->HideDbgState, "Hide Debugger State",       MakeCfgItemID(++OptCtr,dtBool));
     apo.PushArgEx(this->AllowThTerm,  "Allow Thread Termination",  MakeCfgItemID(++OptCtr,dtBool));  
     apo.PushArgEx(this->AllowPrTerm,  "Allow Process Termination", MakeCfgItemID(++OptCtr,dtBool));    

     DBGMSG("miDbgGetConfigs PutMsg: Size=%u",apo.GetLen());
     this->PutMsg(mtUsrRsp, miDbgGetConfigs, Req->Sequence, apo.GetPtr(), apo.GetLen());      
    }
    return 0;
   case miDbgSetConfigs:
    {
     SHM::CArgPack<> api;
     SHM::CArgPack<> apo;
     BOOL  Reslt = TRUE;
     api.Assign((PBYTE)&Req->Data,Req->DataSize);
     for(;;)
      {
       UINT Hint   = 0;
       UINT ValLen = 0;
       PBYTE Ptr   = api.PopBlkEx(&ValLen,NULL,&Hint);
       if(!Ptr)break;   // No more configs
       UINT CfgIdx = 0;
       UINT Type   = ReadCfgItemID(Hint, &CfgIdx);
       DBGMSG("miDbgSetConfigs: Type=%u, CfgIdx=%u, UrsOpts=%u",Type,CfgIdx,this->UrsOpts);
       if(CfgIdx > this->UrsOpts)
        {
         CfgIdx -= this->UrsOpts;
         switch(CfgIdx)
          {
           case 1:
            this->SwBpVal      = *Ptr;
            break;
           case 2:
            this->OnlyOwnTF    = *Ptr;
            break;
           case 3:
            this->OnlyOwnHwBP  = *Ptr;
            break;
           case 4:
            this->OnlyOwnSwBP  = *Ptr;
            break;
           case 5:
            this->HideDbgState = *Ptr;
            break;
           case 6:
            this->AllowThTerm  = *Ptr;
            break;
           case 7:
            this->AllowPrTerm  = *Ptr;
            break;
          }
         if(this->UsrReqCallback)this->UsrReqCallback(Req,0,0);     // Notify DBG config change
        }
         else if(this->UsrReqCallback)this->UsrReqCallback(Req,Ptr,Hint); 
      }  
     apo.PushArg(Reslt);
     DBGMSG("miDbgSetConfigs PutMsg: Size=%u",apo.GetLen());
     this->PutMsg(mtUsrRsp, miDbgSetConfigs, Req->Sequence, apo.GetPtr(), apo.GetLen());     
    }
   return 0;
  }
    
 if(this->UsrReqCallback)return this->UsrReqCallback(Req,0,0);
 return 0;
}
//------------------------------------------------------------------------------------

public:
 int (_stdcall* UsrReqCallback)(SMsgHdr* Req, PVOID ArgA, UINT ArgB);
 bool   AllowPrTerm;
 bool   AllowThTerm;
 bool   HideDbgState;
 bool   EvtSuspAllTh;  // Other threads may measure execution time and fail if suspended (Protectors) // Defauld debug behavoiur is to suspend all threads for eash debug event
 bool   OnlyOwnSwBP;
 bool   OnlyOwnHwBP;
 bool   OnlyOwnTF;
 BYTE   SwBpVal;

static inline UINT MakeCfgItemID(UINT CfgIdx, UINT CfgType){return (CfgIdx << 8)|CfgType;}
static inline UINT ReadCfgItemID(UINT Value, UINT* CfgIdx){if(CfgIdx)*CfgIdx = Value >> 8; return Value & 0xFF;}
static HANDLE UintToFakeHandle(UINT Idx){return HANDLE(~Idx << 5);}
static UINT   FakeHandleToUint(HANDLE hTh){return (~(UINT)hTh >> 5);}
static bool   IsFakeHandle(HANDLE hTh){return !((UINT)hTh & 0x1F) && ((UINT)hTh & 0xF0000000);}      // Pointer Type?
bool IsDbgThreadID(DWORD ThID){return (ThID == this->ClientThID);}
bool IsActive(void){return ((bool)this->hIPCThread && !this->BreakWrk);}
//------------------------------------------------------------------------------------
// Different threads of a debugger will direct their requests here
//
static DWORD WINAPI IPCQueueThread(LPVOID lpThreadParameter)
{    
 DBGMSG("Enter: ThreadId=%u",GetCurrentThreadId());
 CDbgClient* DbgIPC = (CDbgClient*)lpThreadParameter;
 DbgIPC->ClientThID = GetCurrentThreadId();
 DbgIPC->Connect(GetCurrentProcessId(),DbgIPC->IPCSize);       // Create SharedBuffer name for current process, a debugger will connect to it
 DbgIPC->BreakWrk = false;
 while(!DbgIPC->BreakWrk) 
  { 
   SMsgHdr* Cmd = DbgIPC->GetMsg();
   if(!Cmd)continue;   // Timeout and still no messages
//   DBGMSG("MsgType=%04X, MsgID=%04X, DataID=%08X, Sequence=%08X, DataSize=%08X",Cmd->MsgType,Cmd->MsgID,Cmd->DataID,Cmd->Sequence,Cmd->DataSize);   // These Debug messages make it hang!!!
   if(Cmd->MsgType & mtDbgReq)DbgIPC->ProcessRequestDbg(Cmd);
   if(Cmd->MsgType & mtUsrReq)DbgIPC->ProcessRequestUsr(Cmd);  
  }
 DbgIPC->EndMsg();   // Unlock shared buffer if it is still locked
 DbgIPC->ipc.Clear();
 DbgIPC->Disconnect();
 DBGMSG("Exit");
 return 0;
}
//------------------------------------------------------------------------------------
CDbgClient(void)
{
 this->EvtSuspAllTh = true;
 this->HideDbgState = true;
 this->AllowPrTerm  = true;
 this->AllowThTerm  = true;
 this->OnlyOwnSwBP  = true;
 this->OnlyOwnHwBP  = true;
 this->OnlyOwnTF    = true;
 this->SwBpVal      = 0xCC;    // May be required to change this(to INVALID OPCODE) if a kernel driver monitors INT3

 this->hIPCThread     = NULL;
 this->UsrReqCallback = NULL;
 this->ClientThID = 0;
 this->UrsOpts    = 0;
 this->IPCSize    = (1024*1024)*4;  // Smaller is faster   // More DLLs in a target application, more buffer size it will require on attach
 this->BreakWrk   = true;
 this->hClientDll = NULL;
 this->CurProcID  = GetCurrentProcessId(); 
 this->MainThID   = 0;
 GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCSTR)&CDbgClient::IPCQueueThread, &this->hClientDll);
}
//------------------------------------------------------------------------------------
~CDbgClient()
{
 this->Stop();
}
//------------------------------------------------------------------------------------
bool Start(UINT Size=0, HANDLE hThread=NULL, PVOID IPCThProc=NULL)
{        
 if(this->IsActive()){DBGMSG("Already active!"); return false;}
 if(Size)this->IPCSize = Size;
 if(!hThread)
  {
   DBGMSG("IPC thread proc: %p",&CDbgClient::IPCQueueThread);
   this->hIPCThread = CreateThread(NULL,0,&CDbgClient::IPCQueueThread,this,0,NULL);
  }
   else
    {
     DBGMSG("Reusing an existing thread: %p",hThread);
     this->hIPCThread = hThread;
     PVOID Param = this;
     if(!IPCThProc)IPCThProc = &CDbgClient::IPCQueueThread;
     if(!SNtDll::ChangeNewSuspThProcAddr(this->hIPCThread, IPCThProc, &Param))return false;
     ResumeThread(hThread);    
    }
 if(!this->hIPCThread){DBGMSG("Failed to create IPC thread: %u", GetLastError());}
 return (bool)this->hIPCThread;
}
//------------------------------------------------------------------------------------
bool Stop(void)
{
 if(!this->IsActive()){DBGMSG("Not active!"); return false;}
 this->BreakWrk = true;
 if(this->ClientThID != GetCurrentThreadId())
  {
   DBGMSG("Waiting for ClientThread termination");
   DWORD res = WaitForSingleObject(this->hIPCThread,INFINITE); 
   if(WAIT_OBJECT_0 != res)TerminateThread(this->hIPCThread,0);
  }
 CloseHandle(this->hIPCThread);
 this->EndMsg();   // Unlock shared buffer if it is still locked
 this->ipc.Clear();
 this->Disconnect();
 this->hIPCThread = NULL;
 this->ThList.Clear();
 DBGMSG("Done");
 return true;
}
//------------------------------------------------------------------------------------
bool HandleException(DWORD ThID, PEXCEPTION_RECORD ExceptionRecord, PCONTEXT Context)     // TODO: Detect and ignore BP/TF exceptions which are not caused by debugger   // TODO: EXCEPTION_ILLEGAL_INSTRUCTION breakpoints
{  
 if(!ExceptionRecord || !Context || this->IsDbgThreadID(ThID))return false;  // Ignore exceptions from DbgThread. It will stuck if a debugger gets this
 DBGMSG("Code=%08X, Addr=%p, FCtx=%08X",ExceptionRecord->ExceptionCode, ExceptionRecord->ExceptionAddress, Context->ContextFlags);
 int thidx = -1;
 CThreadList::SThDesc* Desc = NULL;
 switch(ExceptionRecord->ExceptionCode)
  {
   case EXCEPTION_BREAKPOINT:    // INT3  // IP points to INT3, not after it(Like with DebugAPI), so IP must be incremented
    if(this->OnlyOwnSwBP && !CDbgClient::IsAddrInDbgBreak(ExceptionRecord->ExceptionAddress) && !this->BpList.IsHitBP(ExceptionRecord->ExceptionAddress)){DBGMSG("SkipSwBP: Addr=%p",ExceptionRecord->ExceptionAddress); return false;}     // This INT3 breakpoint was not set by our debugger
    thidx = this->GetThread(ThID, &Desc);      // Reports this thread if sees it first time
#ifdef _AMD64_
     Context->Rip++;
#else
     Context->Eip++;
#endif
   case EXCEPTION_SINGLE_STEP:   // TF/DRx
    {    
     if(thidx < 0)    // EXCEPTION_SINGLE_STEP
      {
       thidx = this->GetThread(ThID, &Desc);    
       bool TFFail = (this->OnlyOwnTF   && !this->ThList.IsSingleStepping(thidx));
       bool HwFail = (this->OnlyOwnHwBP && !this->ThList.IsHardwareBpHit(thidx,ExceptionRecord->ExceptionAddress));
       if((this->OnlyOwnHwBP || this->OnlyOwnTF) && (HwFail && TFFail)){DBGMSG("SkipHwBP: Addr=%p, TFFail=%u, HwFail=%u",ExceptionRecord->ExceptionAddress,TFFail,HwFail); return false;}     // Not TF and not a HwBp
      }
     Desc->pContext = Context;          // GetThreadContext/SetThreadContext will access this context instead of latest which is in this ExceptionHandler now
     this->Report_EXCEPTION_DEBUG_INFO(thidx, ExceptionRecord);
//     this->ThList.Suspend(thidx);   // moved to Report_EXCEPTION_DEBUG_INFO
     Desc->pContext = NULL;             // Can someone remove this thread from list before resuming it?
     DBGMSG("Resumed: %u",Desc->DbgCont);
    }
    return Desc->DbgCont;        // Continued by debugger
  }
 return false;
} 
//------------------------------------------------------------------------------------
bool TryAddCurrThread(void)
{
 TEB* CurTeb = NtCurrentTeb();
 if(this->ThList.FindThreadIdxInList(NULL, (UINT)CurTeb->ClientId.UniqueThread) >= 0)return false;  // Already in list
 this->Report_CREATE_THREAD_DEBUG_EVENT(CurTeb, true); 
 DBGMSG("Added: ThreadId=%u",(UINT)CurTeb->ClientId.UniqueThread);
 return true;
}
//------------------------------------------------------------------------------------
int GetThread(DWORD ThID, CThreadList::SThDesc** Desc=NULL)
{
 return this->ThList.FindThreadIdxInList(Desc, ThID);
}
//------------------------------------------------------------------------------------
int DebugThreadLoad(DWORD ThID, PCONTEXT Context) 
{
 if(!this->IsActive() || !this->HideDbgState)return -1;
 int thidx = this->GetThread(ThID);
 if(thidx < 0)return -2;
 this->ThList.WriteDbgContext(thidx, Context);    // Restore fake DRx from our buffer to Context   // What about TF?
 return thidx;
}
//------------------------------------------------------------------------------------
int DebugThreadSave(DWORD ThID, PCONTEXT Context)    // Call this after an application has modified the CONTEXT
{
 if(!this->IsActive() || !this->HideDbgState)return -1;
 int thidx = this->GetThread(ThID);
 if(thidx < 0)return -2;
 this->ThList.ReadDbgContext(thidx, Context);   // Save (Possibly modified) DRx into our internal buffer
 return thidx;
}
//------------------------------------------------------------------------------------
void DebugRstExcContext(PCONTEXT ExcCont, PCONTEXT HandledCtx)
{
 if(ExcCont->ContextFlags & CONTEXT_DEBUG_REGISTERS)
  {
   HandledCtx->Dr0 = ExcCont->Dr0;
   HandledCtx->Dr1 = ExcCont->Dr1;
   HandledCtx->Dr2 = ExcCont->Dr2;
   HandledCtx->Dr3 = ExcCont->Dr3;
   HandledCtx->Dr6 = ExcCont->Dr6;
   HandledCtx->Dr7 = ExcCont->Dr7;
   HandledCtx->ContextFlags |= CONTEXT_DEBUG_REGISTERS; 
  }
 memcpy(ExcCont,HandledCtx,sizeof(CONTEXT));
}
//------------------------------------------------------------------------------------
SIZE_T IsMemAvailable(PVOID Addr)      // Helps to skip a Reserved mapping rgions
{
 SIZE_T RetLen = 0; 
 MEMORY_BASIC_INFORMATION minf;                                                                            
 NTSTATUS Status = this->NtDll.NtQueryVirtualMemory(NtCurrentProcess,Addr,MemoryBasicInformation,&minf,sizeof(MEMORY_BASIC_INFORMATION),&RetLen);
 if(Status || !(minf.State & MEM_COMMIT))return 0;
 return minf.RegionSize;
}
//------------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------------
int Report_CREATE_PROCESS_DEBUG_EVENT(TEB* pMainThTeb, bool Suspend=true)    // NOTE: Opening a process is more suspicious than opening a thread
{
 if(!this->IsActive())return -9;                // <<<<<<<<<<<<<<<<<<<<<<<< Threads identified by Index ????????????????????????????????????????
 DbgEvtEx evt; 
 wchar_t PathBuf[MAX_PATH]; 
 evt.dwDebugEventCode = CREATE_PROCESS_DEBUG_EVENT;
 evt.dwProcessId = (UINT)pMainThTeb->ClientId.UniqueProcess;  // this->CurProcID;
 evt.dwThreadId  = (UINT)pMainThTeb->ClientId.UniqueThread;   // this->MainThID;   // This ID is not used by TitanEngine(Set to NULL)            // GetCurrentThreadId();  
   
 int TIdx = this->ThList.AddThreadToList(pMainThTeb);
 if(TIdx < 0){DBGMSG("Failed to add the thread %u", evt.dwThreadId); }

 *evt.FilePath                                 = 0;   
 evt.u.CreateProcessInfo.hFile                 = NULL;              // Cannot allow an another process to have one of DLLs handle opened because some protection driver may track it
 evt.u.CreateProcessInfo.hProcess              = UintToFakeHandle(evt.dwProcessId);  // HANDLE(0 - evt.dwProcessId);    // x64Dbg uses this handles directly and will stop working without them
 evt.u.CreateProcessInfo.hThread               = UintToFakeHandle(TIdx);  // HANDLE(0 - this->ThList.AddThreadToList(evt.dwThreadId) - 1);
 evt.u.CreateProcessInfo.lpBaseOfImage         = GetModuleHandleA(NULL);      // HANDLE is not always a Base?
 evt.u.CreateProcessInfo.dwDebugInfoFileOffset = 0;
 evt.u.CreateProcessInfo.nDebugInfoSize        = 0;
 evt.u.CreateProcessInfo.lpThreadLocalBase     = pMainThTeb;  
 evt.u.CreateProcessInfo.lpStartAddress        = NULL;   // May be required!
 evt.u.CreateProcessInfo.lpImageName           = NULL;
 evt.u.CreateProcessInfo.fUnicode              = 0;
 evt.PathSize = GetModuleFileNameW(GetModuleHandleW(NULL),(PWSTR)&evt.FilePath,MAX_PATH); 
 UINT MemSize = sizeof(evt)+(evt.PathSize * sizeof(wchar_t));
 DBGMSG("PutMsg: CurProcID=%u, MainThID=%u, Size=%u",evt.dwProcessId,evt.dwThreadId,MemSize);
 PVOID MsgPtr = NULL;
 this->DoSuspendThreads(evt.dwThreadId, -1, false, true);     //     if(Suspend)this->ThList.Suspend(TIdx);   // Suspend the specified thread (Never a current one)
 if(this->BeginMsg(&MsgPtr, mtDbgRsp, miWaitForDebugEvent, 0, MemSize, 0) < 0)return -5;
 DbgEvtEx* DstEvt = (DbgEvtEx*)MsgPtr;
 memcpy(DstEvt,&evt,sizeof(evt));
 memcpy(&DstEvt->FilePath, &PathBuf, (evt.PathSize+1) * sizeof(wchar_t));   // Including 0
 return this->DoneMsg(NULL);   // this->PutMsg(mtDbgRsp, miWaitForDebugEvent, 0, &evt, sizeof(evt)); 
}
//------------------------------------------------------------------------------------
int Report_EXIT_PROCESS_DEBUG_EVENT(DWORD ExitCode)      // Unused and unreliable  // A process lives until any of its threads alive
{
 if(!this->IsActive())return -9;
 DbgEvtEx evt;
 evt.dwDebugEventCode = EXIT_PROCESS_DEBUG_EVENT;
 evt.dwProcessId = this->CurProcID;
 evt.dwThreadId  = this->MainThID;   // May be incorrect(Terminated)
 
 evt.u.ExitProcess.dwExitCode = ExitCode;
 DBGMSG("PutMsg: ExitCode=%08X, Size=%08X",ExitCode,sizeof(evt));        
 int res = this->PutMsg(mtDbgRsp, miWaitForDebugEvent, 0, &evt, sizeof(evt));  
 this->DoSuspendThreads(evt.dwThreadId, -1, true);
 return res;
}
//------------------------------------------------------------------------------------
int Report_CREATE_THREAD_DEBUG_EVENT(TEB* pThTeb, bool SingleThSusp=false)
{
 if(!this->IsActive())return -9;
 DbgEvtEx evt;
 evt.dwDebugEventCode = CREATE_THREAD_DEBUG_EVENT;
 evt.dwProcessId = this->CurProcID;
 evt.dwThreadId  = (UINT)pThTeb->ClientId.UniqueThread;
 int TIdx = this->ThList.AddThreadToList(pThTeb);
 if(TIdx < 0){DBGMSG("Failed to add the thread %u", evt.dwThreadId); return -8;}
 
 evt.u.CreateThread.hThread           = UintToFakeHandle(TIdx);   // HANDLE(0 - this->ThList.AddThreadToList(ThreadID) - 1);   // Not a real handle but easy to identify
 evt.u.CreateThread.lpStartAddress    = NULL;   // May be required!
 evt.u.CreateThread.lpThreadLocalBase = pThTeb;   
 DBGMSG("PutMsg: ThreadID=%08X(%u), Size=%08X",evt.dwThreadId,evt.dwThreadId,sizeof(evt));
 int res = this->PutMsg(mtDbgRsp, miWaitForDebugEvent, 0, &evt, sizeof(evt));  
 this->DoSuspendThreads(evt.dwThreadId, -1, false, SingleThSusp);  // if(Suspend)this->ThList.Suspend(TIdx);   // Suspends the specified thread
 return res;
}
//------------------------------------------------------------------------------------
int Report_EXIT_THREAD_DEBUG_EVENT(DWORD ThreadID, DWORD ExitCode)
{
 if(!this->IsActive())return -9;
 DbgEvtEx evt;
 evt.dwDebugEventCode = EXIT_THREAD_DEBUG_EVENT;
 evt.dwProcessId = this->CurProcID;
 evt.dwThreadId  = ThreadID;    
 
 evt.u.ExitThread.dwExitCode = ExitCode;
 if(!this->ThList.RemoveThreadFromList(ThreadID)){DBGMSG("Unregistered thread: ThreadID=%08X(%u)",ThreadID,ThreadID); return -3;};
 DBGMSG("PutMsg: ThreadID=%08X(%u), ExitCode=%08X, Size=%08X",ThreadID,ThreadID,ExitCode,sizeof(evt));
 int res = this->PutMsg(mtDbgRsp, miWaitForDebugEvent, 0, &evt, sizeof(evt));     
 this->DoSuspendThreads(evt.dwThreadId, -1, true);  //if(Suspend)this->ThList.Suspend(this->ThList.FindThreadIdxInList(nullptr, evt.dwThreadId), true);  // Suspends this thread. It will be removed after next Resume call  // Lets hope that it will be executed before miContinueDebugEvent
 return res; 
}
//------------------------------------------------------------------------------------
// TODO: Accept not only mapped PE images?
int Report_LOAD_DLL_DEBUG_INFO(PVOID DllBase, bool DoSuspend=true)    // <<<<<<<<<<<<<<< TODO: Name resolving   // TODO: More checks that PE module is valid
{
 if(!this->IsActive())return -9;
 DbgEvtEx evt;
 wchar_t PathBuf[MAX_PATH];
 evt.dwDebugEventCode = LOAD_DLL_DEBUG_EVENT;
 evt.dwProcessId = this->CurProcID;
 evt.dwThreadId  = NtCurrentThreadId();    // Always acceptable?
 if(evt.dwThreadId == this->ClientThID)evt.dwThreadId = this->MainThID;    // Hide Client thread from debugger
 
 *evt.FilePath                       = 0;
 evt.u.LoadDll.hFile                 = NULL;        // NOTE: TitanEngine will try to call CreateFileMappingA on it
 evt.u.LoadDll.lpBaseOfDll           = DllBase;
 evt.u.LoadDll.dwDebugInfoFileOffset = 0;
 evt.u.LoadDll.nDebugInfoSize        = 0;
 evt.u.LoadDll.lpImageName           = NULL;   // Find it in DLL or in PEB
 evt.u.LoadDll.fUnicode              = 0;
 evt.PathSize = GetModuleFileNameW((HMODULE)DllBase,PathBuf,sizeof(PathBuf)/sizeof(wchar_t)); // TODO: Take it From mapping  // Base is always same as HMODULE?
 UINT MemSize = sizeof(evt)+(evt.PathSize * sizeof(wchar_t));
 DBGMSG("PutMsg: DllBase=%p, Size=%u, DllPath='%ls'",DllBase,MemSize,&PathBuf);

 PVOID MsgPtr = NULL;
 if(this->BeginMsg(&MsgPtr, mtDbgRsp, miWaitForDebugEvent, 0, MemSize, 0) < 0)return -5;
 DbgEvtEx* DstEvt = (DbgEvtEx*)MsgPtr;
 memcpy(DstEvt,&evt,sizeof(evt));
 memcpy(&DstEvt->FilePath, &PathBuf, (evt.PathSize+1) * sizeof(wchar_t));   // Including 0
 int res = this->DoneMsg(NULL);   // this->PutMsg(mtDbgRsp, miWaitForDebugEvent, 0, &evt, sizeof(evt));  
 if(DoSuspend)this->DoSuspendThreads(evt.dwThreadId);    //  if(Suspend)this->DoSuspendThreads(evt.dwThreadId);  //  this->ThList.Suspend(this->ThList.FindThreadIdxInList(nullptr, evt.dwThreadId));  // Suspends this thread
 return res;
}
//------------------------------------------------------------------------------------
int Report_UNLOAD_DLL_DEBUG_EVENT(PVOID DllBase)
{
 if(!this->IsActive())return -9;
 DbgEvtEx evt;
 evt.dwDebugEventCode = UNLOAD_DLL_DEBUG_EVENT;
 evt.dwProcessId = this->CurProcID;
 evt.dwThreadId  = NtCurrentThreadId();    // Always acceptable?   
 
 evt.u.UnloadDll.lpBaseOfDll = DllBase;
 DBGMSG("PutMsg: DllBase=%p, Size=%08X",DllBase,sizeof(evt));
 int res = this->PutMsg(mtDbgRsp, miWaitForDebugEvent, 0, &evt, sizeof(evt)); 
 this->DoSuspendThreads(evt.dwThreadId);  // this->ThList.Suspend(this->ThList.FindThreadIdxInList(nullptr, evt.dwThreadId));  // Suspends this thread
 return res;
}
//------------------------------------------------------------------------------------
int Report_EXCEPTION_DEBUG_INFO(int ThreadIdx, PEXCEPTION_RECORD ExceptionRecord, BOOL FirstChance=1)   // No suspending here, do it in HandleException where the thread index is already found
{
 if(!this->IsActive())return -9;
 DbgEvtEx evt;           
 evt.dwDebugEventCode = EXCEPTION_DEBUG_EVENT;
 evt.dwProcessId = this->CurProcID;
 evt.dwThreadId  = (ThreadIdx >= 0)?(this->ThList.GetThreadDesc(ThreadIdx)->ThreadID):(0);
 
 evt.u.Exception.dwFirstChance = FirstChance;
 memcpy(&evt.u.Exception.ExceptionRecord,ExceptionRecord,sizeof(EXCEPTION_RECORD));
 DBGMSG("PutMsg: ThID=%08X(%u), Code=%08X, Addr=%p, Size=%08X",evt.dwThreadId,evt.dwThreadId,ExceptionRecord->ExceptionCode,ExceptionRecord->ExceptionAddress,sizeof(evt));
 int res = this->PutMsg(mtDbgRsp, miWaitForDebugEvent, 0, &evt, sizeof(evt));  
 this->DoSuspendThreads(evt.dwThreadId, ThreadIdx);
 return res;
}
//------------------------------------------------------------------------------------

};
//====================================================================================
static ULONG GetProcessID(HANDLE hProcess)
{
 if(!hProcess || (hProcess == NtCurrentProcess))return NtCurrentProcessId();     
 THREAD_BASIC_INFORMATION tinf;
 ULONG RetLen = 0;
 HRESULT res  = NtQueryInformationThread(NtCurrentThread,ThreadBasicInformation,&tinf,sizeof(THREAD_BASIC_INFORMATION),&RetLen);
 if(res){DBGMSG("Failed to get process ID: %08X", res); return 0;}   // 0 id belongs to the system 
 return (ULONG)tinf.ClientId.UniqueProcess;
}
//------------------------------------------------------------------------------------
static ULONG GetThreadID(HANDLE hThread, ULONG* ProcessID=NULL)
{
 if(!hThread || (hThread == NtCurrentThread))return NtCurrentThreadId();     
 THREAD_BASIC_INFORMATION tinf;
 ULONG RetLen = 0;
 HRESULT res  = NtQueryInformationThread(hThread,ThreadBasicInformation,&tinf,sizeof(THREAD_BASIC_INFORMATION),&RetLen);
 if(res){DBGMSG("Failed to get thread ID: %08X", res); return 0;}   // 0 id belongs to the system 
 if(ProcessID)*ProcessID = (ULONG)tinf.ClientId.UniqueProcess;
 return (ULONG)tinf.ClientId.UniqueThread;
}
//------------------------------------------------------------------------------------
static ULONG GetCurrProcessThreadID(HANDLE hThread)
{
 ULONG PrID = 0;
 ULONG ThID = GetThreadID(hThread, &PrID);
 if(!ThID || (PrID != NtCurrentProcessId()))return 0;
 return ThID;
}
//------------------------------------------------------------------------------------
static bool IsCurrentProcess(HANDLE hProcess)
{
 return GetProcessID(hProcess) == NtCurrentProcessId();
}
//------------------------------------------------------------------------------------
static bool IsCurrentThread(HANDLE hThread)
{
 return GetThreadID(hThread) == NtCurrentThreadId();
}
//------------------------------------------------------------------------------------
static bool IsCurrentProcessThread(HANDLE hThread)
{
 return GetCurrProcessThreadID(hThread);
}
//====================================================================================
};