
#pragma once
/*
  Copyright (c) 2020 Victor Sheinmann, Vicshann@gmail.com

  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), 
  to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
  and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. 
*/

//---------------------------------------------------------------------------
#include <Windows.h>
#include <intrin.h>
#include <emmintrin.h>        // for _mm_storeu_si128
#include "ThirdParty\NTDLL\ntdll.h"

#include "Utils.h"

extern char* _cdecl gcvt(double f, size_t ndigit, char* buf);

// CRC32 hash: CRC32A("Hello World!")
// https://github.com/Michaelangel007/crc32
// Use polynomial 0x82F63B78 instead of 0xEDB88320 for compatibility with Intel`s hardware CRC32C (SSE 4.2: _mm_crc32_u8) and ARM (ARMv8-A: __crc32d; -march=armv8-a+crc )
/* English dictionary test:
| hash         | collisions | polynomial |
+--------------+------------+------------+
| crc32b       |     44     | 0x04C11DB7 | RFC 1952;   reversed: 0xEDB88320;  reverse of reciprocal: 0x82608EDB
| crc32c       |     62     | 0x1EDC6F41 | Castagnoli; reversed: 0x82F63B78;  reverse of reciprocal: 0x8F6E37A0
| crc32k       |     36     | 0x741B8CD7 | Koopmans;   reversed: 0xEB31D82E;  reverse of reciprocal: 0xBA0DC66B
| crc32q       |     54     | 0x814141AB | AIXM;       reversed: 0xD5828281;  reverse of reciprocal: 0xC0A0A0D5
*/

// Recursive instances! Simple but 8 times slower! Makes compilation process very slow!  // Is it better to generate a compile time CRC32 table?
/*template <UINT32 msk = 0xEDB88320, SIZE_T N, SIZE_T i = 0> constexpr __forceinline static UINT32 CRC32A(const char (&str)[N], UINT32 result = 0xFFFFFFFF)  // Evaluated for each bit   // Only __forceinline can force it to compile time computation
{
 if constexpr (i >= (N << 3))return ~result;
 else return !str[i >> 3] ? ~result : CRC32A<msk, N, i + 1>(str, (result & 1) != (((unsigned char)str[i >> 3] >> (i & 7)) & 1) ? (result >> 1) ^ msk : result >> 1);
}   */
 
template <UINT32 msk = 0xEDB88320, SIZE_T N, SIZE_T i = 0> constexpr __forceinline static UINT32 CRC32A(const char (&str)[N], UINT32 crc=0xFFFFFFFF)  // Unrolling bit hashing makes compilation speed OK again   // Only __forceinline can force it to compile time computation
{
// int bidx = 0;
// auto ChrCrc = [&](UINT32 val) constexpr -> UINT32 {return ((val & 1) != (((UINT32)str[i] >> bidx++) & 1)) ? (val >> 1) ^ msk : val >> 1; };  // MSVC compiler choked on lambda and failed to inline it after fourth instance of CRC32A 
// if constexpr (i < (N-1) )return CRC32A<msk, N, i + 1>(str, ChrCrc(ChrCrc(ChrCrc(ChrCrc(ChrCrc(ChrCrc(ChrCrc(ChrCrc(crc)))))))));  
 if constexpr (i < (N-1))   // No way to read str[i] into a const value? // N-1: Skip 1 null char (Always 1, by C++ standard?)
  {
   crc = ((crc & 1) != (((UINT32)str[i] >> 0) & 1)) ? (crc >> 1) ^ msk : crc >> 1;
   crc = ((crc & 1) != (((UINT32)str[i] >> 1) & 1)) ? (crc >> 1) ^ msk : crc >> 1;
   crc = ((crc & 1) != (((UINT32)str[i] >> 2) & 1)) ? (crc >> 1) ^ msk : crc >> 1;
   crc = ((crc & 1) != (((UINT32)str[i] >> 3) & 1)) ? (crc >> 1) ^ msk : crc >> 1;
   crc = ((crc & 1) != (((UINT32)str[i] >> 4) & 1)) ? (crc >> 1) ^ msk : crc >> 1;
   crc = ((crc & 1) != (((UINT32)str[i] >> 5) & 1)) ? (crc >> 1) ^ msk : crc >> 1;
   crc = ((crc & 1) != (((UINT32)str[i] >> 6) & 1)) ? (crc >> 1) ^ msk : crc >> 1;
   crc = ((crc & 1) != (((UINT32)str[i] >> 7) & 1)) ? (crc >> 1) ^ msk : crc >> 1;
   return CRC32A<msk, N, i + 1>(str, crc);
  }
 else return ~crc;   
}

// Because 'msk' is template param it is impossible to encrypt it  // Is it OK to leave it in a executable as is or petter pass it as the function argument(right after decryption)?
template <UINT32 msk = 0xEDB88320> __forceinline static UINT32 CRC32(char* Text, UINT32 crc = 0xFFFFFFFF)   // Should it be here or moved to some other unit?  // Useful for some small injected pieces of code which do some string search
{
 UINT32 Val;
 for(SIZE_T i=0;Val=Text[i];i++) 
  {        
   crc = crc ^ Val;
   for(SIZE_T j=8;j;j--)crc = (crc >> 1) ^ (msk & -(crc & 1)); 
  }
 return ~crc;
}

