// C program for implementation of ftoa()
#include<stdio.h>
#include<math.h>
 
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
enum EFltFmt {ffZeroPad=0x8000, ffSkipZeroFrac=0x4000, ffCommaSep=0x2000};  // Add this flags to 'afterpoint'
static const int MaxDigit = 19;    // Including an empty position (0)
unsigned __int64 uExp10[MaxDigit] =
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
// Converts a floating point number to string.
// Check for 0?  
// Buffer MUST be at least of 50 bytes of size
// Result is NOT placed at beginning of the buffer, instead returned a pointer inside of it
// Fast but only max up to 18 digits after '.' (limitation of UINT64)!  (Stable amount is 15)
// Must produce: 123.1234000   // Add zeroes to 'afterpoint'
//               123.0         // Keep at least one zero, but trim to 'afterpoint'
//               123           // Do not keep zero if integer
//
char* ftoa_simple(double num, unsigned int afterpoint, char *res, unsigned int len, unsigned int* size)    // TODO: Nan and everything else   // Format options
{    
 int  Zeroes;
 bool negative;
 char *ptr = &res[len-1];   // Will store backwards
 *(ptr--)  = 0;     // Make Optional?
 if(num < 0){negative = true; num = -num;}  // force it positive // -0.0 is in the case?
   else negative = false;              
 int DAfter = afterpoint & 0xFF;
 if(DAfter >= MaxDigit)DAfter = MaxDigit-1;   // Safe check uExp10 array       

 unsigned __int64 ipart = (unsigned __int64)num;              // Extract integer part  
 if(DAfter)
  {
   double  fpartd = num - (double)ipart;     // Extract floating part   // At 18 digit after dot getting mistakes
   if(fpartd <  0)fpartd = num - (double)(--ipart);   // Fix rounding up, don`t touch FPU rounding modes
   if(fpartd >= 1)fpartd = num - (double)(++ipart);   // Fix rounding down 

   unsigned __int64 fpart = fpartd * uExp10[DAfter];  // After 15 digit loses some precision
   if(fpart){Zeroes = 0; for(int ctr=DAfter-1;(fpart < uExp10[ctr]);ctr--)Zeroes++;}  // Count zeroes before dot
     else if(afterpoint & ffZeroPad)Zeroes = DAfter;  // If padding with zeroes allowed
           else Zeroes = 1;    // At least one zero when no fraction part
 
   if(fpart || !(afterpoint & ffSkipZeroFrac))   // Make fraction part
    {
     if(fpart && !(afterpoint & ffZeroPad)){while(!(fpart % 10))fpart /= 10;}  // Skip right zeroes
     for(;fpart;ptr--){*ptr = (fpart % 10) + '0'; fpart /= 10;}
     for(;Zeroes;ptr--,Zeroes--)*ptr = '0';
     *(ptr--) = (afterpoint & ffCommaSep)?(','):('.');   // add a dot
    }
  }

 if(ipart){ for(;ipart;ptr--){*ptr = (ipart % 10) + '0'; ipart /= 10;} }
   else *(ptr--) = '0';
 if(negative)*ptr = '-';  
  else ptr++;     
 if(size)*size = len - (ptr - res);  
 return ptr;
}
//------------------------------------------------------------------------------------ 
extern "C" char* _cdecl gcvt(double f, size_t ndigit, char* buf)
{
 unsigned int size = 0;
 char Tmp[64];
 char* xbuf = ftoa_simple(f, ndigit, Tmp, sizeof(Tmp), &size);   // ndigit after point?
 memmove(buf,xbuf,size+1);    // Move string to beginning of buffer
 return buf;
}
//------------------------------------------------------------------------------------
void FTOA_Test(void)
{
 char* ptr;
 unsigned int size = 0;
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