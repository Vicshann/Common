
struct NTHD
{
enum EThDefs
{
 THD_MAX_MMGRS   = 4,
 //THD_MAX_USR_TLS = 32,
 THD_MAX_PAGELEN  = 4096,  // MEMPAGESIZE is not available here
 THD_MAX_THID     = 65536, // Max count, not index!
 THD_MAX_IDX_NUM  = THD_MAX_PAGELEN / sizeof(uint16),  // 2048
 THD_MAX_IDX_PAGE = THD_MAX_THID / THD_MAX_IDX_NUM,    // Total index pages (32)
 THD_MAX_IDX_RECS = ((THD_MAX_THID * sizeof(vptr)) / THD_MAX_PAGELEN),   // Number of pages for SThCtx pointers (allocated independently)
 THD_MAX_STATUS   = 1 << ((sizeof(size_t)*8)-1),
};


// This thread context is allocated on stack, no separate TLS memory block is used
// Need some means to retrieve its pointer without conflicts with usual TLS mechanisms to allow libc coexist with the framework
// Cannot be allocated by a thread creation function because main(system) entry point will not have same behaviour
// The max value of pid is changeable, in default it is 32768, which is the size of the max value of short int.And, it can compatible with UNIX of early version. (expect 65536)
// NOTE: It is much more efficient to just pass this structure pointer around than request it each time from ThreadID which may require a syscall
//
struct SThCtx
{
 vptr   Self;        // For checks, mostly
 vptr*  SelfPtr;     // Points to this SThCtx record ptr
 vptr   TlsBase;
 size_t TlsSize;
 vptr   StkBase;     // For unmapping
 size_t StkSize;     // Can a thread unmap its own stack before calling 'exit'?
 uint   GroupID;     // Can be changed with setpgid
 uint   ThreadID;    // May be equal ProcesssID?
 uint   ProcesssID;
 uint   LastThrdID;  // Of prev thead that owned this memory (ThreadID is set to 0 already)
 vptr   ThreadProc;
 vptr   ThreadData;
 size_t ThDataSize;
 sint   ExitCode;    // Only if exiting by thread_exit // Have to avoid that mess with 'wait' and children fixation (More than int8 of return value size as a bonus)
 //uint   EntryCtr;   // Need some way to detect dead threads and reuse them?
 uint   Flags;
 vptr   MMPtrs[THD_MAX_MMGRS];      // For thread local memory managers (mempool)
};

using PThreadProc = ssize_t (_scall *)(SThCtx*);

//struct SThPage   // For each 32768 threads id
//{
// SThCtx* ThRecs[THD_MAX_PAGELEN / sizeof(SThCtx*)];    // 32768/8 = 4096 (One page, usually)    // For now this memory is not released (Reallocated if a new thread with same ID requires larger stack)
//};

/*struct SThRec    //  SThRec Recs[THD_MAX_PAGELEN / sizeof(SThRec)];
{
 size_t MagicPtrA;  // Prev^Next   // Sorted by ID
 size_t MagicPtrB;  // Prev^Next   // Sorted by Stack base
 size_t RecPtr;
}; */

// X32: 4096 / 4 = 1024 (IDs per page);  65536 / 1024 = 64 (Page PTRs) * 4 = 256  (bytes for global thread struct)
// X64: 4096 / 8 = 512  (IDs per page);  65536 / 512 = 128 (Page PTRs) * 8 = 1024 (bytes for global thread struct)
// It could be possible to find current thread id by checking its StackFrame against stack ranges of all threads
// Have to make this structure thread safe without any sync-lock
// Showld be fast enough for a small amount of threads. And anything with large amounts of threads should cache this pointers somewhere
// Is it possible to extract ibfo from a stack frame pointer to use it in logging for indentation of function names?
// https://kernelnewbies.kernelnewbies.narkive.com/9Zd9eWeb/waitpid-2-and-clone-thread
// WARNING: BAD DESIGN!!!
//
struct SThInf  // Allocate dynamically?  // Wastes 1024 bytes on X64 in a single threaded app   // Not used for main thread!  // First pointer points to this block and a constant added to idx when accessing to start from Recs field
{
 static constexpr uint RecsOnPage = (THD_MAX_PAGELEN / sizeof(size_t))-1;
// static inline uint TotalRecs  = 0;   // No deallocation
// static inline uint TotalPages = 0;

// uint16** Indexes[THD_MAX_IDX_PAGE];     // Each record corresponds to a tid and contains index of a thread rec    // Usually just one page is wasted
// SThCtx** Pointers[THD_MAX_IDX_RECS];    // Each points to one page of THD_MAX_PAGELEN of SThCtx pointers    // Allocated by number of threads, not by tid   // Usually just one page is wasted    // SThPage* Pages[THD_MAX_THID / THD_MAX_PAGELEN];  // 65536 / 4096 (PageSize)     // idx = gettid() / 4096      // These pointers are allocated on demand
// SThCtx*  Recs[0];  // Rest of the page  // Max Threads will be less than 65536
 SThInf* NextPage;
 //size_t  Total;
 size_t  Recs[RecsOnPage];   // Low bit 1 means the record is unused
//------------------------------------------------------------------------------------------------------------
/*static _finline auto TidToIdx(uint16 tid)
{
 struct SR{uint a; uint b;};
 uint PIdx = tid >> 11;     // / 2048  (0-31)
 uint RIdx = tid & 0x7FF;   // 2047
 return SR{PIdx, RIdx};
}
//------------------------------------------------------------------------------------------------------------
static _finline auto IdxToPtr(uint16 idx)
{
 struct SR{uint a; uint b;};
 uint PIdx = tid >> 11;
 uint RIdx = tid & 0x7FF;
 return SR{PIdx, RIdx};
}  */
//------------------------------------------------------------------------------------------------------------
// A record is in use if its bit 0 is set
//
//
SThCtx** FindOldThreadByTID(size_t tid)
{
 SThInf* ThisPage  = this;
 //uint    TotalRecs = ThisPage->Total;
 for(;;)
  {
   for(uint idx=0;idx < RecsOnPage;idx++)
    {
     size_t val = ThisPage->Recs[idx];
     if(!val)break;   // No more recs    // Free recs are either 1 or a pointer
     SThCtx* ptr = (SThCtx*)(val & (size_t)~1);
     if(ptr->LastThrdID == tid)return (SThCtx**)&ThisPage->Recs[idx];
    }
   if(!ThisPage->NextPage)break;
   ThisPage  = ThisPage->NextPage;
 //  TotalRecs = ThisPage->Total;
  }
 return nullptr;
}
//------------------------------------------------------------------------------------------------------------
SThCtx** FindThByTID(size_t tid, bool ActiveOnly=true)
{
 SThInf* ThisPage  = this;
 //uint    TotalRecs = ThisPage->Total;
 for(;;)
  {
   for(uint idx=0;idx < RecsOnPage;idx++)
    {
     size_t val = ThisPage->Recs[idx];
     if(!val)break;   // No more recs    // Free recs are either 1 or a pointer
     if(!(val & 1) && ActiveOnly)continue;  // Inactive
     SThCtx* ptr = (SThCtx*)(val & (size_t)~1);
     if(ptr->ThreadID == tid)return (SThCtx**)&ThisPage->Recs[idx];
    }
   if(!ThisPage->NextPage)break;
   ThisPage  = ThisPage->NextPage;
 //  TotalRecs = ThisPage->Total;
  }
 return nullptr;
}
//------------------------------------------------------------------------------------------------------------
SThCtx** FindThByStack(vptr ptr, bool ActiveOnly=true)     // Any address on the thread`s stack
{
 SThInf* ThisPage  = this;
// uint    TotalRecs = ThisPage->Total;
 for(;;)
  {
   for(uint idx=0;idx < RecsOnPage;idx++)
    {
     size_t val = ThisPage->Recs[idx];
     if(!val)break;   // No more recs
     if(!(val & 1) && ActiveOnly)continue;  // Inactive
     SThCtx* ptr = (SThCtx*)(val & (size_t)~1);
     if(((uint8*)ptr >= (uint8*)(ptr->StkBase)) && ((uint8*)ptr < ((uint8*)(ptr->StkBase) + ptr->StkSize)))return (SThCtx**)&ThisPage->Recs[idx];
    }
   if(!ThisPage->NextPage)break;
   ThisPage  = ThisPage->NextPage;
 //  TotalRecs = ThisPage->Total;
  }
 return nullptr;
}
//------------------------------------------------------------------------------------------------------------
SThCtx** GetUnusedRec(SThInf*** NeedPage)     // Any address on the thread`s stack
{
 SThInf* ThisPage  = this;
 //uint    TotalRecs = ThisPage->Total;
 for(;;)
  {
   for(uint idx=0;idx < RecsOnPage;idx++)   // Lets hope is have no problems with sync
    {
     size_t val = ThisPage->Recs[idx];     
     if(!(val & 1))      // TODO: InterlockedAnd  (__sync_val_compare_and_swap (type *ptr, type oldval type newval, ...))   //  if the current value of *ptr is oldval, then write newval into *ptr.
      {
       ThisPage->Recs[idx] = val | 1;    // TODO: Interlocked exch     // How to use 'EntryCtr' without locking?  // Need another bit?
       return (SThCtx**)&ThisPage->Recs[idx];
      }
    }
   if(!ThisPage->NextPage)break;
   ThisPage  = ThisPage->NextPage;
//   TotalRecs = ThisPage->Total;
  }
 *NeedPage = &ThisPage->NextPage;
 return nullptr;
}
//------------------------------------------------------------------------------------------------------------
SThCtx** SetNewPageAndGetRec(vptr Page, SThInf** Place)   // NOTE: Rec will be a Null pointer
{
 SThInf* ThisPage  = this;
 SThInf* NewPB = (SThInf*)Page;
 *NewPB->Recs |= 1;   // Set furst rec as used BEFORE the page is added to the list
 for(;;)   // Compare exchange if zero     // Compare exch on a null ptr
  {
   if(!*Place)
    {
     *Place = (SThInf*)Page;
     break;
    }
   *Place = (*Place)->NextPage;
  }
 return (SThCtx**)&NewPB->Recs;
}
//------------------------------------------------------------------------------------------------------------

};

//------------------------------------------------------------------------------------------------------------
static _finline SThCtx* ReadRecPtr(SThCtx** ptr)
{
 return (SThCtx*)(size_t(*ptr) & (size_t)~0x0F);
}
//------------------------------------------------------------------------------------------------------------
static _finline void WriteRecPtr(SThCtx** ptr, SThCtx* rec)   // As busy
{
 *ptr = (SThCtx*)((size_t)rec | 1);
}
//------------------------------------------------------------------------------------------------------------
static _finline void OccupyRec(SThCtx** rec)
{
 *(size_t*)rec |= 1;   // TODO: InterlockedOr  ???
}
//------------------------------------------------------------------------------------------------------------
static _finline void ReleaseRec(SThCtx** rec)
{
 *(size_t*)rec &= (size_t)~1;   // TODO: InterlockedOr  ???
}
//------------------------------------------------------------------------------------------------------------

/*
 For now, no stack memory is reused between threads with different IDs
 Normally exiting threads will free their memory
 Thred pools should not exit their threads anyway.
 And any ended and immediately created threads will likely to have same IDs


       There are various ways of determining the number of CPUs
       available on the system, including: inspecting the contents of
       /proc/cpuinfo; using sysconf(3) to obtain the values of the
       _SC_NPROCESSORS_CONF and _SC_NPROCESSORS_ONLN parameters; and
       inspecting the list of CPU directories under
       /sys/devices/system/cpu/
*/
};
