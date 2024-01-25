
#pragma once


#ifdef COMP_MSVC
extern "C" int _fltused = 0;
#endif


// https://wiki.osdev.org/C_PlusPlus
// Mandatory global methods for C++ support (GCC)
_NOMANGL void __cxa_pure_virtual(void)
{
 // Do nothing or print an error message.
}
//void *__dso_handle = 0;
_NOMANGL int __cxa_atexit(void (*destructor) (void *), void *arg, void *dso)
{
 UNUSED(destructor); UNUSED(arg); UNUSED(dso);
 return 0;
}

_NOMANGL void __cxa_finalize(void *f)
{
 UNUSED(f);
}

_NOMANGL void _ccall _purecall(void){}  // extern "C" ?  // Visual Studio

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
using sint64 = NFWK::sint64;
using uint32 = NFWK::uint32;
using sint32 = NFWK::sint32;

// No DIV on Arm32
#ifdef CPU_ARM
/*extern "C"  uint64 _ccall __umoddi3(uint64 a, uint64 b){return 0;}
extern "C"  uint64 _ccall __udivdi3(uint64 a, uint64 b){return 0;}
//---
extern "C" _used uint64 _ccall __aeabi_uidiv(uint64 a, uint64 b){return 0;}
extern "C" _used uint64 _ccall __aeabi_uldivmod(uint64 a, uint64 b){return 0;}
extern "C" _used uint64 _ccall __aeabi_uidivmod(uint64 a, uint64 b){return 0;}
extern "C" _used uint64 _ccall __aeabi_d2ulz(uint64 a, uint64 b){return 0;}
extern "C" _used uint64 _ccall __aeabi_ul2d(uint64 a, uint64 b){return 0;}
extern "C" _used uint64 _ccall __aeabi_idiv(uint64 a, uint64 b){return 0;} */
//---------------------------------------------------------------------------
//struct dsr {sint32 quot; sint32 rem;};
//struct dur {uint32 quot; uint32 rem;};  // Not working! - A hidden first arg is used

using sint32x2r = sint32 __attribute__ ((vector_size (8)));
using uint32x2r = uint32 __attribute__ ((vector_size (8)));
using uint32x4r = uint32 __attribute__ ((vector_size (16)));  // With the vector type the return value will be stored in R0-R3
//---------------------------------------------------------------------------
extern "C" uint32 _ccall __aeabi_uidiv(uint32 numerator, uint32 denominator)
{
	return NFWK::NMATH::DivModU(numerator, denominator, uint32(0));
}
//---------------------------------------------------------------------------
extern "C" uint32x2r _ccall __aeabi_uidivmod(uint32 numerator, uint32 denominator)
{
 union
 {
  struct
  {
   uint32 q;
   uint32 r;
  } res;
  uint32x2r val;
 } uv;
	uv.res.q = NFWK::NMATH::DivModU(numerator, denominator, uv.res.r);
	return uv.val;
}
//---------------------------------------------------------------------------
extern "C" sint32 _ccall __aeabi_idiv(sint32 numerator, sint32 denominator)
{
 return NFWK::NMATH::DivModS(numerator, denominator, sint32(0));
}
//---------------------------------------------------------------------------
extern "C" sint32x2r _ccall __aeabi_idivmod(sint32 numerator, sint32 denominator)
{
 union
 {
  struct
  {
   sint32 q;
   sint32 r;
  } res;
  uint32x2r val;
 } uv;
	uv.res.q = NFWK::NMATH::DivModS(numerator, denominator, uv.res.r);
	return uv.val;
}
//---------------------------------------------------------------------------
extern "C" uint32x4r _ccall __aeabi_uldivmod(uint64 numerator, uint64 denominator)   // unsigned  // r2,r3 return too, making ABI exception
{
 union
 {
  struct
  {
   uint64 q;
   uint64 r;
  } res;
  uint32x4r val;
 } uv;
	uv.res.q = NFWK::NMATH::DivModU(numerator, denominator, uv.res.r);
	return uv.val;
}
//---------------------------------------------------------------------------
extern "C" uint32x4r _ccall __aeabi_ldivmod(sint64 numerator, sint64 denominator)  // signed  // r2,r3 return too, making ABI exception
{
 union
 {
  struct
  {
   sint64 q;
   sint64 r;
  } res;
  uint32x4r val;
 } uv;
	uv.res.q = NFWK::NMATH::DivModS(numerator, denominator, uv.res.r);
	return uv.val;
}
//---------------------------------------------------------------------------
// https://github.com/OP-TEE/optee_os/blob/master/lib/libutils/isoc/arch/arm/arm32_aeabi_shift.c
union dword
{
	uint64 dw;
	uint32 w[2];
};
//---------------------------------------------------------------------------
extern "C" sint64 _ccall __aeabi_llsl(sint64 a, int shift)  // TODO: Reuse for x86-32
{
 dword dw = { .dw = (uint64)a };
	uint32 hi = dw.w[1];
	uint32 lo = dw.w[0];

	if (shift >= 32) {
		hi = lo << (shift - 32);
		lo = 0;
	} else if (shift > 0) {
		hi = (hi << shift) | (lo >> (32 - shift));
		lo = lo << shift;
	}

	dw.w[1] = hi;
	dw.w[0] = lo;
	return dw.dw;
}
//---------------------------------------------------------------------------
extern "C" sint64 _ccall __aeabi_llsr(sint64 a, int shift)  // TODO: Reuse for x86-32 (move to NMATH)
{
	dword dw = { .dw = (uint64)a };
	uint32 hi = dw.w[1];
	uint32 lo = dw.w[0];

	if (shift >= 32) {
		lo = hi >> (shift - 32);
		hi = 0;
	} else if (shift > 0) {
		lo = (lo >> shift) | (hi << (32 - shift));
		hi = hi >> shift;
	}

	dw.w[1] = hi;
	dw.w[0] = lo;
	return dw.dw;
}
#else // x86

#endif
#endif
//---------------------------------------------------------------------------
// For tests see https://opensource.apple.com/source/gcc/gcc-5664/gcc/testsuite/gcc.dg/arm-eabi1.c.auto.html
// See optee_os arm32_aeabi_softfloat
// GCC\Clang
#if !defined(CPU_ARM) || !defined(ARCH_X32)


#endif
//---------------------------------------------------------------------------
