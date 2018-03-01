
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
 *(PBYTE)BaseAddr = 0;     // Marker, First thead that enters will revert this to 'M'  // Helps to avoid mult-entering when using APC injection
 return EncKey;
}
//------------------------------------------------------------------------------------
__declspec(noinline) static HMODULE ModFixInplaceSelf(PVOID BaseAddr, PVOID pNtDll, UINT ExcludeFlg=(fmCryImp|fmCryExp|fmCryRes), bool RecryptHdr=true)
{
 PBYTE ModuleBase = PBYTE((SIZE_T)BaseAddr & ~0xFFF);
 DOS_HEADER* DosHdr = (DOS_HEADER*)ModuleBase;
 UINT Flags  = (DosHdr->Reserved1 & ~ExcludeFlg)|fmFixSec|fmFixImp|fmFixRel|fmSelfMov;
 UINT RetFix = 0;
 if(Flags & mfX64Mod)TFixUpModuleInplace<PETYPE64>(ModuleBase,pNtDll,Flags,&RetFix);
  else TFixUpModuleInplace<PETYPE32>(ModuleBase,pNtDll,Flags,&RetFix);
 if(RecryptHdr)CryptSensitiveParts(ModuleBase, fmCryHdr|fmEncMode|(Flags & fmEncKeyMsk), false); 
 if(RetFix)*((PBYTE*)_AddressOfReturnAddress()) += RetFix;   // After SelfMove need to fix every return address in this module
 return (HMODULE)ModuleBase;
}
//------------------------------------------------------------------------------------
__declspec(noinline) static PVOID FindNtDllFromAddr(PVOID Addr)   // May crash if Addr is not inside a valid module   // NOTE: We may just pass kernel32/ntdll base from a caller process :)
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
}
//------------------------------------------------------------------------------------


};