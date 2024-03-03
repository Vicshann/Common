
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
/*static int SetSkipThreadReport(HANDLE ThLocalThread)
{
 THREAD_BASIC_INFORMATION tinf;
 ULONG RetLen = 0;
 HRESULT res  = NtQueryInformationThread(ThLocalThread,ThreadBasicInformation,&tinf,sizeof(THREAD_BASIC_INFORMATION),&RetLen);
 if(res)return -1; 
 return SetSkipThreadReport(tinf.TebBaseAddress);  
} 
//---------------------------------------------------------------------------
static int SetSkipThreadReport(PTEB ThTeb)  // See RtlIsCurrentThreadAttachExempt
{
 PBYTE TebAddr = (PBYTE)ThTeb;
 for(int idx=0;idx < 2;idx++)
  {
   bool IsTgtTebX32 = (*(PBYTE*)&TebAddr[0x18] == TebAddr);  // NT_TIB
   bool IsTgtTebX64 = (*(PBYTE*)&TebAddr[0x30] == TebAddr);  // NT_TIB
   if(!IsTgtTebX32 && !IsTgtTebX64)return -2;

   long WowTebOffset;
   USHORT* pSameTebFlags;
   if(IsTgtTebX32)
    {
     WowTebOffset  = *(long*)&TebAddr[0x0FDC];
     pSameTebFlags = (USHORT*)&TebAddr[0x0FCA]; 
    }
     else
      {
       WowTebOffset  = *(long*)&TebAddr[0x180C];
       pSameTebFlags = (USHORT*)&TebAddr[0x17EE];  
      }
   *pSameTebFlags |= 0x0008;   // SkipThreadAttach
   *pSameTebFlags &= ~0x0020;  // RanProcessInit   
   DBGMSG("TEB=%p, pSameTebFlags=%p, IsTgtTebX32=%u, IsTgtTebX64=%u, WowTebOffset=%i", TebAddr, pSameTebFlags, IsTgtTebX32, IsTgtTebX64, WowTebOffset);
   if(WowTebOffset == 0)break;
   TebAddr = &TebAddr[WowTebOffset];  // Next WOW TEB
  }
 return 0;
} */
//------------------------------------------------------------------------------------
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
/*static NTSTATUS CreateNtObjDirectory(PWSTR ObjDirName, PHANDLE phDirObj)   // Create objects directory with NULL security
{
 wchar_t Path[512] = {'\\'}; 
 UNICODE_STRING ObjectNameUS;
 SECURITY_DESCRIPTOR  sd = {SECURITY_DESCRIPTOR_REVISION, 0, 4};    // NULL security descriptor: InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION); SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE);
 OBJECT_ATTRIBUTES oattr = { sizeof(OBJECT_ATTRIBUTES), 0, &ObjectNameUS, OBJ_CASE_INSENSITIVE|OBJ_OPENIF, &sd };
 UINT Length = 1;   
 for(;*ObjDirName;ObjDirName++)Path[Length++] = *ObjDirName;
 ObjectNameUS.Buffer = Path;
 ObjectNameUS.Length = Length * sizeof(wchar_t);
 ObjectNameUS.MaximumLength = ObjectNameUS.Length + sizeof(wchar_t);
 return NtCreateDirectoryObject(phDirObj, DIRECTORY_ALL_ACCESS, &oattr);
}
//----------------------------------------------------------------------------
static NTSTATUS FileCreateSync(PWSTR FileName, ACCESS_MASK DesiredAccess, ULONG FileAttributes, ULONG ShareAccess, ULONG CreateDisposition, ULONG CreateOptions, PHANDLE FileHandle)
{
 IO_STATUS_BLOCK iosb = {};
 OBJECT_ATTRIBUTES oattr = {};
 UNICODE_STRING FilePathUS;
 UINT Length = 11;
 wchar_t Path[512] = {'\\','?','?','\\','G','l','o','b','a','l','\\'};
 for(int idx=0;*FileName;FileName++,Length++)Path[Length] = *FileName;
 FilePathUS.Buffer = Path;
 FilePathUS.Length = Length * sizeof(wchar_t);
 FilePathUS.MaximumLength = FilePathUS.Length + sizeof(wchar_t);
 oattr.Length = sizeof(OBJECT_ATTRIBUTES);
 oattr.RootDirectory = NULL;
 oattr.ObjectName = &FilePathUS;
 oattr.Attributes =  OBJ_CASE_INSENSITIVE;   //| OBJ_KERNEL_HANDLE;
 oattr.SecurityDescriptor = NULL;
 oattr.SecurityQualityOfService = NULL;
 return NtCreateFile(FileHandle, DesiredAccess|SYNCHRONIZE, &oattr, &iosb, NULL, FileAttributes, ShareAccess, CreateDisposition, CreateOptions|FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);
}
//------------------------------------------------------------------------------------
static void NtEndSystem(void)  // Requires debug privileges
{            
 DWORD Value = TRUE;  
 NtCurrentTeb()->ProcessEnvironmentBlock->NtGlobalFlag |= 0x100000;    // ???
 NtSetInformationProcess(NtCurrentProcess,ProcessBreakOnTermination,&Value,sizeof(Value));
 NtTerminateProcess(NtCurrentProcess, 0xBAADC0DE);  
}
//------------------------------------------------------------------------------------
static BOOL NTAPI DeviceIsRunning(IN PWSTR DeviceName)
{
 UNICODE_STRING str;
 OBJECT_ATTRIBUTES attr;
 HANDLE hLink;
 BOOL result = FALSE;

 RtlInitUnicodeString(&str, DeviceName);   // TODO: Replace with macro
 InitializeObjectAttributes(&attr, &str, OBJ_CASE_INSENSITIVE, 0, 0);
 NTSTATUS Status = NtOpenSymbolicLinkObject(&hLink, SYMBOLIC_LINK_QUERY, &attr);
 if(NT_SUCCESS(Status)){result = TRUE;NtClose(hLink);}
 return result;
}
//------------------------------------------------------------------------------------
static NTSTATUS NativeDeleteFile(PWSTR FileName)     // NtDeleteFile
{           
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
  }
 return Status;
}
//------------------------------------------------------------------------------------
static NTSTATUS OpenDevice(PWSTR DevName, HANDLE* hDev)
{
 UNICODE_STRING DevPathUS;
 OBJECT_ATTRIBUTES attr;
 IO_STATUS_BLOCK iost;
 UINT Length = 8;
 wchar_t DevPath[MAX_PATH] = {'\\','D','e','v','i','c','e','\\'};
 for(int idx=0;*DevName;DevName++,Length++)DevPath[Length] = *DevName;
 DevPathUS.Buffer = DevPath;
 DevPathUS.Length = Length * sizeof(wchar_t);
 DevPathUS.MaximumLength = DevPathUS.Length + sizeof(wchar_t);   //RtlInitUnicodeString(&str, L"\\Device\\xxx");

 attr.Length = sizeof(OBJECT_ATTRIBUTES);
 attr.RootDirectory = 0;
 attr.ObjectName = &DevPathUS;
 attr.Attributes = 0;
 attr.SecurityDescriptor = 0;
 attr.SecurityQualityOfService = 0;
 return NtCreateFile(hDev, GENERIC_READ | GENERIC_WRITE, &attr, &iost, 0, 0, 0, FILE_OPEN, 0, 0, 0);
}
//------------------------------------------------------------------------------------
static HANDLE OpenBeep(void)
{
 HANDLE hBeep = NULL;
 OpenDevice(L"Beep", &hBeep);
 return hBeep; 
}
//------------------------------------------------------------------------------------
static NTSTATUS DoBeep(HANDLE hBeep, DWORD Freq, DWORD Duration)
{   
 struct {
  ULONG uFrequency;
  ULONG uDuration;
 } param;
 IO_STATUS_BLOCK iost;
 param.uFrequency = Freq;    // short
 param.uDuration  = Duration;
 return NtDeviceIoControlFile(hBeep, 0, 0, 0, &iost, 0x00010000, &param, sizeof(param), 0, 0);
} */
//------------------------------------------------------------------------------------
typedef void (_scall *PNT_THREAD_PROC)(PVOID Data, SIZE_T Size);   // Param may be on stack in x32 or not  // Exit with NtTerminateThread  // stdcall on X86-X32

