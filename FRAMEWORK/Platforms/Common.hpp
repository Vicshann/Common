
#pragma once

#ifndef _COMMON_
#define _COMMON_

#pragma warning(disable:4800)   // forcing value to bool 'true' or 'false' (performance warning)

//#define __forceinline __attribute__((always_inline))    // GCC
#define FINLINE __forceinline              // MSVC

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
typedef unsigned __int64    UINTPTR, *PUINTPTR;
typedef unsigned __int64    SIZE_T, *PSIZE_T;
#else
typedef unsigned long       UINTPTR, *PUINTPTR;
typedef unsigned long       SIZE_T, *PSIZE_T;
#endif


// Dump here any implementations that should be accessible early and  have no personal HPP yet

template<class T, class U> struct IsSameTypes {enum { value = 0 };};
template<class T> struct IsSameTypes<T, T> {enum { value = 1 };};

template<class N> constexpr FINLINE N _fastcall AlignFrwrd(N Value, unsigned int Alignment){return (((Value/Alignment)+((bool)(Value%Alignment)))*Alignment);}
template<class N> constexpr FINLINE N _fastcall AlignBkwrd(N Value, unsigned int Alignment){return ((Value/Alignment)*Alignment);}

//---------------------------------------------------------------------------
#endif
