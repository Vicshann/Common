
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

//#ifndef FormatPEH
//#define FormatPEH

#include <intrin.h>
#include "ntdll.h"


#pragma warning(push)
#pragma warning(disable:4200)     // Overflow in a key transformation is expected
#pragma warning(disable:4244) 
#pragma warning(disable:4311)     // Type cast (WinAPI compatibility)
#pragma warning(disable:4312)     // Type cast (WinAPI compatibility)
#pragma warning(disable:4302)     // Type cast (WinAPI compatibility)
#pragma warning(disable:4244)     // Type cast (WinAPI compatibility)

#define PLATMEMALIGN    0x00001000
#define PLATMEMALIGNMSK ~(PLATMEMALIGN-1)     // TODO: Move to 'platform.hpp'
//==============================================================================
#define ALIGN_FORWARD(Value,Alignment) ((((Value)/(Alignment))+((bool)((Value)%(Alignment))))*(Alignment))
#define ALIGN_BACKWARD(Value,Alignment) (((Value)/(Alignment))*(Alignment))

#define RVATOADDR(SecRva,ModuleBase)  (((long)(ModuleBase))+((long)(SecRva)))
#define ADDRTORVA(SecAddr,ModuleBase) (((long)(SecAddr))-((long)(ModuleBase)))
//---------------------------------------------------------------------------
#define SIGN_MZ 0x5A4D
#define SIGN_PE 0x4550
//---------------------------------------------------------------------------
#define SECTION_TYPE_DSECT                 0x00000001  // Reserved.
#define SECTION_TYPE_NOLOAD                0x00000002  // Reserved.
#define SECTION_TYPE_GROUP                 0x00000004  // Reserved.
#define SECTION_TYPE_NO_PAD                0x00000008  // Reserved.
#define SECTION_TYPE_COPY                  0x00000010  // Reserved.
#define SECTION_CNT_CODE                   0x00000020  // Section contains code.
#define SECTION_CNT_INITIALIZED_DATA       0x00000040  // Section contains initialized data.
#define SECTION_CNT_UNINITIALIZED_DATA     0x00000080  // Section contains uninitialized data.
#define SECTION_LNK_OTHER                  0x00000100  // Reserved.
#define SECTION_LNK_INFO                   0x00000200  // Section contains comments or some other type of information.
#define SECTION_TYPE_OVER                  0x00000400  // Reserved.
#define SECTION_LNK_REMOVE                 0x00000800  // Section contents will not become part of image.
#define SECTION_LNK_COMDAT                 0x00001000  // Section contents comdat.
#define SECTION_UNKNOW                     0x00002000  // Reserved.
#define SECTION_NO_DEFER_SPEC_EXC          0x00004000  // Reset speculative exceptions handling bits in the TLB entries for this section.
#define SECTION_MEM_FARDATA                0x00008000  // Section content can be accessed relative to GP
#define SECTION_MEM_SYSHEAP                0x00010000  // Obsolete
#define SECTION_MEM_PURGEABLE              0x00020000  //
#define SECTION_MEM_LOCKED                 0x00040000  //
#define SECTION_MEM_PRELOAD                0x00080000  //
#define SECTION_ALIGN_1BYTES               0x00100000  //
#define SECTION_ALIGN_2BYTES               0x00200000  //
#define SECTION_ALIGN_4BYTES               0x00300000  //
#define SECTION_ALIGN_8BYTES               0x00400000  //
#define SECTION_ALIGN_16BYTES              0x00500000  // Default alignment if no others are specified.
#define SECTION_ALIGN_32BYTES              0x00600000  //
#define SECTION_ALIGN_64BYTES              0x00700000  //
#define SECTION_ALIGN_128BYTES             0x00800000  //
#define SECTION_ALIGN_256BYTES             0x00900000  //
#define SECTION_ALIGN_512BYTES             0x00A00000  //
#define SECTION_ALIGN_1024BYTES            0x00B00000  //
#define SECTION_ALIGN_2048BYTES            0x00C00000  //
#define SECTION_ALIGN_4096BYTES            0x00D00000  //
#define SECTION_ALIGN_8192BYTES            0x00E00000  //
#define SECTION_ALIGN_MASK                 0x00F00000  // UNUSED - Helps reading align value
#define SECTION_LNK_NRELOC_OVFL            0x01000000  // Section contains extended relocations.
#define SECTION_MEM_DISCARDABLE            0x02000000  // Section can be discarded.
#define SECTION_MEM_NOT_CACHED             0x04000000  // Section is not cachable.
#define SECTION_MEM_NOT_PAGED              0x08000000  // Section is not pageable.
#define SECTION_MEM_SHARED                 0x10000000  // Section is shareable.
#define SECTION_MEM_EXECUTE                0x20000000  // Section is executable.
#define SECTION_MEM_READ                   0x40000000  // Section is readable.
#define SECTION_MEM_WRITE                  0x80000000  // Section is writeable.


#define PETYPE64  ULONGLONG
#define PETYPE32  ULONG
#define PECURRENT ULONG_PTR

#pragma pack( push, 1 )
//---------------------------------------------------------------------------
struct DOS_HEADER
{
 WORD  FlagMZ;               // Magic number                            0x00
 WORD  LastPageSize;         // Bytes on last page of file              0x02
 WORD  PageCount;            // Pages in file (Page = 512 bytes)        0x04
 WORD  RelocCount;           // Elements count in Relocations table     0x06
 WORD  HeaderSize;           // Size of header in paragraphs            0x08
 WORD  MinMemory;            // Min. extra paragraphs needed            0x0A
 WORD  MaxMemory;            // Max. extra paragraphs needed            0x0C
 WORD  ValueSS;              // Initial SS value                        0x0E
 WORD  ValueSP;              // Initial SP value                        0x10
 WORD  CheckSum;             // Checksum of the file                    0x12
 WORD  ValueIP;              // Initial IP value                        0x14
 WORD  ValueCS;              // Initial CS value                        0x16
 WORD  RelocTableOffset;     // Address of relocation table             0x18
 WORD  OverlayNumber;        // Overlay number (0 for main module)      0x1A
 DWORD Compl20h;             // Double paragraph align                  0x1C
 DWORD Reserved1;            // Reserved words                          0x20
 WORD  OemID;                // OEM identifier                          0x24
 WORD  OemInfo;              // OEM information                         0x26
 BYTE  Reserved2[20];        // Reserved bytes                          0x28
 DWORD OffsetHeaderPE;       // File address of new exe header          0x3C
};
//---------------------------------------------------------------------------
struct DATA_DIRECTORY
{
 DWORD DirectoryRVA;     // RVA of the location of the directory.       0x00
 DWORD DirectorySize;    // Size of the directory.                      0x04
};
//---------------------------------------------------------------------------
struct DATA_DIRECTORIES_TABLE
{
 DATA_DIRECTORY ExportTable;      // Export Directory             0x78
 DATA_DIRECTORY ImportTable;      // Import Directory             0x80
 DATA_DIRECTORY ResourceTable;    // Resource Directory           0x88
 DATA_DIRECTORY ExceptionTable;   // Exception Directory          0x90
 DATA_DIRECTORY SecurityTable;    // Security Directory           0x98
 DATA_DIRECTORY FixUpTable;       // Base Relocation Table        0xA0
 DATA_DIRECTORY DebugTable;       // Debug Directory              0xA8
 DATA_DIRECTORY ImageDescription; // Description String           0xB0
 DATA_DIRECTORY MachineSpecific;  // Machine Value (MIPS GP)      0xB8
 DATA_DIRECTORY TlsDirectory;     // TLS Directory                0xC0
 DATA_DIRECTORY LoadConfigDir;    // Load Configuration Directory 0xC8
 DATA_DIRECTORY BoundImportDir;   // Bound(Delayed) Import Dir    0xD0
 DATA_DIRECTORY ImportAddrTable;  // Import Address Table         0xD8
 DATA_DIRECTORY Reserved1;        // Reserved (Must be NULL)      0xE0
 DATA_DIRECTORY Reserved2;        // Reserved (Must be NULL)      0xE8
 DATA_DIRECTORY Reserved3;        // Reserved (Must be NULL)      0xF0
};
//---------------------------------------------------------------------------
struct FILE_HEADER
{
 WORD  TypeCPU;         // Machine((Alpha/Motorola/.../0x014C = I386)   0x04                        
 WORD  SectionsNumber;  // Number of sections in the file               0x06
 DWORD TimeDateStamp;   // Number of seconds since Dec 31,1969,4:00PM   0x08
 DWORD TablePtrCOFF;    // Used in OBJ files and PE with debug info     0x0C
 DWORD TableSizeCOFF;   // The number of symbols in COFF table          0x10
 WORD  HeaderSizeNT;    // Size of the OptionalHeader structure         0x14
 WORD  Flags;           // 0000-Program; 0001-NoReloc; 0002-Can Exec;   0x16
};                      // 0200-Address fixed; 2000-This DLL
//---------------------------------------------------------------------------
template<typename T> struct OPTIONAL_HEADER
{
 WORD  Magic;           // 0107-ROM projection;010B-Normal projection   0x18
 BYTE  MajLinkerVer;    // Linker version number                        0x1A
 BYTE  MinLinkerVer;    // Linker version number                        0x1B
 DWORD CodeSize;        // Sum of sizes all code sections(ordinary one) 0x1C
 DWORD InitDataSize;    // Size of the initialized data                 0x20
 DWORD UnInitDataSize;  // Size of the uninitialized data section (BSS) 0x24
 DWORD EntryPointRVA;   // Address of 1st instruction to be executed    0x28
 DWORD BaseOfCode;      // Address (RVA) of beginning of code section   0x2C
 union
  {
   struct
    {
     DWORD BaseOfData;  // Address (RVA) of beginning of data section   0x30
     ULONG ImageBase;   // The *preferred* load address of the file     0x34
    };
   ULONGLONG ImageBase64;
  };
 DWORD SectionAlign;    // Alignment of sections when loaded into mem   0x38
 DWORD FileAlign;       // Align. of sections in file(mul of 512 bytes) 0x3C
 WORD  MajOperSysVer;   // Version number of required OS                0x40
 WORD  MinOperSysVer;   // Version number of required OS                0x42
 WORD  MajImageVer;     // Version number of image                      0x44
 WORD  MinImageVer;     // Version number of image                      0x46
 WORD  MajSubSysVer;    // Version number of subsystem                  0x48
 WORD  MinSubSysVer;    // Version number of subsystem                  0x4A
 DWORD Win32Version;    // Dunno! But I guess for future use.           0x4C
 DWORD SizeOfImage;     // Total size of the PE image in memory         0x50
 DWORD SizeOfHeaders;   // Size of all headers & section table          0x54
 DWORD FileCheckSum;    // Image file checksum                          0x58
 WORD  SubSystem;       // 1-NotNeeded;2-WinGUI;3-WinCON;5-OS2;7-Posix  0x5C
 WORD  FlagsDLL;        // Used to indicate if a DLL image includes EPs 0x5E
 T     StackReserveSize;// Size of stack to reserve                     0x60
 T     StackCommitSize; // Size of stack to commit                      0x64 / 0x68
 T     HeapReserveSize; // Size of local heap space to reserve          0x68 / 0x70
 T     HeapCommitSize;  // Size of local heap space to commit           0x6C / 0x78
 DWORD LoaderFlags;     // Choose Break/Debug/RunNormally(def) on load  0x70 / 0x80
 DWORD NumOfSizesAndRVA;// Length of next DataDirectory array(alw10h)   0x74 / 0x84
 DATA_DIRECTORIES_TABLE DataDirectories; //                             0x78 / 0x88
};
//---------------------------------------------------------------------------
template<typename T> struct WIN_HEADER                // Must be QWORD aligned
{
 DWORD                 FlagPE;         // PE File Signature             0x00
 FILE_HEADER           FileHeader;     // File header                   0x04
 OPTIONAL_HEADER<T>    OptionalHeader; // Optional file header          0x18
};
//---------------------------------------------------------------------------
struct EXPORT_DIR
{
 DWORD Characteristics;     // Reserved MUST BE NULL                  0x00
 DWORD TimeDateStamp;       // Date of Creation                       0x04
 DWORD Version;             // Export Version - Not Used              0x08
 DWORD NameRVA;             // RVA of Module Name                     0x0C
 DWORD OrdinalBase;         // Base Number of Functions               0x10
 DWORD FunctionsNumber;     // Number of all exported functions       0x14
 DWORD NamePointersNumber;  // Number of functions names              0x18
 DWORD AddressTableRVA;     // RVA of Functions Address Table         0x1C
 DWORD NamePointersRVA;     // RVA of Functions Name Pointers Table   0x20
 DWORD OrdinalTableRVA;     // RVA of Functions Ordinals Table        0x24
};
//---------------------------------------------------------------------------
struct IMPORT_DESC
{
 DWORD LookUpTabRVA;     // 0x00
 DWORD TimeDateStamp;    // 0x04
 DWORD ForwarderChain;   // 0x08
 DWORD ModuleNameRVA;    // 0x0C
 DWORD AddressTabRVA;    // 0x10
};
//---------------------------------------------------------------------------
struct SECTION_HEADER
{
 char  SectionName[8];        // 00
 DWORD VirtualSize;           // 08
 DWORD SectionRva;            // 0C
 DWORD PhysicalSize;          // 10
 DWORD PhysicalOffset;        // 14
 DWORD PtrToRelocations;      // 18
 DWORD PtrToLineNumbers;      // 1C
 WORD  NumOfRelocations;      // 20
 WORD  NumOfLineNumbers;      // 22
 DWORD Characteristics;       // 24
};
//---------------------------------------------------------------------------
struct DEBUG_DIR
{
 DWORD   Characteristics;
 DWORD   TimeDateStamp;
 WORD    MajorVersion;
 WORD    MinorVersion;
 DWORD   Type;
 DWORD   SizeOfData;
 DWORD   AddressOfRawData;
 DWORD   PointerToRawData;
};
//---------------------------------------------------------------------------
/*struct RELOCATION_DESC    // IMAGE_LINENUMBER ???????????????????????????????????????
{
 union   
  {
   DWORD VirtualAddress;
   DWORD RelocCount;             // Set to the real count when IMAGE_SCN_LNK_NRELOC_OVFL is set
  };
 DWORD SymbolTableIndex;
 WORD  Type;
}; */
//---------------------------------------------------------------------------
struct RELOCATION_DESC     // Max for 4k page // There may be more than one such block on Relocaton Directory
{
 DWORD BaseRVA;
 DWORD BlkSize;
 struct
  {
   WORD Offset : 12;
   WORD Type   : 4;
  }Records[0];

