
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

// ToDo: Optionally export to self and resolve inports instead of ntdll.lib
//---------------------------------------------------------------------------
struct NNTDLL   // NT system service calls encapsulation  // DODO: Rename to NWINNT and make it core of WinNt support in my cross platform framework
{     // See KUSER_SHARED_DATA in ntddk.h
static inline bool IsWinXPOrOlder(void)
{
 PBYTE pKiUserSharedData = reinterpret_cast<PBYTE>(0x7FFE0000);
 return (bool)*(PDWORD)pKiUserSharedData;    // Deprecated since 5.2(XP x64) and equals 0
}
//---------------------------------------------------------------------------
static inline UINT64 GetTicks(void)
{
 PBYTE pKiUserSharedData = reinterpret_cast<PBYTE>(0x7FFE0000);
 return (*(PDWORD)pKiUserSharedData)?((UINT64)*(PDWORD)pKiUserSharedData):(*(UINT64*)&pKiUserSharedData[0x320]); 
}
//----------------------------------------------------------------------------
static inline ULONG GetTicksCount(void)
{
 PBYTE pKiUserSharedData = reinterpret_cast<PBYTE>(0x7FFE0000);
 return (GetTicks() * *(PDWORD)&pKiUserSharedData[4]) >> 24;  // 'TickCountLow * TickCountMultiplier' or 'TickCount * TickCountMultiplier'
}
//----------------------------------------------------------------------------
static inline UINT64 GetInterruptTime(void)   // FILETIME
{
 PBYTE pKiUserSharedData = reinterpret_cast<PBYTE>(0x7FFE0000);
 return *(UINT64*)&pKiUserSharedData[0x0008]; 
}
//----------------------------------------------------------------------------
static inline UINT64 GetSystemTime(void)  // FILETIME
{
 PBYTE pKiUserSharedData = reinterpret_cast<PBYTE>(0x7FFE0000);
 return *(UINT64*)&pKiUserSharedData[0x0014]; 
}
//----------------------------------------------------------------------------
static inline UINT64 GetLocalTime(void)  // FILETIME
{
 PBYTE pKiUserSharedData = reinterpret_cast<PBYTE>(0x7FFE0000);
 UINT64 TimeZoneBias = *(UINT64*)&pKiUserSharedData[0x0020];
 return GetSystemTime() - TimeZoneBias; 
}
//----------------------------------------------------------------------------
// _ReadBarrier       // Forces memory reads to complete
// _WriteBarrier      // Forces memory writes to complete
// _ReadWriteBarrier  // Block the optimization of reads and writes to global memory 
//
static inline volatile UINT64 GetAbstractTimeStamp(UINT64 volatile* PrevVal)
{
 volatile UINT64 cval = __rdtsc();
 volatile UINT64 pval = *PrevVal;   // Interlocked.Read
 if(cval <= pval)return pval;       // Sync to increment   
 _InterlockedCompareExchange64((INT64*)PrevVal,cval, pval);  // Assign atomically if it is not changed yet  // This is the only one 64bit operand LOCK instruction available on x32 (cmpxchg8b)
 return *PrevVal;   // Return a latest value
}
//----------------------------------------------------------------------------
static NTSTATUS NTAPI NtSleep(DWORD dwMiliseconds, BOOL Alertable=FALSE)
{
 struct
  {
   DWORD Low;
   DWORD Hi;
  } MLI = {{-10000 * dwMiliseconds},{0xFFFFFFFF}};    // Relative time used
 return NtDelayExecution(Alertable, (PLARGE_INTEGER)&MLI);
}  
//----------------------------------------------------------------------------
static inline UINT64 GetNativeWowTebAddrWin10(void)   
{
 PTEB teb = NtCurrentTeb();
 if(!teb->WowTebOffset)return 0;  // Not WOW or below Win10  // From 10.0 and higher
 PBYTE TebX64 = (PBYTE)teb;
 if(long(teb->WowTebOffset) < 0)
  {
   TebX64 = &TebX64[(long)teb->WowTebOffset];  // In WOW processes WowTebOffset is negative in x32 TEB and positive in x64 TEB
   if(*(UINT64*)&TebX64[0x30] != (UINT64)TebX64)return 0;   // x64 Self
  }
   else if(*(PDWORD)&TebX64[0x18] != (DWORD)TebX64)return 0;   // x32 Self
 return UINT64(TebX64);
}
//---------------------------------------------------------------------------
// Returns 'false' if running under native x32 or native x64
static inline bool IsWow64(void)
{
#ifdef _AMD64_
 return false;
#else
 PTEB teb = NtCurrentTeb();
 return (bool)teb->WOW32Reserved;   // Is it reliable?  Is it always non NULL under Wow64?   // Contains pointer to 'jmp far 33:Addr'
#endif
}
//---------------------------------------------------------------------------
static int SetSkipThreadReport(HANDLE ThLocalThread)
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
}
//------------------------------------------------------------------------------------
static PVOID GetModuleBaseLdr(LPSTR ModName, ULONG* Size=NULL, long* BaseIdx=NULL)  // NOTE: No loader locking used!
{
 if(!ModName)
  {
   PVOID BaseAddr = NtCurrentTeb()->ProcessEnvironmentBlock->ImageBaseAddress;
   if(Size)
    {
     PEB_LDR_DATA* ldr = NtCurrentTeb()->ProcessEnvironmentBlock->Ldr;
     for(LDR_DATA_TABLE_ENTRY_MO* me = ldr->InMemoryOrderModuleList.Flink;me != (LDR_DATA_TABLE_ENTRY_MO*)&ldr->InMemoryOrderModuleList;me = me->InMemoryOrderLinks.Flink)     // Or just use LdrFindEntryForAddress?
      {
       if(BaseAddr == me->DllBase){*Size = me->SizeOfImage; break;}
      }
    }
   return BaseAddr;
  }
 PEB_LDR_DATA* ldr = NtCurrentTeb()->ProcessEnvironmentBlock->Ldr;
 DBGMSG("PEB_LDR_DATA: %p, %s",ldr,ModName);
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
 DBGMSG("Not found for: %s",ModName);
 return NULL;
}
//------------------------------------------------------------------------------------
static long GetModuleNameLdr(PVOID ModBase, PWSTR ModName, ULONG MaxSize=-1, bool Full=true)  // NOTE: No loader locking used!
{
 PEB_LDR_DATA* ldr = NtCurrentTeb()->ProcessEnvironmentBlock->Ldr;
 DBGMSG("PEB_LDR_DATA: %p, %p",ldr,ModBase);
 for(LDR_DATA_TABLE_ENTRY_MO* me = ldr->InMemoryOrderModuleList.Flink;me != (LDR_DATA_TABLE_ENTRY_MO*)&ldr->InMemoryOrderModuleList;me = me->InMemoryOrderLinks.Flink)     // Or just use LdrFindEntryForAddress?
  {
   if(me->DllBase != ModBase)continue;
   if(!me->BaseDllName.Length || !me->BaseDllName.Buffer){*ModName=0; return 0;}    // No path!
//   DBGMSG("Base=%p, Name='%ls'",me->DllBase,me->BaseDllName.Buffer);    // Zero terminated?     // Spam
   UINT Index = 0;
   MaxSize--;  // For terminating 0
   UNICODE_STRING* UStr = Full?&me->FullDllName:&me->BaseDllName;
   for(UINT tot=UStr->Length/sizeof(WCHAR);(Index < tot) && MaxSize;Index++,MaxSize--)ModName[Index] = UStr->Buffer[Index];
   ModName[Index] = 0;
   return Index;
  }
 DBGMSG("Not found for: %p",ModBase);
 *ModName = 0;
 return -1;
}
//------------------------------------------------------------------------------------
static NTSTATUS CreateNtObjDirectory(PWSTR ObjDirName, PHANDLE phDirObj)   // Create objects directory with NULL security
{
 wchar_t Path[512] = {'\\'}; 
 UNICODE_STRING ObjectNameUS;
 SECURITY_DESCRIPTOR  sd = {SECURITY_DESCRIPTOR_REVISION, 0, 4};    // NULL security descriptor: InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION); SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE);
 OBJECT_ATTRIBUTES oattr = { sizeof(OBJECT_ATTRIBUTES), 0, &ObjectNameUS, OBJ_CASE_INSENSITIVE|OBJ_OPENIF, &sd };
 UINT Length = 1;   
 for(int idx=0;*ObjDirName;ObjDirName++)Path[Length++] = *ObjDirName;
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
static NTSTATUS NativeDeleteFile(PWSTR FileName)
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
   fDispositionInfo.DeleteFile = TRUE;
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
}
//------------------------------------------------------------------------------------
typedef void (_stdcall *PNT_THREAD_PROC)(PVOID ParamA, PVOID ParamB);   // Param may be on stack in x32 vor not  // Exit with NtTerminateThread

