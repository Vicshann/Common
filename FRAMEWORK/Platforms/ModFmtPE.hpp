
#pragma once

//============================================================================================================
template<typename PHT> struct NFMTPE
{
static constexpr uint32 SIGN_MZ = 0x5A4D;
static constexpr uint32 SIGN_PE = 0x4550;
//------------------------------------------------------------------------------------------------------------
enum class ESecFlags: uint32
{
 TYPE_DSECT              =  0x00000001,  // Reserved.
 TYPE_NOLOAD             =  0x00000002,  // Reserved.
 TYPE_GROUP              =  0x00000004,  // Reserved.
 TYPE_NO_PAD             =  0x00000008,  // Reserved.
 TYPE_COPY               =  0x00000010,  // Reserved.
 CNT_CODE                =  0x00000020,  // Section contains code.
 CNT_INITIALIZED_DATA    =  0x00000040,  // Section contains initialized data.
 CNT_UNINITIALIZED_DATA  =  0x00000080,  // Section contains uninitialized data.
 LNK_OTHER               =  0x00000100,  // Reserved.
 LNK_INFO                =  0x00000200,  // Section contains comments or some other type of information.
 TYPE_OVER               =  0x00000400,  // Reserved.
 LNK_REMOVE              =  0x00000800,  // Section contents will not become part of image.
 LNK_COMDAT              =  0x00001000,  // Section contents comdat.
 UNKNOW                  =  0x00002000,  // Reserved.
 NO_DEFER_SPEC_EXC       =  0x00004000,  // Reset speculative exceptions handling bits in the TLB entries for this section.
 MEM_FARDATA             =  0x00008000,  // Section content can be accessed relative to GP
 MEM_SYSHEAP             =  0x00010000,  // Obsolete
 MEM_PURGEABLE           =  0x00020000,  //
 MEM_LOCKED              =  0x00040000,  //
 MEM_PRELOAD             =  0x00080000,  //
 ALIGN_1BYTES            =  0x00100000,  //
 ALIGN_2BYTES            =  0x00200000,  //
 ALIGN_4BYTES            =  0x00300000,  //
 ALIGN_8BYTES            =  0x00400000,  //
 ALIGN_16BYTES           =  0x00500000,  // Default alignment if no others are specified.
 ALIGN_32BYTES           =  0x00600000,  //
 ALIGN_64BYTES           =  0x00700000,  //
 ALIGN_128BYTES          =  0x00800000,  //
 ALIGN_256BYTES          =  0x00900000,  //
 ALIGN_512BYTES          =  0x00A00000,  //
 ALIGN_1024BYTES         =  0x00B00000,  //
 ALIGN_2048BYTES         =  0x00C00000,  //
 ALIGN_4096BYTES         =  0x00D00000,  //
 ALIGN_8192BYTES         =  0x00E00000,  //
 ALIGN_MASK              =  0x00F00000,  // UNUSED - Helps reading align value
 LNK_NRELOC_OVFL         =  0x01000000,  // Section contains extended relocations.
 MEM_DISCARDABLE         =  0x02000000,  // Section can be discarded.
 MEM_NOT_CACHED          =  0x04000000,  // Section is not cachable.
 MEM_NOT_PAGED           =  0x08000000,  // Section is not pageable.
 MEM_SHARED              =  0x10000000,  // Section is shareable.
 MEM_EXECUTE             =  0x20000000,  // Section is executable.
 MEM_READ                =  0x40000000,  // Section is readable.
 MEM_WRITE               =  0x80000000,  // Section is writeable.
};

enum class EImgRelBased: uint32
{
 ABSOLUTE       = 0,
 HIGH           = 1,
 LOW            = 2,
 HIGHLOW        = 3,
 HIGHADJ        = 4,
 MIPS_JMPADDR   = 5,
 MIPS_JMPADDR16 = 9,
 IA64_IMM64     = 9,
 DIR64          = 10,
};
//------------------------------------------------------------------------------------------------------------
#pragma pack( push, 1 )     // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Check alignment! <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

struct SDosHdr   //DOS_HEADER
{
 uint16  FlagMZ;               // Magic number                            0x00
 uint16  LastPageSize;         // Bytes on last page of file              0x02
 uint16  PageCount;            // Pages in file (Page = 512 bytes)        0x04
 uint16  RelocCount;           // Elements count in Relocations table     0x06
 uint16  HeaderSize;           // Size of header in paragraphs            0x08
 uint16  MinMemory;            // Min. extra paragraphs needed            0x0A
 uint16  MaxMemory;            // Max. extra paragraphs needed            0x0C
 uint16  ValueSS;              // Initial SS value                        0x0E
 uint16  ValueSP;              // Initial SP value                        0x10
 uint16  CheckSum;             // Checksum of the file                    0x12
 uint16  ValueIP;              // Initial IP value                        0x14
 uint16  ValueCS;              // Initial CS value                        0x16
 uint16  RelocTableOffset;     // Address of relocation table             0x18
 uint16  OverlayNumber;        // Overlay number (0 for main module)      0x1A
 uint32  Compl20h;             // Double paragraph align                  0x1C
 uint32  Reserved1;            // Reserved words                          0x20
 uint16  OemID;                // OEM identifier                          0x24
 uint16  OemInfo;              // OEM information                         0x26
 uint8   Reserved2[20];        // Reserved bytes                          0x28
 uint32  OffsetHeaderPE;       // File address of new exe header          0x3C
};
//------------------------------------------------------------------------------------------------------------
struct SDataDir     //DATA_DIRECTORY
{
 uint32  DirectoryRVA;     // RVA of the location of the directory.       0x00
 uint32  DirectorySize;    // Size of the directory.                      0x04
};
//------------------------------------------------------------------------------------------------------------
struct SDataDirTbl   //DATA_DIRECTORIES_TABLE
{
 SDataDir  ExportTable;      // Export Directory             0x78
 SDataDir  ImportTable;      // Import Directory             0x80
 SDataDir  ResourceTable;    // Resource Directory           0x88
 SDataDir  ExceptionTable;   // Exception Directory          0x90
 SDataDir  SecurityTable;    // Security Directory           0x98
 SDataDir  FixUpTable;       // Base Relocation Table        0xA0
 SDataDir  DebugTable;       // Debug Directory              0xA8
 SDataDir  ImageDescription; // Description String           0xB0
 SDataDir  MachineSpecific;  // Machine Value (MIPS GP)      0xB8
 SDataDir  TlsDirectory;     // TLS Directory                0xC0
 SDataDir  LoadConfigDir;    // Load Configuration Directory 0xC8
 SDataDir  BoundImportDir;   // Bound(Delayed) Import Dir    0xD0
 SDataDir  ImportAddrTable;  // Import Address Table         0xD8
 SDataDir  Reserved1;        // Reserved (Must be NULL)      0xE0
 SDataDir  Reserved2;        // Reserved (Must be NULL)      0xE8
 SDataDir  Reserved3;        // Reserved (Must be NULL)      0xF0
};
//------------------------------------------------------------------------------------------------------------
struct SFileHdr    // FILE_HEADER
{
 uint16  TypeCPU;         // Machine((Alpha/Motorola/.../0x014C = I386)   0x04
 uint16  SectionsNumber;  // Number of sections in the file               0x06
 uint32  TimeDateStamp;   // Number of seconds since Dec 31,1969,4:00PM   0x08
 uint32  TablePtrCOFF;    // Used in OBJ files and PE with debug info     0x0C
 uint32  TableSizeCOFF;   // The number of symbols in COFF table          0x10
 uint16  HeaderSizeNT;    // Size of the OptionalHeader structure         0x14
 uint16  Flags;           // 0000-Program; 0001-NoReloc; 0002-Can Exec;   0x16
};                        // 0200-Address fixed; 2000-This DLL
//------------------------------------------------------------------------------------------------------------
struct SOptHdr    // OPTIONAL_HEADER
{
STASRT(SameTypes<PHT, PTRCURRENT>::V || SameTypes<PHT, PTRTYPE32>::V || SameTypes<PHT, PTRTYPE64>::V, "Unsupported architecture type!");

 uint16  Magic;           // 0107-ROM projection;010B-Normal projection   0x18
 uint8   MajLinkerVer;    // Linker version number                        0x1A
 uint8   MinLinkerVer;    // Linker version number                        0x1B
 uint32  CodeSize;        // Sum of sizes all code sections(ordinary one) 0x1C
 uint32  InitDataSize;    // Size of the initialized data                 0x20
 uint32  UnInitDataSize;  // Size of the uninitialized data section (BSS) 0x24
 uint32  EntryPointRVA;   // Address of 1st instruction to be executed    0x28
 uint32  BaseOfCode;      // Address (RVA) of beginning of code section   0x2C
 typename TSW<SameTypes<PHT, PTRTYPE32>::V, uint32, ETYPE>::T  BaseOfData;   // Address (RVA) of beginning of data section   0x30
 typename TSW<SameTypes<PHT, PTRTYPE32>::V, uint32, uint64>::T ImageBase;    // The *preferred* load address of the file     0x34
/* union
  {
   struct
    {
     uint32 BaseOfData;  // Address (RVA) of beginning of data section   0x30
     ULONG ImageBase;    // The *preferred* load address of the file     0x34
    };
   ULONGLONG ImageBase64;
  };  */
 uint32  SectionAlign;      // Alignment of sections when loaded into mem   0x38
 uint32  FileAlign;         // Align. of sections in file(mul of 512 bytes) 0x3C
 uint16  MajOperSysVer;     // Version number of required OS                0x40
 uint16  MinOperSysVer;     // Version number of required OS                0x42
 uint16  MajImageVer;       // Version number of image                      0x44
 uint16  MinImageVer;       // Version number of image                      0x46
 uint16  MajSubSysVer;      // Version number of subsystem                  0x48
 uint16  MinSubSysVer;      // Version number of subsystem                  0x4A
 uint32  Win32Version;      // Dunno! But I guess for future use.           0x4C
 uint32  SizeOfImage;       // Total size of the PE image in memory         0x50
 uint32  SizeOfHeaders;     // Size of all headers & section table          0x54
 uint32  FileCheckSum;      // Image file checksum                          0x58
 uint16  SubSystem;         // 1-NotNeeded;2-WinGUI;3-WinCON;5-OS2;7-Posix  0x5C
 uint16  FlagsDLL;          // Used to indicate if a DLL image includes EPs 0x5E
 PHT     StackReserveSize;  // Size of stack to reserve                     0x60
 PHT     StackCommitSize;   // Size of stack to commit                      0x64 / 0x68
 PHT     HeapReserveSize;   // Size of local heap space to reserve          0x68 / 0x70
 PHT     HeapCommitSize;    // Size of local heap space to commit           0x6C / 0x78
 uint32  LoaderFlags;       // Choose Break/Debug/RunNormally(def) on load  0x70 / 0x80
 uint32  NumOfSizesAndRVA;  // Length of next DataDirectory array(alw10h)   0x74 / 0x84
 SDataDirTbl DataDirectories; //                             0x78 / 0x88
};
//------------------------------------------------------------------------------------------------------------
struct SWinHdr   // WIN_HEADER                // Must be uint64 aligned
{
 uint32    FlagPE;          // PE File Signature             0x00
 SFileHdr  FileHeader;      // File header                   0x04
 SOptHdr   OptionalHeader;  // Optional file header          0x18
};
//------------------------------------------------------------------------------------------------------------
struct SExpDir  //EXPORT_DIR
{
 uint32  Characteristics;     // Reserved MUST BE NULL                  0x00
 uint32  TimeDateStamp;       // Date of Creation                       0x04
 uint32  Version;             // Export Version - Not Used              0x08
 uint32  NameRVA;             // RVA of Module Name                     0x0C
 uint32  OrdinalBase;         // Base Number of Functions               0x10
 uint32  FunctionsNumber;     // Number of all exported functions       0x14
 uint32  NamePointersNumber;  // Number of functions names              0x18
 uint32  AddressTableRVA;     // RVA of Functions Address Table         0x1C
 uint32  NamePointersRVA;     // RVA of Functions Name Pointers Table   0x20
 uint32  OrdinalTableRVA;     // RVA of Functions Ordinals Table        0x24
};
//------------------------------------------------------------------------------------------------------------
struct SImpDir   //  IMPORT_DESC
{
 uint32  LookUpTabRVA;     // 0x00
 uint32  TimeDateStamp;    // 0x04
 uint32  ForwarderChain;   // 0x08
 uint32  ModuleNameRVA;    // 0x0C
 uint32  AddressTabRVA;    // 0x10
};
//------------------------------------------------------------------------------------------------------------
struct SSecHdr   // SECTION_HEADER
{
 char    SectionName[8];        // 00
 uint32  VirtualSize;           // 08
 uint32  VirtualOffset;         // 0C   // SectionRva
 uint32  PhysicalSize;          // 10
 uint32  PhysicalOffset;        // 14
 uint32  PtrToRelocations;      // 18
 uint32  PtrToLineNumbers;      // 1C
 uint16  NumOfRelocations;      // 20
 uint16  NumOfLineNumbers;      // 22
 uint32  Characteristics;       // 24
};
//------------------------------------------------------------------------------------------------------------
struct SDbgHdr   // DEBUG_DIR
{
 uint32  Characteristics;
 uint32  TimeDateStamp;
 uint16  MajorVersion;
 uint16  MinorVersion;
 uint32  Type;
 uint32  SizeOfData;
 uint32  AddressOfRawData;
 uint32  PointerToRawData;
};
//------------------------------------------------------------------------------------------------------------
struct SRelocDesc   // RELOCATION_DESC  // Max for 4k page // There may be more than one such block on Relocaton Directory
{
 uint32 BaseRVA;
 uint32 BlkSize;
 struct
  {
   uint16 Offset : 12;
   uint16 Type   : 4;
  }Records[0];

 uint Count(void){return (this->BlkSize - sizeof(SRelocDesc)) / sizeof(uint16);}
};
//------------------------------------------------------------------------------------------------------------
struct SRsrcDirEntry       //   _IMAGE_RESOURCE_DIRECTORY_ENTRY
{
 union
  {
   struct
    {
     uint32 NameOffset   : 31;
     uint32 NameIsString : 1;
    };
   uint32 Name;
   uint16 Id;
  };
 union
  {
   uint32 OffsetToData;
   struct
    {
     uint32 OffsetToDirectory : 31;
     uint32 DataIsDirectory   : 1;
    };
  };
};
//------------------------------------------------------------------------------------------------------------
struct SRsrcDir    //RESOURCE_DIR
{
 uint32  Characteristics;
 uint32  TimeDateStamp;
 uint16  MajorVersion;
 uint16  MinorVersion;
 uint16  NumberOfNamedEntries;
 uint16  NumberOfIdEntries;
 SRsrcDirEntry DirectoryEntries[];    // Difined in WINNT.H    // IMAGE_RESOURCE_DIRECTORY_ENTRY
};
//------------------------------------------------------------------------------------------------------------
struct SImportByName
{
 uint16  Hint;
 uint8   Name[1];
};
struct SImportThunk
{
 PHT Value;	   // ForwarderString; Function; Ordinal; AddressOfData;
};
//------------------------------------------------------------------------------------------------------------
struct SRichRec
{
 uint16  Ver;    // MinVer
 uint16  PId;    // Product Identifier
 uint32  Cntr;   // Counter of what?
};
#pragma pack( pop )
//============================================================================================================
//
//------------------------------------------------------------------------------------------------------------
_finline static SWinHdr* GetWinHdr(void* Base)
{
 return (SWinHdr*)&(((uint8*)Base)[((SDosHdr*)Base)->OffsetHeaderPE]);
}
//------------------------------------------------------------------------------------------------------------
// NOTE: Do not expect that all PE image pages will be allocated
static bool IsValidHeaderPE(void* Base)
{
 SDosHdr* DosHdr  = (SDosHdr*)Base;
 if((DosHdr->FlagMZ != SIGN_MZ))return false;
 auto WinHdr = GetWinHdr(Base);  //  GetWinHdr<PTRCURRENT>(Base);  // (SWinHdr<PTRCURRENT>*)&(((uint8*)Base)[DosHdr->OffsetHeaderPE]);
 if((WinHdr->FlagPE != SIGN_PE))return false;
 return true;
}
//------------------------------------------------------------------------------------------------------------
static bool IsValidHeaderPE(void* Base, uint Size)
{
 if(Size < sizeof(SDosHdr))return false;
 SDosHdr* DosHdr  = (SDosHdr*)Base;
 if((DosHdr->FlagMZ != SIGN_MZ))return false;
 if(Size < (DosHdr->OffsetHeaderPE + sizeof(SWinHdr)))return false;   // sizeof(SWinHdr<PTRCURRENT>)
 auto WinHdr = GetWinHdr(Base);   //  GetWinHdr<PTRCURRENT>(Base);  // (SWinHdr<PTRCURRENT>*)&(((uint8*)Base)[DosHdr->OffsetHeaderPE]);
 if((WinHdr->FlagPE != SIGN_PE))return false;
 return true;
}
//------------------------------------------------------------------------------------------------------------
static bool IsModuleDLL(void* Base)
{
 auto WinHdr = GetWinHdr(Base);  //  GetWinHdr<PTRCURRENT>(Base);  // (SWinHdr<PTRCURRENT>*)&(((uint8*)Base)[DosHdr->OffsetHeaderPE]);
 return (WinHdr->FileHeader.Flags & 0x2002) == 0x2002;   // 0x2000=IsDll; 0x0002=IsExecutable
}
//------------------------------------------------------------------------------------------------------------
static bool IsModuleX64(void* Base)
{
 auto WinHdr = GetWinHdr(Base);  //  GetWinHdr<PTRCURRENT>(Base);  // (SWinHdr<PTRCURRENT>*)&(((uint8*)Base)[DosHdr->OffsetHeaderPE]);
 return (WinHdr->OptionalHeader.Magic == 0x020B);
}
//---------------------------------------------------------------------------
static uint GetPEImageSize(void* Base)
{
 auto WinHdr = GetWinHdr(Base);  //  GetWinHdr<PTRCURRENT>(Base);  // (SWinHdr<PTRCURRENT>*)&(((uint8*)Base)[DosHdr->OffsetHeaderPE]);
 return WinHdr->OptionalHeader.SizeOfImage;
}
//------------------------------------------------------------------------------------------------------------
static bool IsRvaInSection(SSecHdr* Sec, uint Rva)
{
 if(Sec->VirtualOffset > Rva)return false;
 if((Sec->VirtualOffset+Sec->VirtualSize) <= Rva)return false;
 return true;
}
//------------------------------------------------------------------------------------------------------------
static uint GetSections(void* Base, SSecHdr** FirstSec)
{
 SDosHdr*  DosHdr = (SDosHdr*)Base;
 auto      WinHdr = GetWinHdr(Base);  //  GetWinHdr<PTRCURRENT>(Base);  // (SWinHdr<PTRCURRENT>*)&((uint8*)Base)[DosHdr->OffsetHeaderPE];
 uint      HdrLen = DosHdr->OffsetHeaderPE+WinHdr->FileHeader.HeaderSizeNT+sizeof(SFileHdr)+sizeof(uint32);
 *FirstSec = (SSecHdr*)&((uint8*)Base)[HdrLen];
 return WinHdr->FileHeader.SectionsNumber;
}
//------------------------------------------------------------------------------------------------------------
static uint RvaToFileOffset(void* Base, uint Rva)
{
 SSecHdr* CurSec = nullptr;
 for(uint ctr = 0, total = GetSections(Base, &CurSec);ctr < total;ctr++,CurSec++)
  {
   if(!IsRvaInSection(CurSec, Rva))continue;
   Rva -= CurSec->VirtualOffset;
   if(Rva >= CurSec->PhysicalSize)return 0;  // Not present in the file as physical
   return (CurSec->PhysicalOffset + Rva);
  }
 return 0;
}
//------------------------------------------------------------------------------------------------------------
static uint FileOffsetToRva(void* Base, uint Offset)
{
 SSecHdr* CurSec = nullptr;
 for(uint ctr = 0, total = GetSections(Base, &CurSec);ctr < total;ctr++,CurSec++)
  {
   if((Offset >= CurSec->PhysicalOffset)&&(Offset < (CurSec->PhysicalOffset+CurSec->PhysicalSize)))
	{
	 return ((Offset-CurSec->PhysicalOffset)+CurSec->VirtualOffset);
	}
  }
 return 0;
}
//------------------------------------------------------------------------------------------------------------
// NOTE: Assumed that section offsets is sequential!
static uint SizeOfSections(void* Base, uint MaxSecs=(uint)-1, bool RawSize=false)
{
// DOS_HEADER     *DosHdr = (DOS_HEADER*)ModuleBase;
 auto WinHdr = GetWinHdr(Base);  //  GetWinHdr<PTRCURRENT>(Base);  // (WIN_HEADER<PTRCURRENT>*)&((uint8*)ModuleBase)[DosHdr->OffsetHeaderPE];
// UINT            HdrLen = DosHdr->OffsetHeaderPE+WinHdr->FileHeader.HeaderSizeNT+sizeof(FILE_HEADER)+sizeof(DWORD);
 SSecHdr* CurSec = nullptr;  //(SECTION_HEADER*)&((uint8*)ModuleBase)[HdrLen];
 uint TotalSecs = GetSections(Base, &CurSec);
 uint Offs = 0;
 uint Size = 0;
 if(TotalSecs < MaxSecs)TotalSecs = MaxSecs;
 for(uint ctr=0;ctr < TotalSecs;ctr++)
  {
   if(!Offs)Offs = RawSize?(CurSec[ctr].PhysicalOffset):(CurSec[ctr].VirtualOffset);
   Size += RawSize?(AlignFrwd(CurSec[ctr].PhysicalSize, WinHdr->OptionalHeader.FileAlign)):(AlignFrwd(CurSec[ctr].VirtualSize, WinHdr->OptionalHeader.SectionAlign));
  }
 if(!Offs)Offs = RawSize?WinHdr->OptionalHeader.SizeOfHeaders:(AlignFrwd(WinHdr->OptionalHeader.SizeOfHeaders, WinHdr->OptionalHeader.SectionAlign));
 return Offs + Size;
}
//------------------------------------------------------------------------------------------------------------
static bool GetSectionForAddress(void* Base, void* Address, SSecHdr** ResSec)
{
// DOS_HEADER     *DosHdr = (DOS_HEADER*)ModuleBase;
// WIN_HEADER<PTRCURRENT>  *WinHdr = (WIN_HEADER<PTRCURRENT>*)&((PBYTE)ModuleBase)[DosHdr->OffsetHeaderPE];
// UINT            HdrLen = DosHdr->OffsetHeaderPE+WinHdr->FileHeader.HeaderSizeNT+sizeof(FILE_HEADER)+sizeof(DWORD);
 SSecHdr* CurSec = nullptr;  //SECTION_HEADER *CurSec = (SECTION_HEADER*)&((BYTE*)ModuleBase)[HdrLen];
 uint TotalSecs = GetSections(Base, &CurSec);
 for(uint ctr = 0;ctr < TotalSecs;ctr++,CurSec++)
  {
   if(((uint8*)Address >= ((uint8*)Base + CurSec->VirtualOffset)) && ((uint8*)Address < ((uint8*)Base + CurSec->VirtualOffset + CurSec->VirtualSize)))
    {
     if(ResSec)*ResSec = CurSec;  // NULL for only a presense test
     return true;
    }
  }
 return false;
}
//------------------------------------------------------------------------------------------------------------

// TODO: Remove
static char CharCaseUpper(char Chr)
{
 if((Chr > 0x60)&&(Chr < 0x7B))Chr -= 0x20;
 return Chr;
}
//------------------------------------------------------------------------------
static bool IsCharsEqualIC(char ChrA, char ChrB)
{
 return (CharCaseUpper(ChrA) == CharCaseUpper(ChrB));
}
//------------------------------------------------------------------------------
static bool IsSectionNamesEqual(char *NameA, char *NameB)
{
 for(int ctr=0;ctr<8;ctr++)
  {
   if(!IsCharsEqualIC(NameA[ctr], NameB[ctr]))return false;
   if(!NameA[ctr])break; // Both are ZERO
  }
 return true;
}
//------------------------------------------------------------------------------
static bool IsNamesEqual(char *NameA, char *NameB)
{
 for(;;NameA++,NameB++)
  {
   if(*NameA != *NameB)return false;
   if(!*NameA)break;
  }
 return true;
}
//------------------------------------------------------------------------------
static bool IsNamesEqualIC(char *NameA, char *NameB)
{
 for(;;NameA++,NameB++)
  {
   if(!IsCharsEqualIC(*NameA, *NameB))return false;
   if(!*NameA)break;
  }
 return true;
}
//------------------------------------------------------------------------------------------------------------
// SecName can be an integer section index
static bool GetModuleSection(void* Base, char *SecName, SSecHdr** ResSec)
{
// DOS_HEADER     *DosHdr = (DOS_HEADER*)ModuleBase;
// WIN_HEADER<T>  *WinHdr = (WIN_HEADER<T>*)&((PBYTE)ModuleBase)[DosHdr->OffsetHeaderPE];
// UINT            HdrLen = DosHdr->OffsetHeaderPE+WinHdr->FileHeader.HeaderSizeNT+sizeof(FILE_HEADER)+sizeof(DWORD);
 SSecHdr* CurSec = nullptr;  //SECTION_HEADER *CurSec = (SECTION_HEADER*)&((BYTE*)ModuleBase)[HdrLen];
 uint TotalSecs = GetSections(Base, &CurSec);
 for(uint ctr = 0;ctr < TotalSecs;ctr++,CurSec++)
  {
   if(((uint)SecName == ctr) || IsSectionNamesEqual((char*)&CurSec->SectionName, SecName))
    {
     if(ResSec)*ResSec = CurSec;  // NULL for only a presense test
     return true;
    }
  }
 return false;
}
//------------------------------------------------------------------------------------------------------------
// Based on a last section size, unaligned
static uint CalcModuleSize(void* Base, bool RawSize=false)
{
 SSecHdr* CurSec = nullptr;
 uint TotalSecs = GetSections(Base, &CurSec);
 if(!TotalSecs)return 0;
 SSecHdr* LstSec = CurSec;
 if(RawSize)
  {
   for(uint ctr = 0;ctr < TotalSecs;ctr++,CurSec++)
    {
     if(CurSec->PhysicalOffset > LstSec->PhysicalOffset)LstSec = CurSec;
    }
   return LstSec->PhysicalOffset + LstSec->PhysicalSize;
  }
   else
    {
     for(uint ctr = 0;ctr < TotalSecs;ctr++,CurSec++)
      {
       if(CurSec->VirtualOffset > LstSec->VirtualOffset)LstSec = CurSec;
      }
     return LstSec->VirtualOffset + LstSec->VirtualSize;
    }
}
//------------------------------------------------------------------------------------------------------------
static uint GetExpectedVSize(void* Base)
{
 auto WinHdr = GetWinHdr(Base);   //  GetWinHdr<PTRCURRENT>(Base);
 return WinHdr->OptionalHeader.SizeOfImage;
}
//------------------------------------------------------------------------------------------------------------
static void* GetLoadedModuleEntryPoint(void* Base)
{
 if(!IsValidHeaderPE(Base))return nullptr;
// DOS_HEADER *DosHdr = (DOS_HEADER*)ModuleBase;
 auto WinHdr = GetWinHdr(Base);   //  GetWinHdr<PTRCURRENT>(Base);   //WIN_HEADER<PTRCURRENT> *WinHdr = (WIN_HEADER<PTRCURRENT>*)&(((BYTE*)ModuleBase)[DosHdr->OffsetHeaderPE]);
 if(!WinHdr->OptionalHeader.EntryPointRVA)return nullptr;
 return &((uint8*)Base)[WinHdr->OptionalHeader.EntryPointRVA];
}
//------------------------------------------------------------------------------------------------------------
static uint GetModuleEntryOffset(void* Base, bool Raw)
{
// DOS_HEADER     *DosHdr    = (DOS_HEADER*)ModuleBase;
 auto WinHdr  = GetWinHdr(Base);   //  GetWinHdr<PTRCURRENT>(Base);   //(WIN_HEADER<T>*)&ModuleBase[DosHdr->OffsetHeaderPE];
 if(!WinHdr->OptionalHeader.EntryPointRVA)return 0;
 return (Raw)?(RvaToFileOffset(Base,WinHdr->OptionalHeader.EntryPointRVA)):(WinHdr->OptionalHeader.EntryPointRVA);
}
//------------------------------------------------------------------------------------------------------------
static uint32 CalcChecksumPE(void* Base, uint Size)   // TODO: Rewrite
{
// DOS_HEADER *DosHdr = (DOS_HEADER*)ModuleBase;
 auto WinHdr  = GetWinHdr(Base);   //  GetWinHdr<PTRCURRENT>(Base);  //WIN_HEADER<PTRCURRENT> *WinHdr = (WIN_HEADER<PTRCURRENT>*)&(((BYTE*)ModuleBase)[DosHdr->OffsetHeaderPE]);

 unsigned long long checksum = 0;
 unsigned long long top = 0xFFFFFFFF + 1;
// top++;

 uint32 CSimOffs = (uint8*)&WinHdr->OptionalHeader.FileCheckSum - (uint8*)Base;
 uint8* DataPtr  = (uint8*)Base;
 for(uint idx=0;idx < Size;idx += 4)
  {
   if(idx == CSimOffs)continue;   //Skip "CheckSum" DWORD
   checksum = (checksum & 0xFFFFFFFF) + *(uint32*)&DataPtr[idx] + (checksum >> 32);     // Calculate checksum
   if(checksum > top)checksum = (checksum & 0xFFFFFFFF) + (checksum >> 32);               // TODO: Without 64bit shift
  }
 //Finish checksum
 checksum  = (checksum & 0xFFFF) + (checksum >> 16);
 checksum  = checksum + (checksum >> 16);
 checksum  = checksum & 0xFFFF;
 checksum += Size;
 return checksum;
}
//------------------------------------------------------------------------------------------------------------
/*template<typename T, int Raw=0> static bool TFixRelocations(PBYTE ModuleBase, PBYTE TargetBase)   // Possible not fully correct    <<<<<<<<<<<<<<<<<<<<
{
 DOS_HEADER     *DosHdr    = (DOS_HEADER*)ModuleBase;
 WIN_HEADER<T>  *WinHdr    = (WIN_HEADER<T>*)&ModuleBase[DosHdr->OffsetHeaderPE];
 DATA_DIRECTORY *ReloctDir = &WinHdr->OptionalHeader.DataDirectories.FixUpTable;
 if(!ReloctDir->DirectoryRVA || !ReloctDir->DirectorySize)return false;
 SIZE_T ImageBase = TBaseOfImage<T>(ModuleBase);
 SIZE_T LoadDelta = (SIZE_T)TargetBase - ImageBase;
 PBYTE  RelocPtr  = (PBYTE)&ModuleBase[(Raw)?(RvaToFileOffset(ModuleBase,ReloctDir->DirectoryRVA)):(ReloctDir->DirectoryRVA)];
 for(UINT RelOffs=0;RelOffs < ReloctDir->DirectorySize;)
  {
   RELOCATION_DESC* CurRelBlk = (RELOCATION_DESC*)&RelocPtr[RelOffs];
   PBYTE BasePtr = (PBYTE)&ModuleBase[(Raw)?(RvaToFileOffset(ModuleBase,CurRelBlk->BaseRVA)):(CurRelBlk->BaseRVA)];
   for(UINT RIdx=0,RTotal=CurRelBlk->Count();RIdx < RTotal;RIdx++)
    {
     BYTE Type = CurRelBlk->Records[RIdx].Type;   // NOTE: 'switch()' makes the code Base dependant (Remember Reflective Injection)
     if(Type == IMAGE_REL_BASED_HIGHLOW)     // x32
      {
       PUINT32 Value = (PUINT32)&BasePtr[CurRelBlk->Records[RIdx].Offset];
       if(Raw==1)*Value = UINT32(TargetBase + RvaToFileOffset(ModuleBase, (*Value - ImageBase)));    // Direct x32 address      // Resolve to a Raw address
         else *Value += LoadDelta;
      }
     else if(Type == IMAGE_REL_BASED_DIR64)  // x64
      {
       PUINT64 Value = (PUINT64)&BasePtr[CurRelBlk->Records[RIdx].Offset];
       if(Raw==1)*Value = UINT64(TargetBase + RvaToFileOffset(ModuleBase, (*Value - ImageBase)));    // Direct x32 address      // Resolve to a Raw address
         else *Value += LoadDelta;
      }
//     else if(Type != IMAGE_REL_BASED_ABSOLUTE){DBGMSG("Unsupported reloc type: %u", Type);}    // 11:IMAGE_REL_BASED_HIGH3ADJ(3xWORD) and 4:IMAGE_REL_BASED_HIGHADJ(2xWORD)  // Can`t log if self relocating
    }
   RelOffs += CurRelBlk->BlkSize;
  }
 return true;
} */
//------------------------------------------------------------------------------------------------------------
static char* GetExpModuleName(void* Base, bool Raw)  // Only if an Export section is present     // TODO: Optional Pointer validation mode
{
 SDataDir* ExportDir = (IsModuleX64(Base))?((SDataDir*)&NPE64::GetWinHdr(Base)->OptionalHeader.DataDirectories.ExportTable):((SDataDir*)&NPE32::GetWinHdr(Base)->OptionalHeader.DataDirectories.ExportTable);
 if(!ExportDir->DirectoryRVA)return nullptr;
 SExpDir* Export     = (SExpDir*)&((uint8*)Base)[ExportDir->DirectoryRVA];
 return &((char*)Base)[(Raw)?(RvaToFileOffset(Base,Export->NameRVA)):(Export->NameRVA)];
}
//------------------------------------------------------------------------------------------------------------
static void* ModuleAddressToBase(void* Addr)   // NOTE: Will find a PE header or crash trying:)    // Not all module sections may be present in memory (discarded)
{
 uint8* Base = (uint8*)(((size_t)Addr) & (size_t)~0xFFFF);   // All modules are 64k aligned by loader
 while(!IsValidHeaderPE(Base))Base -= 0x10000;  // !MmIsAddressValid(Base) ? IsBadReadPtr(Base,sizeof(DOS_HEADER)) // Why not all pages of ntoskrnl.exe are available on x64?
 //DBGMSG("Found a module base: %p", Base);
 return Base;
}

//------------------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------------------
};

using NPE   = NFMTPE<uint>;
using NPE32 = NFMTPE<uint32>;
using NPE64 = NFMTPE<uint64>;
//============================================================================================================