 UINT Count(void){return (this->BlkSize - sizeof(RELOCATION_DESC)) / sizeof(WORD);}
};
//---------------------------------------------------------------------------
struct RESOURCE_DIR_ENTRY       //   _IMAGE_RESOURCE_DIRECTORY_ENTRY
{
 union 
  {
   struct 
    {
     DWORD NameOffset   : 31;
     DWORD NameIsString : 1;
    };
   DWORD Name;
   WORD  Id;
  };
 union 
  {
   DWORD OffsetToData;
   struct 
    {
     DWORD OffsetToDirectory : 31;
     DWORD DataIsDirectory   : 1;
    };
  };
};
//---------------------------------------------------------------------------
struct RESOURCE_DIR
{
 DWORD   Characteristics;
 DWORD   TimeDateStamp;
 WORD    MajorVersion;
 WORD    MinorVersion;
 WORD    NumberOfNamedEntries;
 WORD    NumberOfIdEntries;
 IMAGE_RESOURCE_DIRECTORY_ENTRY DirectoryEntries[];    // Difined in WINNT.H
};
//---------------------------------------------------------------------------
struct VERSION_INFO   // The only version info?  // VS_VERSION_INFO
{
 WORD   Length;
 WORD   ValueLength;
 WORD   Type;
 WCHAR  Key[0];          // Zero terminated
// WORD Padding1[];        // To make Value aligned to DWORD
// VS_FIXEDFILEINFO Value;
// WORD Padding2[];        // To make Children aligned to DWORD
// WORD Children[];
};
//---------------------------------------------------------------------------
struct SImportByName
{
 WORD    Hint;
 BYTE    Name[1];
}; 
template<typename T> struct SImportThunk 
{
 T Value;	   // ForwarderString; Function; Ordinal; AddressOfData;
};
//---------------------------------------------------------------------------
struct SRichRec 
{
 WORD  Ver;    // MinVer
 WORD  PId;    // Product Identifier
 DWORD Cntr;   // Counter of what?
};
#pragma pack( pop )
//---------------------------------------------------------------------------
static bool _stdcall IsValidPEHeader(PVOID Header)
{
 DOS_HEADER* DosHdr  = (DOS_HEADER*)Header;
 if((DosHdr->FlagMZ != SIGN_MZ))return false;
 WIN_HEADER<PECURRENT>* WinHdr = (WIN_HEADER<PECURRENT>*)&(((BYTE*)Header)[DosHdr->OffsetHeaderPE]);
 if((WinHdr->FlagPE != SIGN_PE))return false;
 return true;
}
//---------------------------------------------------------------------------
static bool _stdcall IsValidPEHeaderBlk(PVOID Header, UINT Size)
{
 if(Size < sizeof(DOS_HEADER))return false;
 DOS_HEADER* DosHdr  = (DOS_HEADER*)Header;
 if((DosHdr->FlagMZ != SIGN_MZ))return false;
 if(Size < (DosHdr->OffsetHeaderPE + sizeof(WIN_HEADER<PECURRENT>)))return false;
 WIN_HEADER<PECURRENT>* WinHdr = (WIN_HEADER<PECURRENT>*)&(((BYTE*)Header)[DosHdr->OffsetHeaderPE]);
 if((WinHdr->FlagPE != SIGN_PE))return false;
 return true;
}
//---------------------------------------------------------------------------
static bool _stdcall IsValidExeFile(PVOID Header)        // TODO: Use IsBadReadPtr or specify a memory range as argument to test WinHdr?   // Leave IsBadReadPtr for now because in a mapped images not all segments readable
{
 if(!IsValidPEHeader(Header))return false;
 DOS_HEADER *DosHdr = (DOS_HEADER*)Header;
 WIN_HEADER<PECURRENT> *WinHdr = (WIN_HEADER<PECURRENT>*)&(((BYTE*)Header)[DosHdr->OffsetHeaderPE]);
 if(IsBadReadPtr(WinHdr,sizeof(*WinHdr)))return false;
 return !(WinHdr->FileHeader.Flags & 0x2000); // IsDll flag
}
//---------------------------------------------------------------------------
static bool _stdcall IsValidModuleX64(PVOID Header)
{
 DOS_HEADER *DosHdr = (DOS_HEADER*)Header;
 WIN_HEADER<PECURRENT> *WinHdr = (WIN_HEADER<PECURRENT>*)&(((BYTE*)Header)[DosHdr->OffsetHeaderPE]);
 return (WinHdr->OptionalHeader.Magic == 0x020B);
}
//---------------------------------------------------------------------------
static bool _stdcall IsRvaInSection(SECTION_HEADER *Sec, UINT Rva)
{
 if(Sec->SectionRva > Rva)return false;
 if((Sec->SectionRva+Sec->VirtualSize) <= Rva)return false;
 return true;
}
//---------------------------------------------------------------------------
static UINT _stdcall GetImageSize(PVOID Header)
{
 DOS_HEADER *DosHdr = (DOS_HEADER*)Header;
 WIN_HEADER<PECURRENT> *WinHdr = (WIN_HEADER<PECURRENT>*)&(((BYTE*)Header)[DosHdr->OffsetHeaderPE]);
 return WinHdr->OptionalHeader.SizeOfImage;
}
//---------------------------------------------------------------------------
template<typename T> static UINT _stdcall TRvaToFileOffset(PBYTE ModuleBase, UINT Rva)
{
 DOS_HEADER     *DosHdr = (DOS_HEADER*)ModuleBase;
 WIN_HEADER<T>  *WinHdr = (WIN_HEADER<T>*)&ModuleBase[DosHdr->OffsetHeaderPE];
 UINT            HdrLen = DosHdr->OffsetHeaderPE+WinHdr->FileHeader.HeaderSizeNT+sizeof(FILE_HEADER)+sizeof(DWORD);
 SECTION_HEADER *CurSec = (SECTION_HEADER*)&ModuleBase[HdrLen];
 for(int ctr = 0;ctr < WinHdr->FileHeader.SectionsNumber;ctr++,CurSec++)
  {
   if(!IsRvaInSection(CurSec, Rva))continue;
   Rva -= CurSec->SectionRva;
   if(Rva >= CurSec->PhysicalSize)return 0;  // Not present in the file as physical
   return (CurSec->PhysicalOffset + Rva);
  }
 return 0;
}
//---------------------------------------------------------------------------
// Works with a Mapped in memory or a Raw file
static UINT _stdcall RvaToFileOffsConvert(PVOID ModuleBase, UINT Rva)
{           
 if(!IsValidPEHeader(ModuleBase))return 0;
 if(IsValidModuleX64(ModuleBase))return TRvaToFileOffset<PETYPE64>((PBYTE)ModuleBase, Rva);
 return TRvaToFileOffset<PETYPE32>((PBYTE)ModuleBase, Rva);
}
//---------------------------------------------------------------------------
template<typename T> static UINT _stdcall TFileOffsetToRva(PBYTE ModuleBase, UINT Offset)
{
 DOS_HEADER     *DosHdr = (DOS_HEADER*)ModuleBase;
 WIN_HEADER<T>  *WinHdr = (WIN_HEADER<T>*)&ModuleBase[DosHdr->OffsetHeaderPE];
 UINT            HdrLen = DosHdr->OffsetHeaderPE+WinHdr->FileHeader.HeaderSizeNT+sizeof(FILE_HEADER)+sizeof(DWORD);
 SECTION_HEADER *CurSec = (SECTION_HEADER*)&ModuleBase[HdrLen];
 for(int ctr = 0;ctr < WinHdr->FileHeader.SectionsNumber;ctr++,CurSec++)
  {
   if((Offset >= CurSec->PhysicalOffset)&&(Offset < (CurSec->PhysicalOffset+CurSec->PhysicalSize)))
	{
	 return ((Offset-CurSec->PhysicalOffset)+CurSec->SectionRva);
	}
  }
 return 0;
}
//---------------------------------------------------------------------------
static UINT _stdcall FileOffsetToRvaConvert(PVOID ModuleBase, UINT Offset)
{           
 if(!IsValidPEHeader(ModuleBase))return 0;
 if(IsValidModuleX64(ModuleBase))return TFileOffsetToRva<PETYPE64>((PBYTE)ModuleBase, Offset);
 return TFileOffsetToRva<PETYPE32>((PBYTE)ModuleBase, Offset);
}
//---------------------------------------------------------------------------


static PWSTR _stdcall FindVerValue(VERSION_INFO* RootBlk, PWSTR Name, UINT Type)
{
 //if(RootBlk->ValueLength && ((Type == (UINT)-1)||(Type == RootBlk->Type)) && (lstrcmpW((PWSTR)&RootBlk->Key,Name)==0)return (PWSTR)&RootBlk->Key;


}
//---------------------------------------------------------------------------

