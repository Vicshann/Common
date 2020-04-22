// C program for implementation of ftoa()
//#include<stdio.h>
//#include<math.h>
 
extern "C" void*  __cdecl memmove(void* _Dst, const void* _Src, size_t _Size);

/*#pragma pack(push,1)
union UDbl         // Do magic without 64 bit shifts?
{
 struct
  {
   unsigned __int64 Fraction: 52;
   unsigned __int64 Exponent: 11;
   unsigned __int64 Sign: 1;
  }Fields;
 double Value;
};
#pragma pack(pop)  */

 // Converts a given integer x to string str[].  d is the number  // 18.01 becomes 18.1 !!!!!!!!!!!!!
 // of digits required in output. If d is more than the number
 // of digits in x, then 0s are added at the beginning.
/*int intToStrBk(unsigned __int64 x, char* str)    // If used simple 'int' then 'ftol_sse' is required   // max int64 20 digits   // points to end of buffer, no terminating 0
{
 int i = 0;
 do    // for(;x;i++,str--)
  {
   *(--str) = (x%10) + '0';
   x = x/10;     // No way to take this from 'double'?
   i++;
  } 
   while(x);
 return i;   // Number of chars
}
 
  
unsigned __int64 slow_pow(int X, int Y)    // TODO: TestIt
{
 __int64 val = 1;
  while(Y--)val *= X;
 return val;
} */
//------------------------------------------------------------------------------------
/*bool custom_isnan(double var){volatile double d = var;return d != d;}
inline bool IsNan(float f)   // ieee754 only
{
 const unsigned int u = *(unsigned int*)&f;    // 32   // Some compilers(GCC) may break this cast
 return (u&0x7F800000) == 0x7F800000 && (u&0x7FFFFF);    // Both NaN and qNan.
}
inline bool IsNan(double d)  // ieee754 only
{
 const unsigned long long u = *(unsigned long long*)&d;     // 64  // Some compilers(GCC) may break this cast
 return (u&0x7FF0000000000000ULL) == 0x7FF0000000000000ULL && (u&0xFFFFFFFFFFFFFULL);
}
//------------------------------------------------------------------------------------
static inline unsigned long long load_ieee754_rep(double a) 
{
//    unsigned long long r;
 static_assert(sizeof(unsigned long long) == sizeof(a), "Unexpected sizes."); // std::memcpy(&r, &a, sizeof(a)); // Generates movq instruction.    
 return *reinterpret_cast<unsigned long long*>(&a);    
}

static inline unsigned long load_ieee754_rep(float a) 
{
//    unsigned long r;
 static_assert(sizeof(unsigned long) == sizeof(a), "Unexpected sizes."); // std::memcpy(&r, &a, sizeof(a)); // Generates movd instruction.    
 return *reinterpret_cast<unsigned long*>(&a); 
}

const unsigned long long inf_double_shl1 = unsigned long long(0xffe0000000000000);
const unsigned long inf_float_shl1 = unsigned long(0xff000000);

// The shift left removes the sign bit. The exponent moves into the topmost bits,
// so that plain unsigned comparison is enough.
static inline bool isnan2(double a)    { return load_ieee754_rep(a) << 1  > inf_double_shl1; }
static inline bool isinf2(double a)    { return load_ieee754_rep(a) << 1 == inf_double_shl1; }
static inline bool isfinite2(double a) { return load_ieee754_rep(a) << 1  < inf_double_shl1; }
static inline bool isnan2(float a)     { return load_ieee754_rep(a) << 1  > inf_float_shl1; }
static inline bool isinf2(float a)     { return load_ieee754_rep(a) << 1 == inf_float_shl1; }
static inline bool isfinite2(float a)  { return load_ieee754_rep(a) << 1  < inf_float_shl1; }
*/
//------------------------------------------------------------------------------------
enum EFltFmt {ffZeroPad=0x8000, ffSkipZeroFrac=0x4000, ffCommaSep=0x2000, ffUpperCase=0x1000};  // Add this flags to 'afterpoint'
static const int MaxDigit = 19;    // Including an empty position (0)
unsigned long long uExp10[MaxDigit] =    // NOTE: No __int64 for GCC!!!
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
char* ftoa_simple(double num, size_t afterpoint, char *buf, size_t len, size_t* size)    // TODO: Move to a converter class
{ 
 static_assert(sizeof(unsigned long long) == sizeof(double), "Sizes mismatch!");
 static const unsigned long long SpecialMsk = (1ULL << 63);  // 0x8000000000000000
 int  Zeroes;
 bool negative;
 char *ptr = &buf[len-1];   // Will store backwards
 *(ptr--)  = 0;     // Make Optional?  // Skipping ZERO is uselles when we writing the buffer backwards
 if(num < 0){negative = true; num = -num;}  // force it positive // -0.0 is in the case?
   else negative = false;              
 int DAfter = afterpoint & 0xFF;
 if(DAfter >= MaxDigit)DAfter = MaxDigit-1;   // Safe check uExp10 array       

 unsigned long long ipart = (unsigned long long)num;    // NOTE: Casting always truncates          // Extract integer part     // Results 0x8000000000000000 if NAN or INF 
 if(DAfter)                       // NOTE: Avoid converting 'long long' to 'double' - compiler will use a complicated function with a bunch of overflow checks(inline or external)!
  {
   double  fpartd = num - ipart;     // (double)ipart   // Extract floating part   // At 18 digit after dot getting mistakes
   if(fpartd <  0)fpartd = num - (--ipart);   // (double)(--ipart)  // Fix rounding up, don`t touch FPU rounding modes   
   if(fpartd >= 1)fpartd = num - (++ipart);   // (double)(++ipart)  // Fix rounding down              // Results in ipart 0x8000000000000001 if INF

   unsigned long long fpart = unsigned long long(fpartd * uExp10[DAfter]);  // After 15 digit loses some precision    // Results 0x8000000000000000 if NAN or INF
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
extern "C" char* _cdecl gcvt(double f, size_t ndigit, char* buf)
{
 size_t size = 0;
 char Tmp[64];
 char* xbuf = ftoa_simple(f, ndigit, Tmp, sizeof(Tmp), &size);   // ndigit after point?
 memmove(buf,xbuf,size+1);    // Move string to beginning of buffer
 return buf;
}
//------------------------------------------------------------------------------------
void FTOA_Test(void)
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
 ptr = 0;
}
//------------------------------------------------------------------------------------
/*
    double x = 0.0;
    double d = 0.0 / x;   // 76.123456789    -INFINITY; 
*/

/*#include "math.h"
#include <stdio.h>

#define PSH(X) (*(buf++)=(X))
#define PSH1(X) (*(buf--)=(X))
#define PEEK() buf[-1]
#define POP() *(--buf) = '\0'

#define PLUS 1
#define SPACE 2

#define fabs(x) ((x)<0 ? (-x) : (x))

// FIXME: This file contains roundoff error

#ifdef original_hacky_gcvt

char* _cdecl gcvt(double f, size_t ndigit, char * buf)
{
  int i;
  int z;
  int flags = 0;
  int exp = 0;
  char *c = buf;

  if (f < 0.0f) {
    PSH('-');
    f = -f;
  } else {
    if (flags & PLUS) PSH('+');
    if (flags & SPACE) PSH(' ');
  }
  
  if (f) {
    while (f < 1.0f) {
      f *=10.0f;
      exp--;
    }

    while (f >= 10.0f) {
      f /=10.0f;
      exp++;
    }
  }

  while ((exp > 0) && (exp < 7)) {
	  PSH('0'+f);
	  z = f;
	  f -= z;
	  f *= 10.0f;
  	exp--;
  }

  PSH('0'+f);
  z = f;
  f -= z;
  f *= 10.0f;

  PSH('.');

  for (i=0;i<ndigit;i++) {
    PSH('0'+f);
    z = f;
    f -= z;
    f *= 10;
  }
  
  if (exp != 0) {
	  PSH('e');
	  if (exp < 0) {
	    PSH('-');
	    exp = -exp;
	  } else {
	    PSH('+');
	  }
	  
	  PSH('0'+exp/10%10);
	  PSH('0'+exp%10);

 }

  PSH(0);

  return c;
}

#else
char* _cdecl gcvt(double f, size_t ndigit, char * buf)
{
  int i;
  unsigned long long z,k;
  int exp = 0;
  char *c = buf;
  double f2,t,scal;
  int   sign = 0;

  if((int)ndigit == -1)
    ndigit = 5;

// Unsigned long long only allows for 20 digits of precision
// which is already more than double supports, so we limit the
// digits to this.  long double might require an increase if it is ever
// implemented.

  if (ndigit > 20)
	  ndigit = 20;
  
  if (f < 0.0) {
    sign = 1;
    f = -f;
	 buf++;
  }

  scal = 1;
  for (i=ndigit; i>0; i--)
	  scal *= 10;
  k = f + 0.1 / scal;
  f2 = f - k;
  if (!f) {
    PSH('0');
    if(ndigit > 0)
      PSH('.');
    for (i=0;i<ndigit;i++)
      PSH('0');
  	   PSH(0);
  	 return c;
  }

  i = 1;
  while (f >= 10.0) {
  	f /= 10.0;
  	i++;
  }

  buf += i + ndigit + 1; 	

  PSH1(0);

  if(ndigit > 0) {	
	  t = f2 * scal;
	 z = t + 0.5;
    for (i = 0;i < ndigit;i++)
    {
      PSH1('0'+ (z % 10));
	   z /= 10;
    }
    PSH1('.');
  }
  else
    PSH1(0);

  do {
    PSH1('0'+ (k % 10));
    k /= 10;
  }while (k);

  if (sign)
    PSH1('-');
  return c;
}
#endif

*/