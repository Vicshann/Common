﻿
#pragma once

#include "../PlatDef.hpp"
//============================================================================================================
_codesec struct SAPI  // All required syscall stubs    // Name 'NTAPI' causes compilation to fail with CLANG in VisualStudio! (Not anymore?)
{
private:
SCVR uint32 HashNtDll = NCRYPT::CRC32("ntdll.dll");    // Low Case
public:
static const inline uint8* pKiUserSharedData = reinterpret_cast<uint8*>(0x7FFE0000);
// By putting these in a separate section and then merging with .text allows to preseve declared orded and avoid mixing in of some global variables
DECL_SYSCALL(WPROCID(HashNtDll,"NtProtectVirtualMemory"),     NT::NtProtectVirtualMemory,     NtProtectVirtualMemory     )   // Should be first
DECL_SYSCALL(WPROCID(HashNtDll,"NtAllocateVirtualMemory"),    NT::NtAllocateVirtualMemory,    NtAllocateVirtualMemory    )
DECL_SYSCALL(WPROCID(HashNtDll,"NtFreeVirtualMemory"),        NT::NtFreeVirtualMemory,        NtFreeVirtualMemory        )
DECL_SYSCALL(WPROCID(HashNtDll,"NtReadVirtualMemory"),        NT::NtReadVirtualMemory,        NtReadVirtualMemory        )
DECL_SYSCALL(WPROCID(HashNtDll,"NtWriteVirtualMemory"),       NT::NtWriteVirtualMemory,       NtWriteVirtualMemory       )
DECL_SYSCALL(WPROCID(HashNtDll,"NtQueryVirtualMemory"),       NT::NtQueryVirtualMemory,       NtQueryVirtualMemory       )

DECL_SYSCALL(WPROCID(HashNtDll,"NtCreateFile"),               NT::NtCreateFile,               NtCreateFile               )     // NtFsControlFile
DECL_SYSCALL(WPROCID(HashNtDll,"NtWriteFile"),                NT::NtWriteFile,                NtWriteFile                )
DECL_SYSCALL(WPROCID(HashNtDll,"NtReadFile"),                 NT::NtReadFile,                 NtReadFile                 )
DECL_SYSCALL(WPROCID(HashNtDll,"NtDeleteFile"),               NT::NtDeleteFile,               NtDeleteFile               )
DECL_SYSCALL(WPROCID(HashNtDll,"NtWriteFileGather"),          NT::NtWriteFileGather,          NtWriteFileGather          )
DECL_SYSCALL(WPROCID(HashNtDll,"NtReadFileScatter"),          NT::NtReadFileScatter,          NtReadFileScatter          )
DECL_SYSCALL(WPROCID(HashNtDll,"NtFlushBuffersFile"),         NT::NtFlushBuffersFile,         NtFlushBuffersFile         )
DECL_SYSCALL(WPROCID(HashNtDll,"NtQueryAttributesFile"),      NT::NtQueryAttributesFile,      NtQueryAttributesFile      )   // Uses a file name
DECL_SYSCALL(WPROCID(HashNtDll,"NtQueryInformationFile"),     NT::NtQueryInformationFile,     NtQueryInformationFile     )   // Uses a file handle
DECL_SYSCALL(WPROCID(HashNtDll,"NtQueryDirectoryFile"),       NT::NtQueryDirectoryFile,       NtQueryDirectoryFile       )
DECL_SYSCALL(WPROCID(HashNtDll,"NtSetInformationFile"),       NT::NtSetInformationFile,       NtSetInformationFile       )   // NtSetInformationFile supports file information classes not supported by SetFileInformationByHandle (FileDispositionInformationEx: FILE_DISPOSITION_POSIX_SEMANTICS [posix stype delete])
DECL_SYSCALL(WPROCID(HashNtDll,"NtMapViewOfSection"),         NT::NtMapViewOfSection,         NtMapViewOfSection         )
DECL_SYSCALL(WPROCID(HashNtDll,"NtUnmapViewOfSection"),       NT::NtUnmapViewOfSection,       NtUnmapViewOfSection       )
DECL_SYSCALL(WPROCID(HashNtDll,"NtCreateSection"),            NT::NtCreateSection,            NtCreateSection            )
DECL_SYSCALL(WPROCID(HashNtDll,"NtOpenSection"),              NT::NtOpenSection,              NtOpenSection              )
DECL_SYSCALL(WPROCID(HashNtDll,"NtQuerySection"),             NT::NtQuerySection,             NtQuerySection             )

DECL_SYSCALL(WPROCID(HashNtDll,"NtCreateSymbolicLinkObject"), NT::NtCreateSymbolicLinkObject, NtCreateSymbolicLinkObject )
DECL_SYSCALL(WPROCID(HashNtDll,"NtOpenSymbolicLinkObject"),   NT::NtOpenSymbolicLinkObject,   NtOpenSymbolicLinkObject   )
DECL_SYSCALL(WPROCID(HashNtDll,"NtQuerySymbolicLinkObject"),  NT::NtQuerySymbolicLinkObject,  NtQuerySymbolicLinkObject  )
DECL_SYSCALL(WPROCID(HashNtDll,"NtQueryInformationProcess"),  NT::NtQueryInformationProcess,  NtQueryInformationProcess  )

DECL_SYSCALL(WPROCID(HashNtDll,"NtClose"),                    NT::NtClose,                    NtClose                    )
                                                                                                                               
DECL_SYSCALL(WPROCID(HashNtDll,"NtDelayExecution"),           NT::NtDelayExecution,           NtDelayExecution           )
DECL_SYSCALL(WPROCID(HashNtDll,"NtCreateThread"),             NT::NtCreateThread,             NtCreateThread             )
DECL_SYSCALL(WPROCID(HashNtDll,"NtCreateProcess"),            NT::NtCreateProcess,            NtCreateProcess            )   // Use NtCreateProcessEx instead?
DECL_SYSCALL(WPROCID(HashNtDll,"NtCreateProcessEx"),          NT::NtCreateProcessEx,          NtCreateProcessEx          )
DECL_SYSCALL(WPROCID(HashNtDll,"NtResumeThread"),             NT::NtResumeThread,             NtResumeThread             )
DECL_SYSCALL(WPROCID(HashNtDll,"NtSuspendThread"),            NT::NtSuspendThread,            NtSuspendThread            )
DECL_SYSCALL(WPROCID(HashNtDll,"NtTerminateThread"),          NT::NtTerminateThread,          NtTerminateThread          )
DECL_SYSCALL(WPROCID(HashNtDll,"NtTerminateProcess"),         NT::NtTerminateProcess,         NtTerminateProcess         )
DECL_SYSCALL(WPROCID(HashNtDll,"NtWaitForSingleObject"),      NT::NtWaitForSingleObject,      NtWaitForSingleObject      )

DECL_SYSCALL(WPROCID(HashNtDll,"NtLoadDriver"),               NT::NtLoadDriver,               NtLoadDriver               )
DECL_SYSCALL(WPROCID(HashNtDll,"NtUnloadDriver"),             NT::NtUnloadDriver,             NtUnloadDriver             )   // Should be last

DECL_SYSCALL(WPROCID(HashNtDll,"NtFsControlFile"),            NT::NtFsControlFile,            NtFsControlFile            )   // FSCTL_XXX
DECL_SYSCALL(WPROCID(HashNtDll,"NtDeviceIoControlFile"),      NT::NtDeviceIoControlFile,      NtDeviceIoControlFile      )   // IOCTL_XXX  

} static constexpr inline SysApi alignas(16);   // Declared to know exact address(?), its size is ALWAYS 1   // Volatile? (static volatile constexpr inline)
//============================================================================================================
#include "../UtilsFmtPE.hpp"
#include "../NtDllEx.hpp"
//============================================================================================================
struct NAPI   // On NIX all syscall stubs will be in NAPI   // uwin-master
{
//#include "../../UtilsNAPI.hpp"

FUNC_WRAPPERFI(PX::exit,       exit       ) { SAPI::NtTerminateThread(NT::NtCurrentThread, (uint32)GetParFromPk<0>(args...)); }
FUNC_WRAPPERFI(PX::exit_group, exit_group ) { SAPI::NtTerminateProcess(NT::NtCurrentProcess, (uint32)GetParFromPk<0>(args...)); }
FUNC_WRAPPERNI(PX::cloneB0,    clone      ) {return 0;}
FUNC_WRAPPERNI(PX::fork,       fork       ) {return 0;}
FUNC_WRAPPERNI(PX::vfork,      vfork      ) {return 0;}
FUNC_WRAPPERNI(PX::execve,     execve     ) {return 0;}

FUNC_WRAPPERNI(PX::wait4,      wait       ) {return 0;}
FUNC_WRAPPERNI(PX::gettid,     gettid     ) {return 0;}
FUNC_WRAPPERNI(PX::getpid,     getpid     ) {return 0;}
FUNC_WRAPPERNI(PX::getppid,    getppid    ) {return 0;}
FUNC_WRAPPERNI(PX::getpgrp,    getpgrp    ) {return 0;}
FUNC_WRAPPERNI(PX::getpgid,    getpgid    ) {return 0;}
FUNC_WRAPPERNI(PX::setpgid,    setpgid    ) {return 0;}
//------------------------------------------------------------------------------------------------------------
/*
https://man7.org/linux/man-pages/man2/mmap.2.html

https://stackoverflow.com/questions/21311080/linux-shared-memory-shmget-vs-mmap

On OSX you want mmap as the max shared memory with shmget is only 4mb across all processes
memory mapped with MAP_ANONYMOUS is NOT backed by a file
some implementations require fd to be -1 if MAP_ANONYMOUS (or MAP_ANON) is specified, and portable applications should ensure this
The use of MAP_ANONYMOUS in conjunction with MAP_SHARED is supported on Linux only since kernel 2.4
NOTE: On Windows all requested memory required t be available (mmap+mlock), MAP_LOCKED will not report ENOMEM as well as MAP_POPULATE

TODO: MEM_PHYSICAL, MEM_LARGE_PAGES, MEM_4MB_PAGES
 MEM_TOP_DOWN
 MEM_PRIVATE
 MEM_MAPPED
 MEM_COMMIT
 MEM_RESERVE

 VirtualAlloc and VirtualAllocEx cannot allocate non-private (shared) memory.

NOTE: Never reserve less memory than Allocation Granularity(64k) or there will be a hole in the address space, unavaliable for further allocations(enlarging of the same block)

NOTE: No way to return an actual allocated region size using only 'mmap' format!

If the memory is being reserved, the specified address is rounded down to the nearest multiple of the allocation granularity.
If the memory is already reserved and is being committed, the address is rounded down to the next page boundary.

 We must implement single mmap call as mem_reserve and mem_commit operations
 mem_commit works same as on Linux, allocating memory by page granularity but can make inaccessible memory holes if not reserved first as 64k block aligned
 On Linux, memory is always behave like reserved and committed at the same time

 MEM_RESERVE will not align Size to 64K, only base address. Both MEM_RESERVE and MEM_COMMIT will align Size to 4k

*/
FUNC_WRAPPERNI(PX::mmapGD,     mmap       )
{
 const vptr   addr     = (vptr)GetParFromPk<0>(args...);
 const size_t length   = GetParFromPk<1>(args...);
       uint32 prot     = GetParFromPk<2>(args...);
 const uint   flags    = GetParFromPk<3>(args...);
       NT::HANDLE  fd  = GetParFromPk<4>(args...);    // TODO: File mappings
       sint64 pgoffset = GetParFromPk<5>(args...);    // Actually should be aligned to 64K for compatibility woth Windows (Enforce on Linux?)

 if(!(flags & (PX::MAP_PRIVATE|PX::MAP_SHARED)))return vptr(-PX::EINVAL);   // One of MAP_SHARED or MAP_PRIVATE must be specified.   // MAP_SHARED: Write changes back to a mapped file (It must be opened as writable)
 if((flags & (PX::MAP_PRIVATE|PX::MAP_SHARED)) == (PX::MAP_PRIVATE|PX::MAP_SHARED))return vptr(-PX::EINVAL);  // ???
 if(pgoffset & (MEMPAGESIZE-1))return vptr(-PX::EINVAL);  // offset must be a multiple of the page size as returned by sysconf(_SC_PAGE_SIZE)  // TODO: Check if it actually reports an error
 if((flags & PX::MAP_ANONYMOUS)&&(fd != NT::NtInvalidHandle))return vptr(-PX::EINVAL);    // For portability
 if(!length)return vptr(-PX::EINVAL);     // Only Windows accepts 0 for entire file mappings

 vptr   RegionBase  = addr;         // Rounded <<<
 NT::NTSTATUS res   = 0;
 if((flags & PX::MAP_ANONYMOUS) && (flags & PX::MAP_PRIVATE))   // Allocate simple private virtual memory      // MAP_ANONYMOUS: No file is used    // MAP_PRIVATE: Changes are privaqte
  {
   size_t RegionSize  = AlignP2Frwd(length, MEMGRANSIZE);       // Rounded >>>
   uint32 AllocProt   = NTX::MemProtPXtoNT(prot);
   if(flags & PX::MAP_NOCACHE)AllocProt |= NT::PAGE_NOCACHE;    // ???
   res = SAPI::NtAllocateVirtualMemory(NT::NtCurrentProcess, &RegionBase, 0, &RegionSize, NT::MEM_RESERVE, AllocProt);  // First reserve with 64k granularity to avoid memory holes (allowed to fail if already has been done)    (Will cause memory waste in Working Set?)
   if(res)return (vptr)-NTX::NTStatusToLinuxErr(res);   // What actual error codes could be?   (If already reserved?)
   RegionBase = addr;     // Reset the base and size
   RegionSize = length;   // Now 4k alignment is OK
   res = SAPI::NtAllocateVirtualMemory(NT::NtCurrentProcess, &RegionBase, 0, &RegionSize, NT::MEM_COMMIT, AllocProt);  // Then commit with 4k granularity (page size)
   if(!res)return RegionBase;
   return (vptr)-NTX::NTStatusToLinuxErr(res);    // ???        // GetMMapErrFromPtr
  }
  
 NT::HANDLE hSec = NT::NtInvalidHandle;
 NT::OBJECT_ATTRIBUTES oattr;        

 oattr.Length = sizeof(NT::OBJECT_ATTRIBUTES);
 oattr.RootDirectory = 0;//NT::NtInvalidHandle;
 oattr.ObjectName = nullptr;        // Unnamed 
 oattr.Attributes = 0;       
 oattr.SecurityDescriptor = nullptr;         
 oattr.SecurityQualityOfService = nullptr;

 if(prot & PX::PROT_WRITE)    // Hints for MemProtPXtoNT
  {
   if(flags & PX::MAP_SHARED)prot |= PX::PROT_READ;  // To avoid copy-on-write for the mapping
     else prot &= ~PX::PROT_READ;     // To ensure copy-on-write for the mapping
  }
 uint32 AllocProt = NTX::MemProtPXtoNT(prot);
 if(flags & PX::MAP_NOCACHE)AllocProt |= NT::PAGE_NOCACHE;    // ???
 sint64 RegionSize;
 if(fd == NT::NtInvalidHandle){RegionSize = AlignP2Frwd(length, MEMGRANSIZE); fd = 0;}  // Unused fd for NtCreateSection is 0 (STATUS_OBJECT_TYPE_MISMATCH if -1)
  else RegionSize = length;

 uint32 SecRights = +NT::SYNCHRONIZE|NT::SECTION_QUERY|NT::STANDARD_RIGHTS_REQUIRED;        // NOTE: Untested
 if(prot & PX::PROT_EXEC)SecRights |= +NT::GENERIC_EXECUTE|NT::SECTION_MAP_EXECUTE;
 if(prot & PX::PROT_READ)SecRights |= +NT::GENERIC_READ|NT::SECTION_MAP_READ;
 if(prot & PX::PROT_WRITE)SecRights |= +NT::GENERIC_WRITE|NT::SECTION_MAP_WRITE|NT::SECTION_EXTEND_SIZE;

 uint32 SecAttrs = NT::SEC_COMMIT;
 if(flags & PX::MAP_HUGETLB)SecAttrs |= NT::SEC_LARGE_PAGES;
 if(flags & PX::MAP_NOCACHE)SecAttrs |= NT::SEC_NOCACHE;

 res = SAPI::NtCreateSection(&hSec,       // TODO: Call shm_open with SHM_ANON
   SecRights,
   &oattr,
   &RegionSize,  // rounds this value up to the nearest multiple of PAGE_SIZE
   AllocProt,
   SecAttrs,
   fd);           // The handle may be 0 to create a memory section, or a file handle to create a file mapping or a section handle from some named section create/open syscall (In this case the call will fail and it is OK)
 if(res) 
  {
   if(res != NT::STATUS_OBJECT_TYPE_MISMATCH)return (vptr)-NTX::NTStatusToLinuxErr(res);    // Allow to fail if a section handle is passed instead of a file handle (From shmget)
   hSec = fd;
  }
 RegionSize = length;
 res = SAPI::NtMapViewOfSection(hSec,
   NT::NtCurrentProcess,
   &RegionBase, // the specified virtual address rounded down to the next 64-kilobyte address boundary
   0,           // ZeroBits
   RegionSize,  // CommitSize is meaningful only for page-file backed sections and is rounded up to the nearest multiple of PAGE_SIZE.
   &pgoffset,   // If this pointer is not NULL, the offset is rounded down to the next allocation-granularity size boundary
   (NT::PSIZE_T)&RegionSize, // If the initial value of this variable is zero, ZwMapViewOfSection maps a view of the section that starts at SectionOffset and continues to the end of the section. Otherwise, the initial value specifies the view's size, in bytes. ZwMapViewOfSection always rounds this value up to the nearest multiple of PAGE_SIZE before mapping the view.
   ((flags & PX::MAP_ANONYMOUS)&&(flags & PX::MAP_SHARED))?(NT::ViewShare):(NT::ViewUnmap),   // Can share with child processes
   0,
   AllocProt);
 SAPI::NtClose(hSec);    // The section handle is not needed anymore and should no t be leaked
 if(res)return (vptr)-NTX::NTStatusToLinuxErr(res);  
 return RegionBase; 
}
//------------------------------------------------------------------------------------------------------------
/*
 The address addr must be a multiple of the page size (but length does not)
 If the MEM_RELEASE flag is set in the FreeType parameter, BaseAddress must be the base address returned by NtAllocateVirtualMemory when the region was reserved.
 If the MEM_RELEASE flag is set in the FreeType parameter, the variable pointed to by RegionSize must be zero.
     NtFreeVirtualMemory frees the entire region that was reserved in the initial allocation call to NtAllocateVirtualMemory.
 NtFreeVirtualMemory does not fail if you attempt to release pages that are in different states, some reserved and some committed
 NOTE: Ideally we must RELEASE only if the Size covers the entire region (But we have to use QueryVirtualMemory to know that)
*/
//------------------------------------------------------------------------------------------------------------
FUNC_WRAPPERNI(PX::munmap,     munmap     )                // ZwUnmapViewOfSectionEx  // TODO: deallocate in loop to cover entire range in case of mremap was used (Cannot free several allocations at once on Windows)
{
 const vptr   addr   = (vptr)GetParFromPk<0>(args...);
 const size_t length = GetParFromPk<1>(args...);

 vptr   RegionBase   = addr;    // Rounded <<<
 size_t RegionSize   = 0;       // Rounded >>>   // If the dwFreeType parameter is MEM_RELEASE, this parameter must be 0

 NT::NTSTATUS res = SAPI::NtFreeVirtualMemory(NT::NtCurrentProcess, &RegionBase, &RegionSize, NT::MEM_RELEASE);   // Releases the entire region but only if addr is the same base address that came from mmap (in most cases this is what you do)
 if(!res)return PX::NOERROR;  // The entire allocated region is free now  
 if(res == NT::STATUS_UNABLE_TO_DELETE_SECTION){res = SAPI::NtUnmapViewOfSection(NT::NtCurrentProcess, RegionBase); if(!res)return PX::NOERROR;}  // This region is a mapped section  // NOTE: The entire region will e unmapped

 RegionBase = addr;
 RegionSize = length;
 res = SAPI::NtFreeVirtualMemory(NT::NtCurrentProcess, &RegionBase, &RegionSize, NT::MEM_DECOMMIT);   // Addr is not region base addr, try at least to decommit some pages (Don`t even try to base your memory manager on this behavior!)
 if(!res)return PX::NOERROR;
 return -(sint)NTX::NTStatusToLinuxErr(res);
}
//------------------------------------------------------------------------------------------------------------
// https://stackoverflow.com/questions/17197615/no-mremap-for-windows
// Can extend only if there are some reserved pages at the end
// NOTE: None of this is tested appropriately
// What about mlock?
//
FUNC_WRAPPERNI(PX::mremap,     mremap     )   // LINUX specific  // Impossible to make actual remapping with a general memory 
{
 const size_t old_address = (size_t)GetParFromPk<0>(args...);
 const size_t old_size    = GetParFromPk<1>(args...);
 const size_t new_size    = GetParFromPk<2>(args...);
       int    flags       = GetParFromPk<3>(args...);
       size_t new_address = (size_t)GetParFromPk<4>(args...);

 if(old_address & (NPTM::MEMPAGESIZE-1))return vptr(-PX::EINVAL); 
 if(flags & PX::MREMAP_FIXED)
  {
   if(!(flags & PX::MREMAP_MAYMOVE))return vptr(-PX::EINVAL); 
   if(new_address & (NPTM::MEMPAGESIZE-1))return vptr(-PX::EINVAL);   
   if(IsRangesIntersect(old_address,old_size, new_address,new_size))return vptr(-PX::EINVAL);
  }
 NT::MEMORY_BASIC_INFORMATION mbi;
 NT::NTSTATUS res = SAPI::NtQueryVirtualMemory(NT::NtCurrentProcess, (vptr)old_address, NT::MemoryBasicInformation, &mbi, sizeof(mbi), nullptr);
 if(res)return vptr(-PX::EFAULT);    // old_address is invalid
 if(mbi.Type != NT::MEM_PRIVATE)return vptr(-PX::EPERM);  // It is impossible to remap a file mapping on Windows by having only a memory pointer to it.

 vptr   nptr = nullptr;
 size_t al_new_size = AlignP2Frwd(new_size, MEMGRANSIZE);       // Rounded >>>
 if(!(flags & PX::MREMAP_FIXED))   // Try to expand/shrink? Shrinking is safe(realign and make remaining pages reserved) Expanding may overlap already committed pages(But only in current allocation block), only sequential expanding is OK.         
  {
   vptr   RegionBase = (vptr)old_address;     
   size_t RegionSize = new_size;          // Now 4k alignment is OK
   res = SAPI::NtAllocateVirtualMemory(NT::NtCurrentProcess, &RegionBase, 0, &RegionSize, NT::MEM_COMMIT, mbi.Protect);  
   if(res)      // Failed to resize
    {
     if(!(flags & PX::MREMAP_MAYMOVE))return nptr;  // Forbidden to relocate   // Try to expand, move if fails at unk addr
     flags |= PX::MREMAP_FIXED;   // Try allocation in a new block
     new_address = 0;             //  On a system-provided addr
    }
     else   // Resized inplace, shrinked. Must decommit the discarded pages
      {
       size_t aos = AlignP2Frwd(old_size, MEMPAGESIZE); 
       if(RegionSize < aos)
        {
         RegionBase = (vptr)((size_t)RegionBase + RegionSize);  
         RegionSize = aos - RegionSize;
         res = SAPI::NtFreeVirtualMemory(NT::NtCurrentProcess, &RegionBase, &RegionSize, NT::MEM_DECOMMIT);
         //if(res)  ?????????
        }
      }
  }

 if(flags & PX::MREMAP_FIXED)    // In a different region, at exact address
  {
   vptr   RegionBase  = (vptr)new_address;
   size_t RegionSize  = al_new_size;
   res = SAPI::NtAllocateVirtualMemory(NT::NtCurrentProcess, &RegionBase, 0, &RegionSize, NT::MEM_RESERVE, mbi.Protect);  // First reserve with 64k granularity to avoid memory holes (allowed to fail if already has been done)    (Will cause memory waste in Working Set?)
   if(res)return (vptr)-NTX::NTStatusToLinuxErr(res);   // What actual error codes could be?   (If already reserved?)
   RegionBase = (vptr)new_address;       // Reset the base and size
   RegionSize = new_size;   // Now 4k alignment is OK
   res = SAPI::NtAllocateVirtualMemory(NT::NtCurrentProcess, &RegionBase, 0, &RegionSize, NT::MEM_COMMIT, mbi.Protect);  // Then commit with 4k granularity (page size)
   if(res)return (vptr)-NTX::NTStatusToLinuxErr(res); 
  }
   
 NMOPS::MemCopy(nptr, (vptr)old_address, old_size);
 res = NAPI::munmap((vptr)old_address, old_size);    // If release fails, tries decommit    // TODO: Check for address space leaks
 //if(res)  ?????????
 return nptr;
}    
//------------------------------------------------------------------------------------------------------------
FUNC_WRAPPERNI(PX::madvise,    madvise    )
{
    // instead of unmapping the address, we're just gonna trick
    // the TLB to mark this as a new mapped area which, due to
    // demand paging, will not be committed until used.

 //   mmap(addr, size, PROT_NONE, MAP_FIXED|MAP_PRIVATE|MAP_ANON, -1, 0);
 //   msync(addr, size, MS_SYNC|MS_INVALIDATE);
 return 0;
}
//------------------------------------------------------------------------------------------------------------
FUNC_WRAPPERNI(PX::mprotect,   mprotect   ) {return 0;}
FUNC_WRAPPERNI(PX::msync,      msync      ) {return 0;}    // NtFlushVirtualMemory  (FlushViewOfFile)
FUNC_WRAPPERNI(PX::mlock,      mlock      ) {return 0;}    // NtLockVirtualMemory
FUNC_WRAPPERNI(PX::munlock,    munlock    ) {return 0;}    // NtUnlockVirtualMemory
//------------------------------------------------------------------------------------------------------------
FUNC_WRAPPERNI(PX::close,      close      ) {return SAPI::NtClose((NT::HANDLE)GetParFromPk<0>(args...));}
//------------------------------------------------------------------------------------------------------------
FUNC_WRAPPERFI(PX::read,       read       )
{
 NT::IO_STATUS_BLOCK iosb = {};
 const NT::HANDLE hnd = (NT::HANDLE)GetParFromPk<0>(args...);
 const vptr   buf = (vptr)GetParFromPk<1>(args...);
 const size_t len = GetParFromPk<2>(args...);
 NT::NTSTATUS res = SAPI::NtReadFile(hnd, 0, nullptr, nullptr, &iosb, buf, (uint32)len, nullptr, nullptr);  // Relative to current file position
 if(!res)return (ssize_t)iosb.Information;    // Number of bytes read
 return -(sint)NTX::NTStatusToLinuxErr(res);
}
//------------------------------------------------------------------------------------------------------------
FUNC_WRAPPERFI(PX::write,      write      )
{
 NT::IO_STATUS_BLOCK iosb = {};
 const NT::HANDLE hnd = (NT::HANDLE)GetParFromPk<0>(args...);
 const vptr   buf = (vptr)GetParFromPk<1>(args...);
 const size_t len = GetParFromPk<2>(args...);
 NT::NTSTATUS res = SAPI::NtWriteFile(hnd, 0, nullptr, nullptr, &iosb, buf, (uint32)len, nullptr, nullptr);  // Relative to current file position
 if(!res)return (ssize_t)iosb.Information;    // Number of bytes written
 return -(sint)NTX::NTStatusToLinuxErr(res);
}
//------------------------------------------------------------------------------------------------------------
FUNC_WRAPPERNI(PX::readv,      readv      ) {return 0;}
FUNC_WRAPPERNI(PX::writev,     writev     ) {return 0;}
//------------------------------------------------------------------------------------------------------------
// Upon successful completion, lseek() returns the resulting offset location as measured in bytes from the beginning of the file.
FUNC_WRAPPERNI(PX::lseekGD,    lseek      )
{
 NT::IO_STATUS_BLOCK iosb = {};
 NT::FILE_POSITION_INFORMATION Pos;

 const NT::HANDLE hnd = (NT::HANDLE)GetParFromPk<0>(args...);
 int64 offset         = GetParFromPk<1>(args...);
 const int whence     = GetParFromPk<2>(args...);
 if(whence > 2)return -PX::EINVAL;

 if(whence == PX::SEEK_END)
  {
   NT::FILE_STANDARD_INFORMATION Inf;
   NT::NTSTATUS res = SAPI::NtQueryInformationFile(hnd, &iosb, &Inf, sizeof(Inf), NT::FileStandardInformation);
   if(res)return -NTX::NTStatusToLinuxErr(res);
   offset += Inf.EndOfFile;
  }
 else if(whence == PX::SEEK_CUR)
  {
   NT::NTSTATUS res = SAPI::NtQueryInformationFile(hnd, &iosb, &Pos, sizeof(Pos), NT::FilePositionInformation);
   if(res)return -NTX::NTStatusToLinuxErr(res);
   if(!offset)return Pos.CurrentByteOffset;        // Just return the current position (offset=0, whence=SEEK_CUR)
   offset += Pos.CurrentByteOffset;
  }
// whence is SEEK_SET
 if(offset < 0)return -PX::EINVAL;
 Pos.CurrentByteOffset = offset;
 NT::NTSTATUS res = SAPI::NtSetInformationFile(hnd, &iosb, &Pos, sizeof(Pos), NT::FilePositionInformation);
 if(res)return -NTX::NTStatusToLinuxErr(res);
 return offset;
}
//------------------------------------------------------------------------------------------------------------
FUNC_WRAPPERNI(PX::ftruncate,     ftruncate     ) 
{
 NT::IO_STATUS_BLOCK iosb = {};
 NT::FILE_END_OF_FILE_INFORMATION End;

 const NT::HANDLE hnd = (NT::HANDLE)GetParFromPk<0>(args...);
 uint64 Len = GetParFromPk<1>(args...);

 End.EndOfFile = Len;
 NT::NTSTATUS res = SAPI::NtSetInformationFile(hnd, &iosb, &End, sizeof(End), NT::FileEndOfFileInformation);
 if(res)return -NTX::NTStatusToLinuxErr(res);
 return 0;
}
//------------------------------------------------------------------------------------------------------------
FUNC_WRAPPERNI(PX::truncate,     truncate       ) 
{
 NT::IO_STATUS_BLOCK iosb = {};
 NT::HANDLE FileHandle = 0;
 const achar* path = GetParFromPk<0>(args...);
 uint64 Len = GetParFromPk<1>(args...);
 NT::NTSTATUS res = NTX::OpenFileObject(&FileHandle, path, +NT::SYNCHRONIZE|NT::FILE_WRITE_ATTRIBUTES|NT::FILE_WRITE_DATA, 0, NT::FILE_ATTRIBUTE_NORMAL, NT::FILE_SHARE_READ|NT::FILE_SHARE_WRITE|NT::FILE_SHARE_DELETE, NT::FILE_OPEN, NT::FILE_SYNCHRONOUS_IO_NONALERT, &iosb);
 if(res)return -NTX::NTStatusToLinuxErr(res);  // Can the handle be open? (Status > 0)
 int rs = NAPI::ftruncate((PX::fdsc_t)FileHandle, Len);
 SAPI::NtClose(FileHandle);
 return rs;
}
//------------------------------------------------------------------------------------------------------------
// Complicated
FUNC_WRAPPERNI(PX::mkfifo,     mkfifo     ) {return 0;}
//------------------------------------------------------------------------------------------------------------
// Or use 'open' with O_DIRECTORY instead?
FUNC_WRAPPERNI(PX::mkdir,      mkdir      )
{
 NT::IO_STATUS_BLOCK iosb = {};
 NT::HANDLE FileHandle = 0;
 const achar* path = (achar*)GetParFromPk<0>(args...);
// int mode = GetParFromPk<1>(args...);   // TODO: Mode support

 NT::NTSTATUS res = NTX::OpenFileObject(&FileHandle, path, +NT::SYNCHRONIZE|NT::FILE_READ_ATTRIBUTES, 0, NT::FILE_ATTRIBUTE_NORMAL, NT::FILE_SHARE_READ|NT::FILE_SHARE_WRITE|NT::FILE_SHARE_DELETE, NT::FILE_CREATE, NT::FILE_DIRECTORY_FILE|NT::FILE_SYNCHRONOUS_IO_NONALERT, &iosb);
 if(res)return -(int32)NTX::NTStatusToLinuxErr(res);  // Can the handle be open? (Status > 0)
 SAPI::NtClose(FileHandle);
 return PX::NOERROR;
}
//------------------------------------------------------------------------------------------------------------
FUNC_WRAPPERNI(PX::rmdir,      rmdir      )
{
 // !!!!!!!!!!!!!!!
 return 0;
}
//------------------------------------------------------------------------------------------------------------
// Note: link behaviour on Windows is different
//This means its not sufficient to delete a file, it may not be deleted immediately, and this may cause problems in deleting directories and/or creating a new file of the same name.

//But you can closely simulate unix semantics by renaming the file to a temporary directory and scheduling it for deletion.
// "File System Behavior Overview.pdf"
// If the name referred to a SYMBOLIC link, the link is removed.
//
FUNC_WRAPPERNI(PX::unlink,     unlink     )
{

 // !!!!!!!!!!!!!!!
 return 0;
}
//------------------------------------------------------------------------------------------------------------
FUNC_WRAPPERNI(PX::rename,     rename     ) {return 0;}
//------------------------------------------------------------------------------------------------------------
FUNC_WRAPPERNI(PX::readlink,   readlink   )
{
/* const achar* path = (achar*)GetParFromPk<0>(args...);
 achar* buf = (achar*)GetParFromPk<1>(args...);
 const size_t bufsiz = GetParFromPk<2>(args...);

 NT::OBJECT_ATTRIBUTES oattr = {};
 NT::UNICODE_STRING FilePathUS;               // RtlAcquirePrivilege(&v26, 1i64, 0i64, &v29);

 uint plen;
 NTX::EPathType ptype;
 uint PathLen = NTX::CalcFilePathBufSize(path, plen, ptype);
 uint32 ObjAttributes = 0;  //NT::OBJ_OPENLINK;     // Only Hard Links to a file can be safely deleted without this
 wchar FullPath[PathLen];
 NTX::InitFileObjectAttributes(path, plen, ptype, ObjAttributes, &FilePathUS, FullPath, &oattr);

 NT::HANDLE LinkHandle = 0;
 NT::NTSTATUS res = SAPI::NtOpenSymbolicLinkObject(&LinkHandle, NT::GENERIC_READ, &oattr);
 if(res)return -NTX::NTStatusToLinuxErr(res);
*/

 return 0;
}
//------------------------------------------------------------------------------------------------------------
// Compatibility?
FUNC_WRAPPERNI(PX::access,     access     )
{
 NT::IO_STATUS_BLOCK iosb = {};
 NT::HANDLE FileHandle = 0;
 const achar* path = (achar*)GetParFromPk<0>(args...);
 int mode = GetParFromPk<1>(args...);

 uint32 BaseAccess = +NT::SYNCHRONIZE|NT::FILE_READ_ATTRIBUTES;   // F_OK
 if(mode & PX::X_OK)BaseAccess |= NT::FILE_EXECUTE;    // Close enough?
 if(mode & PX::W_OK)BaseAccess |= NT::FILE_WRITE_DATA;
 if(mode & PX::R_OK)BaseAccess |= NT::FILE_READ_DATA;
 NT::NTSTATUS res = NTX::OpenFileObject(&FileHandle, path, BaseAccess, 0, NT::FILE_ATTRIBUTE_NORMAL, NT::FILE_SHARE_READ|NT::FILE_SHARE_WRITE|NT::FILE_SHARE_DELETE, NT::FILE_OPEN, NT::FILE_SYNCHRONOUS_IO_NONALERT, &iosb);
 if(res)return -(int32)NTX::NTStatusToLinuxErr(res);  // Can the handle be open? (Status > 0)
 SAPI::NtClose(FileHandle);
 return PX::NOERROR;
}
//------------------------------------------------------------------------------------------------------------
// NOTE: The buffer size is abstract and number of entries returned will depend on a platform and underlying file system
// When the NtQueryDirectoryFile routine is called for a particular handle, the RestartScan parameter is treated as if it were set to TRUE, regardless of its value. On subsequent NtQueryDirectoryFile calls, the value of the RestartScan parameter is honored.
// https://www.boost.org/doc/libs/1_83_0/libs/filesystem/src/directory.cpp
// The buffer cannot be larger than 64k, because up to Windows 8.1, NtQueryDirectoryFile and GetFileInformationByHandleEx fail with ERROR_INVALID_PARAMETER when trying to retrieve the filenames from a network share
// Can the directory offset be accessed with NtQueryInformationFile and NtSetInformationFile ?
// Is '.' and '..' directories always come first on Windows? // On Linux they come first OR last
// Opened directory happened to be locked from file deletion : NPTM::NAPI::open("", PX::O_DIRECTORY|PX::O_RDONLY, 0666);
// On Linux DT_LNK type is set for both file and directory links (Looks like FS itself is not aware to what the links points to when reads its object)  // It is inconvenient but have to be implemented for Windows in similair way (Will have to call 'stat' on any DT_LNK)
//
FUNC_WRAPPERNI(PX::getdentsGD,     getdents     )
{
 NT::IO_STATUS_BLOCK iosb = {};
 NT::HANDLE   hnd = (NT::HANDLE)GetParFromPk<0>(args...);
 const vptr   buf = (vptr)GetParFromPk<1>(args...);
 size_t len = GetParFromPk<2>(args...);
 bool  NoLinks = false;     // Retrieve real info about links (file/dir)
 if((ssize_t)len < 0){len = size_t(-(ssize_t)len); NoLinks = true;}    // FRMWK extension
 NT::NTSTATUS res = SAPI::NtQueryDirectoryFile(hnd, 0, nullptr, nullptr, &iosb, buf, (uint32)len, NT::FileDirectoryInformation, 0, nullptr, 0);
 if(res)
  {
   if(res == NT::STATUS_NO_MORE_FILES)return 0;
   else if(res == NT::STATUS_BUFFER_OVERFLOW)return PX::EINVAL;     // Only first call could return this  // Only if fixed portion of FILE_XXX_INFORMATION doesn`t fit in the buffer
   else return -(int32)NTX::NTStatusToLinuxErr(res);
  }
 uint TotalBytes = iosb.Information;
 if(!TotalBytes)return PX::EINVAL;  // The buffer is too small
 uint InOffs  = 0;
 uint OutOffs = 0;
 for(;;)   // All converted records are expected to fit because they are smaller
  {
   PX::SDirEnt* outrec = (PX::SDirEnt*)((uint8*)buf + OutOffs);
   NT::FILE_DIRECTORY_INFORMATION* inrec = (NT::FILE_DIRECTORY_INFORMATION*)((uint8*)buf + InOffs);
   uint NxtOffs = inrec->NextEntryOffset;
   uint32 Attrs = inrec->FileAttributes;
   outrec->ino  = inrec->FileIndex;  // Looks like it is always 0   // Actual inode is probably in FILE_ID_FULL_DIR_INFORMATION::FileId
   outrec->off  = inrec->EndOfFile;  // Who cares
   size_t nlen  = NUTF::Utf16To8(outrec->name, inrec->FileName, inrec->FileNameLength >> 1);   // Will be smaller     // FileNameLength is in bytes
   uint offs    = AlignP2Frwd(nlen + sizeof(PX::SDirEnt), sizeof(vptr));
   outrec->name[nlen] = 0;
   outrec->reclen = (uint16)offs;
   OutOffs += offs;

   if(NoLinks)
    {
   if(Attrs & NT::FILE_ATTRIBUTE_DIRECTORY)outrec->type = PX::DT_DIR;            // Put first in case it somehow happen to be together with FILE_ATTRIBUTE_NORMAL
   else if(Attrs & (NT::FILE_ATTRIBUTE_NORMAL|NT::FILE_ATTRIBUTE_ARCHIVE))outrec->type = PX::DT_REG;  // FILE_ATTRIBUTE_NORMAL means no other attributes  // For files FILE_ATTRIBUTE_ARCHIVE is valid too and replaces FILE_ATTRIBUTE_NORMAL
   else if(Attrs & NT::FILE_ATTRIBUTE_REPARSE_POINT)outrec->type = PX::DT_LNK;   // Overrides anything else on Linux        // Directory and file symlinks (mklink /D (not mklink /H)) will have FILE_ATTRIBUTE_REPARSE_POINT
   else if(Attrs & NT::FILE_ATTRIBUTE_DEVICE)outrec->type = PX::DT_CHR;          // DT_CHR is more likely than a DT_BLK     // Reserved for system use anyway
   else outrec->type = PX::DT_UNKNOWN;
    }
    else
     {
   if(Attrs & NT::FILE_ATTRIBUTE_REPARSE_POINT)outrec->type = PX::DT_LNK;        // Overrides anything else on Linux        // Directory and file symlinks (mklink /D (not mklink /H)) will have FILE_ATTRIBUTE_REPARSE_POINT
   else if(Attrs & NT::FILE_ATTRIBUTE_DIRECTORY)outrec->type = PX::DT_DIR;       // Put first in case it somehow happen to be together with FILE_ATTRIBUTE_NORMAL
   else if(Attrs & (NT::FILE_ATTRIBUTE_NORMAL|NT::FILE_ATTRIBUTE_ARCHIVE))outrec->type = PX::DT_REG;  // FILE_ATTRIBUTE_NORMAL means no other attributes  // For files FILE_ATTRIBUTE_ARCHIVE is valid too and replaces FILE_ATTRIBUTE_NORMAL
   else if(Attrs & NT::FILE_ATTRIBUTE_DEVICE)outrec->type = PX::DT_CHR;          // DT_CHR is more likely than a DT_BLK     // Reserved for system use anyway
   else outrec->type = PX::DT_UNKNOWN;
     }
   if(!NxtOffs)break;
   InOffs += NxtOffs;
  }
 return (int)OutOffs;
}
//------------------------------------------------------------------------------------------------------------
/*
FILE_STANDARD_INFORMATION   // https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/ns-wdm-_file_standard_information
FILE_BASIC_INFORMATION      // https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/ns-wdm-_file_basic_information
FILE_ALL_INFORMATION        // https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/ns-ntifs-_file_all_information

Time values CreationTime, LastAccessTime, LastWriteTime, and ChangeTime are expressed in absolute system time format.
Absolute system time is the number of 100-nanosecond intervals since the start of the year 1601 in the Gregorian calendar.

Linux timestamps is the number of seconds since the Unix epoch, which was midnight (00:00:00) on January 1, 1970, in Coordinated Universal Time (UTC).
Leap seconds are ignored in Linux timestamps, so they aren’t analogous to real time.

https://github.com/chakra-core/ChakraCore/blob/master/pal/src/file/filetime.cpp

Win32 LastAccessTime is updated after a write operation, but it is not on Unix.

Linux: No permissions are required on the file itself, but-in the case of stat()
*/
FUNC_WRAPPERNI(PX::fstatat,       fstatat       )       // TODO: AT_SYMLINK_FOLLOW ?      // Flags are ignored for now
{
 NT::IO_STATUS_BLOCK iosb = {};
 NT::HANDLE DirHandle = 0;
 NT::HANDLE FileHandle = 0;
 PX::fdsc_t dirfd  = GetParFromPk<0>(args...);     // How to process AT_FDCWD?   // Add the CWD to the name if it is relative or open the CWD and use its handle? // Is it by default on Windows?  // If pathname is absolute, then dirfd is ignored
 const achar* path = (achar*)GetParFromPk<1>(args...);
 PX::SFStat* sti = (PX::SFStat*)GetParFromPk<2>(args...);
 if(dirfd >= 0)DirHandle = (NT::HANDLE)dirfd;
 NT::NTSTATUS res = NTX::OpenFileObject(&FileHandle, path, +NT::SYNCHRONIZE|NT::FILE_READ_ATTRIBUTES, 0, NT::FILE_ATTRIBUTE_NORMAL, NT::FILE_SHARE_READ|NT::FILE_SHARE_WRITE|NT::FILE_SHARE_DELETE, NT::FILE_OPEN, NT::FILE_SYNCHRONOUS_IO_NONALERT, &iosb, DirHandle);
 if(res)return -NTX::NTStatusToLinuxErr(res);  // Can the handle be open? (Status > 0)
 int rs = NAPI::fstat((PX::fdsc_t)FileHandle, sti);
 SAPI::NtClose(FileHandle);
 return rs;
}
//------------------------------------------------------------------------------------------------------------
FUNC_WRAPPERNI(PX::stat,       stat       )
{
 return NAPI::fstatat(PX::AT_FDCWD, args..., 0);
}
//------------------------------------------------------------------------------------------------------------
FUNC_WRAPPERNI(PX::fstat,      fstat      )
{
 NT::IO_STATUS_BLOCK iosb = {};
 NT::FILE_ALL_INFORMATION inf = {};
 NT::HANDLE  hnd = (NT::HANDLE)GetParFromPk<0>(args...);
 PX::SFStat* sti = (PX::SFStat*)GetParFromPk<1>(args...);
 NT::NTSTATUS res = SAPI::NtQueryInformationFile(hnd, &iosb, &inf, sizeof(inf), NT::FileAllInformation);
 if(res && (res != NT::STATUS_BUFFER_OVERFLOW))return -NTX::NTStatusToLinuxErr(res);

 sti->dev     = 0;  // TODO: How?
 sti->ino     = (uint64)inf.InternalInformation.IndexNumber;   // ???
 sti->nlink   = inf.StandardInformation.NumberOfLinks;
 sti->mode    = 0;
 sti->uid     = 0;  // Requires ACL read
 sti->gid     = 0;  // Requires ACL read
 sti->rdev    = 0;  // ???
 sti->size    = inf.StandardInformation.EndOfFile;
 sti->blksize = 0;  // TODO: From AlignmentInformation.AlignmentRequirement somehow
 sti->blocks  = uint64(inf.StandardInformation.AllocationSize / 512);  // AlignP2Frwd(sti->size,512) / 512;   // Number 512-byte blocks allocated.

 if(inf.StandardInformation.Directory)sti->mode |= PX::S_IFDIR;
   else sti->mode |= PX::S_IFREG;   // Anything else?    // How to get rwe flags of a file?

 sti->atime.sec = NDT::FileTimeToUnixTime((uint64)inf.BasicInformation.LastAccessTime, &sti->atime.nsec);
 sti->mtime.sec = NDT::FileTimeToUnixTime((uint64)inf.BasicInformation.LastWriteTime, &sti->mtime.nsec);
 sti->ctime.sec = NDT::FileTimeToUnixTime((uint64)inf.BasicInformation.CreationTime, &sti->ctime.nsec);  // On Unix creation time is not stored (only: access, modification and change) // Last inode change time is close enough    // xstat ?

 return PX::NOERROR;
}
//------------------------------------------------------------------------------------------------------------
/*
 https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-zwcreatefile

 https://learn.microsoft.com/en-us/dotnet/standard/io/file-path-formats

 On Linux, absolute path('/') is relative to root directory
 On Windows such path is relative from the root of the current drive

 NtCreateFile works much similair to Linux and nave access to special directories

 https://learn.microsoft.com/en-us/windows-hardware/drivers/kernel/using-files-in-a-driver

 On Microsoft Windows 2000 and later versions of the operating system, \?? is equivalent to \DosDevices.
 Now \DosDevices is a symbolic link to \?? which is in object root directory
 Looks like \?? is the object root of current session (\Sessions\0\DosDevices\)
 \DosDevices\C:\WINDOWS\example.txt
 So \?? or \DosDevices lead to \Sessions\0\DosDevices\
 And all network shares are there

 It seems that '\??\Global' is an local symbolic link (\Sessions\0\DosDevices\Global) to '\Global??'

 https://superuser.com/questions/884347/win32-and-the-global-namespace

 https://github.com/hfiref0x/WinObjEx64
 DOS device path format:
   \\.\C:\Test\Foo.txt
   \\?\C:\Test\Foo.txt
   \\.\Volume{b75e2c83-0000-0000-0000-602f00000000}\Test\Foo.txt
   \\?\Volume{b75e2c83-0000-0000-0000-602f00000000}\Test\Foo.txt

 OpenFileById ?  // inode?

 NTFS/ReFS:
   ??\C:\FileID
   \device\HardDiskVolume1\ObjectID
      where FileID is 8 bytes and ObjectID is 16 bytes.

For a caller to synchronize an I/O completion by waiting for the returned FileHandle, the SYNCHRONIZE flag must be set. Otherwise, a caller that is a device or intermediate driver must synchronize an I/O completion by using an event object.

If the caller sets only the FILE_APPEND_DATA and SYNCHRONIZE flags, it can write only to the end of the file, and any offset information about write operations to the file is ignored. The file will automatically be extended as necessary for this type of operation.

 LINUX: sd[a-z]  or in some hd[a-z] refers to hard drives  // /dev/sda3  meand Disk A, partition 3

  FileCreateSync(PWSTR FileName, ACCESS_MASK DesiredAccess, ULONG FileAttributes, ULONG ShareAccess, ULONG CreateDisposition, ULONG CreateOptions, PHANDLE FileHandle)
      Status = NCMN::NNTDLL::FileCreateSync(LogFilePath, FILE_APPEND_DATA, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ|FILE_SHARE_WRITE, (LogMode & lmFileUpd)?FILE_OPEN_IF:FILE_OVERWRITE_IF, FILE_NON_DIRECTORY_FILE, &hLogFile);

TODO: Async  (O_ASYNC, O_NONBLOCK, )
 https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-zwreadfile

To create a handle that has an associated current file-position pointer, specify the SYNCHRONIZE access right in
the DesiredAccess parameter to ZwCreateFile, IoCreateFile, or ZwOpenFile, and either FILE_SYNCHRONOUS_IO_ALERT or FILE_SYNCHRONOUS_IO_NONALERT
in the CreateOptions or OpenOptions parameter. Be sure that you do not also specify the FILE_APPEND_DATA access right.

FILE_OPEN_REPARSE_POINT is probably needed together with FILE_DIRECTORY_FILE|FILE_OPEN_FOR_BACKUP_INTENT when opening a directory // FindFirstFile doesn`t use FILE_OPEN_REPARSE_POINT, but uses FILE_OPEN_FOR_BACKUP_INTENT
*/
FUNC_WRAPPERNI(PX::open,       open       )
{
 NT::IO_STATUS_BLOCK iosb = {};
 NT::HANDLE FileHandle = 0;
 NT::ULONG ShareAccess = 0;
 NT::ULONG ObjAttributes = 0;
 NT::ULONG CreateOptions = NT::FILE_SYNCHRONOUS_IO_NONALERT;   // This adds file position support
 NT::ULONG FileAttributes = NT::FILE_ATTRIBUTE_NORMAL;
 NT::ULONG CreateDisposition = 0;
 NT::ACCESS_MASK DesiredAccess = NT::SYNCHRONIZE;    // The File handle will be waitable. The handle is signaled each time that an I/O operation that was issued on the handle completes. However, the caller must not wait on a handle that was opened for synchronous file access (FILE_SYNCHRONOUS_IO_NONALERT or FILE_SYNCHRONOUS_IO_ALERT). In this case, ZwReadFile waits on behalf of the caller and does not return until the read operation is complete.

 const achar* path  = (achar*)GetParFromPk<0>(args...);
 const uint   flags = GetParFromPk<1>(args...);
// const uint   mode  = GetParFromPk<2>(args...);

 if(!(flags & PX::O_CLOEXEC))ObjAttributes |= NT::OBJ_INHERIT;
 if(flags & PX::O_SYMLINK)ObjAttributes |= NT::OBJ_OPENLINK;       // NOTE: There is no O_SYMLINK on Linux
 if(flags & PX::O_EXCL   )ObjAttributes |= NT::OBJ_EXCLUSIVE;
 if(flags & PX::O_TMPFILE)FileAttributes = NT::FILE_ATTRIBUTE_TEMPORARY;    // Incomplete behaviour!

 uint amode = flags & PX::O_ACCMODE;
 if(amode == PX::O_RDONLY){DesiredAccess |= NT::GENERIC_READ; ShareAccess |= NT::FILE_SHARE_READ;}
 else if(amode == PX::O_WRONLY){DesiredAccess |= NT::GENERIC_WRITE; ShareAccess |= NT::FILE_SHARE_WRITE;}
 else if(amode == PX::O_RDWR){DesiredAccess |= NT::GENERIC_READ|NT::GENERIC_WRITE; ShareAccess |= NT::FILE_SHARE_READ|NT::FILE_SHARE_WRITE;}
 if(flags & PX::O_APPEND)  // NOTE: O_RDONLY is 0 and assumed default on Linux but here it is overriden by O_APPEND   // NOTE: Without FILE_APPEND_DATA offsets must be specified to NtWriteFile if no SYNCHRONIZE is specified
  {
   if(amode){DesiredAccess |= +NT::FILE_APPEND_DATA; ShareAccess |= NT::FILE_SHARE_WRITE;}
    else {DesiredAccess = +NT::FILE_APPEND_DATA|NT::SYNCHRONIZE; ShareAccess = NT::FILE_SHARE_WRITE; CreateOptions = 0;}
  }

 if(flags & PX::O_CREAT)      // S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH;
  {
   ShareAccess |= NT::FILE_SHARE_READ|NT::FILE_SHARE_WRITE|NT::FILE_SHARE_DELETE;  // On Linux files are shared
   if(flags & PX::O_EXCL)CreateDisposition |= NT::FILE_CREATE;
    else
     {
      ObjAttributes     |= NT::OBJ_OPENIF;
      CreateDisposition |= (flags & PX::O_TRUNC)?NT::FILE_OVERWRITE_IF:NT::FILE_OPEN_IF;
     }
  }
   else CreateDisposition |= (flags & PX::O_TRUNC)?NT::FILE_OVERWRITE:NT::FILE_OPEN;

 if(flags & PX::O_SYNC     )CreateOptions |= NT::FILE_WRITE_THROUGH;
 if(flags & PX::O_DIRECT   )CreateOptions |= NT::FILE_NO_INTERMEDIATE_BUFFERING;
 if(flags & PX::O_DIRECTORY)
  {
   CreateOptions |= NT::FILE_DIRECTORY_FILE;     // Directory object     // ??? FILE_OPEN_REPARSE_POINT|FILE_OPEN_FOR_BACKUP_INTENT    // Virtual directories/junctions can be mounted as reparse points
   ShareAccess   |= NT::FILE_SHARE_READ|NT::FILE_SHARE_WRITE|NT::FILE_SHARE_DELETE;    // To avoid locking the directory from modification by anyone else while its descriptor is open (Most likely for 'getdents')
  }
  else CreateOptions |= NT::FILE_NON_DIRECTORY_FILE;    // File object: a data file, a logical, virtual, or physical device, or a volume

 NT::NTSTATUS res = NTX::OpenFileObject(&FileHandle, path, DesiredAccess, ObjAttributes, FileAttributes, ShareAccess, CreateDisposition, CreateOptions, &iosb);
 if(!res)return (PX::fdsc_t)FileHandle;
// iosb.Status : EFIOStatus
 return -NTX::NTStatusToLinuxErr(res);   // TODO: Verify conformance with https://man7.org/linux/man-pages/man2/open.2.html
}
//------------------------------------------------------------------------------------------------------------
FUNC_WRAPPERNI(PX::pipe2,      pipe       ) {return 0;}

FUNC_WRAPPERNI(PX::flock,      flock      ) {return 0;}
FUNC_WRAPPERNI(PX::fsync,      fsync      ) {return 0;}
FUNC_WRAPPERNI(PX::fdatasync,  fdatasync  ) {return 0;}

FUNC_WRAPPERNI(PX::dup3,       dup        ) {return 0;}
//------------------------------------------------------------------------------------------------------------
FUNC_WRAPPERNI(PX::fcntl,      fcntl      )    // NtFsControlFile?    // F_DUPFD ? F_NOTIFY ?
{
 NT::IO_STATUS_BLOCK iosb = {};
 PX::fdsc_t fd = GetParFromPk<0>(args...);    
 uint32   code = GetParFromPk<1>(args...);
 PX::PVOID Ptr = GetParFromPk<2>(args...);
 PX::PVOID Len = GetParFromPk<3>(args...);
 NT::ULONG Size = (Len)?(*(uint32*)Len):(0);
 NT::NTSTATUS res = SAPI::NtFsControlFile((NT::HANDLE)fd, (NT::HANDLE)0, nullptr, nullptr, &iosb, code, Ptr, Size, Ptr, Size);
 if(res)return -NTX::NTStatusToLinuxErr(res);
 if(Len)*(uint32*)Len = iosb.Information;
 // TODO: Determine by the control code what and where to take from IO_STATUS_BLOCK
 return 0;
}
//------------------------------------------------------------------------------------------------------------
FUNC_WRAPPERNI(PX::ioctl,      ioctl      )    // NtDeviceIoControlFile   // Just some average implementation
{
 NT::IO_STATUS_BLOCK iosb = {};
 PX::fdsc_t fd = GetParFromPk<0>(args...);    
 uint32   code = GetParFromPk<1>(args...);
 PX::PVOID Ptr = GetParFromPk<2>(args...);
 PX::PVOID Len = GetParFromPk<3>(args...);
 NT::ULONG Size = (Len)?(*(uint32*)Len):(0);
 NT::NTSTATUS res = SAPI::NtDeviceIoControlFile((NT::HANDLE)fd, (NT::HANDLE)0, nullptr, nullptr, &iosb, code, Ptr, Size, Ptr, Size);
 if(res)return -NTX::NTStatusToLinuxErr(res);
 if(Len)*(uint32*)Len = iosb.Information;
 // TODO: Determine by the control code what and where to take from IO_STATUS_BLOCK
 return 0;
} 
//------------------------------------------------------------------------------------------------------------
// https://learn.microsoft.com/ru-ru/archive/blogs/wsl/pico-process-overview
// NOTE Will work only for those processes which are ready to behave like on Linux (No manifest, activation context, Fusion/SxS, ...)
// NOTE: Args may be too big for stack!
FUNC_WRAPPERNI(PX::spawn,      spawn      ) 
{
 return 0;
}
//------------------------------------------------------------------------------------------------------------
FUNC_WRAPPERNI(NTHD::thread,     thread   ) 
{
 auto   ThProc  = GetParFromPk<0>(args...);
 vptr   ThData  = GetParFromPk<1>(args...);
 if(!ThProc)return PXERR(ENOEXEC);     // Nothing to execute
 size_t DatSize = GetParFromPk<2>(args...);
 size_t StkSize = GetParFromPk<3>(args...);   // NOTE: As StkSize is aligned to a page size, there will be at least one page wasted for ThreadContext struct (Assume it always available for some thread local data?)
 size_t TlsSize = GetParFromPk<4>(args...);   // Slots is at least of pointer size

 size_t* StkFrame = nullptr;
 NTHD::SThCtx* ThrFrame = InitThreadRec((vptr)ThProc, ThData, StkSize, TlsSize, DatSize, &StkFrame);
 if(uint err=MMERR(ThrFrame);err)return -err;

 NT::PVOID  StackBase = ThrFrame->StkBase;
 NT::SIZE_T StackSize = ThrFrame->StkOffs;    // Allowed to be not aligned to PAGESIZE if the stack is already allocated
 NT::NTSTATUS res = NTX::NativeCreateThread(NPTM::ThProcCallStub, ThrFrame, 0, NT::NtCurrentProcess, false, &StackBase, &StackSize, (NT::PHANDLE)&ThrFrame->ThreadHndl, (NT::PULONG)&ThrFrame->ThreadID);
 DBGMSG("NativeCreateThread: %08X",res);
 return (uint)ThrFrame->ThreadHndl;    // Use handle instead of ID (Must be closed)   // GetExitCodeThread have problems with return codes(STILL_ACTIVE)
}
//------------------------------------------------------------------------------------------------------------
FUNC_WRAPPERNI(NTHD::thread_sleep,      thread_sleep     ) {return 0;} 
FUNC_WRAPPERNI(NTHD::thread_wait,       thread_wait      ) 
{
 uint64 time = GetParFromPk<1>(args...);
 NTHD::SThCtx* tinf = GetThreadByHandle(GetParFromPk<0>(args...));
 if(!tinf)return PXERR(EBADF);   // i.e. the thread is already finished
 if(time != (uint64)-1)
  {
   // TODO: fill TS
  }
NT::HANDLE  hndl = (NT::HANDLE)tinf->ThreadHndl;    // Cache the value
NT::NTSTATUS res = SAPI::NtWaitForSingleObject(hndl, true, nullptr);  //  TODO: PLARGE_INTEGER Timeout     // On Linux 'wait' is alertable   // Returns 0 after NtTerminateThread too
if((res != NT::STATUS_INVALID_HANDLE) && (tinf->ThreadHndl == (uint)hndl))SAPI::NtClose(hndl);    //NOTE: May crash under debugger if the handle is already closed by the exiting thread
return -NTX::NTStatusToLinuxErr(res);
}    
//------------------------------------------------------------------------------------------------------------
FUNC_WRAPPERNI(NTHD::thread_status,     thread_status    ) 
{
 uint hnd = GetParFromPk<0>(args...);
 NTHD::SThCtx* ThCtx = nullptr;
 NTHD::STDesc* ThDsc = NPTM::GetThDesc();
 if(hnd != ThDsc->MainTh.ThreadID)
  {
   if(!ThDsc->ThreadInfo)return PXERR(ENOMEM); // No more threads
   NTHD::SThCtx** ptr = ThDsc->ThreadInfo->FindOldThreadByHandle(hnd);
   if(!ptr)return PXERR(ENOENT);
   ThCtx = NTHD::ReadRecPtr(ptr);
  }
   else ThCtx = &ThDsc->MainTh;
 if(!ThCtx)return PXERR(EBADF);
 DBGMSG("Status: %08X",ThCtx->ExitCode);
 return ThCtx->ExitCode;
}
//------------------------------------------------------------------------------------------------------------
FUNC_WRAPPERNI(NTHD::thread_exit,       thread_exit      ) 
{
 sint status = GetParFromPk<0>(args...);   // If this var is on stack, the stack may become deallocated (probably - Even marked records should be checked for zero TID)
 NTHD::SThCtx* tinf = GetThreadSelf();
 if(tinf && tinf->SelfPtr)
  {
   tinf->LastThrdID  = tinf->ThreadID; 
   tinf->LastThrdHnd = tinf->ThreadHndl; 
   tinf->ExitCode    = status; 
   NT::HANDLE hndl   = tinf->ThreadHndl;  // TODO: Memory barrier
   tinf->ThreadHndl  = 0;  // The system will not clear this for us  // Windows will not clear ThreadID either!
   tinf->ThreadID    = 0;
   SAPI::NtClose(hndl);
   NTHD::ReleaseRec((NTHD::SThCtx**)tinf->SelfPtr); 
 }
 return NAPI::exit(status);
}    
//------------------------------------------------------------------------------------------------------------
FUNC_WRAPPERNI(NTHD::thread_kill, thread_kill ) {return 0;}
FUNC_WRAPPERNI(NTHD::thread_alert, thread_alert ) {return 0;}
FUNC_WRAPPERNI(NTHD::thread_affinity_set, thread_affinity_set ) {return 0;}
FUNC_WRAPPERNI(NTHD::thread_affinity_get, thread_affinity_get ) {return 0;}
//------------------------------------------------------------------------------------------------------------
FUNC_WRAPPERNI(PX::gettimeofday,  gettimeofday  )
{
 PX::timeval*  tv = GetParFromPk<0>(args...);
 PX::timezone* tz = GetParFromPk<1>(args...);
 uint64 ut = (NTX::GetSystemTime() - NDT::EPOCH_BIAS);
 if(tv)
  {
   tv->sec   = ut / NDT::SECS_TO_FT_MULT;
   uint64 rm = ut % NDT::SECS_TO_FT_MULT;     //   uint64 rm = ut - (tv->sec * NDT::SECS_TO_FT_MULT);
   tv->usec  = rm / (NDT::SECS_TO_FT_MULT/NDT::MICSEC_IN_SEC);
  }
 if(tz)
  {
   if(tz->utcoffs == -1)UpdateTZOffsUTC();
   tz->dsttime = 0;
   tz->utcoffs = GetTZOffsUTC();
  }
 return PX::NOERROR;
}
//------------------------------------------------------------------------------------------------------------
FUNC_WRAPPERNI(PX::settimeofday,  settimeofday  ) {return 0;}
//------------------------------------------------------------------------------------------------------------
// >>>>> MEMORY <<<<<
//#include "Impl_Mem.hpp"
// >>>>> NETWORK <<<<<
//#include "Impl_Net.hpp"
// >>>>> FILE SYSTEM <<<<<
//#include "Impl_FS.hpp"
// >>>>> PROCESSES/THREADS <<<<<
//#include "Impl_PT.hpp"
};
//============================================================================================================
#include "Startup.hpp"

