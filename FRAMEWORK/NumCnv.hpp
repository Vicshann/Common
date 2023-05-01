
#pragma once

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#pragma clang diagnostic ignored "-Wimplicit-int-float-conversion"
#pragma clang diagnostic ignored "-Wsign-compare"
#pragma clang diagnostic ignored "-Wsign-conversion"
#pragma clang diagnostic ignored "-Wcast-align"

struct NCNV
{
//------------------------------------------------------------------------------------
enum EFltFmt {ffZeroPad=0x8000, ffSkipZeroFrac=0x4000, ffCommaSep=0x2000, ffUpperCase=0x1000};  // Add this flags to 'afterpoint'
static const int MaxDigit = 19;    // Including an empty position (0)
static const inline uint64 uExp10[MaxDigit] =
{
0,
10,
100,
1000,
10000,
100000,
1000000,
10000000,
100000000,
1000000000,
10000000000,
100000000000,
1000000000000,
10000000000000,
100000000000000,
1000000000000000,
10000000000000000,
100000000000000000,
1000000000000000000,  // Max for INT64
};
//------------------------------------------------------------------------------------
// #define ROUND_2_INT(f) ((int)(f >= 0.0 ? (f + 0.5) : (f - 0.5)))
// MSVC: Compile option '/d2noftol3' will avoid _dtoul3 and _ultod3
// /Qifist switch with the Microsoft compiler. This switch tells the compiler not to change the rounding mode for float-to-int casts.
// Converts a floating point number to string.
// Check for 0?
// Buffer MUST be at least of 50 bytes of size
// Result is NOT placed at beginning of the buffer, instead returned a pointer inside of it
// Fast but only max up to 18 digits after '.' (limitation of UINT64)!  (Stable amount is 15)
// Must produce: 123.1234000   // Add zeroes to 'afterpoint'
//               123.0         // Keep at least one zero, but trim to 'afterpoint'
//               123           // Do not keep zero if integer
//
template<typename T=char> static T* ftoa_simple(double num, size_t afterpoint, T* buf, size_t len, size_t* size)    // TODO: Move to a converter class
{
 static_assert(sizeof(uint64) == sizeof(double), "Sizes mismatch!");
 static const uint64 SpecialMsk = (1ULL << 63);  // 0x8000000000000000
 int  Zeroes;
 bool negative;
 T* ptr = &buf[len-1];   // Will store backwards
 *(ptr--)  = 0;     // Make Optional?  // Skipping ZERO is uselles when we writing the buffer backwards
 if(num < 0){negative = true; num = -num;}  // force it positive // -0.0 is in the case?
   else negative = false;
 int DAfter = afterpoint & 0xFF;
 if(DAfter >= MaxDigit)DAfter = MaxDigit-1;   // Safe check uExp10 array

 uint64 ipart = (uint64)num;    // NOTE: Casting always truncates          // Extract integer part     // Results 0x8000000000000000 if NAN or INF
 if(DAfter)                       // NOTE: Avoid converting 'long long' to 'double' - compiler will use a complicated function with a bunch of overflow checks(inline or external)!
  {
   double  fpartd = num - ipart;     // (double)ipart   // Extract floating part   // At 18 digit after dot getting mistakes
   if(fpartd <  0)fpartd = num - (--ipart);   // (double)(--ipart)  // Fix rounding up, don`t touch FPU rounding modes
   if(fpartd >= 1)fpartd = num - (++ipart);   // (double)(++ipart)  // Fix rounding down              // Results in ipart 0x8000000000000001 if INF

   uint64 fpart = (uint64)(fpartd * uExp10[DAfter]);  // After 15 digit loses some precision    // Results 0x8000000000000000 if NAN or INF
   if(SpecialMsk == fpart)       // Is these check are not FPU specific?
    {
     if(ipart == SpecialMsk) // NAN
      {
       if(size)*size = 3;
       *(unsigned int*)buf = (afterpoint & ffUpperCase)?0x004E414E:0x006E616E;  // 'NAN' or 'nan'
       return buf;
      }
       else if(ipart == (SpecialMsk+1))  // INF
        {
         if(size)*size = 4;
         *buf = negative?'-':'+';
         *(unsigned int*)&buf[1] = (afterpoint & ffUpperCase)?0x00464E49:0x00666E69;  // 'INF' or 'inf'
         return buf;
        }
    }
   if(fpart){Zeroes = 0; for(int ctr=DAfter-1;(fpart < uExp10[ctr]);ctr--)Zeroes++;}  // Count zeroes before dot
     else if(afterpoint & ffZeroPad)Zeroes = DAfter;  // If padding with zeroes allowed
           else Zeroes = 1;    // At least one zero when no fraction part

   if(fpart || !(afterpoint & ffSkipZeroFrac))   // Make fraction part
    {
     if(fpart && !(afterpoint & ffZeroPad)){while(!(fpart % 10u))fpart /= 10u;}  // Skip right zeroes       // NOTE: Make sure that only one idiv is used when optimization is ON
     for(;fpart;ptr--){*ptr = (fpart % 10u) + '0'; fpart /= 10u;}        // NOTE: Make sure that only one idiv is used when optimization is ON
     for(;Zeroes;ptr--,Zeroes--)*ptr = '0';
     *(ptr--) = (afterpoint & ffCommaSep)?(','):('.');   // add a dot
    }
  }

 if(ipart){ for(;ipart;ptr--){*ptr = (ipart % 10u) + '0'; ipart /= 10u;} }   // NOTE: Make sure that only one idiv is used when optimization is ON
   else *(ptr--) = '0';
 if(negative)*ptr = '-';
  else ptr++;
 if(size)*size = (len - (ptr - buf))-1;  // -1 for a terminating zero
 return ptr;
}
//------------------------------------------------------------------------------------
static void FTOA_Test(void)
{
 char* ptr;
 size_t size = 0;
 char Buf[64];
 ptr = ftoa_simple(7.123, 0, (char*)&Buf, sizeof(Buf), &size);
 ptr = ftoa_simple(9.000001, 6, (char*)&Buf, sizeof(Buf), &size);
 ptr = ftoa_simple(9.123, 6, (char*)&Buf, sizeof(Buf), &size);
 ptr = ftoa_simple(9.123, 6|ffZeroPad, (char*)&Buf, sizeof(Buf), &size);
 ptr = ftoa_simple(7.0, 6|ffZeroPad, (char*)&Buf, sizeof(Buf), &size);
 ptr = ftoa_simple(9.0, 6, (char*)&Buf, sizeof(Buf), &size);
 ptr = ftoa_simple(8.0, 6|ffSkipZeroFrac, (char*)&Buf, sizeof(Buf), &size);
 ptr = ftoa_simple(8.1, 6|ffSkipZeroFrac, (char*)&Buf, sizeof(Buf), &size);
 ptr = nullptr;
}
//------------------------------------------------------------------------------------
//====================================================================================
static _finline int HexCharToHalfByte(uint8 CharVal)
{
 if((CharVal >= '0')&&(CharVal <= '9'))return (CharVal - '0');       // 0 - 9
 if((CharVal >= 'A')&&(CharVal <= 'F'))return (CharVal - ('A'-10));  // A - F
 if((CharVal >= 'a')&&(CharVal <= 'f'))return (CharVal - ('a'-10));  // a - f
 return -1;
}
//---------------------------------------------------------------------------
// Output uint16 can be directly written to a string with right half-byte ordering  (little endian)  // TODO: Endianess check
// 0x30: '0' : 0011 0000
// 0x39: '9' : 0011 1001
// 0x41: 'A' : 0100 0001
// 0x46: 'F' : 0100 0110
// 0x61: 'a' : 0110 0001
// 0x66: 'f' : 0110 0110
//             0010 0000  |(1 << 5)  // OR it with TblVal to have lower case
//
// Return:  LLHH (LE), HHLL (BE)
//
static _finline uint16 ByteToHexChar(uint8 Value, bool UpCase=true)  // Fast but not relocable
{
 const char ChrTable[] = {"0123456789ABCDEF"};    // TODO: Optimize this string
 uint8 cmsk = ((uint8)!UpCase << 5);  // Low case bit mask
 if constexpr(IsBigEnd) return (uint16(ChrTable[Value >> 4] << 8) | uint16(ChrTable[Value & 0x0F]));
 else return (uint16(ChrTable[Value & 0x0F] << 8) | uint16(ChrTable[Value >> 4]));
}
//---------------------------------------------------------------------------
// 'buf' is for storage only, DO NOT expect result to be at beginning of it
template<typename T, typename S> static S DecNumToStrS(T Val, S buf, uint* Len=nullptr)
{
 if(Val == 0){if(Len)*Len = 1; *buf = '0'; buf[1] = 0; return buf;}
 bool isNeg = (Val < 0);
 if(isNeg) Val = -Val;       // warning C4146: unary minus operator applied to unsigned type, result still unsigned
 buf  = &buf[20];
 *buf = 0;
 S end = buf;
 for(buf--;Val;buf--)
  {
   *buf  = (Val % 10) + '0';
   Val  /= 10;
  }
 if(isNeg)*buf = '-';
   else buf++;
 if(Len)*Len = end-buf;     // A counted string
  else buf[end-buf] = 0;    // A null terminated string
 return buf;
}
//---------------------------------------------------------------------------
// No Streams support!
template<typename T, typename O> static O DecNumToStrU(T Val, O buf, int* Len)     // A/W char string and Signed/Unsigned output by constexpr
{
 if(Val == 0){if(Len)*Len = 1; *buf = '0'; buf[1] = 0; return buf;}
 buf  = &buf[20];
 *buf = 0;
 O end = buf;
 for(buf--;Val;buf--)
  {
   *buf  = (Val % 10) + '0';  // NOTE: Ensure that this is optimized to a single DIV operation with remainder preservation
   Val  /= 10;
  }
 buf++;
 if(Len)*Len = end-buf;
  else buf[end-buf] = 0;
 return buf;     // Optionally move?
}
//---------------------------------------------------------------------------
template<typename O, typename T> static O DecStrToNum(T Str, long* Size=nullptr)
{
 O x = 0;
 T Old = Str;
 bool neg = false;
 if (*Str == '-'){neg = true; ++Str;}
 for(unsigned char ch;(ch=*Str++ - '0') <= 9;)x = (x*10) + ch;        // Can use a different base?
 if(Size)*Size = (char*)Str - (char*)Old - 1;               // Constexpr?
 if(neg)x = -x;
 return x;
}
//---------------------------------------------------------------------------
template<typename O, typename T> static O HexStrToNum(T Str, long* Size=nullptr)   // Stops on a first invlid hex char    // Negative values?
{
 O x = 0;
 T Old = Str;
 for(long chv;(chv=HexCharToHalfByte(*Str++)) >= 0;)x = (x<<4) + chv;  // (<<4) avoids call to __llmul which is big
 if(Size)*Size = (char*)Str - (char*)Old - 1;               // Constexpr?
 return x;
}
//---------------------------------------------------------------------------
template<typename T> static uint HexStrToByteArray(uint8* Buffer, T SrcStr, uint HexByteCnt=(uint)-1)
{
 uint ctr = 0;
 for(uint len = 0;(SrcStr[len]&&SrcStr[len+1])&&(ctr < HexByteCnt);len++)   // If it would be possible to make an unmodified defaults to disappear from compilation...
  {
   if(SrcStr[len] <= 0x20)continue;   // Skip spaces and line delimitters
   int ByteHi  = HexCharToHalfByte(SrcStr[len]);
   int ByteLo  = HexCharToHalfByte(SrcStr[len+1]);
   if((ByteHi  < 0)||(ByteLo < 0))return ctr;  // Not a HEX char
   Buffer[ctr] = uint8((ByteHi << 4)|ByteLo);
   ctr++;
   len++;
  }
 return ctr;
}
//---------------------------------------------------------------------------
// Return address always points to Number[16-MaxDigits];
//
template<typename T, typename S> static S ConvertToHexStr(T Value, uint MaxDigits, S NumBuf, bool UpCase, uint* Len=nullptr)
{
 const uint cmax = sizeof(T)*2;      // Number of byte halves (Digits)
 char  HexNums[] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F','0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};     // Must be optimized to PlatLen assignments
 uint Case = UpCase?0:16;
 if(Value)
  {
   if(MaxDigits <= 0)              // Auto set max digits
    {
     MaxDigits = 0;
     T tmp = Value;    // Counter needed to limit a signed value
     for(uint ctr=cmax;tmp && ctr;ctr--,MaxDigits++,tmp >>= 4);    // for(T tmp = Value;tmp;tmp>>=4,MaxDigits++);
     if(MaxDigits & 1)MaxDigits++;    // Full bytes
    }
   S DstPtr = &NumBuf[MaxDigits-1];
   for(uint Ctr = 0;DstPtr >= NumBuf;DstPtr--)   // Start from last digit
    {
     if(Ctr < cmax)
      {
       *DstPtr = HexNums[(Value & 0x0000000F)+Case];   // From end of buffer
       Value = Value >> 4;
       Ctr++;
      }
       else *DstPtr = '0';
    }
  }
   else       // Fast 0
    {
     if(MaxDigits <= 0)MaxDigits = 2;
     for(uint ctr=0;ctr < MaxDigits;ctr++)NumBuf[ctr] = '0';
    }
 if(Len)*Len = MaxDigits;
   else NumBuf[MaxDigits] = 0;
 return NumBuf;
}
//---------------------------------------------------------------------------


};

#pragma clang diagnostic pop
