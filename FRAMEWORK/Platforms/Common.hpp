
#pragma once

// NOTE: This module does not declare its own name space
// NOTE: The Framework is not compatible with MSVC compiler because it uses some GCC style assebbler

//#pragma warning(disable:4800)   // forcing value to bool 'true' or 'false' (performance warning)

// template<[template-parameter-list]> using [your-alias] = [original-type];

 
// MSVC(_MSC_VER), CLANG(__clang__), GCC(__GNUC__), ICC(__INTEL_COMPILER), ICX(__INTEL_LLVM_COMPILER)   
//------------------------------------------------------------------------------------------------------------

#ifdef __GNUC__              // CLANG defines it too
#define COMP_AGCC __GNUC__   // May be GCC, ICC, or something else
#endif
#ifdef _MSC_VER
#define COMP_MSVC _MSC_VER
#undef COMP_AGCC
#endif
#ifdef __clang__              // Must be last here!
#define COMP_CLNG __clang__   // May define __GNUC__ or _MSC_VER
#undef COMP_AGCC
#endif


#ifdef COMP_MSVC                              // __has_cpp_attribute
#define  _finline __forceinline               // At least '/Ob1' is still required
#define  _naked __declspec(naked)
#else
#define  _finline __attribute__((always_inline))    // __attribute__((flatten)) is also useful
#define  _naked   __attribute__((naked))
#endif

// These three work at a call site 
#define SRC_LINE __builtin_LINE()
#define SRC_FILE __builtin_FILE()        // Full path included
#define SRC_FUNC __builtin_FUNCTION()    // Only the name itself, no arguments or a return type

//------------------------------------------------------------------------------------------------------------
template<typename T> consteval static auto ChangeSign(T Val=0)  // Should be compiled faster than a template specialization?
{
 if constexpr (T(-1) < T(0))   // IsSigned
  {
   if constexpr (1 == sizeof(T))return (unsigned char)Val;  
   else if constexpr (2 == sizeof(T))return (unsigned short)Val;
   else if constexpr (4 == sizeof(T))return (unsigned int)Val;
   else if constexpr (8 == sizeof(T))return (unsigned long long)Val;
  }
   else
    {
     if constexpr (1 == sizeof(T))return (signed char)Val;    
     else if constexpr (2 == sizeof(T))return (signed short)Val;
     else if constexpr (4 == sizeof(T))return (signed int)Val;
     else if constexpr (8 == sizeof(T))return (signed long long)Val;
    }  
}

constexpr static bool Is64BitBuild(void){return sizeof(void*) == 8;}   // To be used in constexpr expressions instead of __amd64__ macro
//------------------------------------------------------------------------------------------------------------
namespace NGenericTypes   // You should do 'using' for it yourselves if you want to bring these types to global name space
{
#ifdef FWK_DEBUG
 static_assert(1 == sizeof(unsigned char), "Unsupported size of char!");
 static_assert(2 == sizeof(unsigned short), "Unsupported size of short!");
 static_assert(4 == sizeof(unsigned int), "Unsupported size of int!");
 static_assert(8 == sizeof(unsigned long long), "Unsupported size of int64!");
 static_assert(4 == sizeof(float), "Unsupported size of float!");
 static_assert(8 == sizeof(double), "Unsupported size of double!");
 static_assert(sizeof(void*) == sizeof(decltype(sizeof(void*))), "Unsupported size of size_t!");
#endif

/* https://unix.org/version2/whatsnew/lp64_wp.html
Datatype	LP64	ILP64	LLP64	ILP32	LP32
char	    8	    8	    8	    8	    8
short	    16	    16	    16	    16	    16
_int32		32			
int	        32	    64	    32	    32	    16
long	    64	    64	    32	    32	    32
long long			64		
pointer	    64	    64	    64	    32	    32
*/

 using achar  = char;      // Since C++11: u8"Helloto define a UTF-8 string of chars
 using wchar  = wchar_t;   // Different platforms may use different sizes for it
 using charb  = char8_t;   // u8"" // cannot be signed or unsigned
 using charw  = char16_t;  // u""  // cannot be signed or unsigned
 using chard  = char32_t;  // U""  // cannot be signed or unsigned

 using uint8  = unsigned char;      // 'char' can be signed or unsigned by default
 using uint16 = unsigned short int;
 using uint32 = unsigned int;       // Expected to be 32bit on all supported platforms  // NOTE: int is 32bit even on x64 platforms, meaning that using 'int' everywhere is not architecture friendly
 using uint64 = unsigned long long; // 'long long unsigned int' or 'long unsigned int' ???  // See LP64, ILP64, ILP32 data models on different architectures

 using int8   = signed char;
 using int16  = signed short int;
 using int32  = signed int;
 using int64  = signed long long;   // __int64_t

 using uint   = decltype(sizeof(void*));   // These 'int' are always platform-friendly (same size as pointer type, replace size_t) // "The result of sizeof and sizeof... is a constant of type std::size_t"
 using sint   = decltype(ChangeSign(sizeof(void*)));

