
#pragma once 
/*
  Copyright (c) 2020 Victor Sheinmann, Vicshann@gmail.com

  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), 
  to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
  and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. 
*/

struct NGhDbg
{
typedef NShMem SHM;     // Shorter and helps in case of renaming

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
class CThreadList    // TODO: Decompose or reorganize
{
public:
struct SThDesc
{
 enum ThFlags {tfNone=0,tfTraceFlg=0x01,tfDbgCont=0x02,tfOpenedHnd=0x04,tfMarkedForRemove=0x08,tfSuspendedForAll=0x10,  tfTraceFlagMsk=0x0100};
 volatile DWORD  Flags;
 volatile DWORD  Index;  
 volatile DWORD  ThreadID;  // Not WORD
 volatile HANDLE hThread;
 volatile PVOID  pContext;  // Non NULL only if thread is in Exception Handler (Suspended) 
 volatile TEB*   pThTeb;
 struct SHwBp
  {
   PBYTE Addr;
   DWORD Size;
  } volatile HwBpLst[4]; 
 struct SDbgCtx
  {
   SIZE_T Dr0;
   SIZE_T Dr1;
   SIZE_T Dr2;
   SIZE_T Dr3;
   SIZE_T Dr6;
   SIZE_T Dr7;
   bool   TrFlg;    // Unreliable, because this flag can be accessed with pushf/popf
  } volatile DbgCtx;  
//------------------------------------------------------------------------------------  
inline void  SyncResFlags(DWORD Flg){_InterlockedAnd((long*)&this->Flags,~Flg);}
inline void  SyncSetFlags(DWORD Flg){_InterlockedOr((long*)&this->Flags,Flg);}
inline DWORD SyncGetFlags(DWORD Flg){return this->Flags & Flg;}
inline void  SetTraceFlagState(bool State)
{
 if(State)this->SyncSetFlags(tfTraceFlg); 
   else this->SyncResFlags(tfTraceFlg);
}
//------------------------------------------------------------------------------------
inline void  SetDbgContinueState(bool State)
{
 if(State)this->SyncSetFlags(tfDbgCont); 
   else this->SyncResFlags(tfDbgCont);
}
//------------------------------------------------------------------------------------

};


private:   
 SHM::CGrowArr<SThDesc, 128> ThreadLst; 
 SHM::CArgPack<sizeof(DbgEvtEx) * 32> EvtStk;
 SHM::CCritSectEx<> csec; 
 int TotalInStack;

//------------------------------------------------------------------------------------
void RemoveThreadFromListByIdx(DWORD Index)
{
 if((this->ThreadLst[Index].Flags & SThDesc::tfOpenedHnd) && this->ThreadLst[Index].hThread)NtClose(this->ThreadLst[Index].hThread);
// DBGMSG("Index=%u, ThreadID=%u",Index,this->ThreadLst[Index].ThreadID); 
 this->ThreadLst[Index].hThread  = NULL;          
 this->ThreadLst[Index].ThreadID = 0;
}
//------------------------------------------------------------------------------------
SThDesc* GetThreadDesc(UINT Index)      
{
 if(!this->IsThreadIndexExist(Index))return NULL;
 return &this->ThreadLst[Index];
}
//------------------------------------------------------------------------------------

public:
 UINT DbgEventCnt;

//------------------------------------------------------------------------------------
CThreadList(void)
{
 this->TotalInStack = 0;
 this->DbgEventCnt  = 0;
 this->Clear();
}
//------------------------------------------------------------------------------------
~CThreadList()
{
 this->Clear();
}
//------------------------------------------------------------------------------------
void Lock(void)
{
// DBGMSG("Locking..."); 
 this->csec.Lock(); 
// DBGMSG("Locked"); 
}
//------------------------------------------------------------------------------------
void UnLock(void)
{
// DBGMSG("UnLocking...");
 this->csec.Unlock();
// DBGMSG("Unlocked");
}
//------------------------------------------------------------------------------------
int EventsInStack(void){return this->TotalInStack;}
//------------------------------------------------------------------------------------
DbgEvtEx* PushDbgEvent(UINT ExtraSize=0)   // Requires ThreadList lock
{
 this->TotalInStack++;
 return (DbgEvtEx*)this->EvtStk.PushBlkEx(sizeof(DbgEvtEx) + ExtraSize);
}
//------------------------------------------------------------------------------------
DbgEvtEx* PopDbgEvent(UINT* Size=NULL)    // Requires ThreadList lock
{
 if(this->TotalInStack <= 0){DBGMSG("No events in stack!"); return NULL;}
 this->TotalInStack--;
 return (DbgEvtEx*)this->EvtStk.PopBlkEx(Size);
}
//------------------------------------------------------------------------------------
DbgEvtEx* GetDbgEventAt(UINT& Offset)
{
 return (DbgEvtEx*)this->EvtStk.GetBlkAt(Offset);
}
//------------------------------------------------------------------------------------

// Windows Server 2003 and Windows XP: The value of the THREAD_ALL_ACCESS flag increased on Windows Server 2008 and Windows Vista. If an application compiled for Windows Server 2008 and 
// Windows Vista is run on Windows Server 2003 or Windows XP, the THREAD_ALL_ACCESS flag contains access bits that are not supported and the function specifying this flag fails with ERROR_ACCESS_DENIED. 
//
int AddThreadToList(TEB* pThTeb, DWORD ThreadID=0, HANDLE hThread=NULL, LARGE_INTEGER* CreateTime=NULL, bool CanOpen=true)
{ 
 int NewThIdx = -1;
 if(!ThreadID && pThTeb)ThreadID = (DWORD)pThTeb->ClientId.UniqueThread;
 for(UINT ctr=0,total=this->ThreadLst.Count();ctr < total;ctr++)
  {
   SThDesc* ThDes = &this->ThreadLst[ctr];
   if(ThreadID && (ThDes->ThreadID == ThreadID))return ctr;      // Already in list
   if(hThread  && (ThDes->hThread  == hThread ))return ctr;      // Already in list
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
 ThDes->pContext = NULL;
 ThDes->pThTeb   = pThTeb;
 if(CanOpen && !hThread)
  {
   CLIENT_ID CliID;
   OBJECT_ATTRIBUTES ObjAttr;
   CliID.UniqueThread  = (HANDLE)SIZE_T(ThreadID);
   CliID.UniqueProcess = 0;
   ObjAttr.Length = sizeof(ObjAttr);
   ObjAttr.RootDirectory = NULL;  
   ObjAttr.Attributes = 0;           // bInheritHandle ? 2 : 0;
   ObjAttr.ObjectName = NULL;
   ObjAttr.SecurityDescriptor = ObjAttr.SecurityQualityOfService = NULL;
   if(NTSTATUS stat = NtOpenThread((HANDLE*)&ThDes->hThread, SYNCHRONIZE|THREAD_GET_CONTEXT|THREAD_SET_CONTEXT|THREAD_QUERY_INFORMATION|THREAD_SET_INFORMATION|THREAD_SUSPEND_RESUME|THREAD_TERMINATE, &ObjAttr, &CliID))   
    {
     ThDes->hThread  = NULL;
     ThDes->ThreadID = 0;
     DBGMSG("Failed to open thread %u at %u: %08X",ThreadID,NewThIdx,stat);
     return -1;
    }
   ThDes->Flags = SThDesc::tfOpenedHnd;
   if(ThreadID != NtCurrentThreadId())
    {
     if(NTSTATUS stat = NtSuspendThread(ThDes->hThread, NULL)){DBGMSG("Failed to suspend: %08X",stat);}
       else ThDes->Flags |= SThDesc::tfSuspendedForAll;
    }
   if(CreateTime)
    {
     KERNEL_USER_TIMES times;
     ULONG RetLen = 0;
     if(NTSTATUS stat = NtQueryInformationThread(ThDes->hThread,ThreadTimes,&times,sizeof(times),&RetLen)){DBGMSG("Failed to query thread times: %08X",stat);}  // A debugger will request this many times anyway
     CreateTime->QuadPart = times.CreateTime.QuadPart;
    }
  }
   else
    {
     ThDes->Flags   = SThDesc::tfNone;
     ThDes->hThread = hThread;   
    }
 DBGMSG("ThreadID=%08X(%u), hThread=%08X, CreateTime=%016llX, Index=%u",ThDes->ThreadID,ThDes->ThreadID,ThDes->hThread,(CreateTime?CreateTime->QuadPart:0),NewThIdx);
 return NewThIdx;
}
//------------------------------------------------------------------------------------
static bool IsCurrentThreadDesc(SThDesc* Desc, TEB* CurrTeb)
{
 if(Desc->pThTeb != CurrTeb)return false;
 if(Desc->ThreadID != (UINT)CurrTeb->ClientId.UniqueThread){DBGMSG("Inconsistent Thread entry %u for %u(%u)!",Desc->Index,Desc->ThreadID,(UINT)CurrTeb->ClientId.UniqueThread); return false;}   // Current TEB has been reused!
 return true;
}
//------------------------------------------------------------------------------------
bool IsThreadInList(DWORD ThreadID, HANDLE hThread=NULL){return (this->FindThreadIdxInList(NULL, ThreadID, hThread) >= 0);}
bool IsThreadIndexExist(UINT Index){return ((Index < this->ThreadLst.Count()) && this->ThreadLst[Index].hThread);}
//------------------------------------------------------------------------------------
PTEB GetTebByIndex(UINT Index)
{
 PTEB val = NULL;
 if(Index < this->ThreadLst.Count())val = (PTEB)this->ThreadLst[Index].pThTeb;  // NULL if the slot is empty
 return val;
}
//------------------------------------------------------------------------------------
HANDLE GetHandleByIndex(UINT Index)
{
 HANDLE val = NULL;
 if(Index < this->ThreadLst.Count())val = this->ThreadLst[Index].hThread;  // NULL if the slot is empty
 return val;
}
//------------------------------------------------------------------------------------
HANDLE GetHandleByID(UINT ThreadID)
{
 int Idx = this->FindThreadIdxInList(NULL, ThreadID, NULL);
 if(Idx < 0)return NULL;
 if(Idx < (int)this->ThreadLst.Count())return this->ThreadLst[Idx].hThread;  // NULL if the slot is empty
 return NULL;
}
//------------------------------------------------------------------------------------
int FindThreadIdxInList(SThDesc** Res, DWORD ThreadID, HANDLE hThread=NULL)   // Returns a first thread in the list if both ThreadID and hThread are NULL
{
 for(UINT ctr=0,total=this->ThreadLst.Count();ctr < total;ctr++)
  {
   if(!this->ThreadLst[ctr].hThread)continue;     // The slot is empty
   if(hThread  && (this->ThreadLst[ctr].hThread  != hThread))continue;
   if(ThreadID && (this->ThreadLst[ctr].ThreadID != ThreadID))continue; 
   if(Res)*Res = &this->ThreadLst[ctr]; 
   return ctr;
  }
// DBGMSG("Thread %u:%08X not found!",ThreadID,hThread);
 return -1;
}
//------------------------------------------------------------------------------------
bool RemoveThreadFromList(DWORD ThreadID, HANDLE hThread=NULL)   // A process can create and delete some thread very frequently so no memory moving here, just invalidate removed entries
{
 int Idx = this->FindThreadIdxInList(NULL, ThreadID, hThread);
 if(Idx < 0)return false;
 this->RemoveThreadFromListByIdx(Idx);
 return true;
}
//------------------------------------------------------------------------------------
void Clear(void)
{
 this->Lock();
 for(UINT ctr=0,total=this->ThreadLst.Count();ctr < total;ctr++)
  {
   SThDesc* ThDes = &this->ThreadLst[ctr];
   if((ThDes->Flags & SThDesc::tfOpenedHnd) && ThDes->hThread)NtClose(ThDes->hThread);
  }
 this->ThreadLst.Clear(); 
 this->EvtStk.Clear();
 this->UnLock();
}
//------------------------------------------------------------------------------------
NTSTATUS Suspend(UINT Idx, PULONG PrevCnt=NULL, bool MarkForRemove=false, bool SuspForAll=false)   // TODO: Report_DEAD_THREAD_DEBUG_EVENT if HANDLE in STATUS_THREAD_IS_TERMINATING
{
 if(!this->IsThreadIndexExist(Idx))return STATUS_NOT_FOUND;
 NTSTATUS Status = STATUS_UNSUCCESSFUL;
 if(MarkForRemove)this->ThreadLst[Idx].Flags |= SThDesc::tfMarkedForRemove;  // Will be removed after next Resume
 if(HANDLE hTh = this->ThreadLst[Idx].hThread)
  {
   DBGMSG("Idx=%u, ThID=%u",Idx,this->ThreadLst[Idx].ThreadID);
   if(IsCurrentThreadDesc(&this->ThreadLst[Idx],NtCurrentTeb()))this->UnLock();   // Unlocks if locked to suspend self
   if(SuspForAll)
    {
     if(this->ThreadLst[Idx].Flags & SThDesc::tfSuspendedForAll){DBGMSG("Already suspended for all - skipping!"); return STATUS_SUCCESS;}
     this->ThreadLst[Idx].Flags |= SThDesc::tfSuspendedForAll;    // Prevent the thread from suspending again by SuspendAllThreads until ResumeAllThreads is called 
    }
   Status = NtSuspendThread(hTh, PrevCnt);
   DBGMSG("Status=%08X",Status);
   if(Status == STATUS_THREAD_IS_TERMINATING)this->ReportDeadThreadDbgEvt(Idx);
  }    
 return Status;
}
//------------------------------------------------------------------------------------
NTSTATUS Resume(UINT Idx, PULONG PrevCnt=NULL, int DbgContinue=-1)  // TODO: Report_DEAD_THREAD_DEBUG_EVENT if HANDLE in STATUS_THREAD_IS_TERMINATING
{
 if(!this->IsThreadIndexExist(Idx))return STATUS_NOT_FOUND;
 ULONG Prev = 0;
 NTSTATUS Status = STATUS_UNSUCCESSFUL;
 if(DbgContinue >= 0)this->ThreadLst[Idx].SetDbgContinueState(DbgContinue);    //   DbgCont = DbgContinue;  // ???
 if(this->ThreadLst[Idx].hThread)Status = NtResumeThread(this->ThreadLst[Idx].hThread, &Prev); 
 if((Status || (Prev <= 1)) && (this->ThreadLst[Idx].Flags & SThDesc::tfMarkedForRemove))this->RemoveThreadFromListByIdx(Idx); 
 if(Status == STATUS_THREAD_IS_TERMINATING)this->ReportDeadThreadDbgEvt(Idx);
 if(PrevCnt)*PrevCnt = Prev;  
 return Status;
}
//------------------------------------------------------------------------------------
NTSTATUS SuspendAllThreads(DWORD SingleThreadID, int SingleThreadIdx=-1,  bool MarkForRemove=false, bool SkipSpecified=false)   // Suspends only registered threads so ClientThread is safe    
{
 ULONG PrevCnt = 0;
 if(!SingleThreadID && (SingleThreadIdx >= 0))
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
   if(SkipSpecified && SingleThreadID && (ThDesc->ThreadID == SingleThreadID)){DBGMSG("Skipping: ThID=%u",ThDesc->ThreadID); continue;}
   if(MarkForRemove && SingleThreadID && (ThDesc->ThreadID == SingleThreadID))ThDesc->Flags |= SThDesc::tfMarkedForRemove;  // Will be removed after next Resume
   if(ThDesc->Flags & SThDesc::tfSuspendedForAll)continue;    // Already suspended for all
   if(IsCurrentThreadDesc(ThDesc,CurrTeb)){CurrThDesc = ThDesc; continue;}         // Current thread is in the list - suspend it last  
   NTSTATUS Status = NtSuspendThread(ThDesc->hThread, &PrevCnt); 
   ThDesc->Flags  |= SThDesc::tfSuspendedForAll; 
   DBGMSG("ThID=%u, Status=%08X, SuspLst=%i",ThDesc->ThreadID, Status, PrevCnt);
   if(Status == STATUS_THREAD_IS_TERMINATING)this->ReportDeadThreadDbgEvt(ctr);
  }
 if(CurrThDesc)
  {
   HANDLE hThread = CurrThDesc->hThread;
   DWORD ThID = CurrThDesc->ThreadID;
   DBGMSG("This Thread: ThID=%u",CurrThDesc->ThreadID);
   this->UnLock();   // Release BEFORE suspending!
   if(hThread)   // This thread
    {
     NTSTATUS Status = NtSuspendThread(hThread, &PrevCnt); 
     DBGMSG("CURR: ThID=%u, Status=%08X, SuspLst=%i",ThID, Status, PrevCnt);
     if(Status == STATUS_THREAD_IS_TERMINATING)
      {
       int TIdx = this->FindThreadIdxInList(NULL,ThID); 
       if(TIdx >= 0)this->ReportDeadThreadDbgEvt(TIdx);
      }
    }    
  }
 return STATUS_SUCCESS;    // Always
}
//------------------------------------------------------------------------------------
NTSTATUS ResumeAllThreads(DWORD SingleThreadID, int SingleThreadIdx=-1, int DbgContinue=-1)  
{
 if(!SingleThreadID && (SingleThreadIdx >= 0))
  {
   SThDesc* ThDesc = GetThreadDesc(SingleThreadIdx);
   if(ThDesc)SingleThreadID = ThDesc->ThreadID;
  }
 TEB* CurrTeb = NtCurrentTeb(); 
 for(UINT ctr=0,total=this->ThreadLst.Count();ctr < total;ctr++)
  {
   SThDesc* ThDesc = &this->ThreadLst[ctr];
   if(!ThDesc->hThread)continue;     // The slot is empty
   if((ThDesc->ThreadID == SingleThreadID) && (DbgContinue >= 0))ThDesc->SetDbgContinueState(DbgContinue);   //     DbgCont = DbgContinue;  // ???
   ULONG   PrevCnt = 0;
   NTSTATUS Status = STATUS_UNSUCCESSFUL;
   if(!IsCurrentThreadDesc(ThDesc,CurrTeb)){Status = NtResumeThread(ThDesc->hThread, &PrevCnt); ThDesc->Flags &= ~SThDesc::tfSuspendedForAll; DBGMSG("ThID=%u, Status=%08X, SuspLst=%i",ThDesc->ThreadID, Status, PrevCnt);}  // Current thread is already running but suspend count > 0?
   if((Status || (PrevCnt <= 1)) && (ThDesc->Flags & SThDesc::tfMarkedForRemove))this->RemoveThreadFromListByIdx(ctr);   // Remove ALL marked threads whey they are resumed   // Or Status is something like "TheThreadIsTerminating"
     else if(Status == STATUS_THREAD_IS_TERMINATING)this->ReportDeadThreadDbgEvt(ctr);
  }
 return STATUS_SUCCESS;
}
//------------------------------------------------------------------------------------
NTSTATUS SetContextVal(UINT Idx, PCONTEXT Ctx)
{
 if(!this->IsThreadIndexExist(Idx))return STATUS_NOT_FOUND;
 if(!this->ThreadLst[Idx].pContext)return NtSetContextThread(this->ThreadLst[Idx].hThread, Ctx);    // Not in exception handler
 PCONTEXT ThCtx = (PCONTEXT)this->ThreadLst[Idx].pContext;       // Exception handler`s Context
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
 DBGMSG("Stored locally: %u",Idx);
 return STATUS_SUCCESS;
}
//------------------------------------------------------------------------------------
NTSTATUS GetContextVal(UINT Idx, PCONTEXT Ctx)
{
 if(!this->IsThreadIndexExist(Idx))return STATUS_NOT_FOUND; 
 if(!this->ThreadLst[Idx].pContext)return NtGetContextThread(this->ThreadLst[Idx].hThread, Ctx);     // Not in exception handler
 PCONTEXT ThCtx = (PCONTEXT)this->ThreadLst[Idx].pContext;      // Exception handler`s Context
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
 DBGMSG("Read locally: %u",Idx);
 return STATUS_SUCCESS;
}
//------------------------------------------------------------------------------------
bool IsSingleStepping(UINT Idx)
{
// if(!this->IsThreadIndexExist(Idx))return false;   // Used only by HandleException anyway
 return this->ThreadLst[Idx].SyncGetFlags(SThDesc::tfTraceFlg); 
}
//------------------------------------------------------------------------------------
bool UpdTraceFlag(UINT Idx, PCONTEXT Ctx)  
{
 if(!(Ctx->ContextFlags & CONTEXT_CONTROL) || !this->IsThreadIndexExist(Idx))return false;
 this->ThreadLst[Idx].SetTraceFlagState(Ctx->EFlags & SThDesc::tfTraceFlagMsk);       // Trap flag mask
 return true;
}
//------------------------------------------------------------------------------------
bool UpdHardwareBp(UINT Idx, PCONTEXT Ctx)  
{
 if(!(Ctx->ContextFlags & CONTEXT_DEBUG_REGISTERS) || !this->IsThreadIndexExist(Idx))return false;
 SThDesc* ThDes = &this->ThreadLst[Idx];
 memset((void*)&ThDes->HwBpLst,0,sizeof(ThDes->HwBpLst));  
 for(UINT ctr=0;ctr < 4;ctr++)       // Nothing special for x64?
  {
   if(!(Ctx->Dr7 & (1ULL << (ctr*2))))continue;       // Skip any not enabled
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
 return true;
}
//------------------------------------------------------------------------------------
bool IsHardwareBpHit(UINT Idx, PVOID Addr)
{
// if(!this->IsThreadIndexExist(Idx))return false;   // Used only by HandleException anyway
 SThDesc* ThDes = &this->ThreadLst[Idx];
 for(UINT ctr=0;ctr < 4;ctr++)       // Nothing special for x64?
  {
   if(!ThDes->HwBpLst[ctr].Addr)continue;
   if(((PBYTE)Addr >= ThDes->HwBpLst[ctr].Addr)&&((PBYTE)Addr < &ThDes->HwBpLst[ctr].Addr[ThDes->HwBpLst[ctr].Size]))return true;   
  }
 return false;
}
//------------------------------------------------------------------------------------
bool ReadDbgContext(UINT Idx, PCONTEXT Ctx)    // Remember DRx changes, made by an application 
{
 if(!this->IsThreadIndexExist(Idx))return false;
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
 return false;
}
//------------------------------------------------------------------------------------
bool WriteDbgContext(UINT Idx, PCONTEXT Ctx)   // Restore DRx state to show it to an application
{
 if(!this->IsThreadIndexExist(Idx))return false;
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
 return false;
}
//------------------------------------------------------------------------------------
int ReportDeadThreadDbgEvt(int ThIndex) // Requires Lock  // TEB is already deallocated. Thread`s handle is in STATUS_THREAD_IS_TERMINATING state  // Somehow a thread may avoid NtTerminateThread and die
{ 
 SThDesc* Desc = this->GetThreadDesc(ThIndex); 
 if(Desc->Flags & SThDesc::tfMarkedForRemove)return 0;    // Already removing
 this->DbgEventCnt++;
 DbgEvtEx* evt = this->PushDbgEvent();
 evt->dwDebugEventCode = EXIT_THREAD_DEBUG_EVENT;
 evt->dwProcessId = NtCurrentProcessId();    // this->CurProcID; 
 evt->dwThreadId  = Desc->ThreadID;       // Do not access its TEB in case it is already deallocated
 evt->PathSize    = 0;
 evt->u.ExitThread.dwExitCode = 0;
 Desc->Flags |= SThDesc::tfMarkedForRemove;
 DBGMSG("PushDbgEvt: ThreadID=%08X(%u), ExitCode=%08X",Desc->ThreadID,Desc->ThreadID,evt->u.ExitThread.dwExitCode);
 return 0; 
}
//------------------------------------------------------------------------------------

};
//====================================================================================
class CSwBpList             // TODO: PAGE breakpoints
{
 SHM::CCritSectEx<> csec;      
 SHM::CGrowArr<PVOID, 32> SwBpLst;

public:
//------------------------------------------------------------------------------------
CSwBpList(void)
{

}
//------------------------------------------------------------------------------------
~CSwBpList()
{

}
//------------------------------------------------------------------------------------
int AddBP(PVOID Addr)
{
 this->csec.Lock(); 
 int BpIdx = -1;
 for(UINT ctr=0,total=this->SwBpLst.Count();ctr < total;ctr++)
  {
   PVOID Val = this->SwBpLst[ctr];
   if(Val == Addr){this->csec.Unlock(); return ctr;}   // Already in list
   if(!Val)BpIdx = ctr;
  }
 if(BpIdx < 0)
  {
   BpIdx = this->SwBpLst.Count();
   this->SwBpLst.Add(NULL);
  }
 this->SwBpLst[BpIdx] = Addr;
 this->csec.Unlock();
 DBGMSG("Adding SwBP: %u",BpIdx);
 return BpIdx;
}
//------------------------------------------------------------------------------------
int DelBP(PVOID Addr)
{
 this->csec.Lock(); 
 int BpIdx = this->GetBpIdxForAddr(Addr);
 if(BpIdx < 0){this->csec.Unlock(); return -1;}
 this->SwBpLst[BpIdx] = NULL;  
 this->csec.Unlock();
 DBGMSG("Removing SwBP: %u",BpIdx);
 return BpIdx;
}
//------------------------------------------------------------------------------------
int IsHitBP(PVOID Addr){return (this->GetBpIdxForAddr(Addr) >= 0);}
int GetBpIdxForAddr(PVOID Addr)
{
 this->csec.Lock(); 
 for(UINT ctr=0,total=this->SwBpLst.Count();ctr < total;ctr++)
  {
   PVOID Cadr = this->SwBpLst[ctr];
   if(Cadr && (Cadr == Addr)){this->csec.Unlock(); return ctr;}
  }
 this->csec.Unlock();
 return -1;
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
 bool    InDbgEvent;
 bool    DbgAttached;
 DWORD   ExcludedThID;
 DWORD   DbgBrkThID;
 DWORD   ClientThID;
 DWORD   MainThID;
 DWORD   CurProcID;    
 PVOID   hClientDll;
 HANDLE  hIPCThread;
 UINT    IPCSize;
 UINT    UrsOpts;
 CSwBpList   BpList;
 CThreadList ThList;

//------------------------------------------------------------------------------------
_declspec(noinline) static void DoDbgBreak(PVOID This){__debugbreak();}       // Makes it harder to track 'int 3'?
static bool IsAddrInDbgBreak(PVOID Addr){return (((PBYTE)Addr >= (PBYTE)&DoDbgBreak)&&((PBYTE)Addr < ((PBYTE)&DoDbgBreak + 64)));} 
static void _fastcall DbgBreakThread(PVOID ThreadParameter) 
{           
 CDbgClient* DbgIPC = (CDbgClient*)ThreadParameter;
 DoDbgBreak(DbgIPC);  // x64Dbg will continue normally from this  // Any DebugBreak functions may be tracked  
 NtTerminateThread(NtCurrentThread, 0);
}
//------------------------------------------------------------------------------------
PTEB FindModulesAndTEBs(TEB* CurTeb, SHM::CGrowArrImpl<PBYTE>* ModArr, SHM::CGrowArrImpl<PTEB>* TebArr, int* MainThreadIndex=NULL)
{
 MEMORY_BASIC_INFORMATION meminfo;
 SIZE_T RetLen = 0;
 SIZE_T MemoryInformationLength = sizeof(MEMORY_BASIC_INFORMATION);      
 PBYTE  BaseAddress = NULL;
 PVOID  LastABase = NULL;
 PTEB   MainThTeb = NULL;
 long   MainThIdx = -1;
 LARGE_INTEGER MinCrtTime;
 LARGE_INTEGER CurrCrtTime;
 DBGMSG("Current: PEB=%p, TEB=%p",CurTeb->ProcessEnvironmentBlock,CurTeb);
 this->ThList.Lock();
 MinCrtTime.QuadPart = -1i64;   
 for(;!NtQueryVirtualMemory(NtCurrentProcess,BaseAddress,MemoryBasicInformation,&meminfo,MemoryInformationLength,&RetLen); BaseAddress += meminfo.RegionSize)
  {
//   DBGMSG("BLK: Addr=%p, Base=%p, ABase=%p, Size=%08X, Type=%08X, State=%08X, Protect=%08X, AProtect=%08X",BaseAddress,meminfo.BaseAddress,meminfo.AllocationBase,meminfo.RegionSize,meminfo.Type,meminfo.State,meminfo.Protect,meminfo.AllocationProtect);
   if((meminfo.Type == MEM_IMAGE) && (meminfo.State == MEM_COMMIT))
    {
     if(LastABase == meminfo.AllocationBase)continue;
     LastABase = meminfo.AllocationBase; 
     if(NPEFMT::IsValidPEHeader(BaseAddress))
      {
       DBGMSG("Found PE image at %p",BaseAddress);
       *ModArr->Add(NULL) = BaseAddress;
      }
    }
   else if((meminfo.Type == MEM_PRIVATE) && (meminfo.State == MEM_COMMIT) && (meminfo.Protect == meminfo.AllocationProtect) && (meminfo.Protect == PAGE_READWRITE) && (meminfo.RegionSize >= 0x1000) && (meminfo.RegionSize < 0x100000))   // WinXP TEB >= 0x2000 ? // Win7 x32 TEB Blk is 0x1000  // On latest Win10 many TEBs in same big block as PEB
    {
     PBYTE DataPtr = (PBYTE)BaseAddress;
     for(UINT Offset=0;Offset < meminfo.RegionSize;Offset+=0x1000)   // Find all TEBs in the block
      {
       TEB* MayBeTEB = (TEB*)&DataPtr[Offset];
       if((MayBeTEB->ProcessEnvironmentBlock == CurTeb->ProcessEnvironmentBlock) && (MayBeTEB->ClientId.UniqueProcess == CurTeb->ClientId.UniqueProcess))       //    && (MayBeTEB->CurrentLocale == CurTeb->CurrentLocale))
        {
         DBGMSG("Found TEB at %p, %u",MayBeTEB, (UINT)MayBeTEB->ClientId.UniqueThread);
         if((MayBeTEB != CurTeb) && ((UINT)MayBeTEB->ClientId.UniqueThread != this->ExcludedThID))  // Assuming that CurTeb belongs to Client Thread
          {
           int ThIdx = this->ThList.AddThreadToList(MayBeTEB, 0, NULL, &CurrCrtTime);
           if(ThIdx >= 0)
            {
             *TebArr->Add(NULL) = MayBeTEB;    // Includes ClientThread              
//             DBGMSG("MainThTeb=%p, CurrCrtTime=%016llX, MinCrtTime=%016llX",MainThTeb, CurrCrtTime.QuadPart, MinCrtTime.QuadPart);
             if((UINT64)CurrCrtTime.QuadPart < (UINT64)MinCrtTime.QuadPart){MainThTeb = MayBeTEB; MainThIdx = ThIdx; MinCrtTime.QuadPart = CurrCrtTime.QuadPart;}    // Not reliable!
            }
          }
           else {DBGMSG("Skipping the excluded thread: %u", (UINT)MayBeTEB->ClientId.UniqueThread);}
        }
      }
    }       
  }
 if(MainThreadIndex)*MainThreadIndex = MainThIdx; 
 this->ThList.UnLock();
 return MainThTeb;
}
//------------------------------------------------------------------------------------
// NOTE: Buffer is locked by this thread in message enumeration
//
int ProcessRequestDbg(SMsgHdr* Req)
{
 SHM::CArgPack<2048> api;                     // '__chkstk' is disabled with '/Gs100000000' // -Gs9999999
 SHM::CArgPack<2048> apo;                     // Had to put it here to save some stack space (Because variables for each scope are allocated at beginning of the function)    

 DBGMSG("ReqAddr=%p, MsgID=%u, MsgSeqID=%u",Req,Req->MsgID, Req->GetBlk()->MsgSeqID);
// static UINT LastMsg = 0;
// if(Req->GetBlk()->MsgSeqID <= LastMsg){ DBGMSG("Trouble!"); }   // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// LastMsg = Req->GetBlk()->MsgSeqID;

 switch(Req->MsgID)
  {   
   case miQueryInformationProcess:
    {
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
       else Status = NtQueryInformationProcess(NtCurrentProcess,ProcessInformationClass,apo.PushBlk(ProcessInformationLength),ProcessInformationLength,&RetLen);   // ULONG or SIZE_T ?
     apo.PushArg(RetLen);
     apo.PushArg(Status);
     DBGMSG("miQueryInformationProcess PutMsg: InfoClass=%u, Status=%08X, Size=%u",ProcessInformationClass,Status,apo.GetLen());
     this->PutMsg(mtDbgRsp, miQueryInformationProcess, Req->Sequence, apo.GetPtr(), apo.GetLen());      
    }
   break;
   case miQueryInformationThread:
    {
     THREADINFOCLASS ThreadInformationClass; 
     ULONG ThreadInformationLength; 
     ULONG RetLen = 0;        
     HRESULT Status = STATUS_UNSUCCESSFUL;

     api.Assign((PBYTE)&Req->Data,Req->DataSize);
     this->ThList.Lock();
     UINT   ThIdx        = FakeHandleToUint(api.PopArg<HANDLE>());
     HANDLE ThreadHandle = this->ThList.GetHandleByIndex(ThIdx);
     this->ThList.UnLock();    // NOTE: ThreadHandle may become invalid
     api.PopArg(ThreadInformationClass);
     api.PopArg(ThreadInformationLength); 
     if(ThreadHandle)
      {
       Status = NtQueryInformationThread(ThreadHandle,ThreadInformationClass,apo.PushBlk(ThreadInformationLength),ThreadInformationLength,&RetLen);    // ULONG or SIZE_T ?
       if(Status == STATUS_THREAD_IS_TERMINATING)this->ThList.ReportDeadThreadDbgEvt(ThIdx);
      }
       else {DBGMSG("Thread handle not found: ThIdx=%i",ThIdx); apo.PushBlk(ThreadInformationLength);}
     apo.PushArg(RetLen);
     apo.PushArg(Status);
     DBGMSG("miQueryInformationThread PutMsg: InfoClass=%u, Status=%08X, Size=%u",ThreadInformationClass,Status,apo.GetLen());
     this->PutMsg(mtDbgRsp, miQueryInformationThread, Req->Sequence, apo.GetPtr(), apo.GetLen());      
    }
   break;     
   case miQueryVirtualMemory:     // NOTE: x64Dbg will call this for every memory block and avery request/response will move forward in the buffer (In a small buffer some old messages(Thread reports) will be lost)
    {
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
       else Status = NtQueryVirtualMemory(NtCurrentProcess,BaseAddress,MemoryInformationClass,apo.PushBlk(MemoryInformationLength),MemoryInformationLength,&RetLen);
     apo.PushArg(RetLen);
     apo.PushArg(Status);
//     DBGMSG("miQueryVirtualMemory PutMsg: Status=%08X, InfoClass=%u, BaseAddress=%p, Size=%u",Status,MemoryInformationClass,BaseAddress,apo.GetLen()); 
     this->PutMsg(mtDbgRsp, miQueryVirtualMemory, Req->Sequence, apo.GetPtr(), apo.GetLen());      
    }
   break;
   case miDebugActiveProcess:
    {
     BOOL  Reslt = TRUE;
     DWORD dwProcessId = 0;
     api.Assign((PBYTE)&Req->Data,Req->DataSize);
     api.PopArg(dwProcessId);
     if(dwProcessId != this->CurProcID){DBGMSG("Process ID mismatch: %08X:%08X",dwProcessId,this->CurProcID); return -1;}  // Allow to fail here?
     this->DbgAttached = true;

     SHM::CGrowArr<PTEB,  128> TebArr;    // NOTE: Stack arrays reserved on stack forever
     SHM::CGrowArr<PBYTE, 128> ModArr;    // 2048
     int MainThIdx   = -1;
     PTEB CurTeb     = NtCurrentTeb();     // ThreadID is DbgIPC->ClientThID and should not be reported to a debugger
     PTEB MainThTeb  = this->FindModulesAndTEBs(CurTeb, &ModArr, &TebArr, &MainThIdx);
     DBGMSG("MainThTeb: %p",MainThTeb);    // May happen to be NULL :)
     this->MainThID  = (UINT)MainThTeb->ClientId.UniqueThread;  // MainThTeb should be present!!!   // Report this only in CREATE_PROCESS_DEBUG_EVENT    // Not guranteed to return a real main(first) thread
     HANDLE hPHandle = UintToFakeHandle(this->CurProcID);
     apo.PushArg(hPHandle);
     HANDLE hTHandle = UintToFakeHandle(MainThIdx);
     apo.PushArg(hTHandle);
     apo.PushArg(MainThTeb);
     apo.PushArg(this->MainThID);
     apo.PushArg(Reslt);
     DBGMSG("miDebugActiveProcess PutMsg: hPHandle=%08X, hTHandle=%08X, MainThId=%u, Size=%u",hPHandle,hTHandle,this->MainThID,apo.GetLen());
     this->PutMsg(mtDbgRsp, miDebugActiveProcess, Req->Sequence, apo.GetPtr(), apo.GetLen());  

     DBGMSG("Modules=%u, Threads=%u, MainThreadID=%u, ClientThreadID=%u",ModArr.Count(),TebArr.Count(),this->MainThID,this->ClientThID);
     for(int Ctr=TebArr.Count()-1;Ctr >= 0;Ctr--)   // Originally all threads suspended by DebugActiveProcess before any of debug events reported but we do not have any handles yet
      {
       TEB* pTeb = TebArr[Ctr];
       UINT ThID = (UINT)pTeb->ClientId.UniqueThread;
       if((ThID == this->ClientThID)||(ThID == this->MainThID)||(ThID == this->ExcludedThID))continue;   // Do not report Client, Caller thread and Main thread(again)   // Man thread must be reported only in CREATE_PROCESS_DEBUG_EVENT
       this->Report_CREATE_THREAD_DEBUG_EVENT(pTeb, false, true);   // Open a thread`s handle, report it and suspend
      }
     for(int Ctr=ModArr.Count()-1;Ctr >= 0;Ctr--)  // Must be AFTER reporting and supending of all threads
      {
       PVOID hModule = ModArr[Ctr];
       if((hModule == this->hClientDll)||(hModule == CurTeb->ProcessEnvironmentBlock->ImageBaseAddress))continue;                  // Do not report Client DLL and main module
       this->Report_LOAD_DLL_DEBUG_INFO(MainThTeb, hModule, true);     // No suspending here (Only a main thread is registered anyway)
      }
     this->Report_CREATE_PROCESS_DEBUG_EVENT(MainThTeb);   // Report process creation    // Suspend Main Thread
     this->DispatchDebugEvents(true);    // Dispatch initial events backwards
    }
   break;
   case miDebugActiveProcessStop:
    {
     DBGMSG("miDebugActiveProcessStop");
     this->DbgAttached = false;
    }
   break;
   case miContinueDebugEvent:
    {
     BOOL  Reslt = TRUE;
     DWORD dwProcessId = 0;
     DWORD dwThreadId; 
     DWORD dwContinueStatus;

     api.Assign((PBYTE)&Req->Data,Req->DataSize);
     api.PopArg(dwProcessId);
     api.PopArg(dwThreadId);
     api.PopArg(dwContinueStatus);
     if(dwProcessId != this->CurProcID){DBGMSG("Process ID mismatch: %08X:%08X",dwProcessId,this->CurProcID);}
     int res = -1;
     int SingleThreadIdx = -1;
     bool DbgContinue = (dwContinueStatus == DBG_CONTINUE);
     apo.PushArg(Reslt);
     DBGMSG("miContinueDebugEvent PutMsg: ThId=%u, Size=%u",dwThreadId,apo.GetLen());
     this->PutMsg(mtDbgRsp, miContinueDebugEvent, Req->Sequence, apo.GetPtr(), apo.GetLen());   // Report before resuming any threads because it may be a thread at NtTerminateProcess
     this->ThList.Lock();
     this->ThList.DbgEventCnt--;
     DBGMSG("EventsAfter=%u, InStack=%u",this->ThList.DbgEventCnt,this->ThList.EventsInStack()); 
     if(!this->EvtSuspAllTh)     // Resumes all threads or at least the event`s thread
      {
       if(SingleThreadIdx < 0)SingleThreadIdx = this->ThList.FindThreadIdxInList(nullptr, dwThreadId);
       if(SingleThreadIdx >= 0)res = this->ThList.Resume(SingleThreadIdx, NULL, DbgContinue);
         else {DBGMSG("miContinueDebugEvent: Thread ID not found: ThId=%u",dwThreadId);}  
      }
       else res = (this->ThList.DbgEventCnt > 0)?0:(int)this->ThList.ResumeAllThreads(dwThreadId, SingleThreadIdx, DbgContinue); 
     this->ThList.UnLock();          // What if this thread will be terminatred by NtTerminateProcess before UnLock?
     if(res < 0){DBGMSG("miContinueDebugEvent: DoResumeThread(s) failed: ThId=%u, Err=%i",dwThreadId,res);}  
     this->DispatchDebugEvents(true);    // Dispatch any queued debug events
    }
   break;

/*   case miWaitForDebugEvent:     // This is a response only          
    {

    }
   break; */

   case miDebugBreakProcess:  // Unsafe and unstealthy   // Some debuggers will not read any information about a debuggee unless it breaks somwhere (i.e. WinDbg)
    {                      // All of process`s threads may be in some waiting state and because of that a new thread required
     BOOL   Reslt = FALSE;
     UINT   uExitCode;
     HANDLE hProcess;
     api.Assign((PBYTE)&Req->Data,Req->DataSize);
     api.PopArg(hProcess);
     api.PopArg(uExitCode);
     DBGMSG("Doing miDebugBreakProcess");         // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<
     if(hProcess != UintToFakeHandle(this->CurProcID)){DBGMSG("Process HANDLE mismatch: %08X:%08X",hProcess,UintToFakeHandle(this->CurProcID));}
     HANDLE ThreadHandle = NULL; 
     PVOID  StackBase = NULL; 
     SIZE_T StackSize = 0x10000;
     NtAllocateVirtualMemory(NtCurrentProcess, &StackBase, 0, &StackSize, MEM_COMMIT, PAGE_READWRITE);
     NTSTATUS stat = NNTDLL::NativeCreateThread(&CDbgClient::DbgBreakThread, this, this, NtCurrentProcess, TRUE, &StackBase, &StackSize, &ThreadHandle, NULL);  // HANDLE ThreadHandle = CreateThread(NULL,0,&CDbgClient::DbgBreakThread,this,CREATE_SUSPENDED,&this->DbgBrkThID);  // Just to do int3
     DBGMSG("DebugBreakThread: Addr=%p, StackBase=%p, StackSize=%08X",&CDbgClient::DbgBreakThread,StackBase,StackSize);
	 if(ThreadHandle)
      {
       Reslt = TRUE;
#ifndef _DEBUG
       NtSetInformationThread(ThreadHandle, ThreadHideFromDebugger, NULL, NULL);
#endif
       NtResumeThread(ThreadHandle, NULL);     // Should wait for it?   // Do not report this thread to debugger? Originally it is reported(DebugBreakProcess)
       NtClose(ThreadHandle);
      }
       else {DBGMSG("Create DebugBreak thread failed with %08X",stat);}
     apo.PushArg(Reslt);
     DBGMSG("miDebugBreakProcess PutMsg: Size=%u",apo.GetLen());
     this->PutMsg(mtDbgRsp, miDebugBreakProcess, Req->Sequence, apo.GetPtr(), apo.GetLen()); 
    }
   break;
   case miGetThreadContext:
    {                        
     CONTEXT ctx;
     api.Assign((PBYTE)&Req->Data,Req->DataSize);
     this->ThList.Lock();
     UINT ThIdx = FakeHandleToUint(api.PopArg<HANDLE>());
     api.PopArg(ctx);
     NTSTATUS Status = STATUS_UNSUCCESSFUL;
     if(this->ThList.IsThreadIndexExist(ThIdx))
      {
       Status = this->ThList.GetContextVal(ThIdx, &ctx);
       if(!Status)          // Need update here?
        {
         DBGMSG("GetContext %u: DbgRegs=%u, Dr7=%p, Dr0=%p, Dr1=%p, Dr2=%p, Dr3=%p",ThIdx,bool(ctx.ContextFlags & CONTEXT_DEBUG_REGISTERS),ctx.Dr7,ctx.Dr0,ctx.Dr1,ctx.Dr2,ctx.Dr3);
//         if(this->OnlyOwnHwBP)this->ThList.UpdHardwareBp(ThIdx, &ctx);   // Why update it here?????????????????????
//         if(this->OnlyOwnTF)this->ThList.UpdTraceFlag(ThIdx, &ctx);
        }  
         else if(Status == STATUS_THREAD_IS_TERMINATING)this->ThList.ReportDeadThreadDbgEvt(ThIdx);
      }
       else {DBGMSG("Thread not found: ThIdx=%i",ThIdx);}
     this->ThList.UnLock();
     apo.PushArg(ctx);
     apo.PushArg(Status);
     DBGMSG("miGetThreadContext PutMsg: Status=%08X, PC=%p, Trace=%u, Size=%u", Status, GetInstrPtr(&ctx), bool(ctx.EFlags&0x0100), apo.GetLen());
     this->PutMsg(mtDbgRsp, miGetThreadContext, Req->Sequence, apo.GetPtr(), apo.GetLen()); 
    }
   break;
   case miSetThreadContext:
    {
     CONTEXT ctx;
     api.Assign((PBYTE)&Req->Data,Req->DataSize);
     this->ThList.Lock();
     UINT ThIdx = FakeHandleToUint(api.PopArg<HANDLE>());   
     api.PopArg(ctx);
     NTSTATUS Status = STATUS_UNSUCCESSFUL;
     if(this->ThList.IsThreadIndexExist(ThIdx))
      {
       Status = this->ThList.SetContextVal(ThIdx, &ctx); 
       if(!Status)
        {
         DBGMSG("SetContext %u: DbgRegs=%u, Dr7=%p, Dr0=%p, Dr1=%p, Dr2=%p, Dr3=%p",ThIdx,bool(ctx.ContextFlags & CONTEXT_DEBUG_REGISTERS),ctx.Dr7,ctx.Dr0,ctx.Dr1,ctx.Dr2,ctx.Dr3);
         if(this->OnlyOwnHwBP)this->ThList.UpdHardwareBp(ThIdx, &ctx);         // A debugger sets this BPs and TF, remember them
         if(this->OnlyOwnTF)this->ThList.UpdTraceFlag(ThIdx, &ctx);
        } 
         else if(Status == STATUS_THREAD_IS_TERMINATING)this->ThList.ReportDeadThreadDbgEvt(ThIdx);
      }
       else {DBGMSG("Thread not found: ThIdx=%i",ThIdx);} 
     this->ThList.UnLock();
     apo.PushArg(Status);
     DBGMSG("miSetThreadContext PutMsg: Status=%08X, PC=%p, Trace=%u, Size=%u", Status, GetInstrPtr(&ctx), bool(ctx.EFlags&0x0100), apo.GetLen());    // TF = 0x0100
     this->PutMsg(mtDbgRsp, miSetThreadContext, Req->Sequence, apo.GetPtr(), apo.GetLen());     
    }
   break;
   case miReadVirtualMemory:
    {
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
       else Status = NtReadVirtualMemory(NtCurrentProcess, BaseAddress, apo.PushBlk(BufferLength), BufferLength, &RetLen); 
     apo.PushArg(RetLen);
     apo.PushArg(Status);
//     DBGMSG("miReadVirtualMemory PutMsg: Status=%08X, BaseAddress=%p, BufferLength=%08X, Size=%u",Status,BaseAddress,BufferLength,apo.GetLen());     
     this->PutMsg(mtDbgRsp, miReadVirtualMemory, Req->Sequence, apo.GetPtr(), apo.GetLen());      
    }
   break;
   case miWriteVirtualMemory:
    {
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
           Status = NtWriteVirtualMemory(NtCurrentProcess, BaseAddress, BufPtr, BufferLength, &RetLen);
           if(Status && IsBP)this->BpList.DelBP(BaseAddress);  // Remove a failed BP  // Case when removing a BP is failed are left undefined
          }
           else Status = NtWriteVirtualMemory(NtCurrentProcess, BaseAddress, BufPtr, BufferLength, &RetLen);
        }
     apo.PushArg(RetLen);
     apo.PushArg(Status);
     DBGMSG("miWriteVirtualMemory PutMsg: Status=%08X, Size=%u",Status,apo.GetLen());
     this->PutMsg(mtDbgRsp, miWriteVirtualMemory, Req->Sequence, apo.GetPtr(), apo.GetLen());     
    }
   break;
   case miProtectVirtualMemory:       
    {
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
       else Status = NtProtectVirtualMemory(NtCurrentProcess, &BaseAddress, &RegionSize, NewProtect, &OldProtect);
     apo.PushArg(OldProtect);
     apo.PushArg(BaseAddress);
     apo.PushArg(RegionSize);
     apo.PushArg(Status);
     DBGMSG("miProtectVirtualMemory PutMsg: BaseAddress=%p, Status=%08X, Size=%u",BaseAddress,Status,apo.GetLen());
     this->PutMsg(mtDbgRsp, miProtectVirtualMemory, Req->Sequence, apo.GetPtr(), apo.GetLen());      
    }
   break;
   case miFlushVirtualMemory:       
    {
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
       else Status = NtFlushVirtualMemory(NtCurrentProcess, &BaseAddress, &RegionSize, &IoStatus);
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
     NTSTATUS ExitStatus;
     HRESULT Status = STATUS_UNSUCCESSFUL;
     api.Assign((PBYTE)&Req->Data,Req->DataSize);
     this->ThList.Lock();
     UINT    ThIdx = FakeHandleToUint(api.PopArg<HANDLE>());
     HANDLE ThreadHandle = this->ThList.GetHandleByIndex(ThIdx);
     apo.PopArg(ExitStatus);
     if(ThreadHandle)
      {
       if(this->AllowThTerm)
        {
         Status = NtTerminateThread(ThreadHandle, ExitStatus);
         this->Report_EXIT_THREAD_DEBUG_EVENT(this->ThList.GetTebByIndex(ThIdx), ExitStatus, true);    // Our copy of the syscall and no hook will be called for it!
        }
         else {Status = STATUS_SUCCESS; this->ThList.UnLock();}
      }
       else {DBGMSG("Thread handle not found: ThIdx=%i",ThIdx); this->ThList.UnLock();}    // Do not allow termination before, a target thread may acquire the lock
     apo.PushArg(Status);
     DBGMSG("miTerminateThread PutMsg: Status=%08X, Size=%u",Status,apo.GetLen());
     this->PutMsg(mtDbgRsp, miTerminateThread, Req->Sequence, apo.GetPtr(), apo.GetLen());     
    }
   break;
   case miTerminateProcess:
    {
     HRESULT Status = STATUS_SUCCESS;
     NTSTATUS ExitStatus;
     HANDLE hProcess;
     api.Assign((PBYTE)&Req->Data,Req->DataSize);
     api.PopArg(hProcess);
     api.PopArg(ExitStatus);
     if(hProcess != UintToFakeHandle(this->CurProcID)){DBGMSG("Process HANDLE mismatch: %08X:%08X",hProcess,UintToFakeHandle(this->CurProcID)); Status = STATUS_UNSUCCESSFUL;}
     apo.PushArg(Status);
     DBGMSG("miTerminateProcess PutMsg: Size=%u",apo.GetLen());
     this->PutMsg(mtDbgRsp, miTerminateProcess, Req->Sequence, apo.GetPtr(), apo.GetLen()); 
     if(this->AllowPrTerm && (hProcess == UintToFakeHandle(this->CurProcID)))
      {
       this->EndMsg();
       DBGMSG("Terminating self with code %08X",ExitStatus);
       NtTerminateProcess(NtCurrentProcess, ExitStatus); 
      }
    }
   break;
   case miSuspendThread:
    {
     ULONG  PrevCnt = 0;
     api.Assign((PBYTE)&Req->Data,Req->DataSize);
     this->ThList.Lock();
     UINT    ThIdx  = FakeHandleToUint(api.PopArg<HANDLE>());
     HRESULT Status = this->ThList.Suspend(ThIdx, &PrevCnt);   // NOTE: Suspending a thread which is in a middle of logging(DBGMSG) will cause a deadlock because of logger`s critical section!!!
     this->ThList.UnLock();
     apo.PushArg(PrevCnt);
     apo.PushArg(Status);
     DBGMSG("miSuspendThread PutMsg: Status=%08X, Index=%u, Size=%u",Status,ThIdx,apo.GetLen());
     this->PutMsg(mtDbgRsp, miSuspendThread, Req->Sequence, apo.GetPtr(), apo.GetLen());     
    }
   break;
   case miResumeThread:
    {
     ULONG  PrevCnt = 0;
     api.Assign((PBYTE)&Req->Data,Req->DataSize);
     this->ThList.Lock();
     UINT    ThIdx  = FakeHandleToUint(api.PopArg<HANDLE>());
     HRESULT Status = this->ThList.Resume(ThIdx, &PrevCnt);
     this->ThList.UnLock();
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
int ProcessRequestUsr(SMsgHdr* Req)
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
     apo.PushArgEx(this->SwBpVal,      ctENCSA("Value of SwBP"),             MakeCfgItemID(++OptCtr,dtBYTE));    
     apo.PushArgEx(this->OnlyOwnTF,    ctENCSA("Report Only Own TF"),        MakeCfgItemID(++OptCtr,dtBool));   
     apo.PushArgEx(this->OnlyOwnHwBP,  ctENCSA("Report Only Own HwBP"),      MakeCfgItemID(++OptCtr,dtBool));   
     apo.PushArgEx(this->OnlyOwnSwBP,  ctENCSA("Report Only Own SwBP"),      MakeCfgItemID(++OptCtr,dtBool));
     apo.PushArgEx(this->HideDbgState, ctENCSA("Hide Debugger State"),       MakeCfgItemID(++OptCtr,dtBool));
     apo.PushArgEx(this->AllowThTerm,  ctENCSA("Allow Thread Termination"),  MakeCfgItemID(++OptCtr,dtBool));  
     apo.PushArgEx(this->AllowPrTerm,  ctENCSA("Allow Process Termination"), MakeCfgItemID(++OptCtr,dtBool));    

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
// Dispath a event on top of event stack
int DispatchDebugEvents(bool EvtExit=false)
{
 if(EvtExit)this->InDbgEvent = false;
 if(this->InDbgEvent)return 0;    //{DBGMSG("Still inside an event - skipping!"); return 0;}     // Uncomment to test sequential event reporting // NOTE:Unsequential reporting is broken!
 if(this->ThList.EventsInStack() <= 0)return 0;  // No events to dispatch
 BYTE Buffer[sizeof(DbgEvtEx) + (MAX_PATH * sizeof(wchar_t)) + 8];    // Max buffer for a DebugEvent structure
 this->ThList.Lock();
 this->InDbgEvent = true;     // Report debug events one by one
 DbgEvtEx* evt = this->ThList.PopDbgEvent();
 UINT MemSize  = sizeof(DbgEvtEx) + (evt->PathSize * sizeof(wchar_t));   // NOTE: sizeof(DbgEvtEx) must be same in a debugger plugin
 if(MemSize > sizeof(Buffer))MemSize = sizeof(Buffer);
 memcpy(&Buffer, evt, MemSize);        // Copy the message to minimize lock time
 this->ThList.UnLock();
 DBGMSG("PutMsg: EvtAddr=%p, DebugEventCode=%08X, ThreadId=%u, Size=%08X",evt,evt->dwDebugEventCode,evt->dwThreadId,MemSize);
 return this->PutMsg(mtDbgRsp, miWaitForDebugEvent, 0, &Buffer, MemSize);  
}
//------------------------------------------------------------------------------------
DbgEvtEx* BegDbgEvent(UINT ThreadID, UINT ExtraSize=0)    // Requires ThList.Lock()
{
 DBGMSG("EventsBefore=%u, InStack=%u",this->ThList.DbgEventCnt,this->ThList.EventsInStack()); 
 if(this->EvtSuspAllTh)this->ThList.SuspendAllThreads(NtCurrentThreadId(),-1,false,true);   // Suspend all other threads if this is a first event in stack
 this->ThList.DbgEventCnt++;
 return this->ThList.PushDbgEvent(ExtraSize);
}
//------------------------------------------------------------------------------------
void EndDbgEvent(DbgEvtEx* Evt, int ThIdx)
{
 this->ipc.NotifyChange();  // Fake change notification to break main thread waiting for IPC events    // Is this an optimal solution?   // Is it even works?  // What time it takes?
 DBGMSG("Leave");
}
//------------------------------------------------------------------------------------
bool IsExistDbgEvent(UINT DbgEvtCode, DWORD ThreadID, PVOID BaseAddr)
{
 UINT Offset = -1;
 for(;;)
  {
   DbgEvtEx* Evt = this->ThList.GetDbgEventAt(Offset);
   if(!Evt)break;
   if(Evt->dwDebugEventCode != DbgEvtCode)continue;
   if((LOAD_DLL_DEBUG_EVENT == DbgEvtCode) && (Evt->u.LoadDll.lpBaseOfDll == BaseAddr))return true;
   if((CREATE_THREAD_DEBUG_EVENT == DbgEvtCode) && (Evt->u.CreateThread.lpThreadLocalBase == BaseAddr) && (Evt->dwThreadId == ThreadID))return true;                  
  }
 return false;
}
//------------------------------------------------------------------------------------

public:
 int (_fastcall* UsrReqCallback)(SMsgHdr* Req, PVOID ArgA, UINT ArgB);
 bool   AllowPrTerm;   // Allow termination of target process
 bool   AllowThTerm;   // Allow termination of a target process` thread
 bool   HideDbgState;  // Hide debugger state (DRx)
 bool   EvtSuspAllTh;  // Suspend all threads on Debug Events  // Other threads may measure execution time and fail if suspended (Protectors) // Defauld debug behavoiur is to suspend all threads for eash debug event
 bool   OnlyOwnSwBP;   // Ignore unknown software breakpoints
 bool   OnlyOwnHwBP;   // Ignore unknown hardware breakpoints
 bool   OnlyOwnTF;     // Ignore unexpected Trace break
 BYTE   SwBpVal;       // Software breakpoint value

static inline UINT MakeCfgItemID(UINT CfgIdx, UINT CfgType){return (CfgIdx << 8)|CfgType;}
static inline UINT ReadCfgItemID(UINT Value, UINT* CfgIdx){if(CfgIdx)*CfgIdx = Value >> 8; return Value & 0xFF;}
static HANDLE UintToFakeHandle(UINT Idx){return HANDLE(SIZE_T(~Idx << 5));}
static UINT   FakeHandleToUint(HANDLE hTh){return (~(UINT)hTh >> 5);}
static bool   IsFakeHandle(HANDLE hTh){return !((UINT)hTh & 0x1F) && ((UINT)hTh & 0xF0000000);}      // Pointer Type?
bool IsDbgThreadID(DWORD ThID){return (ThID == this->ClientThID);}
bool IsActive(void){return ((bool)this->hIPCThread && !this->BreakWrk);}
bool IsDbgAttached(void){return this->DbgAttached;}
//------------------------------------------------------------------------------------
static inline PVOID GetInstrPtr(CONTEXT* ctx)
{
#ifdef _AMD64_
 return (PVOID)ctx->Rip;
#else
 return (PVOID)ctx->Eip;
#endif
}
//------------------------------------------------------------------------------------
// Different threads of a debugger will direct their requests here
//
static void _stdcall IPCQueueThread(LPVOID lpThreadParameter)
{    
 DBGMSG("Enter: ThreadId=%u, This=%p", NtCurrentThreadId(), lpThreadParameter);
 CDbgClient* DbgIPC = (CDbgClient*)lpThreadParameter;
 DbgIPC->ClientThID = NtCurrentThreadId();
 DbgIPC->Connect(NtCurrentProcessId(),DbgIPC->IPCSize);       // Create SharedBuffer name for current process, a debugger will connect to it
 DbgIPC->BreakWrk = false;
 while(!DbgIPC->BreakWrk) 
  {    
   if(SMsgHdr* Cmd = DbgIPC->GetMsg())   // Timeout and still no messages
    {
//   DBGMSG("MsgType=%04X, MsgID=%04X, DataID=%08X, Sequence=%08X, DataSize=%08X",Cmd->MsgType,Cmd->MsgID,Cmd->DataID,Cmd->Sequence,Cmd->DataSize);   // These Debug messages make it hang!!!
     if(Cmd->MsgType & mtDbgReq)DbgIPC->ProcessRequestDbg(Cmd);
     if(Cmd->MsgType & mtUsrReq)DbgIPC->ProcessRequestUsr(Cmd); 
    }
   DbgIPC->DispatchDebugEvents();   // Must be afrer responses to any requests     // Putting DispatchDebugEvents here overloads IPC when there is too much modules/threads
  }
 DbgIPC->EndMsg();   // Unlock shared buffer if it is still locked
 DbgIPC->ipc.Clear();
 DbgIPC->Disconnect();
 DBGMSG("Exit");
 NtTerminateThread(NtCurrentThread, 0);
}
//------------------------------------------------------------------------------------
static NTSTATUS CreateIpcThread(PHANDLE pThHndl, PVOID Param, BOOL Suspended)
{
 PVOID  StackBase = NULL; 
 SIZE_T StackSize = 0x10000;    // Should be enough
 NtAllocateVirtualMemory(NtCurrentProcess, &StackBase, 0, &StackSize, MEM_COMMIT, PAGE_READWRITE);
 DBGMSG("Param=%p",Param);                                                                     
 return NNTDLL::NativeCreateThread(&CDbgClient::IPCQueueThread, Param, Param, NtCurrentProcess, Suspended, &StackBase, &StackSize, pThHndl, NULL);   
}
//------------------------------------------------------------------------------------
CDbgClient(PVOID pCallerMod=NULL)
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
 this->ExcludedThID   = 0;
 this->DbgBrkThID     = 0;
 this->ClientThID  = 0;
 this->UrsOpts     = 0;
 this->IPCSize     = 0x100000;  // Smaller is faster   // More DLLs in a target application, more buffer size it will require on attach    // (1024*1024)*4;
 this->DbgAttached = false;
 this->InDbgEvent  = true;      // Until an Attach event
 this->BreakWrk    = true;
 this->hClientDll  = pCallerMod;
 this->CurProcID   = NtCurrentProcessId(); 
 this->MainThID    = 0;
}
//------------------------------------------------------------------------------------
~CDbgClient()
{
 this->Stop();
}
//------------------------------------------------------------------------------------
bool Start(UINT Size=0, HANDLE hThread=NULL, PVOID IPCThProc=NULL, UINT MainThreadId=0, UINT ExcludeThId=0)
{ 
 if(this->IsActive()){DBGMSG("Already active!"); return false;}
 this->ExcludedThID = ExcludeThId;
 this->MainThID     = MainThreadId;
 this->InDbgEvent   = true; 
 this->DbgAttached  = false;
 if(Size)this->IPCSize = Size;
 if(!hThread)
  {
   DBGMSG("IPC thread proc: %p",&CDbgClient::IPCQueueThread);
   NTSTATUS stat = CreateIpcThread(&this->hIPCThread, this, TRUE);
   if(this->hIPCThread)
    {
#ifndef _DEBUG
     NtSetInformationThread(this->hIPCThread, ThreadHideFromDebugger, NULL, NULL);
#endif
     NtResumeThread(this->hIPCThread, NULL); 
    }
     else {DBGMSG("Failed to create IPC thread: %08X", stat);}
  }
   else  
    {     
     this->hIPCThread = hThread;
     PVOID Param = this;
     if(!IPCThProc)IPCThProc = &CDbgClient::IPCQueueThread;
     if(hThread != NtCurrentThread)
      { 
       DBGMSG("Reusing an existing thread: %p",hThread);     
       if(!NNTDLL::ChangeNewSuspThProcAddr(this->hIPCThread, IPCThProc, &Param, true))return false;  
#ifndef _DEBUG
       NtSetInformationThread(this->hIPCThread, ThreadHideFromDebugger, NULL, NULL);
#endif
       NtResumeThread(this->hIPCThread, NULL);
      }
       else 
        {
         DBGMSG("Reusing the current thread");  
         CDbgClient::IPCQueueThread(Param);
        }
    }
 DBGMSG("Continuing");
 return (bool)this->hIPCThread;
}
//------------------------------------------------------------------------------------
bool Stop(void)
{
 if(!this->IsActive()){DBGMSG("Not active!"); return false;}
 this->BreakWrk = true;
 this->DbgAttached = false;
 if(this->ClientThID != NtCurrentThreadId())
  {
   DBGMSG("Waiting for ClientThread termination");
   DWORD res = SHM::WaitForSingle(this->hIPCThread, 1000);    // Not INFINITE, it will stuck on ExitProcess!
   if(WAIT_OBJECT_0 != res)
    {
     this->ThList.Lock();
     NtTerminateThread(this->hIPCThread,0);
     this->ThList.UnLock();
    }
  }
 NtClose(this->hIPCThread);
 this->ThList.ResumeAllThreads(NtCurrentThreadId(), -1, 1);   // In case not all debug events processed
 this->EndMsg();   // Unlock shared buffer if it is still locked
 this->ipc.Clear();
 this->Disconnect();
 this->hIPCThread = NULL;
 this->ThList.Clear();
 DBGMSG("Done");
 return true;
}
//------------------------------------------------------------------------------------
bool HandleException(DWORD ThreadID, PEXCEPTION_RECORD ExceptionRecord, PCONTEXT Context)     // TODO: Detect and ignore BP/TF exceptions which are not caused by debugger   // TODO: EXCEPTION_ILLEGAL_INSTRUCTION breakpoints
{  
 if(!ExceptionRecord || !Context || this->IsDbgThreadID(ThreadID))return false;  // Ignore exceptions from DbgThread. It will stuck if a debugger gets this
 DBGMSG("Code=%08X, Addr=%p, FCtx=%08X, ThID=%u",ExceptionRecord->ExceptionCode, ExceptionRecord->ExceptionAddress, Context->ContextFlags, ThreadID);     // Logging can deadlock if this thead is suspended while holding a logging Critical Section
 int TIdx = -1;
 bool Res = false;
 CThreadList::SThDesc* Desc = NULL;
 switch(ExceptionRecord->ExceptionCode)   // TODO: Breakpoint by a different events
  {
   case EXCEPTION_BREAKPOINT:    // INT3  // IP points to INT3, not after it(Like with DebugAPI), so IP must be incremented
    if(this->OnlyOwnSwBP && !CDbgClient::IsAddrInDbgBreak(ExceptionRecord->ExceptionAddress) && !this->BpList.IsHitBP(ExceptionRecord->ExceptionAddress)){DBGMSG("SkipSwBP: Addr=%p",ExceptionRecord->ExceptionAddress); return false;}     // This INT3 breakpoint was not set by our debugger
#ifdef _AMD64_
     Context->Rip++;
#else
     Context->Eip++;
#endif
   case EXCEPTION_SINGLE_STEP:   // TF/DRx       
    this->ThList.Lock();
    TIdx = this->ThList.FindThreadIdxInList(&Desc, ThreadID);  
    if(TIdx < 0)
     {
      DBGMSG("Failed to find thread %u!", ThreadID);
      TIdx = this->Report_CREATE_THREAD_DEBUG_EVENT(NtCurrentTeb(), true);  // This thread assumed  // Reuses the lock and unlocks
      if(TIdx < 0){DBGMSG("Failed to add the thread %u", ThreadID); break;}
      this->ThList.Lock();
     }
    if(EXCEPTION_SINGLE_STEP == ExceptionRecord->ExceptionCode)    
     { 
      bool TFFail = (this->OnlyOwnTF   && !this->ThList.IsSingleStepping(TIdx));
      bool HwFail = (this->OnlyOwnHwBP && !this->ThList.IsHardwareBpHit(TIdx, ExceptionRecord->ExceptionAddress));
      if(HwFail && TFFail){this->ThList.UnLock(); DBGMSG("SkipHwBP: Addr=%p, TFFail=%u, HwFail=%u",ExceptionRecord->ExceptionAddress,TFFail,HwFail); return false;}     // Not an own TF and not an own HwBp
     }
    Desc->pContext = Context;         // GetThreadContext/SetThreadContext will access this context instead of latest which is in this ExceptionHandler now
    this->Report_EXCEPTION_DEBUG_INFO(TIdx, ThreadID, ExceptionRecord);     // Unlocks and suspends self
    Res = Desc->SyncGetFlags(CThreadList::SThDesc::tfDbgCont);       //DbgCont;              // Continued by a debugger
    Desc->pContext = NULL;            // Is Desc will be valid? Can someone remove this thread from list before resuming it?
    DBGMSG("Resumed: ID=%u, Cont=%u, Addr=%p, TF=%u", ThreadID, (int)Res, GetInstrPtr(Context), bool(Context->EFlags&0x0100));     // Logging can deadlock if this thead is suspended while holding a logging Critical Section
    return Res;
  }
 return false;
} 
//------------------------------------------------------------------------------------
bool TryAddCurrThread(void)
{
 DBGMSG("Enter");
 TEB* CurTeb = NtCurrentTeb();
// if((UINT)CurTeb->ClientId.UniqueThread == this->DbgBrkThID){DBGMSG("Skipping DbgBrk thread"); return false;}
 this->ThList.Lock();
 if(this->ThList.FindThreadIdxInList(NULL, (UINT)CurTeb->ClientId.UniqueThread) < 0)  
  {
   int ThIdx = this->Report_CREATE_THREAD_DEBUG_EVENT(CurTeb, true);     // Reuses the lock
   DBGMSG("Added: ThreadId=%u as %u",(UINT)CurTeb->ClientId.UniqueThread, ThIdx);
   return true;
  }
 this->ThList.UnLock();
 return false;  // Already in list
}
//------------------------------------------------------------------------------------
int DebugThreadLoad(DWORD ThID, PCONTEXT Context) 
{
 if(!this->IsActive() || !this->HideDbgState)return -1;
 this->ThList.Lock();
 int thidx = this->ThList.FindThreadIdxInList(NULL,ThID);
 if(thidx < 0){this->ThList.UnLock(); return -2;}
 this->ThList.WriteDbgContext(thidx, Context);    // Restore fake DRx from our buffer to Context   // What about TF?
 this->ThList.UnLock();
 return thidx;
}
//------------------------------------------------------------------------------------
int DebugThreadSave(DWORD ThID, PCONTEXT Context)    // Call this after an application has modified the CONTEXT
{
 if(!this->IsActive() || !this->HideDbgState)return -1;
 this->ThList.Lock();
 int thidx = this->ThList.FindThreadIdxInList(NULL,ThID);
 if(thidx < 0){this->ThList.UnLock(); return -2;}
 this->ThList.ReadDbgContext(thidx, Context);   // Save (Possibly modified) DRx into our internal buffer
 this->ThList.UnLock();
 return thidx;
} 
//------------------------------------------------------------------------------------
static void DebugRstExcContext(PCONTEXT ExcCont, PCONTEXT HandledCtx)
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
 NTSTATUS Status = NtQueryVirtualMemory(NtCurrentProcess,Addr,MemoryBasicInformation,&minf,sizeof(MEMORY_BASIC_INFORMATION),&RetLen);
 if(Status || !(minf.State & MEM_COMMIT))return 0;
 return minf.RegionSize;
}
//------------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------------
// Suspends the specified main thread
int Report_CREATE_PROCESS_DEBUG_EVENT(TEB* pMainThTeb)    // NOTE: Opening a process is more suspicious than opening a thread
{
 if(!this->IsActive())return -9;  
 wchar_t Path[600];
 HMODULE  hMod = (HMODULE)NtCurrentTeb()->ProcessEnvironmentBlock->ImageBaseAddress;        // NNTDLL::GetModuleBaseLdr(NULL);  
 UINT PathSize = NNTDLL::GetMappedFilePath(NtCurrentProcess, hMod, Path, sizeof(Path));     // Get from mapping, avoid accessing an unlocked loader    
 if(!PathSize){DBGMSG("No name for mapped %p, continuing!", hMod); return -8;}

 this->ThList.Lock();
 UINT ThreadID = (UINT)pMainThTeb->ClientId.UniqueThread;
 int TIdx = this->ThList.AddThreadToList(pMainThTeb);
 if(TIdx < 0){DBGMSG("Failed to add the thread %u", ThreadID);}
 DbgEvtEx* evt = this->BegDbgEvent(ThreadID, PathSize * sizeof(wchar_t));
 evt->dwDebugEventCode = CREATE_PROCESS_DEBUG_EVENT;
 evt->dwProcessId = (UINT)pMainThTeb->ClientId.UniqueProcess;  // this->CurProcID;
 evt->dwThreadId  = (UINT)pMainThTeb->ClientId.UniqueThread;   // this->MainThID;   // This ID is not used by TitanEngine(Set to NULL)           
 *evt->FilePath                                 = 0;   
 evt->u.CreateProcessInfo.hFile                 = NULL;              // Cannot allow an another process to have one of DLLs handle opened because some protection driver may track it
 evt->u.CreateProcessInfo.hProcess              = UintToFakeHandle(evt->dwProcessId);    // HANDLE(0 - evt.dwProcessId);    // x64Dbg uses this handles directly and will stop working without them
 evt->u.CreateProcessInfo.hThread               = UintToFakeHandle(TIdx);     // HANDLE(0 - this->ThList.AddThreadToList(evt.dwThreadId) - 1);
 evt->u.CreateProcessInfo.lpBaseOfImage         = hMod;      // HANDLE is not always a Base?
 evt->u.CreateProcessInfo.dwDebugInfoFileOffset = 0;
 evt->u.CreateProcessInfo.nDebugInfoSize        = 0;
 evt->u.CreateProcessInfo.lpThreadLocalBase     = pMainThTeb;  
 evt->u.CreateProcessInfo.lpStartAddress        = NULL;   // May be required!
 evt->u.CreateProcessInfo.lpImageName           = NULL;
 evt->u.CreateProcessInfo.fUnicode              = 0;
 evt->PathSize = PathSize; 
 if(PathSize)memcpy(&evt->FilePath, &Path, (PathSize+1)*sizeof(wchar_t));
 DBGMSG("PushDbgEvt: CurProcID=%u, MainThID=%u, ExePath(%u)=%ls",evt->dwProcessId,evt->dwThreadId,PathSize,&Path);
 this->EndDbgEvent(evt, TIdx);
 if((pMainThTeb == NtCurrentTeb())&&(TIdx >= 0))this->ThList.Suspend(TIdx,NULL,false,true);   // Unlock and suspend itself (If reporting itself)
   else this->ThList.UnLock();
 return TIdx; 
}
//------------------------------------------------------------------------------------
int Report_EXIT_PROCESS_DEBUG_EVENT(TEB* pThTeb, DWORD ExitCode)      // Unused and unreliable  // A process lives until any of its threads alive
{
 if(!this->IsActive())return -9;
 this->ThList.Lock();
 UINT ThreadID = (UINT)pThTeb->ClientId.UniqueThread;
 int TIdx = this->ThList.FindThreadIdxInList(NULL,ThreadID); 
 if(TIdx < 0)
  {
   DBGMSG("Failed to find thread %u!", ThreadID);
   TIdx = this->Report_CREATE_THREAD_DEBUG_EVENT(pThTeb, true);
   if(TIdx < 0){DBGMSG("Failed to add the thread %u", ThreadID);}
   this->ThList.Lock();
  } 
 DbgEvtEx* evt = this->BegDbgEvent(ThreadID);
 evt->dwDebugEventCode = EXIT_PROCESS_DEBUG_EVENT;
 evt->dwProcessId = this->CurProcID;
 evt->dwThreadId  = this->MainThID;   // May be incorrect(Terminated)
 evt->PathSize    = 0;
 evt->u.ExitProcess.dwExitCode = ExitCode;
 DBGMSG("PushDbgEvt: ExitCode=%08X, Size=%08X",ExitCode,sizeof(evt)); 
 this->EndDbgEvent(evt, TIdx);
 if((pThTeb == NtCurrentTeb())&&(TIdx >= 0))this->ThList.Suspend(TIdx,NULL,false,true);   // Unlock and suspend itself (If reporting itself)
   else this->ThList.UnLock();
 return 0; 
}
//------------------------------------------------------------------------------------
int Report_CREATE_THREAD_DEBUG_EVENT(TEB* pThTeb, bool ReuseLock=false, bool CheckForDups=false)
{
 if(!this->IsActive())return -9;
 if(!ReuseLock)this->ThList.Lock();
 UINT ThreadID = (UINT)pThTeb->ClientId.UniqueThread;
 if(CheckForDups && this->IsExistDbgEvent(CREATE_THREAD_DEBUG_EVENT, ThreadID, pThTeb)){DBGMSG("Already reported!"); this->ThList.UnLock(); return -8;}   // If called from DebugerAttach afrer AddressSpace scan
 int TIdx = this->ThList.AddThreadToList(pThTeb);
 if(TIdx < 0){DBGMSG("Failed to add the thread %u", ThreadID);}
 DbgEvtEx* evt = this->BegDbgEvent(ThreadID);
 evt->dwDebugEventCode = CREATE_THREAD_DEBUG_EVENT;
 evt->dwProcessId = (UINT)pThTeb->ClientId.UniqueProcess;   // this->CurProcID;
 evt->dwThreadId  = (UINT)pThTeb->ClientId.UniqueThread;    // NOTE: May crash if pThTeb become invalid!!!
 evt->PathSize    = 0;
 evt->u.CreateThread.hThread           = UintToFakeHandle(TIdx);    // Not a real handle but easy to identify
 evt->u.CreateThread.lpStartAddress    = NULL;   // May be required!
 evt->u.CreateThread.lpThreadLocalBase = pThTeb;   
 DBGMSG("PushDbgEvt: ThreadID=%08X(%u)",evt->dwThreadId,evt->dwThreadId);
 this->EndDbgEvent(evt, TIdx);
 if((pThTeb == NtCurrentTeb())&&(TIdx >= 0))this->ThList.Suspend(TIdx,NULL,false,true);   // Unlock and suspend itself (If reporting itself)
   else this->ThList.UnLock();
 return TIdx; 
}
//------------------------------------------------------------------------------------
int Report_EXIT_THREAD_DEBUG_EVENT(TEB* pThTeb, DWORD ExitCode, bool ReuseLock=false)
{
 if(!this->IsActive())return -9;
 if(!ReuseLock)this->ThList.Lock();
 UINT ThreadID = (UINT)pThTeb->ClientId.UniqueThread;
 int TIdx = this->ThList.FindThreadIdxInList(NULL,ThreadID);  
 if(TIdx < 0)
  {
   DBGMSG("Failed to find thread %u!", ThreadID);
   TIdx = this->Report_CREATE_THREAD_DEBUG_EVENT(pThTeb, true);
   if(TIdx < 0){DBGMSG("Failed to add the thread %u", ThreadID);}
   this->ThList.Lock();
  }
 DbgEvtEx* evt = this->BegDbgEvent(ThreadID);
 evt->dwDebugEventCode = EXIT_THREAD_DEBUG_EVENT;
 evt->dwProcessId = this->CurProcID;   
 evt->dwThreadId  = ThreadID;       // Do not access its TEB in case it is already deallocated
 evt->PathSize    = 0;
 evt->u.ExitThread.dwExitCode = ExitCode;
 DBGMSG("PushDbgEvt: ThreadID=%08X(%u), ExitCode=%08X",ThreadID,ThreadID,ExitCode);
 this->EndDbgEvent(evt, TIdx);
 if((pThTeb == NtCurrentTeb())&&(TIdx >= 0))this->ThList.Suspend(TIdx,NULL,true,true);   // Unlock and suspend itself (If reporting itself)
   else this->ThList.UnLock();
 return 0; 
}
//------------------------------------------------------------------------------------
// TODO: Accept not only mapped PE images?
int Report_LOAD_DLL_DEBUG_INFO(TEB* pThTeb, PVOID DllBase, bool CheckForDups=false)   
{
 if(!this->IsActive())return -9;
 wchar_t Path[600];
 UINT PathSize = NNTDLL::GetMappedFilePath(NtCurrentProcess, DllBase, Path, sizeof(Path)); // Get from mapping, avoid accessing an unlocked loader   
 if(!PathSize){DBGMSG("No name for mapped %p, skipping!", DllBase); return -8;}
              
 this->ThList.Lock();
 UINT ThreadID = (UINT)pThTeb->ClientId.UniqueThread;
 if(CheckForDups && this->IsExistDbgEvent(LOAD_DLL_DEBUG_EVENT, ThreadID, DllBase)){DBGMSG("Already reported!"); this->ThList.UnLock(); return -8;}   // If called from DebugerAttach afrer AddressSpace scan
 int TIdx = this->ThList.FindThreadIdxInList(NULL,ThreadID);  
 if(TIdx < 0)
  {
   DBGMSG("Failed to find thread %u!", ThreadID);
   TIdx = this->Report_CREATE_THREAD_DEBUG_EVENT(pThTeb, true);
   if(TIdx < 0){DBGMSG("Failed to add the thread %u", ThreadID);}
   this->ThList.Lock();
  }
 DbgEvtEx* evt = this->BegDbgEvent(ThreadID, PathSize * sizeof(wchar_t));
 evt->dwDebugEventCode = LOAD_DLL_DEBUG_EVENT;
 evt->dwProcessId = this->CurProcID;
 evt->dwThreadId  = ThreadID;   // Hide Client thread from debugger    // Always acceptable?
 *evt->FilePath                       = 0;
 evt->u.LoadDll.hFile                 = NULL;        // NOTE: TitanEngine will try to call CreateFileMappingA on it
 evt->u.LoadDll.lpBaseOfDll           = DllBase;
 evt->u.LoadDll.dwDebugInfoFileOffset = 0;
 evt->u.LoadDll.nDebugInfoSize        = 0;
 evt->u.LoadDll.lpImageName           = NULL;   // Find it in DLL or in PEB
 evt->u.LoadDll.fUnicode              = 0;
 evt->PathSize = PathSize; 
 if(PathSize)memcpy(&evt->FilePath, &Path, (PathSize+1)*sizeof(wchar_t));
 DBGMSG("PushDbgEvt: DllBase=%p, DllPath(%u)=%ls",DllBase,PathSize,&Path);
 this->EndDbgEvent(evt, TIdx);
 if((pThTeb == NtCurrentTeb())&&(TIdx >= 0))this->ThList.Suspend(TIdx,NULL,false,true);   // Unlock and suspend itself (If reporting itself)
   else this->ThList.UnLock();
 return 0; 
}
//------------------------------------------------------------------------------------
// Suspends a caller thread
int Report_UNLOAD_DLL_DEBUG_EVENT(TEB* pThTeb, PVOID DllBase)
{
 if(!this->IsActive())return -9;
 this->ThList.Lock();
 UINT ThreadID = (UINT)pThTeb->ClientId.UniqueThread;
 int TIdx = this->ThList.FindThreadIdxInList(NULL,ThreadID);  
 if(TIdx < 0)
  {
   DBGMSG("Failed to find thread %u!", ThreadID);
   TIdx = this->Report_CREATE_THREAD_DEBUG_EVENT(pThTeb, true);
   if(TIdx < 0){DBGMSG("Failed to add the thread %u", ThreadID);}
   this->ThList.Lock();
  }
 DbgEvtEx* evt = this->BegDbgEvent(ThreadID);
 evt->dwDebugEventCode = UNLOAD_DLL_DEBUG_EVENT;
 evt->dwProcessId = this->CurProcID;
 evt->dwThreadId  = ThreadID;    // Always acceptable?   
 evt->PathSize    = 0;
 evt->u.UnloadDll.lpBaseOfDll = DllBase;
 DBGMSG("PushDbgEvt: DllBase=%p",DllBase);
 this->EndDbgEvent(evt, TIdx);
 if((pThTeb == NtCurrentTeb())&&(TIdx >= 0))this->ThList.Suspend(TIdx,NULL,false,true);   // Unlock and suspend itself (If reporting itself)
   else this->ThList.UnLock();
 return 0; 
}
//------------------------------------------------------------------------------------
// Suspends a caller thread
int Report_EXCEPTION_DEBUG_INFO(int ThIdx, DWORD ThreadID, PEXCEPTION_RECORD ExceptionRecord, BOOL FirstChance=1)   // No suspending here, do it in HandleException where the thread index is already found
{
 DbgEvtEx* evt = this->BegDbgEvent(ThreadID);
 evt->dwDebugEventCode = EXCEPTION_DEBUG_EVENT;
 evt->dwProcessId = this->CurProcID;
 evt->dwThreadId  = ThreadID;       // GetThreadDesc is UNSAFE! 
 evt->PathSize    = 0;
 evt->u.Exception.dwFirstChance = FirstChance;
 memcpy(&evt->u.Exception.ExceptionRecord,ExceptionRecord,sizeof(EXCEPTION_RECORD));
 DBGMSG("PushDbgEvt: ThID=%08X(%u), Code=%08X, Addr=%p",evt->dwThreadId,evt->dwThreadId,ExceptionRecord->ExceptionCode,ExceptionRecord->ExceptionAddress);
 this->EndDbgEvent(evt, ThIdx);
 this->ThList.Suspend(ThIdx,NULL,false,true);       // Unlock and Suspend itself
 return 0; 
}
//------------------------------------------------------------------------------------

};
//====================================================================================
};