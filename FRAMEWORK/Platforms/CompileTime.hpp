
#pragma once

namespace NCTM
{
namespace nhidden  // Private stuff
{
//============================================================================================================
//                               Compile time counter implementation
// https://stackoverflow.com/a/63578752
//----  --------------------------------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------------------------------------

static const uint64 ctEncKey = ((uint64)NCRYPT::CRC32(__TIME__ __DATE__) << 32) | NCRYPT::CRC32(__DATE__ __TIME__);  //((~((unsigned int)(__DATE__[4]) * (unsigned int)(__DATE__[5])) & 0xFF) | ((~((unsigned int)(__TIME__[0]) * (unsigned int)(__TIME__[1])) & 0xFF) << 8) | ((~((unsigned int)(__TIME__[3]) * (unsigned int)(__TIME__[4])) & 0xFF) << 16) | ((~((unsigned int)(__TIME__[6]) * (unsigned int)(__TIME__[7])) & 0xFF) << 24));   // DWORD
static const uint64 ExEncKey = 0xA1B2C3D41A2B3C4D;   // TODO: Must be same as a hardware calculated key  // TODO: Move to config

static uint64 MakeExKeyPart(void) // Updates ExEncKeyRT // TODO: Hardware counter based  // TODO: Export from PLATFORM somehow
{
 volatile uint64 val = 0;     // Should be like this until an hardware value is used. Otherwise the decryption will be too optimized and IDA will be able to decrypt all strings
 return ExEncKey ^ val;     // ExEncKeyRT;  (Extern)
}
//============================================================================================================
// Packs bytes so that their in-memory representation will be same on the current platform
template<typename T, typename V> constexpr static T RePackElements(V Arr, unsigned int BLeft)
{
 using ElType = decltype(Arr[0]);
 static_assert(sizeof(T) > sizeof(ElType), "Destination type is smaller!");
 T Result = 0;
 if constexpr (NCFG::IsBigEnd)
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

// Stores a string in code instead of data section(directly in instructions)
//
// NOTE: Even with -O1 CLANG and MSVC can pack the bytes by itself. GCC requires -O2 to pack the bytes. But the string may be put in data section because of XMM instructions
// With manual packing, CLANG with -O2 will also puts the string into data section which makes them perfectly readable there. CLANG does not allow to set optimization modes selectively
// MSVC, CLANG: -O0, -O1 only; GCC -O0, -O1, -O2; Or disable SSE: GCC, CLANG (-mno-sse2); MSVC (???)
//
template<typename T, uint N> struct alignas(8) CPStr
{
 static const uint BytesLen = sizeof(T) * N;
 static const uint ArrSize  = ((BytesLen+(sizeof(uint)-1)) & ~(sizeof(uint)-1)) / sizeof(uint);   //(BytesLen / sizeof(uint)) + (bool)(BytesLen & (sizeof(uint) - 1));      //  AlignP2Frwd(BytesLen, sizeof(uint)); //

 alignas(8) uint Array[ArrSize]; // size_t

 // A type is a literal type if: it has a trivial destructor... // A destructor is trivial if it is not user-provided... // Cannot use with the destructor as an user defined literal!
 //_finline ~CPStr() {for(uint idx=0,vo=N*sizeof(T);idx < ArrSize;vo=Array[idx],idx++)Array[idx] = (Array[idx] * N) ^ vo;}  // Implement stack cleaning on destruction  // <<<<<<<<<<<<< Move this to decrypted string holder
 consteval CPStr(const T* str, uint l) { Init(str, N); }  // Must have an useless arg to be different from another constructor
 consteval CPStr(const T(&str)[N]) { Init(str, N); }
 consteval void Init(const T* str, uint len)
 {
  for(uint sidx = 0, didx = 0; sidx < N; didx++, sidx += sizeof(uint))Array[didx] = RePackElements<uint>(&str[sidx], N - sidx);
 }

 constexpr _finline operator const T* ()  const { return (const T*)this->Array; }
 constexpr _finline const T* Ptr()  const { return (const T*)this->Array; }        // (unsigned char*)"12345"_ps.Ptr()

};

// ps is embedded packed string or EncryptedString using C++20 (Fast, no index sequences required)
template<CPStr str> consteval static const auto operator"" _ps() { return str; }  // 'const auto' or 'auto&&' ???  // must be in a namespace or global scope  // C++20, no inlining required if consteval and MSVC bug is finally fixed   // Examples: auto st = "Hello World!"_ps;  MyProc("Hello World!"_ps);

// Examples: MyProc(_PS("Hello World!"));
// I give up and use the ugly macro(At least it accesses 'str' only once). In C++ we cannot pass constexprness as a function argument and cannot pass 'const char*' as a template argument without complications
//#define _PS(str) ({constexpr auto tsp = (str); CPStr<decltype(CTTypeChr(tsp)), CTStrLen(tsp)>(tsp,1);})   // In most cases this leaves an unreferenced string in data segment (Extra work for optimizer to remove it)
#define _PS(str) NFWK::NCTM::CPStr<decltype(NFWK::NCTM::CTTypeChr(str)), NFWK::NCTM::CTStrLen(str)>(str,1)   // A clean result, but does three accesses to 'str' which may come from '__builtin_FUNCTION()', for example   // Had to use full fixed namespace paths here to be able to use thes macro anywhere

// NOTE: _PS can be encrypted if options is set but _ES is always encrypred
// Encrypted strings should decrypt into a temporary object, not directly inplace(duplicate on stack, but allows them to be global)?

//============================================================================================================
template<typename T, uint N> struct alignas(8) CEStr
{
 using unit = uint;
 static const uint BytesLen = sizeof(T) * N;
 static const uint ArrSize  = ((BytesLen+(sizeof(unit)-1)) & ~(sizeof(unit)-1)) / sizeof(unit);  //  (BytesLen / sizeof(uint)) + (bool)(BytesLen & (sizeof(uint) - 1));    // AlignP2Frwd(BytesLen, sizeof(uint)); //
 static const unit AsyKey   = ctEncKey * BytesLen;

 alignas(8) unit Array[ArrSize]; // size_t    // Some systems require alignment to be 8   // volatile ? // mutable ?
                                        // /*for(uint idx=0,vo=N*sizeof(T);idx < ArrSize;vo=Array[idx],idx++)Array[idx] = ((Array[idx] * N) ^ vo) * (__COUNTER__ * __LINE__);*/
// _finline ~CEStr() {   }  // TODO: Implement stack cleaning on destruction  // <<<<<<<<<<<<< Move this to decrypted string holder
 consteval CEStr(const T* str, uint l) { Init(str, N); }  // Must have an useless arg to be different from another constructor
 consteval CEStr(const T(&str)[N]) { Init(str, N); }
 consteval void Init(const T* str, uint len)
 {
  for(uint sidx = 0, didx = 0, xkey=ctEncKey ^ ExEncKey; sidx < N; didx++, sidx += sizeof(unit), xkey=RotL(xkey, 1))Array[didx] = (RePackElements<unit>(&str[sidx], N - sidx) - AsyKey) ^ RotR(xkey, -didx & 0x0F);
//  static_assert(Array[ArrSize-1] >> ((sizeof(uint)-1)*8) );  // Not constant expression for some reason
 }
//---------------------------------------------------------
 bool _finline IsEncrypted(void) const {return ((char*)&this->Array[ArrSize])[-1];}   // Last byte should be zero when decrypted  // TODO: Make sure that encryption not leaves it as Zero (Xor is safe, +/- not)
//---------------------------------------------------------
_finline  const T* Decrypt(void) const
 {
  volatile uint* arr = (volatile uint*)UnbindPtr(this->Array); // &const_cast<CEStr<T,N>* >(this)->Array[0];  // NOTE: Without 'volatile' this entire function may be optimized away and strings will be left unencrypted
#pragma clang loop unroll(full)  //#pragma unroll  // No effect: it will not unroll unless an optimization mode 1,2 or 3 is enabled
  for(uint ctr=0,xkey=RotR(~((unit)ctEncKey) ^ (unit)MakeExKeyPart(),3);ctr < ArrSize;ctr++, xkey=RotL(xkey, 1))arr[ctr] = (arr[ctr] ^ RotR(~RotL(xkey, 3), -ctr & 0x0F)) + AsyKey;    // Xor key itself is encrypted
  return (const T*)this->Array;    // NOTE: Stays unencrypted // TODO: Return CPStr which can clean itself on destruction.
 }
//---------------------------------------------------------
 T operator [](int idx) const      // TODO: Decrypt by char?  // = delete;   // Why is this operator exist implicitly? It allows accesses as *Val and Val[idx] which is incorrect
 {
  if(idx >= N)return 0;
  if(this->IsEncrypted())this->Decrypt();
  return ((T*)&this->Array)[idx];       // All chars are packed sequentially
 }
//---------------------------------------------------------
 constexpr _finline operator const T* ()  const { return const_cast<CEStr<T,N>* >(this)->Decrypt(); }  // constness of 'this' pointer is removed
// constexpr _finline operator const T* ()  const { return (T*)this->Array; }
 constexpr _finline const T* Ptr()  const { return (T*)this->Array; }
};

template<CEStr str> consteval static const auto operator"" _es() { return str; }

#define _ES(str) NFWK::NCTM::CEStr<decltype(NFWK::NCTM::CTTypeChr(str)), NFWK::NCTM::CTStrLen(str)>(str,1)

//============================================================================================================
// If array type is not UINT8 then last(first?) value have padding (because of alignment) with number of bytes to exclude from full size
// C++17 'Class Template Argument Deduction' is useless here because an extra parameters are needed
// 'DataSize' is real size of data in bytes, not aligned to T
template<typename T, uint N> struct CEncArr
{
 static constexpr uint FullKey  = 0;//MakeUniqueKey(~((sizeof(T)*5) | (N << 23)) * N);  //~((uint)Key * (uint)ExKey);
 static constexpr bool IsBigEnd = false;
 static constexpr int  DataSize = sizeof(T) * N;
 static constexpr int  ArrSize  = ((DataSize+(sizeof(uint)-1)) & ~(sizeof(uint)-1)) / sizeof(uint);   //AlignP2Frwd(DataSize, sizeof(uint)) / sizeof(uint);   //(sizeof(T) < sizeof(void*))?(AlignP2Frwd(sizeof(T) * N, sizeof(uint)) / sizeof(uint)):((sizeof(T) > sizeof(void*))?(N * 2):(N)); // Input array of UINT8, UINT16, UINT32, UINT64 will be packed to SIZE_T and encrypted

 uint Array[ArrSize];   // Type must be always SIZE_T to avoid calling some math functions at runtime decryption  // A source byte array is Compile-time packed to target platform byte-ordered SIZE_T

//------------------------------------------------------------------------------------------------------------
// Highest bit is expected to be a sign bit in signed types
static constexpr _finline uint EncryptDataBlk(const uint Datb, uint Key, uint Idx)
{
 return (uint)((sint)Datb + (sint)RotR(Key, (Idx+1) & 0x1F)) ^ ~(uint)((sint)Key + ~((sint)Idx * (sint)Idx));  // Good enough (Tested on AllZeroes)
}
//------------------------------------------------------------------------------------------------------------
static constexpr _finline uint DecryptDataBlk(const uint Datb, uint Key, uint Idx)
{
 return (uint)((sint)(Datb ^ ~(uint)((sint)Key + ~((sint)Idx * (sint)Idx))) - (sint)RotR(Key, (Idx+1) & 0x1F));
}

public:

// Bytes are expected to be packed as 0xB0B1B2B3, 0xB4B5B6B7 ... . This is reversed in memory on LittleEndian platforms (Intel)
explicit constexpr _finline CEncArr(const T (&arr)[N]): Array{}   // Is it possible to wrap SBlob in some short form with deduction of the type and size?    // No way to pass keys as template arguments and still have it constructible as 'constexpr static CEncArr xx(0xFFFF, 0x2222, {6,8,9})'?
{
 if constexpr (sizeof(T) < sizeof(uint))  // Merge
  {
   int DstIdx = 0;
   int DstNum = 0;
   uint DstVal = 0;
   for(int SrcIdx=0;SrcIdx < N;SrcIdx++)
    {
     DstVal |= arr[SrcIdx];
     DstNum++;
     if(DstNum == (sizeof(uint) / sizeof(T)))
      {
       this->Array[DstIdx++] = EncryptDataBlk(DstVal,FullKey,DstIdx);  // Is DstIdx value here UD? Expected to start with 0
       DstVal = 0;
       DstNum = 0;
      }
       else DstVal <<= (sizeof(T)*8);
    }
   if(DstNum)this->Array[DstIdx] = EncryptDataBlk(DstVal,FullKey,DstIdx);  // Store some leftovers
  }
 else if constexpr (sizeof(T) > sizeof(uint))  // Split
  {
   for(int SrcIdx=0,DstIdx=0;SrcIdx < N;SrcIdx++)
    {
     T SrcVal = arr[SrcIdx];
     this->Array[DstIdx++] = EncryptDataBlk(SrcVal >> (sizeof(uint)*8),FullKey,DstIdx);
     this->Array[DstIdx++] = EncryptDataBlk((uint)SrcVal,FullKey,DstIdx);
    }
  }
 else for(int Idx=0;Idx < N;Idx++)this->Array[Idx] = EncryptDataBlk(arr[Idx],FullKey,Idx);  // Same size
}
//------------------------------------------------------------------------------------------------------------
constexpr _finline uint Size(const bool Encrypted=true)
{
 uint padd = this->Array[ArrSize-1];
 if(Encrypted)padd = DecryptDataBlk(padd, FullKey, ArrSize-1);  // Decrypt the last block to extract a padding value from it
 if constexpr (sizeof(T) < sizeof(uint))padd >>= (sizeof(uint) - sizeof(T)) * 8;       // If merged, skip alignment bytes(No padding is added there)
 return DataSize - (padd & 0xFF);
}
//------------------------------------------------------------------------------------------------------------
constexpr _finline uint BufSize(void){return ArrSize;}  // Returns size of a buffer required for decryption
constexpr _finline void* Data(void){return this->Array;}
//------------------------------------------------------------------------------------------------------------
constexpr _finline void* Encrypt(void* Buf=nullptr, uint Size=0) const  // In case you need to hide an encrypted data on stack or to restore encryption of a global data  // Has to be const if the class ever instantiated as 'constexpr CEncArr MyData(0x5678ba12, 0x34de6795, {1,2,3});'
{
 uint* DstArr = Buf?((uint*)Buf):(const_cast<uint*>(this->Array));   // If the instance declared as global or 'constexpr static' then writing to the array will result in access fault because the data is most likely has been put into a read-only section
 Size = (Size)?(Size / sizeof(uint)):(ArrSize);
 for(uint Idx=0;Idx < Size;Idx++)
  {
   uint val = DstArr[Idx];
   if constexpr (!IsBigEnd)RevByteOrder(val);  // SwapBytes
   DstArr[Idx] = EncryptDataBlk(val,FullKey,Idx);
  }
 return Buf;
}
//------------------------------------------------------------------------------------------------------------
constexpr _finline void* Decrypt(void* Buf=nullptr, uint Size=0) const   // Has to be const if the class ever instantiated as 'constexpr CEncArr MyData(0x5678ba12, 0x34de6795, {1,2,3});'
{
 uint* DstArr = Buf?((uint*)Buf):(const_cast<uint*>(this->Array));   // If the instance declared as global or 'constexpr static' then writing to the array will result in access fault because the data is most likely has been put into a read-only section
 Size = (Size)?(Size / sizeof(uint)):(ArrSize);
 for(uint Idx=0;Idx < Size;Idx++)
  {
   uint val = DecryptDataBlk(this->Array[Idx],FullKey,Idx);
   if constexpr (!IsBigEnd)RevByteOrder(val);  // SwapBytes
   DstArr[Idx] = val;
  }
 return Buf;
}
//------------------------------------------------------------------------------------------------------------
/*uint BuildBlkArrayStr(char* Buffer, void* Data, uint DataLen, int BlkLen=4, int BlkOnLine=64, bool IsStatic=false, char* Name=nullptr)   // TODO: Use embedded strings class
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
} */
//------------------------------------------------------------------------------------------------------------

};
//============================================================================================================
//template<typename T> consteval _finline static char* SigWithTmplParam(void){return FUNC_SIG; };    // Useless for templates
//template<typename T> consteval _finline static char  CurrFuncSigChr(int Idx){return FUNC_SIG[Idx]; };

// Compile-time generator for list of int (0, 1, 2, ...)  // TODO: Use builtins ( MSVC: __make_integer_seq<integer_sequence, _Ty, _Size>;)
//template <int...> struct ctCplIntList {};
//template <typename	IndexList, char Right> struct ctCplAppend;
//template <int... Left,	   char Right> struct ctCplAppend<ctCplIntList<Left...>, Right> { typedef ctCplIntList<Left..., Right> Result; };

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
/*template <typename T, unsigned int O, unsigned int I> struct ctTNChars { typedef typename ctCplAppend<typename ctTNChars<T,O,I - 1>::Result, CurrFuncSigChr<T>((O+I)-1)>::Result Result; };   // Packs  CharAt<L>("Helo",I - 1)
template <typename T, unsigned int O> struct ctTNChars<T,O,0> { typedef ctCplIntList<> Result; };

template <typename ChrLst> struct SChrUnp;
template <char... Idx> struct SChrUnp<ctCplIntList<Idx...> >    // Seems there is no way to use a template function instead
{
 static constexpr _finline const char* Chars(void)
  {
   static const char Array[] = {Idx..., 0};
   return Array;
  }
};

template<typename T> constexpr _finline static int ctTNPos(const char chr, const int offs, const int End){return ((offs < End) && (CurrFuncSigChr<T>(offs) != chr))?(ctTNPos<T>(chr,offs+1,End)):(offs);}
template<typename T> constexpr _finline static int ctTNLen(const int offs){return (CurrFuncSigChr<T>(offs))?(ctTNLen<T>(offs+1)):(offs);}     // Offset included in result
template<typename T> constexpr _finline static int ctTNLenBk(const int offs){return ((offs >= 0) && ((CurrFuncSigChr<T>(offs) > 0x20) || (CurrFuncSigChr<T>(offs-1) == ',')))?(ctTNLenBk<T>(offs-1)):(offs+1);}   // Breaks on any space in case of 'struct SMyStruct' type
template<typename A, typename B> constexpr _finline static int ctTNDif(const int offs){return (CurrFuncSigChr<A>(offs) == CurrFuncSigChr<B>(offs))?(ctTNDif<A,B>(offs+1)):(offs);}   // Offset included in result

struct SCplFuncInfo  // Holds info about a TypeName position in a function signature for current compiler
{
 static constexpr int TypeOffs = ctTNDif<char,long>(0);     // Offset of a type in a string
 static constexpr int TailSize = ctTNLen<char>(TypeOffs+4) - (TypeOffs+4);  // Left of a full string   // 4 is len of 'char' string
};

// Helps to get name of a type without RTTI and RTL
// If Template Params will be included(NoTmpl=false): 'CProp<float,0>'
template<typename T, bool NoTmpl=false> constexpr _finline const char* TypeName(void)  // One instance per requested type, holds only a name
{
 constexpr int End = ctTNLen<T>(SCplFuncInfo::TypeOffs) - SCplFuncInfo::TailSize;   // End if TypeName (Begin for backward reading)
 constexpr int Beg = ctTNLenBk<T>(End);
 constexpr int Pos = (Beg > SCplFuncInfo::TypeOffs)?(Beg):(SCplFuncInfo::TypeOffs);
 constexpr int Ofs = (NoTmpl && (CurrFuncSigChr<T>(End-1) == '>'))?(ctTNPos<T>('<',Pos,End)):(End);      // '<' is expected to be there
 constexpr int Len = Ofs - Pos;
 return SChrUnp<ctTNChars<T,Pos,Len>::Result>::Chars();
}

template<typename A, typename B, bool NoTmpl=false> constexpr _finline bool IsSameTypeNames(void){return (TypeName<A,NoTmpl>() == TypeName<B,NoTmpl>());}
*/
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
