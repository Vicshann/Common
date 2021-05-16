
#pragma once

namespace NCTM
{
namespace nhidden  // Private stuff
{
//============================================================================================================
//                               Compile time counter implementation 
// https://stackoverflow.com/a/63578752
//------------------------------------------------------------------------------------------------------------
#ifdef COMP_AGCC
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnon-template-friend"
#endif
// NOTE: Litters namespaces with recursive template instantiations! (Instantiates a new type for each counter value recursively)
// NOTE: Usually max value is 1024 (By standard) but do not expect it to go above 256 for every compiler
// Works with GCC, CLANG, MSVC  // Fails with ICC  // Lets hope for a better solution in the future of C++ :)
// MSVC: fatal error : recursive template instantiation exceeded maximum depth of 1024
struct NCTCtrImpl       // Can we split it to bits and offsets?
{
template<typename T> class Flag
{
 struct Dummy
  {
   constexpr Dummy(void){}
   friend constexpr void adl_flag(Dummy);   // Cannot be consteval!  // ICC warns and breaks assertions(except a first one): "void adl_flag(NCTM::NCTCtrImpl::Flag<T>::Dummy)" declares a non-template function -- add <> to refer to a template instance
  };

 template<bool> struct Writer
  {
   friend constexpr void adl_flag(Dummy){}  // Cannot be consteval! Everywhere elese 'consteval'can be used instead of 'constexpr'
  };

 template<class Dummy, int = (adl_flag(Dummy{}), 0) > static constexpr bool Check(int){ return true; }
 template<class Dummy> static constexpr bool Check(short){ return false; }

public:
 template<class Dummy = Dummy, bool Value = Check<Dummy>(0)> static constexpr int  Read(void){ return Value; }
 template<class Dummy = Dummy, bool Value = Check<Dummy>(0)> static constexpr bool ReadSet(void)
  {
   Writer<Value && 0> tmp{};
   (void)tmp;
   return Value;
  }
};

template<typename T, int I> struct Tag
{
 constexpr int value(void) const noexcept { return I; }
};

template<typename T, int N, int Step, bool B> struct Checker
{
 static constexpr int currentval(void) noexcept { return N; }
};

template<typename T, int N, int Step> struct CheckerWrapper
{
 template<bool B = Flag<Tag<T, N>>{}.Read(), int M = Checker<T, N, Step, B>{}.currentval() > static constexpr int currentval(void){ return M; }
};

template<typename T, int N, int Step> struct Checker<T, N, Step, true>
{
 template<int M = CheckerWrapper<T, N + Step, Step>{}.currentval() > static constexpr int currentval(void) noexcept{ return M; }
};

template<typename T, int N, bool B = Flag<Tag<T, N>>{}.ReadSet() > struct Next
{
 static constexpr int value(void) noexcept { return N; }
};

}; // NCTCtrImpl
#ifdef COMP_AGCC
#pragma GCC diagnostic pop
#endif 
}  // hidden namespace
//============================================================================================================


// Public stuff
//============================================================================================================
// DISABLED: Use macros instead. This only complicates compilation on each logging call.
//           Even when inlining is done there is some useless code when optimization is not enabled. 
//           And considering number of logging calls in a large project there will be a lot of work for optimizer to clean that up.
/*struct NSRC  // Original std::source_location retrieves all of this at instance creation
{
 constexpr static _finline auto Line(unsigned int s = __builtin_LINE()) { return s; }
// constexpr static _finline auto Column(unsigned int s = __builtin_COLUMN()) { return s; }     // GCC, ICC does not declare this, only MSVC and CLANG
   constexpr static _finline auto FileName(const char* s = __builtin_FILE()) { return s; }        // Full path included
   constexpr static _finline auto FuncName(const char* s = __builtin_FUNCTION()) { return s; }    // Only the name itself, no arguments or a return type
}; */
//============================================================================================================
// Packs bytes so that their in-memory representation will be the same on the current platform
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
//------------------------------------------------------------------------------------------------------------
template<typename T> constexpr auto CTTypeChr(T* str){return *str;}  // Crashes CLANG if 'consteval'    // Returns first char of a string to use with decltype: decltype(CTTypeChr(str) // For some reason 'decltype(str[0])' does not works in template parameters
template<typename T> consteval auto CTStrLen(T* str)
{
 unsigned int idx = 0;   
 for(;str[idx];)idx++;
 return idx;   
}

// Stores a string in code instead data section(directly in instructions)
//
// NOTE: Even with -O1 CLANG and MSVC can pack the bytes by itself. GCC requires -O2 to pack the bytes. But the string may be put in data section because of XMM instructions 
// With manual packing, CLANG with -O2 will also puts the string into data section which makes them perfectly readable there. CLANG does not allow to set optimization modes selectively
// MSVC, CLANG: -O0, -O1 only; GCC -O0, -O1, -O2; Or disable SSE: GCC, CLANG (-mno-sse2); MSVC (???)
//
template<typename T, uint N> struct CTStr
{
 static const uint BytesLen = sizeof(T) * N;
 static const uint ArrSize  = (BytesLen / sizeof(uint)) + (bool)(BytesLen & (sizeof(uint) - 1));

 uint Array[ArrSize]; // size_t

 //_finline ~CTStr() {for(uint idx=0,vo=N*sizeof(T);idx < ArrSize;vo=Array[idx],idx++)Array[idx] = (Array[idx] * N) ^ vo;}  // Implement stack cleaning on destruction  // <<<<<<<<<<<<< Move this to decrypted string holder
 consteval CTStr(const T* str, uint l) { Init(str, N); }  // Must have an useless arg to be different from another constructor
 consteval CTStr(const T(&str)[N]) { Init(str, N); }
 consteval void Init(const T* str, uint len)
 {
  for(uint sidx = 0, didx = 0; sidx < N; didx++, sidx += sizeof(uint))Array[didx] = RePackElements<uint>(&str[sidx], N - sidx);
 }
 constexpr _finline operator const T* ()  const { return (T*)this->Array; }
};

// ps is embedded packed string or EncryptedString using C++20 (Fast, no index sequences required)
template<CTStr str> consteval static const auto operator"" _ps() { return str; }   // must be in a namespace or global scope  // C++20, no inlining required if consteval and MSVC bug is finally fixed   // Examples: auto st = "Hello World!"_ps;  MyProc("Hello World!"_ps);

// Examples: MyProc(_PS("Hello World!")); 
// I give up and use the ugly macro(At least it accesses 'str' only once). In C++ we cannot pass constexprness as a function argument and cannot pass 'const char*' as a template argument without complications 
//#define _PS(str) ({constexpr auto tsp = (str); CTStr<decltype(CTTypeChr(tsp)), CTStrLen(tsp)>(tsp,1);})   // In most cases this leaves an unreferenced string in data segment (Extra work for optimizer to remove it)
#define _PS(str) NFWK::NCTM::CTStr<decltype(NFWK::NCTM::CTTypeChr(str)), NFWK::NCTM::CTStrLen(str)>(str,1)   // A clean result, but does three accesses to 'str' which may come from '__builtin_FUNCTION()', for example   // Had to use full fixed namespace paths here

//============================================================================================================
/* -- Compile time counter
using counter_A_0_1 = SCounter<struct TagA, 0, 1>;    // CtrA: struct TagA; CtrB: struct TagB; CtrC: struct TagC - You must provide an unique typename for each counter
constexpr int a0 = counter_A_0_1::next(); // 0
constexpr int a1 = counter_A_0_1::next(); // 1
constexpr int a2 = counter_A_0_1::next(); // 2
static_assert(a0 == 0);
static_assert(a1 == 1);
static_assert(a2 == 2);
*/
template <class Tag = void, int Start = 0, int Step = 1> struct SCounter
{
 template <int N = nhidden::NCTCtrImpl::CheckerWrapper<Tag, Start, Step>{}.currentval()> static consteval int next(void){ return nhidden::NCTCtrImpl::Next<Tag, N>{}.value(); }   // return N+1 if the name was already added to the scope
};
//============================================================================================================
// 0: AABBCCDDEEFFGGHH <7,>7 (S-1)  AABBCCDD <3,>3 (S-1)  AABB <1,>1 (S-1)  AA
// 1: HHBBCCDDEEFFGGAA <5,>5 (S-3)  DDBBCCAA <1,>1 (S-3)  BBAA
// 2: HHGGCCDDEEFFBBAA <3,>3 (S-5)  DDCCBBAA
// 3: HHGGFFDDEECCBBAA <1,>1 (S-7)
// 4: HHGGFFEEDDCCBBAA
template<typename T> constexpr _finline static T RevByteOrder(T Value) // Can be used at compile time
{
 if constexpr (sizeof(T) > 1)
  {
   T Result = ((Value & 0xFF) << ((sizeof(T)-1)*8)) | ((Value >> ((sizeof(T)-1)*8)) & 0xFF);  // Exchange edge 1
   if constexpr (sizeof(T) > 2) 
    {
     Result |= ((Value & 0xFF00) << ((sizeof(T)-3)*8)) | ((Value >> ((sizeof(T)-3)*8)) & 0xFF00); // Exchange edge 2
     if constexpr (sizeof(T) > 4)
      {
       Result |= ((Value & 0xFF0000) << ((sizeof(T)-5)*8)) | ((Value >> ((sizeof(T)-5)*8)) & 0xFF0000); // Exchange edge 3
       Result |= ((Value & 0xFF000000) << ((sizeof(T)-7)*8)) | ((Value >> ((sizeof(T)-7)*8)) & 0xFF000000); // Exchange edge 4
      }
    }
   return Result;
  }
 return Value;
}
//============================================================================================================
// CRC32 hash: CRC32A("Hello World!")
// Use polynomial 0x82F63B78 instead of 0xEDB88320 for compatibility with Intel`s hardware CRC32C (SSE 4.2: _mm_crc32_u8) and ARM (ARMv8-A: __crc32d; -march=armv8-a+crc )
/* English dictionary test:
| hash         | collisions | polynomial |
+--------------+------------+------------+
| crc32b       |     44     | 0x04C11DB7 | RFC 1952;   reversed: 0xEDB88320;  reverse of reciprocal: 0x82608EDB
| crc32c       |     62     | 0x1EDC6F41 | Castagnoli; reversed: 0x82F63B78;  reverse of reciprocal: 0x8F6E37A0
| crc32k       |     36     | 0x741B8CD7 | Koopmans;   reversed: 0xEB31D82E;  reverse of reciprocal: 0xBA0DC66B
| crc32q       |     54     | 0x814141AB | AIXM;       reversed: 0xD5828281;  reverse of reciprocal: 0xC0A0A0D5
*/

// Recursive instances! Simple but 8 times slower! Makes compilation process very slow!  // Is it better to generate a compile time CRC32 table?
/*template <uint32 msk = 0xEDB88320, uint N, uint i = 0> constexpr __forceinline static uint32 CRC32A(const char (&str)[N], uint32 result = 0xFFFFFFFF)  // Evaluated for each bit   // Only __forceinline can force it to compile time computation
{
 if constexpr (i >= (N << 3))return ~result;
 else return !str[i >> 3] ? ~result : CRC32A<msk, N, i + 1>(str, (result & 1) != (((unsigned char)str[i >> 3] >> (i & 7)) & 1) ? (result >> 1) ^ msk : result >> 1);
}  */
 
template<uint32 msk = 0xEDB88320, uint N, uint i = 0> constexpr _finline static uint32 CRC32A(const char (&str)[N], uint32 crc=0xFFFFFFFF)  // Unrolling bit hashing makes compilation speed OK again   // Only __forceinline can force it to compile time computation
{
// int bidx = 0;
// auto ChrCrc = [&](uint32 val) constexpr -> uint32 {return ((val & 1) != (((uint32)str[i] >> bidx++) & 1)) ? (val >> 1) ^ msk : val >> 1; };  // MSVC compiler choked on lambda and failed to inline it after fourth instance of CRC32A 
// if constexpr (i < (N-1) )return CRC32A<msk, N, i + 1>(str, ChrCrc(ChrCrc(ChrCrc(ChrCrc(ChrCrc(ChrCrc(ChrCrc(ChrCrc(crc)))))))));  
 if constexpr (i < (N-1))   // No way to read str[i] into a const value? // N-1: Skip 1 null char (Always 1, by C++ standard?)
  {
   crc = ((crc & 1) != (((uint32)str[i] >> 0) & 1)) ? (crc >> 1) ^ msk : crc >> 1;
   crc = ((crc & 1) != (((uint32)str[i] >> 1) & 1)) ? (crc >> 1) ^ msk : crc >> 1;
   crc = ((crc & 1) != (((uint32)str[i] >> 2) & 1)) ? (crc >> 1) ^ msk : crc >> 1;
   crc = ((crc & 1) != (((uint32)str[i] >> 3) & 1)) ? (crc >> 1) ^ msk : crc >> 1;
   crc = ((crc & 1) != (((uint32)str[i] >> 4) & 1)) ? (crc >> 1) ^ msk : crc >> 1;
   crc = ((crc & 1) != (((uint32)str[i] >> 5) & 1)) ? (crc >> 1) ^ msk : crc >> 1;
   crc = ((crc & 1) != (((uint32)str[i] >> 6) & 1)) ? (crc >> 1) ^ msk : crc >> 1;
   crc = ((crc & 1) != (((uint32)str[i] >> 7) & 1)) ? (crc >> 1) ^ msk : crc >> 1;
   return CRC32A<msk, N, i + 1>(str, crc);
  }
 else return ~crc;   
}
//------------------------------------------------------------------------------------------------------------
// Because 'msk' is template param it is impossible to encrypt it  // Is it OK to leave it in a executable as is or petter pass it as the function argument(right after decryption)?
template<uint32 msk = 0xEDB88320> _finline static uint32 CRC32(char* Text, uint32 crc = 0xFFFFFFFF)   // Should it be here or moved to some other unit?  // Useful for some small injected pieces of code which do some string search
{
 uint32 Val;
 for(uint i=0;Val=Text[i];i++) 
  {        
   crc = crc ^ Val;
   for(uint j=8;j;j--)crc = (crc >> 1) ^ (msk & -(crc & 1)); 
  }
 return ~crc;
}

//static_assert(CRC32A("Hello World!") == 0x1C291CA3);

// TODO: CRC64
//============================================================================================================
// If array type is not UINT8 then last(first?) value have padding (because of alignment) with number of bytes to exclude from full size
// C++17 'Class Template Argument Deduction' is useless here because an extra parameters are needed
// 'DataSize' is real size of data in bytes, not aligned to T
template<typename T, std::size_t N> struct CEncArr  
{
 static constexpr std::size_t FullKey = MakeUniqueKey(~((sizeof(T)*5) | (N << 23)) * N);  //~((std::size_t)Key * (std::size_t)ExKey);
 static constexpr bool IsBigEnd = false;
 static constexpr int  DataSize = sizeof(T) * N;
 static constexpr int  ArrSize  = AlignP2Frwd(DataSize, sizeof(std::size_t)) / sizeof(std::size_t);   //(sizeof(T) < sizeof(void*))?(AlignP2Frwd(sizeof(T) * N, sizeof(std::size_t)) / sizeof(std::size_t)):((sizeof(T) > sizeof(void*))?(N * 2):(N)); // Input array of UINT8, UINT16, UINT32, UINT64 will be packed to SIZE_T and encrypted

 std::size_t Array[ArrSize];   // Type must be always SIZE_T to avoid calling some math functions at runtime decryption  // Compile-time packed to target platform

//------------------------------
// Highest bit is expected to be a sign bit in signed types
static constexpr __forceinline std::size_t EncryptDataBlk(const std::size_t Datb, std::size_t Key, std::size_t Idx) 
{     
 return (std::size_t)((std::intptr_t)Datb + (std::intptr_t)ctRotR(Key, (Idx+1) & 0x1F)) ^ ~(std::size_t)((std::intptr_t)Key + ~((std::intptr_t)Idx * (std::intptr_t)Idx));  // Good enough (Tested on AllZeroes)
}
//------------------------------
static constexpr __forceinline std::size_t DecryptDataBlk(const std::size_t Datb, std::size_t Key, std::size_t Idx) 
{     
 return (std::size_t)((std::intptr_t)(Datb ^ ~(std::size_t)((std::intptr_t)Key + ~((std::intptr_t)Idx * (std::intptr_t)Idx))) - (std::intptr_t)ctRotR(Key, (Idx+1) & 0x1F));
}

public:

// Bytes are expected to be packed as 0xB0B1B2B3, 0xB4B5B6B7 ... . This is reversed in memory on LittleEndian platforms (Intel)
explicit constexpr __forceinline CEncArr(const T (&arr)[N]): Array{}   // Is it possible to wrap SBlob in some short form with deduction of the type and size?    // No way to pass keys as template arguments and still have it constructible as 'constexpr static CEncArr xx(0xFFFF, 0x2222, {6,8,9})'?
{
 if constexpr (sizeof(T) < sizeof(std::size_t))  // Merge
  {
   int DstIdx = 0;
   int DstNum = 0;
   std::size_t DstVal = 0;
   for(int SrcIdx=0;SrcIdx < N;SrcIdx++) 
    {
     DstVal |= arr[SrcIdx];
     DstNum++;
     if(DstNum == (sizeof(std::size_t) / sizeof(T)))
      {
       this->Array[DstIdx++] = EncryptDataBlk(DstVal,FullKey,DstIdx);  // Is DstIdx value here UD? Expected to start with 0
       DstVal = 0;
       DstNum = 0;
      }
       else DstVal <<= (sizeof(T)*8);
    }
   if(DstNum)this->Array[DstIdx] = EncryptDataBlk(DstVal,FullKey,DstIdx);  // Store some leftovers
  }
 else if constexpr (sizeof(T) > sizeof(std::size_t))  // Split
  {
   for(int SrcIdx=0,DstIdx=0;SrcIdx < N;SrcIdx++)   
    {
     T SrcVal = arr[SrcIdx];
     this->Array[DstIdx++] = EncryptDataBlk(SrcVal >> (sizeof(std::size_t)*8),FullKey,DstIdx);
     this->Array[DstIdx++] = EncryptDataBlk((std::size_t)SrcVal,FullKey,DstIdx);
    }
  }
 else for(int Idx=0;Idx < N;Idx++)this->Array[Idx] = EncryptDataBlk(arr[Idx],FullKey,Idx);  // Same size
}
//------------------------------
constexpr __forceinline std::size_t Size(const bool Encrypted=true)  
{
 std::size_t padd = this->Array[ArrSize-1];
 if constexpr (Encrypted)padd = DecryptDataBlk(padd, FullKey, ArrSize-1);  // Decrypt the last block to extract a padding value from it
 if constexpr (sizeof(T) < sizeof(std::size_t))padd >>= (sizeof(std::size_t) - sizeof(T)) * 8;       // If merged, skip alignment bytes(No padding is added there)
 return DataSize - (padd & 0xFF);
}
//------------------------------
constexpr __forceinline std::size_t BufSize(void){return ArrSize;}  // Returns size of a buffer required for decryption
constexpr __forceinline void* Data(void){return this->Array;}
//------------------------------
constexpr __forceinline void* Encrypt(void* Buf=nullptr, std::size_t Size=0) const  // In case you need to hide an encrypted data on stack or to restore encryption of a global data  // Has to be const if the class ever instantiated as 'constexpr CEncArr MyData(0x5678ba12, 0x34de6795, {1,2,3});'
{
 std::size_t* DstArr = Buf?((std::size_t*)Buf):(const_cast<std::size_t*>(this->Array));   // If the instance declared as global or 'constexpr static' then writing to the array will result in access fault because the data is most likely has been put into a read-only section
 Size = (Size)?(Size / sizeof(std::size_t)):(ArrSize);
 for(std::size_t Idx=0;Idx < Size;Idx++)
  {
   std::size_t val = DstArr[Idx];
   if constexpr (!IsBigEnd)RevByteOrder(val);  // SwapBytes
   DstArr[Idx] = EncryptDataBlk(val,FullKey,Idx);; 
  }
 return Buf;
}
//------------------------------
constexpr __forceinline void* Decrypt(void* Buf=nullptr, std::size_t Size=0) const   // Has to be const if the class ever instantiated as 'constexpr CEncArr MyData(0x5678ba12, 0x34de6795, {1,2,3});'
{
 std::size_t* DstArr = Buf?((std::size_t*)Buf):(const_cast<std::size_t*>(this->Array));   // If the instance declared as global or 'constexpr static' then writing to the array will result in access fault because the data is most likely has been put into a read-only section
 Size = (Size)?(Size / sizeof(std::size_t)):(ArrSize);
 for(std::size_t Idx=0;Idx < Size;Idx++)
  {
   std::size_t val = DecryptDataBlk(this->Array[Idx],FullKey,Idx);
   if constexpr (!IsBigEnd)RevByteOrder(val);  // SwapBytes
   DstArr[Idx] = val; 
  }
 return Buf;
}
//--------------------------------------------------
std::size_t BuildBlkArrayStr(char* Buffer, void* Data, std::size_t DataLen, int BlkLen=4, int BlkOnLine=64, bool IsStatic=false, char* Name=nullptr)
{
 if((BlkLen != 1)&&(BlkLen != 2)&&(BlkLen != 4)&&(BlkLen != 8))return 0;
 unsigned int ValSize  = (BlkLen*2)+3;
 unsigned int BlksNum  = DataLen / BlkLen;
 unsigned int LinesNum = BlksNum / BlkOnLine;
 if(!Buffer)return (LinesNum * 4) + (BlksNum * ValSize) + 256;     // Return required buffer size    // 4 for a new line and two spaces; 256 for naming
 unsigned int Offs = 0;
 Buffer[Offs++] = ' '; Buffer[Offs++] = 'c'; Buffer[Offs++] = 'o'; Buffer[Offs++] = 'n'; Buffer[Offs++] = 's'; Buffer[Offs++] = 't'; Buffer[Offs++] = 'e'; Buffer[Offs++] = 'x'; Buffer[Offs++] = 'p'; Buffer[Offs++] = 'r'; Buffer[Offs++] = ' ';
 if(IsStatic){Buffer[Offs++] = 's'; Buffer[Offs++] = 't'; Buffer[Offs++] = 'a'; Buffer[Offs++] = 't'; Buffer[Offs++] = 'i'; Buffer[Offs++] = 'c'; Buffer[Offs++] = ' ';}
 Buffer[Offs++] = 'C'; Buffer[Offs++] = 'E'; Buffer[Offs++] = 'n'; Buffer[Offs++] = 'c'; Buffer[Offs++] = 'A'; Buffer[Offs++] = 'r'; Buffer[Offs++] = 'r'; Buffer[Offs++] = '<'; Buffer[Offs++] = 'u'; Buffer[Offs++] = 'n'; Buffer[Offs++] = 's'; Buffer[Offs++] = 'i'; Buffer[Offs++] = 'g'; Buffer[Offs++] = 'n'; Buffer[Offs++] = 'e'; Buffer[Offs++] = 'd'; Buffer[Offs++] = ' ';
 if(BlkLen == 8)
  {
   Buffer[Offs++] = 'd'; Buffer[Offs++] = 'd'; Buffer[Offs++] = 'd'; Buffer[Offs++] = 'd'; Buffer[Offs++] = 'd'; Buffer[Offs++] = 'd'; Buffer[Offs++] = 'd'; Buffer[Offs++] = 'd';
  }
 else if(BlkLen == 4)
  {
   Buffer[Offs++] = 'd'; Buffer[Offs++] = 'd'; Buffer[Offs++] = 'd'; Buffer[Offs++] = 'd'; Buffer[Offs++] = 'd'; Buffer[Offs++] = 'd'; Buffer[Offs++] = 'd'; Buffer[Offs++] = 'd';
  }
 else if(BlkLen == 2)
  {
   Buffer[Offs++] = 's'; Buffer[Offs++] = 'h'; Buffer[Offs++] = 'o'; Buffer[Offs++] = 'r'; Buffer[Offs++] = 't';
  }
 else {Buffer[Offs++] = 'c'; Buffer[Offs++] = 'h'; Buffer[Offs++] = 'a'; Buffer[Offs++] = 'r';}  // 'char' is expected to be 1 byte
 Buffer[Offs++] = '>'; Buffer[Offs++] = ' ';
 if(Name)
  {
   for(;*Name;Name++)Buffer[Offs++] = *Name;
  }
   else {Buffer[Offs++] = 'd'; Buffer[Offs++] = 'a'; Buffer[Offs++] = 't'; Buffer[Offs++] = 'a';}
 Buffer[Offs++] = '('; Buffer[Offs++] = '{'; Buffer[Offs++] = '\r'; Buffer[Offs++] = '\n';
 for(std::uint8_t DataPtr=(std::uint8_t*)Data;DataLen;DataLen--,DataPtr++)
  {
       // Don`t forget padding
  }
 Buffer[Offs++] = ' '; Buffer[Offs++] = '}'; Buffer[Offs++] = ')'; Buffer[Offs++] = ';'; Buffer[Offs++] = '\r'; Buffer[Offs++] = '\n';
 return Offs;
}
//--------------------------------------------------

};
//============================================================================================================

/*
CLANG:
 __builtin_choose_expr
 __builtin_offsetof
 __builtin_FILE
 __builtin_FUNCTION
 __builtin_LINE
 __builtin_COLUMN
 __builtin_types_compatible_p
 __builtin_va_arg
 ...
 __is_base_of
 __is_class
 __is_rvalue_expr
 __is_rvalue_reference
 __is_fundamental
 __is_pointer
 __is_compound
 ...
 __read_only
 __write_only
__read_write
__addrspace_cast
__ptr64
__ptr32
...
__builtin_bit_cast    // constexpr auto one = __builtin_bit_cast(std::uint64_t, 1.0); - OK // constexpr auto ptr = __builtin_bit_cast(void*, (long long)56); - FAIL
__builtin_available
__is_signed
__is_same
*/
//============================================================================================================


};