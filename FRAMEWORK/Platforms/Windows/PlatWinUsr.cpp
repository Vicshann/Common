
// All platform specific includes go below. Because of them we have this as a CPP file

#include "windows.h"  

// NOTE: Type definitions must match with definitions in a Header file
// Keep arguments of 'AllocMemLL' and 'AllocMemHL' same to allow the functions to be usable by pointers to any of them
namespace Platform
{
HANDLE hHeap = NULL;
//===========================================================================
PVOID _fastcall AllocMemLL(SIZE_T AllocSize, PVOID Mem, SIZE_T ReserveSize, SIZE_T Align)  // ReserveSize includes AllocSize   // Can`t do reallocation, only can extend a prev reserved memory
{
 if(ReserveSize)
  {
   Mem = VirtualAlloc(Mem,ReserveSize,MEM_RESERVE,PAGE_EXECUTE_READWRITE);
   if(!AllocSize)return Mem;      // Reserve only
  }
 return VirtualAlloc(Mem,AllocSize,MEM_COMMIT,PAGE_EXECUTE_READWRITE);
}
//---------------------------------------------------------------------------
void _fastcall FreeMemLL(PVOID Mem, SIZE_T Size)
{
 if(Size)VirtualFree(Mem,Size,MEM_DECOMMIT);
   else VirtualFree(Mem,0,MEM_RELEASE);    // Mem must point to base address returned from VirtualAlloc
}
//===========================================================================
PVOID _fastcall AllocMemHL(SIZE_T AllocSize, PVOID Mem, SIZE_T ReserveSize, SIZE_T Align)         // Can do reallocation of memory, returning a new pointer
{
 if(!hHeap)hHeap = GetProcessHeap();
 if(Mem)return HeapReAlloc(hHeap,HEAP_ZERO_MEMORY,Mem,AllocSize); 
 return HeapAlloc(hHeap,HEAP_ZERO_MEMORY,AllocSize);
}
//---------------------------------------------------------------------------
void _fastcall FreeMemHL(PVOID Mem, SIZE_T Size)
{
 if(!hHeap)hHeap = GetProcessHeap();
 HeapFree(hHeap,0,Mem);
}
//===========================================================================
/*  // bool ProbeMemory(void* Addr, size_t Size, int Flags[Read,Write])  // 
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