// CreateRemoteThread requires PROCESS_VM_WRITE but that is too much to ask for a stealth thread injection
static NTSTATUS NTAPI NativeCreateThread(PVOID ThreadRoutine, PVOID ParamA, PVOID ParamB, HANDLE ProcessHandle, BOOL CrtSusp, PVOID* StackBase, SIZE_T* StackSize, HANDLE* ThreadHandle, ULONG* ThreadID)
{
 static const int PAGE_SIZE = 4096;
 NTSTATUS Status = 0;
 USER_STACK UserStack;
 CONTEXT Context;
 CLIENT_ID ClientId;
 PVOID  LocStackBase = NULL;
 SIZE_T LocStackSize = 0;
                              
 DBGMSG("ProcessHandle=%p, ThreadRoutine=%p, ParamA=%p, ParamB=%p",ProcessHandle,ThreadRoutine,ParamA,ParamB);
 UserStack.FixedStackBase  = NULL;
 UserStack.FixedStackLimit = NULL;
 if(!StackBase)StackBase = &LocStackBase;
 if(!StackSize)StackSize = &LocStackSize;
 if(!*StackBase)
  {
   *StackSize = PAGE_SIZE * 256;   // 1Mb
   Status = NtAllocateVirtualMemory(ProcessHandle, StackBase, 0, StackSize, MEM_RESERVE, PAGE_READWRITE);
   if(!NT_SUCCESS(Status)){DBGMSG("AVM Failed 1"); return Status;}

   UserStack.ExpandableStackBase   = &((PBYTE)*StackBase)[*StackSize];  
   UserStack.ExpandableStackLimit  = &((PBYTE)*StackBase)[*StackSize - PAGE_SIZE]; 
   UserStack.ExpandableStackBottom = *StackBase;
 
   SIZE_T StackReserve = PAGE_SIZE * 2;
   PVOID ExpandableStackBase = &((PBYTE)UserStack.ExpandableStackBase)[-(PAGE_SIZE * 2)];    
   Status = NtAllocateVirtualMemory(ProcessHandle, &ExpandableStackBase, 0, &StackReserve, MEM_COMMIT, PAGE_READWRITE);
   if(!NT_SUCCESS(Status)){DBGMSG("AVM Failed 2"); return Status;}
   ULONG OldProtect;
   StackReserve = PAGE_SIZE;
   Status = NtProtectVirtualMemory(ProcessHandle, &ExpandableStackBase, &StackReserve, PAGE_READWRITE | PAGE_GUARD, &OldProtect);   //create GUARD page           
   if(!NT_SUCCESS(Status)){DBGMSG("PVM Failed"); return Status;}
  }
   else
    {
     UserStack.ExpandableStackBase   = &((PBYTE)*StackBase)[*StackSize];   
     UserStack.ExpandableStackLimit  = &((PBYTE)*StackBase)[*StackSize - PAGE_SIZE]; 
     UserStack.ExpandableStackBottom = *StackBase;
    }
// Avoiding RtlInitializeContext because it uses NtWriteVirtualMemory for a Parameter on x32 (Process may be opened without rights to do that)
 Context.EFlags = 0x3000;    // ??? 0x200 ?
 Context.ContextFlags = CONTEXT_CONTROL|CONTEXT_INTEGER;   
#ifdef _AMD64_ 
 Context.Rsp  = (SIZE_T)UserStack.ExpandableStackBase;
 Context.Rip  = (SIZE_T)ThreadRoutine;
 Context.Rsp -= sizeof(void*) * 5;  // For a reserved block of 4 Arguments (RCX, RDX, R8, R9) and a fake ret addr
 Context.Rcx  = Context.Rax = (SIZE_T)ParamA;    // Match with fastcall
 Context.Rdx  = Context.Rbx = (SIZE_T)ParamB;
#else        // On WOW64 these are passed directly to new x64 thread context: EIP(RIP), ESP(R8), EBX(RDX), EAX(RCX)  // Later EAX and EBX will get to x32 entry point even without Wow64pCpuInitializeStartupContext  // On native x32/64 all registers are passed to thread`s entry point
 Context.Esp  = (SIZE_T)UserStack.ExpandableStackBase;
 Context.Eip  = (SIZE_T)ThreadRoutine;
 Context.Esp -= sizeof(void*) * 5;  // For a Return addr(fake, just to kepp frame right) and a possible Params for not fastcall ThreadProc if you prepared it in your stack
 Context.Ecx  = Context.Eax = (SIZE_T)ParamA;    // Match with fastcall if Wow64pCpuInitializeStartupContext is used
 Context.Edx  = Context.Ebx = (SIZE_T)ParamB;

 Context.ContextFlags |= CONTEXT_SEGMENTS;    // NOTE: Only Windows x32 requires segment registers to be set 
 Context.SegGs = 0x00;
 Context.SegFs = 0x38;
 Context.SegEs = 0x20;
 Context.SegDs = 0x20;
 Context.SegSs = 0x20;
 Context.SegCs = 0x18;  

 if(ProcessHandle == NtCurrentProcess)  
  {
   ((PVOID*)Context.Esp)[1] = ParamA;    // Win7 WOW64 pops ret addr from stack making this argument unavailable // Solution: Pass same value in ParamA and ParamB
   ((PVOID*)Context.Esp)[2] = ParamB;
  }
   else
    {
     PVOID Arr[] = {ParamA,ParamB};
     if(NTSTATUS stat = NtWriteVirtualMemory(ProcessHandle, &((PVOID*)Context.Esp)[1], &Arr, sizeof(Arr), 0)){DBGMSG("Write stack args failed with: %08X",stat);}   // It is OK to fail if no PROCESS_VM_WRITE
    }
#endif

 ULONG ThAcc = SYNCHRONIZE|THREAD_GET_CONTEXT|THREAD_SET_CONTEXT|THREAD_QUERY_INFORMATION|THREAD_SET_INFORMATION|THREAD_SUSPEND_RESUME|THREAD_TERMINATE;
 Status = NtCreateThread(ThreadHandle, ThAcc, nullptr, ProcessHandle, &ClientId, &Context, (PINITIAL_TEB)&UserStack, CrtSusp); // Always returns ACCESS_DENIED for any CFG enabled process? // The thread starts at specified IP and with specified SP and no return address // End it with NtTerminateThread  // CrtSusp must be exactly 1 to suspend     
#ifndef _AMD64_ 
 if((Status == STATUS_ACCESS_DENIED) && (FixWinTenWow64NtCreateThread() > 0))Status = NtCreateThread(ThreadHandle, ThAcc, nullptr, ProcessHandle, &ClientId, &Context, (PINITIAL_TEB)&UserStack, CrtSusp);     // Try to fix Wow64NtCreateThread bug in latest versions of Windows 10   
#endif
 if(!NT_SUCCESS(Status)){DBGMSG("CreateThread Failed"); return Status;}	
 if(ThreadID)*ThreadID = ULONG(ClientId.UniqueThread);              
 return Status;
}
//------------------------------------------------------------------------------------
#ifndef _AMD64_ 
// 00:  00 10 00 00 01 00 00 00 00 00 00 00 00 00 00 00 
// 10:  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  
// 20:  64 86 4C 01 00 00 00 00 00 00 00 00 00 00 00 00 
//
static int FixWinTenWow64NtCreateThread(void)
{
 static int LastResult = 0;
 if(LastResult != 0)return LastResult;
 UINT64 NTeb = GetNativeWowTebAddrWin10();
 if(!NTeb){LastResult=-1; return LastResult;}    // Not under WOW or an older system
 DBGMSG("Trying to fix Wow64: CurrTeb=%p, NTeb=%p",NtCurrentTeb(),(PVOID)NTeb);
 UINT64 TlsSlot = NTeb + 0x1480 + (sizeof(UINT64) * 10);  // For now in slot 10    // 0x1480 is offset of TlsSlots in x64 TEB
 UINT64 SubSysInfoAddr = *(UINT64*)TlsSlot;    // Lets hope that it will always be in low address space
 if(!SubSysInfoAddr){LastResult=-2; return LastResult;}  // No address in the slot!
 if(SubSysInfoAddr > 0xFFFFFFFFULL){LastResult=-3; return LastResult;}   // The Addr is unreachable from x32
 PBYTE SubSysInfo = (PBYTE)SubSysInfoAddr;
 DBGMSG("SubSysInfo block %p:",SubSysInfo);
 DBGMSG("\r\n%048.16D",SubSysInfo,SubSysInfo);   // As a separate message in case of incorrect pointer
 PDWORD pValue = (PDWORD)&SubSysInfo[0x20];
 DWORD  OldVal = *pValue;       // No way to check this address :(  (No Kernel32, no ntdll, no ExceptionHandler)
 if(OldVal != 0x014C8664){LastResult=-4; return LastResult;}   // Not a broken Windows 10    // 014C:IMAGE_FILE_MACHINE_I386 ;   8664:IMAGE_FILE_MACHINE_AMD64  // See IsWow64Process2   // Wow64NtCreateThread is broken since IsWow64Process2 has been added?
 DWORD   DllSize = 0;
// wchar_t DllName[] = {'w','o','w','6','4','.','d','l','l',0};
 DWORD64 wowdll = NWOW64E::GetModuleHandle64(ctENCSW(L"wow64.dll"), &DllSize);
 if(!wowdll || (DllSize < 4096)){LastResult=-5; return LastResult;} 
 DBGMSG("Wow64DllBase=%016llX, Wow64DllSize=%08X",wowdll,DllSize);

 DWORD64 WrMemProc = NWOW64E::GetProcAddressSimpleX64(NWOW64E::getNTDLL64(), ctENCSA("NtWriteVirtualMemory"));    
 if(!WrMemProc){LastResult=-6; return LastResult;} 

 PVOID  BlockBase = NULL;
 SIZE_T BlockSize = DllSize;
 if(NTSTATUS stat = NtAllocateVirtualMemory(NtCurrentProcess, &BlockBase, 0, &BlockSize, MEM_COMMIT, PAGE_READWRITE)){LastResult=-7; return LastResult;}
 NWOW64E::getMem64(BlockBase, wowdll, DllSize);
           
 DWORD OffsSigA = 0;
 DWORD OffsSigB = 0;
 DWORD AOffset  = 1024;
 DWORD RwImportRecRva = 0;
 DllSize -= 1024;
 struct SSigChk
  {
   static bool IsSigMatchForWow64NtCreateThread(PBYTE Addr)      // FF 15 ?? ?? ?? ?? 85 C0 0F 88 ?? ?? 00 00 66 44 39 6C 24 ?? 74 ?? B8 22 00 00 C0 E9 ?? ?? 00 00   
    {
     if(*(PWORD)&Addr[0] != 0x15FF)return false;
     if(*(PDWORD)&Addr[6] != 0x880FC085)return false;
     if(*(PDWORD)&Addr[12] != 0x44660000)return false;
     if(*(PDWORD)&Addr[15] != 0x246C3944)return false;
     if(Addr[20] != 0x74)return false;
     if(*(PDWORD)&Addr[22] != 0x000022B8)return false;
     if(*(PWORD)&Addr[26] != 0xE9C0)return false;
     if(*(PWORD)&Addr[30] != 0x0000)return false;
     return true;
    }
//----------------------------------------                                    
   static bool IsSigMatchForWow64pCpuInitializeStartupContext(PBYTE Addr)     // 4C 8D 44 24 ?? 41 B9 CC 02 00 00 48 8D 57 04 48 8B CB FF 15   
    {
     if(*(PDWORD)&Addr[0]  != 0x24448D4C)return false;
     if(*(PDWORD)&Addr[5]  != 0x02CCB941)return false;
     if(*(PDWORD)&Addr[9]  != 0x8D480000)return false;
     if(*(PDWORD)&Addr[13] != 0x8B480457)return false;
     if(*(PDWORD)&Addr[16] != 0x15FFCB8B)return false;
     return true;
    }
  };

 for(;(AOffset < DllSize) && (!OffsSigA || !OffsSigB);AOffset++)    
  {
   PBYTE Addr = (PBYTE)BlockBase + AOffset;
   if(!OffsSigA && SSigChk::IsSigMatchForWow64NtCreateThread(Addr))
    {
     OffsSigA = AOffset;
     DBGMSG("SignatureA at: %08X",AOffset);
    }
 /*  if(!OffsSigB && SSigChk::IsSigMatchForWow64pCpuInitializeStartupContext(Addr))
    {
     OffsSigB = AOffset;
     DBGMSG("SignatureB at: %08X",AOffset);
    }  */
  }

/* if(OffsSigB)        // Wow64pCpuInitializeStartupContext is broken too (NtReadVirtualMemory instead of final NtWriteVirtualMemory)
  {
   NPEFMT::SImportThunk<DWORD64>* ImportRec = NULL;
   if(NPEFMT::TFindImportRecord<DWORD64>((PBYTE)BlockBase, "ntdll.dll", "NtWriteVirtualMemory", NULL, &ImportRec, false))RwImportRecRva = (PBYTE)ImportRec - (PBYTE)BlockBase;   // if(TFindImportTable<DWORD64>((PBYTE)BlockBase, "ntdll.dll", NULL, &ImportTbl, false))  
     else {DBGMSG("Failed to find NtWriteVirtualMemory!");}
  } */
 DBGMSG("RwImportRecRva=%08X, OffsSigA=%08X, OffsSigB=%08X",RwImportRecRva,OffsSigA,OffsSigB);
 if(!OffsSigA /*|| !OffsSigB || !RwImportRecRva*/){LastResult=-8; return LastResult;} 
 NtFreeVirtualMemory(NULL,&BlockBase,&BlockSize,MEM_RELEASE); 
 
 {
  BYTE    Patch[]    = {0x75};   // Write JNZ instead of JZ
  DWORD64 PatchAddr  = wowdll + OffsSigA + 20;
  DWORD64 RegionBase = PatchAddr;
  DWORD64 RegionSize = sizeof(Patch) + 4;
  ULONG   OldProtect = 0;
  NWOW64E::ProtectVirtualMemory(NtCurrentProcess, &RegionBase, &RegionSize, PAGE_EXECUTE_READWRITE, &OldProtect);
  NWOW64E::setMem64(PatchAddr, &Patch, sizeof(Patch));   // Patch it or crash :)
  NWOW64E::ProtectVirtualMemory(NtCurrentProcess, &RegionBase, &RegionSize, OldProtect, &OldProtect);
  DBGMSG("Patched: Wow64NtCreateThread");
 }

/* {      // Useless: Wow64pCpuInitializeStartupContext is allowed to fail if NtReadVirtualMemory/NtWriteVirtualMemory fails
  DWORD64 PatchAddr  = wowdll + OffsSigB + 20;
  DWORD64 RegionBase = PatchAddr;
  DWORD64 RegionSize = sizeof(DWORD) + 4;
  ULONG   OldProtect = 0;
  DWORD   NewRvaAddr = AddrToRelAddr<long>(PatchAddr-2,6,wowdll+RwImportRecRva);       // Write rel of__imp_NtWriteVirtualMemory instead of __imp_NtReadVirtualMemory    // FF 15 NN NN NN NN
  NWOW64E::ProtectVirtualMemory(NtCurrentProcess, &RegionBase, &RegionSize, PAGE_EXECUTE_READWRITE, &OldProtect);
  NWOW64E::setMem64(PatchAddr, &NewRvaAddr, sizeof(NewRvaAddr));   // Patch it or crash :)
  NWOW64E::ProtectVirtualMemory(NtCurrentProcess, &RegionBase, &RegionSize, OldProtect, &OldProtect);
  DBGMSG("Patched: Wow64pCpuInitializeStartupContext");
 } */
                  
 LastResult = 1;
 return 1;
}
//------------------------------------------------------------------------------------
static inline void _stdcall RawThreadFixParams(PVOID* ParamA, PVOID* ParamB)     // Why no '__declspec(naked)' supported even for static functions?
{
 // put EAX and EBX to ParamA and ParamB
}
//------------------------------------------------------------------------------------
#else
static inline void _fastcall RawThreadFixParams(PVOID* ParamA, PVOID* ParamB){}
#endif
//------------------------------------------------------------------------------------
// Starting in Windows 8.1, GetVersion() and GetVersionEx() are subject to application manifestation
// See https://stackoverflow.com/questions/32115255/c-how-to-detect-windows-10
//
static inline DWORD _fastcall GetRealVersionInfo(PDWORD dwMajor=NULL, PDWORD dwMinor=NULL, PDWORD dwBuild=NULL, PDWORD dwPlatf=NULL)
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
}
//------------------------------------------------------------------------------------
/*
RtlUserThreadStart (Callback)    // CREATE_SUSPENDED thread`s IP 
  x32  x64
  EAX  RCX = ThreadProc
  EBX  RDX = ThreadParam

Native thread:   (Suspend is TRUE)
  x32  x64
  EAX  RCX = ThreadProc
  EBX  RDX = ThreadParam
*/
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
static jmp_buf jenv;  // setjmp env for the jump back into the fork() function

static int child_entry(void)  // entry point for our child thread process - just longjmp into fork
{
 longjmp(jenv, 1);
 return 0;
}
//------------------------------------------------------------------------------------
static void set_inherit_all(void)
{
/* ULONG  n = 0x1000;
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
 free(p); */
}
//------------------------------------------------------------------------------------
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

};
//------------------------------------------------------------------------------------
// TODO: Declare all required NtDll imports in NtDllEx.cpp and resolve syscalls at runtime there instead of ResolveSysSrvCallImports