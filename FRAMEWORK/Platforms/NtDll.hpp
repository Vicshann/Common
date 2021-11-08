
#pragma once

//============================================================================================================
// Only most useful NTDLL functions will go here for now
template<typename PHT> struct NNTDLL  // For members: alignas(sizeof(PHT))
{
using PVOID    = SPTR<void, PHT>;
using HANDLE   = PVOID;
using SIZE_T   = SPTR<uint, PHT>;
using LONG     = int32;
using ULONG    = uint32;
using PSIZE_T  = SIZE_T*;
using PULONG   = ULONG*;
using NTSTATUS = LONG;

// TODO: Allow only SPTR<uint>, SPTR<uint32>, SPTR<uint64>
static NTSTATUS _STDC NtProtectVirtualMemory(HANDLE ProcessHandle, PVOID* BaseAddress, PSIZE_T RegionSize, ULONG NewProtect, PULONG OldProtect);


};

using NT = NNTDLL<uint>;
//============================================================================================================


