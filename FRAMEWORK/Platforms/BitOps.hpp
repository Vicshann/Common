
#pragma once

// NOTE: Duplicates in ArbiNum
//===========================================================================
// https://github.com/nemequ/portable-snippets
//
// https://en.cppreference.com/w/cpp/numeric/countl_zero

// Count Leading Zeros
template<typename T> constexpr static inline int clz(T Num)
{
 return 0;
}
//------------------------------------------------------------------------------------
// Count Trailing Zeros
//
template<typename T> constexpr static inline int ctz(T Num)
{
#ifdef REAL_MSVC
 unsigned long Index;   // Is it changed by _BitScanForward in case of Zero input? If not we can store (sizeof(Num)*8) in it
 unsigned char res;
 if constexpr (sizeof(T) > sizeof(long))
  {
   if constexpr (__has_builtin(_BitScanForward64))res = _BitScanForward64(&Index, (unsigned long long)Num);    // ARM and AMD64
    else
     {
      res = _BitScanForward(&Index, (unsigned long)Num)
      if(!res)res = _BitScanForward(&Index, (unsigned long)(Num >> 32));
      Index += 32;
     }
  }
   else res = _BitScanForward(&Index, (unsigned long)Num);
 if(res)return Index;    // Found 1 at some position
  else return (sizeof(Num)*8);  // Num is zero, all bits is zero
#else
 if constexpr (sizeof(T) > sizeof(long))return  __builtin_ctzll((unsigned long long)Num);  // X64 CPUs only?
   else return __builtin_ctz((unsigned long)Num);
#endif
/* else   // TODO: optimize?
  {
    int i = 0;
	for(;!(Num & 1u); Num >>= 1, i++);
	return i;
  }
 return 0; */
}
//------------------------------------------------------------------------------------
// https://stackoverflow.com/questions/3849337/msvc-equivalent-to-builtin-popcount
// Counts the number of 1 bits (population count) in a 16-, 32-, or 64-bit unsigned integer.
//
template<typename T> constexpr static inline int PopCnt(T Num)
{
 if constexpr (__has_builtin(__builtin_popcount) && (sizeof(T) <= sizeof(long)))
  {
   return __builtin_popcount((unsigned long)Num);
  }
 else if constexpr (__has_builtin(__builtin_popcountll))
  {
   return __builtin_popcountll((unsigned long long)Num);
  }
 else // UNTESTED!!!  // up to 128 bits    // MSVC does not have '__builtin_popcount' alternative for ARM
 {
  Num = Num - ((Num >> 1) & (T)~(T)0/3);
  Num = (Num & (T)~(T)0/15*3) + ((Num >> 2) & (T)~(T)0/15*3);
  Num = (Num + (Num >> 4)) & (T)~(T)0/255*15;
  return (T)(Num * ((T)~(T)0/255)) >> (sizeof(T) - 1) * 8;  // 8 is number of bits in char
 }
}
//---------------------------------------------------------------------------
template <class T> T RevBits(T n)
{
 short bits = sizeof(n) * 8;
 T mask = ~T(0); // equivalent to uint32_t mask = 0b11111111111111111111111111111111;
 while (bits >>= 1)
  {
   mask ^= mask << (bits); // will convert mask to 0b00000000000000001111111111111111;
   n = (n & ~mask) >> bits | (n & mask) << bits; // divide and conquer
  }
 return n;
}

