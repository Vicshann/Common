
// All platform specific includes go below. Because of them we have this as a CPP file

#include "windows.h"  
#include "Platforms\Platform.hpp" 

// NOTE: Type definitions must match with definitions in a Header file
namespace NPTM
{
HANDLE hHeap = NULL;

extern "C"
{
// Keep arguments of 'AllocMemLL' and 'AllocMemHL' same to allow the functions to be usable by pointers to any of them
//===========================================================================
// You must know a platform`s alloc granularity to determine real size of allocated block
//
PVOID _fastcall AllocMemLL(PVOID Mem, SIZE_T Size, SIZE_T AllocSize, SIZE_T ReserveSize, SIZE_T Align)  // ReserveSize includes AllocSize   // Alignment is already max 64k on Windows
{
 bool   NoReloc = Align > 0x7FFFFFFF;  // See MEMNORELOCATE in Common.hpp         // TODO: Alloc in cycle from a last addr aligned to specified alignment
 PVOID  OldMem  = Mem;
 PVOID  LstBase = NULL;
 SIZE_T LstSize = 0;
 if(Mem && Size && (AllocSize < Size))   // Shrinking: Put the pages in Reserved state
  {
   Size      = AlignFrwrd(Size, MEMPAGESIZE);
   AllocSize = AlignFrwrd(AllocSize, MEMPAGESIZE);
   if(AllocSize != Size)VirtualFree(((char*)Mem + AllocSize),Size-AllocSize,MEM_DECOMMIT);   // TODO: Check alignment
   return Mem;
  }
 if(ReserveSize)
  {
   Mem = VirtualAlloc(Mem,ReserveSize,MEM_RESERVE,PAGE_EXECUTE_READWRITE);    // Can try to reserve more after a committed block
   if(!Mem && OldMem && !NoReloc)Mem = VirtualAlloc(NULL,ReserveSize,MEM_RESERVE,PAGE_EXECUTE_READWRITE);   // Reserve an new block (Old data need to be copied after this)
   if(!Mem)return NULL;    // Failed to reserve even as a different block!
  }
 if(AllocSize)
  {
   PVOID PrvMem = Mem;
   Mem = VirtualAlloc(Mem,AllocSize,MEM_COMMIT,PAGE_EXECUTE_READWRITE);       // What if COMMIT failed in a block from 'VirtualAlloc(NULL,ReserveSize,MEM_RESERVE,PAGE_EXECUTE_READWRITE)' ??? - Memory will leak!  (Should not happen with sane usage of allocators)
   if(!Mem && PrvMem && !ReserveSize && !NoReloc)Mem = VirtualAlloc(NULL,AllocSize,MEM_COMMIT,PAGE_EXECUTE_READWRITE);   // Try to allocate without reserving
   if(!Mem)return NULL;  // Failed to commit even as a different block!
  }
 if(OldMem && Mem && (OldMem != Mem))
  {
   memcpy(Mem, OldMem, Size);    // Specifying separate Size we can copy only used data from an old block 
   VirtualFree(OldMem,0,MEM_RELEASE);  // FreeMemLL(OldMem, ReserveSize);     // Free only if a new is allocated  // Releases everything from this base address
  }
 return Mem;
}
//---------------------------------------------------------------------------
bool _fastcall FreeMemLL(PVOID Mem, SIZE_T Size)
{
 if(Size)return VirtualFree(Mem,Size,MEM_DECOMMIT);
   else return VirtualFree(Mem,0,MEM_RELEASE);    // Mem must point to base address returned from VirtualAlloc
}
//===========================================================================
PVOID _fastcall AllocMemHL(PVOID Mem, SIZE_T Size, SIZE_T AllocSize, SIZE_T ReserveSize, SIZE_T Align)         // Can do reallocation of memory, returning a new pointer and copying old contents
{
 if(!hHeap)hHeap = GetProcessHeap();
 if(Mem)
  {
   UINT Flags = HEAP_ZERO_MEMORY;    // All low level mem allocation functions shoul return Zeroed memory
   if(Align > 0x7FFFFFFF)Flags |= HEAP_REALLOC_IN_PLACE_ONLY;   // Alignment is NOT supported for this allocation type
   return HeapReAlloc(hHeap,HEAP_ZERO_MEMORY,Mem,AllocSize); 
  }
 return HeapAlloc(hHeap,HEAP_ZERO_MEMORY,AllocSize);
}
//---------------------------------------------------------------------------
bool _fastcall FreeMemHL(PVOID Mem, SIZE_T Size)
{
 if(!hHeap)hHeap = GetProcessHeap();
 return HeapFree(hHeap,0,Mem);
}
//===========================================================================
/*  


OK, I just tried it on Linux myself. The mprotect thing didn't work by itself, but this does:
    mprotect(addr, len, PROT_NONE);
    madvise(addr, len, MADV_DONTNEED);

I'm not sure about Mac.
>>>
Hmm, I realized I may have been wrong here, Paul. In theory, if you do this:
  mprotect(addr, len, PROT_NONE);
  mprotect(addr, len, PROT_READ);
then the data stored at addr should still be accessible. That means that the OS is not allowed to throw the page away. It's possible that on MacOS, it's just being paged out to disk, which is not what we want.

On Linux, the madvise(DONTNEED) call actually causes the page to be thrown away. I wonder if it has the same effect on the Mac. One way to test this is as follows:
  memset(addr, 0x37, len);
  mprotect(addr, len, PROT_NONE);
  mprotect(addr, len, PROT_READ);
  printf("%x\n", *addr);
If you get 0x37, then it was just paged out and not thrown away. In Linux, adding the DONTNEED call between the mprotect calls causes you to get 0.

Also, it turns out you don't even need mprotect on Linux. The madvise is sufficient.

This doesn't work on Mac. However, I tested the mmap call from comment 3, and that _does_ decommit, so I think we have a winner there. I'm going to experiment on doing that for jemalloc, and watch whether Tp5 drops. That will definitely confirm it.
>>>


Other (linux+android I guess):


                madvise((void *)((uintptr_t)chunk + (i <<
                    pagesize_2pow)), (npages << pagesize_2pow),
                    MADV_FREE);

    if (mmap(addr, size, PROT_NONE, MAP_FIXED | MAP_PRIVATE | MAP_ANON, -1,
        0) == MAP_FAILED)
        abort();


 >>>



1. On Linux/Mac, there's no explicit commit step.  Memory freed with MADV_DONTNEED (Linux) or MADV_FREE (Mac) doesn't need to be explicitly re-madvise'd before it's touched.  When you modify an MADV_DONTNEED'ed page, you incur a page fault which gives you a physical page there.

2. jemalloc keeps around some amount (a few mb) of committed but unused memory.  (This is reported in about:memory as heap-dirty.)

3. When jemalloc decommits, it decommits adjacent pages in a single operation.

>>>
From everything I can see, the new (> 8.1) DiscardVirtualMemory API results in very similar functionality to MEM_RESET - but MEM_RESET is available right back to XP.

For my ref:
"
nt!MiResetVirtualMemory is the kernel workhorse for VirtualAlloc(MEM_RESET). It does
the following:
1) If the page is present - Clears the DIRTY bit in PTE. (Since W2k3 if ACCCESSED bit in PTE is 1, it is cleared and the WSLE age is set to maximum, so this page is marked as a 'good candidate' to be removed from the process' working set next time the trimming happens). The space in page file allocated for this page is marked as free.  Note the page remains in working set.
2) If the page is in transition state - 
- If the page is in standby-list, there is nothing to do.
- If the page is in modified-list, it is moved to standby-list and page file space is freed.
3) Otherwise if page has been swapped out and it in the page file - page file space
is freed.

Read attempt from page that has been "reseted":
If the PTE is in demand-zero state, and you try to read, page fault happens and fresh zeroed page will be returned. If PTE is in transition state, soft-page fault happens, page is moved from the standby list to the process working set, and original content of the page will be read. And finally if PTE is in working set - original content will be read.  Note than in any moment, even in the middle of read, the page content may be taken away from you. That's why we need to write into page in order to "take ownership" of it.

Write attempt to page that has been "reseted":
If page is in the working set - PTE's DIRTY bit will be set by the hardware.
If page is demand-zero - soft page fault, fresh zeroed page is returned. PTE's DIRTY
bit will be set.
If page is in the standby list - soft page fault; page will be moved from the standby list to the working set. PTE's DIRTY bit will be set.  
So a write operation puts page back in "normal" state.
"

DiscardVirtualMemory: "Use this function to discard memory contents that are no longer needed, while keeping the memory region itself committed. Discarding memory may give physical RAM back to the system. When the region of memory is again accessed by the application, the backing RAM is restored, and the contents of the memory is undefined."


>>>


VirtualFree/munmap: removal of the pages from the process' memory mapping.  What we are currently doing is VirtualAlloc(..., MEM_RESET, ...) and mmadvise(..., MADV_DONTNEED, ...) which 
keeps the pages in the process' management, but tells the kernel that the pages are no longer in use.  The effect of this is that if the kernel needs to free up physical memory in the future,
 it will preferentially use these pages and not bother swapping them out before overwriting them.

>>>
The stricter decommit is not needed with MADV_DONTNEED on Linux.  On Linux, MADV_DONTNEED'ed pages don't count against your RSS.  
It is needed with MADV_FREE'd pages on Mac.  I don't know about Windows; we should test...
>>>
// Linux and Android provide MADV_REMOVE which is preferred as it has a
// behavior that can be verified in tests. Other POSIX flavors (MacOSX, BSDs),
// provide MADV_FREE which has the same result but memory is purged lazily.

/// GCC                 // On MacOS X use 'vm_map' for aligned allocations
#if defined(__i386__)

static __inline__ unsigned long long rdtsc(void)
{
    unsigned long long int x;
    __asm__ volatile (".byte 0x0f, 0x31" : "=A" (x));
    return x;
}

#elif defined(__x86_64__)

static __inline__ unsigned long long rdtsc(void)
{
    unsigned hi, lo;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
}

#endif


--------------------
#ifdef __i386
__inline__ uint64_t rdtsc() {
  uint64_t x;
  __asm__ volatile ("rdtsc" : "=A" (x));
  return x;
}
#elif __amd64
__inline__ uint64_t rdtsc() {
  uint64_t a, d;
  __asm__ volatile ("rdtsc" : "=a" (a), "=d" (d));
  return (d<<32) | a;
}
#endif

// bool ProbeMemory(void* Addr, size_t Size, int Flags[Read,Write])  // 
IsBadReadPtr(Exceptions) is SLOW! And on POSIX there is SUPER SLOW analog of VirtualQuery exist!

bool IsBadReadPtr(void* p)
{
    MEMORY_BASIC_INFORMATION mbi = {0};
    if (::VirtualQuery(p, &mbi, sizeof(mbi)))
    {
        DWORD mask = (PAGE_READONLY|PAGE_READWRITE|PAGE_WRITECOPY|PAGE_EXECUTE_READ|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY);
        bool b = !(mbi.Protect & mask);
        // check the page is not a guard page
        if (mbi.Protect & (PAGE_GUARD|PAGE_NOACCESS)) b = true;

        return b;
    }
    return true;
}

BOOL CanRead(LPVOID p)
{
  MEMORY_BASIC_INFORMATION mbi;
  mbi.Protect = 0;
  ::VirtualQuery(((LPCSTR)p) + len - 1, &mbi, sizeof(mbi));
  return ((mbi.Protect & 0xE6) != 0 && (mbi.Protect & PAGE_GUARD) == 0);
}

void* AllocateAddressSpace(size_t size)
{
    return VirtualAlloc(NULL, size, MEM_RESERVE , PAGE_NOACCESS);
}
 
void* CommitMemory(void* addr, size_t size)
{
    return VirtualAlloc(addr, size, MEM_COMMIT, PAGE_READWRITE);
}
 
void DecommitMemory(void* addr, size_t size)
{
    VirtualFree((void*)addr, size, MEM_DECOMMIT);
}
 
void FreeAddressSpace(void* addr, size_t size)
{
    VirtualFree((void*)addr, 0, MEM_RELEASE)
}

-------------------------------------------------------------
void* AllocateAddressSpace(size_t size)
{
    void * ptr = mmap((void*)0, size, PROT_NONE, MAP_PRIVATE|MAP_ANON, -1, 0);
    msync(ptr, size, MS_SYNC|MS_INVALIDATE);
    return ptr;
}
 
void* CommitMemory(void* addr, size_t size)
{
    void * ptr = mmap(addr, size, PROT_READ|PROT_WRITE, MAP_FIXED|MAP_SHARED|MAP_ANON, -1, 0);
    msync(addr, size, MS_SYNC|MS_INVALIDATE);
    return ptr;
}
 
void DecommitMemory(void* addr, size_t size)
{
    // instead of unmapping the address, we're just gonna trick 
    // the TLB to mark this as a new mapped area which, due to 
    // demand paging, will not be committed until used.
 
    mmap(addr, size, PROT_NONE, MAP_FIXED|MAP_PRIVATE|MAP_ANON, -1, 0);
    msync(addr, size, MS_SYNC|MS_INVALIDATE);
}
 
void FreeAddressSpace(void* addr, size_t size)
{
    msync(addr, size, MS_SYNC);
    munmap(addr, size);
}
*/

//===========================================================================
}
}
