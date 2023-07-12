
#pragma once

//------------------------------------------------------------------------------------------------------------
// https://crccalc.com/
// https://www.scadacore.com/tools/programming-calculators/online-checksum-calculator/
// https://emn178.github.io/online-tools/crc32.html
// https://github.com/Michaelangel007/crc32
// CRC32 hash: CRC32A("Hello World!")
// Normal table initialization checks the top bit and shifts left,
// Reflected table initialiation checks the bottom bit and shifts right.
// Use polynomial 0x82F63B78 instead of 0xEDB88320 for compatibility with Intel`s hardware CRC32C (SSE 4.2: _mm_crc32_u8) and ARM (ARMv8-A: __crc32d; -march=armv8-a+crc )
/* English dictionary test:
| hash         | collisions | polynomial |
+--------------+------------+------------+
| crc32b       |     44     | 0x04C11DB7 | RFC 1952;   reversed: 0xEDB88320;  reverse of reciprocal: 0x82608EDB
| crc32c       |     62     | 0x1EDC6F41 | Castagnoli; reversed: 0x82F63B78;  reverse of reciprocal: 0x8F6E37A0
  crc32d                      0xA833982B                         0xD419CC15
| crc32k       |     36     | 0x741B8CD7 | Koopmans;   reversed: 0xEB31D82E;  reverse of reciprocal: 0xBA0DC66B
| crc32q       |     54     | 0x814141AB | AIXM;       reversed: 0xD5828281;  reverse of reciprocal: 0xC0A0A0D5
*/

// All Crc32 implementations here are for reversed polynomial
static constexpr uint32 RevPolyCrc32b   = 0xEDB88320;
static constexpr uint32 RevPolyCrc32c   = 0x82F63B78;  // Intel uses this for hardware Crc32
static constexpr uint32 RevPolyCrc32d   = 0xD419CC15;
static constexpr uint32 RevPolyCrc32k   = 0xEB31D82E;
static constexpr uint32 RevPolyCrc32q   = 0xD5828281;
static constexpr uint32 DefRevPolyCrc32 = RevPolyCrc32b;

static constexpr uint32 InitialCrc32    = 0;     // Will be inverted on entry to make CRC updates consistent

//------------------------------------------------------------------------------------------------------------
// Bitwice computation, no table (means no table to look for in your binary)
constexpr _finline static uint32 ByteCrc32(uint32 val, uint32 crc, uint32 poly)
{
 crc = crc ^ val;
 for(uint j=8;j;j--)crc = (crc >> 1) ^ (poly & -(crc & 1));   // TODO: Use some global magic macro to encrypt 'msk'?
 return crc;
}
//------------------------------------------------------------------------------------------------------------
// Each bit condition may be precomputed
template<uint32 poly> constexpr _finline static uint32 ByteCrc32(uint32 val, uint32 crc)
{
 crc = ((crc & 1) != ((val >> 0) & 1)) ? (crc >> 1) ^ poly : crc >> 1;
 crc = ((crc & 1) != ((val >> 1) & 1)) ? (crc >> 1) ^ poly : crc >> 1;
 crc = ((crc & 1) != ((val >> 2) & 1)) ? (crc >> 1) ^ poly : crc >> 1;
 crc = ((crc & 1) != ((val >> 3) & 1)) ? (crc >> 1) ^ poly : crc >> 1;
 crc = ((crc & 1) != ((val >> 4) & 1)) ? (crc >> 1) ^ poly : crc >> 1;
 crc = ((crc & 1) != ((val >> 5) & 1)) ? (crc >> 1) ^ poly : crc >> 1;
 crc = ((crc & 1) != ((val >> 6) & 1)) ? (crc >> 1) ^ poly : crc >> 1;
 crc = ((crc & 1) != ((val >> 7) & 1)) ? (crc >> 1) ^ poly : crc >> 1;
 return crc;
}
//------------------------------------------------------------------------------------------------------------
// Crc32 for 'achar' string literal. Terminating 0 is skipped
template<uint32 poly = DefRevPolyCrc32, sint N, sint i = 0> constexpr _finline static uint32 CRC32(const achar (&str)[N], uint32 crc=InitialCrc32)
{
 if constexpr (i < (N-1))return CRC32<poly, N, i + 1>(str, ~ByteCrc32<poly>(str[i], ~crc));
 else return crc;
}
//------------------------------------------------------------------------------------------------------------
// For const strings
template<uint32 poly = DefRevPolyCrc32> constexpr _finline static uint32 CRC32(const volatile achar* str, uint32 crc=InitialCrc32)   // 'volatile' solves ambiguity with 'const achar (&str)[N]' above
{
 uint32 Val;
 crc = ~crc;
 for(uint i=0;(Val=uint32(str[i]));i++)crc = ByteCrc32(Val,crc,poly);
 return ~crc;
}
//------------------------------------------------------------------------------------------------------------
// For counted const strings
template<uint32 poly = DefRevPolyCrc32> constexpr _finline static uint32 Crc32(const achar* str, uint len, uint32 crc=InitialCrc32)
{
 uint32 Val;
 crc = ~crc;
 for(uint i=0;i < len;i++)crc = ByteCrc32(str[i],crc,poly);
 return ~crc;
}
//------------------------------------------------------------------------------------------------------------
// No constexpr because of casting
_finline static uint32 Crc32(const void* data, uint size, uint32 crc=InitialCrc32, uint32 poly=DefRevPolyCrc32)
{
 uint32 Val;
 crc = ~crc;
 for(uint i=0;i < size;i++)crc = ByteCrc32(((uint8*)data)[i],crc,poly);
 return ~crc;
}

