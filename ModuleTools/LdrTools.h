//---------------------------------------------------------------------------

#ifndef LdrToolsH
#define LdrToolsH

#include <windows.h>
#include <Tlhelp32.h>
//#include "HDE.h"



void _stdcall FillString(LPSTR Str, BYTE Value);
BYTE _stdcall ByteHashAddress(PVOID Addr);
// GetMappedFileName(
//__readgsdword (IN DWORD Offset);
// NtCurrentTeb()
// #define GetCurrentFiber() (((PNT_TIB)NtCurrentTeb())->FiberData)
// #define PcTeb 0x18   // struct _NT_TIB *Self;   // 18h
// __inline struct _TEB * NtCurrentTeb( void ) { return (struct _TEB *) (ULONG_PTR) __readfsdword (PcTeb); }
//
// #define CONTAINING_RECORD(address, type, field) ((type *)((PCHAR)(address) - (ULONG_PTR)(&((type *)0)->field)))
//---------------------------------------------------------------------------
#define LDRP_HASH_TABLE_SIZE 32
#define LDRP_HASH_MASK       (LDRP_HASH_TABLE_SIZE-1)
#define LDRP_COMPUTE_HASH_INDEX(wch) ( (RtlUpcaseUnicodeChar((wch)) - (WCHAR)'A') & LDRP_HASH_MASK )

typedef LONG NTSTATUS;

typedef struct _LDR_LIST_ENTRY
{
 struct _LDR_DATA_TABLE_ENTRY *Next;  // struct _LDR_LIST_ENTRY *Flink;
 struct _LDR_DATA_TABLE_ENTRY *Prev;  // struct _LDR_LIST_ENTRY *Blink;
} LDR_LIST_ENTRY, *PLDR_LIST_ENTRY, *RESTRICTED_POINTER PRLDR_LIST_ENTRY;

typedef struct _LDR_LIST_ENTRY_LO
{
 struct _LDR_MODULE_ENTRY_LO *Next;
 struct _LDR_MODULE_ENTRY_LO *Prev;
} LDR_LIST_ENTRY_LO, *PLDR_LIST_ENTRY_LO, *RESTRICTED_POINTER PRLDR_LIST_ENTRY_LO;

typedef struct _LDR_LIST_ENTRY_MO
{
 struct _LDR_MODULE_ENTRY_MO *Next;
 struct _LDR_MODULE_ENTRY_MO *Prev;
} LDR_LIST_ENTRY_MO, *PLDR_LIST_ENTRY_MO, *RESTRICTED_POINTER PRLDR_LIST_ENTRY_MO;

typedef struct _LDR_LIST_ENTRY_IO
{
 struct _LDR_MODULE_ENTRY_IO *Next;
 struct _LDR_MODULE_ENTRY_IO *Prev;
} LDR_LIST_ENTRY_IO, *PLDR_LIST_ENTRY_IO, *RESTRICTED_POINTER PRLDR_LIST_ENTRY_IO;


//LIST_ENTRY LdrpHashTable[LDRP_HASH_TABLE_SIZE];


typedef struct _UNICODE_STR
{
 USHORT Length;
 USHORT MaximumLength;
 PWSTR  Buffer;
} UNICODE_STR;
typedef UNICODE_STR *PUNICODE_STR;
typedef const UNICODE_STR *PCUNICODE_STR;
#define UNICODE_NULL ((WCHAR)0) // winnt


//
// Private flags for loader data table entries
//
 
