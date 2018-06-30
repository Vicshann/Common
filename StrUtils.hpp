
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
namespace NSTR        // TODO: Goes to FRAMEWORK
{
// #define haszero(v) (((v) - 0x01010101UL) & ~(v) & 0x80808080UL)
// #define hasvalue(x,n) \ (haszero((x) ^ (~0UL/255 * (n))))
//
// These templates should work on ANY input strings, built in ones or some classes 
// NUL char is always terminates but strings not have to be NUL terminated
//
template<typename T=wchar_t> union ChrOpNone {static T DoOp(T val){return val;}};      
template<typename T=wchar_t> union ChrOpSiLC {static T DoOp(T val){return (((val >= 'A')&&(val <= 'Z'))?(val + 0x20):(val));}};  
template<typename T=wchar_t> union ChrOpSiUC {static T DoOp(T val){return (((val >= 'a')&&(val <= 'z'))?(val - 0x20):(val));}};  

template<typename COp=ChrOpNone<>, typename A=wchar_t*, typename B=wchar_t*> int _fastcall Compare(A StrValA, B StrValB, size_t MaxLen=-1)  // Not exactly standart!  // NOTE: For a case sensitive strings there should be a faster way
{
 for(size_t voffs=0;;voffs++,MaxLen--)
  {
   if(!MaxLen)return 0;
   int ValA = StrValA[voffs];
   int ValB = StrValB[voffs];
   if(!(ValA|ValB))return 0;          // Strings match by specified size or by a terminating 0
   if(!ValA)return -1;                // With this you can simply compare some substring value (StrValA) at any position in a base string (StrValB)
   if(COp::DoOp(ValA) != COp::DoOp(ValB))return voffs+1;     // Returns 'voffs+1' to avoid 0 when a first char didn`t match
  }
}
//--------------------------------------------------------------------------- 
template<typename COp=ChrOpNone<>, typename A=wchar_t*, typename B=wchar_t*> int _fastcall ChrOffset(A ChrVal, B StrBase, size_t Offs=0, size_t Len=-1)   // TODO: constexpr if to exclude 'Len' if not used (-1)
{
 if(Offs > Len)return -2;    // NOTE: No pointer check
 ChrVal = COp::DoOp(ChrVal);
 for(;StrBase[Offs] && Len;Offs++,Len--)    // NOTE: Would be nice to have an optional ability to exclude any operation on an argument which hasn`t ben changed from its default value
  {
   if(COp::DoOp(StrBase[Offs]) == ChrVal)return Offs;         
  }
 return -1;
}
//--------------------------------------------------------------------------- 
template<typename COp=ChrOpNone<>, typename A=wchar_t*, typename B=wchar_t*> int _fastcall SubOffset(A StrVal, B StrBase, size_t Offs=0, size_t BaseLen=-1, size_t ValLen=-1)   
{
 if(Offs > BaseLen)return -2;    // if(!StrBase || !StrVal || !*StrVal || (Offs > Len))
 for(size_t voffs=0;StrBase[Offs] && (Offs < BaseLen);)
  {
   if(COp::DoOp(StrBase[Offs]) != COp::DoOp(StrVal[voffs])){Offs += (bool)!voffs; voffs = 0; continue;}  // Reset scan
    else { voffs++; if(!StrVal[voffs] || (voffs >= ValLen))return Offs-voffs+1; }  // Exit if Full sub str match found
   Offs++;      
  }
 return -1;  // BaseStr terminated early
}
//--------------------------------------------------------------------------- 
template<typename T> size_t StrLen(T Path)
{
 size_t len = 0;
 for(;Path[len];len++);
 return len;
}
//---------------------------------------------------------------------------
template<typename D, typename S> size_t StrCopy(D Dst, S Src)
{
 size_t len = 0;
 for(;Src[len];len++)Dst[len] = Src[len];
 Dst[len] = 0;
 return len;
}
//---------------------------------------------------------------------------
template<typename D, typename S> UINT StrCopy(D Dst, S Src, size_t MaxLen)
{
 size_t len = 0;
 for(;Src[len] && (len < MaxLen);len++)Dst[len] = Src[len];
 Dst[len] = 0;
 return len;
}
//---------------------------------------------------------------------------
template<typename D, typename S> size_t StrCat(D Dst, S Src)
{
 size_t slen = 0;
 size_t dlen = StrLen(Dst);
 for(;Src[slen];dlen++,slen++)Dst[dlen] = Src[slen];
 Dst[dlen] = 0;
 return dlen;
}
//---------------------------------------------------------------------------

//--------------------------------------------------------------------------- 




template<typename COp=ChrOpNone<>, typename A=wchar_t*, typename B=wchar_t*> bool IsContainSubStr(A StrVal, B StrBase){return (SubOffset<COp>(StrVal, StrBase) >= 0);}
template<typename A, typename B> int _fastcall CompareIC(A StrValA, B StrValB, size_t MaxLen=-1){return Compare<ChrOpSiLC<>, A, B>(StrValA, StrValB, MaxLen);} 
template<typename A, typename B> int _fastcall CompareSC(A StrValA, B StrValB, size_t MaxLen=-1){return Compare<ChrOpNone<>, A, B>(StrValA, StrValB, MaxLen);}  // Template alias argument deduction is not implemented in C++


//---------------------------------------------------------------------------
class Mini
{


};
//---------------------------------------------------------------------------

};

//------------------------------- TEMPORARY, FOR COMPATIBILITTY -------------
// typedef NStr::Mini CMiniStr
// template <typename A, typename B> using StrCompareSimple = NStr::Compare<A,B, ChrOpNone<decltype(*A)> >; // Template alias argument deduction is not implemented in C++
#define StrCompareSimple        NSTR::Compare<NSTR::ChrOpNone<> >      
#define StrCompareSimpleIC      NSTR::Compare<NSTR::ChrOpSiLC<> >  
#define GetChrOffsSimpleIC      NSTR::ChrOffset<NSTR::ChrOpSiLC<> >
#define GetSubStrOffsSimpleIC   NSTR::SubOffset<NSTR::ChrOpSiLC<> >
#define IsContainSubStrSimpleIC NSTR::IsContainSubStr<NSTR::ChrOpSiLC<> >
//---------------------------------------------------------------------------
