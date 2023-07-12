
#pragma once

//#define FWK_RCE_PROTECT

namespace NCFG
{
static constexpr bool InitCon = false;      // Init console (console app)
static constexpr bool IsBigEnd = false;     // Since the Framework is for x86 and ARM its base operation mode is LittleEndian  // __BIG_ENDIAN__ // __BYTE_ORDER == __BIG_ENDIAN
static constexpr bool InitSyscalls = true;  // Initialize syscalls to be ready at compile time, if possible
static constexpr bool VectorizeMemOps = true;
}