//static_assert(CRC32A("Hello World!") == 0x1C291CA3);

// TODO: CRC64
//==============================================================================

static consteval __forceinline SIZE_T MakeBuildKey(void)
{
 constexpr DWORD DTCrc  = CRC32A(__DATE__ __TIME__);
 constexpr DWORD EncKey =  ((~((unsigned int)(__DATE__[4]) * (unsigned int)(__DATE__[5])) & 0xFF) | ((~((unsigned int)(__TIME__[0]) * (unsigned int)(__TIME__[1])) & 0xFF) << 8) | ((~((unsigned int)(__TIME__[3]) * (unsigned int)(__TIME__[4])) & 0xFF) << 16) | ((~((unsigned int)(__TIME__[6]) * (unsigned int)(__TIME__[7])) & 0xFF) << 24));   // DWORD
 SIZE_T Result = EncKey ^ DTCrc;
 if constexpr(sizeof(SIZE_T) > 4)
  {
   Result <<= 32;
   Result  |= DTCrc;
  }    
 return Result;
}

struct NCMN
{


template<typename T> inline static long  AddrToRelAddr(T CmdAddr, UINT CmdLen, T TgtAddr){return -((CmdAddr + CmdLen) - TgtAddr);}        // x86 only?
template<typename T> inline static T     RelAddrToAddr(T CmdAddr, UINT CmdLen, long TgtOffset){return ((CmdAddr + CmdLen) + TgtOffset);}  // x86 only?

template <typename T> constexpr inline static T RotL(T Value, unsigned int Shift){constexpr unsigned int MaxBits = sizeof(T) * 8U; return (Value << Shift) | (Value >> ((MaxBits - Shift)&(MaxBits-1)));}
template <typename T> constexpr inline static T RotR(T Value, unsigned int Shift){constexpr unsigned int MaxBits = sizeof(T) * 8U; return (Value >> Shift) | (Value << ((MaxBits - Shift)&(MaxBits-1)));}

template<typename N, typename M> inline static M NumToPerc(N Num, M MaxVal){return (((Num)*100)/(MaxVal));}               // NOTE: Can overflow!
template<typename P, typename M> inline static M PercToNum(P Per, M MaxVal){return (((Per)*(MaxVal))/100);}               // NOTE: Can overflow!          

template<class N, class M> constexpr __forceinline static M AlignFrwd(N Value, M Alignment){return ((Value/Alignment)+(bool(Value%Alignment)))*Alignment;}    // NOTE: Slow but works with any Alignment value
template<class N, class M> constexpr __forceinline static M AlignBkwd(N Value, M Alignment){return (Value/Alignment)*Alignment;}                            // NOTE: Slow but works with any Alignment value

// 2,4,8,16,...
template<typename N> constexpr __forceinline static bool IsPowerOf2(N Value){return Value && !(Value & (Value - 1));}
template<typename N> constexpr __forceinline static N AlignP2Frwd(N Value, unsigned int Alignment){return (Value+((N)Alignment-1)) & ~((N)Alignment-1);}    // NOTE: Result is incorrect if Alignment is not power of 2
template<typename N> constexpr __forceinline static N AlignP2Bkwd(N Value, unsigned int Alignment){return Value & ~((N)Alignment-1);}                    // NOTE: Result is incorrect if Alignment is not power of 2

//---------------------------------------------------------------------------
#include "UTF.hpp"
#include "HDE.hpp"
#include "StrUtils.hpp"
#include "FormatPE.hpp"
//#include "BdsCompat.hpp"         // Rarely used. Let it be outside for now
#include "CompileTime.hpp"
#include "InjDllLdr.hpp"
#include "NtDllEx.hpp"
#include "UniHook.hpp"
#include "Patcher.hpp"
#include "ShMemIPC.hpp"
#include "GhostDbg.hpp"

#include "MiniString.h"
#include "FileStream.h"
#include "json.h"
//#include "Base64.hpp"
#include "BaseP2.hpp"

//#ifndef _AMD64_
#include "wow64ext.hpp"    // Must be present even in X64 builds
//#endif
};
//---------------------------------------------------------------------------

typedef NCMN::CJSonItem CJSonItem;
typedef NCMN::CMiniStr  CMiniStr;
typedef NCMN::CJSonItem CJSonItem;

typedef NCMN::NShMem NShMem;
typedef NCMN::NGhDbg NGhDbg;
                              
typedef NCMN::NInjLdr NInjLdr;
typedef NCMN::NUNIHK  NUNIHK;      
typedef NCMN::NNTDLL  NNTDLL;      
typedef NCMN::NPEFMT  NPEFMT; 
typedef NCMN::NSTR    NSTR;
typedef NCMN::NCTM    NCTM;
typedef NCMN::NUTF    NUTF;
typedef NCMN::NSIGP   NSIGP;
typedef NCMN::NSTR    NSTR;
//typedef NCMN::NBase64 NBase64;
typedef NCMN::NBP2::NBase64 NBase64;
typedef NCMN::NBP2 NBP2;
#ifndef _AMD64_
typedef NCMN::NWOW64E NWOW64E;
#endif
//---------------------------------------------------------------------------