//============================================================================================================
static sint Initialize(vptr StkFrame=nullptr, vptr ArgA=nullptr, vptr ArgB=nullptr, vptr ArgC=nullptr, bool InitConLog=false)    // _finline ?
{
 if(IsInitialized())return 1;
 if(!NLOG::CurrLog)NLOG::CurrLog = &NLOG::GLog;  // Will be set with correct address, relative to the Base
 if(InitConLog)   // On this stage file logging is not possible yet (needs InitStartupInfo)
  {
   NPTM::NLOG::GLog.LogModes   = NPTM::NLOG::lmCons;
   NPTM::NLOG::GLog.ConsHandle = NPTM::GetStdErr();
  }
 InitSyscalls();
 InitStartupInfo(StkFrame, ArgA, ArgB, ArgC);
 SetErrorHandlers();
                         
 IFDBG{DbgLogStartupInfo();}
 if(NTHD::SThCtx* MainTh=&NPTM::GetThDesc()->MainTh; !MainTh->Self)
  {
   MainTh->Self       = MainTh;     // For checks
   MainTh->SelfPtr    = nullptr;    // Not owned
   MainTh->TlsBase    = nullptr;    // Allocate somewhere on demand?
   MainTh->TlsSize    = 0;
   MainTh->StkBase    = nullptr;    // Get from ELF header or proc/mem ???
   MainTh->StkSize    = 0;          // StkSize; ??? // Need full size for unmap  // Can a thread unmap its own stack before calling 'exit'?
   MainTh->GroupID    = NAPI::getpgrp();   // pid
   MainTh->ThreadID   = NAPI::gettid();
   MainTh->ProcesssID = NAPI::getpid();
   MainTh->ThreadProc = nullptr;    // Get it from ELF or set from arg?
   MainTh->ThreadData = nullptr;
   MainTh->ThDataSize = 0;
   MainTh->Flags      = 0;    // ???
  }
 return 0;// crc;
}
//============================================================================================================
/*
 	SetConsoleCP(CP_UTF8);
	SetConsoleOutputCP(CP_UTF8);
*/