template<uint32 poly = DefRevPolyCrc32> _finline static uint32 Crc32(const void* data, uint size, uint32 crc=InitialCrc32) {return Crc32((achar*)data,size,crc,poly);}
//------------------------------------------------------------------------------------------------------------





// Recursive instances! Simple but 8 times slower! Makes compilation process very slow!  // Is it better to generate a compile time CRC32 table?
/*template <uint32 msk = 0xEDB88320, uint N, uint i = 0> constexpr _finline static uint32 CRC32A(const char (&str)[N], uint32 result = 0xFFFFFFFF)  // Evaluated for each bit   // Only _finline can force it to compile time computation
{
 if constexpr (i >= (N << 3))return ~result;
 else return !str[i >> 3] ? ~result : CRC32A<msk, N, i + 1>(str, (result & 1) != (((unsigned char)str[i >> 3] >> (i & 7)) & 1) ? (result >> 1) ^ msk : result >> 1);
}  */

// constexpr version  0xEB31D82E
/*template<uint32 msk = 0xEDB88320, uint N, uint i = 0> constexpr _finline static uint32 CRC32(const char (&str)[N], uint32 crc=0xFFFFFFFF)  // Unrolling bit hashing makes compilation speed OK again   // Only _finline can force it to compile time computation
{
// int bidx = 0;
// auto ChrCrc = [&](uint32 val) constexpr -> uint32 {return ((val & 1) != (((uint32)str[i] >> bidx++) & 1)) ? (val >> 1) ^ msk : val >> 1; };  // MSVC compiler choked on lambda and failed to inline it after fourth instance of CRC32A
// if constexpr (i < (N-1) )return CRC32A<msk, N, i + 1>(str, ChrCrc(ChrCrc(ChrCrc(ChrCrc(ChrCrc(ChrCrc(ChrCrc(ChrCrc(crc)))))))));
 if constexpr (i < (N-1))   // No way to read str[i] into a const value? // N-1: Skip 1 null char (Always 1, by C++ standard?)
  {
   crc = ((crc & 1) != (((uint32)str[i] >> 0) & 1)) ? (crc >> 1) ^ msk : crc >> 1;
   crc = ((crc & 1) != (((uint32)str[i] >> 1) & 1)) ? (crc >> 1) ^ msk : crc >> 1;
   crc = ((crc & 1) != (((uint32)str[i] >> 2) & 1)) ? (crc >> 1) ^ msk : crc >> 1;
   crc = ((crc & 1) != (((uint32)str[i] >> 3) & 1)) ? (crc >> 1) ^ msk : crc >> 1;
   crc = ((crc & 1) != (((uint32)str[i] >> 4) & 1)) ? (crc >> 1) ^ msk : crc >> 1;
   crc = ((crc & 1) != (((uint32)str[i] >> 5) & 1)) ? (crc >> 1) ^ msk : crc >> 1;
   crc = ((crc & 1) != (((uint32)str[i] >> 6) & 1)) ? (crc >> 1) ^ msk : crc >> 1;
   crc = ((crc & 1) != (((uint32)str[i] >> 7) & 1)) ? (crc >> 1) ^ msk : crc >> 1;
   return CRC32<msk, N, i + 1>(str, crc);
  }
 else return ~crc;
} */
//------------------------------------------------------------------------------------------------------------
// Runtime version
// Because 'msk' is template param it is impossible to encrypt it  // Is it OK to leave it in a executable as is or better pass it as the function argument(right after decryption)?
/*template<uint32 msk = 0xEDB88320> constexpr _finline static uint32 Crc32(const char* Text, uint32 crc = 0xFFFFFFFF)   // Should it be here or moved to some other unit?  // Useful for some small injected pieces of code which do some string search
{
 uint32 Val;
 for(uint i=0;(Val=uint32(Text[i]));i++)
  {
   crc = crc ^ Val;
   for(uint j=8;j;j--)crc = (crc >> 1) ^ (msk & -(crc & 1));   // TODO: Use some global magic macro to encrypt 'msk'?
  }
 return ~crc;
}

template<uint32 msk = 0xEDB88320> constexpr _finline static uint32 Crc32(const unsigned char* Data, uint Size, uint32 crc = 0xFFFFFFFF)   // Should it be here or moved to some other unit?  // Useful for some small injected pieces of code which do some string search
{
 uint32 Val;
 for(uint i=0;Size;Size--,i++)
  {
   crc = crc ^ Data[i];
   for(uint j=8;j;j--)crc = (crc >> 1) ^ (msk & -(crc & 1));   // TODO: Use some global magic macro to encrypt 'msk'?
  }
 return ~crc;
}

_finline static uint32 Crc32v(const void* Data, uint Size, uint32 crc = 0xFFFFFFFF, uint32 msk = 0xEB31D82E)  // Slow but undetectable by precomputed tables
{
 for(uint i=0;i < Size;i++)
  {
   crc = crc ^ ((unsigned char*)Data)[i];
   for(uint j=8;j;j--)crc = (crc >> 1) ^ (msk & -(crc & 1));    // Unroll?
  }
 return ~crc;
}
//------------------------------------------------------------------------------------------------
//static_assert(CRC32A("Hello World!") == 0x1C291CA3);

// TODO: CRC64
//------------------------------------------------------------------------------------------------------------
int TestCrc32(void)
{

}  */
//------------------------------------------------------------------------------------------------------------
