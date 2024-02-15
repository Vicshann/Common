
#pragma once
/*
  Copyright (c) 2021 Victor Sheinmann, Vicshann@gmail.com

  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"),
  to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
  and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

//---------------------------------------------------------------------------
struct NUTF
{    // NOTE: Streams should emulate array access
static const uint32 maxUtf32 = 0x0010FFFFUL;

static const inline uint32 IllegalChar[]  = {0x0000FFFDUL, 0};   // Allow to replace as Global static?
static const inline uint32 UTF8_OFFSETS[] = {0x00000000UL, 0x00003080UL, 0x000E2080UL, 0x03C82080UL, 0xFA082080UL, 0x82082080UL};
//static const inline uint8  FIRST_BYTE_MARK[] = {0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC};
static const inline uint8  UTF8_BYTES[] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,5};
// TODO: Make the table shorter by replacing values with their correspoindig counts: {192, 32, 16, 8, 4, 4}  for 0, 1, 2, 3, 4, 5
//---------------------------------------------------------------------------
static uint CharLenUtf8(achar LeadChr)     // Same as 'return UTF8_BYTES[LeadChr]'
{
 if((LeadChr & 0xFE) == 0xFC)return 6;
 if((LeadChr & 0xFC) == 0xF8)return 5;
 if((LeadChr & 0xF8) == 0xF0)return 4;
 else if ((LeadChr & 0xF0) == 0xE0)return 3;
 else if ((LeadChr & 0xE0) == 0xC0)return 2;
 return 1;
}
//---------------------------------------------------------------------------
// Convert UTF-32 character to UTF-16
// Input:  1 UTF-32 char
// Output: 1 - 2 UTF-16 chars
// Return: Number of destination uint16 written
template<typename TDst, typename TSrc> static uint ChrUtf32To16(TDst Dst, TSrc Src, uint DstIdx=0, uint SrcIdx=0)
{
 constexpr uint32 highBegin = 0xD800;
 constexpr uint32 lowBegin  = 0xDC00;
 constexpr uint32 maxbmp    = 0x0000FFFF;
 constexpr uint32 base      = 0x0010000UL;
 constexpr uint32 mask      = 0x3FFUL;
 constexpr int shift = 10;

 uint32 c = uint32(Src[SrcIdx]);    // No 0 detection here!
 const uint OrigIdx = DstIdx;
 if(c <= maxbmp)
  {
   if((c >= highBegin) && (c <= lowBegin))Dst[DstIdx++] = IllegalChar[0];
      else Dst[DstIdx++] = uint16(c);
  }
   else if(c > maxUtf32)Dst[DstIdx++] = IllegalChar[0];
     else
      {
       c -= base;
       Dst[DstIdx++] = uint16((c >> shift) + highBegin);
       Dst[DstIdx++] = uint16((c & mask) + lowBegin);
      }
 return DstIdx - OrigIdx;
}
//---------------------------------------------------------------------------
// Convert UTF-32 character to UTF-8
// Input:  1 UTF-32 char
// Output: 1 - 4 UTF-8 chars
// Return: Number of destination uint8 written
template<typename TDst, typename TSrc> static uint ChrUtf32To8(TDst Dst, TSrc Src, uint DstIdx=0, uint SrcIdx=0)
{
 constexpr uint32 bytemark = 0x80;
 constexpr uint32 bytemask = 0xBF;
 using DType = typename RemoveRef<decltype(Dst[0])>::T;  // ???: static_assert on size of this type because it is expected to be of uint8 size? Or allow Dst to expand   // What type if multiple 'operator[]' is present?

// short bytes;         // calculate bytes to write
 uint32 c = uint32(Src[SrcIdx]);   // No 0 detection here!
 const uint OrigIdx = DstIdx;
 if(c < 0x80)   // 1 byte         // Removed 'switch' with backward indexing to make it stream friendly
  {
   Dst[DstIdx++] = DType(uint8(c));   //  | FIRST_BYTE_MARK[bytes]      // {0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC};
  }                                                              //  0     1     2     3     4     5     6
 else if(c < 0x800)  // 2 bytes
  {
   Dst[DstIdx++] = DType(uint8((c >> 6) | 0xC0));   // FIRST_BYTE_MARK[bytes]
   Dst[DstIdx++] = DType(uint8((c | bytemark) & bytemask));
  }
 else if(c < 0x10000)  // 3 bytes
  {
BadChar:
   Dst[DstIdx++] = DType(uint8((c >> 12) | 0xE0));   // FIRST_BYTE_MARK[bytes]
   Dst[DstIdx++] = DType(uint8(((c >> 6) | bytemark) & bytemask));
   Dst[DstIdx++] = DType(uint8((c | bytemark) & bytemask));
  }
 else if(c <= maxUtf32)   // 4 bytes
  {
   Dst[DstIdx++] = DType(uint8((c >> 18) | 0xF0));   // FIRST_BYTE_MARK[bytes]
   Dst[DstIdx++] = DType(uint8(((c >> 12) | bytemark) & bytemask));
   Dst[DstIdx++] = DType(uint8(((c >> 6) | bytemark) & bytemask));
   Dst[DstIdx++] = DType(uint8((c | bytemark) & bytemask));
  }
 else {c = IllegalChar[0]; goto BadChar;}  // 3 bytes, IllegalChar
 return DstIdx - OrigIdx;
}
//---------------------------------------------------------------------------
// Convert UTF-16 characters to UTF-32
// Input:  1 - 2 UTF-16 chars
// Output: 1 UTF-32 char
// Return: Number of source uint16 read
template<typename TDst, typename TSrc> static uint ChrUtf16To32(TDst Dst, TSrc Src, uint DstIdx=0, uint SrcIdx=0)
{
 constexpr uint32 highBegin = 0xD800;
 constexpr uint32 highEnd   = 0xDBFF;
 constexpr uint32 lowBegin  = 0xDC00;
 constexpr uint32 lowEnd    = 0xDFFF;
 constexpr uint32 base = 0x0010000UL;
 constexpr int shift = 10;

 const uint OrigIdx = SrcIdx;
 const uint32 c1 = uint16(Src[SrcIdx++]);
 if((c1 >= highBegin) && (c1 <= highEnd))
  {
   const uint32 c2 = uint16(Src[SrcIdx++]);   // Take another uint16. 0 will be invalid here
   if((c2 >= lowBegin) && (c2 <= lowEnd))Dst[DstIdx] = ((c1 - highBegin) << shift) + (c2 - lowBegin) + base;    // Merge the pair
     else Dst[DstIdx] = IllegalChar[!(bool)c2];   // Write 0 if c2 is 0
  }
   else if((c1 >= lowBegin) && (c1 <= lowEnd))Dst[DstIdx] = IllegalChar[0];   // Not 0
      else Dst[DstIdx] = c1;   // May be 0
 return SrcIdx - OrigIdx;
}
//---------------------------------------------------------------------------
// Convert UTF-8 character to UTF-32
// Input:  1 - 4(6?) UTF-8 chars
// Output: 1 UTF-32 char
// Return: Number of source uint8 read
template<typename TDst, typename TSrc> static uint ChrUtf8To32(TDst Dst, TSrc Src, uint DstIdx=0, uint SrcIdx=0)
{
 uint32 c = 0;
 uint8 bytes = UTF8_BYTES[uint8(Src[SrcIdx])];
 const uint OrigIdx = SrcIdx;
 switch(bytes)    // Compose UTF-32 character
  {
   case 5:
      c = IllegalChar[0];
      c <<= 6;
   case 4:
      c = IllegalChar[0];
      c <<= 6;
   case 3:
      c += uint8(Src[SrcIdx++]);   // May be 0
      c <<= 6;
   case 2:
      c += uint8(Src[SrcIdx++]);   // May be 0
      c <<= 6;
   case 1:
      c += uint8(Src[SrcIdx++]);   // May be 0
      c <<= 6;
   case 0:
      c += uint8(Src[SrcIdx++]);   // May be 0
  }
 Dst[DstIdx] = c - UTF8_OFFSETS[bytes];
 return SrcIdx - OrigIdx;
}
//---------------------------------------------------------------------------
// Convert UTF-8 character to UTF-16
// Input:  1 - 4(6?) UTF-16 chars
// Output: 1 - 2 UTF-16 chars
// Return: Original char value
template<typename TDst, typename TSrc> static uint32 ChrUtf8To16(TDst Dst, TSrc Src, uint& DstIdx, uint& SrcIdx)
{
 uint32 Val;
 SrcIdx += ChrUtf8To32(&Val, Src, 0, SrcIdx);
 DstIdx += ChrUtf32To16(Dst, &Val, DstIdx);
 return Val;
}
//---------------------------------------------------------------------------
// Convert UTF-16 character to UTF-8
// Input:  1 - 2 UTF-16 chars
// Output: 1 - 4(6?) UTF-16 chars
// Return: Original char value
template<typename TDst, typename TSrc> static uint32 ChrUtf16To8(TDst Dst, TSrc Src, uint& DstIdx, uint& SrcIdx)
{
 uint32 Val;
 SrcIdx += ChrUtf16To32(&Val, Src, 0, SrcIdx);
 DstIdx += ChrUtf32To8(Dst, &Val, DstIdx);
 return Val;
}
//===========================================================================
// NOTE: Streams expected to return 0 when reading beyond their end and no more data is available
//---------------------------------------------------------------------------
// Convert UTF-32 to UTF-16
// Return: Number of chars written to Dst
template<typename TDst, typename TSrc> static size_t Utf32To16(TDst Dst, TSrc Src, size_t SrcChrCnt=(size_t)-1, uint DstIdx=0, uint SrcIdx=0)
{
 const uint OrigIdx = DstIdx;
 for(;SrcChrCnt;SrcChrCnt--)
  {
   uint32 Val = Src[SrcIdx++];
   if(!Val)break;
   DstIdx += ChrUtf32To16(Dst, &Val, DstIdx);
  }
 return DstIdx - OrigIdx;
}
//---------------------------------------------------------------------------
// Convert UTF-16 to UTF-32
// Return: Number of chars written to Dst
template<typename TDst, typename TSrc> static size_t Utf16To32(TDst Dst, TSrc Src, size_t SrcChrCnt=(size_t)-1, uint DstIdx=0, uint SrcIdx=0)
{
 const uint OrigIdx = DstIdx;
 for(;SrcChrCnt;SrcChrCnt--)
  {
   uint32 Val;
   SrcIdx += ChrUtf16To32(&Val, Src, 0, SrcIdx);
   if(!Val)break;
   Dst[DstIdx++] = Val;
  }
 return DstIdx - OrigIdx;
}
//---------------------------------------------------------------------------
// Convert UTF-32 to UTF-8
// Return: Number of chars written to Dst
template<typename TDst, typename TSrc> static size_t Utf32To8(TDst Dst, TSrc Src, size_t SrcChrCnt=(size_t)-1, uint DstIdx=0, uint SrcIdx=0)
{
 const uint OrigIdx = DstIdx;
 for(;SrcChrCnt;SrcChrCnt--)
  {
   uint32 Val = Src[SrcIdx++];
   if(!Val)break;
   DstIdx += ChrUtf32To8(Dst, &Val, DstIdx);
  }
 return DstIdx - OrigIdx;
}
//---------------------------------------------------------------------------
// Convert UTF-8 to UTF-32
// Return: Number of chars written to Dst
template<typename TDst, typename TSrc> static size_t Utf8To32(TDst Dst, TSrc Src, size_t SrcChrCnt=(size_t)-1, uint DstIdx=0, uint SrcIdx=0)
{
 const uint OrigIdx = DstIdx;
 for(;SrcChrCnt;SrcChrCnt--)
  {
   uint32 Val;
   SrcIdx += ChrUtf8To32(&Val, Src, 0, SrcIdx);
   if(!Val)break;
   Dst[DstIdx++] = Val;
  }
 return DstIdx - OrigIdx;
}
//---------------------------------------------------------------------------
// Convert UTF-16 to UTF-8
// Return: Number of chars written to Dst
template<typename TDst, typename TSrc> static size_t Utf16To8(TDst Dst, TSrc Src, size_t SrcChrCnt=(size_t)-1, uint DstIdx=0, uint SrcIdx=0)
{
 const uint OrigIdx = DstIdx;
 for(;SrcChrCnt;SrcChrCnt--)
  {
   uint32 Val;
   SrcIdx += ChrUtf16To32(&Val, Src, 0, SrcIdx);
   if(!Val)break;
   DstIdx += ChrUtf32To8(Dst, &Val, DstIdx);
  }
 return DstIdx - OrigIdx;
}
//---------------------------------------------------------------------------
// Convert UTF-8 to UTF-16
// Return: Number of chars written to Dst
template<typename TDst, typename TSrc> static size_t Utf8To16(TDst Dst, TSrc Src, size_t SrcChrCnt=(size_t)-1, uint DstIdx=0, uint SrcIdx=0)
{
 const uint OrigIdx = DstIdx;
 for(;SrcChrCnt;SrcChrCnt--)
  {
   uint32 Val;
   SrcIdx += ChrUtf8To32(&Val, Src, 0, SrcIdx);
   if(!Val)break;
   DstIdx += ChrUtf32To16(Dst, &Val, DstIdx);
  }
 return DstIdx - OrigIdx;
}
//---------------------------------------------------------------------------
};
//---------------------------------------------------------------------------
