
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

#pragma warning(push)
#pragma warning(disable:4307)     // Overflow in a key transformation is expected

//==============================================================================
// C++11 things are used. Microsoft specific things are avoided?
// NOTE: 'unsigned int' is assumed to be 32 bit
// NOTE: Only works as expected with O2(Maximize Speed) optimization! (Os and Ot have no effect)
// NOTE: This is also embedds a string into a code and makes it relocable
// NOTE: Without '__forceinline' the MSVC compiler likes to ignore inlining and compile-time encryption
// TODO: Add another encryption class that supports an encrypted string tables
//------------------------------------------------------------------------------
namespace CT
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

// GCC: #define __forceinline __attribute__((always_inline))

#ifndef ctCPLSEED
	// If you don't specify the seed for algorithms, the time when compilation started will be used, seed actually changes the results of algorithms...
#define ctCPLSEED (((__TIME__[7] - '0') * 1	+ (__TIME__[6] - '0') * 10)  +  ((__TIME__[4] - '0') * 60 + (__TIME__[3] - '0') * 600)  +  ((__TIME__[1] - '0') * 3600 + (__TIME__[0] - '0') * 36000))
#endif

#define ctEncKey   ((~((unsigned int)__DATE__[4] * (unsigned int)__DATE__[5]) & 0xFF) | ((~((unsigned int)__TIME__[0] * (unsigned int)__TIME__[1]) & 0xFF) << 8) | ((~((unsigned int)__TIME__[3] * (unsigned int)__TIME__[4]) & 0xFF) << 16) | ((~((unsigned int)__TIME__[6] * (unsigned int)__TIME__[7]) & 0xFF) << 24))   // DWORD

#ifndef ctMSEDITANDCONT
#define ctEncKeyEx  (~((__LINE__ + 1) * ((__COUNTER__ + 3) << 1)))    // WORD   // No Edit and continue in MSVC(/ZI) or __LINE__ will not be considered as a constant
#else
#define ctEncKeyEx  (~((__TIME__[7] + __TIME__[6]) * ((__COUNTER__ + 3) << 1)))   // WORD
#endif

constexpr unsigned int ctAlignAsPtr(unsigned int Size){return (Size/sizeof(void*)) + (bool)(Size%sizeof(void*));}   // Used to calculate a string size in pointer-sized blocks

template <typename T> constexpr __forceinline T _fastcall ctRotL(T Value, unsigned char Shift){return (Value << Shift) | (Value >> ((sizeof(T) * 8U) - Shift));}
template <typename T> constexpr __forceinline T _fastcall ctRotR(T Value, unsigned char Shift){return (Value >> Shift) | (Value << ((sizeof(T) * 8U) - Shift));}

constexpr static __forceinline int ctStrLen(char const* str, const int offs){return (str[offs])?(ctStrLen(str, offs+1)):(offs);}       // Offset included in result
constexpr static __forceinline int ctStrDif(char const* sa, char const* sb, const int offs){return (sa[offs] == sb[offs])?(ctStrDif(sa, sb, offs+1)):(offs);}   // Offset included in result
template <int N> constexpr __forceinline char CharAt(char const(&s)[N], int i){return s[i];}     
template<typename T> constexpr __forceinline char* CurrFuncSig(void){return ctFUNC; };    // Useless for templates
template<typename T> constexpr __forceinline char  CurrFuncSigChr(int Idx){return ctFUNC[Idx]; }; 


// The constantify template is used to make sure that the result of constexpr function will be computed at compile-time instead of run-time
template <unsigned int Const> struct ctCplConstantify { enum { Value = Const }; };

// Compile-time mod of a linear congruential pseudorandom number generator, the actual algorithm was taken from "Numerical Recipes" book
constexpr unsigned int ctCplRandom(unsigned int Id){ return (1013904323 + 1664625 * ((Id > 0) ? (ctCplRandom(Id - 1)) : (ctCPLSEED))) & 0xFFFFFFFF; }  // Orig: 1013904223 + 1664525 

// Compile-time random macros, can be used to randomize execution path for separate builds, or compile-time trash code generation
#define ctRANDOM(Min, Max) (Min + (ctRAND() % (Max - Min + 1)))
#define ctRAND()		   (ctCplConstantify<ctCplRandom(__COUNTER__ + 1)>::Value)