#define LDRP_STATIC_LINK                0x00000002
#define LDRP_IMAGE_DLL                  0x00000004
#define LDRP_LOAD_IN_PROGRESS           0x00001000
#define LDRP_UNLOAD_IN_PROGRESS         0x00002000
#define LDRP_ENTRY_PROCESSED            0x00004000
#define LDRP_ENTRY_INSERTED             0x00008000
#define LDRP_CURRENT_LOAD               0x00010000
#define LDRP_FAILED_BUILTIN_LOAD        0x00020000
#define LDRP_DONT_CALL_FOR_THREADS      0x00040000
#define LDRP_PROCESS_ATTACH_CALLED      0x00080000
#define LDRP_DEBUG_SYMBOLS_LOADED       0x00100000
#define LDRP_IMAGE_NOT_AT_BASE          0x00200000
#define LDRP_COR_IMAGE                  0x00400000
#define LDRP_COR_OWNS_UNMAP             0x00800000
#define LDRP_SYSTEM_MAPPED              0x01000000
#define LDRP_IMAGE_VERIFYING            0x02000000
#define LDRP_DRIVER_DEPENDENT_DLL       0x04000000
#define LDRP_ENTRY_NATIVE               0x08000000
#define LDRP_REDIRECTED                 0x10000000
#define LDRP_NON_PAGED_DEBUG_INFO       0x20000000
#define LDRP_MM_LOADED                  0x40000000
#define LDRP_COMPAT_DATABASE_PROCESSED  0x80000000
//
// Loader Data Table Entry
//
typedef struct _LDR_MODULE
{
 PVOID DllBase;
 PVOID EntryPoint;
 ULONG SizeOfImage;
 UNICODE_STR FullDllName;
 UNICODE_STR BaseDllName;
 ULONG Flags;  // see LDRP_***
 WORD LoadCount;
 WORD TlsIndex;
 union
  {
   LDR_LIST_ENTRY HashLinks; // in LdrpHashTable[]        // !!!!!!!!!!!!!!!!!!!!!!!! Process this, when removing a Dll !!!!!!!!!!!!!!!!!!!!!!!!!
   struct
	{
	 PVOID SectionPointer; // for kernel mode and session images only.
	 ULONG CheckSum;       // for kernel mode images only.
	};
  };
 union
  {
   ULONG TimeDateStamp;
   PVOID LoadedImports; // for kernel mode images only.
  };
 PVOID EntryPointActivationContext;    // Rare! ?????
 PVOID PatchInformation;               // Rare! ?????
} LDR_MODULE, *PLDR_MODULE;
//---------------------------------------------------------------------------
// Usable only with CONTAINING_RECORD macro
typedef struct _LDR_DATA_TABLE_ENTRY
{
 LDR_LIST_ENTRY InLoadOrderLinks;           //
 LDR_LIST_ENTRY InMemoryOrderLinks;         // +- see PEB_LDR_DATA
 LDR_LIST_ENTRY InInitializationOrderLinks; //
 LDR_MODULE     Module;
} LDR_DATA_TABLE_ENTRY, *PLDR_DATA_TABLE_ENTRY;
//---------------------------------------------------------------------------
typedef struct _LDR_MODULE_ENTRY_LO
{
 LDR_LIST_ENTRY_LO InLoadOrderLinks;
 LDR_LIST_ENTRY_MO InMemoryOrderLinks;
 LDR_LIST_ENTRY_IO InInitializationOrderLinks;
 LDR_MODULE     Module;
} LDR_MODULE_ENTRY_LO, *PLDR_MODULE_ENTRY_LO;
//---------------------------------------------------------------------------
typedef struct _LDR_MODULE_ENTRY_MO
{
 LDR_LIST_ENTRY_MO InMemoryOrderLinks;
 LDR_LIST_ENTRY_IO InInitializationOrderLinks;
 LDR_MODULE     Module;
} LDR_MODULE_ENTRY_MO, *PLDR_MODULE_ENTRY_MO;
//---------------------------------------------------------------------------
typedef struct _LDR_MODULE_ENTRY_IO
{
 LDR_LIST_ENTRY_IO InInitializationOrderLinks;
 LDR_MODULE     Module;
} LDR_MODULE_ENTRY_IO, *PLDR_MODULE_ENTRY_IO;
//---------------------------------------------------------------------------
//
// Loader Data stored in the PEB
//
typedef struct _PEB_LDR_DATA
{
 ULONG      Length;          // 48
 BOOLEAN    Initialized;
 PVOID      SsHandle;
 LDR_LIST_ENTRY_LO InLoadOrderModuleList;
 LDR_LIST_ENTRY_MO InMemoryOrderModuleList;
 LDR_LIST_ENTRY_IO InInitializationOrderModuleList;  // Not including EXE
 PVOID      EntryInProgress;     // Rare! ????
} PEB_LDR_DATA, *PPEB_LDR_DATA;
//---------------------------------------------------------------------------
typedef struct _RTL_DRIVE_LETTER_CURDIR {
  USHORT                  Flags;
  USHORT                  Length;
  ULONG                   TimeStamp;
  UNICODE_STR             DosPath;
} RTL_DRIVE_LETTER_CURDIR, *PRTL_DRIVE_LETTER_CURDIR;

