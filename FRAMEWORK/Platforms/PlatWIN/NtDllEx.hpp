
#pragma once


//============================================================================================================
template<typename PHT=PTRCURRENT> struct NNTDEX: public NNTDLL<PHT>    // NUFmtPE
{
// See KUSER_SHARED_DATA in ntddk.h
static _finline uint8* GetUserSharedData(void)
{
 return reinterpret_cast<uint8*>(0x7FFE0000);
}
//---------------------------------------------------------------------------
static _finline bool IsWinXPOrOlder(void)
{
 uint8* pKiUserSharedData = GetUserSharedData();
 return (bool)*(uint32*)pKiUserSharedData;    // Deprecated since 5.2(XP x64) and equals 0
}
//---------------------------------------------------------------------------
static _finline uint64 GetTicks(void)
{
 uint8* pKiUserSharedData = GetUserSharedData();
 return (*(uint16*)pKiUserSharedData)?((uint64)*(uint32*)pKiUserSharedData):(*(uint64*)&pKiUserSharedData[0x320]);
}
//----------------------------------------------------------------------------
static _finline uint64 GetTicksCount(void)
{
 uint8* pKiUserSharedData = GetUserSharedData();
 return (GetTicks() * *(uint32*)&pKiUserSharedData[4]) >> 24;  // 'TickCountLow * TickCountMultiplier' or 'TickCount * TickCountMultiplier'
}
//----------------------------------------------------------------------------
static _finline uint64 GetInterruptTime(void)   // FILETIME
{
 uint8* pKiUserSharedData = GetUserSharedData();
 return *(uint64*)&pKiUserSharedData[0x0008];
}
//----------------------------------------------------------------------------
static _finline uint64 GetTimeZoneBias(void)  // FILETIME
{
 uint8* pKiUserSharedData = GetUserSharedData();
 return *(uint64*)&pKiUserSharedData[0x0020];
}
//----------------------------------------------------------------------------
static _finline uint64 GetSystemTime(void)  // FILETIME
{
 uint8* pKiUserSharedData = GetUserSharedData();
 return *(uint64*)&pKiUserSharedData[0x0014];
}
//----------------------------------------------------------------------------
static _finline uint64 GetLocalTime(void)  // FILETIME
{
 return GetSystemTime() - GetTimeZoneBias();
}
//----------------------------------------------------------------------------
// _ReadBarrier       // Forces memory reads to complete
// _WriteBarrier      // Forces memory writes to complete
// _ReadWriteBarrier  // Block the optimization of reads and writes to global memory
//
/*static inline volatile UINT64 GetAbstractTimeStamp(UINT64 volatile* PrevVal)
{
 volatile UINT64 cval = __rdtsc();
 volatile UINT64 pval = *PrevVal;   // Interlocked.Read
 if(cval <= pval)return pval;       // Sync to increment
 _InterlockedCompareExchange64((INT64*)PrevVal,cval, pval);  // Assign atomically if it is not changed yet  // This is the only one 64bit operand LOCK instruction available on x32 (cmpxchg8b)
 return *PrevVal;   // Return a latest value
} */
//----------------------------------------------------------------------------
static NT::NTSTATUS NtSleep(uint32 dwMiliseconds, bool Alertable=false)
{
 struct
  {
   uint32 Low;
   uint32 Hi;
  } MLI = {{uint32((uint32)-10000 * dwMiliseconds)},{uint32(0xFFFFFFFFu)}};    // Relative time used
 return SAPI::NtDelayExecution(Alertable, (NT::PLARGE_INTEGER)&MLI);
}
//----------------------------------------------------------------------------
static inline uint64 GetNativeWowTebAddrWin10(void)
{
 NT::PTEB teb = NT::NtCurrentTeb();
 if(!teb->WowTebOffset)return 0;  // Not WOW or below Win10  // From 10.0 and higher
 uint8* TebX64 = (uint8*)teb;
 if(long(teb->WowTebOffset) < 0)
  {
   TebX64 = &TebX64[(long)teb->WowTebOffset];  // In WOW processes WowTebOffset is negative in x32 TEB and positive in x64 TEB
   if(*(uint64*)&TebX64[0x30] != (uint64)TebX64)return 0;   // x64 Self
  }
   else if((size_t)(*(uint32*)&TebX64[0x18]) != (size_t)TebX64)return 0;   // x32 Self
 return uint64(TebX64);
}
//---------------------------------------------------------------------------
// Returns 'false' if running under native x32 or native x64
static inline bool IsWow64(void)
{
 if constexpr(!IsArchX64)return NT::NtCurrentTeb()->WOW32Reserved;   // Is it reliable?  Is it always non NULL under Wow64?   // Contains pointer to 'jmp far 33:Addr'
 return false;
}
//---------------------------------------------------------------------------
// Returns base of NTDLL and optionally its path (can be used to get the system drive letter)
//
static vptr GetBaseOfNtdll(wchar* buf=nullptr, uint len=0)
{
 NT::PEB* CurPeb = NT::NtCurrentPeb();
 uint8* AddrInNtDll = (uint8*)CurPeb->FastPebLock;    // Stable?
 NT::PEB_LDR_DATA* ldr = CurPeb->Ldr; //  CurTeb->ProcessEnvironmentBlock->Ldr;
 for(NT::LDR_DATA_TABLE_ENTRY_MO* me = ldr->InMemoryOrderModuleList.Flink;me != (NT::LDR_DATA_TABLE_ENTRY_MO*)&ldr->InMemoryOrderModuleList;me = me->InMemoryOrderLinks.Flink)     // Or just use LdrFindEntryForAddress?
  {
   uint8* DllBeg = (uint8*)me->DllBase;
   uint8* DllEnd = DllBeg + me->SizeOfImage;
   if((AddrInNtDll >= DllBeg)&&(AddrInNtDll < DllEnd))
    {
     if(buf && len)NSTR::StrCopy((wchar*)buf, (wchar*)(me->FullDllName.Buffer), (len > me->FullDllName.Length)?me->FullDllName.Length:len);
     return me->DllBase;
    }
  }
 return nullptr;
}
//---------------------------------------------------------------------------
/*static PVOID LdrGetModuleBase(LPSTR ModName, PULONG Size=NULL, PULONG BaseIdx=NULL)  // NOTE: No loader locking used!
{
 if(!ModName)
  {
   PVOID BaseAddr = NtCurrentTeb()->ProcessEnvironmentBlock->ImageBaseAddress;
   if(Size)
    {
     PPEB_LDR_DATA ldr = NtCurrentTeb()->ProcessEnvironmentBlock->Ldr;
     for(LDR_DATA_TABLE_ENTRY_MO* me = ldr->InMemoryOrderModuleList.Flink;me != (LDR_DATA_TABLE_ENTRY_MO*)&ldr->InMemoryOrderModuleList;me = me->InMemoryOrderLinks.Flink)     // Or just use LdrFindEntryForAddress?
      {
       if(BaseAddr == me->DllBase){*Size = me->SizeOfImage; break;}
      }
    }
   return BaseAddr;
  }
 PPEB_LDR_DATA ldr = NtCurrentTeb()->ProcessEnvironmentBlock->Ldr;
// DBGMSG("PEB_LDR_DATA: %p, %s",ldr,ModName);     // TODO: Use only in FULL info mode
 long StartIdx = (BaseIdx)?(*BaseIdx + 1):0;
 long CurrIdx  = 0;
 for(LDR_DATA_TABLE_ENTRY_MO* me = ldr->InMemoryOrderModuleList.Flink;me != (LDR_DATA_TABLE_ENTRY_MO*)&ldr->InMemoryOrderModuleList;CurrIdx++,me = me->InMemoryOrderLinks.Flink)     // Or just use LdrFindEntryForAddress?
  {
   if(!me->BaseDllName.Length || !me->BaseDllName.Buffer)continue;
   if(CurrIdx < StartIdx)continue;
//   DBGMSG("Base=%p, Name='%ls'",me->DllBase,me->BaseDllName.Buffer);    // Zero terminated?     // Spam
   bool Match = true;
   UINT ctr = 0;
   for(UINT tot=me->BaseDllName.Length/sizeof(WCHAR);ctr < tot;ctr++)
    {
     if(me->BaseDllName.Buffer[ctr] != (WCHAR)ModName[ctr]){Match=false; break;}
    }
   if(Match && !ModName[ctr])
    {
     if(BaseIdx)*BaseIdx = CurrIdx;
     if(Size)*Size = me->SizeOfImage;
     return me->DllBase;
    }
  }
// DBGMSG("Not found for: %s",ModName);   // TODO: Use only in FULL info mode
 return NULL;
}*/
//------------------------------------------------------------------------------------
/*static sint LdrGetModuleName(PVOID ModAddr, achar* ModName, uint MaxSize=-1, bool Full=true)  // NOTE: No loader locking is used!
{
 PPEB_LDR_DATA ldr = NtCurrentTeb()->ProcessEnvironmentBlock->Ldr;
// DBGMSG("PEB_LDR_DATA: %p, %p",ldr,ModBase);
 for(LDR_DATA_TABLE_ENTRY_MO* me = ldr->InMemoryOrderModuleList.Flink;me != (LDR_DATA_TABLE_ENTRY_MO*)&ldr->InMemoryOrderModuleList;me = me->InMemoryOrderLinks.Flink)     // Or just use LdrFindEntryForAddress?
  {
   if(me->DllBase != ModBase)continue;
   if(!me->BaseDllName.Length || !me->BaseDllName.Buffer){*ModName=0; return 0;}    // No path!
//   DBGMSG("Base=%p, Name='%ls'",me->DllBase,me->BaseDllName.Buffer);    // Zero terminated?     // Spam
   UINT Index = 0;
   MaxSize--;  // For terminating 0
   PUNICODE_STRING UStr = Full?&me->FullDllName:&me->BaseDllName;
   for(UINT tot=UStr->Length/sizeof(WCHAR);(Index < tot) && MaxSize;Index++,MaxSize--)ModName[Index] = UStr->Buffer[Index];
   ModName[Index] = 0;
   return Index;
  }
// DBGMSG("Not found for: %p",ModBase);
 *ModName = 0;
 return -1;
}*/
//------------------------------------------------------------------------------------
static vptr LdrGetModuleByAddr(vptr ModAddr, size_t* ModSize=nullptr)
{
 NT::PPEB_LDR_DATA ldr = NT::NtCurrentTeb()->ProcessEnvironmentBlock->Ldr;    // TODO: Loader lock
 for(NT::LDR_DATA_TABLE_ENTRY_MO* me = ldr->InMemoryOrderModuleList.Flink;me != (NT::LDR_DATA_TABLE_ENTRY_MO*)&ldr->InMemoryOrderModuleList;me = me->InMemoryOrderLinks.Flink)     // Or just use LdrFindEntryForAddress?
  {
   if(((uint8*)ModAddr < (uint8*)me->DllBase) || ((uint8*)ModAddr >= ((uint8*)me->DllBase + me->SizeOfImage)))continue;
   if(ModSize)*ModSize = me->SizeOfImage;
   return me->DllBase;
  }
 return nullptr;
}
//------------------------------------------------------------------------------------
// Replaces '/' with '\'
// Removes any '.\'
// Removes excess '\' duplicates
// Excludes from the path directories that preceeded by '..\' up until root directory '\'
// So the output path can only become shorter
//
template<typename T> static uint NormalizePathNt(T Src, T Dst=nullptr)
{
 if(!Dst)Dst = Src;
 sint SrcOffs = 0;
 uint DstOffs = 0;
 sint LastSep = -1;
 uint LstSep0 = 0;
 uint LstSep1 = 0;
 for(sint DotCtr=0;;SrcOffs++)
  {
   uint32 val = Src[SrcOffs];
   if(!val)break;
   if(val == '/')val = '\\';
   if(val == '\\')
    {
     sint slen = ((SrcOffs-1) - LastSep);
     LastSep = SrcOffs;
     if(!slen && SrcOffs)continue;    // Skip sequential '\\'
     if(DotCtr)
      {
       if((DotCtr <= 2) && (slen == DotCtr))  // Check that we have only dots between separators   // When we got a separator then we check if we have '.' or '..'
        {
         if(DotCtr == 1){DstOffs = LstSep0; LstSep0 = LstSep1;}   // Just remove '.\'
         else if(DotCtr == 2){DstOffs = LstSep1;}                 // Remove 'SomeDir\..\'
        }
       DotCtr = 0;
      }
     LstSep1 = LstSep0;
     LstSep0 = DstOffs;
    }
   else if(val == '.')DotCtr++;
   Dst[DstOffs++] = (decltype(*Dst))val;
  }
 Dst[DstOffs] = 0;
 return DstOffs;
}
//------------------------------------------------------------------------------------
enum EPathType {ptUnknown=0,ptAbsolute=1,ptSysRootRel=2,ptCurrDirRel=3};

template<typename T> static EPathType GetPathTypeNt(T Src)
{
 if(Src[1] == ':')return ptAbsolute;   // 'C:\'
 if((Src[0] == '/') || (Src[0] == '\\'))return ptSysRootRel;  // \MyHomeFolder
 //if((Src[0] == '.') && ((Src[1] == '/') || (Src[1] == '\\')))return ptCurrDirRel;    // .\MyDrkDir
 return ptCurrDirRel;  //ptUnknown;
}
//------------------------------------------------------------------------------------
static _finline size_t CalcFilePathBufSize(const achar* Path, uint& plen, EPathType& ptype)
{
 plen  = NSTR::StrLen(Path);
 ptype = NTX::GetPathTypeNt(Path);
 uint ExtraLen = 4+10;  // +10 for size of "\\GLOBAL??\\"
 NT::UNICODE_STRING* CurrDir = &NT::NtCurrentTeb()->ProcessEnvironmentBlock->ProcessParameters->CurrentDirectory.DosPath;    // 'C:\xxxxx\yyyyy\'
 if(ptype == NTX::ptSysRootRel)ExtraLen += NSTR::StrLen((wchar*)&fwsinf.SysDrive);
 else if(ptype == NTX::ptCurrDirRel)ExtraLen += CurrDir->Length / sizeof(wchar);
 return (plen*4)+ExtraLen;
}
//------------------------------------------------------------------------------------
static _finline void InitFileObjectAttributes(const achar* Path, uint plen, EPathType ptype, uint32 ObjAttributes, NT::UNICODE_STRING* buf_ustr, wchar* buf_path, NT::OBJECT_ATTRIBUTES* oattr, NT::HANDLE RootDir=0)
{
 uint DstLen = NSTR::StrCopy(buf_path, L"\\GLOBAL??\\");     // Windows XP?
 NT::UNICODE_STRING* CurrDir = &NT::NtCurrentTeb()->ProcessEnvironmentBlock->ProcessParameters->CurrentDirectory.DosPath;
 if(ptype == NTX::ptSysRootRel)DstLen += NSTR::StrCopy(&buf_path[DstLen], (wchar*)&fwsinf.SysDrive);
 else if(ptype == NTX::ptCurrDirRel)DstLen += NSTR::StrCopy(&buf_path[DstLen], (wchar*)CurrDir->Buffer);
 DstLen += NUTF::Utf8To16(&buf_path[DstLen], Path, plen);
 buf_path[DstLen] = 0;
 DstLen  = NTX::NormalizePathNt(buf_path);
 buf_path[DstLen] = 0;
 buf_ustr->Set(buf_path, DstLen);

 oattr->Length = sizeof(NT::OBJECT_ATTRIBUTES);
 oattr->RootDirectory = RootDir;
 oattr->ObjectName = buf_ustr;
 oattr->Attributes = ObjAttributes;            // OBJ_CASE_INSENSITIVE;   //| OBJ_KERNEL_HANDLE;
 oattr->SecurityDescriptor = nullptr;          // TODO: Arg3: mode_t mode
 oattr->SecurityQualityOfService = nullptr;
}
//------------------------------------------------------------------------------------
static NT::NTSTATUS OpenFileObject(NT::PHANDLE FileHandle, const achar* Path, NT::ACCESS_MASK DesiredAccess, NT::ULONG ObjAttributes, NT::ULONG FileAttributes, NT::ULONG ShareAccess, NT::ULONG CreateDisposition, NT::ULONG CreateOptions, NT::PIO_STATUS_BLOCK IoStatusBlock, NT::HANDLE RootDir=0)
{
 NT::OBJECT_ATTRIBUTES oattr = {};
 NT::UNICODE_STRING FilePathUS;

 uint plen;
 NTX::EPathType ptype;
 uint PathLen = CalcFilePathBufSize(Path, plen, ptype);

 wchar FullPath[PathLen];
 InitFileObjectAttributes(Path, plen, ptype, ObjAttributes, &FilePathUS, FullPath, &oattr, RootDir);
 return SAPI::NtCreateFile(FileHandle, DesiredAccess, &oattr, IoStatusBlock, nullptr, FileAttributes, ShareAccess, CreateDisposition, CreateOptions, nullptr, 0);
}
//------------------------------------------------------------------------------------
/*
 https://stackoverflow.com/questions/46473507/delete-folder-for-which-every-api-call-fails-with-error-access-denied

for delete file enough 2 things - we have FILE_DELETE_CHILD on parent folder. and file is not read only.
in this case call ZwDeleteFile (but not DeleteFile or RemoveDirectory - both this api will fail if file have empty DACL) is enough.
in case file have read-only attribute - ZwDeleteFile fail with code STATUS_CANNOT_DELETE. in this case we need first remove read only.
for this need open file with FILE_WRITE_ATTRIBUTES access. we can do this if we have SE_RESTORE_PRIVILEGE and set FILE_OPEN_FOR_BACKUP_INTENT option in call to ZwOpenFile.

A file in the file system is basically a link to an inode.
A hard link, then, just creates another file with a link to the same underlying inode.
When you delete a file, it removes one link to the underlying inode. The inode is only deleted (or deletable/over-writable) when all links to the inode have been deleted.

Once a hard link has been made the link is to the inode. Deleting, renaming, or moving the original file will not affect the hard link as it links to the underlying inode.
Any changes to the data on the inode is reflected in all files that refer to that inode.

https://superuser.com/questions/343074/directory-junction-vs-directory-symbolic-link

Default behaviour:
 NtDeleteFile("C:/TST/TestDirJunction");       // Reparse point          // Deleted target directory itself, not the link!
 NtDeleteFile("C:/TST/TestDirSoftLink");       // Reparse point          // Deleted target directory itself, not the link!
 NtDeleteFile("C:/TST/TestFileSoftLink");      // Reparse point          // Deleted target file itself, not the link!
 NtDeleteFile("C:/TST/TestFileHardLink.ini");  // A Link to same INODE   // Deleted the link, not a target file!

IoFileObjectType           // NtDeleteFile (CreateOptions = FILE_DELETE_ON_CLOSE, DeleteOnly)  // IoFileObjectType: InvalidAttributes is OBJ_PERMANENT | OBJ_EXCLUSIVE | OBJ_OPENLINK
ObpDirectoryObjectType
ObpSymbolicLinkObjectType

NtOpenSymbolicLinkObject
 NtDeleteFile is much simpler and faster but it is not used by DeleteFileW (Because it just follows a file obect`s name by any link?)
*/
static NT::NTSTATUS DeleteFileObject(const achar* Path, bool Force, int AsDir)     // NtDeleteFile
{
 NT::OBJECT_ATTRIBUTES oattr = {};
 NT::UNICODE_STRING FilePathUS;

 uint plen;
 NTX::EPathType ptype;
 uint PathLen = CalcFilePathBufSize(Path, plen, ptype);
 uint32 ObjAttributes = NT::OBJ_DONT_REPARSE;  //  Symlinks are implemented as Reparse Points   // Only Hard Links to a file can be safely deleted without this
 wchar FullPath[PathLen];
 InitFileObjectAttributes(Path, plen, ptype, ObjAttributes, &FilePathUS, FullPath, &oattr);
 NT::NTSTATUS Status = SAPI::NtDeleteFile(&oattr);     // Deletes files and directories(empty)  // ObOpenObjectByNameEx as
 if(Status != NT::STATUS_REPARSE_POINT_ENCOUNTERED)return Status;   // Succeeded or failed

 // A Symlink encountered on the path
/*
 HANDLE hFile;
 OBJECT_ATTRIBUTES attr;
 IO_STATUS_BLOCK iost;
 FILE_DISPOSITION_INFORMATION fDispositionInfo;
 FILE_BASIC_INFORMATION fBasicInfo;
 UNICODE_STRING TargetFileName;

 RtlInitUnicodeString(&TargetFileName, FileName);
 InitializeObjectAttributes(&attr, &TargetFileName, OBJ_CASE_INSENSITIVE, 0, 0);
 NTSTATUS Status = NtOpenFile(&hFile, DELETE | FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES, &attr, &iost, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, FILE_NON_DIRECTORY_FILE | FILE_OPEN_FOR_BACKUP_INTENT);
 if(NT_SUCCESS(Status))
  {
   Status = NtQueryInformationFile(hFile, &iost, &fBasicInfo, sizeof(FILE_BASIC_INFORMATION), FileBasicInformation);
   if(NT_SUCCESS(Status))
    {
     fBasicInfo.FileAttributes = FILE_ATTRIBUTE_NORMAL;
     NtSetInformationFile(hFile, &iost, &fBasicInfo, sizeof(FILE_BASIC_INFORMATION), FileBasicInformation);
    }
   fDispositionInfo.DeleteFile = TRUE;  // puts the file into a "delete pending" state. It will be deleted once all existing handles are closed. No new handles will be possible to open
   Status = NtSetInformationFile(hFile, &iost, &fDispositionInfo, sizeof(FILE_DISPOSITION_INFORMATION), FileDispositionInformation);
   NtClose(hFile);
  }  */
 return Status;
}
//------------------------------------------------------------------------------------
static NT::NTSTATUS DeleteSymbolicLinkObject(const achar* Path)
{
/*if (NtOpenSymbolicLinkObject( &handle, DELETE, &objectAttributes) == STATUS_SUCCESS)
{
    NtMakeTemporaryObject( handle);
    NtClose( handle);
} */
 return 0;
}
//------------------------------------------------------------------------------------
// On some hardware architectures (e.g., i386), PROT_WRITE implies PROT_READ.
// It is architecture dependent whether PROT_READ implies PROT_EXEC or not.
// Portable programs should always set PROT_EXEC if they intend to execute code in the new mapping.
//
// Works for memory allocation/protection and file mapping
//
static uint32 MemProtPXtoNT(uint32 prot)
{
 uint32 PageProt = 0;
 if(prot & PX::PROT_EXEC)     // Use EXEC group of flags
  {
   if(prot & PX::PROT_WRITE)
    {
     if(prot & PX::PROT_READ)PageProt |= NT::PAGE_EXECUTE_READWRITE;
       else PageProt |= NT::PAGE_EXECUTE_WRITECOPY;
    }
   else if(prot & PX::PROT_READ)PageProt |= NT::PAGE_EXECUTE_READ;
   else PageProt |= NT::PAGE_EXECUTE;
  }
 else   // Use ordinary R/W group of flags
  {
   if(prot & PX::PROT_WRITE)
    {
     if(prot & PX::PROT_READ)PageProt |= NT::PAGE_READWRITE;
       else PageProt |= NT::PAGE_WRITECOPY;
    }
   else
    {
     if(prot & PX::PROT_READ)PageProt |= NT::PAGE_READONLY;
      else PageProt |= NT::PAGE_NOACCESS;
    }
  }
 return PageProt;
}
//------------------------------------------------------------------------------------
// From MingW
static uint NTStatusToLinuxErr(NT::NTSTATUS err, uint Default=PX::EPERM)
{
 if(!err)return PX::NOERROR;
 switch((uint)err)
  {
  case NT::STATUS_ABANDONED:
//  case NT::STATUS_ABANDONED_WAIT_0:
    return PX::ECHILD;

  case NT::STATUS_TIMEOUT:
  case NT::STATUS_DEVICE_BUSY:
  case NT::STATUS_SHARING_VIOLATION:
  case NT::STATUS_FILE_LOCK_CONFLICT:
  case NT::STATUS_LOCK_NOT_GRANTED:
  case NT::STATUS_INSTANCE_NOT_AVAILABLE:
  case NT::STATUS_PIPE_NOT_AVAILABLE:
  case NT::STATUS_PIPE_BUSY:
  case NT::STATUS_PIPE_CONNECTED:
    return PX::EBUSY;

  case NT::STATUS_PENDING:
  case NT::STATUS_ALREADY_DISCONNECTED:
  case NT::STATUS_PAGEFILE_QUOTA:
  case NT::STATUS_WORKING_SET_QUOTA:
  case NT::STATUS_FILES_OPEN:
  case NT::STATUS_CONNECTION_IN_USE:
  case NT::STATUS_COMMITMENT_LIMIT:
    return PX::EAGAIN;

  case NT::STATUS_MORE_ENTRIES:
  case NT::STATUS_BUFFER_OVERFLOW:
  case NT::STATUS_MORE_PROCESSING_REQUIRED:
    return PX::EMSGSIZE;

  case NT::STATUS_WORKING_SET_LIMIT_RANGE:
  case NT::STATUS_INVALID_EA_NAME:
  case NT::STATUS_EA_LIST_INCONSISTENT:
  case NT::STATUS_INVALID_EA_FLAG:
  case NT::STATUS_INVALID_INFO_CLASS:
  case NT::STATUS_INVALID_CID:
  case NT::STATUS_INVALID_PARAMETER:
  case NT::STATUS_NONEXISTENT_SECTOR:
  case NT::STATUS_CONFLICTING_ADDRESSES:
  case NT::STATUS_NOT_MAPPED_VIEW:
  case NT::STATUS_UNABLE_TO_FREE_VM:
  case NT::STATUS_UNABLE_TO_DELETE_SECTION:
  case NT::STATUS_UNABLE_TO_DECOMMIT_VM:
  case NT::STATUS_NOT_COMMITTED:
  case NT::STATUS_INVALID_PARAMETER_MIX:
  case NT::STATUS_INVALID_PAGE_PROTECTION:
  case NT::STATUS_PORT_ALREADY_SET:
  case NT::STATUS_SECTION_NOT_IMAGE:
  case NT::STATUS_BAD_WORKING_SET_LIMIT:
  case NT::STATUS_INCOMPATIBLE_FILE_MAP:
  case NT::STATUS_SECTION_PROTECTION:
  case NT::STATUS_EA_TOO_LARGE:
  case NT::STATUS_NONE_MAPPED:
  case NT::STATUS_NO_TOKEN:
  case NT::STATUS_NOT_MAPPED_DATA:
  case NT::STATUS_FREE_VM_NOT_AT_BASE:
  case NT::STATUS_MEMORY_NOT_ALLOCATED:
  case NT::STATUS_BAD_MASTER_BOOT_RECORD:
  case NT::STATUS_INVALID_PIPE_STATE:
  case NT::STATUS_INVALID_READ_MODE:
  case NT::STATUS_INVALID_PARAMETER_1:
  case NT::STATUS_INVALID_PARAMETER_2:
  case NT::STATUS_INVALID_PARAMETER_3:
  case NT::STATUS_INVALID_PARAMETER_4:
  case NT::STATUS_INVALID_PARAMETER_5:
  case NT::STATUS_INVALID_PARAMETER_6:
  case NT::STATUS_INVALID_PARAMETER_7:
  case NT::STATUS_INVALID_PARAMETER_8:
  case NT::STATUS_INVALID_PARAMETER_9:
  case NT::STATUS_INVALID_PARAMETER_10:
  case NT::STATUS_INVALID_PARAMETER_11:
  case NT::STATUS_INVALID_PARAMETER_12:
  case NT::STATUS_DEVICE_CONFIGURATION_ERROR:
  case NT::STATUS_FAIL_CHECK:
    return PX::EINVAL;

  case NT::STATUS_DATATYPE_MISALIGNMENT:
  case NT::STATUS_ACCESS_VIOLATION:
  case NT::STATUS_DATATYPE_MISALIGNMENT_ERROR:
    return PX::EFAULT;

/*  case NT::STATUS_NO_MORE_FILES:
  case NT::STATUS_NO_MORE_EAS:
  case NT::STATUS_NO_MORE_ENTRIES:
  case NT::STATUS_GUIDS_EXHAUSTED:
  case NT::STATUS_AGENTS_EXHAUSTED:
    return PX::ENMFILE;  */

/*  case NT::STATUS_DEVICE_POWERED_OFF:
  case NT::STATUS_DEVICE_OFF_LINE:
  case NT::STATUS_NO_MEDIA_IN_DEVICE:
  case NT::STATUS_DEVICE_POWER_FAILURE:
  case NT::STATUS_DEVICE_NOT_READY:
  case NT::STATUS_NO_MEDIA:
  case NT::STATUS_VOLUME_DISMOUNTED:
  case NT::STATUS_POWER_STATE_INVALID:
    return PX::ENOMEDIUM;   */

  case NT::STATUS_FILEMARK_DETECTED:
  case NT::STATUS_BUS_RESET:
  case NT::STATUS_BEGINNING_OF_MEDIA:
  case NT::STATUS_SETMARK_DETECTED:
  case NT::STATUS_NO_DATA_DETECTED:
  case NT::STATUS_DISK_CORRUPT_ERROR:
  case NT::STATUS_DATA_OVERRUN:
  case NT::STATUS_DATA_LATE_ERROR:
  case NT::STATUS_DATA_ERROR:
  case NT::STATUS_CRC_ERROR:
  case NT::STATUS_QUOTA_EXCEEDED:
  case NT::STATUS_SUSPEND_COUNT_EXCEEDED:
  case NT::STATUS_DEVICE_DATA_ERROR:
  case NT::STATUS_UNEXPECTED_NETWORK_ERROR:
  case NT::STATUS_UNEXPECTED_IO_ERROR:
  case NT::STATUS_LINK_FAILED:
  case NT::STATUS_LINK_TIMEOUT:
  case NT::STATUS_INVALID_CONNECTION:
  case NT::STATUS_INVALID_ADDRESS:
  case NT::STATUS_FT_MISSING_MEMBER:
  case NT::STATUS_FT_ORPHANING:
  case NT::STATUS_INVALID_BLOCK_LENGTH:
  case NT::STATUS_EOM_OVERFLOW:
  case NT::STATUS_DRIVER_INTERNAL_ERROR:
  case NT::STATUS_IO_DEVICE_ERROR:
  case NT::STATUS_DEVICE_PROTOCOL_ERROR:
  case NT::STATUS_USER_SESSION_DELETED:
  case NT::STATUS_TRANSACTION_ABORTED:
  case NT::STATUS_TRANSACTION_TIMED_OUT:
  case NT::STATUS_TRANSACTION_NO_RELEASE:
  case NT::STATUS_TRANSACTION_NO_MATCH:
  case NT::STATUS_TRANSACTION_RESPONDED:
  case NT::STATUS_TRANSACTION_INVALID_ID:
  case NT::STATUS_TRANSACTION_INVALID_TYPE:
  case NT::STATUS_DEVICE_REQUIRES_CLEANING:
  case NT::STATUS_DEVICE_DOOR_OPEN:
    return PX::EIO;

  case NT::STATUS_END_OF_MEDIA:
  case NT::STATUS_DISK_FULL:
    return PX::ENOSPC;

  case NT::STATUS_NOT_IMPLEMENTED:
  case NT::STATUS_INVALID_DEVICE_REQUEST:
  case NT::STATUS_INVALID_SYSTEM_SERVICE:
  case NT::STATUS_ILLEGAL_FUNCTION:
  case NT::STATUS_VOLUME_NOT_UPGRADED:
  case NT::DBG_NO_STATE_CHANGE:
    return PX::EBADRQC;

  case NT::STATUS_INVALID_HANDLE:
  case NT::STATUS_OBJECT_TYPE_MISMATCH:
  case NT::STATUS_PORT_DISCONNECTED:
  case NT::STATUS_INVALID_PORT_HANDLE:
  case NT::STATUS_FILE_CLOSED:
  case NT::STATUS_HANDLE_NOT_CLOSABLE:
  case NT::RPC_NT_INVALID_BINDING:
  case NT::RPC_NT_SS_IN_NULL_CONTEXT:
  case NT::RPC_NT_SS_CONTEXT_MISMATCH:
    return PX::EBADF;

  case NT::STATUS_BAD_INITIAL_PC:
  case NT::STATUS_INVALID_FILE_FOR_SECTION:
  case NT::STATUS_INVALID_IMAGE_FORMAT:
  case NT::STATUS_INVALID_IMAGE_NE_FORMAT:
  case NT::STATUS_INVALID_IMAGE_LE_FORMAT:
  case NT::STATUS_INVALID_IMAGE_NOT_MZ:
  case NT::STATUS_INVALID_IMAGE_PROTECT:
  case NT::STATUS_INVALID_IMAGE_WIN_16:
  case NT::STATUS_IMAGE_CHECKSUM_MISMATCH:
  case NT::STATUS_IMAGE_MP_UP_MISMATCH:
  case NT::STATUS_INVALID_IMAGE_WIN_32:
  case NT::STATUS_INVALID_IMAGE_WIN_64:
    return PX::ENOEXEC;

  case NT::STATUS_NO_SUCH_FILE:
  case NT::STATUS_OBJECT_NAME_INVALID:
  case NT::STATUS_OBJECT_NAME_NOT_FOUND:
  case NT::STATUS_OBJECT_PATH_INVALID:
  case NT::STATUS_OBJECT_PATH_NOT_FOUND:
  case NT::STATUS_OBJECT_PATH_SYNTAX_BAD:
  case NT::STATUS_DFS_EXIT_PATH_FOUND:
  case NT::STATUS_BAD_NETWORK_PATH:
  case NT::STATUS_DEVICE_DOES_NOT_EXIST:
  case NT::STATUS_NETWORK_NAME_DELETED:
  case NT::STATUS_BAD_NETWORK_NAME:
  case NT::STATUS_REDIRECTOR_NOT_STARTED:
  case NT::STATUS_DLL_NOT_FOUND:
  case NT::STATUS_LOCAL_DISCONNECT:
  case NT::STATUS_REMOTE_DISCONNECT:
  case NT::STATUS_ADDRESS_CLOSED:
  case NT::STATUS_CONNECTION_DISCONNECTED:
  case NT::STATUS_CONNECTION_RESET:
  case NT::STATUS_DIRECTORY_IS_A_REPARSE_POINT:
  case NT::STATUS_OBJECTID_NOT_FOUND:
  case NT::DBG_APP_NOT_IDLE:
    return PX::ENOENT;

  case NT::STATUS_END_OF_FILE:
  case NT::STATUS_FILE_FORCED_CLOSED:
    return PX::ENODATA;

  case NT::STATUS_NO_MEMORY:
  case NT::STATUS_SECTION_TOO_BIG:
  case NT::STATUS_SECTION_NOT_EXTENDED:
  case NT::STATUS_TOO_MANY_PAGING_FILES:
    return PX::ENOMEM;

  case NT::STATUS_INVALID_LOCK_SEQUENCE:
  case NT::STATUS_INVALID_VIEW_SIZE:
  case NT::STATUS_ALREADY_COMMITTED:
  case NT::STATUS_ACCESS_DENIED:
  case NT::STATUS_PORT_CONNECTION_REFUSED:
  case NT::STATUS_THREAD_IS_TERMINATING:
  case NT::STATUS_DELETE_PENDING:
  case NT::STATUS_FILE_IS_A_DIRECTORY:
  case NT::STATUS_FILE_RENAMED:
  case NT::STATUS_PROCESS_IS_TERMINATING:
  case NT::STATUS_CANNOT_DELETE:
  case NT::STATUS_FILE_DELETED:
  case NT::STATUS_ENCRYPTION_FAILED:
  case NT::STATUS_DECRYPTION_FAILED:
  case NT::STATUS_NO_RECOVERY_POLICY:
  case NT::STATUS_NO_EFS:
  case NT::STATUS_WRONG_EFS:
  case NT::STATUS_NO_USER_KEYS:
    return PX::EACCES;

  case NT::STATUS_OBJECT_NAME_COLLISION:
  case NT::STATUS_NONEXISTENT_EA_ENTRY:
  case NT::STATUS_NO_EAS_ON_FILE:
  case NT::STATUS_EA_CORRUPT_ERROR:
  case NT::STATUS_FILE_CORRUPT_ERROR:
    return PX::EEXIST;

  case NT::STATUS_MUTANT_NOT_OWNED:
  case NT::STATUS_PRIVILEGE_NOT_HELD:
  case NT::STATUS_RESOURCE_NOT_OWNED:
  case NT::STATUS_CANNOT_MAKE:
    return PX::EPERM;

//  case NT::STATUS_EAS_NOT_SUPPORTED:
//    return PX::ENOTSUP;

  case NT::STATUS_CTL_FILE_NOT_SUPPORTED:
  case NT::STATUS_NOT_SUPPORTED:
  case NT::STATUS_INVALID_NETWORK_RESPONSE:
  case NT::STATUS_NOT_SERVER_SESSION:
  case NT::STATUS_NOT_CLIENT_SESSION:
  case NT::STATUS_WMI_NOT_SUPPORTED:
    return PX::ENOSYS;

  case NT::STATUS_PROCEDURE_NOT_FOUND:
  case NT::STATUS_ENTRYPOINT_NOT_FOUND:
  case NT::STATUS_DRIVER_ENTRYPOINT_NOT_FOUND:
    return PX::ESRCH;

  case NT::STATUS_FILE_INVALID:
  case NT::STATUS_MAPPED_FILE_SIZE_ZERO:
    return PX::ENXIO;

  case NT::STATUS_INSUFFICIENT_RESOURCES:
    return PX::EFBIG;

  case NT::STATUS_MEDIA_WRITE_PROTECTED:
  case NT::STATUS_TOO_LATE:
    return PX::EROFS;

  case NT::STATUS_PIPE_DISCONNECTED:
  case NT::STATUS_PIPE_LISTENING:
    return PX::ECOMM;

  case NT::STATUS_PIPE_CLOSING:
  case NT::STATUS_PIPE_EMPTY:
  case NT::STATUS_PIPE_BROKEN:
    return PX::EPIPE;

  case NT::STATUS_REMOTE_NOT_LISTENING:
  case NT::STATUS_REMOTE_RESOURCES:
    return PX::ENONET;

  case NT::STATUS_DUPLICATE_NAME:
  case NT::STATUS_ADDRESS_ALREADY_EXISTS:
    return PX::ENOTUNIQ;

  case NT::STATUS_NOT_SAME_DEVICE:
    return PX::EXDEV;

  case NT::STATUS_DIRECTORY_NOT_EMPTY:
    return PX::ENOTEMPTY;

  case NT::STATUS_NOT_A_DIRECTORY:
    return PX::ENOTDIR;

  case NT::STATUS_NAME_TOO_LONG:
    return PX::ENAMETOOLONG;

  case NT::STATUS_TOO_MANY_OPENED_FILES:
    return PX::EMFILE;

  case NT::STATUS_POSSIBLE_DEADLOCK:
    return PX::EDEADLOCK;

  case NT::STATUS_CONNECTION_REFUSED:
    return PX::ECONNREFUSED;

  case NT::STATUS_TOO_MANY_LINKS:
    return PX::EMLINK;
  }
 if(!(err & 0xC0000000))return PX::NOERROR;    // Unknown Status, not an error
 return Default;
}
//------------------------------------------------------------------------------------


};

using NTX = NNTDEX<uint>;
//============================================================================================================
