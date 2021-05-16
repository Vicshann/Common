
#pragma once
/*
  Copyright (c) 2020 Victor Sheinmann, Vicshann@gmail.com

  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), 
  to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
  and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. 
*/

//---------------------------------------------------------------------------

struct NBP2       
{
//--------------------------------------
// Base2:   1 bits per byte
// Base4:   2 bits per byte
// Base8:   3 bits per byte
// Base16:  4 bits per byte
// Base32:  5 bits per byte
// Base64:  6 bits per byte (0-63)
// Base128: 7 bits per byte
// Base256: 8 bits per byte (No changes)
//
enum EBaseType {btBase2=1,btBase4,btBase8,btBase16,btBase32,btBase64,btBase128,btBase256};
//--------------------------------------
static inline UINT TblSize(EBaseType BaseVal)
{
 return 1 << (int)BaseVal;
}
//--------------------------------------
static inline UINT BlkSize(EBaseType BaseVal)
{
 char Padd[] = {1,1,8,1,8,4,8,1};   // ([8bit group size] / BaseX)  Base64: 4 = 24 / 6
 return Padd[(int)BaseVal-1];
}
//--------------------------------------
// Index table is 256 bytes to skip any checks
static inline void MakeIdxTbl(EBaseType BaseVal, PBYTE IdxTbl, PBYTE Dict)
{
 memset(IdxTbl,0,256);
 for(int idx=0,tot=TblSize(BaseVal);Dict[idx] && (idx < tot);idx++)IdxTbl[Dict[idx]] = idx;
}
//--------------------------------------
static inline UINT EncodedSize(UINT SrcSize, EBaseType BaseVal)
{
 UINT bitcnt = SrcSize * 8;
 UINT size   = (bitcnt / (UINT)BaseVal) + (bool)(bitcnt % (UINT)BaseVal);    // In chars
 UINT blen   = BlkSize(BaseVal);
 UINT bcnt   = (size / blen) + (bool)(size % blen);
 return bcnt * blen;
}
//--------------------------------------
static inline UINT DecodedSize(UINT SrcSize, EBaseType BaseVal)   // Unprecise: Treats paddings as zeroes
{
 UINT bitcnt = SrcSize * (UINT)BaseVal;
 return (bitcnt / 8) + 1;
}
//===========================================================================
struct SBP2Coder
{
 PBYTE SrcPtr; 
 PBYTE DstPtr; 
 PBYTE IdxTbl;
 PBYTE Dict;
 UINT  SrcLen; 
 UINT  DstLen;
 UINT  SrcPos; 
 UINT  DstPos;
 UINT  BitsLeft;
 UINT  BaseVal;
 UINT  Value;  
 UINT  PadChr;
 UINT  Mask;

//--------------------------------------
void Init(EBaseType Type, BYTE Padding, PBYTE Dictionary, PBYTE IndexTable)
{
 memset(this,0,sizeof(SBP2Coder));
 BaseVal = Type;
 PadChr  = Padding;
 IdxTbl  = IndexTable;
 Dict    = Dictionary;
 Mask    = ((UINT)-1) >> ((sizeof(UINT)*8) - BaseVal);
}
//--------------------------------------
void Reset(void)
{
 SrcPos = DstPos = BitsLeft = Value = 0;
}
//--------------------------------------
void SetSrcBuf(PBYTE Ptr, UINT Len)
{
 SrcPtr = Ptr;
 SrcLen = Len;
 SrcPos = 0;
}
//--------------------------------------
void SetDstBuf(PBYTE Ptr, UINT Len)
{
 DstPtr = Ptr;
 DstLen = Len;
 DstPos = 0;
}
//--------------------------------------
UINT Encode(void)
{
 PadChr |= 0x100;  // Set Encode flag
 for(;SrcPos < SrcLen;SrcPos++)
  {
   Value = (Value << 8) | SrcPtr[SrcPos];   // Split it as a bit stream   
   for(BitsLeft += 8;BitsLeft > BaseVal;BitsLeft -= BaseVal)DstPtr[DstPos++] = Dict[(Value >> (BitsLeft - BaseVal)) & Mask];   // Dst is expected to be correctly precalculated
  }
 return DstPos;
}
//--------------------------------------
UINT Decode(void)
{
 PadChr &= ~0x100;  // Reset Encode flag
 for(;SrcPos < SrcLen;SrcPos++)
  {
   Value = (Value << BaseVal) | IdxTbl[SrcPtr[SrcPos]];    // Merge it as a bit stream    // Padding chars give zero bits  
   for(BitsLeft += BaseVal;BitsLeft >= 8;BitsLeft -= 8)DstPtr[DstPos++] = (Value >> (BitsLeft - 8));   // Dst is expected to be correctly precalculated
  }
 return DstPos;  
}
//--------------------------------------
UINT EndEncode(void)   // Encode: Add padding chars
{
 if(BitsLeft)DstPtr[DstPos++] = Dict[(Value << (BaseVal - BitsLeft)) & Mask];
 UINT DVal = BlkSize((EBaseType)BaseVal);
 UINT CVal = (DstPos % DVal);
 if(CVal)     // Pad to group size (4 chars for Base64)
  {
   for(UINT aleft = DVal - CVal;aleft;aleft--)DstPtr[DstPos++] = PadChr;
  }
 return DstPos;
}
//--------------------------------------
UINT EndDecode(void)  // Decoding: Remove padding zeroes
{
 int PadCtr = 0;
 for(int pos=SrcPos-1;(SrcPtr[pos] == (BYTE)PadChr) && (pos >= 0);pos--)PadCtr++;
 if(PadCtr)
  {
   int bitctr = PadCtr * BaseVal;
   DstPos -= (bitctr >> 3) + (bool)(bitctr & 7);
  }
 return DstPos;
}
//--------------------------------------

};
//===========================================================================
struct SBP2TCoder: public SBP2Coder     // Note: Index Table is not needed for encoding 
{
 private:
 BYTE Table[256];

void Init(EBaseType Type, BYTE Padding, PBYTE Dictionary, PBYTE IndexTable) = delete;    // using SBP2Coder::Init; ???

public:
//--------------------------------------
void Init(EBaseType Type, BYTE Padding, PBYTE Dictionary)
{
 this->SBP2Coder::Init(Type, Padding, Dictionary, Table);
 MakeIdxTbl(Type, Table, Dictionary);
}
//--------------------------------------

};
//===========================================================================
private:
template<unsigned N> struct SBaseDict        // C++20
{
 unsigned char dict[N] {};
 unsigned char idxt[256] {};
 constexpr SBaseDict(char const* s) 
  { 
   for(unsigned i = 0; i < N; ++i)
    {
     BYTE val = s[i];
     dict[i] = val;
     idxt[val] = i;
    }
  }
// constexpr operator char const*() const { return buf; }
};
template<unsigned N> SBaseDict(char const (&)[N]) -> SBaseDict<N - 1>;

public:
//---------------------------------------------------------------------------

template<EBaseType Type, char Padding, SBaseDict Dictionary> struct CBaseP2 
{

static UINT Encode(CMiniStr& Src)
{
 CMiniStr Dst;
 SBP2Coder bc;
 bc.Init(Type, Padding, (PBYTE)&Dictionary.dict, (PBYTE)&Dictionary.idxt);  // (PBYTE)IdxTbl
 Dst.SetLength(EncodedSize(Src.Length(), Type));
 bc.SetSrcBuf(Src.c_data(), Src.Length());
 bc.SetDstBuf(Dst.c_data(), Dst.Length());
 UINT len = bc.Encode();
 len = bc.EndEncode();    // Len must match with Dst.Length()
 Src.cAssign(Dst.c_str(), len);
 return len; 
}
//-------------------------------------- 
static UINT Decode(CMiniStr& Src)
{
 CMiniStr Dst;
 SBP2Coder bc;
 bc.Init(Type, Padding, (PBYTE)&Dictionary.dict, (PBYTE)&Dictionary.idxt);    // (PBYTE)IdxTbl
 Dst.SetLength(DecodedSize(Src.Length(), Type));
 bc.SetSrcBuf(Src.c_data(), Src.Length());
 bc.SetDstBuf(Dst.c_data(), Dst.Length());
 UINT len = bc.Decode();
 len = bc.EndDecode();    // Len must match with Dst.Length()
 Src.cAssign(Dst.c_str(), len);
 return len; 
} 
//--------------------------------------
static bool IsCharInDict(BYTE Char)
{
 PBYTE Chars = (PBYTE)&Dictionary.dict;
 if(Char == Padding)return true;
 for(;*Chars;Chars++)
  {
   if(*Chars == Char)return true;
  }
 return false;
}
//--------------------------------------
};
//===========================================================================
using NBase64 = CBaseP2<btBase64, '=', "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/">;

//---------------------------------------------------------------------------
}; 
//===========================================================================