typedef struct _CURDIR
{
	UNICODE_STR DosPath;
	PVOID Handle;
} CURDIR, *PCURDIR;

typedef struct _RTL_USER_PROCESS_PARAMETERS {
  ULONG                   MaximumLength;
  ULONG                   Length;
  ULONG                   Flags;
  ULONG                   DebugFlags;
  PVOID                   ConsoleHandle;
  ULONG                   ConsoleFlags;
  HANDLE                  StandardInput;
  HANDLE                  StandardOutput;
  HANDLE                  StandardError;
  CURDIR                  CurrentDirectory;
  UNICODE_STR          DllPath;
  UNICODE_STR          ImagePathName;
  UNICODE_STR          CommandLine;
  PVOID                   Environment;
  ULONG                   StartingX;
  ULONG                   StartingY;
  ULONG                   CountX;
  ULONG                   CountY;
  ULONG                   CountCharsX;
  ULONG                   CountCharsY;
  ULONG                   FillAttribute;
  ULONG                   WindowFlags;
  ULONG                   ShowWindowFlags;
  UNICODE_STR          WindowTitle;
  UNICODE_STR          DesktopInfo;
  UNICODE_STR          ShellInfo;
  UNICODE_STR          RuntimeData;
  RTL_DRIVE_LETTER_CURDIR CurrentDirectories[0x20];
} RTL_USER_PROCESS_PARAMETERS, *PRTL_USER_PROCESS_PARAMETERS;
//---------------------------------------------------------------------------
typedef VOID (NTAPI *PPEBLOCKROUTINE)(PVOID);
 
typedef PVOID *PPVOID, **PPPVOID;
 
