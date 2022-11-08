
#pragma once

//============================================================================================================
// Only most useful NTDLL functions will go here for now
template<typename PHT> struct NNTDLL  // For members: alignas(sizeof(PHT))
{
using SIZE_T    = PHT; //TSW<sizeof(PHT) == sizeof(uint64), uint64, uint32>::T;    // Use direct type instead of SPTR<uint, PHT> to avoid unnecessary truncation
using PVOID     = SPTR<void, PHT>;    // PHT aligned void* of current native type (32 bit for 32bit build)    // x32: You can assign a x32 void* and read it back but if some x64 code assigned a x64 value to it then you have to read it as UINT64 in your x32 code
//using ADDR      = TSW<sizeof(PHT) == sizeof(void*), SPTR<void, PHT>, SPTR<uint, PHT>>::T;   // Can hold an address of PHT size for current build (Unsigned if PHT is not of native void* size)
using HANDLE    = PVOID;

using LONG      = int32;
using ULONG     = uint32;
using DWORD     = uint32;
using NTSTATUS  = LONG;
using ULONG_PTR = SIZE_T;
// All pointers must be aligned to PHT size to make a correct stack frame
using PSIZE_T   = SPTR<SIZE_T, PHT>;  //SIZE_T*;
using PULONG    = SPTR<ULONG, PHT>;   //ULONG*;
using PPVOID    = SPTR<PVOID, PHT>;
using PHANDLE   = SPTR<HANDLE, PHT>;

using ACCESS_MASK = DWORD;

//============================================================================================================
//                                          CORE FUNCTIONS
//============================================================================================================

enum EMFlags
{
// Memory protection flags
 PAGE_NOACCESS          = 0x00000001,     
 PAGE_READONLY          = 0x00000002,     
 PAGE_READWRITE         = 0x00000004,     
 PAGE_WRITECOPY         = 0x00000008,     
 PAGE_EXECUTE           = 0x00000010,     
 PAGE_EXECUTE_READ      = 0x00000020,     
 PAGE_EXECUTE_READWRITE = 0x00000040,     
 PAGE_EXECUTE_WRITECOPY = 0x00000080,     
 PAGE_GUARD             = 0x00000100,    
 PAGE_NOCACHE           = 0x00000200,    
 PAGE_WRITECOMBINE      = 0x00000400,  
   
// Memory type flags (AllocationType)
 MEM_COMMIT             = 0x00001000,     
 MEM_RESERVE            = 0x00002000,     
 MEM_DECOMMIT           = 0x00004000,     
 MEM_RELEASE            = 0x00008000,     
 MEM_FREE               = 0x00010000,     
 MEM_PRIVATE            = 0x00020000,     
 MEM_MAPPED             = 0x00040000,     
 MEM_RESET              = 0x00080000,     
 MEM_TOP_DOWN           = 0x00100000,     
 MEM_WRITE_WATCH        = 0x00200000,     
 MEM_PHYSICAL           = 0x00400000,     
 MEM_ROTATE             = 0x00800000,     
 MEM_LARGE_PAGES        = 0x20000000,     
 MEM_4MB_PAGES          = 0x80000000,  
   
// Mapped section types
 SEC_FILE               = 0x00800000,     
 SEC_IMAGE              = 0x01000000,     
 SEC_PROTECTED_IMAGE    = 0x02000000,  
 SEC_RESERVE            = 0x04000000,     
 SEC_COMMIT             = 0x08000000,     
 SEC_NOCACHE            = 0x10000000,     
 SEC_WRITECOMBINE       = 0x40000000,     
 SEC_LARGE_PAGES        = 0x80000000, 
    
 MEM_IMAGE              = SEC_IMAGE 
};



// TODO: Allow only SPTR<uint>, SPTR<uint32>, SPTR<uint64>
// NOTE: All pointers must go through SPTR

static NTSTATUS _STDC NtProtectVirtualMemory(HANDLE ProcessHandle, PPVOID BaseAddress, PSIZE_T RegionSize, ULONG NewProtect, PULONG OldProtect);

static NTSTATUS _STDC NtAllocateVirtualMemory(HANDLE ProcessHandle, PPVOID BaseAddress, ULONG_PTR ZeroBits, PSIZE_T RegionSize, ULONG AllocationType, ULONG Protect);

static NTSTATUS _STDC NtFreeVirtualMemory(HANDLE ProcessHandle, PPVOID BaseAddress, PSIZE_T RegionSize, ULONG FreeType);

static NTSTATUS _STDC NtReadVirtualMemory(HANDLE ProcessHandle, PVOID BaseAddress, PVOID Buffer, SIZE_T BufferSize, PSIZE_T NumberOfBytesRead);

static NTSTATUS _STDC NtWriteVirtualMemory(HANDLE ProcessHandle, PVOID BaseAddress, PVOID Buffer, SIZE_T BufferSize, PSIZE_T NumberOfBytesWritten);



struct IO_STATUS_BLOCK
{
 union
  {
   NTSTATUS Status;
   PVOID Pointer;
  };
 ULONG_PTR Information;
};

/*
static NTSTATUS _STDC NtOpenSection(PHANDLE SectionHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes);

static NTSTATUS _STDC NtCreateSection(PHANDLE SectionHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, PLARGE_INTEGER MaximumSize, ULONG SectionPageProtection, ULONG AllocationAttributes, HANDLE FileHandle);

static NTSTATUS _STDC NtMapViewOfSection(HANDLE SectionHandle, HANDLE ProcessHandle, PVOID* BaseAddress, ULONG_PTR ZeroBits, SIZE_T CommitSize, PLARGE_INTEGER SectionOffset, PSIZE_T ViewSize, SECTION_INHERIT InheritDisposition, ULONG AllocationType, ULONG Win32Protect);
 */
};

using NT = NNTDLL<uint>;
//============================================================================================================


