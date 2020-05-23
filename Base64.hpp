
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
// TODO: Preprocessor Flags to make it relocatable and static but with generated chars table to remove it from a binary  // And make MiniString optional

class NBase64          // TODO: Derive this from CAlphaCoder <type,alphabet>
{
 static const char PaddChar  = '='; 
 static const int  CharsNum  = 64;
 static const int  IdxTblLen = 80;  

 static inline const char*  CharsTbl(void){return "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";}  // 64 chars  (Must always be 64, bound to algorythm)
 static inline PBYTE IndexTbl(void){static BYTE IndexTbl[IdxTblLen]; return (PBYTE)&IndexTbl;}  

public:
//--------------------------------------
static bool IsSkippableChar(BYTE Char)
{
 return ((Char == 0x0D)||(Char == 0x0A));    // There is more?
}
//--------------------------------------
//#define BASE64_DECODED_SIZE(cb)  (((cb)>>2)*3)
static UINT DecodedSize(UINT size)
{
 return (size / 4) * 3;
}
//--------------------------------------
//#define BASE64_ENCODED_SIZE(cb)  (((cb)*4+11)/12*4+1)
static UINT EncodedSize(UINT size)
{
 return ((size / 3) + (bool)(size % 3)) * 4;
}
//--------------------------------------
static int GetByteFromIndex(BYTE IByte, int& padd)  // For Decoding
{
 if(IByte == PaddChar){padd--; return 0;}
 if((IByte < 43)||(IByte > 122))return -1;  // Not a Base64 char
 IByte -= 43;
 if(IByte >= IdxTblLen)return -2;  // The index table is smaller!
 return (NBase64::IndexTbl()[IByte] - 1);
}
//--------------------------------------

static bool IsCharBase64(BYTE Char)
{
 const char*  Chars = NBase64::CharsTbl();
 if(Char == NBase64::PaddChar)return true;
 for(;*Chars;Chars++)
  {
   if(*Chars == Char)return true;
  }
 return false;
}
//--------------------------------------
static void Initialize(void)
{
 static bool DoOnce = true;
 if(DoOnce) // static DoOnce
  {
   DoOnce = false; 
  // CharsTbl[62] = 45; // 0x2D '-'
  // CharsTbl[63] = 95; // 0x5F '_'
   PBYTE IndexTbl = NBase64::IndexTbl();
   for (int i = 0; i < CharsNum; i++)
	{
	 int j = -43 + CharsTbl()[i];
	 IndexTbl[j] = (BYTE)(i + 1);
	}
   IndexTbl[2]  = 63;
   IndexTbl[52] = 64;
  }
}
//--------------------------------------
static int Encode(CMiniStr &str)
{
 NBase64::Initialize();
 int octr = 0;
 int padd = 0;
 CMiniStr SrcStr = str;
 PBYTE CharsTbl = (PBYTE)NBase64::CharsTbl();
 str.SetLength(EncodedSize(str.Length()),0);
 if(SrcStr.Length() % 3)
  {
   padd = 3-(SrcStr.Length() % 3);
   SrcStr.AddChars(0,padd);
  }
 for(UINT ictr=0;ictr < SrcStr.Length();ictr+=3,octr+=4)
  {
   BYTE CharA  = SrcStr[ictr+0];
   BYTE CharB  = SrcStr[ictr+1];
   BYTE CharC  = SrcStr[ictr+2];
   str[octr+0] = CharsTbl[ CharA >> 2]; // 11111100 -> 00111111
   str[octr+1] = CharsTbl[(CharB >> 4) | ((CharA & 0x03) << 4)];
   str[octr+2] = CharsTbl[(CharC >> 6) | ((CharB & 0x0F) << 2)];
   str[octr+3] = CharsTbl[ CharC & 0x3F];       // 00111111
  }
 for(;padd > 0;padd--)str[octr-padd] = PaddChar; // RF3='{'
 return octr;
}
//--------------------------------------
static int Decode(CMiniStr &str)
{
 NBase64::Initialize();
 int pos  = 0;
 int Padd = 0;
 int Size = str.Length() & ~3; 
 if(!Size)return -1;
 while((Size > 0) && (str.c_data()[Size-4] == PaddChar))Size -= 4;   // Remove some excess padding (Prevents converting them into zeroes)
 if(Size < 4)return -2; // Too small for a Base64 message!  // TODO: Allow incomplete and unpadded segments
 int PExt = (Size % 4);
 if(PExt)
  {
   str.SetLength(Size + (4-PExt));
   PExt = (PExt-1)-3;     // 1,2,3 -> -3,-2,-1
  }
 for(int ctr = 0;ctr < Size;ctr+=4,pos+=3)
  {
   Padd = 0;
   BYTE ByteA = GetByteFromIndex(str[ctr+0],Padd);
   BYTE ByteB = GetByteFromIndex(str[ctr+1],Padd);
   BYTE ByteC = GetByteFromIndex(str[ctr+2],Padd);
   BYTE ByteD = GetByteFromIndex(str[ctr+3],Padd);
   str[pos+0] = (ByteA << 2) | ((ByteB & 0x30) >> 4);           // 2 chars make 1 complete byte
   str[pos+1] = ((ByteB & 0x0F) << 4) | ((ByteC & 0x3C) >> 2);  // 3 chars make 2 complete bytes
   str[pos+2] = ((ByteC & 0x03) << 6) | ByteD;                  // 4 chars make 3 complete bytes
  }
 pos += Padd + PExt;   //for(;pos > 0;pos--){if(str[pos-1]!=0)break;}
 str.SetLength(pos);
 return pos;
}
//--------------------------------------

};
//---------------------------------------------------------------------------