 // Add definitions for floating point types?
};

using namespace NGenericTypes;   // For 'Framework.hpp'   // You may do the same in your code if you want
//------------------------------------------------------------------------------------------------------------

/*
#ifdef __GNUG__
#define _ISGCC      // GCC, Clang else MSVC  // What else do you need?

#include <x86intrin.h>         // Needed only for SIMD ?

#else     // MSVC assumed

#include <intrin.h>

#endif
*/
//#define __forceinline __attribute__((always_inline))    // GCC
//#define _X64BIT
/*#define FINLINE __forceinline              // MSVC


#ifdef _WIN64
#define MAXMEMBLK (((UINT32)-1)+1)    // SIZE_T    // 4GB
#else
#define MAXMEMBLK (~(((unsigned int)-1) >> 1))    // SIZE_T  // 2GB
#endif
*/
/*
#include "Platforms\Windows\PlatWin.h"

#include "Platforms\Atomics.hpp"

// <<< Dump here any implementations that should be accessible early and have no personal HPP yet >>>

template<typename T, class U> struct Is_Same_Types {enum { value = 0 };};
template<typename T> struct Is_Same_Types<T, T> {enum { value = 1 };};

template<typename T> struct Is_Pointer { static const bool value = false; };
template<typename T> struct Is_Pointer<T*> { static const bool value = true; };

template<typename N> constexpr FINLINE N _fastcall AlignFrwrd(N Value, unsigned int Alignment){return (((Value/Alignment)+((bool)(Value%Alignment)))*Alignment);}
template<typename N> constexpr FINLINE N _fastcall AlignBkwrd(N Value, unsigned int Alignment){return ((Value/Alignment)*Alignment);}
template<typename N> constexpr FINLINE N _fastcall AlignFrwrdPow2(N Value, unsigned int Alignment){return (Value + (Alignment-1)) & ~(Alignment-1);}
template<typename N> constexpr FINLINE N _fastcall AlignBkwrdPow2(N Value, unsigned int Alignment){return (Value & ~(Alignment-1));}
*/
//------------------------------------------------------------------------------------------------------------
template<typename T> constexpr _finline static T SwapBytes(T Value)  // Unsafe with optimizations?
{
 uint8* SrcBytes = (uint8*)&Value;     // TODO: replace the cast with __builtin_bit_cast
 uint8  DstBytes[sizeof(T)];
 for(uint idx=0;idx < sizeof(T);idx++)DstBytes[idx] = SrcBytes[(sizeof(T)-1)-idx];
 return *(T*)&DstBytes;
}
//------------------------------------------------------------------------------------------------------------
template<typename T> constexpr _finline static int32 AddrToRelAddr(T CmdAddr, unsigned int CmdLen, T TgtAddr){return -((CmdAddr + CmdLen) - TgtAddr);}         // x86 only?
template<typename T> constexpr _finline static T     RelAddrToAddr(T CmdAddr, unsigned int CmdLen, int32 TgtOffset){return ((CmdAddr + CmdLen) + TgtOffset);}  // x86 only?

template <typename T> constexpr _finline static T RotL(T Value, unsigned int Shift){constexpr unsigned int MaxBits = sizeof(T) * 8U; return (Value << Shift) | (Value << ((MaxBits - Shift)&(MaxBits-1)));}
template <typename T> constexpr _finline static T RotR(T Value, unsigned int Shift){constexpr unsigned int MaxBits = sizeof(T) * 8U; return (Value >> Shift) | (Value << ((MaxBits - Shift)&(MaxBits-1)));}

template<typename N, typename M> constexpr _finline static M NumToPerc(N Num, M MaxVal){return (((Num)*100)/(MaxVal));}               // NOTE: Can overflow!
template<typename P, typename M> constexpr _finline static M PercToNum(P Per, M MaxVal){return (((Per)*(MaxVal))/100);}               // NOTE: Can overflow!          

template<class N, class M> constexpr _finline static M AlignFrwd(N Value, M Alignment){return (Value/Alignment)+(bool(Value%Alignment)*Alignment);}    // NOTE: Slow but works with any Alignment value
template<class N, class M> constexpr _finline static M AlignBkwd(N Value, M Alignment){return (Value/Alignment)*Alignment;}                            // NOTE: Slow but works with any Alignment value

// 2,4,8,16,...
template<typename N> constexpr _finline static bool IsPowerOf2(N Value){return Value && !(Value & (Value - 1));}
template<typename N> constexpr _finline static N AlignP2Frwd(N Value, unsigned int Alignment){return (Value+((N)Alignment-1)) & ~((N)Alignment-1);}    // NOTE: Result is incorrect if Alignment is not power of 2
template<typename N> constexpr _finline static N AlignP2Bkwd(N Value, unsigned int Alignment){return Value & ~((N)Alignment-1);}                       // NOTE: Result is incorrect if Alignment is not power of 2
//------------------------------------------------------------------------------------------------------------