/*
template<typename T> T RevBits( T n )
{
    // we force the passed-in type to its unsigned equivalent, because C++ may
    // perform arithmetic right shift instead of logical right shift, depending
    // on the compiler implementation.
    typedef typename std::make_unsigned<T>::type unsigned_T;
    unsigned_T v = (unsigned_T)n;

    // swap every bit with its neighbor
    v = ((v & 0xAAAAAAAAAAAAAAAA) >> 1)  | ((v & 0x5555555555555555) << 1);

    // swap every pair of bits
    v = ((v & 0xCCCCCCCCCCCCCCCC) >> 2)  | ((v & 0x3333333333333333) << 2);

    // swap every nybble
    v = ((v & 0xF0F0F0F0F0F0F0F0) >> 4)  | ((v & 0x0F0F0F0F0F0F0F0F) << 4);
    // bail out if we've covered the word size already
    if( sizeof(T) == 1 ) return v;

    // swap every byte
    v = ((v & 0xFF00FF00FF00FF00) >> 8)  | ((v & 0x00FF00FF00FF00FF) << 8);
    if( sizeof(T) == 2 ) return v;

    // etc...
    v = ((v & 0xFFFF0000FFFF0000) >> 16) | ((v & 0x0000FFFF0000FFFF) << 16);
    if( sizeof(T) <= 4 ) return v;

    v = ((v & 0xFFFFFFFF00000000) >> 32) | ((v & 0x00000000FFFFFFFF) << 32);

    // explictly cast back to the original type just to be pedantic
    return (T)v;
}
*/

//---------------------------------------------------------------------------
// MSVC:
//unsigned short _byteswap_ushort(unsigned short value);
//unsigned long _byteswap_ulong(unsigned long value);
//unsigned __int64 _byteswap_uint64(unsigned __int64 value);
// GCC:
//int32_t __builtin_bswap32 (int32_t x)
//int64_t __builtin_bswap64 (int64_t x)
//            // TODO: Use refs (L,R val)
// NOTE: Can`t sift, should work on any types AND arrays
//
template<typename T> constexpr _finline static T SwapBytes(T Value)  // Unsafe with optimizations?  // TODO: Expand for basic types
{
 uint8* SrcBytes = (uint8*)&Value;     // TODO: replace the cast with __builtin_bit_cast because it cannot be constexpr if contains a pointer cast
 uint8  DstBytes[sizeof(T)];
 for(uint idx=0;idx < sizeof(T);idx++)DstBytes[idx] = SrcBytes[(sizeof(T)-1)-idx];    // Lets hope it will be optimized to bswap
 return *(T*)&DstBytes;
}
//------------------------------------------------------------------------------------------------------------
// 0: AABBCCDDEEFFGGHH <7,>7 (S-1)  AABBCCDD <3,>3 (S-1)  AABB <1,>1 (S-1)  AA
// 1: HHBBCCDDEEFFGGAA <5,>5 (S-3)  DDBBCCAA <1,>1 (S-3)  BBAA
// 2: HHGGCCDDEEFFBBAA <3,>3 (S-5)  DDCCBBAA
// 3: HHGGFFDDEECCBBAA <1,>1 (S-7)
// 4: HHGGFFEEDDCCBBAA
template<typename T> constexpr _finline static T RevByteOrder(T Value)   // Can be used at compile time
{
 if constexpr (sizeof(T) > 1)
  {
   T Result = ((Value & 0xFF) << ((sizeof(T)-1)*8)) | ((Value >> ((sizeof(T)-1)*8)) & 0xFF);  // Exchange edge 1
   if constexpr (sizeof(T) > 2)
    {
     Result |= ((Value & 0xFF00) << ((sizeof(T)-3)*8)) | ((Value >> ((sizeof(T)-3)*8)) & 0xFF00); // Exchange edge 2
     if constexpr (sizeof(T) > 4)
      {
       Result |= ((Value & 0xFF0000) << ((sizeof(T)-5)*8)) | ((Value >> ((sizeof(T)-5)*8)) & 0xFF0000); // Exchange edge 3
       Result |= ((Value & 0xFF000000) << ((sizeof(T)-7)*8)) | ((Value >> ((sizeof(T)-7)*8)) & 0xFF000000); // Exchange edge 4
      }
    }
   return Result;
  }
 return Value;
}
//------------------------------------------------------------------------------------
//===========================================================================
