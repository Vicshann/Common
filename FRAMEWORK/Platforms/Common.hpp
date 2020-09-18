
#pragma once

// NOTE: This module does not declare its own name space

//#pragma warning(disable:4800)   // forcing value to bool 'true' or 'false' (performance warning)

// template<[template-parameter-list]> using [your-alias] = [original-type];

template<typename T> constexpr static auto ChangeSign(T Val=0)  // should be compiled faster than a template specialization?
{
 if constexpr (1 == sizeof(T))return (T(-1) < 0)?((unsigned char)Val):((signed char)Val);    // if constexpr (1 == sizeof(T))      (T(-1) < 0)?(unsigned char(Val)):(char(Val));
 else if constexpr (2 == sizeof(T))return (T(-1) < 0)?((unsigned short)Val):((signed short)Val);
 else if constexpr (4 == sizeof(T))return (T(-1) < 0)?((unsigned int)Val):((signed int)Val);
 else if constexpr (8 == sizeof(T))return (T(-1) < 0)?((unsigned long long)Val):((signed long long)Val);
}

constexpr static bool Is64BitBuild(void){return sizeof(void*) == 8;}   // To be used in constexpr expressions instead of __amd64__ macro

namespace NGenericTypes   // You should do 'using' for it yourselves if you want to bring these types to global name space
{
#ifdef FWK_DEBUG
 static_assert(1 == sizeof(unsigned char), "Unsupported size of char!");
 static_assert(2 == sizeof(unsigned short), "Unsupported size of short!");
 static_assert(4 == sizeof(unsigned int), "Unsupported size of int!");
 static_assert(8 == sizeof(unsigned long long), "Unsupported size of int64!");
 static_assert(4 == sizeof(float), "Unsupported size of float!");
 static_assert(8 == sizeof(double), "Unsupported size of double!");
#endif

 using achar  = char;
 using wchar  = wchar_t;   // Different platforms may use different sizes for it

 using uint8  = unsigned char;      // 'char' can be signed or unsigned by default
 using uint16 = unsigned short int;
 using uint32 = unsigned int;       // Expected to be 32bit on all supported platforms  // NOTE: int is 32bit even on x64 platforms, meaning that using 'int' everywhere is not architecture friendly
 using uint64 = unsigned long long;

 using int8   = signed char;
 using int16  = signed short int;
 using int32  = signed int;
 using int64  = signed long long;

 using uint   = decltype(sizeof(void*));   // These 'int' are always platform-friendly (same size as pointer type, replace size_t) // "The result of sizeof and sizeof... is a constant of type std::size_t"
 using sint   = decltype(ChangeSign(uint(1)));

 // Add definitions for floating point types?
};

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

// Dump here any implementations that should be accessible early and have no personal HPP yet

template<typename T, class U> struct Is_Same_Types {enum { value = 0 };};
template<typename T> struct Is_Same_Types<T, T> {enum { value = 1 };};

template<typename T> struct Is_Pointer { static const bool value = false; };
template<typename T> struct Is_Pointer<T*> { static const bool value = true; };

template<typename N> constexpr FINLINE N _fastcall AlignFrwrd(N Value, unsigned int Alignment){return (((Value/Alignment)+((bool)(Value%Alignment)))*Alignment);}
template<typename N> constexpr FINLINE N _fastcall AlignBkwrd(N Value, unsigned int Alignment){return ((Value/Alignment)*Alignment);}
template<typename N> constexpr FINLINE N _fastcall AlignFrwrdPow2(N Value, unsigned int Alignment){return (Value + (Alignment-1)) & ~(Alignment-1);}
template<typename N> constexpr FINLINE N _fastcall AlignBkwrdPow2(N Value, unsigned int Alignment){return (Value & ~(Alignment-1));}
*/
//----  --------------------------------------------------------------------------------------------------------

