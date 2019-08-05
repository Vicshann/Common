
#pragma once
/*
  Copyright (c) 2018 Victor Sheinmann, Vicshann@gmail.com

  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), 
  to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
  and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. 
*/

//---------------------------------------------------------------------------
namespace NMath
{
// Binary - These two are much faster
// pow_mod right-shifts n (the exponent) each iteration through the loop, so the number of iterations is proportional to the number of bits in the number 
template <typename T> T mul_mod(T a, T b, T m)    // m is Modulus
{
// if (m == 0) return a * b;
 T r = 0;    // T r = T(); // 0
 for(;a > 0;a >>= 1)
 {
  if (a & 1)
	if ((r += b) > m) r %= m;
  if ((b <<= 1) > m) b %= m;
 }
 return r;
}
//---------------------------------------------------------------------------
template <typename T> T pow_mod(T a, T n, T m)  // a is Base, n is Exponent, m is Modulus
{
 T r = 1;
 for(;n > 0;n >>= 1)
  {
   if (n & 1)r = mul_mod(r, a, m);
   a = mul_mod(a, a, m);
  }
 return r;
}
//---------------------------------------------------------------------------
template <typename T> T bin_pow(T a, T n)     // Untested!
{
 T res = 1;
 while (n) 
 {
  if (n & 1)res *= a;
  a *= a;
  n >>= 1;
 }
 return res;
}
//---------------------------------------------------------------------------

}
//---------------------------------------------------------------------------
