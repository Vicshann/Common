
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
#define CompTimeH
#endif

#pragma warning(push)
#pragma warning(disable:4307)     // Overflow in a key transformation is expected

//==============================================================================
// C++11 things are used. Microsoft specific things are avoided?
// NOTE: Only works as expected with O2(Maximize Speed) optimization! (Os and Ot have no effect)
// NOTE: This is also embedds a string into a code and makes it relocable
// NOTE: Without '__forceinline' the MSVC compiler likes to ignore inlining and compile-time encryption
// TODO: Add another encryption class that supports an encrypted string tables
//------------------------------------------------------------------------------
#ifndef ctCPLSEED
	// If you don't specify the seed for algorithms, the time when compilation started will be used, seed actually changes the results of algorithms...
#define ctCPLSEED (((__TIME__[7] - '0') * 1	+ (__TIME__[6] - '0') * 10)  +  ((__TIME__[4] - '0') * 60 + (__TIME__[3] - '0') * 600)  +  ((__TIME__[1] - '0') * 3600 + (__TIME__[0] - '0') * 36000))
#endif

#define ctEncKey   ((~((UINT)__DATE__[4] * (UINT)__DATE__[5]) & 0xFF) | ((~((UINT)__TIME__[0] * (UINT)__TIME__[1]) & 0xFF) << 8) | ((~((UINT)__TIME__[3] * (UINT)__TIME__[4]) & 0xFF) << 16) | ((~((UINT)__TIME__[6] * (UINT)__TIME__[7]) & 0xFF) << 24))   // DWORD

#ifndef ctMSEDITANDCONT
#define ctEncKeyEx  (~((__LINE__ + 1) * ((__COUNTER__ + 3) << 1)))    // WORD   // No Edit and continue in MSVC(/ZI) or __LINE__ will not be considered as a constant
#else
#define ctEncKeyEx  (~((__TIME__[7] + __TIME__[6]) * ((__COUNTER__ + 3) << 1)))   // WORD
#endif

constexpr UINT32 ctAlignAsPtr(UINT32 Size){return (Size/sizeof(void*)) + (bool)(Size%sizeof(void*));}   // Used to calculate a string size in pointer-sized blocks

template <typename T> constexpr __forceinline T _fastcall ctRotL(T Value, unsigned char Shift){return (Value << Shift) | (Value >> ((sizeof(T) * 8U) - Shift));}
template <typename T> constexpr __forceinline T _fastcall ctRotR(T Value, unsigned char Shift){return (Value >> Shift) | (Value << ((sizeof(T) * 8U) - Shift));}

// The constantify template is used to make sure that the result of constexpr function will be computed at compile-time instead of run-time
template <UINT32 Const> struct ctCplConstantify { enum { Value = Const }; };

// Compile-time mod of a linear congruential pseudorandom number generator, the actual algorithm was taken from "Numerical Recipes" book
constexpr UINT32 ctCplRandom(UINT32 Id){ return (1013904323 + 1664625 * ((Id > 0) ? (ctCplRandom(Id - 1)) : (ctCPLSEED))) & 0xFFFFFFFF; }  // Orig: 1013904223 + 1664525 

// Compile-time random macros, can be used to randomize execution path for separate builds, or compile-time trash code generation
#define ctRANDOM(Min, Max) (Min + (ctRAND() % (Max - Min + 1)))
#define ctRAND()		   (ctCplConstantify<ctCplRandom(__COUNTER__ + 1)>::Value)

// Compile-time generator for list of indexes (0, 1, 2, ...)
template <UINT32...> struct ctCplIndexList {};
template <typename	IndexList, UINT32 Right> struct ctCplAppend;
template <UINT32... Left,	  UINT32 Right> struct ctCplAppend<ctCplIndexList<Left...>, Right> { typedef ctCplIndexList<Left..., Right> Result; };
template <UINT32 N> struct ctCplIndexes { typedef typename ctCplAppend<typename ctCplIndexes<N - 1>::Result, N - 1>::Result Result; };
template <> struct ctCplIndexes<0> { typedef ctCplIndexList<> Result; };


//------------------------------------------------------------------------------
template <typename T, typename IndexList> struct ctStrHldr;
template <typename C, UINT32... Idx> struct ctStrHldr<C,ctCplIndexList<Idx...> >
{
 C Array[sizeof...(Idx)];

 constexpr __forceinline ctStrHldr(const C* const Str) : Array{ Str[Idx]... } {}  // Compile-time constructor  

 constexpr __forceinline UINT Size(void){return (sizeof...(Idx));}
 constexpr __forceinline C* Value(void){return (C*)&this->Array;}   
 constexpr __forceinline C* Decrypt(void){return (C*)&this->Array;}  
 constexpr __forceinline operator   const C*()    {return this->Array;}
};
//------------------------------------------------------------------------------

