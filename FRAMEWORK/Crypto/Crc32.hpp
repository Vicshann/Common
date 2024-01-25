
#pragma once


//---------------------------------------------------------------------------
/*
  Generate a table for a byte-wise 32-bit CRC calculation on the polynomial:
  x^32+x^26+x^23+x^22+x^16+x^12+x^11+x^10+x^8+x^7+x^5+x^4+x^2+x+1.

  Polynomials over GF(2) are represented in binary, one bit per coefficient,
  with the lowest powers in the most significant bit.  Then adding polynomials
  is just exclusive-or, and multiplying a polynomial by x is a right shift by
  one.  If we call the above polynomial p, and represent a byte as the
  polynomial q, also with the lowest power in the most significant bit (so the
  byte 0xb1 is the polynomial x^7+x^3+x+1), then the CRC is (q*x^32) mod p,
  where a mod b means the remainder after dividing a by b.

  This calculation is done using the shift-register method of multiplying and
  taking the remainder.  The register is initialized to zero, and for each
  incoming bit, x^32 is added mod p to the register if the bit is a one (where
  x^32 mod p is p+x^32 = x^26+...+1), and the register is multiplied mod p by
  x (which is shifting right by one and adding x^32 mod p if the bit shifted
  out is a one).  We start with the highest power (least significant bit) of
  q and repeat for all eight bits of q.

  The table is simply the CRC of all possible eight bit values.  This is all
  the information needed to generate CRC's on data a byte at a time for all
  combinations of CRC register values and incoming bytes.
*/
/*local void make_crc_table()
{
  uLong c;
  int n, k;
  uLong poly;            // polynomial exclusive-or pattern 
  // terms of polynomial defining this crc (except x^32):
  static const Byte p[] = {0,1,2,4,5,7,8,10,11,12,16,22,23,26};

  // make exclusive-or pattern from polynomial (0xedb88320L) 
  poly = 0L;
  for (n = 0; n < sizeof(p)/sizeof(Byte); n++)
    poly |= 1L << (31 - p[n]);
 
  for (n = 0; n < 256; n++)
  {
    c = (uLong)n;
    for (k = 0; k < 8; k++)
      c = c & 1 ? poly ^ (c >> 1) : c >> 1;
    crc_table[n] = c;
  }
  crc_table_empty = 0;
}



// https://github.com/Michaelangel007/crc32
template<UINT32 Poly=0xEDB88320> struct SCrc32
{
 UINT32 Table[256];
// The forward polynomial, 0x04C11DB7,
// The reverse polynomial, 0xEDB88320, where the bits are reversed.
//
// Normal initialization checks the top bit and shifts left,
// Reflected initialiation checks the bottom bit and shifts right.
SCrc32(void)
{
  for (UINT i = 0; i < 256; ++i )
  {
    UINT32 v62 = (i >> 1) ^ Poly;
    if ( (i & 1) == 0 )v62 = i >> 1;
    UINT32 v63 = (v62 >> 1) ^ Poly;
    if ( (v62 & 1) == 0 )v63 = v62 >> 1;
    UINT32 v64 = (v63 >> 1) ^ Poly;
    if ( (v63 & 1) == 0 )v64 = v63 >> 1;
    UINT32 v65 = (v64 >> 1) ^ Poly;
    if ( (v64 & 1) == 0 )v65 = v64 >> 1;
    UINT32 v66 = (v65 >> 1) ^ Poly;
    if ( (v65 & 1) == 0 )v66 = v65 >> 1;
    UINT32 v67 = (v66 >> 1) ^ Poly;
    if ( (v66 & 1) == 0 )v67 = v66 >> 1;
    UINT32 v68 = (v67 >> 1) ^ Poly;
    if ( (v67 & 1) == 0 )v68 = v67 >> 1;
    UINT32 v69 = (v68 >> 1) ^ Poly;
    if ( (v68 & 1) == 0 )v69 = v68 >> 1;
    Table[i] = v69;
  }
}

*/
//------------------------------------------------------------------------------
struct CCRC32
{
static uint32 Table(uint8 Idx)
{
static const uint32  crc_table[256] = {
  0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
  0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988, 0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
  0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
  0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
  0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172, 0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
  0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
  0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
  0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924, 0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
  0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
  0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
  0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e, 0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
  0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
  0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
  0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0, 0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
  0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
  0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
  0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a, 0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
  0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
  0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
  0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc, 0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
  0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
  0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
  0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236, 0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
  0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
  0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
  0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38, 0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
  0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
  0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
  0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2, 0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
  0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
  0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
  0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94, 0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d};

 return crc_table[Idx];
}
//------------------------------------------------------------------------------
/*
unsigned int __stdcall CalcAlignedCRC32(unsigned __int8 *aData, signed int aSize)
{
  unsigned __int8 *vData; // edx
  unsigned int ResCRC; // eax
  signed int i; // edi
  unsigned int v5; // esi
  int v6; // ecx
  unsigned int v7; // eax
  unsigned int v8; // eax
  signed int v9; // ecx
  int v10; // edi
  int v11; // esi

  vData = aData;
  ResCRC = 0xFFFFFFFF;                          
  for ( i = aSize; (unsigned __int8)vData & 3; --i )
  {
    if ( i <= 0 )
      break;
    ResCRC = Crc32Tbl[*vData++ ^ (unsigned __int8)ResCRC] ^ (ResCRC >> 8);// Byte CRC 32 loop
  }
  if ( i >= 4 )
  {
    v5 = (unsigned int)i >> 2;
    i -= 4 * ((unsigned int)i >> 2);
    do
    {
      v6 = *(_DWORD *)vData;
      vData += 4;
      v7 = ((v6 ^ ResCRC) >> 8) ^ Crc32Tbl[(unsigned __int8)(v6 ^ ResCRC)];
      v8 = (((v7 >> 8) ^ Crc32Tbl[(unsigned __int8)v7]) >> 8) ^ Crc32Tbl[(unsigned __int8)(BYTE1(v7) ^ LOBYTE(Crc32Tbl[(unsigned __int8)v7]))];
      ResCRC = Crc32Tbl[(unsigned __int8)v8] ^ (v8 >> 8);
      --v5;
    }
    while ( v5 );
  }
  v9 = i;
  v10 = i - 1;
  if ( v9 )
  {
    v11 = v10 + 1;
    do
    {
      ResCRC = Crc32Tbl[*vData++ ^ (unsigned __int8)ResCRC] ^ (ResCRC >> 8);
      --v11;
    }
    while ( v11 );
  }
  return ResCRC;
}
*/
static uint32 ByteCRC32(uint32 crc, uint8 val)
{
 return Table((uint8)crc ^ val) ^ (crc >> 8);
}
//------------------------------------------------------------------------------
static uint32 DoCRC32(uint8* buf, uint len, uint32 crc=0xFFFFFFFF)
{
 while(len--)crc = ByteCRC32(crc, (*buf++));
 return ~crc;
}

};
//---------------------------------------------------------------------------
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
 if constexpr (i < (N-1))return CRC32<poly, N, i + 1>(str, ~ByteCrc32<poly>((uint8)str[i], ~crc));
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
 crc = ~crc;
 for(uint i=0;i < len;i++)crc = ByteCrc32(str[i],crc,poly);
 return ~crc;
}
//------------------------------------------------------------------------------------------------------------
// No constexpr because of casting
_finline static uint32 Crc32(const void* data, uint size, uint32 crc=InitialCrc32, uint32 poly=DefRevPolyCrc32)
{
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
