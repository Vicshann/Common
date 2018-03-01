//---------------------------------------------------------------------------

#ifndef FormatPEH
#define FormatPEH

#include <windows.h>
//---------------------------------------------------------------------------
#define ALIGN_FORWARD(Value,Alignment) ((((Value)/(Alignment))+((bool)((Value)%(Alignment))))*(Alignment))
#define ALIGN_BACKWARD(Value,Alignment) (((Value)/(Alignment))*(Alignment))

#define RVATOADDR(SecRva,ModuleBase)  (((long)(ModuleBase))+((long)(SecRva)))
#define ADDRTORVA(SecAddr,ModuleBase) (((long)(SecAddr))-((long)(ModuleBase)))
//---------------------------------------------------------------------------
#define SIGN_MZ 0x5A4D
#define SIGN_PE 0x00004550
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
struct OPTIONAL_HEADER
{
 WORD  Magic;           // 0107-ROM projection;010B-Normal projection   0x18
 WORD  LinkerVer;       // Linker version number                        0x1A
 DWORD CodeSize;        // Sum of sizes all code sections(ordinary one) 0x1C
 DWORD InitDataSize;    // Size of the initialized data                 0x20
 DWORD UnInitDataSize;  // Size of the uninitialized data section (BSS) 0x24
 DWORD EntryPointRVA;   // Address of 1st instruction to be executed    0x28
 DWORD BaseOfCode;      // Address (RVA) of beginning of code section   0x2C
 DWORD BaseOfData;      // Address (RVA) of beginning of data section   0x30
 DWORD ImageBase;       // The *preferred* load address of the file     0x34
 DWORD SectionAlign;    // Alignment of sections when loaded into mem   0x38
 DWORD FileAlign;       // Align. of sections in file(mul of 512 bytes) 0x3C
 DWORD OperSystemVer;   // Version number of required OS                0x40
 DWORD ImageVersion;    // Version number of image                      0x44
 DWORD SubSystemVer;    // Version number of subsystem                  0x48
 DWORD Win32Version;    // Dunno! But I guess for future use.           0x4C
 DWORD SizeOfImage;     // Total size of the PE image in memory         0x50
 DWORD SizeOfHeaders;   // Size of all headers & section table          0x54
 DWORD FileCheckSum;    // Image file checksum                          0x58
 WORD  SubSystem;       // 1-NotNeeded;2-WinGUI;3-WinCON;5-OS2;7-Posix  0x5C
 WORD  FlagsDLL;        // Used to indicate if a DLL image includes EPs 0x5E
 DWORD StackReserveSize;// Size of stack to reserve                     0x60
 DWORD StackCommitSize; // Size of stack to commit                      0x64
 DWORD HeapReserveSize; // Size of local heap space to reserve          0x68
 DWORD HeapCommitSize;  // Size of local heap space to commit           0x6C
 DWORD LoaderFlags;     // Choose Break/Debug/RunNormally(def) on load  0x70
 DWORD NumOfSizesAndRVA;// Length of next DataDirectory array(alw10h)   0x74
 DATA_DIRECTORIES_TABLE DataDirectories; //                             0x78
};
//---------------------------------------------------------------------------
struct WIN_HEADER                // Must be QWORD aligned
{
 DWORD                 FlagPE;         // PE File Signature             0x00
 FILE_HEADER           FileHeader;     // File header                   0x04
 OPTIONAL_HEADER       OptionalHeader; // Optional file header          0x18
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
 char  SectionName[8];
 DWORD VirtualSize;
 DWORD SectionRva;
 DWORD PhysicalSize;
 DWORD PhysicalOffset;
 DWORD PtrToRelocations;
 DWORD PtrToLineNumbers;
 WORD  NumOfRelocations;
 WORD  NumOfLineNumbers;
 DWORD Characteristics;
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
struct RELOCATION_DESC 
{
 union 
  {
   DWORD VirtualAddress;
   DWORD RelocCount;             // Set to the real count when IMAGE_SCN_LNK_NRELOC_OVFL is set
  };
 DWORD SymbolTableIndex;
 WORD  Type;
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
 IMAGE_RESOURCE_DIRECTORY_ENTRY DirectoryEntries[];    // ???????????
};
//---------------------------------------------------------------------------
bool  _stdcall IsValidPEHeader(PVOID Header);

UINT  _stdcall GetEntryPoint(PVOID Header);
void  _stdcall SetEntryPoint(PVOID Header, UINT Entry);
PDWORD _stdcall GetEntryPointOffset(PVOID Header);
void  _stdcall InjLoadLibraryA(LPSTR LibNameToLoad, PVOID AddrInKernel32);
DWORD _stdcall RvaToFileOffset(HANDLE hModuleFile, DWORD ModuleRva, SECTION_HEADER **RvaInSection);
DWORD _stdcall RvaToFileOffsetF(HANDLE hModuleFile, DWORD ModuleRva, SECTION_HEADER *RvaInSection);
DWORD _stdcall FileOffsetToRva(PVOID ModuleInMem, DWORD FileOffset, SECTION_HEADER **OffsetInSection);
DWORD _stdcall FileOffsetToRvaF(HANDLE hModuleFile, DWORD FileOffset, SECTION_HEADER *OffsetInSection);
bool  _stdcall GetSectionForAddress(HMODULE ModulePtr, PVOID Address, SECTION_HEADER **Section);
//---------------------------------------------------------------------------
#endif