// TODO: Make it correct for search by a name
// Name is more a Lng here
/*
int _pe_iterate_resources(
    PE* pe,
    PIMAGE_RESOURCE_DIRECTORY resource_dir,
    uint8_t* rsrc_data,
    int rsrc_tree_level,
    int* type,
    int* id,
    int* language,
    uint8_t* type_string,
    uint8_t* name_string,
    uint8_t* lang_string,
    RESOURCE_CALLBACK_FUNC callback,
    void* callback_data)
{
*/
template<typename T> PVOID _stdcall TGetResBodyRaw(PBYTE ModuleBase, PBYTE ResBase, RESOURCE_DIR* Resour, UINT RootID, PUINT ResIdx, PUINT Size, LPSTR* Name, IMAGE_RESOURCE_DATA_ENTRY** DEntr)
{
 for(UINT ctr=0;ctr < Resour->NumberOfIdEntries;ctr++)
  {
   IMAGE_RESOURCE_DIRECTORY_ENTRY* entr = &Resour->DirectoryEntries[ctr];
   if((RootID != (UINT)-1)&&((PBYTE)Resour == ResBase))
    {
     if(entr->Name & 0x80000000)continue; // Name instead of ID encountered in the Root
     if(RootID != (UINT)entr->Name)continue;
    }
   if(entr->OffsetToData & 0x80000000)
    {
     RESOURCE_DIR* RDir = (RESOURCE_DIR*)&ResBase[entr->OffsetToData & ~0x80000000];
     PVOID res = TGetResBodyRaw<T>(ModuleBase, ResBase, RDir, RootID, ResIdx, Size, Name, DEntr);
     if(res)return res;  // Found resource
    }
     else
      {
       if(ResIdx && *ResIdx){(*ResIdx)--; continue;}
       LPSTR LNam = NULL;
       if(entr->Name & 0x80000000)    // Here it is a LangID only?
        {
         IMAGE_RESOURCE_DIRECTORY_STRING* str = (IMAGE_RESOURCE_DIRECTORY_STRING*)&ResBase[entr->Name & ~0x80000000];
         LNam = (LPSTR)&str->NameString;  // -2(WORD) is size of the string
        }
         else LNam = (LPSTR)entr->Name;   // Type      // x64 ????????????????????????????
       if(*Name)
        {
         if(entr->Name & 0x80000000){if(lstrcmp(LNam,*Name) != 0)continue;}
           else if(LNam != *Name)continue;
        }
         else *Name = LNam;
       IMAGE_RESOURCE_DATA_ENTRY* Dentry = (IMAGE_RESOURCE_DATA_ENTRY*)&ResBase[entr->OffsetToData];
       *Size = Dentry->Size;
       if(DEntr)*DEntr = Dentry;
       return &ModuleBase[TRvaToFileOffset<T>(ModuleBase,Dentry->OffsetToData)];
      }
  }
 return NULL; 
}
//---------------------------------------------------------------------------
template<typename T> static PVOID _stdcall TGetResourceBodyRaw(PBYTE ModuleBase, UINT RootID, UINT ResIdx, PUINT Size, LPSTR* Name, IMAGE_RESOURCE_DATA_ENTRY** DEntr)
{
 DOS_HEADER     *DosHdr = (DOS_HEADER*)ModuleBase;
 WIN_HEADER<T>  *WinHdr = (WIN_HEADER<T>*)&((PBYTE)ModuleBase)[DosHdr->OffsetHeaderPE];
 UINT            HdrLen = DosHdr->OffsetHeaderPE+WinHdr->FileHeader.HeaderSizeNT+sizeof(FILE_HEADER)+sizeof(DWORD);
 DATA_DIRECTORY *ResDir = &WinHdr->OptionalHeader.DataDirectories.ResourceTable;
 RESOURCE_DIR   *Resour = (RESOURCE_DIR*)&ModuleBase[TRvaToFileOffset<T>(ModuleBase,ResDir->DirectoryRVA)];
 return TGetResBodyRaw<T>(ModuleBase, (PBYTE)Resour, Resour, RootID, &ResIdx, Size, Name, DEntr);
}
//---------------------------------------------------------------------------
static PVOID _stdcall GetResourceBodyRaw(PVOID ModuleBase, UINT RootID, UINT ResIdx, PUINT Size, LPSTR* Name, IMAGE_RESOURCE_DATA_ENTRY** DEntr=NULL)
{
 if(!IsValidPEHeader(ModuleBase))return NULL;
 if(IsValidModuleX64(ModuleBase))return TGetResourceBodyRaw<PETYPE64>((PBYTE)ModuleBase, RootID, ResIdx, Size, Name, DEntr);
 return TGetResourceBodyRaw<PETYPE32>((PBYTE)ModuleBase, RootID, ResIdx, Size, Name, DEntr);
}
//---------------------------------------------------------------------------