// Compile-time generator for list of int (0, 1, 2, ...)
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

#define ctSCOMPA(Val,Num) (((Idx+Num) < Len)?(((Val) <<  8) | Str[Idx+Num]):(0))
#define ctSCOMPW(Val,Num) (((Idx+Num) < Len)?(((Val) << 16) | Str[Idx+Num]):(0))

// Compile-time string encryption class
template <typename T, unsigned int le,  unsigned int ka,  unsigned int kb, typename IndexList> struct ctCplEncryptedString;
template <typename C, unsigned int Len, unsigned int Key, unsigned int ExKey, unsigned int... Idx> struct ctCplEncryptedString<C,Len,Key,ExKey,ctCplIntList<Idx...> >
{
 SIZE_T Array[sizeof...(Idx)]; // Buffer for a string(In aligned byte blocks, not in chars)   // NOTE: Extending this to LONGLONG will require a 64 bit shift and mul
//------------------------------

template <unsigned int Idx> constexpr __forceinline SIZE_T ctCompose(const char* const Str, const SIZE_T Res)
{
#if defined(_AMD64_)
 return (ctSCOMPA(ctSCOMPA(ctSCOMPA(ctSCOMPA(ctSCOMPA(ctSCOMPA(ctSCOMPA(ctSCOMPA(Res,7),6),5),4),3),2),1),0));
#else
 return (ctSCOMPA(ctSCOMPA(ctSCOMPA(ctSCOMPA(Res,3),2),1),0));
#endif
}
//------------------------------
template <unsigned int Idx> constexpr __forceinline SIZE_T ctCompose(const wchar_t* const Str, const SIZE_T Res)
{
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
 return (ctRotL((ctRotL((ctRotL(Chb, 9) ^ (DKey+(DUpd*1))), 9) ^ (DKey+(DUpd*2))), 9) ^ (DKey+(DUpd*3)));  // There are some limit on this ecpressions to be compile-time! 
#else
 return (ctRotL(Chb, ++Idx & 0x0F)) ^ ~((SIZE_T)Key * (SIZE_T)ExKey);    // Invert mult result to help with too small mult result on x64 
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
 return (ctRotR(Chb ^ ~((SIZE_T)Key * (SIZE_T)ExKey), ++Idx & 0x0F));   
#endif 
}

public:
//------------------------------	
 constexpr __forceinline ctCplEncryptedString(const C* const Str) : Array{ ctCplEncryptCharBlk(ctCompose<(Idx*sizeof(void*))/sizeof(C)>(Str, 0), Idx)... } {}  // Compile-time constructor  
//------------------------------
#ifndef ctNoProtStack
__forceinline ~ctCplEncryptedString()
{
 for(volatile unsigned int t = 0; t < sizeof...(Idx); t++)this->Array[t] = 0;  // Zeroes look less suspicious on stack and this simple fill is much faster with some vrtualizing protector          
// for(volatile unsigned int t = 0; t < sizeof...(Idx); t++)this->Array[t] = ((Key * (unsigned int)((short)this)) >> ((t * ExKey) & 7));         // Clear stack     // 'for(unsigned int t = 0; t < sizeof...(Idx); t++)this->Array[t] = ~(Key * (ExKey * (t+1)));'  produces an overcomplicated SSE2 usage!
// volatile unsigned int tmp = this->Array[0];  // The only way to not let it be thrown out by optimizer
}
#endif
//------------------------------		
__forceinline C* Decrypt(void)  // Run-time decryption   // There will be a copy of this function(CALL) for each encrypted string
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

// Compile-time string encryption macro
#ifndef ctDISENCSTR
#define ctOENCSA(Str, Name) CT::ctCplEncryptedString<char,  sizeof(Str), ctEncKey, ctEncKeyEx, CT::ctCplIndexes<AlignAsPtr(sizeof(Str))>::Result> Name(Str)   // Str size includes a terminating NULL   
#define ctOENCSW(Str, Name) CT::ctCplEncryptedString<wchar_t, sizeof(Str)/sizeof(wchar_t), ctEncKey, ctEncKeyEx, CT::ctCplIndexes<AlignAsPtr(sizeof(Str))>::Result> Name(Str)   // Str size includes a terminating NULL

#define ctCENCSA(Str) CT::ctCplEncryptedString<char,  sizeof(Str), ctEncKey, ctEncKeyEx, CT::ctCplIndexes<AlignAsPtr(sizeof(Str))>::Result>(Str)   // Str size includes a terminating NULL   
#define ctCENCSW(Str) CT::ctCplEncryptedString<wchar_t, sizeof(Str)/sizeof(wchar_t), ctEncKey, ctEncKeyEx, CT::ctCplIndexes<AlignAsPtr(sizeof(Str))>::Result>(Str)   // Str size includes a terminating NULL

#define ctENCSA(Str) (ctCENCSA(Str).Decrypt())   // Str size includes a terminating NULL   
#define ctENCSW(Str) (ctCENCSW(Str).Decrypt())   // Str size includes a terminating NULL
#else
#define ctOENCSA(Str, Name) CT::ctStrHldr<char, CT::ctCplIndexes<sizeof(Str)>::Result> Name(Str)   
#define ctOENCSW(Str, Name) CT::ctStrHldr<wchar_t, CT::ctCplIndexes<sizeof(Str)/sizeof(wchar_t)>::Result> Name(Str)   

#define ctCENCSA(Str) (Str) CT::ctStrHldr<char, CT::ctCplIndexes<sizeof(Str)>::Result>(Str) 
#define ctCENCSW(Str) (Str) CT::ctStrHldr<wchar_t, CT::ctCplIndexes<sizeof(Str)/sizeof(wchar_t)>::Result>(Str)   

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

template<typename T> constexpr __forceinline int ctTNPos(const char chr, const int offs, const int End){return ((offs < End) && (CurrFuncSigChr<T>(offs) != chr))?(ctTNPos<T>(chr,offs+1,End)):(offs);} 
template<typename T> constexpr __forceinline int ctTNLen(const int offs){return (CurrFuncSigChr<T>(offs))?(ctTNLen<T>(offs+1)):(offs);}     // Offset included in result
template<typename T> constexpr __forceinline int ctTNLenBk(const int offs){return ((offs >= 0) && ((CurrFuncSigChr<T>(offs) > 0x20) || (CurrFuncSigChr<T>(offs-1) == ',')))?(ctTNLenBk<T>(offs-1)):(offs+1);}   // Breaks on any space in case of 'struct SMyStruct' type
template<typename A, typename B> constexpr __forceinline int ctTNDif(const int offs){return (CurrFuncSigChr<A>(offs) == CurrFuncSigChr<B>(offs))?(ctTNDif<A,B>(offs+1)):(offs);}   // Offset included in result

struct SCplFuncInfo  // Holds info about a TypeName position in a function signature for current compiler
{
 static const int TypeOffs = ctTNDif<char,long>(0);     // Offset of a type in a string
 static const int TailSize = ctTNLen<char>(TypeOffs+4) - (TypeOffs+4);  // Left of a full string   // 4 is len of 'char' string
};

// Helps to get name of a type without RTTI and RTL
// If Template Params will be included(NoTmpl=false): 'CProp<float,0>'
template<typename T, bool NoTmpl=false> constexpr __forceinline const char* TypeName(void)  // One instance per requested type, holds only a name
{
 constexpr int End = ctTNLen<T>(SCplFuncInfo::TypeOffs) - SCplFuncInfo::TailSize;   // End if TypeName (Begin for backward reading)
 constexpr int Beg = ctTNLenBk<T>(End);
 constexpr int Pos = (Beg > SCplFuncInfo::TypeOffs)?(Beg):(SCplFuncInfo::TypeOffs);   
 constexpr int Ofs = (NoTmpl && (CurrFuncSigChr<T>(End-1) == '>'))?(ctTNPos<T>('<',Pos,End)):(End);      // '<' is expected to be there
 constexpr int Len = Ofs - Pos;
 return SChrUnp<ctTNChars<T,Pos,Len>::Result>::Chars();
}

template<typename A, typename B, bool NoTmpl=false> constexpr __forceinline bool IsSameTypes(void){return (TypeName<A,NoTmpl>() == TypeName<B,NoTmpl>());}  
//==============================================================================
/*  // TODO: Case sens mode select and WideStr support
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
 }
#pragma warning(pop)