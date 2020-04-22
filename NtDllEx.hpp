
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
#include "ntdll.h"
//---------------------------------------------------------------------------
struct NNTDLL   // NT system service calls encapsulation
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
static PVOID GetModuleBaseLdr(LPSTR ModName, long* BaseIdx=NULL)  // NOTE: No loader locking used!
{
 if(!ModName)return NtCurrentTeb()->ProcessEnvironmentBlock->ImageBaseAddress;
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
static HANDLE OpenBeep(void)
{
 UNICODE_STRING str;
 OBJECT_ATTRIBUTES attr;
 IO_STATUS_BLOCK iost;
 HANDLE hBeep = NULL;

 RtlInitUnicodeString(&str, L"\\Device\\Beep");
 attr.Length = sizeof(OBJECT_ATTRIBUTES);
 attr.RootDirectory = 0;
 attr.ObjectName = &str;
 attr.Attributes = 0;
 attr.SecurityDescriptor = 0;
 attr.SecurityQualityOfService = 0;
 NtCreateFile(&hBeep, GENERIC_READ | GENERIC_WRITE, &attr, &iost, 0, 0, 0, FILE_OPEN, 0, 0, 0);
 return hBeep;
}
//------------------------------------------------------------------------------------
static void DoBeep(HANDLE hBeep)
{   
 struct {
  ULONG uFrequency;
  ULONG uDuration;
 } param;
 IO_STATUS_BLOCK iost;
 param.uFrequency = 2500;
 param.uDuration  = 10;
 NtDeviceIoControlFile(hBeep, 0, 0, 0, &iost, 0x00010000, &param, sizeof(param), 0, 0);
}
//------------------------------------------------------------------------------------
typedef void (_fastcall *PNT_THREAD_PROC)(LPVOID Param);

static NTSTATUS NTAPI NativeCreateThread(PVOID ThreadRoutine, PVOID Param, HANDLE ProcessHandle, BOOL CrtSusp, PVOID* StackBase, SIZE_T* StackSize, HANDLE* ThreadHandle, ULONG* ThreadID)
{
 static const int PAGE_SIZE = 4096;
 NTSTATUS Status = 0;
 USER_STACK UserStack;
 CONTEXT Context;
 CLIENT_ID ClientId;
 PVOID  LocStackBase = NULL;
 SIZE_T LocStackSize = 0;

 UserStack.FixedStackBase  = NULL;
 UserStack.FixedStackLimit = NULL;
 if(!StackBase)StackBase = &LocStackBase;
 if(!StackSize)StackSize = &LocStackSize;
 if(!*StackBase)
  {
   *StackSize = PAGE_SIZE * 256;   // 1Mb
   Status = NtAllocateVirtualMemory(ProcessHandle, StackBase, 0, StackSize, MEM_RESERVE, PAGE_READWRITE);
   if(!NT_SUCCESS(Status))return Status;

   UserStack.ExpandableStackBase   = &((PBYTE)*StackBase)[*StackSize];  
   UserStack.ExpandableStackLimit  = &((PBYTE)*StackBase)[*StackSize - PAGE_SIZE]; 
   UserStack.ExpandableStackBottom = *StackBase;
 
   SIZE_T StackReserve = PAGE_SIZE * 2;
   PVOID ExpandableStackBase = &((PBYTE)UserStack.ExpandableStackBase)[-(PAGE_SIZE * 2)];    
   Status = NtAllocateVirtualMemory(ProcessHandle, &ExpandableStackBase, 0, &StackReserve, MEM_COMMIT, PAGE_READWRITE);
   if(!NT_SUCCESS(Status))return Status;
   ULONG OldProtect;
   StackReserve = PAGE_SIZE;
   Status = NtProtectVirtualMemory(ProcessHandle, &ExpandableStackBase, &StackReserve, PAGE_READWRITE | PAGE_GUARD, &OldProtect);   //create GUARD page           
   if(!NT_SUCCESS(Status))return Status;
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
 Context.Rcx  = (SIZE_T)Param;
#else
 Context.Esp  = (SIZE_T)UserStack.ExpandableStackBase;
 Context.Eip  = (SIZE_T)ThreadRoutine;
 Context.Esp -= sizeof(void*) * 5;  // For a Return addr(fake, just to kepp frame right) and a possible Params for not fastcall ThreadProc if you prepared it in your stack
 Context.Ecx  = (SIZE_T)Param;

 Context.ContextFlags |= CONTEXT_SEGMENTS;    // NOTE: Only Windows x32 requires segment registers to be set 
 Context.SegGs = 0x00;
 Context.SegFs = 0x38;
 Context.SegEs = 0x20;
 Context.SegDs = 0x20;
 Context.SegSs = 0x20;
 Context.SegCs = 0x18;  
#endif

 Status = NtCreateThread(ThreadHandle, SYNCHRONIZE|THREAD_GET_CONTEXT|THREAD_SET_CONTEXT|THREAD_QUERY_INFORMATION|THREAD_SET_INFORMATION|THREAD_SUSPEND_RESUME|THREAD_TERMINATE, NULL, ProcessHandle, &ClientId, &Context, (PINITIAL_TEB)&UserStack, CrtSusp);  // The thread starts at specified IP and with specified SP and no return address // End it with NtTerminateThread     
 if(!NT_SUCCESS(Status))return Status;	
 if(ThreadID)*ThreadID = ULONG(ClientId.UniqueThread);              
 return Status;
}
//------------------------------------------------------------------------------------
// Starting in Windows 8.1, GetVersion() and GetVersionEx() are subject to application manifestation
// See https://stackoverflow.com/questions/32115255/c-how-to-detect-windows-10
//
static inline DWORD _fastcall GetRealVersionInfo(PDWORD dwMajor=NULL, PDWORD dwMinor=NULL, PDWORD dwBuild=NULL, PDWORD dwPlatf=NULL)
{
 PPEB teb = NtCurrentPeb();
 if(dwMajor)*dwMajor = teb->OSMajorVersion;
 if(dwMinor)*dwMinor = teb->OSMinorVersion;
 if(dwBuild)*dwBuild = teb->OSBuildNumber;
 if(dwPlatf)*dwPlatf = teb->OSPlatformId;
 DWORD Composed = (teb->OSPlatformId << 16)|(teb->OSMinorVersion << 8)|teb->OSMajorVersion;
 return Composed;
}
//---------------------------------------------------------------------------
static int GetMappedFilePath(HANDLE hProcess, PVOID BaseAddr, PWSTR PathBuf, UINT BufByteSize)   // Returns as '\Device\HarddiskVolume'
{
 SIZE_T RetLen;           
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

};
//------------------------------------------------------------------------------------
// TODO: Declare all required NtDll imports in NtDllEx.cpp and resolve syscalls at runtime there instead of ResolveSysSrvCallImports