/*template<typename T> static UINT _stdcall GetEntryPointersForApiName(PSTRING ModuleName, PSTRING ApiName, PVOID ModuleBase, T* *EntryA, T* *EntryB)
{
 if(!IsValidPEHeader(ModuleBase))return 1;
 DBGMSG("ModuleName='%Z', ApiName='%Z', ModuleBase=%p",ModuleName,ApiName,ModuleBase);
 DOS_HEADER     *DosHdr    = (DOS_HEADER*)ModuleBase;
 WIN_HEADER<T>  *WinHdr    = (WIN_HEADER<T>*)&((PBYTE)ModuleBase)[DosHdr->OffsetHeaderPE];
 DATA_DIRECTORY *ImportDir = &WinHdr->OptionalHeader.DataDirectories.ImportTable;
                         
 if(!ImportDir->DirectoryRVA)return 1;
 IMPORT_DESC    *Import    = (IMPORT_DESC*)&((PBYTE)ModuleBase)[RvaToFileOffset(ModuleBase,ImportDir->DirectoryRVA,NULL)];
 for(UINT tctr=0;Import[tctr].AddressTabRVA;tctr++)
  {
   STRING CurModlName;
   T* LTable  = (Import[tctr].LookUpTabRVA )?((T*)&((PBYTE)ModuleBase)[RvaToFileOffset(ModuleBase,Import[tctr].LookUpTabRVA, NULL)]):(NULL);
   T* ATable  = (Import[tctr].AddressTabRVA)?((T*)&((PBYTE)ModuleBase)[RvaToFileOffset(ModuleBase,Import[tctr].AddressTabRVA,NULL)]):(NULL); // Will be converted to API addresses by loader
   T* Table   = (LTable)?(LTable):(ATable);
   CurModlName.Buffer        = (LPSTR)&((PBYTE)ModuleBase)[RvaToFileOffset(ModuleBase,Import[tctr].ModuleNameRVA,NULL)];
   CurModlName.Length        = strlen(CurModlName.Buffer);
   CurModlName.MaximumLength = CurModlName.Length+sizeof(CHAR);
   if(!RtlEqualString(&CurModlName,ModuleName,TRUE))continue;
   DBGMSG("Module <%s>, LTable=%p, ATable=%p",CurModlName.Buffer,LTable,ATable);
   for(UINT actr=0;Table[actr];actr++)
	{
     STRING CurProcName;
	 if((Table[actr] & 0xFFFF0000)==0x80000000)continue;  // Skip imports by ordinal
     CurProcName.Buffer        = (Table[actr] > 103809024)?((LPSTR)(Table[actr]+2)):((LPSTR)&((PBYTE)ModuleBase)[RvaToFileOffset(ModuleBase,Table[actr]+2,NULL)]);  // May be already address, not offset  // 103809024 = Max offset
     CurProcName.Length        = strlen(CurProcName.Buffer);
     CurProcName.MaximumLength = CurProcName.Length+sizeof(CHAR);
     //DBGMSG("Proc <%s>",CurProcName.Buffer);
     if(RtlEqualString(&CurProcName,ApiName,FALSE))
      {
       if(EntryA)*EntryA = (LTable)?(&LTable[actr]):(NULL); 
       if(EntryB)*EntryB = (ATable)?(&ATable[actr]):(NULL); 
       DBGMSG("Found Import: <%s> %s",CurModlName.Buffer,CurProcName.Buffer);
       return 0;
      }
	}
  }   
 return 1;
}*/
//---------------------------------------------------------------------------
template<typename T> static LPSTR _stdcall TGetExpModuleName(PBYTE ModuleBase, bool Raw)  // Not for EXE     // TODO: Optional Pointer validation mode
{
 DOS_HEADER* DosHdr        = (DOS_HEADER*)ModuleBase;
 WIN_HEADER<T>* WinHdr     = (WIN_HEADER<T>*)&((PBYTE)ModuleBase)[DosHdr->OffsetHeaderPE];
 DATA_DIRECTORY* ExportDir = &WinHdr->OptionalHeader.DataDirectories.ExportTable;
 if(!ExportDir->DirectoryRVA)return NULL;
 EXPORT_DIR* Export        = (EXPORT_DIR*)&((PBYTE)ModuleBase)[ExportDir->DirectoryRVA];
 return (LPSTR)&ModuleBase[(Raw)?(TRvaToFileOffset<T>(ModuleBase,Export->NameRVA)):(Export->NameRVA)];
}
//---------------------------------------------------------------------------
static LPSTR _stdcall GetExpModuleName(PVOID ModuleBase, bool Raw)
{
 if(IsValidModuleX64(ModuleBase))return TGetExpModuleName<PETYPE64>((PBYTE)ModuleBase, Raw);
 return TGetExpModuleName<PETYPE32>((PBYTE)ModuleBase, Raw);
}
//---------------------------------------------------------------------------
static PVOID _stdcall ModuleAddressToBase(PVOID Address)  // Not all module sections may present in memory (discarded)
{
 PBYTE Base = (PBYTE)(((ULONG_PTR)Address) & ~0xFFF);   
 while(IsBadReadPtr(Base,sizeof(DOS_HEADER)) || !IsValidPEHeader(Base))Base -= 0x1000;  // !MmIsAddressValid(Base) // Why not all pages of ntoskrnl.exe are available on x64?
 LOGMSG("Found module base: %p", Base);
 return Base;
}
//---------------------------------------------------------------------------
/*static PVOID _stdcall GetSystemRoutineAddress(CHAR *ProcName) 
{
 UNICODE_STRING ustr; 
 WCHAR Name[256];

 ustr.Buffer = (PWSTR)&Name;
 ustr.Length = ustr.MaximumLength = 0;
 for(int ctr=0;ProcName[ctr] && (ctr < ((sizeof(Name)/2)-1));ctr++){Name[ctr] = ProcName[ctr];ustr.Length = ++ustr.MaximumLength;}
 ustr.Buffer[ustr.Length] = 0;
 ustr.Length = ustr.MaximumLength = ustr.Length * 2;
 return MmGetSystemRoutineAddress(&ustr);   
} */
//---------------------------------------------------------------------------
static CHAR _stdcall CharCaseUpper(CHAR Chr)
{
 if((Chr > 0x60)&&(Chr < 0x7B))Chr -= 0x20;
 return Chr;  
}
//------------------------------------------------------------------------------
static BOOL _stdcall IsCharsEqualIC(CHAR ChrA, CHAR ChrB)
{
 return (CharCaseUpper(ChrA) == CharCaseUpper(ChrB));
}
//------------------------------------------------------------------------------
static BOOL _stdcall IsSectionNamesEqual(CHAR *NameA, CHAR *NameB)
{
 for(int ctr=0;ctr<8;ctr++)
  {
   if(!IsCharsEqualIC(NameA[ctr], NameB[ctr]))return FALSE;
   if(!NameA[ctr])break; // Both are ZERO
  }
 return TRUE;
}
//------------------------------------------------------------------------------
static BOOL _stdcall IsNamesEqual(CHAR *NameA, CHAR *NameB)
{
 for(;;NameA++,NameB++)
  {
   if(*NameA != *NameB)return FALSE;
   if(!*NameA)break;
  }
 return TRUE;
}
//------------------------------------------------------------------------------
static BOOL _stdcall IsNamesEqualIC(CHAR *NameA, CHAR *NameB)
{
 for(;;NameA++,NameB++)
  {
   if(!IsCharsEqualIC(*NameA, *NameB))return FALSE;
   if(!*NameA)break;
  }
 return TRUE;
}
//------------------------------------------------------------------------------
template<typename T> static bool _stdcall TGetModuleSection(PVOID ModuleBase, CHAR *SecName, SECTION_HEADER **ResSec)
{
 DOS_HEADER     *DosHdr = (DOS_HEADER*)ModuleBase;
 WIN_HEADER<T>  *WinHdr = (WIN_HEADER<T>*)&((PBYTE)ModuleBase)[DosHdr->OffsetHeaderPE];
 UINT            HdrLen = DosHdr->OffsetHeaderPE+WinHdr->FileHeader.HeaderSizeNT+sizeof(FILE_HEADER)+sizeof(DWORD);
 SECTION_HEADER *CurSec = (SECTION_HEADER*)&((BYTE*)ModuleBase)[HdrLen];

 for(UINT ctr = 0;ctr < WinHdr->FileHeader.SectionsNumber;ctr++,CurSec++)
  {
   if(((SIZE_T)SecName == ctr) || IsSectionNamesEqual((CHAR*)&CurSec->SectionName, SecName))
    {
     if(ResSec)*ResSec = CurSec;  // NULL for only a presense test
     return true;
    }
  }
 return false;
}
//---------------------------------------------------------------------------
template<typename T> static UINT _stdcall TCalcRawModuleSize(PVOID ModuleBase)
{
 DOS_HEADER     *DosHdr = (DOS_HEADER*)ModuleBase;
 WIN_HEADER<T>  *WinHdr = (WIN_HEADER<T>*)&((PBYTE)ModuleBase)[DosHdr->OffsetHeaderPE];
 UINT            HdrLen = DosHdr->OffsetHeaderPE+WinHdr->FileHeader.HeaderSizeNT+sizeof(FILE_HEADER)+sizeof(DWORD);
 SECTION_HEADER *CurSec = (SECTION_HEADER*)&((BYTE*)ModuleBase)[HdrLen];
 if(!WinHdr->FileHeader.SectionsNumber)return WinHdr->OptionalHeader.SizeOfHeaders + WinHdr->OptionalHeader.CodeSize + WinHdr->OptionalHeader.InitDataSize + WinHdr->OptionalHeader.UnInitDataSize;
 SECTION_HEADER *LstSec = CurSec;
 for(UINT ctr = 0;ctr < WinHdr->FileHeader.SectionsNumber;ctr++,CurSec++)
  {
   if(CurSec->PhysicalOffset > LstSec->PhysicalOffset)LstSec = CurSec; 
  }
 return LstSec->PhysicalOffset + LstSec->PhysicalSize;
}
//---------------------------------------------------------------------------
static bool _stdcall GetModuleSizes(PBYTE ModuleBase, UINT* RawSize, UINT* VirSize)
{
 DOS_HEADER     *DosHdr = (DOS_HEADER*)ModuleBase;
 WIN_HEADER<PECURRENT> *WinHdr = (WIN_HEADER<PECURRENT>*)&((PBYTE)ModuleBase)[DosHdr->OffsetHeaderPE];
 if(VirSize)*VirSize = WinHdr->OptionalHeader.SizeOfImage;     
 if(RawSize)   
  {
   if(IsValidModuleX64(ModuleBase))*RawSize = TCalcRawModuleSize<PETYPE64>(ModuleBase);
     else *RawSize = TCalcRawModuleSize<PETYPE32>(ModuleBase);
  }
 return true;
}
//---------------------------------------------------------------------------
// Works with a Mapped in memory or a Raw file
static bool _stdcall GetModuleSection(PVOID ModuleBase, CHAR *SecName, SECTION_HEADER **ResSec)
{
 if(!IsValidPEHeader(ModuleBase))return false;
 if(IsValidModuleX64(ModuleBase))return TGetModuleSection<PETYPE64>(ModuleBase, SecName, ResSec);
 return TGetModuleSection<PETYPE32>(ModuleBase, SecName, ResSec);
}
//---------------------------------------------------------------------------
static PVOID _stdcall GetLoadedModuleEntryPoint(PVOID ModuleBase)
{
 if(!IsValidPEHeader(ModuleBase))return NULL;
 DOS_HEADER *DosHdr = (DOS_HEADER*)ModuleBase;
 WIN_HEADER<PECURRENT> *WinHdr = (WIN_HEADER<PECURRENT>*)&(((BYTE*)ModuleBase)[DosHdr->OffsetHeaderPE]);
 if(!WinHdr->OptionalHeader.EntryPointRVA)return NULL;				  
 return &((PBYTE)ModuleBase)[WinHdr->OptionalHeader.EntryPointRVA];
}
//---------------------------------------------------------------------------
template<typename T> static UINT _stdcall TGetModuleEntryOffset(PBYTE ModuleBase, bool Raw)
{
 DOS_HEADER     *DosHdr    = (DOS_HEADER*)ModuleBase;
 WIN_HEADER<T>  *WinHdr    = (WIN_HEADER<T>*)&ModuleBase[DosHdr->OffsetHeaderPE];
 if(!WinHdr->OptionalHeader.EntryPointRVA)return 0;	 
 return (Raw)?(TRvaToFileOffset<T>(ModuleBase,WinHdr->OptionalHeader.EntryPointRVA)):(WinHdr->OptionalHeader.EntryPointRVA);
}
//---------------------------------------------------------------------------
static UINT _stdcall GetModuleEntryOffset(PBYTE ModuleBase, bool Raw)
{
 if(IsValidModuleX64(ModuleBase))return TGetModuleEntryOffset<PETYPE64>(ModuleBase, Raw);
 return TGetModuleEntryOffset<PETYPE32>(ModuleBase, Raw);
}
//---------------------------------------------------------------------------
struct SImportRec
{
 LPSTR RealLibName;
 LPSTR LibName;
 LPSTR ProcName;
 PVOID Pointer;
};
//---------------------------------------------------------------------------
template<typename T> static bool _stdcall TFindImportRecord(PBYTE ModuleBase, LPSTR LibName, LPSTR ProcName, SImportThunk<T>** LookUpRec, SImportThunk<T>** AddrRec, bool Raw)   // Possible not fully correct  // TODO: By ordinal
{
 DOS_HEADER     *DosHdr    = (DOS_HEADER*)ModuleBase;
 WIN_HEADER<T>  *WinHdr    = (WIN_HEADER<T>*)&ModuleBase[DosHdr->OffsetHeaderPE];
 DATA_DIRECTORY *ImportDir = &WinHdr->OptionalHeader.DataDirectories.ImportTable;
 if(!ImportDir->DirectoryRVA)return NULL;
 IMPORT_DESC    *Import    = (IMPORT_DESC*)&ModuleBase[(Raw)?(TRvaToFileOffset<T>(ModuleBase,ImportDir->DirectoryRVA)):(ImportDir->DirectoryRVA)];

 T    OMask = ((T)1 << ((sizeof(T)*8)-1));
 bool ByOrd = ((T)ProcName <= 0xFFFF);
 for(DWORD tctr=0;Import[tctr].AddressTabRVA;tctr++)
  {
   LPSTR ModName = (LPSTR)&ModuleBase[(Raw)?(TRvaToFileOffset<T>(ModuleBase,Import[tctr].ModuleNameRVA)):(Import[tctr].ModuleNameRVA)];
   if(LibName && !IsNamesEqualIC(ModName,LibName))continue;     // Can search for a proc of any module  
   SImportThunk<T>* Table = (SImportThunk<T>*)&((BYTE*)ModuleBase)[(Raw)?(TRvaToFileOffset<T>(ModuleBase,Import[tctr].LookUpTabRVA)):(Import[tctr].LookUpTabRVA)];
   SImportThunk<T>* LtRVA = (SImportThunk<T>*)&((BYTE*)ModuleBase)[(Raw)?(TRvaToFileOffset<T>(ModuleBase,Import[tctr].AddressTabRVA)):(Import[tctr].AddressTabRVA)];
   if(ProcName == LibName)    // Secret :)   // If we need just any address inside that module
    {
     for(;!(*Table).Value || !(*LtRVA).Value;Table++,LtRVA++);
     if(AddrRec)*AddrRec = LtRVA;
     if(LookUpRec)*LookUpRec = Table;
	 return true;
    }
   for(DWORD actr=0;Table[actr].Value;actr++)
    {
	 bool OnlyOrd = (Table[actr].Value & OMask);
	 WORD OIndex  = 0;
	 if(OnlyOrd)
	  {
	   if(!ByOrd)continue;
	   T Ord = Table[actr].Value & ~OMask;
	   if((T)ProcName != Ord)continue;
	   OIndex = Ord;          // LONGLONG ordinal?
	  }
	   else
	    {
		 SImportByName* INam = (SImportByName*)&ModuleBase[Table[actr].Value];
		 if(ByOrd)
		  {
		   if(((T)ProcName != INam->Hint))continue;
		  }
		   else
		    {
			 if(!IsNamesEqual((LPSTR)&INam->Name, ProcName))continue;
			}
		 OIndex = OIndex;
		}
     if(AddrRec)*AddrRec = &LtRVA[actr];
     if(LookUpRec)*LookUpRec = &Table[actr];
	 return true;
    } 
  }
 return false;
}
//---------------------------------------------------------------------------
template<typename T> static PVOID _stdcall TResolveImportRecord(PVOID ModuleBase, SImportRec* IRec)
{
 SImportThunk<T>* AddrRec;
 SImportThunk<T>* LookUpRec;
 bool ByOrd = ((T)IRec->ProcName <= 0xFFFF);
 if(!TFindImportRecord<T>((PBYTE)ModuleBase, IRec->LibName, IRec->ProcName, &LookUpRec, &AddrRec, false))return NULL;
 PVOID   Old = (PVOID)AddrRec->Value;    // What about x64 ???????????????????????????
 AddrRec->Value = (T)IRec->Pointer;	// NOTE: Offset of address not always have same index as SImportThunk entry!
 return Old;
}
//---------------------------------------------------------------------------
static PVOID _stdcall ResolveImportRecord(PVOID ModuleBase, SImportRec* IRec)
{
 if(!IsValidPEHeader(ModuleBase))return NULL;
 if(IsValidModuleX64(ModuleBase))return TResolveImportRecord<PETYPE64>(ModuleBase, IRec);
 return TResolveImportRecord<PETYPE32>(ModuleBase,IRec);
}
//---------------------------------------------------------------------------
// NOTE: Kernel32.dll exports some functions with a different names(i.e. lstrlen and lstrlenA)
// Will not find a forwarded address
//
template<typename T> _declspec(noinline) static LPSTR _stdcall TGetProcedureInfoByAddr(PBYTE ModuleBase, PVOID ProcAddr, PDWORD OrdinalOut=NULL, int MatchIdx=0)  
{
 DOS_HEADER* DosHdr        = (DOS_HEADER*)ModuleBase;
 WIN_HEADER<T>* WinHdr     = (WIN_HEADER<T>*)&ModuleBase[DosHdr->OffsetHeaderPE];
 DATA_DIRECTORY* ExportDir = &WinHdr->OptionalHeader.DataDirectories.ExportTable;
 EXPORT_DIR* Export        = (EXPORT_DIR*)&ModuleBase[ExportDir->DirectoryRVA];
                   
 PDWORD NamePointers = (PDWORD)&ModuleBase[Export->NamePointersRVA];
 PDWORD AddressTable = (PDWORD)&ModuleBase[Export->AddressTableRVA];
 PWORD  OrdinalTable = (PWORD )&ModuleBase[Export->OrdinalTableRVA];
 for(UINT Ordinal=0; (Ordinal < Export->FunctionsNumber) && (Ordinal <= 0xFFFF);Ordinal++)
  {
   PBYTE Addr = &ModuleBase[AddressTable[Ordinal]];  
   if(Addr != ProcAddr)continue;
   MatchIdx--;
   if(MatchIdx > 0)continue;
   for(DWORD ctr=0;ctr < Export->NamePointersNumber;ctr++)  // By name
    {      
     if(Ordinal != OrdinalTable[ctr])continue;
     DWORD nrva  = NamePointers[ctr];   
     if(OrdinalOut)*OrdinalOut = Ordinal;
     return (LPSTR)&ModuleBase[nrva];
    }
   break;
  }
 return NULL;
}
//---------------------------------------------------------------------------      
template<typename T, bool Raw=false> _declspec(noinline) static PVOID _stdcall TGetProcedureAddress(PBYTE ModuleBase, LPSTR ProcName, LPSTR* Forwarder=NULL, PVOID* ProcEntry=NULL)  // No forwarding support, no ordinals
{
 DOS_HEADER* DosHdr        = (DOS_HEADER*)ModuleBase;
 WIN_HEADER<T>* WinHdr     = (WIN_HEADER<T>*)&ModuleBase[DosHdr->OffsetHeaderPE];
 DATA_DIRECTORY* ExportDir = &WinHdr->OptionalHeader.DataDirectories.ExportTable;
 if(!ExportDir->DirectoryRVA || !ExportDir->DirectorySize)return NULL;		 // No export directory!
 EXPORT_DIR* Export        = (EXPORT_DIR*)&ModuleBase[(Raw)?(TRvaToFileOffset<T>(ModuleBase,ExportDir->DirectoryRVA)):(ExportDir->DirectoryRVA)];
                   
 PDWORD NamePointers = (PDWORD)&ModuleBase[(Raw)?(TRvaToFileOffset<T>(ModuleBase,Export->NamePointersRVA)):(Export->NamePointersRVA)];
 PDWORD AddressTable = (PDWORD)&ModuleBase[(Raw)?(TRvaToFileOffset<T>(ModuleBase,Export->AddressTableRVA)):(Export->AddressTableRVA)];
 PWORD  OrdinalTable = (PWORD )&ModuleBase[(Raw)?(TRvaToFileOffset<T>(ModuleBase,Export->OrdinalTableRVA)):(Export->OrdinalTableRVA)];
 SIZE_T Ordinal = (SIZE_T)ProcName;
 if(Ordinal <= 0xFFFF)  // By Ordnal
  {
   if(Ordinal < Export->OrdinalBase)return NULL;
   Ordinal -= Export->OrdinalBase;
  }
   else
    {
     for(DWORD ctr=0;ctr < Export->NamePointersNumber;ctr++)  // By name
      {      
       DWORD nrva  = NamePointers[ctr];   
       if(!nrva || !IsNamesEqual((LPSTR)&ModuleBase[(Raw)?(TRvaToFileOffset<T>(ModuleBase,nrva)):(nrva)],ProcName))continue;    
       Ordinal = OrdinalTable[ctr];      // Name Ordinal 
       break;
      }
     if(Ordinal > 0xFFFF)return NULL;
    }

 PBYTE Addr = &ModuleBase[(Raw)?(TRvaToFileOffset<T>(ModuleBase,AddressTable[Ordinal])):(AddressTable[Ordinal])];  
 if(ProcEntry)*ProcEntry = &AddressTable[Ordinal];
 if(Forwarder && (Addr >= (PBYTE)Export) && (Addr < ((PBYTE)Export+ExportDir->DirectorySize)))*Forwarder = (LPSTR)Addr;
 return Addr;     
}
//---------------------------------------------------------------------------
static PVOID _stdcall GetProcedureAddress(PVOID ModuleBase, LPSTR ApiName, LPSTR* Forwarder=NULL, PVOID* ProcEntry=NULL) 
{
// DBGMSG("ApiName: %s",ApiName);
 if(!ModuleBase)return NULL;
 if(!IsValidPEHeader(ModuleBase))return NULL;
 if(IsValidModuleX64(ModuleBase))return TGetProcedureAddress<PETYPE64>((PBYTE)ModuleBase,ApiName,Forwarder,ProcEntry); 
 return TGetProcedureAddress<PETYPE32>((PBYTE)ModuleBase,ApiName,Forwarder,ProcEntry); 
}
//---------------------------------------------------------------------------
static PVOID _stdcall GetProcAddr(PVOID ModuleBase, LPSTR ApiName)
{
 LPSTR Forwarder = NULL;
 PVOID Addr = GetProcedureAddress(ModuleBase, ApiName, &Forwarder);
 if(Addr && Forwarder)
  {
   int DotPos = 0;
   char StrBuf[256];
   for(int ctr=0;Forwarder[ctr];ctr++)
     if(Forwarder[ctr]=='.')DotPos = ctr;  
   memcpy(&StrBuf,Forwarder,DotPos);
   StrBuf[DotPos] = 0;
   ModuleBase = GetModuleHandleA(StrBuf);          // TODO: Use PEB  // How to decode 'api-ms-win-core-com-l1-1-0.dll' ?
   if(!ModuleBase)return NULL;
   return GetProcAddr(ModuleBase, &Forwarder[DotPos+1]);
  }
 return Addr;
}
//---------------------------------------------------------------------------
static int _stdcall ParseForwarderStr(LPSTR InStr, LPSTR OutDllName, LPSTR OutProcName)    //  “NTDLL.RtlDeleteCriticalSection”,  “NTDLL.#491”
{         
 int namctr = 0;
 for(;InStr[namctr] && (InStr[namctr] != '.');namctr++)OutDllName[namctr] = InStr[namctr];
 OutDllName[namctr++] = '.';
 OutDllName[namctr+0] = 'd';
 OutDllName[namctr+1] = 'l';
 OutDllName[namctr+2] = 'l';
 OutDllName[namctr+3] = 0;
 if(InStr[namctr] == '#')
  {
   *OutProcName = 0;
   return DecStrToNum<int>(&InStr[++namctr]);
  }
   else 
    {
     int ctr=0;
     for(;InStr[namctr];ctr++,namctr++)OutProcName[ctr] = InStr[namctr];
     OutProcName[ctr] = 0;
    }
 return -1;
}
//---------------------------------------------------------------------------
template<typename T> SIZE_T TBaseOfImage(PBYTE ModuleBase)
{
 WIN_HEADER<T> *WinHdr = (WIN_HEADER<T>*)&ModuleBase[((DOS_HEADER*)ModuleBase)->OffsetHeaderPE];
 return (sizeof(PETYPE64)==sizeof(T))?(WinHdr->OptionalHeader.ImageBase64):(WinHdr->OptionalHeader.ImageBase);
}       
//---------------------------------------------------------------------------
template<typename T> T* TBaseOfImagePtr(PBYTE ModuleBase)
{
 WIN_HEADER<T> *WinHdr = (WIN_HEADER<T>*)&ModuleBase[((DOS_HEADER*)ModuleBase)->OffsetHeaderPE];
 return (sizeof(PETYPE64)==sizeof(T))?((T*)&WinHdr->OptionalHeader.ImageBase64):((T*)&WinHdr->OptionalHeader.ImageBase);
}       
//---------------------------------------------------------------------------

