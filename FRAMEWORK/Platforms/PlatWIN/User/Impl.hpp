
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
DECL_SYSCALL(WPROCID(HashNtDll,"NtSetInformationFile"),       NT::NtSetInformationFile,       NtSetInformationFile       )
DECL_SYSCALL(WPROCID(HashNtDll,"NtMapViewOfSection"),         NT::NtMapViewOfSection,         NtMapViewOfSection         )
DECL_SYSCALL(WPROCID(HashNtDll,"NtUnmapViewOfSection"),       NT::NtUnmapViewOfSection,       NtUnmapViewOfSection       )
DECL_SYSCALL(WPROCID(HashNtDll,"NtCreateSection"),            NT::NtCreateSection,            NtCreateSection            )
DECL_SYSCALL(WPROCID(HashNtDll,"NtOpenSection"),              NT::NtOpenSection,              NtOpenSection              )
DECL_SYSCALL(WPROCID(HashNtDll,"NtQuerySection"),             NT::NtQuerySection,             NtQuerySection             )

DECL_SYSCALL(WPROCID(HashNtDll,"NtCreateSymbolicLinkObject"), NT::NtCreateSymbolicLinkObject, NtCreateSymbolicLinkObject )
DECL_SYSCALL(WPROCID(HashNtDll,"NtOpenSymbolicLinkObject"),   NT::NtOpenSymbolicLinkObject,   NtOpenSymbolicLinkObject   )
DECL_SYSCALL(WPROCID(HashNtDll,"NtQuerySymbolicLinkObject"),  NT::NtQuerySymbolicLinkObject,  NtQuerySymbolicLinkObject  )

DECL_SYSCALL(WPROCID(HashNtDll,"NtClose"),                    NT::NtClose,                    NtClose                    )

DECL_SYSCALL(WPROCID(HashNtDll,"NtDelayExecution"),           NT::NtDelayExecution,           NtDelayExecution           )
DECL_SYSCALL(WPROCID(HashNtDll,"NtTerminateThread"),          NT::NtTerminateThread,          NtTerminateThread          )
DECL_SYSCALL(WPROCID(HashNtDll,"NtTerminateProcess"),         NT::NtTerminateProcess,         NtTerminateProcess         )

DECL_SYSCALL(WPROCID(HashNtDll,"NtLoadDriver"),               NT::NtLoadDriver,               NtLoadDriver               )
DECL_SYSCALL(WPROCID(HashNtDll,"NtUnloadDriver"),             NT::NtUnloadDriver,             NtUnloadDriver             )   // Should be last

} static volatile constexpr inline SysApi alignas(16);   // Declared to know exact address(?), its size is ALWAYS 1
//============================================================================================================
#include "../UtilsFmtPE.hpp"
#include "../NtDllEx.hpp"
//============================================================================================================
struct NAPI   // On NIX all syscall stubs will be in NAPI   // uwin-master
{
//#include "../POSX.hpp"

FUNC_WRAPPERFI(PX::exit,       exit       ) { SAPI::NtTerminateThread(NT::NtCurrentThread, GetParFromPk<0>(args...)); }
FUNC_WRAPPERFI(PX::exit_group, exit_group ) { SAPI::NtTerminateProcess(NT::NtCurrentProcess, GetParFromPk<0>(args...)); }
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
 const uint   prot     = GetParFromPk<2>(args...);
 const uint   flags    = GetParFromPk<3>(args...);
 const NT::HANDLE  fd  = (NT::HANDLE*)GetParFromPk<4>(args...); 
 const uint64 pgoffset = GetParFromPk<5>(args...);

 if(!(flags & (PX::MAP_PRIVATE|PX::MAP_SHARED)))return -PX::EINVAL;   // One of MAP_SHARED or MAP_PRIVATE must be specified.
 if((flags & (PX::MAP_PRIVATE|PX::MAP_SHARED)) == (PX::MAP_PRIVATE|PX::MAP_SHARED))return -PX::EINVAL;  // ???

 vptr   RegionBase  = addr;         // Rounded <<<  
 size_t RegionSize  = AlignP2Frwd(length, MEMGRANSIZE);       // Rounded >>>   
 uint32 AllocType   = 0;    // MEM_COMMIT
 uint32 AllocProt   = NTX::MemProtPXtoNT(prot); 
 NT::NTSTATUS res   = 0;