#define ctSCOMPA(Val,Num) (((Idx+Num) < Len)?(((Val) <<  8) | Str[Idx+Num]):(0))
#define ctSCOMPW(Val,Num) (((Idx+Num) < Len)?(((Val) << 16) | Str[Idx+Num]):(0))

// Compile-time string encryption class
template <typename T, UINT32 le, UINT32 ka,  UINT32 kb, typename IndexList> struct ctCplEncryptedString;
template <typename C, UINT32 Len, UINT32 Key, UINT32 ExKey, UINT32... Idx> struct ctCplEncryptedString<C,Len,Key,ExKey,ctCplIndexList<Idx...> >
{
 UINT_PTR Array[sizeof...(Idx)]; // Buffer for a string(In aligned byte blocks, not in chars)   // NOTE: Extending this to LONGLONG will require a 64 bit shift and mul
//------------------------------

template <UINT32 Idx> constexpr __forceinline UINT_PTR ctCompose(const char* const Str, const UINT_PTR Res)
{
#if defined(_AMD64_)
 return (ctSCOMPA(ctSCOMPA(ctSCOMPA(ctSCOMPA(ctSCOMPA(ctSCOMPA(ctSCOMPA(ctSCOMPA(Res,7),6),5),4),3),2),1),0));
#else
 return (ctSCOMPA(ctSCOMPA(ctSCOMPA(ctSCOMPA(Res,3),2),1),0));
#endif
}
//------------------------------
template <UINT32 Idx> constexpr __forceinline UINT_PTR ctCompose(const wchar_t* const Str, const UINT_PTR Res)
{
#if defined(_AMD64_)
 return (ctSCOMPW(ctSCOMPW(ctSCOMPW(ctSCOMPW(Res,3),2),1),0));
#else
 return (ctSCOMPW(ctSCOMPW(Res,1),0));
#endif
}
//------------------------------
static constexpr __forceinline UINT_PTR ctCplEncryptCharBlk(const UINT_PTR Chb, UINT32 Idx) 
{
#ifdef ctEncSlow
 const UINT32 DUpd = ctRotL((++Idx * (((Key + ExKey) & 0xFF))+1), Idx & 0x0F);
 const UINT32 DKey = (ctRotR(ExKey * Idx, 4) ^ Key);
 return (ctRotL((ctRotL((ctRotL(Chb, 9) ^ (DKey+(DUpd*1))), 9) ^ (DKey+(DUpd*2))), 9) ^ (DKey+(DUpd*3)));  // There are some limit on this ecpressions to be compile-time! 
#else
 return (ctRotL(Chb, ++Idx & 0x0F)) ^ ~((UINT_PTR)Key * (UINT_PTR)ExKey);    // Invert mult result to help with too small mult result on x64 
#endif 
}
//------------------------------
static constexpr __forceinline UINT_PTR ctCplDecryptCharBlk(const UINT_PTR Chb, UINT32 Idx) 
{
#ifdef ctEncSlow
 const UINT32 DUpd = ctRotL((++Idx * (((Key + ExKey) & 0xFF))+1), Idx & 0x0F);
 const UINT32 DKey = (ctRotR(ExKey * Idx, 4) ^ Key);
 return ctRotR(ctRotR(ctRotR(Chb ^ (DKey+(DUpd*3)),9) ^ (DKey+(DUpd*2)),9) ^ (DKey+(DUpd*1)),9);
#else
 return (ctRotR(Chb ^ ~((UINT_PTR)Key * (UINT_PTR)ExKey), ++Idx & 0x0F));   
#endif 
}

public:
//------------------------------	
 constexpr __forceinline ctCplEncryptedString(const C* const Str) : Array{ ctCplEncryptCharBlk(ctCompose<(Idx*sizeof(void*))/sizeof(C)>(Str, 0), Idx)... } {}  // Compile-time constructor  
//------------------------------
#ifndef ctNoProtStack
__forceinline ~ctCplEncryptedString()
{
 for(volatile UINT32 t = 0; t < sizeof...(Idx); t++)this->Array[t] = 0;  // Zeroes look less suspicious on stack and this simple fill is much faster with some vrtualizing protector          
// for(volatile UINT32 t = 0; t < sizeof...(Idx); t++)this->Array[t] = ((Key * (UINT)((short)this)) >> ((t * ExKey) & 7));         // Clear stack     // 'for(UINT32 t = 0; t < sizeof...(Idx); t++)this->Array[t] = ~(Key * (ExKey * (t+1)));'  produces an overcomplicated SSE2 usage!
// volatile UINT tmp = this->Array[0];  // The only way to not let it be thrown out by optimizer
}
#endif
//------------------------------		
__forceinline C* Decrypt(void)  // Run-time decryption   // There will be a copy of this function(CALL) for each encrypted string
{
 for(UINT32 t = 0; t < sizeof...(Idx); t++)this->Array[t] = ctCplDecryptCharBlk(this->Array[t], t);   // Decrypt blocks of chars           
// ((C*)&this->Value)[(((sizeof...(Idx))*sizeof(void*)) / sizeof(C))-1] = 0; // Force a terminating NULL (In case of failed decryption)
// ((C*)&this->Value)[Len-1] = 0;      // Last char is 0
 return (C*)&this->Array;
}
//------------------------------
 constexpr __forceinline UINT Size(void){return Len;}	
 constexpr __forceinline C*   Value(void){return (C*)&this->Array;}	
 constexpr __forceinline operator   const C*()    {return this->Decrypt();}      // 'Decrypt' version to use this class un macros and skip it if necessary
}; 

