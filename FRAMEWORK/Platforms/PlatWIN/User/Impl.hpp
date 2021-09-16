
#pragma once

#include "../ntdll.hpp"
#include "../ModFmtPE.hpp"
//============================================================================================================
struct NTSYSAPI  // All required syscall stubs  
{
template<class> struct SFuncStub;
template<class TRet, class... TPar> struct SFuncStub<TRet(TPar...)>      // TODO: Hash function name at construction time (constexpr) with randomization if needed(protection)
{
 using TFuncPtr = TRet (*)(TPar...);
 static const int StubSize = 32;   // Let it be copyable with __m256 
 uint8 Stub[StubSize];     // TFuncPtr ptr;  // TReturn (*ptr)(TParameter...);

 _finline TRet operator()(TPar... params){return ((TFuncPtr)&Stub)(params...);}     //return ptr(params...);
};
//------------------------------------------------------------------------------------------------------------

 SFuncStub<decltype(NT::NtProtectVirtualMemory)> NtProtectVirtualMemory;
 

};
//============================================================================================================
struct NPOSX   // On NIX all syscall stubs will be here   // https://docs.oracle.com/cd/E19048-01/chorus4/806-3328/6jcg1bm05/index.html
{
#include "../POSX.hpp"

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
private:  
//static inline  decltype(TypeSwitch<IsBigEnd, uint32, uint64>()) Val = 0;   
//static inline TSW<IsBigEnd, uint32, uint64>::T Val = 0;         
public:
//============================================================================================================
static int Initialize(void)
{
// TODO: Validate presence of all required POSX functions implementation?
// SigWithTmplParam<NTSYSAPI::NtProtectVirtualMemory>();
 return 0;
}
//============================================================================================================