 if(flags & PX::MAP_NOCACHE)AllocProt |= NT::PAGE_NOCACHE;   // ???
 if((flags & PX::MAP_ANONYMOUS) && (flags & PX::MAP_PRIVATE))   // Allocate simple private virtual memory
  {
/*   size_t RegResSize = AlignP2Frwd(length, MEMGRANSIZE); 
   size_t RegComSize = AlignP2Frwd(length, MEMPAGESIZE); 
   if(addr)   // Either we want memory at specific address or we commiting in a previously reserved area (which is not exposed by mmap)
    {
     AllocType |= MEM_RESERVE;  // Attempting to commit a specific address range by specifying MEM_COMMIT without MEM_RESERVE and a non-NULL lpAddress fails unless the entire range has already been reserved.

     // ???
    }
     else  // If the address is NULL then we can only RESERVE+COMMIT
      {
       if(RegResSize != RegComSize)    // TODO: Just commit the entire RegResSize? (Will cause memory waste in Working Set)
        {
         res = SAPI::NtAllocateVirtualMemory(NT::NtCurrentProcess, &RegionBase, 0, &RegResSize, MEM_RESERVE, AllocProt);
         if(!res)res = SAPI::NtAllocateVirtualMemory(NT::NtCurrentProcess, &RegionBase, 0, &RegComSize, MEM_COMMIT, AllocProt);
        }
         else res = SAPI::NtAllocateVirtualMemory(NT::NtCurrentProcess, &RegionBase, 0, &RegionSize, MEM_RESERVE|MEM_COMMIT, AllocProt);  // The size is already 64k aligned, no holes
      } */
   res = SAPI::NtAllocateVirtualMemory(NT::NtCurrentProcess, &RegionBase, 0, &RegionSize, NT::MEM_RESERVE, AllocProt);  // First reserve with 64k granularity to avoid memory holes (allowed to fail if already has been done)
   if(res)RegionBase = addr;     // Reset the base and size
   RegionSize = length;   // Now 4k alignment is OK
   res = SAPI::NtAllocateVirtualMemory(NT::NtCurrentProcess, &RegionBase, 0, &RegionSize, NT::MEM_COMMIT,  AllocProt);  // Then commit with 4k granularity (page size)
   if(!res)return RegionBase;
   return (vptr)-NTX::NTStatusToLinuxErr(res);    // ???        // GetMMapErrFromPtr
  }

 //
 // TODO: Shared memory
 //
 return 0;
}

/*
 The address addr must be a multiple of the page size (but length need not be)
 If the MEM_RELEASE flag is set in the FreeType parameter, BaseAddress must be the base address returned by ZwAllocateVirtualMemory when the region was reserved.
 If the MEM_RELEASE flag is set in the FreeType parameter, the variable pointed to by RegionSize must be zero. 
     ZwFreeVirtualMemory frees the entire region that was reserved in the initial allocation call to ZwAllocateVirtualMemory.
 ZwFreeVirtualMemory does not fail if you attempt to release pages that are in different states, some reserved and some committed
 NOTE: Ideally we must RELEASE only if the Size covers the entire region (But we have to use QueryVirtualMemory to know that)

*/
FUNC_WRAPPERNI(PX::munmap,     munmap     )                // ZwUnmapViewOfSectionEx
{
 const vptr   addr   = (vptr)GetParFromPk<0>(args...); 
 const size_t length = GetParFromPk<1>(args...); 

 vptr   RegionBase   = addr;    // Rounded <<<  
 size_t RegionSize   = 0;       // Rounded >>>   // If the dwFreeType parameter is MEM_RELEASE, this parameter must be 0

 NT::NTSTATUS res = SAPI::NtFreeVirtualMemory(NT::NtCurrentProcess, &RegionBase, &RegionSize, NT::MEM_RELEASE);   // Releases the entire region but only if addr is the same base address that came from mmap (in most cases this is what you do)
 if(!res)return PX::NOERROR;  // The entire allocated region is free now   // TODO: Check returned error, detect shared/private mappings and do UnmapViewOfSection for the addr instead of NtFreeVirtualMemory 
 RegionBase = addr;
 RegionSize = length;
 res = SAPI::NtFreeVirtualMemory(NT::NtCurrentProcess, &RegionBase, &RegionSize, NT::MEM_DECOMMIT);   // Addr is not region base addr, try at least to decommit some pages (Don`t even try to base your memory manager on this behaviour!)
 if(!res)return PX::NOERROR; 
 return -NTX::NTStatusToLinuxErr(res);
}

