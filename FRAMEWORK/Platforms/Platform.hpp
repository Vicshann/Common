
#pragma once

#include "Syscall.hpp"    // Cannot be in class/struct or no constexpr constructors will be possible ("undefined constructor cannot be used in a constant expression")

struct NPTM    // Can`t be 'struct' if we want that stubs go in the real executable '.text'  section instead of a data section with the same name
{
// Everything below may require to reference each other and SAPI+NAPI
#include "Atomics.hpp"
#include "LogErr.hpp"       // Thish should be visible everywhere
#include "Utils.hpp"        // Anything that doesn`t have a separate HPP and still doesn`t use any of system API
#include "ModFmtMachO.hpp"
#include "ModFmtELF.hpp"
#include "ModFmtPE.hpp"
#include "Threads.hpp"     // Thread definitions for "StartInfo.hpp"
#include "NtDll.hpp"
#include "POSIX.hpp"
#include "TZif.hpp"

// Anything that may have a different set of system API is put here
#if    defined(PLT_EFI)     // SYS_WINDOWS
// TODO
#elif  defined(PLT_UBOOT)   // SYS_LINUX
#include "PlatUBT/Impl.hpp"
#elif  defined(PLT_WEBASM)
// TODO
#elif  defined(PLT_LIN_USR)    // Put BSD support here(similair startup, ELF format)?

#include "PlatLIN/User/Impl.hpp"

#elif  defined(PLT_LIN_KRN)

#elif  defined(PLT_BSD_USR)    // TODO: Try to support as mush BSD variants as possible?

#elif  defined(PLT_BSD_KRN)
// TODO
#elif  defined(PLT_MAC_USR)    // Put BSD support here(similair syscalls)?    // Share some code with BSD or redeclare everything?

#include "PlatMAC/PlatDef.hpp"
#include "StartInfo.hpp"
#include "PlatMAC/User/Impl.hpp"

#elif  defined(PLT_MAC_KRN)
// TODO
#elif  defined(PLT_WIN_USR)

#include "PlatWIN/User/Impl.hpp"

#elif  defined(PLT_WIN_KRN)
// TODO
#endif

#include "Misc.hpp"
//============================================================================================================

//------------------------------------------------------------------------------------------------------------
// https://justine.lol/ape.html // ???

//static uint32 MyProc(uint val){return val+1;}

/*extern "C"        // GetPageSize
{
PVOID _fastcall AllocMemLL(PVOID Mem, SIZE_T Size, SIZE_T AllocSize, SIZE_T ReserveSize=0, SIZE_T Align=MEMPAGESIZE);
bool  _fastcall FreeMemLL(PVOID Mem, SIZE_T Size=0);

PVOID _fastcall AllocMemHL(PVOID Mem, SIZE_T Size, SIZE_T AllocSize, SIZE_T ReserveSize=0, SIZE_T Align=MEMPAGESIZE);    // EMemAlign is not supported!
bool  _fastcall FreeMemHL(PVOID Mem, SIZE_T Size=0);
}*/
#ifdef DBGBUILD
#include "IntfValidator.hpp"
#endif
};
//------------------------------------------------------------------------------------------------------------
#include "LibC.hpp"  // Defined outside to keep it defined in namespace
//------------------------------------------------------------------------------------------------------------