// Compile-time string encryption macro
#ifndef ctDISENCSTR
#define ctOENCSA(Str, Name) ctCplEncryptedString<char,  sizeof(Str), ctEncKey, ctEncKeyEx, ctCplIndexes<ctAlignAsPtr(sizeof(Str))>::Result> Name(Str)   // Str size includes a terminating NULL   
#define ctOENCSW(Str, Name) ctCplEncryptedString<wchar_t, sizeof(Str)/sizeof(wchar_t), ctEncKey, ctEncKeyEx, ctCplIndexes<ctAlignAsPtr(sizeof(Str))>::Result> Name(Str)   // Str size includes a terminating NULL

#define ctCENCSA(Str) ctCplEncryptedString<char,  sizeof(Str), ctEncKey, ctEncKeyEx, ctCplIndexes<ctAlignAsPtr(sizeof(Str))>::Result>(Str)   // Str size includes a terminating NULL   
#define ctCENCSW(Str) ctCplEncryptedString<wchar_t, sizeof(Str)/sizeof(wchar_t), ctEncKey, ctEncKeyEx, ctCplIndexes<ctAlignAsPtr(sizeof(Str))>::Result>(Str)   // Str size includes a terminating NULL

#define ctENCSA(Str) (ctCENCSA(Str).Decrypt())   // Str size includes a terminating NULL   
#define ctENCSW(Str) (ctCENCSW(Str).Decrypt())   // Str size includes a terminating NULL
#else
#define ctOENCSA(Str, Name) ctStrHldr<char, ctCplIndexes<sizeof(Str)>::Result> Name(Str)   
#define ctOENCSW(Str, Name) ctStrHldr<wchar_t, ctCplIndexes<sizeof(Str)/sizeof(wchar_t)>::Result> Name(Str)   

#define ctCENCSA(Str) (Str) ctStrHldr<char, ctCplIndexes<sizeof(Str)>::Result>(Str) 
#define ctCENCSW(Str) (Str) ctStrHldr<wchar_t, ctCplIndexes<sizeof(Str)/sizeof(wchar_t)>::Result>(Str)   

#define ctENCSA(Str) (Str)
#define ctENCSW(Str) (Str)
#endif

//==============================================================================
/*  // TODO: Case sens mode select and WideStr support
// Compile-time recursive mod of string hashing algorithm, the actual algorithm was taken from Qt library (this function isn't case sensitive due to ctCplTolower)
constexpr UINT8	 ctCplTolower(UINT8 Ch)				   { return (Ch >= 'A' && Ch <= 'Z') ? (Ch - 'A' + 'a') : (Ch); }
constexpr UINT32 ctCplHashPart3(UINT8 Ch, UINT32 Hash) { return ((Hash << 4) + ctCplTolower(Ch)); }
constexpr UINT32 ctCplHashPart2(UINT8 Ch, UINT32 Hash) { return (ctCplHashPart3(Ch, Hash) ^ ((ctCplHashPart3(Ch, Hash) & 0xF0000000) >> 23)); }
constexpr UINT32 ctCplHashPart1(UINT8 Ch, UINT32 Hash) { return (ctCplHashPart2(Ch, Hash) & 0x0FFFFFFF); }
constexpr UINT32 ctCplHash(const UINT8* Str)           { return (*Str) ? (ctCplHashPart1(*Str, ctCplHash(Str + 1))) : (0); }

// Compile-time hashing macro, hash values changes using the first pseudorandom number in sequence
#define ctHASH(Str) (UINT32)(ctCplConstantify<ctCplHash(Str)>::Value ^ ctCplConstantify<ctCplRandom(1)>::Value)
*/
//==============================================================================

#pragma warning(pop)