FUNC_WRAPPERNI(PX::mremap,     mremap     ) {return 0;}    // LINUX specific

FUNC_WRAPPERNI(PX::madvise,    madvise    ) 
{
    // instead of unmapping the address, we're just gonna trick 
    // the TLB to mark this as a new mapped area which, due to 
    // demand paging, will not be committed until used.

 //   mmap(addr, size, PROT_NONE, MAP_FIXED|MAP_PRIVATE|MAP_ANON, -1, 0);
 //   msync(addr, size, MS_SYNC|MS_INVALIDATE);
 return 0;
}
FUNC_WRAPPERNI(PX::mprotect,   mprotect   ) {return 0;}
FUNC_WRAPPERNI(PX::msync,      msync      ) {return 0;}    // NtFlushVirtualMemory  (FlushViewOfFile)
FUNC_WRAPPERNI(PX::mlock,      mlock      ) {return 0;}    // NtLockVirtualMemory
FUNC_WRAPPERNI(PX::munlock,    munlock    ) {return 0;}    // NtUnlockVirtualMemory

FUNC_WRAPPERNI(PX::close,      close      ) {return SAPI::NtClose((NT::HANDLE)GetParFromPk<0>(args...));}

FUNC_WRAPPERFI(PX::read,       read       ) 
{
 NT::IO_STATUS_BLOCK iosb = {};
 const NT::HANDLE hnd = (NT::HANDLE)GetParFromPk<0>(args...); 
 const vptr   buf = (vptr)GetParFromPk<1>(args...);
 const size_t len = GetParFromPk<2>(args...);
 NT::NTSTATUS res = SAPI::NtReadFile(hnd, nullptr, nullptr, nullptr, &iosb, buf, len, nullptr, nullptr);  // Relative to current file position
 if(!res)return iosb.Information;    // Number of bytes read
 return -NTX::NTStatusToLinuxErr(res);
}

FUNC_WRAPPERFI(PX::write,      write      ) 
{
 NT::IO_STATUS_BLOCK iosb = {};
 const NT::HANDLE hnd = (NT::HANDLE)GetParFromPk<0>(args...); 
 const vptr   buf = (vptr)GetParFromPk<1>(args...);
 const size_t len = GetParFromPk<2>(args...);
 NT::NTSTATUS res = SAPI::NtWriteFile(hnd, nullptr, nullptr, nullptr, &iosb, buf, len, nullptr, nullptr);  // Relative to current file position
 if(!res)return iosb.Information;    // Number of bytes written
 return -NTX::NTStatusToLinuxErr(res);
}

FUNC_WRAPPERNI(PX::readv,      readv      ) {return 0;}
FUNC_WRAPPERNI(PX::writev,     writev     ) {return 0;}

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

// Complicated
FUNC_WRAPPERNI(PX::mkfifo,     mkfifo     ) {return 0;}

// Or use 'open' with O_DIRECTORY instead?
FUNC_WRAPPERNI(PX::mkdir,      mkdir      ) 
{
 NT::IO_STATUS_BLOCK iosb = {};
 NT::HANDLE FileHandle = nullptr;
 const achar* path = (achar*)GetParFromPk<0>(args...);
 int mode = GetParFromPk<1>(args...);   // TODO: Mode support

 NT::NTSTATUS res = NTX::OpenFileObject(path, NT::SYNCHRONIZE|NT::FILE_READ_ATTRIBUTES, 0, NT::FILE_ATTRIBUTE_NORMAL, NT::FILE_SHARE_READ|NT::FILE_SHARE_WRITE|NT::FILE_SHARE_DELETE, NT::FILE_CREATE, NT::FILE_DIRECTORY_FILE|NT::FILE_SYNCHRONOUS_IO_NONALERT, &iosb, &FileHandle); 
 if(res)return -NTX::NTStatusToLinuxErr(res);  // Can the handle be open? (Status > 0)
 SAPI::NtClose(FileHandle);
 return PX::NOERROR;;
}
FUNC_WRAPPERNI(PX::rmdir,      rmdir      ) 
{
 
 return 0;
}

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

