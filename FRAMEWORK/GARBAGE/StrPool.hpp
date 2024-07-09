
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
// For each string search class there are different bucket group
// String class step is 16 bytes
// Trades some memory for perfomance
// If a new string won`t fit in current block, then it will be pyt in a new one and rest of current block`s memory will be wasted ??????????
//
/*class CStrPool    // TODO: Add sync class type(static, empty by default)
{
typedef SAllocLL<> Alloca; 
    

struct alignas(ALIGNSIMD) SStrRec  // Size must be EQ 16   // 32 bytes at least for each string   // For SIMD operations alignment of 16 is required    // Same collision probability on x32 and x64
{
 SStrRec* Next;    // Next of same size  // Enough for search enumeration    // 4/8
 UINT32   Hash;    // TODO: Use UINT64 on x64???
 UINT16   Size;    // String size is limited to block size(Which is 64K (Allocated by 4K minimum))
 UINT16   Extra;   // First and last letters

 static SStrRec* GetHdr(char* Str){return (SStrRec*)&Str[-sizeof(SStrRec)];}
};

 SIZE_T SizeTblSize;
 SIZE_T SizeTblFull;

 UINT MaxStrLen;    // Growing
 UINT BucketsNum;   // Constant
 SStrRec** SizeTbl;
 CMemChain<>* BukChain;   // Hash buckets   
 CMemChain<>* StrChain;

//----------------------------------------------
// Enlarges size table and initializes hash bucket
// 0-sized string is a valid record too
//
void EnlargeStrIdxFor(UINT Size)
{
 if(Size <= this->MaxStrLen)return;

}
//----------------------------------------------

public:
//----------------------------------------------
CStrPool(UINT BucketsNum=503)   // 503 is Prime Number  (1 bucket per page(x64); 2 buckets per page (x32))
{
 this->MaxStrLen  = 0;
 this->BucketsNum = BucketsNum; 

// this->PageInc = 1;
// this->Chain   = NFRM::CPageChain<CStrPool*>::Create(this->PageInc++);  
// *this->Chain->GetUserData() = this;    // We can get instance of this stream from any item address in it
}
//----------------------------------------------
~CStrPool()
{
 this->BukChain->Destroy();
 this->StrChain->Destroy();
}
//----------------------------------------------
/*void Load(CMemStrm* Strm)    // For serialization
{

}
//----------------------------------------------
void Save(CMemStrm* Strm)    // For serialization
{

} */
//----------------------------------------------
/*UINT AddStr(char* Str, UINT Size=0)
{
 return 0;
}
//----------------------------------------------
char* GetStrAt(UINT Offset, UINT* Size=nullptr)
{
 return 0;
}
//----------------------------------------------
static UINT GetSizeOf(char* Str)
{
 return 0;
}
//----------------------------------------------
static CStrPool* GetPoolOf(char* Str)
{
 return 0;
}
//----------------------------------------------

};
//---------------------------------------------------------------------------
*/
