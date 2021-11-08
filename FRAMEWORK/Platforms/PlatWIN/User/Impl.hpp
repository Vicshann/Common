
#pragma once

//============================================================================================================
namespace NTAPI  // All required syscall stubs    // Name 'NTAPI' causes compilation to fail with CLANG in VisualStudio!
{
 static inline SFuncStub<decltype(NT::NtProtectVirtualMemory)> NtProtectVirtualMemory;


};
//============================================================================================================
namespace NAPI   // On NIX all syscall stubs will be here   // https://docs.oracle.com/cd/E19048-01/chorus4/806-3328/6jcg1bm05/index.html
{
//#include "../POSX.hpp"

// >>>>> MEMORY <<<<<
#include "Impl_Mem.hpp"
// >>>>> NETWORK <<<<<
#include "Impl_Net.hpp"
// >>>>> FILE SYSTEM <<<<<
#include "Impl_FS.hpp"
// >>>>> PROCESSES/THREADS <<<<<
#include "Impl_PT.hpp"
};
//============================================================================================================
//private:
//static inline  decltype(TypeSwitch<IsBigEnd, uint32, uint64>()) Val = 0;
//static inline TSW<IsBigEnd, uint32, uint64>::T Val = 0;
//public:
//============================================================================================================
static int Initialize(void* DescrPtr)
{
// TODO: Validate presence of all required POSX functions implementation?
// SigWithTmplParam<NTSYSAPI::NtProtectVirtualMemory>();
 return 0;
}
//============================================================================================================