template<typename T> static bool _stdcall TFixRelocations(PBYTE ModuleBase, bool Raw)   // Possible not fully correct
{
 DOS_HEADER     *DosHdr    = (DOS_HEADER*)ModuleBase;
 WIN_HEADER<T>  *WinHdr    = (WIN_HEADER<T>*)&ModuleBase[DosHdr->OffsetHeaderPE];
 DATA_DIRECTORY *ReloctDir = &WinHdr->OptionalHeader.DataDirectories.FixUpTable;
 if(!ReloctDir->DirectoryRVA)return false;
 SIZE_T ImageBase = TBaseOfImage<T>(ModuleBase);
 SIZE_T LoadDelta = (SIZE_T)ModuleBase - ImageBase;
 PBYTE  RelocPtr  = (PBYTE)&ModuleBase[(Raw)?(TRvaToFileOffset<T>(ModuleBase,ReloctDir->DirectoryRVA)):(ReloctDir->DirectoryRVA)];
 for(UINT RelOffs=0;RelOffs < ReloctDir->DirectorySize;)
  {
   RELOCATION_DESC* CurRelBlk = (RELOCATION_DESC*)&RelocPtr[RelOffs];
   PBYTE BasePtr = (PBYTE)&ModuleBase[(Raw)?(TRvaToFileOffset<T>(ModuleBase,CurRelBlk->BaseRVA)):(CurRelBlk->BaseRVA)];
   for(UINT RIdx=0,RTotal=CurRelBlk->Count();RIdx < RTotal;RIdx++)
    {   
     BYTE Type = CurRelBlk->Records[RIdx].Type;   // NOTE: 'switch()' makes the code Base dependant (Remember Reflective Injection)
     if(Type == IMAGE_REL_BASED_HIGHLOW)     // x32
      {
       PUINT32 Value = (PUINT32)&BasePtr[CurRelBlk->Records[RIdx].Offset];     
       if(Raw)*Value = UINT32(ModuleBase + TRvaToFileOffset<T>(ModuleBase, (*Value - ImageBase)));    // Direct x32 address
         else *Value += LoadDelta;           
      }              
     else if(Type == IMAGE_REL_BASED_DIR64)  // x64
      {
       PUINT64 Value = (PUINT64)&BasePtr[CurRelBlk->Records[RIdx].Offset];     
       if(Raw)*Value = UINT64(ModuleBase + TRvaToFileOffset<T>(ModuleBase, (*Value - ImageBase)));    // Direct x32 address
         else *Value += LoadDelta;           
      }
//     else if(Type != IMAGE_REL_BASED_ABSOLUTE){LOGMSG("Unsupported reloc type: %u", Type);}    // 11:IMAGE_REL_BASED_HIGH3ADJ(3xWORD) and 4:IMAGE_REL_BASED_HIGHADJ(2xWORD)  // Can`t log if self relocating
    }
   RelOffs += CurRelBlk->BlkSize;
  }
 return true;
}
//--------------------------------------------------------------------------- 
static PVOID _stdcall LLLoadLibrary(LPSTR LibName, PVOID pLdrLoadDll)
{
 WCHAR NamBuf[MAX_PATH];
 UNICODE_STRING DllName; // we will use this to hold the information for the LdrLoadDll call
 PVOID ModBase = NULL;
 int ctr = 0;
 for(;LibName[ctr] && (ctr < (sizeof(NamBuf)/2));ctr++)NamBuf[ctr] = LibName[ctr];
 NamBuf[ctr] = 0;
 DllName.Buffer = (PWSTR)&NamBuf; // the dll path must be the .Buffer -> you can always just do = L"path" instead of passing a param for it
 DllName.Length = (ctr * sizeof(WCHAR)); // calc the length
 DllName.MaximumLength = (DllName.Length + sizeof(WCHAR)); // max length calc
 if(((decltype(LdrLoadDll)*)pLdrLoadDll)(NULL, NULL, &DllName, (PVOID*)&ModBase))return NULL;
 return ModBase;
}
//--------------------------------------------------------------------------- 
template<typename T> static int _stdcall TResolveImportsForMod(LPSTR ImpModName, PBYTE ModuleBase, PBYTE ExpModase, PVOID pLdrLoadDll=NULL)
{
 DOS_HEADER     *DosHdr    = (DOS_HEADER*)ModuleBase;
 WIN_HEADER<T>  *WinHdr = (WIN_HEADER<T>*)&ModuleBase[DosHdr->OffsetHeaderPE];
 DATA_DIRECTORY *ImportDir = &WinHdr->OptionalHeader.DataDirectories.ImportTable;
 if(!ImportDir->DirectoryRVA)return 0;
 IMPORT_DESC    *Import    = (IMPORT_DESC*)&ModuleBase[ImportDir->DirectoryRVA];

 T OMask = ((T)1 << ((sizeof(T)*8)-1));
 for(DWORD tctr=0;Import[tctr].AddressTabRVA;tctr++)
  {
   DWORD OldProt = 0;
   LPSTR ModName = (LPSTR)&ModuleBase[Import[tctr].ModuleNameRVA];
   if(!IsNamesEqualIC(ModName,ImpModName))continue;     // May by more than one entry for a same module
   LOGMSG("Updating import for '%s'",ImpModName);
   SImportThunk<T>* Table = (SImportThunk<T>*)&ModuleBase[Import[tctr].LookUpTabRVA];
   SImportThunk<T>* LtRVA = (SImportThunk<T>*)&ModuleBase[Import[tctr].AddressTabRVA];
   for(DWORD actr=0;Table[actr].Value;actr++)
    {
     bool OnlyOrd = (Table[actr].Value & OMask);
     if(OnlyOrd)   // Have only API ordinal
      {	                                                         
       T Ord = Table[actr].Value & ~OMask;
       LOGMSG("Ordinal: %u",Ord);
       if(!OldProt && !VirtualProtect(LtRVA,0x1000,PAGE_EXECUTE_READWRITE,&OldProt)){LOGMSG("VP failed: %u", GetLastError()); return -2;}  // TODO: IAT may span multiple pages
       LtRVA[actr].Value = (SIZE_T)TGetProcedureAddress<T>(ExpModase, (LPSTR)Ord);
      }
       else    // Have an import API name
        {
         SImportByName* INam = (SImportByName*)&ModuleBase[Table[actr].Value]; 
         LOGMSG("Name: %s",(LPSTR)&INam->Name);
         LPSTR Forwarder = NULL;
         PVOID PAddr = TGetProcedureAddress<T>(ExpModase, (LPSTR)&INam->Name, &Forwarder);   
         LOGMSG("New Address: %p",PAddr);
         if(Forwarder)
          {
           BYTE OutDllName[MAX_PATH]; 
           BYTE OutProcName[MAX_PATH];
           LPSTR PNamePtr;
           UINT Ord = ParseForwarderStr(Forwarder, (LPSTR)&OutDllName, (LPSTR)&OutProcName);  
           if(!OutProcName[0])PNamePtr = (LPSTR)Ord;
            else PNamePtr = (LPSTR)&OutProcName;
           PBYTE ImpModBaseF = (PBYTE)GetModuleHandleA((LPSTR)&OutDllName);         // <<<<<<<<<<<<<<<<<<<<<<<<<<
           if(!ImpModBaseF)return -1;
           PAddr = TGetProcedureAddress<T>(ImpModBaseF, PNamePtr, &Forwarder);    // No more forwarding?
          }
//         if(IsBadWritePtr(&LtRVA[actr].Value,sizeof(PVOID))){LOGMSG("Import table is not writable at %p !",&LtRVA[actr].Value); return -2;}      // Make optional?   // Avoid  IsBadReadPtr?
         if(!OldProt && !VirtualProtect(LtRVA,0x1000,PAGE_EXECUTE_READWRITE,&OldProt)){LOGMSG("VP failed: %u", GetLastError()); return -2;}   // TODO: IAT may span multiple pages
         LtRVA[actr].Value = (SIZE_T)PAddr;  
        }
     if(!LtRVA[actr].Value)return -3;  // Leaving OldProt unrestored is OK?
    }
   if(OldProt)VirtualProtect(LtRVA,0x1000,OldProt,&OldProt);
  }
 return 0;
}
//--------------------------------------------------------------------------- 