FUNC_WRAPPERNI(PX::rename,     rename     ) {return 0;}
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

 NT::HANDLE LinkHandle = nullptr;
 NT::NTSTATUS res = SAPI::NtOpenSymbolicLinkObject(&LinkHandle, NT::GENERIC_READ, &oattr);
 if(res)return -NTX::NTStatusToLinuxErr(res); 
*/      

 return 0;
}

// Compatibility?
FUNC_WRAPPERNI(PX::access,     access     ) 
{
 NT::IO_STATUS_BLOCK iosb = {};
 NT::HANDLE FileHandle = nullptr;
 const achar* path = (achar*)GetParFromPk<0>(args...);
 int mode = GetParFromPk<1>(args...);

 uint32 BaseAccess = NT::SYNCHRONIZE|NT::FILE_READ_ATTRIBUTES;   // F_OK
 if(mode & PX::X_OK)BaseAccess |= NT::FILE_EXECUTE;    // Close enough?
 if(mode & PX::W_OK)BaseAccess |= NT::FILE_WRITE_DATA;
 if(mode & PX::R_OK)BaseAccess |= NT::FILE_READ_DATA;
 NT::NTSTATUS res = NTX::OpenFileObject(path, BaseAccess, 0, NT::FILE_ATTRIBUTE_NORMAL, NT::FILE_SHARE_READ|NT::FILE_SHARE_WRITE|NT::FILE_SHARE_DELETE, NT::FILE_OPEN, NT::FILE_SYNCHRONOUS_IO_NONALERT, &iosb, &FileHandle); 
 if(res)return -NTX::NTStatusToLinuxErr(res);  // Can the handle be open? (Status > 0)
 SAPI::NtClose(FileHandle);
 return PX::NOERROR;
}

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
FUNC_WRAPPERNI(PX::stat,       stat       ) 
{
 NT::IO_STATUS_BLOCK iosb = {};
 NT::HANDLE FileHandle = nullptr;
 const achar* path = (achar*)GetParFromPk<0>(args...);
 PX::SFStat* sti = (PX::SFStat*)GetParFromPk<1>(args...);
 NT::NTSTATUS res = NTX::OpenFileObject(path, NT::SYNCHRONIZE|NT::FILE_READ_ATTRIBUTES, 0, NT::FILE_ATTRIBUTE_NORMAL, NT::FILE_SHARE_READ|NT::FILE_SHARE_WRITE|NT::FILE_SHARE_DELETE, NT::FILE_OPEN, NT::FILE_SYNCHRONOUS_IO_NONALERT, &iosb, &FileHandle); 
 if(res)return -NTX::NTStatusToLinuxErr(res);  // Can the handle be open? (Status > 0)
 int rs = NAPI::fstat((PX::fdsc_t)FileHandle, sti);
 SAPI::NtClose(FileHandle);
 return rs;
}

