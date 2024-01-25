
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
struct NSTR
{
// #define haszero(v) (((v) - 0x01010101UL) & ~(v) & 0x80808080UL)
// #define hasvalue(x,n) \ (haszero((x) ^ (~0UL/255 * (n))))
//
// These templates should work on ANY input strings, built in ones or some classes
// NUL char is always terminates but strings not have to be NUL terminated
//

static auto _finline CharToLoASCII(auto chr){return chr | 0x20;}
static auto _finline CharToUpASCII(auto chr){return chr & ~0x20;}

template<typename T=wchar_t> union ChrOpNone {static const int Dir=1; static T DoOp(T val){return val;}};
template<typename T=wchar_t> union ChrOpSiLC {static const int Dir=0; static T DoOp(T val){return CharToLoASCII(val);}};
template<typename T=wchar_t> union ChrOpSiUC {static const int Dir=0; static T DoOp(T val){return CharToUpASCII(val);}};    // TODO: Use tables(Optional)

// Return: 0 if strings match
//   UNRELIABLE !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// Need to separate?
//
template<typename COp=ChrOpNone<>, typename A=wchar_t*, typename B=wchar_t*> _finline static sint StrCompare(A StrValA, B StrValB, size_t MaxLen=(size_t)-1)  // Not exactly standard!  // NOTE: For a case sensitive strings there should be a faster way
{
 for(sint voffs=0;;voffs++,MaxLen--)    // TODO: Do not decrement, precalculate EndOfStr
  {
   if(!MaxLen)return 0;               // Match by size????????????????????????????????????????????????????????????????????????
   int ValA = StrValA[voffs];
   int ValB = StrValB[voffs];
   if(!(ValA|ValB))return 0;          // Strings match by specified size or by a terminating 0
   if(!ValA)return -1;                // With this you can simply compare some substring value (StrValA) at any position in a base string (StrValB) without specifying size of StrValA; !!!!!!!!!!!!!!!!!!!!
   if(COp::DoOp((wchar_t)ValA) != COp::DoOp((wchar_t)ValB))return voffs+1;     // Returns 'voffs+1' to avoid 0 when a first char didn`t match
  }
}
//---------------------------------------------------------------------------
template<typename COp=ChrOpNone<>, typename A=wchar_t*> _finline static sint ChrOffset(A StrVal, wchar_t ChrVal, size_t Offs=0, size_t Len=(size_t)-1)   // TODO: constexpr if to exclude 'Len' if not used (-1)
{
 if(Offs > Len)return -2;    // NOTE: No pointer check
 ChrVal = COp::DoOp(ChrVal);
 for(;StrVal[Offs] && Len;Offs++,Len--)    // NOTE: Would be nice to have an optional ability to exclude any operation on an argument which hasn`t been changed from its default value
  {
   if(COp::DoOp(StrVal[Offs]) == ChrVal)return (sint)Offs;
  }
 return -1;
}
//---------------------------------------------------------------------------
// Offs is needed bacause you can pass in StrBase an object which just supports operator[]
// But in case of a plain strings, a size is passed separatedly
//
template<typename COp=ChrOpNone<>, typename A=wchar_t*, typename B=wchar_t*> _finline static sint StrOffset(A StrBase, B StrVal, size_t Offs=0, size_t BaseLen=(size_t)-1, size_t ValLen=(size_t)-1)    // char *strstr(const char *haystack, const char *needle)
{
 if(Offs > BaseLen)return -2;    // if(!StrBase || !StrVal || !*StrVal || (Offs > Len))
 for(size_t voffs=0;StrBase[Offs] && (Offs < BaseLen);)     // Slow, by one char. TODO: Use memcmp for case sensitive strngs
  {
   if(COp::DoOp(StrBase[Offs]) != COp::DoOp(StrVal[voffs])){Offs += (bool)!voffs; voffs = 0; continue;}  // Reset scan
    else { voffs++; if(!StrVal[voffs] || (voffs >= ValLen))return sint(Offs-voffs+1); }  // Exit if Full sub str match found
   Offs++;
  }
 return -1;  // BaseStr terminated early
}
//---------------------------------------------------------------------------
template<typename T> _finline static size_t StrLen(T Path)
{
 size_t len = 0;
 for(;Path[len];len++);
 return len;
}
//---------------------------------------------------------------------------
template<typename D, typename S> _finline static size_t StrCopy(D Dst, S Src)      // Shlwapi.h on Windows use #define for StrCpy and StrCat
{
 size_t len = 0;
 for(;Src[len];len++)Dst[len] = Src[len];
 Dst[len] = 0;
 return len;
}
//---------------------------------------------------------------------------
template<typename D, typename S> _finline static size_t StrCopy(D Dst, S Src, size_t MaxLen)
{
 size_t len = 0;
 for(;Src[len] && (len < MaxLen);len++)Dst[len] = Src[len];         // Probably 'if constexpr (!MaxLen || (len < MaxLen))'  to exclude it if MaxLen is 0 and not needed
 Dst[len] = 0;
 return len;
}
//---------------------------------------------------------------------------
template<typename D, typename S> _finline static size_t StrCnat(D Dst, S Src)
{
 size_t slen = 0;
 size_t dlen = StrLen(Dst);
 for(;Src[slen];dlen++,slen++)Dst[dlen] = Src[slen];
 Dst[dlen] = 0;
 return dlen;
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------




//template<typename COp=ChrOpNone<>, typename A=wchar_t*, typename B=wchar_t*> bool IsContainSubStr(A StrVal, B StrBase){return (SubOffset<COp>(StrVal, StrBase) >= 0);}

// NOTE: Offsets are provided since source str may be a class, not a pointer   // TODO: Support passing by refs for a classes
template<typename A> _finline static int CharOffsetCS(A StrVal, wchar_t ChrVal, size_t Offs=0, size_t Len=(size_t)-1){return ChrOffset<ChrOpNone<>, A>(StrVal, ChrVal, Offs, Len);}
template<typename A> _finline static int CharOffsetCI(A StrVal, wchar_t ChrVal, size_t Offs=0, size_t Len=(size_t)-1){return ChrOffset<ChrOpSiLC<>, A>(StrVal, ChrVal, Offs, Len);}

template<typename A, typename B> _finline static int StrOffsetCS(A StrBase, B StrVal, size_t Offs=0){return StrOffset<ChrOpNone<> >(StrBase, StrVal, Offs);}
template<typename A, typename B> _finline static int StrOffsetCI(A StrBase, B StrVal, size_t Offs=0){return StrOffset<ChrOpSiLC<> >(StrBase, StrVal, Offs);}

template<typename A, typename B> _finline static bool IsContainSubStrCS(A StrBase, B StrVal){return (StrOffset<ChrOpNone<> >(StrBase, StrVal) >= 0);}
template<typename A, typename B> _finline static bool IsContainSubStrCI(A StrBase, B StrVal){return (StrOffset<ChrOpSiLC<> >(StrBase, StrVal) >= 0);}

template<typename A, typename B> _finline static int CompareCS(A StrValA, B StrValB, size_t MaxLen=(size_t)-1){return StrCompare<ChrOpNone<>, A, B>(StrValA, StrValB, MaxLen);}  // Template alias argument deduction is not implemented in C++
template<typename A, typename B> _finline static int CompareCI(A StrValA, B StrValB, size_t MaxLen=(size_t)-1){return StrCompare<ChrOpSiLC<>, A, B>(StrValA, StrValB, MaxLen);}

template<typename A, typename B> _finline static bool IsStrEqualCS(A StrValA, B StrValB, size_t MaxLen=(size_t)-1){return !StrCompare<ChrOpNone<>, A, B>(StrValA, StrValB, MaxLen);}  // == 0
template<typename A, typename B> _finline static bool IsStrEqualCI(A StrValA, B StrValB, size_t MaxLen=(size_t)-1){return !StrCompare<ChrOpSiLC<>, A, B>(StrValA, StrValB, MaxLen);}  // == 0

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
// Return is negative if there is more elements
template<typename T> static sint SplitBySep(T Str, T* PtrArr, uint Cnt, RemovePtr<T>::T Sep)
{
 uint Total = 0;
 PtrArr[Total++] = Str;
 for(decltype(Sep) v=0;(v=*Str);Str++)
  {
   if(v != Sep)continue;
   if(Total >= (int)Cnt)return -(sint)Total;
   PtrArr[Total++] = &Str[1];
   *Str = 0;
  }
 return Total;
}
//---------------------------------------------------------------------------
struct SCStr       // SCStr cstr("Hello World");
{
 const unsigned char Size;
 const char* Data;
 template<int N> constexpr SCStr(const char (&val)[N]):Size(N),Data(val) {}     // char (& Value)[10]):Data(Value),Size(sizeof(Value)) {}
 operator const char* (void) {return this->Data;}
};
//---------------------------------------------------------------------------
class Mini
{


};
//---------------------------------------------------------------------------

};

