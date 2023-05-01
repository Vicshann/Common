
#pragma once


#ifdef COMP_MSVC 
extern "C" int _fltused = 0;
#endif


// https://wiki.osdev.org/C_PlusPlus
// Mandatory global methods for C++ support (GCC)
extern "C" void __cxa_pure_virtual(void)
{
 // Do nothing or print an error message.
}
//void *__dso_handle = 0;
extern "C" int __cxa_atexit(void (*destructor) (void *), void *arg, void *dso)
{
 return 0;
}

extern "C" void __cxa_finalize(void *f)
{

}

static void _ccall _purecall(void){}  // extern "C" ?  // Visual Studio

//---------------------------------------------------------------------------
/*static void* _ccall memset(void* _Dst, int _Val, NFWK::size_t _Size)   // __cdecl   // TODO: Aligned, SSE by MACRO   // DDUUPPLLIICCAATT
{
 NFWK::size_t ALen = _Size/sizeof(NFWK::size_t);
// NFWK::size_t BLen = _Size%sizeof(NFWK::size_t);
 NFWK::size_t DVal =_Val & 0xFF;               // Bad and incorrect For x32: '(size_t)_Val * 0x0101010101010101;'         // Multiply by 0x0101010101010101 to copy the lowest byte into all other bytes
 if(DVal)
  {
   DVal = _Val | (_Val << 8) | (_Val << 16) | (_Val << 24);
#ifdef ARCH_X64
   DVal |= DVal << 32;
#endif
  }
 for(NFWK::size_t ctr=0;ctr < ALen;ctr++)((NFWK::size_t*)_Dst)[ctr] = DVal;
 for(NFWK::size_t ctr=(ALen*sizeof(NFWK::size_t));ctr < _Size;ctr++)((char*)_Dst)[ctr] = DVal;
 return _Dst;
} */
//---------------------------------------------------------------------------
#ifdef ARCH_X32
using uint64 = NFWK::uint64;
/*extern "C"  uint64 _ccall __umoddi3(uint64 a, uint64 b){return 0;}
extern "C"  uint64 _ccall __udivdi3(uint64 a, uint64 b){return 0;}
//---
extern "C" _used uint64 _ccall __aeabi_uidiv(uint64 a, uint64 b){return 0;}
extern "C" _used uint64 _ccall __aeabi_uldivmod(uint64 a, uint64 b){return 0;}
extern "C" _used uint64 _ccall __aeabi_uidivmod(uint64 a, uint64 b){return 0;}
extern "C" _used uint64 _ccall __aeabi_d2ulz(uint64 a, uint64 b){return 0;}
extern "C" _used uint64 _ccall __aeabi_ul2d(uint64 a, uint64 b){return 0;}
extern "C" _used uint64 _ccall __aeabi_idiv(uint64 a, uint64 b){return 0;} */
#endif
//---------------------------------------------------------------------------
// For tests see https://opensource.apple.com/source/gcc/gcc-5664/gcc/testsuite/gcc.dg/arm-eabi1.c.auto.html
// See optee_os arm32_aeabi_softfloat
// GCC\Clang
#if !defined(CPU_ARM) || !defined(ARCH_X32)


#endif
//---------------------------------------------------------------------------
