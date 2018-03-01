
#pragma hdrstop

#include "HideModule.h"
#include "OldFormatPE.h"

//---------------------------------------------------------------------------
// TODO: Rework and Test
unsigned short xor_ror_hash(char* string)
{
	unsigned short hash = 0;
	while (*string) {
		hash ^= *(unsigned short*)string;
		hash >>= 1;
		string++;
	}
	return hash;
}
//---------------------------------------------------------------------------
// TODO: Rework and Test
/*unsigned short xor_ror_hashW(wchar_t* wstring)
{
	unsigned short hash = 0;
	char* string = (char*)malloc(wcslen(wstring)+1);
	__try {
		WideCharToMultiByte(CP_ACP, 0, wstring, -1, string, wcslen(wstring)+1, NULL, NULL);
		hash = xor_ror_hash(string);
	} __finally {
		free(string);
	}
	return hash;
}  */      
//---------------------------------------------------------------------------
// -- Unfinished !!!
// Debug Directory    // And all related
// Reloc Directory    // And all related
// Import Directory   // And all related
// Export Directory   // And all related
// Resource Directory // And all related 
//
BOOL _stdcall WipeOutModuleInfo(HMODULE Module)  // Module MUST be already removed from the Loader List or else will be AV
{
 DOS_HEADER     *DOSHeader;
 WIN_HEADER     *WINHeader;
 SECTION_HEADER *CurSection;
 DATA_DIRECTORY *TmpDir;

 if(!IsValidPEHeader(Module))return FALSE;
 DOSHeader = (DOS_HEADER*)Module;
 WINHeader = (WIN_HEADER*)&((BYTE*)Module)[DOSHeader->OffsetHeaderPE];

// Set mem protection
 DWORD OldProt;
 UINT  ModuleSize = GetMappedModuleSize(Module);
 VirtualProtect(Module,ModuleSize,PAGE_READWRITE,&OldProt);    // TODO: Do this by regions only!

 TmpDir = &WINHeader->OptionalHeader.DataDirectories.ExportTable;
 if(TmpDir->DirectoryRVA && TmpDir->DirectorySize)
  {
   EXPORT_DIR *Export = (EXPORT_DIR*)&((BYTE*)Module)[TmpDir->DirectoryRVA];
   UINT ExportCtr     = (TmpDir->DirectorySize / sizeof(EXPORT_DIR));
   for(UINT ectr=0;ectr < ExportCtr;ectr++,Export++)              // Is this correct processing?
    {
     PBYTE  Name         = &((BYTE*)Module)[Export->NameRVA];
     PWORD  OrdinalTable = (PWORD) &((BYTE*)Module)[Export->OrdinalTableRVA];  // How to clear? // Where counter?        
     PDWORD NamePtrTable = (PDWORD)&((BYTE*)Module)[Export->NamePointersRVA];            
     PDWORD AddressTable = (PDWORD)&((BYTE*)Module)[Export->AddressTableRVA];  // How to clear? // Where counter?        
     FillString((LPSTR)Name,ByteHashAddress(Name));
     for(UINT ctr=0;ctr<Export->NamePointersNumber;ctr++)
      {
       FillString((LPSTR)&((BYTE*)Module)[NamePtrTable[ctr]], ByteHashAddress(&((BYTE*)Module)[NamePtrTable[ctr]]));
       NamePtrTable[ctr] = 0;
      }
     memset(Export,~ByteHashAddress(Export),sizeof(EXPORT_DIR));
    }
   TmpDir->DirectoryRVA = TmpDir->DirectorySize = ByteHashAddress(TmpDir);
  }

 TmpDir = &WINHeader->OptionalHeader.DataDirectories.ImportTable;
 if(TmpDir->DirectoryRVA && TmpDir->DirectorySize)
  {
   IMPORT_DESC *Import = (IMPORT_DESC*)&((BYTE*)Module)[TmpDir->DirectoryRVA];
   UINT ImportCtr      = (TmpDir->DirectorySize / sizeof(IMPORT_DESC));
   for(UINT ictr=0;(ictr < ImportCtr);ictr++,Import++)        // Is this correct processing?
    {
     if(!Import->LookUpTabRVA)continue;
     PBYTE  Name        = &((BYTE*)Module)[Import->ModuleNameRVA];
     PDWORD ImpEntryPtr = (PDWORD)&((BYTE*)Module)[Import->LookUpTabRVA];  // _IMAGE_THUNK_DATA32    
     for(;*ImpEntryPtr;ImpEntryPtr++)
      {
       if(*ImpEntryPtr & 0x80000000)continue;      // Check this
       PIMAGE_IMPORT_BY_NAME impn = (PIMAGE_IMPORT_BY_NAME)&((BYTE*)Module)[*ImpEntryPtr];   
       FillString((LPSTR)&impn->Name,ByteHashAddress(impn));
       impn->Hint = 0;
      }
     FillString((LPSTR)Name,ByteHashAddress(Name));

     memset(Import,~ByteHashAddress(Import),sizeof(IMPORT_DESC));
    }
   TmpDir->DirectoryRVA = TmpDir->DirectorySize = ByteHashAddress(TmpDir);
  }

 TmpDir = &WINHeader->OptionalHeader.DataDirectories.FixUpTable;    // Relocations
 memset(&((BYTE*)Module)[TmpDir->DirectoryRVA],~ByteHashAddress(&((BYTE*)Module)[TmpDir->DirectoryRVA]),TmpDir->DirectorySize);
 TmpDir->DirectoryRVA = TmpDir->DirectorySize = ByteHashAddress(TmpDir);

 TmpDir = &WINHeader->OptionalHeader.DataDirectories.DebugTable;    // Debug info   // Remove also PDB path. But how to find it?
 memset(&((BYTE*)Module)[TmpDir->DirectoryRVA],~ByteHashAddress(&((BYTE*)Module)[TmpDir->DirectoryRVA]),TmpDir->DirectorySize);
 TmpDir->DirectoryRVA = TmpDir->DirectorySize = ByteHashAddress(TmpDir);

 TmpDir = &WINHeader->OptionalHeader.DataDirectories.ResourceTable; // Resources
 memset(&((BYTE*)Module)[TmpDir->DirectoryRVA],~ByteHashAddress(&((BYTE*)Module)[TmpDir->DirectoryRVA]),TmpDir->DirectorySize);
 TmpDir->DirectoryRVA = TmpDir->DirectorySize = ByteHashAddress(TmpDir);

 UINT HdrSize = (WINHeader->OptionalHeader.BaseOfCode > WINHeader->OptionalHeader.BaseOfData)?(WINHeader->OptionalHeader.BaseOfData):(WINHeader->OptionalHeader.BaseOfCode);
 memset(Module,~ByteHashAddress(Module),HdrSize);

 return TRUE;
}
//------------------------------------------------------------------------------------
#undef RtlMoveMemory
extern "C" WINBASEAPI void WINAPI RtlMoveMemory(PVOID Destination, const VOID* Source, SIZE_T Length);    // __declspec(dllimport) 
//------------------------------------------------------------------------------------
struct SModDescr
{
 HMODULE Module; 
 PVOID   ModCopy; 
 UINT    ModSize;
 void   (WINAPI *pRtlMoveMemory)(PVOID Destination, const VOID* Source, SIZE_T Length);
 BOOL   (WINAPI *pFreeLibrary)(HMODULE hLibModule);
 LPVOID (WINAPI *pVirtualAlloc)(LPVOID lpAddress,SIZE_T dwSize,DWORD flAllocationType,DWORD flProtect);
 BYTE    ReMapProcCode[1024];
};
void _stdcall CurrentModuleReMap(SModDescr* Descr)    // Returns to remapped 'DeMapModuleInplace' or fails
{
 Descr->pFreeLibrary(Descr->Module);
 Descr->pVirtualAlloc(Descr->Module,Descr->ModSize,MEM_RESERVE,PAGE_EXECUTE_READWRITE);
 Descr->pVirtualAlloc(Descr->Module,Descr->ModSize,MEM_COMMIT,PAGE_EXECUTE_READWRITE);
 Descr->pRtlMoveMemory(Descr->Module,Descr->ModCopy,Descr->ModSize);
}
//------------------------------------------------------------------------------------
// Unfinished!!!   (Free Kernel32/ntdll ?)  Import only from ntdll.dll and check if trying hide it (Need RELOC fixing)
//
BOOL _stdcall DeMapModuleInplace(HMODULE Module)  // Uses normal WinAPI // Removed from Loader`s list and file mapping
{
 HANDLE hActCtx;
 UINT ModuleSize  = GetMappedModuleSize(Module);
 if(!ModuleSize)return FALSE;
 PVOID DllCopy    = VirtualAlloc(NULL,ModuleSize+sizeof(SModDescr),MEM_COMMIT,PAGE_EXECUTE_READWRITE);
 if(!DllCopy)return FALSE;
 DisableThreadLibraryCalls(Module);
 memcpy(DllCopy,Module,ModuleSize);
 PBYTE ThisAddr   = (PBYTE)DeMapModuleInplace;  // Address of this function
 PebGetModuleRefCount(Module, 1);       // Cannot be replaced with 'FreeLibrary'! // (A pinned module?)
 if(((PBYTE)Module <= ThisAddr)&&(&((PBYTE)Module)[ModuleSize] > ThisAddr))
  {
   GetCurrentActCtx(&hActCtx);    // Not because of Frost(bns.exe)?        // Increase ref counter of a ActivationContext and prevent it from beeng corrupted(Does not set a pointer to NULL after freeing a memory) by FreeLibrary
   SModDescr* Descr = (SModDescr*)&((PBYTE)DllCopy)[ModuleSize];
   memcpy(&Descr->ReMapProcCode,CurrentModuleReMap,1000);
   Descr->Module  = Module;
   Descr->ModCopy = DllCopy;
   Descr->ModSize = ModuleSize;
   Descr->pRtlMoveMemory = RtlMoveMemory;
   Descr->pFreeLibrary   = FreeLibrary;
   Descr->pVirtualAlloc  = VirtualAlloc;
   ((void   (_stdcall *)(SModDescr*))&Descr->ReMapProcCode)(Descr);  
/*   __asm                // Instead of this a loose memory block can be allocated for a specific function   // Jumping in stack from one proc to another is stelthier?
    {
     push dword ptr [ModuleSize]
     push dword ptr [DllCopy]
     push dword ptr [Module]
     push [RetLabel]                 // << From RtlMoveMemory
     push PAGE_EXECUTE_READWRITE
     push MEM_COMMIT
     push dword ptr [ModuleSize]
     push dword ptr [Module]
     push dword ptr [RtlMoveMemory]  // << From second VirtualAlloc   // Real, not a #define  // How about ntdll.dll in Windows 7 x64?
     push PAGE_EXECUTE_READWRITE
     push MEM_RESERVE
     push dword ptr [ModuleSize]
     push dword ptr [Module]
     push dword ptr [VirtualAlloc]   // << From first VirtualAlloc  
     push dword ptr [Module]         // FreeLibrary(Module)
     push dword ptr [VirtualAlloc]   // << From FreeLibrary
     jmp  dword ptr [FreeLibrary]    // FreeLibrary/LdrUnloadDll alse decreases ref counter of the ActivationContext
RetLabel:
     nop
    } */ 
  }
   else   // Demapping not this module   // What about ActivationContext? 
    {       
     FreeLibrary(Module);
     VirtualAlloc(Module,ModuleSize,MEM_RESERVE,PAGE_EXECUTE_READWRITE);   // First RESERVE, then COMMIT when base address is specified
     if(VirtualAlloc(Module,ModuleSize,MEM_COMMIT,PAGE_EXECUTE_READWRITE))memcpy(Module,DllCopy,ModuleSize);    
    }
 VirtualFree(DllCopy,0,MEM_RELEASE);
 return TRUE;
}
//---------------------------------------------------------------------------




//---------------------------------------------------------------------------