// http://rsdn.org/forum/winapi/164784.hot
// CreateRemoteThread requires PROCESS_VM_WRITE but that is too much to ask for a stealth thread injection
static NT::NTSTATUS NativeCreateThread(PNT_THREAD_PROC ThreadRoutine, NT::PVOID ParData, NT::SIZE_T ParSize, NT::HANDLE ProcessHandle, bool CrtSusp, NT::PPVOID StackBase, NT::PSIZE_T StackSize, NT::PHANDLE ThreadHandle, NT::PULONG ThreadID)
{
 static constexpr int PAGE_SIZE = MEMPAGESIZE;
 NT::NTSTATUS   Status = 0;
 NT::USER_STACK UserStack;
 NT::CONTEXT    Context;
 NT::CLIENT_ID  ClientId;
 NT::PVOID      LocStackBase = nullptr;
 NT::SIZE_T     LocStackSize = 0;
                              
 DBGMSG("ProcessHandle=%p, ThreadRoutine=%p, ParData=%p, ParSize=%p",ProcessHandle,ThreadRoutine,ParData,ParSize);
 UserStack.FixedStackBase  = nullptr;
 UserStack.FixedStackLimit = nullptr;
 if(!StackBase)StackBase = &LocStackBase;
 if(!StackSize)StackSize = &LocStackSize;

 if(!*StackBase)
  {
   if(*StackSize)*StackSize = AlignP2Frwd(*StackSize + PAGE_SIZE, MEMGRANSIZE);    // Must allocate by 64k to avoid address space wasting
    else *StackSize = PAGE_SIZE * 256;   // 1Mb as default stack size (Including a guard page)
   Status = SAPI::NtAllocateVirtualMemory(ProcessHandle, StackBase, 0, StackSize, NT::MEM_RESERVE, NT::PAGE_READWRITE);   // Reserve the memory first
   if(!NT::NT_SUCCESS(Status)){DBGMSG("AVM Failed 1"); return Status;}

   // NOTE: Growing of a reserved stack space is not used but a guard page is still placed because kernel may check for it 
   UserStack.ExpandableStackBase   = &((uint8*)*StackBase)[*StackSize];   // Where the stack memory block ends (entire reserved region) // ESP is usually points here initially
   UserStack.ExpandableStackLimit  = &((uint8*)*StackBase)[PAGE_SIZE];    // Points to beginning of committed memory range
   UserStack.ExpandableStackBottom = *StackBase;    // Beginning of the memory block, no growing above it
 
   NT::SIZE_T StackCommit = *StackSize;         // Use 2 guard pages
   NT::PVOID  CommitBase  = *StackBase;    
   Status = SAPI::NtAllocateVirtualMemory(ProcessHandle, &CommitBase, 0, &StackCommit, NT::MEM_COMMIT, NT::PAGE_READWRITE);    // Stack commit, includes one guard page
   if(!NT::NT_SUCCESS(Status)){DBGMSG("AVM Failed 2"); return Status;}
  }
   else    // Assume that there may be some data on the stack already
    {
     UserStack.ExpandableStackBase   = &((uint8*)*StackBase)[AlignP2Frwd(*StackSize, MEMGRANSIZE)];  // Arbitrary - StackSize may specify offset to some data already on the stack  
     UserStack.ExpandableStackLimit  = &((uint8*)*StackBase)[PAGE_SIZE]; 
     UserStack.ExpandableStackBottom = *StackBase;
    }
 {
  NT::ULONG  OldProtect;
  NT::SIZE_T GuardSize = PAGE_SIZE;    
  NT::PVOID  GuardBase = *StackBase; 
  Status = SAPI::NtProtectVirtualMemory(ProcessHandle, &GuardBase, &GuardSize, NT::PAGE_READWRITE | NT::PAGE_GUARD, &OldProtect);   // create a GUARD page           
  if(!NT::NT_SUCCESS(Status)){DBGMSG("PVM Failed"); return Status;}
 }

// Avoiding RtlInitializeContext because it uses NtWriteVirtualMemory for a Parameter on x32 (A remote process may be opened without rights to do that)
 Context.EFlags       = 0x3000;    // ??? 0x200 ?
 Context.ContextFlags = NT::CONTEXT_CONTROL|NT::CONTEXT_INTEGER;   
#ifdef ARCH_X64     // Cannot use 'if constexr' it requires the names to exist unless used from a template type
 Context.Rsp  = (NT::SIZE_T)&((uint8*)*StackBase)[*StackSize]; 
 Context.Rip  = (NT::SIZE_T)ThreadRoutine;
 Context.Rsp -= sizeof(NT::SIZE_T) * 5;  // For a reserved block of 4 Arguments (RCX, RDX, R8, R9) and a fake ret addr
 Context.Rcx  = Context.Rax = (NT::SIZE_T)ParData;    // Match with fastcall
 Context.Rdx  = Context.Rbx = (NT::SIZE_T)ParSize;
#else        // On WOW64 these are passed directly to new x64 thread context: EIP(RIP), ESP(R8), EBX(RDX), EAX(RCX)  // Later EAX and EBX will get to x32 entry point even without Wow64pCpuInitializeStartupContext  // On native x32/64 all registers are passed to thread`s entry point
 Context.Esp  = (NT::SIZE_T)&((uint8*)*StackBase)[*StackSize]; 
 Context.Eip  = (NT::SIZE_T)ThreadRoutine;   // Native only
 Context.Esp -= sizeof(NT::SIZE_T) * 5;  // For a Return addr(fake, just to keep frame right) and a possible Params for not fastcall ThreadProc if you prepared it in your stack
 Context.Ecx  = Context.Eax = (NT::SIZE_T)ParData;    // Match with fastcall if Wow64pCpuInitializeStartupContext is used
 Context.Edx  = Context.Ebx = (NT::SIZE_T)ParSize;

 Context.ContextFlags |= NT::CONTEXT_SEGMENTS;    // NOTE: Only Windows x32 requires segment registers to be set 
 Context.SegGs = 0x00;
 Context.SegFs = 0x38;
 Context.SegEs = 0x20;
 Context.SegDs = 0x20;
 Context.SegSs = 0x20;
 Context.SegCs = 0x18;  

 if(ProcessHandle == NtCurrentProcess)  
  {
   ((PVOID*)Context.Esp)[1] = ParData;    // Win7 WOW64 pops ret addr from stack making this argument unavailable // Solution: Pass same value in ParData and ParSize
   ((PVOID*)Context.Esp)[2] = ParSize;
  }
   else  // Creating in a remote process
    {
     PVOID Arr[] = {ParData,ParSize};
     if(NTSTATUS stat = NtWriteVirtualMemory(ProcessHandle, &((PVOID*)Context.Esp)[1], &Arr, sizeof(Arr), 0)){DBGMSG("Write stack args failed with: %08X",stat);}   // It is OK to fail if no PROCESS_VM_WRITE
    }
#endif

 NT::ULONG ThAcc = NT::SYNCHRONIZE|NT::THREAD_GET_CONTEXT|NT::THREAD_SET_CONTEXT|NT::THREAD_QUERY_INFORMATION|NT::THREAD_SET_INFORMATION|NT::THREAD_SUSPEND_RESUME|NT::THREAD_TERMINATE;
// NOTE: To avoid some WOW64 bugs on Windows10 we must always call a native version of this function 
 Status = SAPI::NtCreateThread(ThreadHandle, ThAcc, nullptr, ProcessHandle, &ClientId, &Context, &UserStack, (uint)CrtSusp); // Always returns ACCESS_DENIED for any CFG enabled process? // The thread starts at specified IP and with specified SP and no return address // End it with NtTerminateThread  // CrtSusp must be exactly 1 to suspend     
#ifdef ARCH_X32 
// Always use native syscalls // if((Status == STATUS_ACCESS_DENIED) && (FixWinTenWow64NtCreateThread() > 0))Status = NtCreateThread(ThreadHandle, ThAcc, nullptr, ProcessHandle, &ClientId, &Context, (PINITIAL_TEB)&UserStack, CrtSusp);     // Try to fix Wow64NtCreateThread bug in latest versions of Windows 10   
#endif
 if(!NT::NT_SUCCESS(Status)){DBGMSG("CreateThread Failed"); return Status;}	
 if(ThreadID)*ThreadID = NT::ULONG(ClientId.UniqueThread);              
 return Status;
}
//------------------------------------------------------------------------------------
// Starting in Windows 8.1, GetVersion() and GetVersionEx() are subject to application manifestation
// See https://stackoverflow.com/questions/32115255/c-how-to-detect-windows-10
//
/*static inline DWORD _fastcall GetRealVersionInfo(PDWORD dwMajor=NULL, PDWORD dwMinor=NULL, PDWORD dwBuild=NULL, PDWORD dwPlatf=NULL)
{
 PPEB peb = NtCurrentPeb();
 if(dwMajor)*dwMajor = peb->OSMajorVersion;
 if(dwMinor)*dwMinor = peb->OSMinorVersion;
 if(dwBuild)*dwBuild = peb->OSBuildNumber;
 if(dwPlatf)*dwPlatf = peb->OSPlatformId;
 DWORD Composed = (peb->OSPlatformId << 16)|(peb->OSMinorVersion << 8)|peb->OSMajorVersion;
 return Composed;
}
//---------------------------------------------------------------------------
static int GetMappedFilePath(HANDLE hProcess, PVOID BaseAddr, PWSTR PathBuf, UINT BufByteSize)   // Returns as '\Device\HarddiskVolume'
{
 SIZE_T RetLen = 0;       
 if(!NtQueryVirtualMemory(hProcess,BaseAddr,MemoryMappedFilenameInformation,PathBuf,BufByteSize,&RetLen) && RetLen)
  {
   PWSTR PathStr = ((UNICODE_STRING*)PathBuf)->Buffer;
   UINT  PathLen = ((UNICODE_STRING*)PathBuf)->Length; 
   memmove(PathBuf, PathStr, PathLen);
   *((WCHAR*)&((PBYTE)PathBuf)[PathLen]) = 0; 
   return PathLen / sizeof(WCHAR);
  }
 *PathBuf = 0;
 return 0;
};
//------------------------------------------------------------------------------------
static inline ULONG GetProcessID(HANDLE hProcess)
{
 if(!hProcess || (hProcess == NtCurrentProcess))return NtCurrentProcessId();     
 PROCESS_BASIC_INFORMATION pinf;
 ULONG RetLen = 0;
 HRESULT res  = NtQueryInformationProcess(hProcess,ProcessBasicInformation,&pinf,sizeof(PROCESS_BASIC_INFORMATION),&RetLen);
 if(res){DBGMSG("Failed to get process ID: %08X", res); return 0;}   // 0 id belongs to the system 
 return (ULONG)pinf.UniqueProcessId;
}
//------------------------------------------------------------------------------------
static inline ULONG GetThreadID(HANDLE hThread, ULONG* ProcessID=NULL, PTEB* pTeb=NULL)
{
 if(!hThread || (hThread == NtCurrentThread))      
  {
   PTEB teb = NtCurrentTeb();  
   if(ProcessID)*ProcessID = (ULONG)teb->ClientId.UniqueProcess;
   if(pTeb)*pTeb = teb;   
//   DBGMSG("hThread=%p, CurrentTEB=%p, ProcID=%u, ThID=%u", hThread, teb, (ULONG)teb->ClientId.UniqueProcess, (ULONG)teb->ClientId.UniqueThread);
   return (ULONG)teb->ClientId.UniqueThread;
  }
 THREAD_BASIC_INFORMATION tinf;
 ULONG RetLen = 0;
 HRESULT res  = NtQueryInformationThread(hThread,ThreadBasicInformation,&tinf,sizeof(THREAD_BASIC_INFORMATION),&RetLen);
 if(res){DBGMSG("Failed to get thread ID: %08X", res); return 0;}   // 0 id belongs to the system 
 if(ProcessID)*ProcessID = (ULONG)tinf.ClientId.UniqueProcess;
 if(pTeb)*pTeb = (PTEB)tinf.TebBaseAddress;
// DBGMSG("hThread=%p, TEB=%p, ProcID=%u, ThID=%u", hThread, tinf.TebBaseAddress, (ULONG)tinf.ClientId.UniqueProcess, (ULONG)tinf.ClientId.UniqueThread);
 return (ULONG)tinf.ClientId.UniqueThread;
}
//------------------------------------------------------------------------------------
static inline ULONG GetCurrProcessThreadID(HANDLE hThread)
{
 ULONG PrID = 0;
 ULONG ThID = GetThreadID(hThread, &PrID);
 if(!ThID || (PrID != NtCurrentProcessId()))return 0;
 return ThID;
}
//------------------------------------------------------------------------------------
static inline PTEB GetCurrProcessTEB(HANDLE hThread)
{
 PTEB  teb  = NULL;
 ULONG PrID = 0;
 ULONG ThID = GetThreadID(hThread, &PrID, &teb);
 if(!ThID || (PrID != NtCurrentProcessId()))return 0;
 return teb;
}
//------------------------------------------------------------------------------------
static inline bool IsCurrentProcess(HANDLE hProcess)
{
 return GetProcessID(hProcess) == NtCurrentProcessId();
}
//------------------------------------------------------------------------------------
static inline bool IsCurrentThread(HANDLE hThread)
{
 return GetThreadID(hThread) == NtCurrentThreadId();
}
//------------------------------------------------------------------------------------
static inline bool IsCurrentProcessThread(HANDLE hThread)
{
 return GetCurrProcessThreadID(hThread);
}  */
//------------------------------------------------------------------------------------
// 
// RtlUserThreadStart (Callback)    // CREATE_SUSPENDED thread`s IP 
//   x32  x64
//   EAX  RCX = ThreadProc
//   EBX  RDX = ThreadParam
// 
// Native thread:   (Suspend is TRUE)
//   x32  x64
//   EAX  RCX = ThreadProc
//   EBX  RDX = ThreadParam
// 
static bool ChangeNewSuspThProcAddr(HANDLE hThread, PVOID NewThProc, PVOID* Param, bool Native=false)
{
 CONTEXT ctx;
 ctx.ContextFlags = CONTEXT_INTEGER;   // CONTEXT_CONTROL - no check if IP is at RtlUserThreadStart
 if(Native)ctx.ContextFlags |= CONTEXT_CONTROL;
 if(NTSTATUS stat = NtGetContextThread(hThread, &ctx)){DBGMSG("Failed to get CONTEXT: %08X",stat); return false;}
#ifdef _AMD64_ 
 if(NewThProc)
  {
   if(Native)ctx.Rip = (DWORD64)NewThProc;
     else ctx.Rcx = (DWORD64)NewThProc;
  }
 if(Param)
  {
   DWORD64 Prv;
   if(Native){Prv = ctx.Rcx; ctx.Rcx = (DWORD64)*Param;}
    else {Prv = ctx.Rdx; ctx.Rdx = (DWORD64)*Param;}  
   *Param  = (PVOID)Prv;
  }
#else
 if(NewThProc)
  {
   if(Native)ctx.Eip = (DWORD)NewThProc;    // Doesn`t work for x32!!!!!!!!!!!!!!!!!!!
     else ctx.Eax = (DWORD)NewThProc;
  }
 if(Param)
  {
   DWORD Prv;
   if(Native){Prv = ctx.Ecx; ctx.Ecx = (DWORD)*Param;} 
     else {Prv = ctx.Ebx; ctx.Ebx = (DWORD)*Param;}       
   *Param  = (PVOID)Prv;
  }
#endif
 if(NTSTATUS stat = NtSetContextThread(hThread, &ctx)){DBGMSG("Failed to set CONTEXT: %08X",stat); return false;}
 DBGMSG("NewThProc=%p, Param=%p", NewThProc,Param?*Param:0);
 return true;
} 
//------------------------------------------------------------------------------------
/*static jmp_buf jenv;  // setjmp env for the jump back into the fork() function

static int child_entry(void)  // entry point for our child thread process - just longjmp into fork
{
 longjmp(jenv, 1);
 return 0;
}
//------------------------------------------------------------------------------------
static void set_inherit_all(void)
{
 ULONG  n = 0x1000;
 PULONG p = (PULONG) calloc(n, sizeof(ULONG));
 while(NtQuerySystemInformation(SystemHandleInformation, p, n * sizeof(ULONG), 0) == STATUS_INFO_LENGTH_MISMATCH)  // some guesswork to allocate a structure that will fit it all
  {
   free(p);
   n *= 2;
   p  = (PULONG) calloc(n, sizeof(ULONG));
  }	
 PSYSTEM_HANDLE_INFORMATION h = (PSYSTEM_HANDLE_INFORMATION)(p + 1);  // p points to an ULONG with the count, the entries follow (hence p[0] is the size and p[1] is where the first entry starts
 ULONG pid = GetCurrentProcessId();
 ULONG i = 0, count = *p;
 while(i < count) 
  {
   if(h[i].ProcessId == pid)SetHandleInformation((HANDLE)(ULONG) h[i].Handle, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
   i++;
  }
 free(p); 
}
//------------------------------------------------------------------------------------
// https://github.com/Mattiwatti/BSOD10/
// On Windows 10 >= 10586 and < 16299,
// Windows 10 after 1511 (10.0.10586, TH2) up to 1703 (10.0.15063, RS2)
// One interesting thing to note is that NtCreateUserProcess requires an initial thread, unlike NtCreateProcess[Ex]. 
//   Because terminating the last thread of a process will always terminate the process, this makes NtCreateProcess[Ex] the only way to create a Windows process with zero threads.
// The crash occurs in PspInitializeFullProcessImageName, which obtains the process name from the process attribute list (the first argument to the function).
// NOTE: The process may inherit any system lib hooks (The memory is CopyOnWrite of a parent process) // NOTE: Address space will be reduced by number of parent`s region allocations
// NtCreaterProcess[Ex] is very similair to Linux 'clone' syscall with (CLONE_VM) buth there is no actual execution thread is created
// NtCreateUserProcess is a system call. It exposes process forking by setting the PS_ATTRIBUTE_PARENT_PROCESS within the PPS_ATTRIBUTE_LIST AttributeList parameter
// https://www.deepinstinct.com/blog/dirty-vanity-a-new-approach-to-code-injection-edr-bypass
// Process Snapshotting is invoked with Kernel32!PssCaptureSnapshot and if we go down the call chain we will see Kernel32!PssCaptureSnapshot calls ntdll!PssNtCaptureSnapshot calls ntdll!NtCreateProcessEx
//   ZwCreateProcessEx(vpProcessHandle, 0x2000000, 0, aParentProcess, aFlags, 0, 0, 0, 0);
// NtCreateProcess[Ex] are two legacy process creation syscalls that offer another route to access the forking mechanism. 
//  However, as opposed to the newer NtCreateUserProcess, one can fork a remote process with them by setting the HANDLE ParentProcess parameter with the target process handle.
// MmInitializeProcessAddressSpace with a flag specifying that the address should be a copy-on-write copy of the target process instead of an initial process address space.
// https://groups.google.com/forum/#!topic/comp.os.ms-windows.programmer.nt.kernel-mode/hoN_RYtnp58
// The most important parameter here is SectionHandle. If this parameter is NULL, the kernel will fork the current process. 
//  Otherwise, this parameter must be a handle of the SEC_IMAGE section object created on the EXE file before calling ZwCreateProcess().
//

static int NtFork(bool InheritAll=false)   // RtlCloneUserProcess ?
{
 if(setjmp(jenv) != 0)return 0;    // return as a child
 if(InheritAll)set_inherit_all();  //  make sure all handles are inheritable
 HANDLE hProcess = 0, hThread = 0;
 OBJECT_ATTRIBUTES oa = { sizeof(oa) };
 NtCreateProcess(&hProcess, PROCESS_ALL_ACCESS, &oa, NtCurrentProcess, TRUE, 0, 0, 0);  // create forked process
 CONTEXT context = {CONTEXT_FULL | CONTEXT_DEBUG_REGISTERS | CONTEXT_FLOATING_POINT};

 NtGetContextThread(NtCurrentThread, &context);     // set the Eip for the child process to our child function
// context.Eip = (ULONG)child_entry;
 
 MEMORY_BASIC_INFORMATION mbi;
// NtQueryVirtualMemory(NtCurrentProcess, (PVOID)context.Esp, MemoryBasicInformation, &mbi, sizeof mbi, 0);
 
 USER_STACK stack = {0, 0, (PCHAR)mbi.BaseAddress + mbi.RegionSize, mbi.BaseAddress, mbi.AllocationBase};
 CLIENT_ID cid;
// NtCreateThread(&hThread, THREAD_ALL_ACCESS, &oa, hProcess, &cid, &context, &stack, TRUE);  // create thread using the modified context and stack   // Do not use THREAD_ALL_ACCESS it has been changed
 
 THREAD_BASIC_INFORMATION tbi;
 NtQueryInformationThread(NtCurrentThread, ThreadBasicInformation, &tbi, sizeof tbi, 0);
 PNT_TIB tib = (PNT_TIB)tbi.TebBaseAddress;
 NtQueryInformationThread(hThread, ThreadBasicInformation, &tbi, sizeof tbi, 0);
 NtWriteVirtualMemory(hProcess, tbi.TebBaseAddress, &tib->ExceptionList, sizeof tib->ExceptionList, 0);  // copy exception table
 
 NtResumeThread(hThread, 0);   // start (resume really) the child
 
 NtClose(hThread);
 NtClose(hProcess);
 return (int)cid.UniqueProcess;  //  exit with child's pid
}
//------------------------------------------------------------------------------------
static UINT64 GetObjAddrByHandle(HANDLE hObj, DWORD OwnerProcId)  
{
 SIZE_T InfoBufSize = 1048576;
 NTSTATUS stat = 0;
 ULONG RetLen  = 0;
 UINT64 Addr   = 0;
 PVOID InfoArr = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, InfoBufSize);
 if(IsWow64())
  {
   while(stat = NWOW64E::QuerySystemInformation(SystemExtendedHandleInformation, InfoArr, InfoBufSize, &RetLen) == STATUS_INFO_LENGTH_MISMATCH)InfoArr = HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,InfoArr,InfoBufSize *= 2);
   for(ULONG idx = 0;idx < ((NWOW64E::SYSTEM_HANDLE_INFORMATION_EX_64*)InfoArr)->NumberOfHandles;idx++)
    {
     NWOW64E::SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX<DWORD64>* HndlInfo = &((NWOW64E::SYSTEM_HANDLE_INFORMATION_EX_64*)InfoArr)->Handles[idx];
     if(HndlInfo->UniqueProcessId != (UINT64)OwnerProcId)continue;     // Wrong process
     if(HndlInfo->HandleValue != (UINT64)hObj)continue;     // Wrong Handle
     Addr = (UINT64)HndlInfo->Object;
     break;  
    }
  }
 else
  {
   while(stat = NtQuerySystemInformation(SystemExtendedHandleInformation, InfoArr, InfoBufSize, &RetLen) == STATUS_INFO_LENGTH_MISMATCH)InfoArr = HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,InfoArr,InfoBufSize *= 2);
   for(ULONG idx = 0;idx < ((PSYSTEM_HANDLE_INFORMATION_EX)InfoArr)->NumberOfHandles;idx++)
    {
     PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX HndlInfo = &((PSYSTEM_HANDLE_INFORMATION_EX)InfoArr)->Handles[idx];
     if(HndlInfo->UniqueProcessId != OwnerProcId)continue;     // Wrong process
     if(HndlInfo->HandleValue != (ULONG_PTR)hObj)continue;     // Wrong Handle
     Addr = (ULONG_PTR)HndlInfo->Object;   // Why casting to UINT64 makes this pointer sign extended on x32???  // Is 'void*' actually a signed type?
     break; 
    }
   }
 if(InfoArr)HeapFree(GetProcessHeap(),0,InfoArr);
 return Addr;
}
//------------------------------------------------------------------------------------
static UINT64 GeKernelModuleBase(LPSTR ModuleName, ULONG* ImageSize)  
{
 SIZE_T InfoBufSize = 1048576;
 NTSTATUS stat = 0;
 ULONG RetLen  = 0;
 UINT64 Addr   = 0;
 PVOID InfoArr = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, InfoBufSize);
 if(IsWow64())
  {
   while(stat = NWOW64E::QuerySystemInformation(SystemModuleInformation, InfoArr, InfoBufSize, &RetLen) == STATUS_INFO_LENGTH_MISMATCH)InfoArr = HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,InfoArr,InfoBufSize *= 2);
   for(ULONG idx = 0;idx < ((NWOW64E::RTL_PROCESS_MODULES_64*)InfoArr)->NumberOfModules;idx++)
    {
     NWOW64E::RTL_PROCESS_MODULE_INFORMATION<DWORD64>* HndlInfo = &((NWOW64E::RTL_PROCESS_MODULES_64*)InfoArr)->Modules[idx];
    // if(HndlInfo->UniqueProcessId != (UINT64)OwnerProcId)continue;     // Wrong process
    // if(HndlInfo->HandleValue != (UINT64)hObj)continue;     // Wrong Handle
    // Addr = (UINT64)HndlInfo->Object;
     break;  
    }
  }
 else
  {
   while(stat = NtQuerySystemInformation(SystemModuleInformation, InfoArr, InfoBufSize, &RetLen) == STATUS_INFO_LENGTH_MISMATCH)InfoArr = HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,InfoArr,InfoBufSize *= 2);
   for(ULONG idx = 0;idx < ((PRTL_PROCESS_MODULES)InfoArr)->NumberOfModules;idx++)
    {
     PRTL_PROCESS_MODULE_INFORMATION HndlInfo = &((PRTL_PROCESS_MODULES)InfoArr)->Modules[idx];
    // if(HndlInfo->UniqueProcessId != OwnerProcId)continue;     // Wrong process
    // if(HndlInfo->HandleValue != (ULONG_PTR)hObj)continue;     // Wrong Handle
    // Addr = (ULONG_PTR)HndlInfo->Object;   // Why casting to UINT64 makes this pointer sign extended on x32???  // Is 'void*' actually a signed type?
     break; 
    }
   }
 if(InfoArr)HeapFree(GetProcessHeap(),0,InfoArr);
 return Addr;
}
//------------------------------------------------------------------------------------
static SIZE_T IsMemAvailable(PVOID Addr, bool* IsImage=nullptr)      // Helps to skip a Reserved mapping rgions
{
 SIZE_T RetLen = 0; 
 MEMORY_BASIC_INFORMATION minf;                                                                            
 NTSTATUS Status = NtQueryVirtualMemory(NtCurrentProcess,Addr,MemoryBasicInformation,&minf,sizeof(MEMORY_BASIC_INFORMATION),&RetLen);
 if(Status || !(minf.State & MEM_COMMIT))return 0;
 if(IsImage)*IsImage = (minf.Type & MEM_IMAGE);  // (minf.Type & MEM_MAPPED)
 return minf.RegionSize;
}
*/
//------------------------------------------------------------------------------------
// NOTE: LdrInitializeThunk will crash without this
// CreateProcessParameters(hProcess, pbi.PebBaseAddress, name);     // Do not forget that the we can be a X32 process and the target process is X64 or vice versa
//
static NT::NTSTATUS InitProcessEnvironment(NT::HANDLE hProc, const NT::UNICODE_STRING* ImgPath, const achar** Args)  // TODO TODO TODO
{
 NT::PEB* ThisPEB = NT::NtCurrentPeb();
 NT::RTL_USER_PROCESS_PARAMETERS* ThisProcParams = ThisPEB->ProcessParameters;
 if(ThisProcParams)    // [RTL_USER_PROCESS_PARAMETERS,Strings],Environment
  {
   size_t RmtBlkSize = ThisProcParams->MaximumLength;
   RmtBlkSize += ThisProcParams->EnvironmentSize;

 //  CurrentDirectories   // Useless // Always empty?

  /*  NT::PPROCESS_PARAMETERS pp;

        p->MaximumLength = ByteCount;
        p->Length = ByteCount;
        p->Flags = RTL_USER_PROC_PARAMS_NORMALIZED;
        p->DebugFlags = 0;
        p->Environment = Environment;
        p->CurrentDirectory.Handle = CurDirHandle;

    NT::RtlCreateProcessParameters(&pp, ImageFile, 0, 0, 0, 0, 0, 0, 0, 0);

    pp->Environment = CopyEnvironment(hProcess);

    ULONG n = pp->Size;
    PVOID p = 0;
    NT::ZwAllocateVirtualMemory(hProcess, &p, 0, &n, MEM_COMMIT, PAGE_READWRITE);

    NT::ZwWriteVirtualMemory(hProcess, p, pp, pp->Size, 0);

    NT::ZwWriteVirtualMemory(hProcess, PCHAR(Peb) + 0x10, &p, sizeof p, 0);

    NT::RtlDestroyProcessParameters(pp);
       */
  }
 return 0;
}
//------------------------------------------------------------------------------------
// ntdll!LdrInitializeThunk
// https://vrodxda.hatenablog.com/entry/2019/09/18/085454
// https://github.com/mic101/windows/blob/master/WRK-v1.2/base/ntos/rtl/rtlexec.c
// https://learn.microsoft.com/en-us/windows/win32/sbscs/activation-contexts
// 
// Init by system(NtCreateProcess) PEB at LdrInitializeThunk:
//    BeingDebugged, BitField(?), Mutant, ImageBaseAddress, ApiSetMap, AnsiCodePageData, OemCodePageData, UnicodeCaseTableData, NumberOfProcessors, CriticalSectionTimeout, HeapSegmentReserve, HeapSegmentCommit, HeapDeCommitTotalFreeThreshold, HeapDeCommitFreeBlockThreshold
//    OSMajorVersion, OSMinorVersion, OSBuildNumber, OSCSDVersion, OSPlatformId, ImageSubsystem, ImageSubsystemMajorVersion, ImageSubsystemMinorVersion, ActiveProcessAffinityMask, SessionId
// CreateProcess -> NtCreateUserProcess adds:
//    ProcessParameters, pShimData(64k), ActivationContextData(64k), SystemDefaultActivationContextData(64k)
//
static NT::NTSTATUS NativeCreateProcess(const achar* ImgPath, const achar** Args, const achar** EVars, bool Susp, NT::HANDLE* phProc, NT::HANDLE* phThrd)
{
 NT::HANDLE hProcess, hThread, hSection, hFile;
 NT::NTSTATUS res;
 NT::IO_STATUS_BLOCK iosb = {};
 NT::OBJECT_ATTRIBUTES oattr = {};
 NT::UNICODE_STRING FilePathUS;
 NT::SECTION_IMAGE_INFORMATION sii;   // Stack info of the PE image
 NT::PROCESS_BASIC_INFORMATION pbi;   // For PEB addr

 uint plen;
 NTX::EPathType ptype;
 uint PathLen = CalcFilePathBufSize(ImgPath, plen, ptype);

 wchar FullPath[PathLen];
 InitFileObjectAttributes(ImgPath, plen, ptype, 0, &FilePathUS, FullPath, &oattr);

 res = SAPI::NtCreateFile(&hFile, NT::FILE_EXECUTE | NT::SYNCHRONIZE, &oattr, &iosb, nullptr, NT::FILE_ATTRIBUTE_NORMAL, NT::FILE_SHARE_READ, NT::FILE_OPEN, NT::FILE_SYNCHRONOUS_IO_NONALERT, nullptr, 0);  
 if(res)return res;

 oattr.ObjectName = 0;  // Actually, it is possible to create a named process object here
 res = SAPI::NtCreateSection(&hSection, NT::SECTION_ALL_ACCESS, &oattr, 0, NT::PAGE_EXECUTE, NT::SEC_IMAGE, hFile);
 SAPI::NtClose(hFile);   // Not inherited even if not closed (Not inheritable)
 if(res)return res;

// NOTE: Execution actually starts at ntdll.dll::LdrInitializeThunk  // WOW64?
 res = SAPI::NtCreateProcess(&hProcess, NT::PROCESS_ALL_ACCESS, &oattr, NT::NtCurrentProcess, true, hSection, 0, 0);   // Are there any use for 'fork' ability? // Only Ntdll.dll is mapped(Not a GUI process?), console handles are inherited
 if(res){SAPI::NtClose(hSection); return res;}
    
 res = SAPI::NtQuerySection(hSection, NT::SectionImageInformation, &sii, sizeof sii, 0);
 SAPI::NtClose(hSection);
 if(res){SAPI::NtClose(hProcess); return res;}
   
 res = SAPI::NtQueryInformationProcess(hProcess, NT::ProcessBasicInformation, &pbi, sizeof pbi, 0);
 if(res){SAPI::NtClose(hProcess); return res;}

 res = InitProcessEnvironment(hProcess, &FilePathUS, Args);
 if(res){SAPI::NtClose(hProcess); return res;} 

 NT::ULONG  ThreadID;
 NT::PVOID  StackBase = nullptr;   // Alloc at any addr
 NT::SIZE_T StackSize = sii.MaximumStackSize;   
 res = NTX::NativeCreateThread((PNT_THREAD_PROC)(sii.TransferAddress), nullptr, 0, hProcess, true, &StackBase, &StackSize, &hThread, &ThreadID);  // Suspended?  // Parameters for the thread?
 if(res){SAPI::NtClose(hProcess); return res;}

   // InformCsrss(hProcess, hThread, ULONG(cid.UniqueProcess), ULONG(cid.UniqueThread));    // Only if suspended

 if(!Susp)SAPI::NtResumeThread(hThread, 0);     // If suspended
 if(!phProc)SAPI::NtClose(hProcess);   
  else *phProc = hProcess;
 if(!phThrd)SAPI::NtClose(hThread);   
  else *phThrd = hThread;
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
