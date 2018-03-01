// C program for implementation of ftoa()
#include<stdio.h>
#include<math.h>
 
extern "C" void*  __cdecl memmove(void* _Dst, const void* _Src, size_t _Size);

#pragma pack(push,1)
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
#pragma pack(pop)

 // Converts a given integer x to string str[].  d is the number
 // of digits required in output. If d is more than the number
 // of digits in x, then 0s are added at the beginning.
int intToStrBk(unsigned __int64 x, char* str)    // If used simple 'int' then 'ftol_sse' is required   // max int64 20 digits   // points to end of buffer, no terminating 0
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
}
          
// Converts a floating point number to string.
// Check for 0?  
// Buffer MUST be at least of 50 bytes of size
// Result is NOT placed at beginning of the buffer, instead returned a pointer inside of it
// Fast but only max up to 19 digits after '.' (limitation of UINT64)!
char* ftoa_simple(double n, char *res, unsigned int afterpoint, unsigned int* size)    // TODO: Nan and everything else
{    
 bool negative = (n < 0);                // -0.0 is in the case?
 if(negative)n = -n;             // force it positive
 unsigned __int64 ipart = (unsigned __int64)n;            // Extract integer part  
 double  fpart = n - (double)ipart;     // Extract floating part
 if(fpart <  0)fpart = n - (double)(--ipart);   // Fix rounding up, don`t touch FPU rounding modes
 if(fpart >= 1)fpart = n - (double)(++ipart);   // Fix rounding down  
 if(afterpoint > 19)afterpoint = 19;   // Hardware limit
 res += 64;       // Starting offset in the buffer
 int sc = 0;      // Number of chars in the buffer
 *(--res) = 0;    // Terminating NULL
 if(afterpoint)   // check for display option after point
  {
   fpart = fpart * slow_pow(10, afterpoint); // pow  // Get the value of fraction part upto given no. of points after dot. The third parameter is needed to handle cases like 233.007  // 'pow' is hardware?
   sc  += intToStrBk((unsigned __int64)fpart, res);
   res -= sc;
   if(sc < afterpoint)afterpoint = sc;
   res[afterpoint] = 0;  // Trim to AfterPoint digits
   *(--res) = '.';  // add dot
  }
 int ic = intToStrBk(ipart, res);  // convert integer part to string // At least keep one 0 of integer part 
 res -= ic;
 if(negative){*(--res) = '-'; sc++;}
 if(size)*size = (sc + ic);
 return res;
}
 
extern "C" char* _cdecl gcvt(double f, size_t ndigit, char* buf)
{
 char* xbuf = ftoa_simple(f, buf, ndigit, 0);   // ndigit after point?
 memmove(buf,xbuf,xbuf-buf);    // Move string to beginning of buffer
 return buf;
}

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