typedef struct _PEB_FREE_BLOCK {
  struct _PEB_FREE_BLOCK  *Next;
  ULONG                   Size;
} PEB_FREE_BLOCK, *PPEB_FREE_BLOCK;
//---------------------------------------------------------------------------
typedef struct _PEB
{
  BOOLEAN                 InheritedAddressSpace;
  BOOLEAN                 ReadImageFileExecOptions;
  BOOLEAN                 BeingDebugged;
  BOOLEAN                 SpareBool;
  HANDLE                  Mutant;
  PVOID                   ImageBaseAddress;
  PPEB_LDR_DATA           Ldr;
  PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
  PVOID                   SubSystemData;
  PVOID                   ProcessHeap;
  PVOID                   FastPebLock;
  PPEBLOCKROUTINE         FastPebLockRoutine;
  PPEBLOCKROUTINE         FastPebUnlockRoutine;
  ULONG                   EnvironmentUpdateCount;
  PPVOID                  KernelCallbackTable;
    union {
        struct {
            PVOID                   EventLogSection;
            PVOID                   EventLog;
        };
        struct {
            PVOID                   SystemReserved[1];
            PVOID                   AltThunkSListPtr32;
        };
    };
  PPEB_FREE_BLOCK         FreeList;
  ULONG                   TlsExpansionCounter;
  PVOID                   TlsBitmap;
  ULONG                   TlsBitmapBits[2];
  PVOID                   ReadOnlySharedMemoryBase;
  PVOID                   ReadOnlySharedMemoryHeap;
  PPVOID                  ReadOnlyStaticServerData;
  PVOID                   AnsiCodePageData;
  PVOID                   OemCodePageData;
  PVOID                   UnicodeCaseTableData;
  ULONG                   NumberOfProcessors;
  ULONG                   NtGlobalFlag;
  ULONG                   Spare2;
  LARGE_INTEGER           CriticalSectionTimeout;
  ULONG                   HeapSegmentReserve;
  ULONG                   HeapSegmentCommit;
  ULONG                   HeapDeCommitTotalFreeThreshold;
  ULONG                   HeapDeCommitFreeBlockThreshold;
  ULONG                   NumberOfHeaps;
  ULONG                   MaximumNumberOfHeaps;
  PPVOID                  *ProcessHeaps;
  PVOID                   GdiSharedHandleTable;
  PVOID                   ProcessStarterHelper;
  PVOID                   GdiDCAttributeList;
  PVOID                   LoaderLock;
  ULONG                   OSMajorVersion;
  ULONG                   OSMinorVersion;
  USHORT                  OSBuildNumber;
  USHORT                  OSCSDVersion;
  ULONG                   OSPlatformId;
  ULONG                   ImageSubsystem;
  ULONG                   ImageSubSystemMajorVersion;
  ULONG                   ImageSubSystemMinorVersion;
  ULONG                   ImageProcessAffinityMask;
  ULONG                   GdiHandleBuffer[34];
  PVOID                   PostProcessInitRoutine; // void (*)();
  ULONG                   TlsExpansionBitmap;
  ULONG                   TlsExpansionBitmapBits[32];
  ULONG                   SessionId;
	ULARGE_INTEGER          AppCompatFlags;
    ULARGE_INTEGER          AppCompatFlagsUser;
    PVOID                   pShimData;
    PVOID                   AppCompatInfo;
    UNICODE_STR          CSDVersion;
    PVOID                   ActivationContextData;
    PVOID                   ProcessAssemblyStorageMap;
    PVOID                   SystemDefaultActivationContextData;
    PVOID                   SystemAssemblyStorageMap;
    ULONG                   MinimumStackCommit;
} PEB, *PPEB;
//---------------------------------------------------------------------------
typedef struct _CLIENT_ID {
	DWORD         UniqueProcess;
	DWORD         UniqueThread;
} CLIENT_ID;

typedef struct _ACTIVATION_CONTEXT_STACK {
	ULONG Flags;
	ULONG NextCookieSequenceNumber;
	PVOID ActiveFrame;
	LIST_ENTRY FrameListCache;
} ACTIVATION_CONTEXT_STACK, *PACTIVATION_CONTEXT_STACK;

typedef struct _GDI_TEB_BATCH {
	ULONG Offset;
	ULONG HDC;
	ULONG Buffer[310];
} GDI_TEB_BATCH, *PGDI_TEB_BATCH;

typedef struct _Wx86ThreadState {
	ULONG* CallBx86Eip;
	PVOID DeallocationCpu;
	BOOLEAN UseKnownWx86Dll;
	CHAR OleStubInvoked;
} Wx86ThreadState, *PWx86ThreadState;

typedef struct _TEB_ACTIVE_FRAME_CONTEXT {
	ULONG Flags;
	PSTR FrameName;
} TEB_ACTIVE_FRAME_CONTEXT, *PTEB_ACTIVE_FRAME_CONTEXT;

