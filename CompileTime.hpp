
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
 
#ifndef CompTimeH
#define CompTimeH    // Defuned just for detection of this file presense
#endif

// ctDISENCSTR           // Disable encryption
// ctNoProtStack         // Do not clean the stack when destructing decrypted string objects
// ctEncSlow             // Use a slow and more complex encryption
// ctCPLSEED             // Specify a custom SEED
// ctMSEDITANDCONT       // Use when comiling with EditAndContinue

#pragma warning(push)
#pragma warning(disable:4307)     // Overflow in a key transformation is expected

//==============================================================================
// C++11 things are used. Microsoft specific things are avoided?
// NOTE: 'unsigned int' is assumed to be 32 bit
// NOTE: Only works as expected with O2(Maximize Speed) optimization! (Os and Ot have no effect)
// NOTE: This is also embeds a string into a code and makes it relocable
// NOTE: Without '__forceinline' the MSVC compiler likes to ignore inlining and compile-time encryption
// TODO: Add another encryption class that supports an encrypted string tables
//------------------------------------------------------------------------------
struct NCTM
{
#if defined(__GNUC__) || (defined(__MWERKS__) && (__MWERKS__ >= 0x3000)) || (defined(__ICC) && (__ICC >= 600)) || defined(__ghs__)
#define ctFUNC __PRETTY_FUNCTION__
#elif defined(__DMC__) && (__DMC__ >= 0x810)
#define ctFUNC __PRETTY_FUNCTION__
#elif defined(__FUNCSIG__)   
#define ctFUNC __FUNCSIG__
#elif (defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 600)) || (defined(__IBMCPP__) && (__IBMCPP__ >= 500))
#define ctFUNC __FUNCTION__
#elif defined(__BORLANDC__) && (__BORLANDC__ >= 0x550)
#define ctFUNC __FUNC__
#else
#define ctFUNC __func__   // Useless, because no types are provided  (Part of C++ 11 standart)
#endif

#ifdef __MSVC__
#define finline  __forceinline 
#else
#define finline  __attribute__((always_inline))   // GCC, CLANG
#endif

 

#ifndef ctCPLSEED
	// If you don't specify the seed for algorithms, the time when compilation started will be used, seed actually changes the results of algorithms...
#define ctCPLSEED (((__TIME__[7] - '0') * 1	+ (__TIME__[6] - '0') * 10)  +  ((__TIME__[4] - '0') * 60 + (__TIME__[3] - '0') * 600)  +  ((__TIME__[1] - '0') * 3600 + (__TIME__[0] - '0') * 36000))
#endif

#define ctEncKey   ((~((unsigned int)(__DATE__[4]) * (unsigned int)(__DATE__[5])) & 0xFF) | ((~((unsigned int)(__TIME__[0]) * (unsigned int)(__TIME__[1])) & 0xFF) << 8) | ((~((unsigned int)(__TIME__[3]) * (unsigned int)(__TIME__[4])) & 0xFF) << 16) | ((~((unsigned int)(__TIME__[6]) * (unsigned int)(__TIME__[7])) & 0xFF) << 24))   // DWORD

#ifndef ctMSEDITANDCONT
#define ctEncKeyEx  (~((__LINE__ + 1) * ((__COUNTER__ + 3) << 1)))    // WORD   // No Edit and continue in MSVC(/ZI) or __LINE__ will not be considered as a constant
#else
#define ctEncKeyEx  (~((__TIME__[7] + __TIME__[6]) * ((__COUNTER__ + 3) << 1)))   // WORD
#endif

constexpr static unsigned int ctAlignAsPtr(unsigned int Size){return (Size/sizeof(void*)) + (bool)(Size%sizeof(void*));}   // Used to calculate a string size in pointer-sized blocks

//template <typename T> constexpr inline static T ctRotL(T Value, unsigned int Shift){constexpr unsigned int MaxBits = sizeof(T) * 8U; return (Value << Shift) | (Value << ((MaxBits - Shift)&(MaxBits-1)));}  // Should be moved to Common.hpp
//template <typename T> constexpr inline static T ctRotR(T Value, unsigned int Shift){constexpr unsigned int MaxBits = sizeof(T) * 8U; return (Value >> Shift) | (Value << ((MaxBits - Shift)&(MaxBits-1)));}  // Should be moved to Common.hpp

template <typename T> constexpr __forceinline static T RotL(T Value, unsigned int Shift){constexpr unsigned int MaxBits = sizeof(T) * 8U; return (Value << Shift) | (Value >> ((MaxBits - Shift)&(MaxBits-1)));}
template <typename T> constexpr __forceinline static T RotR(T Value, unsigned int Shift){constexpr unsigned int MaxBits = sizeof(T) * 8U; return (Value >> Shift) | (Value << ((MaxBits - Shift)&(MaxBits-1)));}

constexpr  __forceinline static int ctStrLen(char const* str, const int offs){return (str[offs])?(ctStrLen(str, offs+1)):(offs);}       // Offset included in result
constexpr  __forceinline static int ctStrDif(char const* sa, char const* sb, const int offs){return (sa[offs] == sb[offs])?(ctStrDif(sa, sb, offs+1)):(offs);}   // Offset included in result
template <int N> constexpr __forceinline static char CharAt(char const(&s)[N], int i){return s[i];}     
template<typename T> constexpr __forceinline static char* CurrFuncSig(void){return ctFUNC; };    // Useless for templates
template<typename T> constexpr __forceinline static char  CurrFuncSigChr(int Idx){return ctFUNC[Idx]; }; 




// The constantify template is used to make sure that the result of constexpr function will be computed at compile-time instead of run-time
template <unsigned int Const> struct ctCplConstantify { enum { Value = Const }; };

// Compile-time mod of a linear congruential pseudorandom number generator, the actual algorithm was taken from "Numerical Recipes" book
constexpr unsigned int ctCplRandom(unsigned int Id){ return (1013904323 + 1664625 * ((Id > 0) ? (ctCplRandom(Id - 1)) : (ctCPLSEED))) & 0xFFFFFFFF; }  // Orig: 1013904223 + 1664525 

// Compile-time random macros, can be used to randomize execution path for separate builds, or compile-time trash code generation
#define ctRANDOM(Min, Max) (Min + (ctRAND() % (Max - Min + 1)))
#define ctRAND()		   (ctCplConstantify<ctCplRandom(__COUNTER__ + 1)>::Value)

// Compile-time generator for list of int (0, 1, 2, ...)  // TODO: Use builtins ( MSVC: __make_integer_seq<integer_sequence, _Ty, _Size>;)
template <int...> struct ctCplIntList {};
template <typename	IndexList, char Right> struct ctCplAppend;
template <int... Left,	   char Right> struct ctCplAppend<ctCplIntList<Left...>, Right> { typedef ctCplIntList<Left..., Right> Result; };

template <unsigned int N> struct ctCplIndexes { typedef typename ctCplAppend<typename ctCplIndexes<N - 1>::Result, N - 1>::Result Result; };
template <> struct ctCplIndexes<0> { typedef ctCplIntList<> Result; };
//------------------------------------------------------------------------------
template <typename T, typename IndexList> struct ctStrHldr;
template <typename C, unsigned int... Idx> struct ctStrHldr<C,ctCplIntList<Idx...> >
{
 C Array[sizeof...(Idx)];

 constexpr __forceinline ctStrHldr(const C* const Str) : Array{ Str[Idx]... } {}  // Compile-time constructor  

 constexpr __forceinline unsigned int Size(void){return (sizeof...(Idx));}
 constexpr __forceinline C* Value(void){return (C*)&this->Array;}   
 constexpr __forceinline C* Decrypt(void){return (C*)&this->Array;}  
 constexpr __forceinline operator   const C*()    {return this->Array;}
};
//------------------------------------------------------------------------------

// Compile-time string encryption class  [Deprecated for C++20]

template <typename T, unsigned int le,  unsigned int ka,  unsigned int kb, typename IndexList> struct ctCplEncryptedString;
template <typename C, unsigned int Len, unsigned int Key, unsigned int ExKey, unsigned int... Idx> struct ctCplEncryptedString<C,Len,Key,ExKey,ctCplIntList<Idx...> >
{
 SIZE_T Array[sizeof...(Idx)]; // Buffer for a string(In aligned byte blocks, not in chars)   // NOTE: Extending this to LONGLONG will require a 64 bit shift and mul
//------------------------------
//template <typename X> constexpr __forceinline SCOMP(X Val, int Num){return (((Idx+Num) < Len)?(((Val) << sizeof(Val)) | Str[Idx+Num]):(0)); }

template <unsigned int Idx> constexpr __forceinline SIZE_T Compose(const char* const Str, const SIZE_T Res)
{
 auto ctSCOMPA = [=](SIZE_T Val, int Num) constexpr -> SIZE_T{ return (((Idx+Num) < Len)?(((Val) << 8) | Str[Idx+Num]):(0)); };
#if defined(_AMD64_)
 return (ctSCOMPA(ctSCOMPA(ctSCOMPA(ctSCOMPA(ctSCOMPA(ctSCOMPA(ctSCOMPA(ctSCOMPA(Res,7),6),5),4),3),2),1),0));
#else
 return (ctSCOMPA(ctSCOMPA(ctSCOMPA(ctSCOMPA(Res,3),2),1),0));
#endif
}
//------------------------------
template <unsigned int Idx> constexpr __forceinline SIZE_T Compose(const wchar_t* const Str, const SIZE_T Res)
{
 auto ctSCOMPW = [=](SIZE_T Val, int Num) constexpr -> SIZE_T{ return (((Idx+Num) < Len)?(((Val) << 16) | Str[Idx+Num]):(0)); };
#if defined(_AMD64_)
 return (ctSCOMPW(ctSCOMPW(ctSCOMPW(ctSCOMPW(Res,3),2),1),0));
#else
 return (ctSCOMPW(ctSCOMPW(Res,1),0));
#endif
}
//------------------------------
static constexpr __forceinline SIZE_T ctCplEncryptCharBlk(const SIZE_T Chb, unsigned int Idx) 
{
#ifdef ctEncSlow
 const unsigned int DUpd = ctRotL((++Idx * (((Key + ExKey) & 0xFF))+1), Idx & 0x0F);
 const unsigned int DKey = (ctRotR(ExKey * Idx, 4) ^ Key);
 return (ctRotL((ctRotL((ctRotL(Chb, 9) ^ (DKey+(DUpd*1))), 9) ^ (DKey+(DUpd*2))), 9) ^ (DKey+(DUpd*3)));  // There are some limit on this expressions to be compile-time! 
#else
 return (ctRotL(Chb, ++Idx & 0x0F) ^ ~((SIZE_T)Key * (SIZE_T)ExKey));    // Invert mult result to help with too small mult result on x64 
#endif 
}
//------------------------------
static constexpr __forceinline SIZE_T ctCplDecryptCharBlk(const SIZE_T Chb, unsigned int Idx) 
{
#ifdef ctEncSlow
 const unsigned int DUpd = ctRotL((++Idx * (((Key + ExKey) & 0xFF))+1), Idx & 0x0F);
 const unsigned int DKey = (ctRotR(ExKey * Idx, 4) ^ Key);
 return ctRotR(ctRotR(ctRotR(Chb ^ (DKey+(DUpd*3)),9) ^ (DKey+(DUpd*2)),9) ^ (DKey+(DUpd*1)),9);
#else
 return ctRotR(Chb ^ ~((SIZE_T)Key * (SIZE_T)ExKey), ++Idx & 0x0F);   
#endif 
}

public:
//------------------------------	
 constexpr __forceinline ctCplEncryptedString(const C* const Str) : Array{ ctCplEncryptCharBlk(Compose<(Idx*sizeof(void*))/sizeof(C)>(Str, 0), Idx)... } {}  // Compile-time constructor  
//------------------------------
#ifndef ctNoProtStack
__forceinline ~ctCplEncryptedString()
{
 for(volatile unsigned int t = 0; t < sizeof...(Idx); t++)this->Array[t] = 0;  // Zeroes are look less suspicious on stack and this simple fill is much faster with some virtualizing protectors          
// for(volatile unsigned int t = 0; t < sizeof...(Idx); t++)this->Array[t] = ((Key * (unsigned int)((short)this)) >> ((t * ExKey) & 7));         // Clear stack     // 'for(unsigned int t = 0; t < sizeof...(Idx); t++)this->Array[t] = ~(Key * (ExKey * (t+1)));'  produces an overcomplicated SSE2 usage!
// volatile unsigned int tmp = this->Array[0];  // The only way to not let it be thrown out by optimizer
}
#endif
//------------------------------		
__forceinline C* Decrypt(void)  // Run-time decryption   // There will be a copy of this function body for each encrypted string usage
{
 for(unsigned int t = 0; t < sizeof...(Idx); t++)this->Array[t] = ctCplDecryptCharBlk(this->Array[t], t);   // Decrypt blocks of chars           
// ((C*)&this->Value)[(((sizeof...(Idx))*sizeof(void*)) / sizeof(C))-1] = 0; // Force a terminating NULL (In case of failed decryption)
// ((C*)&this->Value)[Len-1] = 0;      // Last char is 0
 return (C*)&this->Array;
}
//------------------------------
 constexpr __forceinline unsigned int Size(void){return Len;}	
 constexpr __forceinline C*   Value(void){return (C*)&this->Array;}	
 constexpr __forceinline operator   const C*()    {return this->Decrypt();}      // 'Decrypt' version to use this class un macros and skip it if necessary
}; 

// Compile-time string encryption macro   // TODO: Can user-defined literals help to avoid macros here? RValue refs?
/*#ifndef ctDISENCSTR
// Declare: ctOENCSA("Hello World!", HelloStr);  MyProcA(HelloStr.Decrypt());  MyProcB(HelloStr.Decrypt());
#define ctOENCSA(Str, Name) NCTM::ctCplEncryptedString<char,  sizeof(Str), ctEncKey, ctEncKeyEx, NCTM::ctCplIndexes<NCTM::ctAlignAsPtr(sizeof(Str))>::Result> Name(Str)   // Str size includes a terminating NULL   
#define ctOENCSW(Str, Name) NCTM::ctCplEncryptedString<wchar_t, sizeof(Str)/sizeof(wchar_t), ctEncKey, ctEncKeyEx, NCTM::ctCplIndexes<NCTM::ctAlignAsPtr(sizeof(Str))>::Result> Name(Str)   // Str size includes a terminating NULL

// Declare: ctOENCSA("Hello World!") HelloStr;  MyProc(HelloStr.Decrypt());  MyProcB(HelloStr.Decrypt());
#define ctCENCSA(Str) NCTM::ctCplEncryptedString<char,  sizeof(Str), ctEncKey, ctEncKeyEx, NCTM::ctCplIndexes<NCTM::ctAlignAsPtr(sizeof(Str))>::Result>(Str)   // Str size includes a terminating NULL   
#define ctCENCSW(Str) NCTM::ctCplEncryptedString<wchar_t, sizeof(Str)/sizeof(wchar_t), ctEncKey, ctEncKeyEx, NCTM::ctCplIndexes<NCTM::ctAlignAsPtr(sizeof(Str))>::Result>(Str)   // Str size includes a terminating NULL

// Single use: MyProc(ctENCSA("Hello World!"));
#define ctENCSA(Str) (ctCENCSA(Str).Decrypt())   // Str size includes a terminating NULL   
#define ctENCSW(Str) (ctCENCSW(Str).Decrypt())   // Str size includes a terminating NULL     
#else
#define ctOENCSA(Str, Name) NCTM::ctStrHldr<char, NCTM::ctCplIndexes<sizeof(Str)>::Result> Name(Str)   
#define ctOENCSW(Str, Name) NCTM::ctStrHldr<wchar_t, NCTM::ctCplIndexes<sizeof(Str)/sizeof(wchar_t)>::Result> Name(Str)   

#define ctCENCSA(Str) (Str) NCTM::ctStrHldr<char, NCTM::ctCplIndexes<sizeof(Str)>::Result>(Str)                     // TODO: Use xmm registers should not be allowed because they are stored in data section
#define ctCENCSW(Str) (Str) NCTM::ctStrHldr<wchar_t, NCTM::ctCplIndexes<sizeof(Str)/sizeof(wchar_t)>::Result>(Str)   

#define ctENCSA(Str) (Str)
#define ctENCSW(Str) (Str)  
#endif  */

//====================================================================================
static const bool   IsBigEnd = false;
static const SIZE_T ExEncKey = (SIZE_T)0xD6B4A9C5E2B4C7D3ull;   // TODO: Must be same as a hardware calculated key

static constexpr SIZE_T ctBuildKey = (SIZE_T)MakeBuildKey();  // TODO: Move to CT_CRYPT // NOTE: Data members come before function members so if MakeBuildKey is member of the same class then it is considered an undefined function

__forceinline static SIZE_T MakeExKeyPart(void) // Updates ExEncKeyRT // TODO: Hardware counter based
{
 return ExEncKey;  // TODO: Must calculate ExEncKey somehow
}
//-------------------------------------------------------------------
// Packs bytes so that their in-memory representation will be same on the current platform
template<typename T, typename V> constexpr static T RePackElements(V Arr, unsigned int BLeft)
{
 using ElType = decltype(Arr[0]);
 static_assert(sizeof(T) > sizeof(ElType), "Destination type is smaller!");
 T Result = 0;
 if constexpr (IsBigEnd)
 {
  for(unsigned int ctr = 0; BLeft && (ctr < (sizeof(T) / sizeof(ElType))); ctr++, BLeft--)
   Result |= (T)Arr[ctr] << ((((sizeof(T) / sizeof(ElType))-1)-ctr) * (8*sizeof(ElType)));
 }
 else
 {
  for(unsigned int ctr = 0; BLeft && (ctr < (sizeof(T) / sizeof(ElType))); ctr++, BLeft--)
   Result |= (T)Arr[ctr] << (ctr * (8*sizeof(ElType)));
 }
 return Result;
}
//----------------------------------------------------------------------

template<typename T, SIZE_T N> struct CEStr
{
 static const SIZE_T BytesLen  = sizeof(T) * N;     // Size  (Str including 0)
 static const SIZE_T ArrSize   = (BytesLen / sizeof(SIZE_T)) + (bool)(BytesLen & (sizeof(SIZE_T) - 1));  // FullSize (Allocated)
 //static const SIZE_T UniqueKey;

 SIZE_T Array[ArrSize]; // size_t
                                        // /*for(SIZE_T idx=0,vo=N*sizeof(T);idx < ArrSize;vo=Array[idx],idx++)Array[idx] = ((Array[idx] * N) ^ vo) * (__COUNTER__ * __LINE__);*/
// _finline ~CEStr() {   }  // Implement stack cleaning on destruction  // <<<<<<<<<<<<< Move this to decrypted string holder
 consteval CEStr(const T* str, SIZE_T l) { Init(str, N); }  // Must have an useless arg to be different from another constructor
 consteval CEStr(const T(&str)[N]) { Init(str, N); }
 consteval void Init(const T* str, SIZE_T len)
 {
  //static constexpr SIZE_T UniqueKey = CRC32(str,0); // Makes every string encrypted with an unique key    // How to pass it to 'Decrypt'?
  for(SIZE_T sidx = 0, didx = 0, xkey = ctBuildKey ^ ExEncKey; sidx < N; didx++, sidx += (sizeof(SIZE_T)/sizeof(T)), xkey = RotL(xkey, 1))Array[didx] = RePackElements<SIZE_T>(&str[sidx], N - sidx) ^ RotR(xkey, -didx & 0x0F); 
 }
__forceinline  T* Decrypt(void) const    // const result?
 {
  volatile SIZE_T* arr = &const_cast<CEStr<T,N>* >(this)->Array[0];  // NOTE: Without 'volatile' this entire function may be optimized away and strings will be left unencrypted
//#pragma clang loop unroll(full)  //#pragma unroll  // No effect: it will not unroll unless an optimization mode 1,2 or 3 is enabled
  for(SIZE_T ctr=0,xkey=RotR(~ctBuildKey ^ MakeExKeyPart(),3);ctr < ArrSize;ctr++, xkey=RotL(xkey, 1))arr[ctr] = (arr[ctr] ^ RotR(~RotL(xkey, 3), -ctr & 0x0F));    // Xor key itself is encrypted
  return (T*)this->Array;
 }
 constexpr __forceinline operator const T* ()  const { return const_cast<CEStr<T,N>* >(this)->Decrypt();}  // constness of 'this' pointer is removed
// constexpr __forceinline operator const T* ()  const { return (T*)this->Array; }
 constexpr __forceinline T* Ptr()  const { return (T*)this->Array; }
 constexpr static __forceinline SIZE_T Size(void){return N-1;}  // In chars, without 0
};
#ifndef ctDISENCSTR
#define ctENCSA(Str) NCTM::CEStr(Str).Decrypt()
#define ctENCSW(Str) NCTM::CEStr(Str).Decrypt()
#else
#define ctENCSA(Str) (Str)
#define ctENCSW(Str) (Str)
#endif 
//====================================================================================
/*
 G++   __PRETTY_FUNCTION__:  const char* CProp<T, name>::GetName() [with T = float; char* name = 0]     
                             const char* GetName() [with T = SMyStruct]     
                             const char* GetName() [with T = SMyStruct<float, 9>]
       __FUNCTION__:  GetName
       __func__:  GetName

 MSVC  __FUNCTION__:  CProp<int,0>::GetName
       __FUNCSIG__:  const char *__cdecl CProp<int,0>::GetName(void)
                     char *__cdecl GetName<struct SMyStruct>(void)
                     char *__cdecl GetName<class CProp<float,0>>(void)
       __func__:  GetName
*/                                                                                                                                                     
// Char (TypeName) unpacking     // No way to pass any const char array here?
template <typename T, unsigned int O, unsigned int I> struct ctTNChars { typedef typename ctCplAppend<typename ctTNChars<T,O,I - 1>::Result, CurrFuncSigChr<T>((O+I)-1)>::Result Result; };   // Packs  CharAt<L>("Helo",I - 1) 
template <typename T, unsigned int O> struct ctTNChars<T,O,0> { typedef ctCplIntList<> Result; };

template <typename ChrLst> struct SChrUnp;      
template <char... Idx> struct SChrUnp<ctCplIntList<Idx...> >    // Seems there is no way to use a template function instead
{
 static constexpr __forceinline const char* Chars(void)
  {
   static const char Array[] = {Idx..., 0};
   return Array;
  }
};

template<typename T> constexpr __forceinline static int ctTNPos(const char chr, const int offs, const int End){return ((offs < End) && (CurrFuncSigChr<T>(offs) != chr))?(ctTNPos<T>(chr,offs+1,End)):(offs);} 
template<typename T> constexpr __forceinline static int ctTNLen(const int offs){return (CurrFuncSigChr<T>(offs))?(ctTNLen<T>(offs+1)):(offs);}     // Offset included in result
template<typename T> constexpr __forceinline static int ctTNLenBk(const int offs){return ((offs >= 0) && ((CurrFuncSigChr<T>(offs) > 0x20) || (CurrFuncSigChr<T>(offs-1) == ',')))?(ctTNLenBk<T>(offs-1)):(offs+1);}   // Breaks on any space in case of 'struct SMyStruct' type
template<typename A, typename B> constexpr __forceinline static int ctTNDif(const int offs){return (CurrFuncSigChr<A>(offs) == CurrFuncSigChr<B>(offs))?(ctTNDif<A,B>(offs+1)):(offs);}   // Offset included in result

struct SCplFuncInfo  // Holds info about a TypeName position in a function signature for current compiler
{
 static constexpr int TypeOffs = ctTNDif<char,long>(0);     // Offset of a type in a string   
 static constexpr int TailSize = ctTNLen<char>(TypeOffs+4) - (TypeOffs+4);  // Left of a full string   // 4 is len of 'char' string
};

// Helps to get name of a type without RTTI and RTL
// If Template Params will be included(NoTmpl=false): 'CProp<float,0>'
template<typename T, bool NoTmpl=false> static constexpr __forceinline const char* TypeName(void)  // One instance per requested type, holds only a name
{
 constexpr int End = ctTNLen<T>(SCplFuncInfo::TypeOffs) - SCplFuncInfo::TailSize;   // End if TypeName (Begin for backward reading)
 constexpr int Beg = ctTNLenBk<T>(End);
 constexpr int Pos = (Beg > SCplFuncInfo::TypeOffs)?(Beg):(SCplFuncInfo::TypeOffs);   
 constexpr int Ofs = (NoTmpl && (CurrFuncSigChr<T>(End-1) == '>'))?(ctTNPos<T>('<',Pos,End)):(End);      // '<' is expected to be there
 constexpr int Len = Ofs - Pos;
 return SChrUnp<ctTNChars<T,Pos,Len>::Result>::Chars();
}

template<typename A, typename B, bool NoTmpl=false> constexpr __forceinline bool IsSameTypes(void){return (TypeName<A,NoTmpl>() == TypeName<B,NoTmpl>());}   // Same strings expected to have same address
//==============================================================================

template<unsigned N> struct StaticStr        // C++20
{
 char buf[N + 1] {};
 constexpr StaticStr(char const* s) { for (unsigned i = 0; i != N; ++i) buf[i] = s[i]; }
 constexpr operator char const*() const { return buf; }
};
template<unsigned N> StaticStr(char const (&)[N]) -> StaticStr<N - 1>;
//==============================================================================
 /*
// TODO: Case sens mode select and WideStr support
// Compile-time recursive mod of string hashing algorithm, the actual algorithm was taken from Qt library (this function isn't case sensitive due to ctCplTolower)
constexpr unsigned char	 ctCplTolower(unsigned char Ch)				   { return (Ch >= 'A' && Ch <= 'Z') ? (Ch - 'A' + 'a') : (Ch); }
constexpr unsigned int ctCplHashPart3(unsigned char Ch, unsigned int Hash) { return ((Hash << 4) + ctCplTolower(Ch)); }
constexpr unsigned int ctCplHashPart2(unsigned char Ch, unsigned int Hash) { return (ctCplHashPart3(Ch, Hash) ^ ((ctCplHashPart3(Ch, Hash) & 0xF0000000) >> 23)); }
constexpr unsigned int ctCplHashPart1(unsigned char Ch, unsigned int Hash) { return (ctCplHashPart2(Ch, Hash) & 0x0FFFFFFF); }
constexpr unsigned int ctCplHash(const unsigned char* Str)           { return (*Str) ? (ctCplHashPart1(*Str, ctCplHash(Str + 1))) : (0); }

// Compile-time hashing macro, hash values changes using the first pseudorandom number in sequence
#define ctHASH(Str) (unsigned int)(ctCplConstantify<ctCplHash(Str)>::Value ^ ctCplConstantify<ctCplRandom(1)>::Value)
*/
//==============================================================================

template<typename Ty> struct RemoveRef { using T = Ty; };
template<typename Ty> struct RemoveRef<Ty&> { using T = Ty; };
template<typename Ty> struct RemoveRef<Ty&&> { using T = Ty; };
};

// ps is embedded packed string or EncryptedString using C++20 (Fast, no index sequences required)
//template<CTStr str> consteval static const auto operator"" _ps() { return str; }   // must be in a namespace or global scope  // C++20, no inlining required if consteval and MSVC bug is finally fixed   // Examples: auto st = "Hello World!"_ps;  MyProc("Hello World!"_ps);
//template<CNStr str> consteval static const auto operator"" _es() { return str; }   // must be in a namespace or global scope  // C++20, no inlining required if consteval and MSVC bug is finally fixed   // Examples: auto st = "Hello World!"_ps;  MyProc("Hello World!"_ps);



#pragma warning(pop)