enum EFixMod {fmNone,fmEncKeyMsk=0xFF, fmFixSec=0x0100,fmFixImp=0x0200,fmFixRel=0x0400,   fmCryHdr=0x1000,fmCryImp=0x2000,fmCryExp=0x4000,fmCryRes=0x8000,  fmEncMode=0x00010000,fmOwnLDib=0x00040000,fmSelfMov=0x00080000};

template<typename T> static int _stdcall TResolveImports(PBYTE ModuleBase, PVOID pLdrLoadDll, UINT Flags=0)
{
 BYTE EncKey = Flags & fmEncKeyMsk;
 DOS_HEADER     *DosHdr = (DOS_HEADER*)ModuleBase;
 WIN_HEADER<T>  *WinHdr = (WIN_HEADER<T>*)&((PBYTE)ModuleBase)[DosHdr->OffsetHeaderPE];
 UINT            HdrLen = DosHdr->OffsetHeaderPE+WinHdr->FileHeader.HeaderSizeNT+sizeof(FILE_HEADER)+sizeof(DWORD);
 DATA_DIRECTORY *ImportDir = &WinHdr->OptionalHeader.DataDirectories.ImportTable;
 if(!ImportDir->DirectoryRVA)return 1;     // Not Present
 IMPORT_DESC *Import = (IMPORT_DESC*)&ModuleBase[ImportDir->DirectoryRVA];
 T OMask = ((T)1 << ((sizeof(T)*8)-1));
 for(DWORD tctr=0;Import[tctr].AddressTabRVA;tctr++)
  {
   BYTE  MName[256];
   LPSTR MNamePtr = (LPSTR)&ModuleBase[Import[tctr].ModuleNameRVA];
   if(EncKey){DecryptStrSimple(MNamePtr,(LPSTR)&MName, EncKey); MNamePtr=(LPSTR)&MName;}   
   PBYTE ImpModBase;
   if(Flags & fmOwnLDib)ImpModBase = NULL;  // TODO: Own Load library (Hidden)
    else if(pLdrLoadDll)ImpModBase = (PBYTE)LLLoadLibrary(MNamePtr, pLdrLoadDll);
   if(!ImpModBase)return -1;                // No logging in case of self import resolving
   SImportThunk<T>* Table = (SImportThunk<T>*)&ModuleBase[Import[tctr].LookUpTabRVA];
   SImportThunk<T>* LtRVA = (SImportThunk<T>*)&ModuleBase[Import[tctr].AddressTabRVA];
   for(DWORD actr=0;Table[actr].Value;actr++)
    {
     bool OnlyOrd = (Table[actr].Value & OMask);
     if(OnlyOrd)   // Have only API ordinal
      {	                                                         
       T Ord = Table[actr].Value & ~OMask;
       LtRVA[actr].Value = (SIZE_T)TGetProcedureAddress<T>(ImpModBase, (LPSTR)Ord);
      }
       else    // Have an import API name
        {
  	     SImportByName* INam = (SImportByName*)&ModuleBase[Table[actr].Value];
         BYTE  PName[256];
         LPSTR PNamePtr = (LPSTR)&INam->Name;
         if(EncKey){DecryptStrSimple(PNamePtr,(LPSTR)&PName, EncKey); PNamePtr=(LPSTR)&PName;}     
         LPSTR Forwarder=NULL;
         PVOID PAddr = TGetProcedureAddress<T>(ImpModBase, PNamePtr, &Forwarder);          
         if(Forwarder)
          {
           BYTE OutDllName[MAX_PATH]; 
           BYTE OutProcName[MAX_PATH];
           UINT Ord = ParseForwarderStr(Forwarder, (LPSTR)&OutDllName, (LPSTR)&OutProcName);  
           if(!OutProcName[0])PNamePtr = (LPSTR)Ord;
             else PNamePtr = (LPSTR)&OutProcName;
           PBYTE ImpModBaseF = (PBYTE)LLLoadLibrary((LPSTR)&OutDllName, pLdrLoadDll);
           if(!ImpModBaseF)return -2;
           PAddr = TGetProcedureAddress<T>(ImpModBaseF, PNamePtr, &Forwarder);    // No more forwarding?
          }
         LtRVA[actr].Value = (SIZE_T)PAddr;        
	    }
     if(!LtRVA[actr].Value)return -3;
    } 
  }
 return 0;
}
//--------------------------------------------------------------------------- 
_declspec(noinline) static UINT _stdcall MoveSections(UINT SecArrOffs, UINT TotalSecs, PBYTE ModuleBase, PBYTE Headers=NULL, UINT HdrSize=0)  // Must not be any function calls in body of this function
{
 if(!Headers || !HdrSize)Headers = ModuleBase;
 SECTION_HEADER *SecArr = (SECTION_HEADER*)&Headers[SecArrOffs];
 PBYTE* pRetAddr = (PBYTE*)_AddressOfReturnAddress();
 UINT RetFix = 0;
 for(int ctr = TotalSecs-1;ctr >= 0;ctr--)  // Compatible with any normal linker that keeps sections in order  // All section moved forward
  {
   if(HdrSize && ((PBYTE)&SecArr[ctr] >= &Headers[HdrSize]))SecArr = (SECTION_HEADER*)&ModuleBase[SecArrOffs];   // Continue at original header
   SECTION_HEADER* CurSec = &SecArr[ctr];
   PBYTE Dst   = &ModuleBase[CurSec->SectionRva]; 
   PBYTE Src   = &ModuleBase[CurSec->PhysicalOffset];
   size_t Size = CurSec->PhysicalSize;
   size_t ALen = Size/sizeof(size_t);
   size_t BLen = Size%sizeof(size_t);
   if(!RetFix && (*pRetAddr >= Src) && (*pRetAddr < &Src[Size]))RetFix = (Dst - Src);  // If RetAddr in current section being moved
   for(size_t ctr=Size-1;BLen;ctr--,BLen--)((char*)Dst)[ctr] = ((char*)Src)[ctr];  
   for(size_t ctr=ALen-1;ALen;ctr--,ALen--)((size_t*)Dst)[ctr] = ((size_t*)Src)[ctr];                // Copy of memcpy to avoid an inlining problems
   if(CurSec->VirtualSize > CurSec->PhysicalSize)                  // Fill ZERO space
    {
     Dst  = &ModuleBase[CurSec->SectionRva+CurSec->PhysicalSize]; 
     Size = CurSec->VirtualSize - CurSec->PhysicalSize;
     ALen = Size/sizeof(size_t);
     BLen = Size%sizeof(size_t);
     for(size_t ctr=0;ctr < ALen;ctr++)((size_t*)Dst)[ctr] = 0; 
     for(size_t ctr=(ALen*sizeof(size_t));ctr < Size;ctr++)((char*)Dst)[ctr] = 0;  
    }
  }
 if(RetFix)*pRetAddr += RetFix;
 return RetFix;
}
//---------------------------------------------------------------------------
template<typename T> static int _stdcall TCryptSensitiveParts(PBYTE ModuleBase, UINT Flags, bool Raw)
{
 BYTE EncKey = Flags & fmEncKeyMsk;
 if((Flags & (fmCryHdr|fmEncMode)) == fmCryHdr)   // Decrypt header first
  {
   UINT Offs    = 0;
   UINT EndOffs = sizeof(DOS_HEADER);
   for(;Offs < EndOffs;Offs++)ModuleBase[Offs] = DecryptByteWithCtr(ModuleBase[Offs], EncKey, Offs);
   DOS_HEADER *DosHdr = (DOS_HEADER*)ModuleBase;
   EndOffs = DosHdr->OffsetHeaderPE + sizeof(WIN_HEADER<T>);
   for(;Offs < EndOffs;Offs++)ModuleBase[Offs] = DecryptByteWithCtr(ModuleBase[Offs], EncKey, Offs);
   WIN_HEADER<T>  *WinHdr = (WIN_HEADER<T>*)&ModuleBase[DosHdr->OffsetHeaderPE];
   EndOffs = WinHdr->OptionalHeader.SizeOfHeaders;
   for(;Offs < EndOffs;Offs++)ModuleBase[Offs] = DecryptByteWithCtr(ModuleBase[Offs], EncKey, Offs);    
  }
 DOS_HEADER     *DosHdr = (DOS_HEADER*)ModuleBase;
 WIN_HEADER<T>  *WinHdr = (WIN_HEADER<T>*)&((PBYTE)ModuleBase)[DosHdr->OffsetHeaderPE];
 UINT            HdrLen = DosHdr->OffsetHeaderPE+WinHdr->FileHeader.HeaderSizeNT+sizeof(FILE_HEADER)+sizeof(DWORD);
 DATA_DIRECTORY* ImportDir = &WinHdr->OptionalHeader.DataDirectories.ImportTable;
 DATA_DIRECTORY* ExportDir = &WinHdr->OptionalHeader.DataDirectories.ExportTable;
 DATA_DIRECTORY* ResourDir = &WinHdr->OptionalHeader.DataDirectories.ResourceTable;

 if((Flags & fmCryImp) && ImportDir->DirectoryRVA)
  {
   IMPORT_DESC *Import = (IMPORT_DESC*)&ModuleBase[(Raw)?(TRvaToFileOffset<T>(ModuleBase,ImportDir->DirectoryRVA)):(ImportDir->DirectoryRVA)];
   T OMask = ((T)1 << ((sizeof(T)*8)-1));
   for(DWORD tctr=0;Import[tctr].AddressTabRVA;tctr++)
    {
     LPSTR MNamePtr = (LPSTR)&ModuleBase[(Raw)?(TRvaToFileOffset<T>(ModuleBase,Import[tctr].ModuleNameRVA)):(Import[tctr].ModuleNameRVA)];        
     if(Flags & fmEncMode)EncryptStrSimple(MNamePtr, MNamePtr, EncKey);      
       else DecryptStrSimple(MNamePtr, MNamePtr, EncKey);   
     SImportThunk<T>* Table = (SImportThunk<T>*)&((BYTE*)ModuleBase)[(Raw)?(TRvaToFileOffset<T>(ModuleBase,Import[tctr].LookUpTabRVA)):(Import[tctr].LookUpTabRVA)];
     for(DWORD actr=0;Table[actr].Value;actr++)
      {
       if(Table[actr].Value & OMask)continue; // No name, only ordinal
  	   SImportByName* INam = (SImportByName*)&ModuleBase[(Raw)?(TRvaToFileOffset<T>(ModuleBase,Table[actr].Value)):(Table[actr].Value)];        
       if(Flags & fmEncMode)EncryptStrSimple((LPSTR)&INam->Name, (LPSTR)&INam->Name, EncKey);          
        else DecryptStrSimple((LPSTR)&INam->Name, (LPSTR)&INam->Name, EncKey);  
      } 
    }
  }
 if((Flags & fmCryExp) && ExportDir->DirectoryRVA)
  {
   EXPORT_DIR* Export = (EXPORT_DIR*)&ModuleBase[(Raw)?(TRvaToFileOffset<T>(ModuleBase,ExportDir->DirectoryRVA)):(ExportDir->DirectoryRVA)];
   LPSTR ModName = (LPSTR)&ModuleBase[(Raw)?(TRvaToFileOffset<T>(ModuleBase,Export->NameRVA)):(Export->NameRVA)]; 
   if(Flags & fmEncMode)EncryptStrSimple(ModName, ModName, EncKey);            
     else  DecryptStrSimple(ModName, ModName, EncKey);            
   for(DWORD ctr=0;ctr < Export->NamePointersNumber;ctr++)
    {   
     DWORD Offs = (Raw)?(TRvaToFileOffset<T>(ModuleBase,Export->NamePointersRVA)):(Export->NamePointersRVA);     
     DWORD rva  = (((PDWORD)&ModuleBase[Offs])[ctr]);                
     LPSTR CurProcName = (LPSTR)&ModuleBase[(Raw)?(TRvaToFileOffset<T>(ModuleBase,rva)):(rva)];         
     if(Flags & fmEncMode)EncryptStrSimple(CurProcName, CurProcName, EncKey);     
       else DecryptStrSimple(CurProcName, CurProcName, EncKey);     
    }
  }
 if((Flags & fmCryRes) && ResourDir->DirectoryRVA)
  {
   PBYTE ResBase = &ModuleBase[(Raw)?(TRvaToFileOffset<T>(ModuleBase,ResourDir->DirectoryRVA)):(ResourDir->DirectoryRVA)];
   if(Flags & fmEncMode){for(UINT ctr=0;ctr < ResourDir->DirectorySize;ctr++)ResBase[ctr] = EncryptByteWithCtr(ResBase[ctr], EncKey, ctr);}
     else {for(UINT ctr=0;ctr < ResourDir->DirectorySize;ctr++)ResBase[ctr] = DecryptByteWithCtr(ResBase[ctr], EncKey, ctr);}      
  }
 if((Flags & (fmCryHdr|fmEncMode)) == (fmCryHdr|fmEncMode))
  {
   UINT HdrLen = WinHdr->OptionalHeader.SizeOfHeaders;
   for(UINT ctr=0;ctr < HdrLen;ctr++)ModuleBase[ctr] = EncryptByteWithCtr(ModuleBase[ctr], EncKey, ctr);
  }
 return 0;
}
//---------------------------------------------------------------------------
__declspec(noinline) static PBYTE RetAddrProc(void) {return (PBYTE)_ReturnAddress();}   // Helps to avoid problems with inlining of functions, called from a thread`s EP (Or we may get address in ntdll.dll instead of our module)

