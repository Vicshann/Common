
private:
//------------------------------------------------------------------------------------------------------------
static NTHD::SThCtx* InitThreadRec(vptr ThProc, vptr ThData, size_t StkSize, size_t TlsSize, size_t DatSize, size_t** StkFrame)
{
 DatSize = AlignP2Frwd(DatSize, 16);
 if(StkSize)StkSize = AlignP2Frwd(StkSize, MEMPAGESIZE);   // NOTE: As StkSize is aligned to a page size, there will be at least one page wasted for ThreadContext struct (Assume it always available for some thread local data?)
   else StkSize = 0x10000;  // 64K should be optimal
 TlsSize = AlignP2Frwd(TlsSize, 16);   // Slots is at least of pointer size
 size_t FStkLen = AlignP2Frwd(DatSize+StkSize+TlsSize+sizeof(NTHD::SThCtx), MEMGRANSIZE);     // NOTE: MEMGRANSIZE may be more than MEMPAGESIZE (On windows)

// Find/alloc a new thread rec                  / TODO: Init several pages if available
 uint8* StkPtr = nullptr;
 if(!fwsinf.ThreadInfo)    // Alloc first thread list page
  {
   DBGMSG("Allocating first thread list page");
#ifdef SYS_WINDOWS
   vptr NewPage = NPTM::NAPI::mmap(nullptr, MEMPAGESIZE, PX::PROT_READ|PX::PROT_WRITE, PX::MAP_PRIVATE|PX::MAP_ANONYMOUS, -1, 0);  // Actually reserves entire 64K block  // Must be a separate allocation on Windows - cannot unmap partially
   if(uint err=MMERR(NewPage);err)return (NTHD::SThCtx*)err;
   fwsinf.ThreadInfo = (NTHD::SThInf*)NewPage;
   NewPage = NPTM::NAPI::mmap(nullptr, FStkLen, PX::PROT_READ|PX::PROT_WRITE, PX::MAP_PRIVATE|PX::MAP_ANONYMOUS, -1, 0); 
   if(uint err=MMERR(NewPage);err)return (NTHD::SThCtx*)err;
   StkPtr  = (uint8*)NewPage;
#else
   vptr NewPage = NPTM::NAPI::mmap(nullptr, MEMPAGESIZE+FStkLen, PX::PROT_READ|PX::PROT_WRITE, PX::MAP_PRIVATE|PX::MAP_ANONYMOUS, -1, 0);  // Allocate together with a new rec stack  
   if(uint err=MMERR(NewPage);err)return (NTHD::SThCtx*)err;
   fwsinf.ThreadInfo = (NTHD::SThInf*)NewPage;
   StkPtr = ((uint8*)NewPage + MEMPAGESIZE);
#endif
  }
 NTHD::SThInf** PNewPagePtr = nullptr;
 NTHD::SThCtx** PRecPtr     = fwsinf.ThreadInfo->GetUnusedRec(&PNewPagePtr);
 if(!PRecPtr)         // NOTE: On Windows entire 64K is reserved and additional 4k pages are allocated from that
  {
   DBGMSG("Allocating another thread list page");
#ifdef SYS_WINDOWS
   vptr Addr = vptr(AlignP2Bkwd((size_t)PNewPagePtr, MEMPAGESIZE) + MEMPAGESIZE);  // Next page in the reserved 64K block  // PNewPagePtr is in the last page
#else
   vptr Addr = nullptr;
#endif
   vptr NewPage = NPTM::NAPI::mmap(Addr, MEMPAGESIZE, PX::PROT_READ|PX::PROT_WRITE, PX::MAP_PRIVATE|PX::MAP_ANONYMOUS, -1, 0);    // Allocate together with a new stack area stack
   if(uint err=MMERR(NewPage);err)return (NTHD::SThCtx*)err;
   StkPtr  = ((uint8*)NewPage + MEMPAGESIZE);
   PRecPtr = fwsinf.ThreadInfo->SetNewPageAndGetRec(NewPage, PNewPagePtr);
  }
 NTHD::SThCtx* ThRec = NTHD::ReadRecPtr(PRecPtr);
 uint OldID   = -1;
 uint OldHnd  = -1;
 sint OldStat = NTHD::THD_MAX_STATUS;  // Reset (If this code stays after a thread exits - the exit was not normal)
 if(ThRec)   // Already allocated
  {
   DBGMSG("Reusing the thread rec: %p",ThRec);
   OldID   = ThRec->LastThrdID;
   OldHnd  = ThRec->LastThrdHnd;
   OldStat = ThRec->ExitCode;    // Preserve last thread info
   if(ThRec->StkSize < FStkLen)
    {
     StkPtr = (uint8*)NPTM::NAPI::mmap(nullptr, FStkLen, PX::PROT_READ|PX::PROT_WRITE, PX::MAP_PRIVATE|PX::MAP_ANONYMOUS, -1, 0);   // TODO: mrealloc
     if(uint err=MMERR(StkPtr);err)return (NTHD::SThCtx*)err;
     NPTM::NAPI::munmap(ThRec->StkBase, ThRec->StkSize);   // Unmap old stack
    }
    else
     {
      StkPtr  = (uint8*)ThRec->StkBase;
      FStkLen = ThRec->StkSize;
     }
  }
  else if(!StkPtr)StkPtr = (uint8*)NPTM::NAPI::mmap(nullptr, FStkLen, PX::PROT_READ|PX::PROT_WRITE, PX::MAP_PRIVATE|PX::MAP_ANONYMOUS, -1, 0);  // May be already allocated with a thread rec page
 if(uint err=MMERR(StkPtr);err)return (NTHD::SThCtx*)err;

 NTHD::SThCtx* ThrFrame = (NTHD::SThCtx*)&StkPtr[StkSize];  // Since StkSize is page-aligned, ThrFrame is also page aligned. It is possible to find it by scanning the stack forward by pages from any addr on that stack. Is it faster than scanning thread list?  (Need to place main thread`s ctx on stack too)
 vptr DataPtr = &StkPtr[StkSize+sizeof(NTHD::SThCtx)];
 vptr TlsPtr  = &StkPtr[StkSize+DatSize+sizeof(NTHD::SThCtx)];
 *StkFrame    = (size_t*)&StkPtr[StkSize];      // Decreasing stack pointer only!      // NOTE: Keep the stack aligned to 16
 if(ThData && DatSize)memcpy(DataPtr, ThData, DatSize);    // User data is right at the bottom

 ThrFrame->Self        = ThrFrame;   // For checks
 ThrFrame->SelfPtr     = (vptr*)PRecPtr;    // Need thread id to init  (Assigned in STC::ThProcCall)
 ThrFrame->TlsBase     = TlsPtr;
 ThrFrame->TlsSize     = TlsSize;
 ThrFrame->StkBase     = StkPtr;     // For unmapping
 ThrFrame->StkSize     = FStkLen;    // StkSize; ??? // Need full size for unmap  // Can a thread unmap its own stack before calling 'exit'?
 ThrFrame->StkOffs     = StkSize;
 ThrFrame->GroupID     = NAPI::getpgrp();   // pid
 ThrFrame->ThreadID    = 0;  // Will be written to by 'clone'  // And reset at its termination by the system
 ThrFrame->ProcesssID  = NAPI::getpid();
 ThrFrame->LastThrdID  = OldID;
 ThrFrame->LastThrdHnd = OldHnd;
 ThrFrame->ThreadHndl  = 0;    // Set by system (Windows)
 ThrFrame->ThreadProc  = (vptr)ThProc;
 ThrFrame->ThreadData  = DataPtr;
 ThrFrame->ThDataSize  = DatSize;
 ThrFrame->ExitCode    = OldStat;
 //ThrFrame->EntryCtr    = 0;   // Unentered  // Later, any entered thread with zero TID will be considered dead and for reuse
 ThrFrame->Flags       = 0;   
 NTHD::WriteRecPtr(PRecPtr, ThrFrame);   // Update the pointer
 return ThrFrame;
}
//------------------------------------------------------------------------------------------------------------
_noret static void _ninline 
#ifdef SYS_WINDOWS
_scall ThProcCallStub(NT::PVOID Data, NT::SIZE_T Size)       // Static, no inlining, args in registers   // TODO: Register it with Control Flow Guard somehow or NtCreateThread will fail in CFG enabled processes
{
 NTHD::SThCtx* ThrFrame = (NTHD::SThCtx*)Data;   // Should be same ptr or something is pushed
#else
_fcall ThProcCallStub(void)       // Static, no inlining, args in registers
{
 NTHD::SThCtx* ThrFrame = (NTHD::SThCtx*)AlignP2Frwd((size_t)GETSTKFRAME(), 16);   // Should be same ptr or something is pushed
#endif
 DBGMSG("hello thread: ThrFrame=%p, GroupID=%i, ProcesssID=%i, ThreadID=%i: %p",ThrFrame,ThrFrame->GroupID,ThrFrame->ProcesssID,ThrFrame->ThreadID,GetThreadByID(ThrFrame->ThreadID));
 sint res = PXERR(EFAULT);
 if(ThrFrame == ThrFrame->Self)
  {
//     ThrFrame->EntryCtr++;    // Entered, TID is not zero
   res = ((NTHD::PThreadProc)ThrFrame->ThreadProc)(ThrFrame);
   ThrFrame->LastThrdID  = ThrFrame->ThreadID;               // Is it OK that the exit point will belong to this module? May be it will be reqiured to create a thread in another module and unload this later (hot reloading?)
   ThrFrame->LastThrdHnd = ThrFrame->ThreadHndl;    
   ThrFrame->ExitCode    = res;
#ifdef SYS_WINDOWS
   NT::HANDLE hndl = ThrFrame->ThreadHndl;  // TODO: Memory barrier
   ThrFrame->ThreadHndl  = 0;  // The system will not clear this for us  // Windows will not clear ThreadID either!
   ThrFrame->ThreadID    = 0;
   SAPI::NtClose(hndl);        // If we close this handle before TerminateThread then anyone who waits on the handle will get ABORT notification. GetExitCodeThread will not be returning the valid exit code yet but we do not use that anyway 
#endif
   NTHD::ReleaseRec((NTHD::SThCtx**)(ThrFrame->SelfPtr));     // TODO: Remove from mem rec. For now: Keep the stack memory to be reused by another new thread (Cannot deallocate stack without ASM, the compiler won`t store 'res' in a register and will touch the stack for some other useless reasons anyway)
  }
 NAPI::exit(res);  // Any ABI preserved registers are not important at this point  ThProc(nullptr,0)
} 
//------------------------------------------------------------------------------------------------------------
public: