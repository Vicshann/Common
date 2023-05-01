
#pragma once
/*
  Copyright (c) 2019 Victor Sheinmann, Vicshann@gmail.com

  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), 
  to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
  and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. 
*/

struct NInjLdr
{  
static const SIZE_T RemThModMarker = 0x0F;
#ifndef LOGMSG
#define LOGMSG(msg,...)
#endif

#ifndef DBGMSG
#define DBGMSG(msg,...)
#endif
                              
enum EMapFlf {mfNone, mfInjVAl=0x00100000,mfInjMap=0x00200000,mfInjAtom=0x00400000,mfInjWnd=0x00800000,  mfRunUAPC=0x01000000,mfRunRMTH=0x02000000,mfRunThHij=0x04000000,mfRawRMTH=0x08000000,  mfRawMod=0x10000000,mfX64Mod=0x20000000,mfResSyscall=0x40000000,mfNoThreadReport=0x80000000};  // These must not conflict with PE::EFixMod
//------------------------------------------------------------------------------------
static HANDLE OpenRemoteProcess(DWORD ProcessID, UINT Flags, bool Suspend=false, DWORD ExtraFlags=0)
{
 DWORD FVals = PROCESS_VM_OPERATION|PROCESS_QUERY_INFORMATION|ExtraFlags;
 if(Flags & mfRunRMTH)FVals |= PROCESS_CREATE_THREAD;
 if(Flags & mfInjVAl)FVals |= PROCESS_VM_WRITE;
 if(Flags & mfNoThreadReport)FVals |= PROCESS_VM_READ|PROCESS_VM_WRITE;    // Do not report created threads to DllMains and TLS callbacks by setting SkipThreadAttach in TEB->SameTebFlags
 if(Suspend)FVals |= PROCESS_SUSPEND_RESUME;
 HANDLE hResHndl = NULL;             
 CLIENT_ID CliID;
 OBJECT_ATTRIBUTES ObjAttr;
 CliID.UniqueThread  = 0;
 CliID.UniqueProcess = (HANDLE)SIZE_T(ProcessID);
 ObjAttr.Length = sizeof(ObjAttr);
 ObjAttr.RootDirectory = NULL;  
 ObjAttr.Attributes = 0;           // bInheritHandle ? 2 : 0;
 ObjAttr.ObjectName = NULL;
 ObjAttr.SecurityDescriptor = ObjAttr.SecurityQualityOfService = NULL;
 if(NTSTATUS stat = NtOpenProcess(&hResHndl, FVals, &ObjAttr, &CliID)){LOGMSG("Failed to open process: %08X(%u)", ProcessID,ProcessID);}
   else if(Suspend){NTSTATUS stat = NtSuspendProcess(hResHndl); DBGMSG("Entire process been suspended: Status=%08X, Handle=%p",stat,hResHndl);}
 return hResHndl;
}
//------------------------------------------------------------------------------------
static bool UnMapRemoteMemory(HANDLE hProcess, PVOID BaseAddr)
{
 LOGMSG("BaseAddr: %p", BaseAddr);
 return (STATUS_SUCCESS == NtUnmapViewOfSection(hProcess, BaseAddr));
}
//------------------------------------------------------------------------------------
// Fix relocs and move sections. Resolve NTDLL syscalls but leave other imports unresolved
private: static int PrepareRawModule(PBYTE LocalBase, PBYTE RemoteBase, PVOID NtDllBase, SIZE_T SysCallBlkOffs, SIZE_T SysCallBlkLen, UINT Flags, int MaxSecs)
{
 NPEFMT::TFixRelocations<NPEFMT::PECURRENT, 2>(LocalBase, RemoteBase);  // Do this BEFORE ResolveSysCallImports which will overwrite some affected parts
 if(Flags & mfResSyscall)
  {
   int len = NPEFMT::ResolveSysCallImports<NPEFMT::PECURRENT,false,true>(LocalBase, NtDllBase, &LocalBase[SysCallBlkOffs], &RemoteBase[SysCallBlkOffs], SysCallBlkLen, true);
   if(len < 0){LOGMSG("Failed to resolve syscalls: %i", len); return -1;}  
  }
 NPEFMT::DOS_HEADER     *DosHdr = (NPEFMT::DOS_HEADER*)LocalBase;
 NPEFMT::WIN_HEADER<NPEFMT::PECURRENT>  *WinHdr = (NPEFMT::WIN_HEADER<NPEFMT::PECURRENT>*)&LocalBase[DosHdr->OffsetHeaderPE];
 NPEFMT::DATA_DIRECTORY *ReloctDir = &WinHdr->OptionalHeader.DataDirectories.FixUpTable;
 NPEFMT::DATA_DIRECTORY* ImportDir = &WinHdr->OptionalHeader.DataDirectories.ImportTable;
 NPEFMT::DATA_DIRECTORY* ExportDir = &WinHdr->OptionalHeader.DataDirectories.ExportTable;
 NPEFMT::DATA_DIRECTORY* ResourDir = &WinHdr->OptionalHeader.DataDirectories.ResourceTable;
 ReloctDir->DirectoryRVA = ReloctDir->DirectorySize = 0;       // Relocs are done
 if(Flags & mfResSyscall)ImportDir->DirectoryRVA = ImportDir->DirectorySize = 0;       // Imports are resolved (Syscalls only)
 ExportDir->DirectoryRVA = ExportDir->DirectorySize = 0;   
 ResourDir->DirectoryRVA = ResourDir->DirectorySize = 0;   
 if((MaxSecs > 0) && (MaxSecs < WinHdr->FileHeader.SectionsNumber))WinHdr->FileHeader.SectionsNumber = MaxSecs;

 UINT SecArrOffs = DosHdr->OffsetHeaderPE+WinHdr->FileHeader.HeaderSizeNT+sizeof(NPEFMT::FILE_HEADER)+sizeof(DWORD);
 UINT SecsNum    = WinHdr->FileHeader.SectionsNumber;
 NPEFMT::MoveSections(SecArrOffs, SecsNum, LocalBase);

 EncryptModuleParts(LocalBase, NtDllBase, Flags);       // It need to save Flags at least        // GetModuleHandleA("ntdll.dll")
 return 0;
}
//------------------------------------------------------------------------------------
// ModuleData may be raw or already mapped module(If mapped, it must not have an encrypted headers)
//  MODULE | NTDLL_SYSCALLS | STACK
//  [      EXEC_RW        ]   [RW ]
//
public: static int InjModuleIntoProcessAndExec(HANDLE hProcess, PVOID ModuleData, SIZE_T ModuleSize, UINT Flags, int MaxSecs=-1, PVOID Param=NULL, PVOID RemoteAddr=NULL, PVOID NtDllBase=NULL, SIZE_T RawStackSize=0)  // Param is for lpReserved of DllMain when injecting using mfInjUAPC
{
 if(!ModuleData || !NPEFMT::IsValidPEHeader(ModuleData)){LOGMSG("Bad PE image: ModuleData=%p", ModuleData); return -1;}
 if(!(Flags & mfRawRMTH))RawStackSize = 0;      // No stack block needed
 if(RawStackSize)RawStackSize = ((RawStackSize + 0xFFFF) & ~0xFFFF);     // Is DEP may react to thread`s stack being in same executable block?    // Expect the entire stack block to be set to RW protection only
 SIZE_T SyscallsSize = (Flags & mfResSyscall)?4096:0;      // Align RawStackSize to 64k?  // 4096 for syscall block(Is enough?)   // Only syscall imports from NTDLL are resolved
 SIZE_T ModFullSize  = NPEFMT::SizeOfSections(ModuleData, MaxSecs, true, false);      // Module size after it is mapped and its sections are moved (A raw module may move its own sections)
 UINT   EPOffset     = NPEFMT::GetModuleEntryOffset((PBYTE)ModuleData, false);  // Flags & mfRawMod
 SIZE_T ExecBlkSize  = ((ModFullSize+SyscallsSize) + 0xFFFF) & ~0xFFFF;    // Size of a executable memory block
 SIZE_T ModASize     = ExecBlkSize + RawStackSize;    // Stack size must include syscall table if syscall resolving is forces
 PVOID  FlagArg      = NULL;
 PBYTE  Buf          = NULL;  

 if(!NtDllBase)NtDllBase = NPEFMT::GetNtDllBaseFast(false);    // Must be clean of any hooks and not a raw file 
 NtAllocateVirtualMemory(NtCurrentProcess,(PVOID*)&Buf,0,&ModASize,MEM_COMMIT,PAGE_EXECUTE_READWRITE);   // Temporary local buffer
 memcpy(Buf,ModuleData,ModuleSize);       // Copy before encryption
 ModuleData = Buf;        // Now points to prepared data
 if(MaxSecs > 0)ModuleSize = NPEFMT::SizeOfSections(ModuleData, MaxSecs, true, (Flags & mfRawMod));    
 if(Flags & mfInjVAl)        
  {
   if(NTSTATUS status = NtAllocateVirtualMemory(hProcess,&RemoteAddr,0,&ModASize,MEM_COMMIT,PAGE_EXECUTE_READWRITE)){LOGMSG("NtAllocateVirtualMemory failed(%08X): Size=%08X", status, ModASize); return -6;}     // RemoteAddr = (PBYTE)VirtualAllocEx(hProcess,NULL,ModSize,MEM_COMMIT,PAGE_EXECUTE_READWRITE);
   if((Flags & mfRawMod) && (PrepareRawModule((PBYTE)ModuleData, (PBYTE)RemoteAddr, NtDllBase, ModFullSize, SyscallsSize, Flags, MaxSecs) < 0))return -8;
   FlagArg = PVOID((SIZE_T)RemoteAddr | ((Flags >> 24) & RemThModMarker));     // Mark base address   // Passed to DllMain as hModule
#ifndef _AMD64_
   if(RawStackSize)
    {
     ModuleSize = ExecBlkSize + RawStackSize;
     *(PVOID*)&((PBYTE)ModuleData)[ModuleSize-(sizeof(void*)*4)] = FlagArg;   // 4 Extra reserved args
     *(PVOID*)&((PBYTE)ModuleData)[ModuleSize-(sizeof(void*)*3)] = FlagArg;   // Keep its copy for Win7 WOW64
    }
#endif
   if(NTSTATUS status = NtWriteVirtualMemory(hProcess, RemoteAddr, ModuleData, ExecBlkSize, &ModASize)){LOGMSG("NtWriteVirtualMemory failed(%08X): Size=%08X", status, ModASize); return -7;}
  }
 else if(Flags & mfInjMap)
  {
   OBJECT_ATTRIBUTES ObjAttrs = {0};
   ObjAttrs.Length = sizeof(OBJECT_ATTRIBUTES); 
   HANDLE hSec = NULL;
   LARGE_INTEGER MaxSize;
   MaxSize.QuadPart = ModASize;  
   if(NTSTATUS status = NtCreateSection(&hSec, SECTION_ALL_ACCESS, &ObjAttrs, &MaxSize, PAGE_EXECUTE_READWRITE, SEC_COMMIT, NULL)){LOGMSG("CreateMapping failed(%08X): Size=%08X", status, ModASize); return -2;}  
   PVOID LocalAddr = NULL;
   NTSTATUS res = NtMapViewOfSection(hSec,hProcess,&RemoteAddr,0,ModASize,NULL,&ModASize,ViewShare,0,PAGE_EXECUTE_READWRITE);  // Map remotely
   if(res){NtClose(hSec); LOGMSG("Remote MapSection failed: Addr=%p, Size=%08X", RemoteAddr, ModASize); return -3;}
   res = NtMapViewOfSection(hSec,NtCurrentProcess,&LocalAddr,0,ModASize,NULL,&ModASize,ViewShare,0,PAGE_EXECUTE_READWRITE);    // Map locally
   NtClose(hSec);
   if(res){LOGMSG("Local MapSection failed: Addr=%p, Size=%08X", LocalAddr, ModASize); return -4;}  
   if((Flags & mfRawMod) && (PrepareRawModule((PBYTE)ModuleData, (PBYTE)RemoteAddr, NtDllBase, ModFullSize, SyscallsSize, Flags, MaxSecs) < 0))return -8;
   FlagArg = PVOID((SIZE_T)RemoteAddr | ((Flags >> 24) & RemThModMarker));     // Mark base address   // Passed to DllMain as hModule
   memcpy(LocalAddr,ModuleData,ExecBlkSize);
#ifndef _AMD64_
   if(RawStackSize)
    {
     ModuleSize = ExecBlkSize + RawStackSize;
     *(PVOID*)&((PBYTE)LocalAddr)[ModuleSize-(sizeof(void*)*4)] = FlagArg;   // 4 Extra reserved args
     *(PVOID*)&((PBYTE)LocalAddr)[ModuleSize-(sizeof(void*)*3)] = FlagArg;   // Keep its copy for Win7 WOW64
    }
#endif
   NtUnmapViewOfSection(NtCurrentProcess, LocalAddr);
  }
 else if(Flags & mfInjAtom)
  {

  }
 else if(Flags & mfInjWnd)
  {

  }

 ModASize = 0;
 NtFreeVirtualMemory(NULL,&ModuleData,&ModASize,MEM_RELEASE);  
 PVOID RemEntry = &((PBYTE)RemoteAddr)[EPOffset];

 if(Flags & (mfInjVAl|mfInjMap))    // Mapped or Allocated remotely
  {
   if(Flags & mfRunUAPC)         // An APC queued to all existing threads
    {

    }   
   else if(Flags & mfRunRMTH)    // Calls DLLMain as ThreadProc(hModule)
    {
     HANDLE hThread  = NULL;
     DWORD  ThreadId = 0;
     DWORD  Status   = 0;
     BOOL   Suspend  = (Flags & mfNoThreadReport)?CREATE_SUSPENDED:0; 

     if(Flags & mfRawRMTH)   // Fails on latest Win 10 for x32 processes 
      {
       PVOID  StackBase = NULL; 
       SIZE_T StackSize = RawStackSize;
       if(RawStackSize)StackBase = &((PBYTE)RemoteAddr)[ExecBlkSize];   // At beginning or end?????????????    // TODO Check atack base at thread start!!!!!!!!!!!!!     + RawStackSize
       Status = NNTDLL::NativeCreateThread(RemEntry, FlagArg, FlagArg, hProcess, (bool)Suspend, &StackBase, &StackSize, &hThread, &ThreadId);   // On WinXP it receives APC for LdrInitializeThunk before it is suspended?   // WinXP, StartNewProcess: LdrpInitializeProcess:LdrpRunInitializeRoutines:Kernel32EntryPoint:CsrClientConnectToServer fails with 0xC0000142
      }
       else {hThread = CreateRemoteThread(hProcess,NULL,0,(LPTHREAD_START_ROUTINE)RemEntry,FlagArg,Suspend,&ThreadId); Status = GetLastError();}       // Firefox Quantum: It gets into LdrInitializeThunk but not into specified ThreadProc!!!!
     if(!hThread){LOGMSG("Failed to create a remote thread (%08X): RemEntry=%p, FlagArg=%p", Status, RemEntry, FlagArg); return -5;}
     LOGMSG("ThreadId=%u, RawTh=%u, RemEntry=%p, FlagArg=%p", ThreadId, bool(Flags & mfRawRMTH), RemEntry, FlagArg);
     if(Suspend)
      {
       if(SetSkipThreadReport(hProcess, hThread) < 0){LOGMSG("Failed to disable the thread reports!");}
       NtResumeThread(hThread, NULL);
      }
//     NtWaitForSingleObject(hThread, FALSE, NULL);   // WaitForSingleObject(hTh, INFINITE);   // No waiting - the thread may be reused
     NtClose(hThread);
    }
   else if(Flags & mfRunThHij)   // An existing thread hijacked
    {

    }
  }
 LOGMSG("RemoteAddr: %p", RemoteAddr);
 return 0;
}
//------------------------------------------------------------------------------------
static int SetSkipThreadReport(HANDLE hProcess, HANDLE hThread)  // See RtlIsCurrentThreadAttachExempt   // TODO: Move to NtDllEx
{
 THREAD_BASIC_INFORMATION tinf;
 ULONG RetLen = 0;
 HRESULT res  = NtQueryInformationThread(hThread,ThreadBasicInformation,&tinf,sizeof(THREAD_BASIC_INFORMATION),&RetLen);
 if(res)return -1; 

 SIZE_T ResLen = 0; 
 PBYTE TebAddr = (PBYTE)tinf.TebBaseAddress;
 BYTE teb[0x2000];
 for(int idx=0;idx < 2;idx++)
  {
   res = NtReadVirtualMemory(hProcess, TebAddr, &teb, sizeof(teb), &ResLen);   
   if(res)return -2; 

   bool IsTgtTebX32 = (*(PBYTE*)&teb[0x18] == TebAddr);  // NT_TIB
   bool IsTgtTebX64 = (*(PBYTE*)&teb[0x30] == TebAddr);  // NT_TIB
   if(!IsTgtTebX32 && !IsTgtTebX64)return -4;

   long WowTebOffset;
   USHORT* pSameTebFlags;
   if(IsTgtTebX32)
    {
     WowTebOffset  = *(long*)&teb[0x0FDC];
     pSameTebFlags = (USHORT*)&teb[0x0FCA]; 
    }
     else
      {
       WowTebOffset  = *(long*)&teb[0x180C];
       pSameTebFlags = (USHORT*)&teb[0x17EE];  
      }
   *pSameTebFlags |= 0x0008;   // SkipThreadAttach
   *pSameTebFlags &= ~0x0020;  // RanProcessInit   
   PBYTE Addr = &TebAddr[(PBYTE)pSameTebFlags - (PBYTE)&teb];
   LOGMSG("TEB=%p, pSameTebFlags=%p, IsTgtTebX32=%u, IsTgtTebX64=%u, WowTebOffset=%i", TebAddr, pSameTebFlags, IsTgtTebX32, IsTgtTebX64, WowTebOffset);
   res = NtWriteVirtualMemory(hProcess, Addr, pSameTebFlags, sizeof(USHORT), &ResLen);  
   if(res)return -5; 

   if(WowTebOffset == 0)break;
   TebAddr = &TebAddr[WowTebOffset];  // Next WOW TEB
  }
 return 0;
}
//------------------------------------------------------------------------------------
static void UnmapAndTerminateSelf(PVOID BaseAddr)  // TODO: Replace with chain of  NtUnmapViewOfSection(-1, hLibModule & 0xFFFFFFFFFFFFFFFC) or LdrUnloadDll(hLibModule) and RtlExitUserThread(v4);
{
 LOGMSG("Ejecting: %p", BaseAddr);
 NPEFMT::MakeFakeHeaderPE((PBYTE)BaseAddr);   // Don`t care if the header encrypted or not
 FreeLibraryAndExitThread(HMODULE((SIZE_T)BaseAddr | 3),0);  // Can unmap any PE image(Not removing it from list) or view of section ()
}
//------------------------------------------------------------------------------------
static BYTE EncryptModuleParts(PVOID BaseAddr, PVOID NtDllBase, UINT Flags)
{
 DWORD Ticks  = NNTDLL::GetTicks();   //  GetTickCount();
 BYTE EncKey  = 0;
 for(int idx=4;!EncKey && (idx < 28);idx++)EncKey = Ticks >> idx;
 Flags |= EncKey;
 UINT VirSize = 0;
 UINT RawSize = 0;
 NPEFMT::GetModuleSizes((PBYTE)BaseAddr, &RawSize, &VirSize);
 if(NPEFMT::IsValidModuleX64(BaseAddr)){Flags |= mfX64Mod; NPEFMT::TCryptSensitiveParts<NPEFMT::PETYPE64>((PBYTE)BaseAddr, Flags|NPEFMT::fmEncMode, Flags & mfRawMod);}   
  else NPEFMT::TCryptSensitiveParts<NPEFMT::PETYPE32>((PBYTE)BaseAddr, Flags|NPEFMT::fmEncMode, Flags & mfRawMod);   

 NPEFMT::DOS_HEADER* DosHdr = (NPEFMT::DOS_HEADER*)BaseAddr;     // This header may be encrypted
 DosHdr->Reserved1 = Flags;
 *(UINT64*)&DosHdr->Reserved2 = (UINT64)NtDllBase;
 *(UINT*)&DosHdr->Reserved2[10] = VirSize;
 *(UINT*)&DosHdr->Reserved2[14] = RawSize;
 *(PBYTE)BaseAddr = 0;     // Marker, First thead that enters will revert this to 'M'  // Helps to avoid multi-entering when using APC injection
 return EncKey;
}
//------------------------------------------------------------------------------------
__declspec(noinline) static PVOID PEImageInitialize(PVOID BaseAddr, PVOID NtDllBase=NULL, UINT* OffsToVA=NULL, bool RecryptHdr=true, UINT ExcludeFlg=(NPEFMT::fmCryImp|NPEFMT::fmCryExp|NPEFMT::fmCryRes))
{
 PBYTE ModuleBase = PBYTE((SIZE_T)BaseAddr & ~RemThModMarker);
 NPEFMT::DOS_HEADER* DosHdr = (NPEFMT::DOS_HEADER*)ModuleBase;
 UINT  Flags  = (DosHdr->Reserved1 & ~(ExcludeFlg|NPEFMT::fmSelfMov))|NPEFMT::fmFixSec|NPEFMT::fmFixImp|NPEFMT::fmFixRel;   // If you already resolved imports just destroy import descriptors
 UINT  RetFix = 0;
 int    mres  = 0;
 if(!NtDllBase)NtDllBase = (PVOID)*(UINT64*)&DosHdr->Reserved2;
 if(!NtDllBase)NPEFMT::GetNtDllBaseFast(false);
 if(Flags & mfX64Mod)mres = NPEFMT::TFixUpModuleInplace<NPEFMT::PETYPE64>(ModuleBase,NtDllBase,Flags,&RetFix);    
  else mres = NPEFMT::TFixUpModuleInplace<NPEFMT::PETYPE32>(ModuleBase,NtDllBase,Flags,&RetFix); 
 if(mres < 0)return NULL;
 if(OffsToVA)*OffsToVA = NPEFMT::FileOffsetToRvaConvert(ModuleBase, *OffsToVA);           
 if(RecryptHdr)NPEFMT::CryptSensitiveParts(ModuleBase, NPEFMT::fmCryHdr|NPEFMT::fmEncMode|(Flags & NPEFMT::fmEncKeyMsk), false); 
 if(Flags & NPEFMT::fmSelfMov)
  {
   PBYTE* RetPtr = (PBYTE*)_AddressOfReturnAddress();
   if(RetFix)*RetPtr += RetFix;   // After SelfMove need to fix every return address in this module
  }
 return ModuleBase;
}
//------------------------------------------------------------------------------------
_declspec(noinline) static PVOID ReflectiveRelocateSelf(PVOID BaseAddr, PVOID NtDllBase=NULL)    // Should not be called from deeper than DLLMain!
{
 PBYTE ModuleBase = PBYTE((SIZE_T)BaseAddr & ~RemThModMarker);
 NPEFMT::DOS_HEADER* DosHdr = (NPEFMT::DOS_HEADER*)ModuleBase;
 NTSTATUS (NTAPI* pNtAllocateVirtualMemory)(HANDLE ProcessHandle,PVOID* BaseAddress,ULONG_PTR ZeroBits,PSIZE_T RegionSize,ULONG AllocationType,ULONG Protect) = NULL;

 DWORD NNtAllocateVirtualMemory[] = {0x93BE8BB1u,0x9E9C9093u,0x96A99A8Bu,0x9E8A8B8Du,0x929AB293u,0xFF868D90u};  // NtAllocateVirtualMemory     
 NNtAllocateVirtualMemory[0] = ~NNtAllocateVirtualMemory[0];
 NNtAllocateVirtualMemory[1] = ~NNtAllocateVirtualMemory[1];
 NNtAllocateVirtualMemory[2] = ~NNtAllocateVirtualMemory[2];
 NNtAllocateVirtualMemory[3] = ~NNtAllocateVirtualMemory[3];
 NNtAllocateVirtualMemory[4] = ~NNtAllocateVirtualMemory[4];
 NNtAllocateVirtualMemory[5] = ~NNtAllocateVirtualMemory[5];
 if(!NtDllBase)NtDllBase = (PVOID)*(UINT64*)&DosHdr->Reserved2;
 if(!NtDllBase)NPEFMT::GetNtDllBaseFast(false);
 *(PVOID*)&pNtAllocateVirtualMemory = NPEFMT::TGetProcedureAddress<NPEFMT::PECURRENT>((PBYTE)NtDllBase, (LPSTR)&NNtAllocateVirtualMemory);
 if(!pNtAllocateVirtualMemory)return NULL;
 UINT VirSize = *(UINT*)&DosHdr->Reserved2[10];
 UINT RawSize = *(UINT*)&DosHdr->Reserved2[14];

 PVOID  BaseAddress = NULL;
 SIZE_T RegionSize  = VirSize;    
 NTSTATUS stat = pNtAllocateVirtualMemory(NtCurrentProcess, &BaseAddress, 0, &RegionSize, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
 if(stat)return NULL;
 PBYTE* RetPtr = (PBYTE*)_AddressOfReturnAddress();
 UINT   RetRVA = *RetPtr - ModuleBase;   // File Offset of return address    
 memcpy(BaseAddress, ModuleBase, RawSize);
 BaseAddr = PEImageInitialize(BaseAddress, (PVOID)NtDllBase, &RetRVA);   // After this we can access static variables
 if(!BaseAddr)return NULL;
 *RetPtr  = (PBYTE)BaseAddr + RetRVA;    // Return to the relocated copy
 return BaseAddr;
}
//------------------------------------------------------------------------------------
__declspec(noinline) static void RedirRet(PBYTE OldBase, PBYTE NewBase)  
{
 PBYTE RetAddr = (PBYTE)_ReturnAddress();
 RetAddr = &NewBase[RetAddr - OldBase];
*(PVOID*)_AddressOfReturnAddress() = RetAddr;
}
//------------------------------------------------------------------------------------
// Doesn`t uses LoadLibrary for RealDll mapping because of LoaderLock in DLL main from which it may be called
// NOTE: After hiding thes DLL call to LdrLoadDll will try to load it again
// NOTE: Unicode strings of DLL path is incorrect, buffer will overflow and old memory will leak
// NOTE: GetModuleHandle will return NULL
// NOTE: Any already resolved import from this DLL will be wrong
//
static int HideSelfProxyDll(PVOID DllBase, PVOID pNtDll, LPSTR RealDllPath, PVOID* NewBase, PVOID* EntryPT)   // Call this from DllMain
{    
 LOGMSG("DllBase=%p, pNtDll=%p, RealDllPath=%s",DllBase,pNtDll,RealDllPath);                        
   //  lstrcpyA(GetFileName(RealDllPath),"version.dll");
   //  LoadLibraryA(RealDllPath);
 UINT SysPLen = 0;
 int DelPos = -1;
 BYTE  DllPath[MAX_PATH];
 WCHAR SysPath[MAX_PATH];
 lstrcpyA((LPSTR)&DllPath,RealDllPath);
 LPSTR DllName = GetFileName((LPSTR)&DllPath);
 for(;RealDllPath[SysPLen] && (SysPLen < (MAX_PATH-1));SysPLen++)     // RealDllPath on stack will become invalid after relocation
  {
   SysPath[SysPLen] = RealDllPath[SysPLen]; 
   if((RealDllPath[SysPLen] == PATHDLML)||(RealDllPath[SysPLen] == PATHDLMR))DelPos = SysPLen+1;
  }
 SysPath[SysPLen] = 0;

 SIZE_T ModSize = GetRealModuleSize(DllBase);
 SIZE_T ModALen = ModSize;
 PVOID  ModCopy = NULL;
 NtAllocateVirtualMemory(NtCurrentProcess,&ModCopy,0,&ModALen,MEM_COMMIT,PAGE_EXECUTE_READWRITE);  //  PVOID  ModCopy = VirtualAlloc(NULL,ModSize,MEM_COMMIT,PAGE_EXECUTE_READWRITE); 
 memcpy(ModCopy,DllBase,ModSize);
 HANDLE hFile = CreateFileA(RealDllPath,GENERIC_READ|GENERIC_EXECUTE,FILE_SHARE_READ|FILE_SHARE_DELETE,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
 if(hFile == INVALID_HANDLE_VALUE){LOGMSG("Failed to open: %s", RealDllPath); return -1;}
 HANDLE hSec  = CreateFileMappingA(hFile,NULL,SEC_IMAGE|PAGE_EXECUTE_READ,0,0,NULL);                                                      // Mapping to same address may fail if there is not enough unallocated space. Add a dummy data space to make your proxy DLL of same size as an real one
 if(!hSec){NtClose(hFile); LOGMSG("Failed to create mapping for: %s", RealDllPath); return -2;}

 *NPEFMT::TBaseOfImagePtr<NPEFMT::PECURRENT>((PBYTE)ModCopy) = (NPEFMT::PECURRENT)DllBase;    // Need current Base for Reloc recalculation
 NPEFMT::TFixRelocations<NPEFMT::PECURRENT, false>((PBYTE)ModCopy, (PBYTE)ModCopy);
 DBGMSG("Start redirecting itself: NewBase=%p, Size=%08X",ModCopy,ModSize);
 RedirRet((PBYTE)DllBase, (PBYTE)ModCopy);   // After this we are inside of ModCopy
 DBGMSG("Done redirecting!");
 *(PVOID*)_AddressOfReturnAddress() = &((PBYTE)ModCopy)[(PBYTE)_ReturnAddress() - (PBYTE)DllBase];
 if(NtUnmapViewOfSection(NtCurrentProcess, DllBase)){LOGMSG("Failed to unmap this proxy dll: %p", DllBase);return -3;}  
 DBGMSG("Proxy dll unmapped"); 
 

 PVOID MapAddr = MapViewOfFileEx(hSec,FILE_MAP_EXECUTE|FILE_MAP_READ,0,0,0,DllBase);
 int err = GetLastError();     // <<<<<<<<<<<<<<<<<<<<<<
 NtClose(hSec);
 NtClose(hFile);
 if(!MapAddr){LOGMSG("Failed to map a real DLL(%u): %p!", err, DllBase); return -4;}
 DBGMSG("A real dll is unmapped");
 NPEFMT::TSectionsProtectRW<NPEFMT::PECURRENT>((PBYTE)MapAddr, false);
 DBGMSG("A real dll`s memory is unprotected");
 NPEFMT::TFixRelocations<NPEFMT::PECURRENT, false>((PBYTE)MapAddr, (PBYTE)MapAddr);
 DBGMSG("A real dll`s relocs fixed");
   DWORD NLdrLoadDll[] = {~0x4C72644Cu, ~0x4464616Fu, ~0x00006C6Cu};  // LdrLoadDll     
   NLdrLoadDll[0] = ~NLdrLoadDll[0];
   NLdrLoadDll[1] = ~NLdrLoadDll[1];
   NLdrLoadDll[2] = ~NLdrLoadDll[2];
   PVOID Proc = NPEFMT::TGetProcedureAddress<NPEFMT::PECURRENT>((PBYTE)pNtDll, (LPSTR)&NLdrLoadDll);
 DBGMSG("LdrLoadDll: %p",Proc);
 NPEFMT::TResolveImports<NPEFMT::PECURRENT>((PBYTE)MapAddr, Proc, 0);
 DBGMSG("A real dll`s imports resolved");
 NPEFMT::TSectionsProtectRW<NPEFMT::PECURRENT>((PBYTE)MapAddr, true);
 DBGMSG("A real dll`s memory protection restored");
 
 PVOID ModEP = NPEFMT::GetLoadedModuleEntryPoint(DllBase);         // New EP address (A real system module)  
 NPEFMT::DOS_HEADER *DosHdr = (NPEFMT::DOS_HEADER*)DllBase;
 NPEFMT::WIN_HEADER<NPEFMT::PECURRENT>  *WinHdr = (NPEFMT::WIN_HEADER<NPEFMT::PECURRENT>*)&((PBYTE)DllBase)[DosHdr->OffsetHeaderPE];              
 PEB_LDR_DATA* ldr = NtCurrentTeb()->ProcessEnvironmentBlock->Ldr;
 DBGMSG("PEB_LDR_DATA: %p",ldr);
 for(LDR_DATA_TABLE_ENTRY_MO* me = ldr->InMemoryOrderModuleList.Flink;me != (LDR_DATA_TABLE_ENTRY_MO*)&ldr->InMemoryOrderModuleList;me = me->InMemoryOrderLinks.Flink)     // Or just use LdrFindEntryForAddress?
  {
   DBGMSG("Begin LDR_DATA_TABLE_ENTRY_MO: %p",me);
   if(me->DllBase == DllBase)
    {
     DBGMSG("Updating DLL info: %ls",(me->FullDllName.Length)?(me->FullDllName.Buffer):(L""));
     me->EntryPoint = ModEP;                      // Set EP of a real module
     me->SizeOfImage = WinHdr->OptionalHeader.SizeOfImage;
     me->TimeDateStamp = WinHdr->FileHeader.TimeDateStamp;
     if(me->FullDllName.Length >= (SysPLen*sizeof(WCHAR)))     // Skip renaming if a system DLL path is longer
      {
       memcpy(me->FullDllName.Buffer,&SysPath,(SysPLen+1)*sizeof(WCHAR));
       me->FullDllName.Length = SysPLen * sizeof(WCHAR); 
       me->FullDllName.MaximumLength = me->FullDllName.Length + sizeof(WCHAR);
       if(DelPos > 0)
        {
         me->BaseDllName.Buffer = &me->FullDllName.Buffer[DelPos];
         me->BaseDllName.Length = SysPLen - DelPos;
         me->BaseDllName.MaximumLength = me->BaseDllName.Length + sizeof(WCHAR);
        }
      }
     DBGMSG("Done updating DLL info");
    }
     else 
      {
       DBGMSG("Fixing moved import for: %p, %ls", me->DllBase,(me->FullDllName.Length)?(me->FullDllName.Buffer):(L""));
       NPEFMT::DOS_HEADER     *DosHdr    = (NPEFMT::DOS_HEADER*)me->DllBase;
       NPEFMT::WIN_HEADER<NPEFMT::PECURRENT> *WinHdr = (NPEFMT::WIN_HEADER<NPEFMT::PECURRENT>*)&((PBYTE)me->DllBase)[DosHdr->OffsetHeaderPE];
       NPEFMT::DATA_DIRECTORY *ImportDir = &WinHdr->OptionalHeader.DataDirectories.ImportTable;       // ImportAddressTable may not be inside it!
       if(ImportDir->DirectoryRVA)NPEFMT::TResolveImportsForMod<NPEFMT::PECURRENT>(DllName, (PBYTE)me->DllBase, (PBYTE)DllBase);  // Reimport api to a real DLL
       DBGMSG("Done fixing moved import");
      }
   DBGMSG("End LDR_DATA_TABLE_ENTRY_MO: %p",me);
  }
 if(EntryPT)*EntryPT = ModEP;
 if(NewBase)*NewBase = ModCopy;
 return (MapAddr == DllBase);
}
//------------------------------------------------------------------------------------
PVOID LoadRawDllFile(LPSTR DllPath)
{   
 HANDLE hFile = CreateFileA(DllPath,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL|FILE_FLAG_SEQUENTIAL_SCAN,NULL);   // TODO: Receive this from debugger to avoid CreateFile("ntdll.dll") detection
 if(hFile == INVALID_HANDLE_VALUE)return NULL;
 DWORD Result   = 0;
 DWORD FileSize = GetFileSize(hFile,NULL);
 PBYTE pLibMem  = (PBYTE)VirtualAlloc(NULL,FileSize,MEM_COMMIT,PAGE_EXECUTE_READWRITE);  // if(Ptr)VirtualFree(Ptr,0,MEM_RELEASE);
 if(FileSize)ReadFile(hFile,pLibMem,FileSize,&Result,NULL);
 CloseHandle(hFile); 
 NPEFMT::TFixRelocations<NPEFMT::PECURRENT, true>(pLibMem, pLibMem);  // Left it raw
 return pLibMem;
}
//------------------------------------------------------------------------------------

/*
752D0000 00001000 kernel32.dll                                                                   IMG -R--- ERWC-
752D1000 0000F000 Reserved (752D0000)                                                            IMG       ERWC-
752E0000 00066000  ".text"                                            Executable code            IMG ER--- ERWC-
75346000 0000A000 Reserved (752D0000)                                                            IMG       ERWC-
75350000 00026000  ".rdata"                                           Read-only initialized data IMG -R--- ERWC-
75376000 0000A000 Reserved (752D0000)                                                            IMG       ERWC-
75380000 00001000  ".data"                                            Initialized data           IMG -RW-- ERWC-
75381000 0000F000 Reserved (752D0000)                                                            IMG       ERWC-
75390000 00001000  ".didat"                                                                      IMG -R--- ERWC-
75391000 0000F000 Reserved (752D0000)                                                            IMG       ERWC-
753A0000 00001000  ".rsrc"                                            Resources                  IMG -R--- ERWC-
753A1000 0000F000 Reserved (752D0000)                                                            IMG       ERWC-
753B0000 00005000  ".reloc"                                           Base relocations           IMG -R--- ERWC-
753B5000 0000B000 Reserved (752D0000)                                                            IMG       ERWC-
*/
/*__declspec(noinline) static PVOID FindNtDllFromAddr(PVOID Addr) // USELESS, see kernel32 mapping above  // May crash if Addr is not inside a valid module   // NOTE: It is possible to pass ntdll base from a caller process instead
{
 UINT64 NtName = ~0x6C642E6C6C64746E;  // 'ntdll.dl'
 SIZE_T MemPtr = (SIZE_T)Addr & PLATMEMALIGNMSK;
 while(!IsValidPEHeader((PVOID)MemPtr))MemPtr -= PLATMEMALIGN;
 LPSTR ModName = GetExpModuleName((PVOID)MemPtr, false);
 if(*(UINT64*)ModName == ~NtName)return (PVOID)MemPtr;  // This is ntdll.dll    // TODO: Check case?
 BYTE LibName[10];
 *(UINT64*)&LibName = ~NtName;
 LibName[8] = 'l';
 LibName[9] = 0;
 SImportThunk<PECURRENT>* AddrRec = NULL;
 if(!TFindImportRecord<PECURRENT>((PBYTE)MemPtr, (LPSTR)&LibName, (LPSTR)&LibName, NULL, &AddrRec, false) || !AddrRec)return NULL;
 MemPtr  = (SIZE_T)AddrRec->Value & PLATMEMALIGNMSK; 
 while(!IsValidPEHeader((PVOID)MemPtr))MemPtr -= PLATMEMALIGN;
 ModName = GetExpModuleName((PVOID)MemPtr, false);
 if(*(UINT64*)ModName != ~NtName)return NULL;
 return (PVOID)MemPtr;
}*/
//------------------------------------------------------------------------------------


};