
#pragma once

#include "../PlatDef.hpp"
//============================================================================================================
_codesec struct SAPI  // All required syscall stubs    // Name 'NTAPI' causes compilation to fail with CLANG in VisualStudio! (Not anymore?)
{
private:
SCVR uint32 HashNtDll = NCTM::CRC32("ntdll.dll");    // Low Case
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
DECL_SYSCALL(WPROCID(HashNtDll,"NtMapViewOfSection"),         NT::NtMapViewOfSection,         NtMapViewOfSection         )
DECL_SYSCALL(WPROCID(HashNtDll,"NtUnmapViewOfSection"),       NT::NtUnmapViewOfSection,       NtUnmapViewOfSection       )
DECL_SYSCALL(WPROCID(HashNtDll,"NtCreateSection"),            NT::NtCreateSection,            NtCreateSection            )
DECL_SYSCALL(WPROCID(HashNtDll,"NtOpenSection"),              NT::NtOpenSection,              NtOpenSection              )
DECL_SYSCALL(WPROCID(HashNtDll,"NtQuerySection"),             NT::NtQuerySection,             NtQuerySection             )
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

FUNC_WRAPPERNI(PX::mmapGD,     mmap       ) {return 0;}
FUNC_WRAPPERNI(PX::munmap,     munmap     ) {return 0;}
FUNC_WRAPPERNI(PX::madvise,    madvise    ) {return 0;}
FUNC_WRAPPERNI(PX::mprotect,   mprotect   ) {return 0;}

FUNC_WRAPPERNI(PX::close,      close      ) {return SAPI::NtClose((NT::HANDLE)GetParFromPk<0>(args...));}

FUNC_WRAPPERFI(PX::read,       read       ) 
{
 NT::IO_STATUS_BLOCK iosb = {};
 const uint32 hnd = (uint32)GetParFromPk<0>(args...);
 const vptr   buf = (vptr)GetParFromPk<1>(args...);
 const size_t len = GetParFromPk<2>(args...);
 NT::NTSTATUS res = SAPI::NtReadFile(hnd, nullptr, nullptr, nullptr, &iosb, buf, len, nullptr, nullptr);  // Relative to current file position
 if(!res)return iosb.Information;    // Number of bytes read
 return -NTX::NTStatusToLinuxErr(res);
}

FUNC_WRAPPERFI(PX::write,      write      ) 
{
 NT::IO_STATUS_BLOCK iosb = {};
 const uint32 hnd = (uint32)GetParFromPk<0>(args...);
 const vptr   buf = (vptr)GetParFromPk<1>(args...);
 const size_t len = GetParFromPk<2>(args...);
 NT::NTSTATUS res = SAPI::NtWriteFile(hnd, nullptr, nullptr, nullptr, &iosb, buf, len, nullptr, nullptr);  // Relative to current file position
 if(!res)return iosb.Information;    // Number of bytes written
 return -NTX::NTStatusToLinuxErr(res);
}

FUNC_WRAPPERNI(PX::readv,      readv      ) {return 0;}
FUNC_WRAPPERNI(PX::writev,     writev     ) {return 0;}
FUNC_WRAPPERNI(PX::lseek,      lseek      ) 
{
 return 0;
}

// Complicated
FUNC_WRAPPERNI(PX::mkfifo,     mkfifo     ) {return 0;}
FUNC_WRAPPERNI(PX::mkdir,      mkdir      ) {return 0;}
FUNC_WRAPPERNI(PX::rmdir,      rmdir      ) {return 0;}

// Note: link behaviour on Windows is different
//This means its not sufficient to delete a file, it may not be deleted immediately, and this may cause problems in deleting directories and/or creating a new file of the same name.

//But you can closely simulate unix semantics by renaming the file to a temporary directory and scheduling it for deletion.
// "File System Behavior Overview.pdf"
//
FUNC_WRAPPERNI(PX::unlink,     unlink     ) 
{
 return 0;
}

FUNC_WRAPPERNI(PX::rename,     rename     ) {return 0;}
FUNC_WRAPPERNI(PX::readlink,   readlink   ) {return 0;}
FUNC_WRAPPERNI(PX::access,     access     ) {return 0;}
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
 NT::OBJECT_ATTRIBUTES oattr = {};
 NT::UNICODE_STRING FilePathUS;
 NT::HANDLE FileHandle = nullptr;
 NT::ULONG ShareAccess = 0;
 NT::ULONG CreateOptions = NT::FILE_SYNCHRONOUS_IO_NONALERT;   // This adds file position support
 NT::ULONG FileAttributes = NT::FILE_ATTRIBUTE_NORMAL;
 NT::ULONG CreateDisposition = 0;
 NT::ACCESS_MASK DesiredAccess = NT::SYNCHRONIZE;    // The File handle will be waitable. The handle is signaled each time that an I/O operation that was issued on the handle completes. However, the caller must not wait on a handle that was opened for synchronous file access (FILE_SYNCHRONOUS_IO_NONALERT or FILE_SYNCHRONOUS_IO_ALERT). In this case, ZwReadFile waits on behalf of the caller and does not return until the read operation is complete.

 const achar* path  = (achar*)GetParFromPk<0>(args...);
 const uint   flags = GetParFromPk<1>(args...);
 const uint   mode  = GetParFromPk<2>(args...);
 
 uint plen = NSTR::StrLen(path);
 NTX::EPathType ptype = NTX::GetPathTypeNt(path);
 uint ExtraLen = 4;
 NT::UNICODE_STRING* CurrDir = &NT::NtCurrentTeb()->ProcessEnvironmentBlock->ProcessParameters->CurrentDirectory.DosPath;    // 'C:\xxxxx\yyyyy\'
 if(ptype == NTX::ptSysRootRel)ExtraLen += NSTR::StrLen((wchar*)&fwsinf.SysDrive);
 else if(ptype == NTX::ptCurrDirRel)ExtraLen += CurrDir->Length / sizeof(wchar);

 wchar FullPath[(plen*4)+ExtraLen];
 uint DstLen = NSTR::StrCopy(FullPath, L"\\GLOBAL??\\");     // Windows XP?
 if(ptype == NTX::ptSysRootRel)DstLen += NSTR::StrCopy(&FullPath[DstLen], (wchar*)&fwsinf.SysDrive);
 else if(ptype == NTX::ptCurrDirRel)DstLen += NSTR::StrCopy(&FullPath[DstLen], (wchar*)CurrDir->Buffer);
 DstLen += NUTF::Utf8To16(&FullPath[DstLen], path, plen);
 FullPath[DstLen] = 0;
 DstLen  = NTX::NormalizePathNt(FullPath);
 FullPath[DstLen] = 0;
 FilePathUS.Set(FullPath, DstLen);

 oattr.Length = sizeof(NT::OBJECT_ATTRIBUTES);
 oattr.RootDirectory = nullptr;
 oattr.ObjectName = &FilePathUS;
 oattr.Attributes = 0;            // OBJ_CASE_INSENSITIVE;   //| OBJ_KERNEL_HANDLE;
 oattr.SecurityDescriptor = nullptr;         // TODO: Arg3: mode_t mode
 oattr.SecurityQualityOfService = nullptr;
                                             
 if(!(flags & PX::O_CLOEXEC))oattr.Attributes |= NT::OBJ_INHERIT;
 if(flags & PX::O_SYMLINK)oattr.Attributes |= NT::OBJ_OPENLINK;       // NOTE: There is no O_SYMLINK on Linux
 if(flags & PX::O_EXCL   )oattr.Attributes |= NT::OBJ_EXCLUSIVE;
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
      oattr.Attributes  |= NT::OBJ_OPENIF;
      CreateDisposition |= (flags & PX::O_TRUNC)?NT::FILE_OVERWRITE_IF:NT::FILE_OPEN_IF;
     }
  }
   else CreateDisposition |= (flags & PX::O_TRUNC)?NT::FILE_OVERWRITE:NT::FILE_OPEN;

 if(flags & PX::O_SYNC     )CreateOptions |= NT::FILE_WRITE_THROUGH;
 if(flags & PX::O_DIRECT   )CreateOptions |= NT::FILE_NO_INTERMEDIATE_BUFFERING;
 if(flags & PX::O_DIRECTORY)CreateOptions |= NT::FILE_DIRECTORY_FILE;     // Directory object
  else CreateOptions |= NT::FILE_NON_DIRECTORY_FILE;    // File object: a data file, a logical, virtual, or physical device, or a volume
                                
 NT::NTSTATUS res = SAPI::NtCreateFile(&FileHandle, DesiredAccess, &oattr, &iosb, nullptr, FileAttributes, ShareAccess, CreateDisposition, CreateOptions, nullptr, 0);
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

// >>>>> MEMORY <<<<<
#include "Impl_Mem.hpp"
// >>>>> NETWORK <<<<<
#include "Impl_Net.hpp"
// >>>>> FILE SYSTEM <<<<<
#include "Impl_FS.hpp"
// >>>>> PROCESSES/THREADS <<<<<
#include "Impl_PT.hpp"
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
   NPTM::NLOG::GLog.LogModes   = NPTM::NLOG::BaseMsgFlags | NPTM::NLOG::lmCons;
   NPTM::NLOG::GLog.ConsHandle = NPTM::GetStdErr();
  }
 InitSyscalls(); 
 InitStartupInfo(StkFrame, ArgA, ArgB, ArgC);
 IFDBG{DbgLogStartupInfo();}
 return 0;// crc;
}

};
//============================================================================================================