typedef struct _TEB_ACTIVE_FRAME {
	ULONG Flags;
	struct _TEB_ACTIVE_FRAME* Previous;
	PTEB_ACTIVE_FRAME_CONTEXT Context;
} TEB_ACTIVE_FRAME, *PTEB_ACTIVE_FRAME;
//---------------------------------------------------------------------------
typedef struct _TEB {
  NT_TIB                  NtTib;
  PVOID                   EnvironmentPointer;
  CLIENT_ID               ClientId;
  PVOID                   ActiveRpcHandle;
  PVOID                   ThreadLocalStoragePointer;
  PPEB                    Peb;
  ULONG                   LastErrorValue;
  ULONG                   CountOfOwnedCriticalSections;
  PVOID                   CsrClientThread;
  PVOID                   Win32ThreadInfo;
  ULONG                   User32Reserved[26];
  ULONG                   UserReserved[5];
  PVOID                   WOW32Reserved;
  ULONG                   CurrentLocale;
  ULONG                   FpSoftwareStatusRegister;
  PVOID                   SystemReserved1[54];
  ULONG                   ExceptionCode;
  ACTIVATION_CONTEXT_STACK  ActivationContextStack;
  UCHAR                   SpareBytes1[24];
  GDI_TEB_BATCH           GdiTebBatch;
  CLIENT_ID               RealClientId;
  PVOID                   GdiCachedProcessHandle;
  ULONG                   GdiClientPID;
  ULONG                   GdiClientTID;
  PVOID                   GdiThreadLocalInfo;
  ULONG                   Win32ClientInfo[62];
  PVOID                   GlDispatchTable[233];
  ULONG                   GlReserved1[29];
  PVOID                   GlReserved2;
  PVOID                   GlSectionInfo;
  PVOID                   GlSection;
  PVOID                   GlTable;
  PVOID                   GlCurrentRC;
  PVOID                   GlContext;
  NTSTATUS                LastStatusValue;
  UNICODE_STR             StaticUnicodeString;
  WCHAR                   StaticUnicodeBuffer[261];
  PVOID                   DeallocationStack;
  PVOID                   TlsSlots[64];
  LIST_ENTRY              TlsLinks;
  PVOID                   Vdm;
  PVOID                   ReservedForNtRpc;
  PVOID                   DbgSsReserved[2];
  ULONG                   HardErrorsAreDisabled;
  PVOID                   Instrumentation[16];
  PVOID                   WinSockData;
  ULONG                   GdiBatchCount;
    UCHAR                   InDbgPrint;
    UCHAR                   FreeStackOnTermination;
    UCHAR                   HasFiberData;
	UCHAR                   IdealProcessor;
  ULONG                   Spare3;
  PVOID                   ReservedForPerf;
  PVOID                   ReservedForOle;
  ULONG                   WaitingOnLoaderLock;
    Wx86ThreadState         Wx86Thread;
    PVOID*                  TlsExpansionSlots;
    ULONG                   ImpersonationLocale;
    ULONG                   IsImpersonating;
	PVOID                   NlsCache;
    PVOID                   pShimData;
    ULONG                   HeapVirtualAffinity;
    PVOID                   CurrentTransactionHandle;
    PTEB_ACTIVE_FRAME       ActiveFrame;
    BOOLEAN                 SafeThunkCall;
    BOOLEAN                 BooleanSpare[3];
} TEB, *PTEB;
//---------------------------------------------------------------------------




PVOID _stdcall SetNewModuleBase(HMODULE CurrentBase, PVOID NewBase);



HMODULE _stdcall MakeFakeModule(IN PWSTR ModuleName, IN HMODULE Module=NULL, IN HMODULE Exclude=NULL, OUT HMODULE *Original=NULL, BOOL WipeHdr=0);
UINT _stdcall CopyModuleWithProtection(HMODULE DstModule, HMODULE SrcModule);
UINT  _stdcall SetThreadsState(DWORD ProcessID, bool ThreadState);
UINT  _stdcall GetMappedModuleSize(HMODULE Module);
HMODULE  _stdcall DuplicateModule(HMODULE Module, PWSTR ModulePath);
BOOL  _stdcall DestroyModuleHeader(HMODULE Module);
UINT _stdcall GetModuleFilePath(HMODULE Module, PWSTR Path);
BOOL _stdcall SetModuleEntryPoint(HMODULE Module, UINT EntryRVA);

BOOL _stdcall RemoveModuleFromLdrList(HMODULE Module);
int  _stdcall PebGetModuleRefCount(HMODULE Module, int NewRef);

//---------------------------------------------------------------------------
#endif