FUNC_WRAPPERNI(PX::fstat,      fstat      ) 
{
 NT::IO_STATUS_BLOCK iosb = {};
 NT::FILE_ALL_INFORMATION inf = {};
 NT::HANDLE  hnd = (NT::HANDLE)GetParFromPk<0>(args...); 
 PX::SFStat* sti = (PX::SFStat*)GetParFromPk<1>(args...);
 NT::NTSTATUS res = SAPI::NtQueryInformationFile(hnd, &iosb, &inf, sizeof(inf), NT::FileAllInformation);
 if(res && (res != NT::STATUS_BUFFER_OVERFLOW))return -NTX::NTStatusToLinuxErr(res);

 sti->st_dev     = 0;  // TODO: How?
 sti->st_ino     = inf.InternalInformation.IndexNumber;
 sti->st_nlink   = inf.StandardInformation.NumberOfLinks;
 sti->st_mode    = 0;
 sti->st_uid     = 0;  // Requires ACL read
 sti->st_gid     = 0;  // Requires ACL read
 sti->__pad0     = 0;
 sti->st_rdev    = 0;  // ???
 sti->st_size    = inf.StandardInformation.EndOfFile;
 sti->st_blksize = 0;  // TODO: From AlignmentInformation.AlignmentRequirement somehow
 sti->st_blocks  = inf.StandardInformation.AllocationSize / 512;  // AlignP2Frwd(sti->st_size,512) / 512;   // Number 512-byte blocks allocated. 
 sti->__unused[0] = sti->__unused[1] = sti->__unused[2] = 0;
  
 if(inf.StandardInformation.Directory)sti->st_mode |= PX::S_IFDIR;
   else sti->st_mode |= PX::S_IFREG;   // Anything else?    // How to get rwe flags of a file?
                                                
 sti->st_atime.sec = NDT::FileTimeToUnixTime(inf.BasicInformation.LastAccessTime, &sti->st_atime.nsec);
 sti->st_mtime.sec = NDT::FileTimeToUnixTime(inf.BasicInformation.LastWriteTime, &sti->st_mtime.nsec);
 sti->st_ctime.sec = NDT::FileTimeToUnixTime(inf.BasicInformation.CreationTime, &sti->st_ctime.nsec);  // On Unix creation time is not stored (only: access, modification and change) // Last inode change time is close enough    // xstat ?

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
*/
FUNC_WRAPPERNI(PX::open,       open       ) 
{
 NT::IO_STATUS_BLOCK iosb = {};
 NT::HANDLE FileHandle = nullptr;
 NT::ULONG ShareAccess = 0;
 NT::ULONG ObjAttributes = 0;
 NT::ULONG CreateOptions = NT::FILE_SYNCHRONOUS_IO_NONALERT;   // This adds file position support
 NT::ULONG FileAttributes = NT::FILE_ATTRIBUTE_NORMAL;
 NT::ULONG CreateDisposition = 0;
 NT::ACCESS_MASK DesiredAccess = NT::SYNCHRONIZE;    // The File handle will be waitable. The handle is signaled each time that an I/O operation that was issued on the handle completes. However, the caller must not wait on a handle that was opened for synchronous file access (FILE_SYNCHRONOUS_IO_NONALERT or FILE_SYNCHRONOUS_IO_ALERT). In this case, ZwReadFile waits on behalf of the caller and does not return until the read operation is complete.

 const achar* path  = (achar*)GetParFromPk<0>(args...);
 const uint   flags = GetParFromPk<1>(args...);
 const uint   mode  = GetParFromPk<2>(args...);
                                             
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
   if(amode){DesiredAccess |= NT::FILE_APPEND_DATA; ShareAccess |= NT::FILE_SHARE_WRITE;}
    else {DesiredAccess = NT::FILE_APPEND_DATA|NT::SYNCHRONIZE; ShareAccess = NT::FILE_SHARE_WRITE; CreateOptions = 0;}
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
 if(flags & PX::O_DIRECTORY)CreateOptions |= NT::FILE_DIRECTORY_FILE;     // Directory object
  else CreateOptions |= NT::FILE_NON_DIRECTORY_FILE;    // File object: a data file, a logical, virtual, or physical device, or a volume
      
 NT::NTSTATUS res = NTX::OpenFileObject(path, DesiredAccess, ObjAttributes, FileAttributes, ShareAccess, CreateDisposition, CreateOptions, &iosb, &FileHandle);                        
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

FUNC_WRAPPERNI(PX::spawn,      spawn      ) {return 0;}
FUNC_WRAPPERNI(PX::thread,     thread     ) {return 0;}

FUNC_WRAPPERNI(PX::gettimeofday,  gettimeofday  ) 
{


 return 0;
}
FUNC_WRAPPERNI(PX::settimeofday,  settimeofday  ) {return 0;}

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
struct SFWCTX
{
 // Some thread context data here (to be stored on stack)

sint Initialize(void* StkFrame=nullptr, vptr ArgA=nullptr, vptr ArgB=nullptr, vptr ArgC=nullptr, bool InitConLog=false)    // _finline ?
{
 if(IsInitialized())return 1;
// wchar srcp[] = {L"../path//to/../my/./.././file"};
// wchar buf[256];
// uint len = NTX::NormalizePathNt(srcp, buf);

/* volatile int crc = 0;//NCTM::Crc32((unsigned char*)&SysApi, sizeof(SAPI));
 crc += NCTM::Crc32((unsigned char*)&SAPI::NtProtectVirtualMemory, sizeof(SAPI::NtProtectVirtualMemory));
 crc += NCTM::Crc32((unsigned char*)&SAPI::NtAllocateVirtualMemory, sizeof(SAPI::NtAllocateVirtualMemory));
 crc += NCTM::Crc32((unsigned char*)&SAPI::NtFreeVirtualMemory, sizeof(SAPI::NtFreeVirtualMemory));
 crc += NCTM::Crc32((unsigned char*)&SAPI::NtReadVirtualMemory, sizeof(SAPI::NtReadVirtualMemory));
 crc += NCTM::Crc32((unsigned char*)&SAPI::NtWriteVirtualMemory, sizeof(SAPI::NtWriteVirtualMemory));
 crc += NCTM::Crc32((unsigned char*)&SAPI::NtQueryVirtualMemory, sizeof(SAPI::NtQueryVirtualMemory));
 crc += NCTM::Crc32((unsigned char*)&SAPI::NtCreateFile, sizeof(SAPI::NtCreateFile));
 crc += NCTM::Crc32((unsigned char*)&SAPI::NtWriteFile, sizeof(SAPI::NtWriteFile));
 crc += NCTM::Crc32((unsigned char*)&SAPI::NtReadFile, sizeof(SAPI::NtReadFile));
 crc += NCTM::Crc32((unsigned char*)&SAPI::NtDeleteFile, sizeof(SAPI::NtDeleteFile));
 crc += NCTM::Crc32((unsigned char*)&SAPI::NtWriteFileGather, sizeof(SAPI::NtWriteFileGather));
 crc += NCTM::Crc32((unsigned char*)&SAPI::NtReadFileScatter, sizeof(SAPI::NtReadFileScatter));
 crc += NCTM::Crc32((unsigned char*)&SAPI::NtFlushBuffersFile, sizeof(SAPI::NtFlushBuffersFile));
 crc += NCTM::Crc32((unsigned char*)&SAPI::NtQueryAttributesFile, sizeof(SAPI::NtQueryAttributesFile));
 crc += NCTM::Crc32((unsigned char*)&SAPI::NtQueryInformationFile, sizeof(SAPI::NtQueryInformationFile));
 crc += NCTM::Crc32((unsigned char*)&SAPI::NtMapViewOfSection, sizeof(SAPI::NtMapViewOfSection));
 crc += NCTM::Crc32((unsigned char*)&SAPI::NtUnmapViewOfSection, sizeof(SAPI::NtUnmapViewOfSection));
 crc += NCTM::Crc32((unsigned char*)&SAPI::NtCreateSection, sizeof(SAPI::NtCreateSection));
 crc += NCTM::Crc32((unsigned char*)&SAPI::NtOpenSection, sizeof(SAPI::NtOpenSection));
 crc += NCTM::Crc32((unsigned char*)&SAPI::NtQuerySection, sizeof(SAPI::NtQuerySection));
 crc += NCTM::Crc32((unsigned char*)&SAPI::NtClose, sizeof(SAPI::NtClose));
 crc += NCTM::Crc32((unsigned char*)&SAPI::NtDelayExecution, sizeof(SAPI::NtDelayExecution));
 crc += NCTM::Crc32((unsigned char*)&SAPI::NtTerminateThread, sizeof(SAPI::NtTerminateThread));
 crc += NCTM::Crc32((unsigned char*)&SAPI::NtTerminateProcess, sizeof(SAPI::NtTerminateProcess));
 crc += NCTM::Crc32((unsigned char*)&SAPI::NtLoadDriver, sizeof(SAPI::NtLoadDriver));
 crc += NCTM::Crc32((unsigned char*)&SAPI::NtUnloadDriver, sizeof(SAPI::NtUnloadDriver));  */

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
 return 0;// crc;
}

};
//============================================================================================================


