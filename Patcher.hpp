
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

class CPatcher    // CHooker will be derived from it?
{
enum EPOp {poDat=0x00,     // Next byte is an raw byte
           poLoH=0x10,      // Low half contains LoHalf of a data byte  // Terminates current data byte - Test poHiH first if you need its HiHalf
           poHiH=0x20,     // Low half contains HiHalf of a data byte  // Have higher priority than poLoH - Combine with poLoH to skip low half of a data byte 
           poRaw=0x40,     // Raw bytes, if (& 0x7F)==0 then next byte is an counter of raw bytes or else rest of bits is a byte counter(63 max)
           poSkp=0x80,     // Skip bytes, same as poRaw but max 127 without a separate counter
  };
struct SPatchRec   // Must be initialized statically!   // Derive from it to save an original data?
{
 UINT  Flags;    // Backwards
 UINT  Size;     // Size of data to be patched (Calculated if not specified)
 UINT  DSize;    // Size of 'Patch' block
 PBYTE Addr;     // Found for this signature
 PBYTE Patch;  // 2-byte specifiers, Hi byte is an command ()
// Orig Data
};


};