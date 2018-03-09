
#pragma once
/*
  Copyright (c) 2018 Victor Sheinmann, Vicshann@gmail.com

  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), 
  to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
  and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. 
*/

struct InjLdr
{                                
enum EMapFlf {mfNone, mfInjVAl=0x00100000,mfInjMap=0x00200000,mfInjAtom=0x00400000,mfInjWnd=0x00800000,  mfRunUAPC=0x01000000,mfRunRMTH=0x02000000,mfRunThHij=0x04000000,  mfRawMod=0x10000000,mfX64Mod=0x20000000};  // These must not conflict with PE::EFixMod
//------------------------------------------------------------------------------------
static HANDLE OpenRemoteProcess(DWORD ProcessID, UINT Flags)
{
 DWORD FVals = PROCESS_VM_OPERATION|PROCESS_QUERY_INFORMATION;
 if(Flags & mfRunRMTH)FVals |= PROCESS_CREATE_THREAD;
 if(Flags & mfInjVAl)FVals |= PROCESS_VM_WRITE;
 HANDLE res = OpenProcess(FVals,false,ProcessID);
 if(!res){LOGMSG("Failed to open process: %08X(%u)", ProcessID,ProcessID);}
 return res;
}
//------------------------------------------------------------------------------------
static bool UnMapRemoteMemory(HANDLE hProcess, PVOID BaseAddr)
{
 LOGMSG("BaseAddr: %p", BaseAddr);
 return (STATUS_SUCCESS == NtUnmapViewOfSection(hProcess, BaseAddr));
}
//------------------------------------------------------------------------------------
// ModuleData may be raw or already mapped module(If mapped, it must not have an encrypted headers)
//
static int InjModuleIntoProcessAndExec(HANDLE hProcess, PVOID ModuleData, UINT ModuleSize, UINT Flags, PVOID Param=NULL, PVOID Addr=NULL)  // Param is for lpReserved of DllMain when injecting using mfInjUAPC
{
 if(!ModuleData || !IsValidPEHeader(ModuleData)){LOGMSG("Bad PE image: ModuleData=%p", ModuleData); return -1;}
 SIZE_T MapModSize = GetImageSize(ModuleData);
 SIZE_T ModSize = (MapModSize > ModuleSize)?(MapModSize):(ModuleSize);   
 UINT  EPOffset = GetModuleEntryOffset((PBYTE)ModuleData, Flags & mfRawMod);
 PVOID RemoteAddr = Addr;
 PBYTE  Buf = (PBYTE)VirtualAlloc(NULL,ModSize,MEM_COMMIT,PAGE_EXECUTE_READWRITE);
 memcpy(Buf,ModuleData,ModuleSize);       // Copy before encryption
 ModuleData = Buf;
 CryptModule(ModuleData, Flags);       // It need to save Flags at least
 if(Flags & mfInjVAl)
  {
   RemoteAddr = (PBYTE)VirtualAllocEx(hProcess,NULL,ModSize,MEM_COMMIT,PAGE_EXECUTE_READWRITE);
   if(!RemoteAddr){LOGMSG("VirtualAllocEx failed(%u): Size=%08X", GetLastError(), ModSize); return -6;}
   if(!WriteProcessMemory(hProcess,RemoteAddr,ModuleData,ModSize,NULL)){LOGMSG("WriteProcessMemory failed(%u): Size=%08X", GetLastError(), ModSize); return -7;}
  }
 else if(Flags & mfInjMap)
  {
   HANDLE hSec = CreateFileMappingW(INVALID_HANDLE_VALUE,NULL,PAGE_EXECUTE_READWRITE|SEC_COMMIT,0,ModSize,NULL);
   if(!hSec){LOGMSG("CreateMapping failed(%u): Size=%08X", GetLastError(), ModSize); return -2;}
   PVOID LocalAddr = NULL;
   NTSTATUS res = NtMapViewOfSection(hSec,hProcess,&RemoteAddr,0,ModSize,NULL,&ModSize,ViewShare,0,PAGE_EXECUTE_READWRITE);
   if(res){CloseHandle(hSec); LOGMSG("Remote MapSection failed: Addr=%p, Size=%08X", RemoteAddr, ModSize); return -3;}
   res = NtMapViewOfSection(hSec,GetCurrentProcess(),&LocalAddr,0,ModSize,NULL,&ModSize,ViewShare,0,PAGE_EXECUTE_READWRITE);
   CloseHandle(hSec);
   if(res){LOGMSG("Local MapSection failed: Addr=%p, Size=%08X", LocalAddr, ModSize); return -4;}  
   memcpy(LocalAddr,ModuleData,ModSize);
   NtUnmapViewOfSection(GetCurrentProcess(), LocalAddr);
  }
 else if(Flags & mfInjAtom)
  {

  }
 else if(Flags & mfInjWnd)
  {

  }

 VirtualFree(ModuleData,0,MEM_RELEASE);
 PVOID RemEntry = &((PBYTE)RemoteAddr)[EPOffset];

 if(Flags & (mfInjVAl|mfInjMap))
  {
   if(Flags & mfRunUAPC)
    {

    }   
   else if(Flags & mfRunRMTH)
    {
     PVOID FlagArg = PVOID((SIZE_T)RemoteAddr | 0xFF);     // Mark base address
     HANDLE hTh = CreateRemoteThread(hProcess,NULL,0,(LPTHREAD_START_ROUTINE)RemEntry,FlagArg,0,NULL);
     if(!hTh){LOGMSG("Failed to create a remote thread (%u): RemEntry=%p, FlagArg=%p", GetLastError(), RemEntry, FlagArg); return -5;}
     WaitForSingleObject(hTh, INFINITE);
    }
   else if(Flags & mfRunThHij)
    {

    }
  }
 LOGMSG("RemoteAddr: %p", RemoteAddr);
 return 0;
}
//------------------------------------------------------------------------------------
static void UnmapAndTerminateSelf(PVOID BaseAddr)
{
 LOGMSG("Ejecting: %p", BaseAddr);
 MakeFakeHeaderPE((PBYTE)BaseAddr);   // Don`t care if the header encrypted or not
 FreeLibraryAndExitThread(HMODULE((SIZE_T)BaseAddr | 3),0);  // Can unmap any DLL(Not removing it from list) or view of section
}
//------------------------------------------------------------------------------------
static BYTE CryptModule(PVOID BaseAddr, UINT Flags)
{
 BYTE EncKey = GetTickCount() >> 4;
 Flags |= EncKey;
 if(IsValidModuleX64(BaseAddr)){Flags |= mfX64Mod; TCryptSensitiveParts<PETYPE64>((PBYTE)BaseAddr, Flags|fmEncMode, Flags & mfRawMod);}
  else TCryptSensitiveParts<PETYPE32>((PBYTE)BaseAddr, Flags|fmEncMode|EncKey, Flags & mfRawMod);   
 DOS_HEADER* DosHdr = (DOS_HEADER*)BaseAddr;
 DosHdr->Reserved1 = Flags;
 *(PVOID*)&DosHdr->Reserved2 = GetModuleHandleA("ntdll.dll");
 *(PBYTE)BaseAddr = 0;     // Marker, First thead that enters will revert this to 'M'  // Helps to avoid mult-entering when using APC injection
 return EncKey;
}
//------------------------------------------------------------------------------------
__declspec(noinline) static HMODULE ModFixInplaceSelf(PVOID BaseAddr, UINT ExcludeFlg=(fmCryImp|fmCryExp|fmCryRes), bool RecryptHdr=true)
{
 PBYTE ModuleBase = PBYTE((SIZE_T)BaseAddr & ~0xFFF);
 DOS_HEADER* DosHdr = (DOS_HEADER*)ModuleBase;
 UINT Flags   = (DosHdr->Reserved1 & ~ExcludeFlg)|fmFixSec|fmFixImp|fmFixRel|fmSelfMov;
 UINT RetFix  = 0;
 PVOID pNtDll = *(PVOID*)&DosHdr->Reserved2;
 if(Flags & mfX64Mod)TFixUpModuleInplace<PETYPE64>(ModuleBase,pNtDll,Flags,&RetFix);
  else TFixUpModuleInplace<PETYPE32>(ModuleBase,pNtDll,Flags,&RetFix);
 if(RecryptHdr)CryptSensitiveParts(ModuleBase, fmCryHdr|fmEncMode|(Flags & fmEncKeyMsk), false); 
 if(RetFix)*((PBYTE*)_AddressOfReturnAddress()) += RetFix;   // After SelfMove need to fix every return address in this module
 return (HMODULE)ModuleBase;
}
//------------------------------------------------------------------------------------
__declspec(noinline) static void RedirRet(PBYTE OldBase, PBYTE NewBase)  
{
 PBYTE RetAddr = (PBYTE)_ReturnAddress();
 RetAddr = &NewBase[RetAddr - OldBase];
*(PVOID*)_AddressOfReturnAddress() = RetAddr;
}
//------------------------------------------------------------------------------------
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
 PVOID  ModCopy = VirtualAlloc(NULL,ModSize,MEM_COMMIT,PAGE_EXECUTE_READWRITE);  
 memcpy(ModCopy,DllBase,ModSize);
 HANDLE hFile = CreateFileA(RealDllPath,GENERIC_READ|GENERIC_EXECUTE,FILE_SHARE_READ|FILE_SHARE_DELETE,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
 if(hFile == INVALID_HANDLE_VALUE){LOGMSG("Failed to open: %s", RealDllPath); return -1;}
 HANDLE hSec  = CreateFileMappingA(hFile,NULL,SEC_IMAGE|PAGE_EXECUTE_READ,0,0,NULL);                                                      // Mapping to same address may fail if there is not enough unallocated space. Add a dummy data space to make your proxy DLL of same size as an real one
 if(!hSec){CloseHandle(hFile); LOGMSG("Failed to create mapping for: %s", RealDllPath); return -2;}

 *TBaseOfImagePtr<PECURRENT>((PBYTE)ModCopy) = (PECURRENT)DllBase;    // Need current Base for Reloc recalculation
 TFixRelocations<PECURRENT>((PBYTE)ModCopy, false);
 DBGMSG("Start redirecting itself: NewBase=%p, Size=%08X",ModCopy,ModSize);
 RedirRet((PBYTE)DllBase, (PBYTE)ModCopy);   // After this we are inside of ModCopy
 DBGMSG("Done redirecting!");
 *(PVOID*)_AddressOfReturnAddress() = &((PBYTE)ModCopy)[(PBYTE)_ReturnAddress() - (PBYTE)DllBase];
 if(NtUnmapViewOfSection(NtCurrentProcess, DllBase)){LOGMSG("Failed to unmap this proxy dll: %p", DllBase);return -3;}  
 DBGMSG("Proxy dll unmapped");                                                                  
 PVOID MapAddr = MapViewOfFileEx(hSec,FILE_MAP_EXECUTE|FILE_MAP_READ,0,0,0,DllBase);
 CloseHandle(hSec);
 CloseHandle(hFile);
 if(!MapAddr){LOGMSG("Failed to map a real DLL!"); return -4;}
 DBGMSG("A real dll is unmapped");
 TSectionsProtectRW<PECURRENT>((PBYTE)MapAddr, false);
 DBGMSG("A real dll`s memory is unprotected");
 TFixRelocations<PECURRENT>((PBYTE)MapAddr, false);
 DBGMSG("A real dll`s relocs fixed");
   DWORD NLdrLoadDll[] = {~0x4C72644C, ~0x4464616F, ~0x00006C6C};  // LdrLoadDll     
   NLdrLoadDll[0] = ~NLdrLoadDll[0];
   NLdrLoadDll[1] = ~NLdrLoadDll[1];
   NLdrLoadDll[2] = ~NLdrLoadDll[2];
   PVOID Proc = TGetProcedureAddress<PECURRENT>((PBYTE)pNtDll, (LPSTR)&NLdrLoadDll);
 DBGMSG("LdrLoadDll: %p",Proc);
 TResolveImports<PECURRENT>((PBYTE)MapAddr, Proc, 0);
 DBGMSG("A real dll`s imports resolved");
 TSectionsProtectRW<PECURRENT>((PBYTE)MapAddr, true);
 DBGMSG("A real dll`s memory protection restored");
 
 PVOID ModEP = GetLoadedModuleEntryPoint(DllBase);         // New EP address (A real system module)  
 DOS_HEADER *DosHdr = (DOS_HEADER*)DllBase;
 WIN_HEADER<PECURRENT>  *WinHdr = (WIN_HEADER<PECURRENT>*)&((PBYTE)DllBase)[DosHdr->OffsetHeaderPE];              
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
       DOS_HEADER     *DosHdr    = (DOS_HEADER*)me->DllBase;
       WIN_HEADER<PECURRENT> *WinHdr = (WIN_HEADER<PECURRENT>*)&((PBYTE)me->DllBase)[DosHdr->OffsetHeaderPE];
       DATA_DIRECTORY *ImportDir = &WinHdr->OptionalHeader.DataDirectories.ImportTable;       // ImportAddressTable may not be inside it!
       if(ImportDir->DirectoryRVA)TResolveImportsForMod<PECURRENT>(DllName, (PBYTE)me->DllBase, (PBYTE)DllBase);  // Reimport api to a real DLL
       DBGMSG("Done fixing moved import");
      }
   DBGMSG("End LDR_DATA_TABLE_ENTRY_MO: %p",me);
  }
 if(EntryPT)*EntryPT = ModEP;
 if(NewBase)*NewBase = ModCopy;
 return (MapAddr == DllBase);
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