template<typename T> __declspec(noinline) static int _stdcall TFixUpModuleInplace(PBYTE& ModuleBase, PVOID pNtDll, UINT& Flags, UINT* pRetFix=NULL)   // Buffer at ModuleBase must be large enough to contain all sections
{
 if(!ModuleBase)    // Will move Self if Base is not specified
  {
   SIZE_T ThisModBase = (SIZE_T)RetAddrProc();  
   if(Flags & fmCryHdr)   // Search for encrypted PE header
    {
     for(ThisModBase &= PLATMEMALIGNMSK;;ThisModBase -= PLATMEMALIGN)   // May crash if header is incorrect
      {
       BYTE Header[0x400];
       BYTE EncKey = Flags & fmEncKeyMsk;
       UINT Offs    = 0;
       UINT EndOffs = sizeof(DOS_HEADER);
       memcpy(&Header,(PBYTE)ThisModBase,sizeof(Header));
       for(;Offs < EndOffs;Offs++)Header[Offs] = DecryptByteWithCtr(Header[Offs], EncKey, Offs);
       DOS_HEADER *XDosHdr = (DOS_HEADER*)&Header;
       if((XDosHdr->FlagMZ != SIGN_MZ) || (XDosHdr->OffsetHeaderPE < sizeof(IMAGE_DOS_HEADER)) || (XDosHdr->OffsetHeaderPE >= 0x400))continue;
       EndOffs = XDosHdr->OffsetHeaderPE + sizeof(WIN_HEADER<T>);
       for(;Offs < EndOffs;Offs++)Header[Offs] = DecryptByteWithCtr(Header[Offs], EncKey, Offs);
       WIN_HEADER<T>  *XWinHdr = (WIN_HEADER<T>*)&Header[XDosHdr->OffsetHeaderPE];
       if((XWinHdr->FlagPE != SIGN_PE))continue;
       break;
      }
    }
     else
      {
       for(ThisModBase &= PLATMEMALIGNMSK;;ThisModBase -= PLATMEMALIGN)   // May crash if header is incorrect
        {
         DOS_HEADER *XDosHdr = (DOS_HEADER*)ThisModBase;
         if((XDosHdr->FlagMZ != SIGN_MZ) || (XDosHdr->OffsetHeaderPE < sizeof(IMAGE_DOS_HEADER)) || (XDosHdr->OffsetHeaderPE >= 0x400))continue;
         WIN_HEADER<T>  *XWinHdr = (WIN_HEADER<T>*)&((PBYTE)ThisModBase)[XDosHdr->OffsetHeaderPE];
         if((XWinHdr->FlagPE != SIGN_PE))continue;
         break;
        }
      }
   Flags |= fmSelfMov;
   ModuleBase = (PBYTE)ThisModBase;
  }
 DOS_HEADER *DosHdr = (DOS_HEADER*)ModuleBase;
 if((Flags & fmEncKeyMsk) && (Flags & (fmCryHdr|fmCryImp|fmCryExp|fmCryRes)))
  {
   TCryptSensitiveParts<T>(ModuleBase, Flags, (Flags & fmFixSec));   // fmFixSec == Raw module?
   if(ModuleBase[1] != 'Z')return -1;        // Invalid
   if(*ModuleBase != 'M')*ModuleBase = 'M';  // May be invalidated on purpose 
  }
 WIN_HEADER<T>  *WinHdr = (WIN_HEADER<T>*)&((PBYTE)ModuleBase)[DosHdr->OffsetHeaderPE];
 UINT RetFix = 0;
 if(Flags & fmFixSec)         // A Raw module assumed as input
  {
   UINT SecArrOffs = DosHdr->OffsetHeaderPE+WinHdr->FileHeader.HeaderSizeNT+sizeof(FILE_HEADER)+sizeof(DWORD);
   UINT SecsNum    = WinHdr->FileHeader.SectionsNumber;
   if(!(Flags & fmSelfMov))   // Check if this code is inside of this PE image to move
    {
     PBYTE ThisProcAddr = RetAddrProc();  
     SIZE_T RawSize = TCalcRawModuleSize<T>(ModuleBase);
     if((ThisProcAddr >= ModuleBase)&&(ThisProcAddr < &ModuleBase[RawSize]))Flags |= fmSelfMov;    // Inside
    }
   if(Flags & fmSelfMov)
    {
     PBYTE MProcAddr = (PBYTE)&MoveSections;    // On x64 it will be a correct address of 'MoveSections' because of usage of LEA instruction for RIP addressing
#ifndef _AMD64_ 
     MProcAddr = &ModuleBase[TRvaToFileOffset<T>(ModuleBase,(MProcAddr - (PBYTE)TBaseOfImage<T>(ModuleBase)))];    // Fix section in a Raw module image    // Assume that &MoveSections originally will be as for default image base
#endif  
     BYTE Header[0x400];  // Must be enough to save a full PE header 
     memcpy(&Header,ModuleBase,sizeof(Header));   // Save an original PE header
     memcpy(ModuleBase, MProcAddr, sizeof(Header));
     RetFix = ((decltype(&MoveSections))ModuleBase)(SecArrOffs, SecsNum, ModuleBase, (PBYTE)&Header, sizeof(Header));  // (UINT (_stdcall *)(UINT,UINT,PBYTE,PBYTE,UINT))
     memcpy(ModuleBase,&Header,sizeof(Header));   // Put header back
     if(pRetFix)*pRetFix = RetFix;
     if(RetFix)*((PBYTE*)_AddressOfReturnAddress()) += RetFix;
    }
     else MoveSections(SecArrOffs, SecsNum, ModuleBase);   // Not this module, just move its sections   
  }
 if(Flags & fmFixRel)TFixRelocations<T>(ModuleBase, false);
 if((Flags & fmFixImp) && WinHdr->OptionalHeader.DataDirectories.ImportTable.DirectoryRVA)  
  {
   DWORD NLdrLoadDll[] = {~0x4C72644C, ~0x4464616F, ~0x00006C6C};  // LdrLoadDll     
   NLdrLoadDll[0] = ~NLdrLoadDll[0];
   NLdrLoadDll[1] = ~NLdrLoadDll[1];
   NLdrLoadDll[2] = ~NLdrLoadDll[2];
   PVOID Proc = TGetProcedureAddress<T>((PBYTE)pNtDll, (LPSTR)&NLdrLoadDll);
   if(TResolveImports<T>(ModuleBase, Proc, Flags) < 0)return -2;
  }
 return 0;    //((WinHdr->OptionalHeader.EntryPointRVA)?(&ModuleBase[WinHdr->OptionalHeader.EntryPointRVA]):(NULL));
}
//---------------------------------------------------------------------------
static int _stdcall FixUpModuleInplace(PBYTE& ModuleBase, PVOID pNtDll, UINT Flags=0, UINT* pRetFix=NULL)
{
 if(IsValidModuleX64(ModuleBase))return TFixUpModuleInplace<PETYPE64>(ModuleBase, pNtDll, Flags, pRetFix);
 return TFixUpModuleInplace<PETYPE32>(ModuleBase, pNtDll, Flags, pRetFix);
}
//---------------------------------------------------------------------------
static int _stdcall CryptSensitiveParts(PBYTE ModuleBase, UINT Flags, bool Raw)
{
 if(IsValidModuleX64(ModuleBase))return TCryptSensitiveParts<PETYPE64>(ModuleBase, Flags, Raw);
 return TCryptSensitiveParts<PETYPE32>(ModuleBase, Flags, Raw);      
}
//---------------------------------------------------------------------------
static void _stdcall MakeFakeHeaderPE(PBYTE Header)  // To pass a check in RtlImageNtHeaderEx
{
 DOS_HEADER *DosHdr = (DOS_HEADER*)Header;
 DosHdr->FlagMZ = SIGN_MZ;
 DosHdr->OffsetHeaderPE = sizeof(DOS_HEADER);
 WIN_HEADER<PECURRENT>  *WinHdr = (WIN_HEADER<PECURRENT>*)&Header[DosHdr->OffsetHeaderPE];
 WinHdr->FlagPE = SIGN_PE;
}
//---------------------------------------------------------------------------
template<typename T> static int _stdcall TSectionsProtectRW(PBYTE ModuleBase, bool Restore)
{
 DOS_HEADER     *DosHdr = (DOS_HEADER*)ModuleBase;
 WIN_HEADER<T>  *WinHdr = (WIN_HEADER<T>*)&ModuleBase[DosHdr->OffsetHeaderPE];
 UINT            HdrLen = DosHdr->OffsetHeaderPE+WinHdr->FileHeader.HeaderSizeNT+sizeof(FILE_HEADER)+sizeof(DWORD);
 SECTION_HEADER *CurSec = (SECTION_HEADER*)&ModuleBase[HdrLen];
 DWORD Oldp;
 if(!Restore)VirtualProtect(ModuleBase,WinHdr->OptionalHeader.SizeOfHeaders,PAGE_EXECUTE_READWRITE,&Oldp);
 WinHdr->OptionalHeader.Win32Version = Oldp; 
 for(int ctr = 0;ctr < WinHdr->FileHeader.SectionsNumber;ctr++,CurSec++)
  {
   if(Restore)
    {     
     VirtualProtect(&ModuleBase[CurSec->SectionRva],CurSec->VirtualSize,CurSec->PtrToLineNumbers,&Oldp);
     CurSec->PtrToLineNumbers = 0;
    }
    else 
     {
      VirtualProtect(&ModuleBase[CurSec->SectionRva],CurSec->VirtualSize,PAGE_EXECUTE_READWRITE,&Oldp);
      CurSec->PtrToLineNumbers = Oldp;
     }
  }
 if(Restore)VirtualProtect(ModuleBase,WinHdr->OptionalHeader.SizeOfHeaders,WinHdr->OptionalHeader.Win32Version,&Oldp);
 return 0;
}
//---------------------------------------------------------------------------
static DWORD _stdcall CalcChecksumPE(PBYTE ModuleBase, UINT Size)
{
 DOS_HEADER *DosHdr = (DOS_HEADER*)ModuleBase;
 WIN_HEADER<PECURRENT> *WinHdr = (WIN_HEADER<PECURRENT>*)&(((BYTE*)ModuleBase)[DosHdr->OffsetHeaderPE]);

 unsigned long long checksum = 0;
 unsigned long long top = 0xFFFFFFFF;
 top++;

 DWORD CSimOffs = (PBYTE)&WinHdr->OptionalHeader.FileCheckSum - ModuleBase;
 PDWORD DataPtr = (PDWORD)ModuleBase;
 for(UINT idx=0;idx < Size;idx += 4)
  {
   if(idx == CSimOffs)continue;   //Skip "CheckSum" DWORD		
   checksum = (checksum & 0xffffffff) + *(PDWORD)&ModuleBase[idx] + (checksum >> 32);     // Calculate checksum
   if(checksum > top)checksum = (checksum & 0xffffffff) + (checksum >> 32);               // TODO: Without 64bit shift
  }
 //Finish checksum
 checksum  = (checksum & 0xffff) + (checksum >> 16);
 checksum  = checksum + (checksum >> 16);
 checksum  = checksum & 0xffff;
 checksum += Size;
 return checksum;
}
//---------------------------------------------------------------------------
static int _stdcall LoadRichInfo(PBYTE ModuleBase, SRichRec* Recs, PUINT RecsNum, PUINT Offset=nullptr, bool* IsChkValid=nullptr)
{
 if(!IsValidPEHeader(ModuleBase))return -1;
 DOS_HEADER* DosHdr  = (DOS_HEADER*)ModuleBase;
 UINT EndOffs = DosHdr->OffsetHeaderPE;      // Rich is before PE
 if(EndOffs < sizeof(DOS_HEADER))return -1;
 if(EndOffs > 1024)EndOffs = 1024;  
 PDWORD EndPtr = (PDWORD)&ModuleBase[EndOffs];
 DWORD  XorKey = 0;
 DWORD  Sign   = 'hciR';
 PDWORD RBeg   = nullptr;
 PDWORD REnd   = nullptr;
 do
  {
   EndPtr--;
   if(*EndPtr == Sign)
    {
     if(REnd){RBeg=EndPtr; break;}
     XorKey = EndPtr[1];
     REnd   = EndPtr;
     Sign   = 'SnaD' ^ XorKey;
    }
  }
   while(EndPtr > (PDWORD)ModuleBase);
 if(!RBeg || !REnd)return -3;
 int  Counter = ((REnd - RBeg) / 2) - 2;
 UINT ROffs   = (PBYTE)RBeg - (PBYTE)ModuleBase;
 if(Counter < 0)return -4;
 if(RecsNum)*RecsNum = Counter;
 if(Offset)*Offset = ROffs;
 RBeg += 4;
 PDWORD DstPtr = (PDWORD)Recs;
 UINT RichSize = ((((XorKey >> 5) % 3) + Counter) * 8) + 0x20;   // Counter not includes first 4 DWORDs
 for(UINT ctr=0;ctr < Counter;ctr++)
 {
  *(DstPtr++) = *(RBeg++) ^ XorKey;
  *(DstPtr++) = *(RBeg++) ^ XorKey;
 }
 if(IsChkValid)
  {
   DWORD OrgXorKey = ROffs;
   for(UINT Idx=0;Idx < ROffs;Idx++)OrgXorKey += ((Idx < 0x3C)||(Idx > 0x3F))?(RotL((DWORD)ModuleBase[Idx], Idx)):(0);    // Skipping OffsetHeaderPE (Slower but without a buffer)
   for(UINT Idx=0;Idx < Counter;Idx++)
    {
     PDWORD Rec = (PDWORD)&Recs[Idx];
     OrgXorKey += RotL(Rec[0], (BYTE)Rec[1]);
    }
   *IsChkValid = (OrgXorKey == XorKey);
  }
 return RichSize;
}
//---------------------------------------------------------------------------
static int _stdcall SaveRichInfo(PBYTE ModuleBase, SRichRec* Recs, UINT RecsNum, UINT Offset)
{
 if(!IsValidPEHeader(ModuleBase))return -1;
 DOS_HEADER* DosHdr  = (DOS_HEADER*)ModuleBase;
 WIN_HEADER<PECURRENT>* WinHdr = (WIN_HEADER<PECURRENT>*)&(ModuleBase[DosHdr->OffsetHeaderPE]);

 UINT OldOffsPE = DosHdr->OffsetHeaderPE;
 DWORD XorKey = Offset;
 DosHdr->OffsetHeaderPE = 0;    // Must be set to 0 before calculation
 for(UINT Idx=0;Idx < Offset;Idx++)XorKey += RotL((DWORD)ModuleBase[Idx], Idx);
 for(UINT Idx=0;Idx < RecsNum;Idx++)
  {
   PDWORD Rec = (PDWORD)&Recs[Idx];
   XorKey += RotL(Rec[0], (BYTE)Rec[1]);
  }
 UINT RichSize  = ((((XorKey >> 5) % 3) + RecsNum) * 8) + 0x20;
 PDWORD OPtr    = (PDWORD)&ModuleBase[Offset];

 UINT NewOffsPE = Offset + RichSize;
 UINT SizePE    = WinHdr->OptionalHeader.SizeOfHeaders - OldOffsPE;
 if(NewOffsPE < OldOffsPE)
  {
   memmove(&ModuleBase[NewOffsPE], &ModuleBase[OldOffsPE], SizePE);
   memset(&ModuleBase[NewOffsPE+SizePE],0,OldOffsPE-NewOffsPE);
  }
   else if(NewOffsPE > OldOffsPE)
    {
     SizePE -= (NewOffsPE - OldOffsPE);
     memmove(&ModuleBase[NewOffsPE], &ModuleBase[OldOffsPE], SizePE);
    }

 DosHdr->OffsetHeaderPE = NewOffsPE;
 memset(OPtr, 0, RichSize);  
 *(OPtr++) = XorKey ^ 'SnaD';
 *(OPtr++) = XorKey;
 *(OPtr++) = XorKey;
 *(OPtr++) = XorKey;
 for(UINT Idx=0,Tot=RecsNum*2;Idx < Tot;Idx++)*(OPtr++) = ((PDWORD)Recs)[Idx] ^ XorKey; 
 *(OPtr++) = 'hciR';
 *(OPtr++) = XorKey;
 return RichSize;
}
//---------------------------------------------------------------------------

/*UINT  _stdcall GetEntryPoint(PVOID Header);
void  _stdcall SetEntryPoint(PVOID Header, UINT Entry);
PDWORD _stdcall GetEntryPointOffset(PVOID Header);
void  _stdcall InjLoadLibraryA(LPSTR LibNameToLoad, PVOID AddrInKernel32);
DWORD _stdcall RvaToFileOffset(HANDLE hModuleFile, DWORD ModuleRva, SECTION_HEADER **RvaInSection);
DWORD _stdcall RvaToFileOffsetF(HANDLE hModuleFile, DWORD ModuleRva, SECTION_HEADER *RvaInSection);
DWORD _stdcall FileOffsetToRva(PVOID ModuleInMem, DWORD FileOffset, SECTION_HEADER **OffsetInSection);
DWORD _stdcall FileOffsetToRvaF(HANDLE hModuleFile, DWORD FileOffset, SECTION_HEADER *OffsetInSection);
bool  _stdcall GetSectionForAddress(HMODULE ModulePtr, PVOID Address, SECTION_HEADER **Section); */
//---------------------------------------------------------------------------
class CRawResEnum
{



};


//==============================================================================
#pragma warning(pop)

//#endif
