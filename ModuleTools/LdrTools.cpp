//---------------------------------------------------------------------------


#pragma hdrstop

#include "LdrTools.h"
#include "OldFormatPE.h"

//---------------------------------------------------------------------------

//DWORD HOOKAddr;
//---------------------------------------------------------------------------
void _stdcall FillString(LPSTR Str, BYTE Value)
{
 if(Str){for(UINT ctr=0;Str[ctr];ctr++)Str[ctr] = Value;}
}
//---------------------------------------------------------------------------
BYTE _stdcall ByteHashAddress(PVOID Addr)
{
 BYTE Hash = 0;
 for(UINT ctr=0;ctr<sizeof(UINT);ctr++)Hash ^= (((UINT)Addr) >> (ctr*8)) & 0xFF;
 return Hash;
}
//---------------------------------------------------------------------------
// Use ModuleName or Module to specify a target module
HMODULE _stdcall MakeFakeModule(IN PWSTR ModuleName, IN HMODULE Module, IN HMODULE Exclude, OUT HMODULE *Original, BOOL WipeHdr)
{
 PPEB peb          = NtCurrentTeb()->Peb;
 PPEB_LDR_DATA ldr = peb->Ldr;

 for(PLDR_MODULE_ENTRY_MO me = peb->Ldr->InMemoryOrderModuleList.Next;me != (PLDR_MODULE_ENTRY_MO)&peb->Ldr->InMemoryOrderModuleList;me = me->InMemoryOrderLinks.Next)
  {
   if(Exclude == me->Module.DllBase)continue;
   if(ModuleName)
	{
	 if((ModuleName[1] == ':')&&(lstrcmpiW(ModuleName,me->Module.FullDllName.Buffer) != 0))continue;
	 if(lstrcmpiW(ModuleName,me->Module.BaseDllName.Buffer) != 0)continue;
	}
     else if(Module != me->Module.DllBase)continue;

   PVOID   OldBase = me->Module.DllBase;
   HMODULE FakeMod = DuplicateModule((HMODULE)me->Module.DllBase, me->Module.FullDllName.Buffer);
   if(FakeMod)
    {
     PBYTE eptr  = (PBYTE)me->Module.EntryPoint;
     UINT  epoff = eptr-(PBYTE)me->Module.DllBase;
     me->Module.DllBase    = FakeMod;
     me->Module.EntryPoint = &((PBYTE)FakeMod)[epoff];   // Keep original EntryPoint?
    //      HOOKAddr = (DWORD)&me->Module.DllBase;  // !!!!!!!!!!!!
     GetModuleHandleW(peb->Ldr->InMemoryOrderModuleList.Next->Module.BaseDllName.Buffer);  // Update a Cached Entry

     //
     // Reallocate original module
     //

     if(WipeHdr)DestroyModuleHeader((HMODULE)OldBase);   // Do it ONLY if you sure that other DLLs dont`t have already original HMODULE
    }  
   if(Original)*Original = (HMODULE)OldBase;
   return (HMODULE)FakeMod;
  }
 return NULL;   // Module not found
}
//---------------------------------------------------------------------------
PVOID _stdcall SetNewModuleBase(HMODULE CurrentBase, PVOID NewBase)
{
 PPEB peb          = NtCurrentTeb()->Peb;
 PPEB_LDR_DATA ldr = peb->Ldr;

 for(PLDR_MODULE_ENTRY_MO me = peb->Ldr->InMemoryOrderModuleList.Next;me != (PLDR_MODULE_ENTRY_MO)&peb->Ldr->InMemoryOrderModuleList;me = me->InMemoryOrderLinks.Next)
  {
   if(CurrentBase != me->Module.DllBase)continue;
   PVOID OldBase = me->Module.DllBase;
   me->Module.DllBase = NewBase;
   return OldBase;
  }
 return NULL;
}
//---------------------------------------------------------------------------
HMODULE _stdcall DuplicateModule(HMODULE Module, PWSTR ModulePath)
{
 WCHAR MPath[MAX_PATH];
 if(!ModulePath)
  {
   MPath[0]   = 0;
   ModulePath = (PWSTR)&MPath;
   GetModuleFileNameW(Module,ModulePath,MAX_PATH);      // From PEB!
  }
 HANDLE hModFile = CreateFileW(ModulePath,GENERIC_READ|GENERIC_EXECUTE,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
 if(hModFile == INVALID_HANDLE_VALUE)return NULL;
 HANDLE hModMap  = CreateFileMapping(hModFile,NULL,PAGE_READONLY|SEC_IMAGE,0,0,NULL); //|SEC_IMAGE |PAGE_WRITECOPY   PAGE_READWRITE
 CloseHandle(hModFile);
 if(!hModMap)return NULL;
 PVOID  ModBase  = MapViewOfFile(hModMap,FILE_MAP_COPY,0,0,0);
 CloseHandle(hModMap);
 if(!ModBase)return NULL;
 if(CopyModuleWithProtection((HMODULE)ModBase, Module))return (HMODULE)ModBase;
 UnmapViewOfFile(ModBase);
 return NULL;
}
//---------------------------------------------------------------------------
UINT _stdcall CopyModuleWithProtection(HMODULE DstModule, HMODULE SrcModule)
{
 MEMORY_BASIC_INFORMATION meminfo;

 DWORD ModuleSize = 0;
 PVOID ModuleAddr = SrcModule;
 PVOID NewModAddr = DstModule;
/* memset(&meminfo,0,sizeof(MEMORY_BASIC_INFORMATION));
 while(VirtualQuery((LPCVOID)ModuleAddr, &meminfo, sizeof(MEMORY_BASIC_INFORMATION)))
  {
   if(meminfo.AllocationBase != SrcModule)break;
   ModuleSize  += meminfo.RegionSize;
   DWORD OldProt;
   VirtualProtect(NewModAddr,meminfo.RegionSize,PAGE_READWRITE,&OldProt);
   switch(meminfo.Protect)  
    {
     case PAGE_EXECUTE:
     case PAGE_EXECUTE_READ:
     case PAGE_EXECUTE_READWRITE:
     case PAGE_EXECUTE_WRITECOPY:
       {               // Parse code and try to detect an installed hooks to fix ( GameOverlayRenderer.dll installs these hooks )    
        HDE::HDE32 HdeInfoDst;
        HDE::HDE32 HdeInfoSrc;
        PBYTE DstPtr = (PBYTE)NewModAddr;
        PBYTE SrcPtr = (PBYTE)ModuleAddr;
        UINT Offset  = 0;
        do
         {
          HdeInfoDst.disasm(&((BYTE*)DstPtr)[Offset]);
          HdeInfoSrc.disasm(&((BYTE*)SrcPtr)[Offset]);
          UINT CLen = (HdeInfoDst.len > HdeInfoSrc.len)?(HdeInfoDst.len):(HdeInfoSrc.len);
          if((Offset+CLen) > meminfo.RegionSize)CLen = (meminfo.RegionSize - Offset);
          if((HdeInfoSrc.opcode != HdeInfoDst.opcode) && (HdeInfoSrc.flags & HDE::F_RELATIVE) && (HdeInfoSrc.opcode == 0xE9))
           {
            PBYTE Addr = (((PBYTE)SrcPtr) + Offset + HdeInfoSrc.imm.imm32 + HdeInfoSrc.len);   // RelToAddr  // imm or disp - TEST IT!
            long  NRel = -((((PBYTE)DstPtr)+Offset+HdeInfoSrc.len) - Addr);      // AddrToRel
            ((PBYTE)DstPtr)[Offset] = 0xE9;
            ((PDWORD)&((BYTE*)DstPtr)[Offset+1])[0] = NRel; 
           }
            else memcpy(&((BYTE*)DstPtr)[Offset],&((BYTE*)SrcPtr)[Offset],CLen);    
         Offset += CLen;
        }
         while(Offset < meminfo.RegionSize);
       }
       break; 
     case PAGE_READONLY:
     case PAGE_READWRITE:
     case PAGE_WRITECOPY:
       memcpy(NewModAddr,ModuleAddr,meminfo.RegionSize);
       break;
    }
   VirtualProtect(NewModAddr,meminfo.RegionSize,meminfo.Protect,&OldProt);

   ModuleAddr = &((PBYTE)ModuleAddr)[meminfo.RegionSize];
   NewModAddr = &((PBYTE)NewModAddr)[meminfo.RegionSize];
  }	*/
 return ModuleSize;
}
//---------------------------------------------------------------------------
BOOL _stdcall DestroyModuleHeader(HMODULE Module)
{
 DWORD OldProt;
 VirtualProtect(Module,64,PAGE_READWRITE,&OldProt);
 memset(Module,0,64);
 VirtualProtect(Module,64,OldProt,&OldProt);
 return TRUE;
}
//---------------------------------------------------------------------------
// If ThreadState match with targrt thread`s state, no state change will occur (just increments counter)
UINT _stdcall SetThreadsState(DWORD ProcessID, bool ThreadState)
{
 HANDLE        hThreadsSnap;
 THREADENTRY32 tent32;

 DWORD CurThrdID = GetCurrentThreadId();
 hThreadsSnap    = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, NULL);
 if(hThreadsSnap == INVALID_HANDLE_VALUE)return 2;
 ZeroMemory(&tent32, sizeof(THREADENTRY32));
 tent32.dwSize   = sizeof(THREADENTRY32);
 if(Thread32First(hThreadsSnap, &tent32))
  {
   do
	{
	 if((tent32.th32OwnerProcessID == ProcessID)&&(tent32.th32ThreadID != CurThrdID))
	  {
	   if(HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME,false,tent32.th32ThreadID))
		{
		 if(ThreadState)ResumeThread(hThread);
		   else SuspendThread(hThread);
		 CloseHandle(hThread);
		}
	  }
	}
	 while(Thread32Next(hThreadsSnap, &tent32));
  }
 CloseHandle(hThreadsSnap);
 return 0;
}
//---------------------------------------------------------------------------
UINT _stdcall GetMappedModuleSize(HMODULE Module)
{
 MEMORY_BASIC_INFORMATION meminfo;
 UINT  ModuleSize = 0;
 PVOID ModuleAddr = Module;
 memset(&meminfo,0,sizeof(MEMORY_BASIC_INFORMATION));
 while(VirtualQuery((LPCVOID)ModuleAddr, &meminfo, sizeof(MEMORY_BASIC_INFORMATION)))
  {
   if(meminfo.AllocationBase != Module)break;
   ModuleSize += meminfo.RegionSize;
   ModuleAddr  = &((PBYTE)ModuleAddr)[meminfo.RegionSize];
  }
 return ModuleSize;
}
//---------------------------------------------------------------------------
// From File mapping, not from PEB
// Device//harddiskvolume//
UINT _stdcall GetModuleFilePath(HMODULE Module, PWSTR Path)
{
 static PVOID QueryVirtualMemory = NULL;
 ULONG  ResLen;
 BYTE   Buffer[528];   // Size taken from psapi.dll

 if(!QueryVirtualMemory)QueryVirtualMemory = GetProcAddress(GetModuleHandle("ntdll.dll"),"NtQueryVirtualMemory");  // Encrypt
 DWORD res = ((HRESULT (_stdcall *)(HANDLE,PVOID,UINT,PVOID,ULONG,PULONG))(QueryVirtualMemory))(GetCurrentProcess(),Module,2,&Buffer,sizeof(Buffer),&ResLen);   // MemorySectionName
 if(!ResLen)return 0;
 UNICODE_STR* ustr = ((UNICODE_STR*)&Buffer);
 // TODO:  Normalize path and Copy mem
 memcpy(Path, ustr->Buffer, ustr->MaximumLength);
 return ustr->Length;
}
//---------------------------------------------------------------------------
BOOL _stdcall SetModuleEntryPoint(HMODULE Module, UINT EntryRVA)
{
 if(!IsValidPEHeader(Module))return FALSE;
 PDWORD Epoff = GetEntryPointOffset(Module);

 DWORD OldProt;
 VirtualProtect(Epoff,8,PAGE_READWRITE,&OldProt);
 Epoff[0] = EntryRVA; 
 VirtualProtect(Epoff,8,OldProt,&OldProt);

 PPEB peb          = NtCurrentTeb()->Peb;
 PPEB_LDR_DATA ldr = peb->Ldr;
 for(PLDR_MODULE_ENTRY_MO me = peb->Ldr->InMemoryOrderModuleList.Next;me != (PLDR_MODULE_ENTRY_MO)&peb->Ldr->InMemoryOrderModuleList;me = me->InMemoryOrderLinks.Next)
  {
   if(Module != me->Module.DllBase)continue;
   me->Module.EntryPoint = &((PBYTE)Module)[EntryRVA];
   me->Module.Flags = (me->Module.Flags & ~LDRP_DONT_CALL_FOR_THREADS); // ??????
   return TRUE;
  }
 return FALSE;
}
//---------------------------------------------------------------------------
BOOL _stdcall RemoveModuleFromLdrList(HMODULE Module)   // The module still will be visible as file mapping
{
 PPEB peb          = NtCurrentTeb()->Peb;
 PPEB_LDR_DATA ldr = peb->Ldr;

 for(PLDR_MODULE_ENTRY_LO me = peb->Ldr->InLoadOrderModuleList.Next;me != (PLDR_MODULE_ENTRY_LO)&peb->Ldr->InLoadOrderModuleList;me = me->InLoadOrderLinks.Next)
  {
   if(Module == me->Module.DllBase)
    {
     me->InLoadOrderLinks.Prev->InLoadOrderLinks.Next = me->InLoadOrderLinks.Next;
     me->InLoadOrderLinks.Next->InLoadOrderLinks.Prev = me->InLoadOrderLinks.Prev;
     me->InMemoryOrderLinks.Prev->InMemoryOrderLinks.Next = me->InMemoryOrderLinks.Next;
     me->InMemoryOrderLinks.Next->InMemoryOrderLinks.Prev = me->InMemoryOrderLinks.Prev;
     me->InInitializationOrderLinks.Prev->InInitializationOrderLinks.Next = me->InInitializationOrderLinks.Next;
     me->InInitializationOrderLinks.Next->InInitializationOrderLinks.Prev = me->InInitializationOrderLinks.Prev;
     /// TODO: Remove from Hash table
     memset(me->Module.FullDllName.Buffer,0,me->Module.FullDllName.Length);
     memset(me,0,sizeof(LDR_MODULE_ENTRY_LO));
     return TRUE;
    }
  }
 return FALSE;
}
//---------------------------------------------------------------------------
int _stdcall PebGetModuleRefCount(HMODULE Module, int NewRef)
{
 PPEB peb          = NtCurrentTeb()->Peb;
 PPEB_LDR_DATA ldr = peb->Ldr;

 for(PLDR_MODULE_ENTRY_LO me = peb->Ldr->InLoadOrderModuleList.Next;me != (PLDR_MODULE_ENTRY_LO)&peb->Ldr->InLoadOrderModuleList;me = me->InLoadOrderLinks.Next)
  {
   if(Module == me->Module.DllBase)
    {
     int ref = me->Module.LoadCount;
     if(NewRef >= 0)me->Module.LoadCount = NewRef;
     return ref;
    }
  }
 return -1;
}
//---------------------------------------------------------------------------
