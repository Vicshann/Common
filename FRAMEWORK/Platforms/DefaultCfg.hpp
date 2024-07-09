
#pragma once

// NOTE: All available configs put here
//#define FWK_RCE_PROTECT
// FWK_OLD_UBOOT
//#define FCFG_FORCE_DBGMSG

namespace NCFG
{
static constexpr long MemPageSize = 0;
static constexpr long MemGranSize = 0;
static constexpr bool InitCon = false;      // Init console (console app) at early initialization stage
static constexpr bool IsBigEnd = false;     // Since the Framework is for x86 and ARM its base operation mode is LittleEndian  // __BIG_ENDIAN__ // __BYTE_ORDER == __BIG_ENDIAN
static constexpr bool InitSyscalls = true;  // Initialize syscalls to be ready at compile time, if possible
static constexpr bool VectorizeMemOps = true;
}
