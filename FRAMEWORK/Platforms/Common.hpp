
#pragma once

#pragma warning(disable:4800)   // forcing value to bool 'true' or 'false' (performance warning)
   

#ifdef __GNUG__   
#define _ISGCC      // GCC, Clang else MSVC  // What else do you need?

#include <x86intrin.h>         // Needed only for SIMD ?

#else     // MSVC assumed

#include <intrin.h>

#endif

//#define __forceinline __attribute__((always_inline))    // GCC
//#define _X64BIT
#define FINLINE __forceinline              // MSVC
#define FCALLCNV _fastcall

#define ALIGNPTR   sizeof(void*)
#define ALIGNSIMD  16
#define ALIGNPLAIN 1
#define MEMNORELOCATE 0x80000000     // Used with Align argument of allocators to avoid relocation of a block if inplace enlargement is failed

typedef void *PVOID;
typedef unsigned int        UINT, *PUINT;       

typedef signed char         INT8, *PINT8;
typedef signed short        INT16, *PINT16;
typedef signed int          INT32, *PINT32;
typedef signed __int64      INT64, *PINT64;
typedef unsigned char       UINT8, *PUINT8;
typedef unsigned short      UINT16, *PUINT16;
typedef unsigned int        UINT32, *PUINT32;
typedef unsigned __int64    UINT64, *PUINT64;
                                            
#ifdef _WIN64        // MSVC only?
#define _X64BIT
typedef unsigned __int64    UINTPTR, *PUINTPTR;
typedef unsigned __int64    SIZE_T, *PSIZE_T;
typedef unsigned __int64    SIZEP, *PSIZEP;
#else
typedef unsigned long       UINTPTR, *PUINTPTR;
typedef unsigned long       SIZE_T, *PSIZE_T;
typedef unsigned long       SIZEP, *PSIZEP;
#endif

#ifdef _WIN64
#define MAXMEMBLK (((UINT32)-1)+1)    // SIZE_T    // 4GB
#else
#define MAXMEMBLK (~(((unsigned int)-1) >> 1))    // SIZE_T  // 2GB
#endif

#include "Platforms\Windows\PlatWin.h" 

#include "Platforms\Atomics.hpp" 

// Dump here any implementations that should be accessible early and have no personal HPP yet

template<typename T, class U> struct IsSameTypes {enum { value = 0 };};
template<typename T> struct IsSameTypes<T, T> {enum { value = 1 };};

template<typename N> constexpr FINLINE N _fastcall AlignFrwrd(N Value, unsigned int Alignment){return (((Value/Alignment)+((bool)(Value%Alignment)))*Alignment);}
template<typename N> constexpr FINLINE N _fastcall AlignBkwrd(N Value, unsigned int Alignment){return ((Value/Alignment)*Alignment);}
template<typename N> constexpr FINLINE N _fastcall AlignFrwrdPow2(N Value, unsigned int Alignment){return (Value + (Alignment-1)) & ~(Alignment-1);}
template<typename N> constexpr FINLINE N _fastcall AlignBkwrdPow2(N Value, unsigned int Alignment){return (Value & ~(Alignment-1));}

//---------------------------------------------------------------------------

