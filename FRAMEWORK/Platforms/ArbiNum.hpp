
#pragma once

// >>>>>>>>>>>>>>>>>>>>>>> OBFUSCATED INTEGER <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

//#pragma intrinsic(_BitScanReverse)
/*
#if defined(_MSC_VER) && !defined(__clang__)
#define REAL_MSVC
#pragma message(">>>> Compiler is MSVC")
#else
#pragma message(">>>> Compiler is CLANG")
#endif

#ifdef REAL_MSVC
#define FUNCSIG __FUNCSIG__
#define  _finline __forceinline
#else
#define FUNCSIG __PRETTY_FUNCTION__
#define  _finline __attribute__((always_inline))
#endif
*/

enum EBbfLevel {
               olNone,      // No obfuscation
               olBasic,     // Strings are obfuscated
               olLight,     // Adds substitution of numbers with their obfuscated implementation
               olStrong,    // Obfusation is enabled everywhere, numbers are obfuscated with binary arithmetics
               olExtreme    // Most heavy variants of obfuscation, if available  (stack should be cleaned after call returns)
};

constexpr int GObfuLevel = olStrong;


namespace NARNUM
{
/*using uint   = unsigned long;
using uint32 = unsigned int;

template<typename Ty> struct RemoveRef { using T = Ty; };
template<typename Ty> struct RemoveRef<Ty&> { using T = Ty; };
template<typename Ty> struct RemoveRef<Ty&&> { using T = Ty; };

template<typename T> constexpr inline static T RotL(T Value, unsigned int Shift){return (Value << Shift) | (Value >> ((sizeof(T) * 8u) - Shift));}
template<typename T> constexpr inline static T RotR(T Value, unsigned int Shift){return (Value >> Shift) | (Value << ((sizeof(T) * 8u) - Shift));}

template<typename N> constexpr inline static N AlignP2Frwd(N Value, unsigned int Alignment){return (Value+((N)Alignment-1)) & ~((N)Alignment-1);}


template<bool UseTypeA, typename A, typename B> struct TSW;                   // Usage: TSW<MyCompileTimeCondition, MyStructA, MyStructB>::T Val;   // Same as std::conditional
template<typename A, typename B> struct TSW<true, A, B> {using T = A;};
template<typename A, typename B> struct TSW<false, A, B> {using T = B;};

template <int Size> struct TypeForSizeU
{
 using T = typename TSW<(Size < 8), typename TSW<Size < 4, typename TSW<Size < 2, unsigned char, unsigned short>::T, unsigned int>::T, unsigned long long>::T;    // without 'typename' the compiler refuses to look for TSW type
};

template<uint32 msk = 0xEDB88320, uint N, uint i = 0> constexpr inline static uint32 CT_CRC32(const char (&str)[N], uint32 crc=0xFFFFFFFF)  // Unrolling bit hashing makes compilation speed OK again   // Only _finline can force it to compile time computation
{
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
   return CT_CRC32<msk, N, i + 1>(str, crc);
  }
 else return ~crc;
}

static constexpr inline unsigned int CT_SEED = CT_CRC32(__DATE__ __TIME__, -1);

consteval unsigned int CT_Random(unsigned int Ctr)
{
 return (1013904323 + 1664625 * (CT_SEED * (Ctr+1)));
}
consteval unsigned int CT_Random(unsigned int Ctr, unsigned int VMin, unsigned int VMax){return (VMin + (CT_Random(Ctr) % (VMax - VMin + 1)));}


//------------------------------------------------------------------------------------
// https://en.cppreference.com/w/cpp/numeric/countl_zero
// https://github.com/nemequ/portable-snippets
//
template<typename T> constexpr static inline int ctz(T Num)
{
#ifdef REAL_MSVC
 unsigned long Index;   // Is it changed by _BitScanForward in case of Zero input? If not we can store (sizeof(Num)*8) in it
 unsigned char res;
 if constexpr (sizeof(T) > sizeof(long))
  {
   if constexpr (__has_builtin(_BitScanForward64))res = _BitScanForward64(&Index, (unsigned long long)Num);    // ARM and AMD64
    else
     {
      res = _BitScanForward(&Index, (unsigned long)Num)
      if(!res)res = _BitScanForward(&Index, (unsigned long)(Num >> 32));
      Index += 32;
     }
  }
   else res = _BitScanForward(&Index, (unsigned long)Num);
 if(res)return Index;    // Found 1 at some position
  else return (sizeof(Num)*8);  // Num is zero, all bits is zero
#else
 if constexpr (sizeof(T) > sizeof(long))return  __builtin_ctzll((unsigned long long)Num);  // X64 CPUs only?
   else return __builtin_ctz((unsigned long)Num);
#endif
// else   // TODO: optimize?
//  {
//    int i = 0;
//	for(;!(Num & 1u); Num >>= 1, i++);
//	return i;
//  }
// return 0;
}
//------------------------------------------------------------------------------------
// https://stackoverflow.com/questions/3849337/msvc-equivalent-to-builtin-popcount
//
template<typename T> constexpr static inline int popcount(T Num)
{
 if constexpr (__has_builtin(__builtin_popcount) && (sizeof(T) <= sizeof(long)))
  {
   return __builtin_popcount((unsigned long)Num);
  }
 else if constexpr (__has_builtin(__builtin_popcountll))
  {
   return __builtin_popcountll((unsigned long long)Num);
  }
 else // UNTESTED!!!  // up to 128 bits    // MSVC does not have '__builtin_popcount' alternative for ARM
 {
  Num = Num - ((Num >> 1) & (T)~(T)0/3);
  Num = (Num & (T)~(T)0/15*3) + ((Num >> 2) & (T)~(T)0/15*3);
  Num = (Num + (Num >> 4)) & (T)~(T)0/255*15;
  return (T)(Num * ((T)~(T)0/255)) >> (sizeof(T) - 1) * 8;  // 8 is number of bits in char
 }
} */
//====================================================================================
// https://www.techtarget.com/whatis/definition/logic-gate-AND-OR-XOR-NOT-NAND-NOR-and-XNOR
struct SBitLogic
{
enum EBitOp
{
 boNone,
// Base logic gates
 boOr,
 boAnd,
 boNot,
// Combined logic gates
 boXOr,
 boNOr,
 boXNOr,
 boNAnd
};

static _finline bool OpNOT(bool V1){return !V1;}
static _finline bool OpOR(bool V1, bool V2){return V1|V2;}
static _finline bool OpAND(bool V1, bool V2){return V1&V2;}
static _finline bool OpXOR(bool V1, bool V2){return V1^V2;}

static _finline bool OpNOR(bool V1, bool V2){return OpNOT(OpOR(V1,V2));}
static _finline bool OpXNOR(bool V1, bool V2){return OpNOT(OpXOR(V1,V2));}
static _finline bool OpNAND(bool V1, bool V2){return OpNOT(OpAND(V1,V2));}

static _finline bool ModBit(EBitOp Op, bool ValA, bool ValB=0)
{
 switch(Op)
  {
   case SBitLogic::boNot:  {return SBitLogic::OpNOT( ValA); break;}      // Input ValB is ignored
   case SBitLogic::boOr:   {return SBitLogic::OpOR(  ValA, ValB); break;}
   case SBitLogic::boAnd:  {return SBitLogic::OpAND( ValA, ValB); break;}
   case SBitLogic::boXOr:  {return SBitLogic::OpXOR( ValA, ValB); break;}
   case SBitLogic::boNOr:  {return SBitLogic::OpNOR( ValA, ValB); break;}
   case SBitLogic::boXNOr: {return SBitLogic::OpXNOR(ValA, ValB); break;}
   case SBitLogic::boNAnd: {return SBitLogic::OpNAND(ValA, ValB); break;}
  }
 return 0;
}
//------------------------------------------------------------------------------------

};
//====================================================================================
// Bitset static Interface (Should be optional, for debug or test builds)
template<typename D> class CBSBase
{
protected:
CBSBase(void)
{  // No specific signatures, but some test arguments are possible (And reqiured to compile with GCC, but Clang and MSVC are OK)
 static_assert(requires(D&& v) { v.OffsForIdx; });
 static_assert(requires(D&& v) { v.Get; });
 static_assert(requires(D&& v) { v.Set; });
 static_assert(requires(D&& v) { v.SetBits; });
 static_assert(requires(D&& v) { v.GetBits; });
 static_assert(requires(D&& v) { v.GetNext; });
 static_assert(requires(D&& v) { v.GetPrev; });
 static_assert(requires(D&& v) { v.SetNext; });
 static_assert(requires(D&& v) { v.SetPrev; });
 static_assert(requires(D&& v) { v.GetBitAt; });
 static_assert(requires(D&& v) { v.SetBitAt; });
}
};

// https://github.com/lemire/EWAHBoolArray/blob/master/include/ewah/ewah.h  // For example of a compressed bitset
//------------------------------------------------------------------------------------
// NOTE: use only builtin types in case for substitution of custom types with this implementation
// First arg is for the bitset size in multiple of 8 bits. Had to be a type for constructor deduction to work
// One byte per bit. DP is type size in which bytes are packed. Size is in bits
//
template<unsigned int S=sizeof(void*)*8, typename DP=unsigned int> class CObfBitset  //: public CBSBase<CObfBitset<T,DP>>                                                    // <int Size=4, typename DT=unsigned char>
{
 static constexpr unsigned int UnitMsk = sizeof(DP)-1;         // To extract byte index in Unit
 static constexpr unsigned int UnitSft = ((sizeof(DP) > 1)?( ((sizeof(DP) > 2)?( ((sizeof(DP) > 4)?( 3 ):(2)) ):(1)) ):(0));   // To extract Unit index from Idx
public:
 static constexpr unsigned int MaxBits     = S;     // Stop any operations on this to allow expected overflow behaviour on derived types?
 static constexpr unsigned int SizeInBits  = AlignP2Frwd(MaxBits, sizeof(DP));   // AlignP2Frwd(sizeof(T)*8, sizeof(DP));  // Bits must fit in DP units as bytes
 static constexpr unsigned int SizeInUnits = SizeInBits / sizeof(DP);
 static constexpr unsigned int SizeInBytes = SizeInBits / 8;      // Useless. 4 for int32 and so on

private:
 static constexpr unsigned int BitRange    = 8;           // One byte
 static constexpr unsigned int OffsMsk     = BitRange-1;  // 7 bits, single byte
 static constexpr unsigned int BitShift    = 3;           // Adjustable. Shift of bit value position, relative to DT index
 static constexpr unsigned int RotDir      = 1;           // Adjustable. Left or Right

 DP Data[SizeInUnits];   // One byte stores one bit (for obfuscation). Size is in units  // Format is LittleEndian   // Initialized to random
//------------------------------------------------------------------------------------
template<typename X> consteval static bool IsBitsetType(void) {return sizeof(X) > 8;}   //     {return requires(const X&& v) { sizeof(v.Data); };}    // Cannot recognize v.IsBitsetType or X::IsBitsetType !!!
//------------------------------------------------------------------------------------
consteval static auto GetMaxType(void)       // Usage: struct SMyStruct { decltype(TypeSwitch<MyCompileTimeCondition, MyStructA, MyStructB>()) Val; }
{
 if constexpr (SizeInBytes == sizeof(char)) {unsigned char val{0}; return val;}
 else if constexpr (SizeInBytes == sizeof(short)) {unsigned short val{0}; return val;}
 else if constexpr (SizeInBytes == sizeof(int)) {unsigned int val{0}; return val;}
  else {unsigned long long val{0}; return val;}
}
//------------------------------------------------------------------------------------
// Return a byte with halves in range 1 - 14
consteval static unsigned char NextRndByte(unsigned int Idx, unsigned int& Val)
{
 constexpr unsigned int TheID = NCRYPT::CRC32(FUNC_SIG);    // __func__ skips template parameters, use __PRETTY_FUNCTION__ instead (__FUNCSIG__ in MSVC)
 unsigned char res = 0;
 for(int ctr=2,trp=8;ctr && trp;)
  {
   if(!Val){Val = NCRYPT::CT_Random(Idx) ^ TheID; trp--;}
   char v = Val & 0x0F;
   if(v && (v < 0x0F))
    {
     res <<= 4;
     res  |= v;
     ctr--;
    }
   Val >>= 4;
  }
 return res;
}
//------------------------------------------------------------------------------------
consteval _finline void InitArray(unsigned int Base)
{
 unsigned int Val = 0;
 for(unsigned int idx=0;idx < SizeInUnits;idx++)
  {
   DP unit = 0;
   for(unsigned int ctr=0;ctr < sizeof(DP);ctr++)
    {
     unit <<= 8;
     unit |= NextRndByte(idx+Base, Val);  // 0x11 for debugging
    }
   this->Data[idx] = unit;
  }
}
//------------------------------------------------------------------------------------

public:


consteval CObfBitset(void) {this->InitArray(__COUNTER__ + 1);}        // NOTE: The counter must be in the constructor or it will be same for every instance
//consteval CObfBitset(const T&& Val) {this->InitArray(__COUNTER__ + 1); this->SetBits(Val);}
//_finline CObfBitset(volatile T& Val){InitArray(__COUNTER__ + 1); this->SetBits(Val);}
// TODO: From array and any integer type
//------------------------------------------------------------------------------------
// Returns data idx and relative bit idx for full bit idx
static constexpr _finline unsigned int OffsForIdx(unsigned int Idx){return ((Idx+1)*BitShift) & OffsMsk;}
//------------------------------------------------------------------------------------
// TODO: Total bits/Last nonzero bit (for Add,Sub)
//
//------------------------------------------------------------------------------------
// Fast
_finline bool Get(unsigned int Idx) const
{
 return this->GetBitAtPos(Idx, OffsForIdx(Idx));
}
//------------------------------------------------------------------------------------
// Slow. Rotate the byte to get required bit value at its offset
constexpr _finline void Set(bool Val, unsigned int Idx)
{
 this->SetBitAtPos(Val, Idx, OffsForIdx(Idx));
}
//------------------------------------------------------------------------------------
constexpr _finline void Mod(bool Val, SBitLogic::EBitOp Op, unsigned int Idx)
{
 this->ModBitAtPos(Val, Op, Idx, OffsForIdx(Idx));
}
//------------------------------------------------------------------------------------
// Set bits from Val. Intentionally allowed to set even if the rest of bits in Val are zero
// TODO: Separate processing for an array as Val
template<typename B> constexpr _finline void SetBits(B Val, unsigned int Cnt=SizeInBits, unsigned int From=0)
{
 if(From >= SizeInBits)return;
 if((From+Cnt) > SizeInBits)Cnt = SizeInBits - From;
 for(unsigned int idx=From,end=From+Cnt,offs=OffsForIdx(idx);idx < end;Val>>=1)this->SetNext(Val & 1, idx, offs);    // Extra bits will be zero
}
//------------------------------------------------------------------------------------
template<typename B> constexpr _finline void ModBits(B Val, SBitLogic::EBitOp Op, unsigned int Cnt=SizeInBits, unsigned int From=0)
{
 if(From >= SizeInBits)return;
 if((From+Cnt) > SizeInBits)Cnt = SizeInBits - From;
 for(unsigned int idx=From,end=From+Cnt,offs=OffsForIdx(idx);idx < end;Val>>=1)this->ModNext(Val & 1, Op, idx, offs);    // Extra bits will be zero
}
//------------------------------------------------------------------------------------
template<typename B> constexpr _finline int CompareBits(B Val, unsigned int Cnt=SizeInBits, unsigned int From=0) const
{
// if(From >= SizeInBits)return;
// if((From+Cnt) > SizeInBits)Cnt = SizeInBits - From;
// for(unsigned int idx=From,end=From+Cnt,offs=OffsForIdx(idx);idx < end;Val>>=1)this->ModNext(Val & 1, op, idx, offs);    // Extra bits will be zero
 return 0;
}
//------------------------------------------------------------------------------------
// Get bits as Val. Extracts lower bits if the output value is smaller
template<typename B=decltype(GetMaxType())> _finline B GetBits(unsigned int Cnt=SizeInBits, unsigned int From=0) const   // Should never be constexpr if obfuscation is required (Can the compiler restore original values at compile time?)
{
 if(From >= SizeInBits)return 0;
 if((From+Cnt) > SizeInBits)Cnt = SizeInBits - From;
 B Val = 0;   //  decltype(GetMaxType()) Val = 0;
 unsigned int BitLen = sizeof(Val)*8;
 if(BitLen > Cnt)BitLen = Cnt;
 for(unsigned int idx=(From+BitLen)-1,end=idx-BitLen,offs=OffsForIdx(idx);idx != end;)Val = (Val << 1)|(unsigned int)this->GetPrev(idx, offs);
 return Val;
}
//------------------------------------------------------------------------------------
constexpr _finline void SetFrom(auto&& Val, unsigned int SrcCnt=(unsigned int)-1, unsigned int SrcIdx=0, unsigned int DstCnt=SizeInBits, unsigned int DstIdx=0)
{
 static_assert(IsBitsetType<decltype(Val)>(), "Only a BitSet is allowed!");
 if(DstIdx >= SizeInBits)return;
 if((DstIdx+DstCnt) > SizeInBits)DstCnt = SizeInBits - DstIdx;

 if(SrcIdx >= Val.SizeInBits)return;
 if((SrcIdx+SrcCnt) > SizeInBits)SrcCnt = SizeInBits - SrcIdx;

 for(unsigned int dend=DstIdx+DstCnt,doffs=OffsForIdx(DstIdx),soffs=Val.OffsForIdx(SrcIdx);DstIdx < dend;)this->SetNext(Val.GetNext(SrcIdx, soffs), DstIdx, doffs);
}
//------------------------------------------------------------------------------------
// NOTE: We may want to init a new bitset as a result this so it is better to have the source is separate
constexpr _finline void ModWith(auto&& Val, SBitLogic::EBitOp Op, unsigned int SrcCnt=(unsigned int)-1, unsigned int SrcIdx=0, unsigned int DstCnt=SizeInBits, unsigned int DstIdx=0)
{
 static_assert(IsBitsetType<decltype(Val)>(), "Only a BitSet is allowed!");
 if(DstIdx >= SizeInBits)return;
 if((DstIdx+DstCnt) > SizeInBits)DstCnt = SizeInBits - DstIdx;

 if(SrcIdx >= Val.SizeInBits)return;
 if((SrcIdx+SrcCnt) > SizeInBits)SrcCnt = SizeInBits - SrcIdx;

 for(unsigned int dend=DstIdx+DstCnt,doffs=OffsForIdx(DstIdx),soffs=Val.OffsForIdx(SrcIdx);DstIdx < dend;)this->ModNext(Val.GetNext(SrcIdx, soffs), Op, DstIdx, doffs);
}
//------------------------------------------------------------------------------------
constexpr _finline int CompareWith(auto&& Val, unsigned int Count=SizeInBits, unsigned int SrcIdx=0, unsigned int DstIdx=0) const
{
/* static_assert(IsBitsetType<decltype(Val)>(), "Only a BitSet is allowed!");    // Xor result with sign bit?
 if(DstIdx >= SizeInBits)return;
 if((DstIdx+DstCnt) > SizeInBits)DstCnt = SizeInBits - DstIdx;

 if(SrcIdx >= Val.SizeInBits)return;
 if((SrcIdx+SrcCnt) > SizeInBits)SrcCnt = SizeInBits - SrcIdx;

 for(unsigned int dend=DstIdx+DstCnt,doffs=OffsForIdx(DstIdx),soffs=Val.OffsForIdx(SrcIdx);DstIdx < dend;)this->ModNext(Val.GetNext(SrcIdx, soffs), Op, DstIdx, doffs); */
 return 0;
}
//------------------------------------------------------------------------------------
constexpr _finline void Fill(bool Val, unsigned int Cnt=SizeInBits, unsigned int From=0)
{
 if(From >= SizeInBits)return;
 if((From+Cnt) > SizeInBits)Cnt = SizeInBits - From;
 for(unsigned int idx=From,end=From+Cnt,offs=OffsForIdx(idx);idx < end;)this->SetNext(Val, idx, offs);    // Extra bits will be zero
}
//------------------------------------------------------------------------------------
constexpr _finline void Invert(void)
{
 for(unsigned int idx=0,end=SizeInBits,offs=OffsForIdx(idx);idx < end;)this->SetNext(!this->GetBitAt(idx, offs), idx, offs);
}
//------------------------------------------------------------------------------------
constexpr _finline void Revert(void){}   // <<<<<< TODO
// RotL/R
// ShiftL/R
//------------------------------------------------------------------------------------
constexpr _finline void Reset(void){return this->Fill(0);}
//------------------------------------------------------------------------------------
static constexpr _finline void StepNext(unsigned int& Idx, unsigned int& Offs) {Offs = (Offs + BitShift) & OffsMsk; Idx++;}
static constexpr _finline void StepPrev(unsigned int& Idx, unsigned int& Offs) {Offs = (Offs - BitShift) & OffsMsk; Idx--;}
//------------------------------------------------------------------------------------
// Initial offs is 0
_finline bool GetNext(unsigned int& Idx, unsigned int& Offs) const
{
 bool res = this->GetBitAt(Idx, Offs);
 StepNext(Idx, Offs);
 return res;
}
//------------------------------------------------------------------------------------
_finline bool GetPrev(unsigned int& Idx, unsigned int& Offs) const
{
 bool res = this->GetBitAt(Idx, Offs);
 StepPrev(Idx, Offs);
 return res;
}
//------------------------------------------------------------------------------------
constexpr _finline void SetNext(bool Val, unsigned int& Idx, unsigned int& Offs)
{
 this->SetBitAt(Val, Idx, Offs);
 StepNext(Idx, Offs);
}
//------------------------------------------------------------------------------------
constexpr _finline void SetPrev(bool Val, unsigned int& Idx, unsigned int& Offs)
{
 this->SetBitAt(Val, Idx, Offs);
 StepPrev(Idx, Offs);
}
//------------------------------------------------------------------------------------
constexpr _finline void ModNext(bool Val, SBitLogic::EBitOp Op, unsigned int& Idx, unsigned int& Offs)
{
 this->ModBitAt(Val, Op, Idx, Offs);
 StepNext(Idx, Offs);
}
//------------------------------------------------------------------------------------
constexpr _finline void ModPrev(bool Val, SBitLogic::EBitOp Op, unsigned int& Idx, unsigned int& Offs)
{
 this->ModBitAt(Val, Op, Idx, Offs);
 StepPrev(Idx, Offs);
}
//------------------------------------------------------------------------------------
_finline bool GetBitAt(unsigned int Idx, unsigned int Offs) const
{
 if(Idx >= SizeInBits)return false;        // Useful when copying
 unsigned int  uidx = Idx >> UnitSft;
 unsigned int  bidx = Idx & UnitMsk;
 unsigned char bval = (this->Data[uidx] >> (bidx << 3)); // *8
 return bval & (1 << Offs);
}
//------------------------------------------------------------------------------------
// The byte is rotated until there is same bit as Val at offset Offs
constexpr _finline bool SetBitAt(bool Val, unsigned int Idx, unsigned int Offs)
{
 if(Idx >= SizeInBits)return Val;
 unsigned int  uidx  = Idx >> UnitSft;
 unsigned int  bidx  = Idx & UnitMsk;
 unsigned int  bofs  = (bidx << 3);   // *8
 DP Unit = this->Data[uidx];
 unsigned char bval  = (Unit >> bofs);
 if constexpr (GObfuLevel >= olExtreme)
  {
   unsigned char mval  = (Val)?(~bval):(bval);  // Always scan for nearest 1
   if(!(mval & (1 << Offs)))return Val;    // No change  // NOTE: More secure is to always rotate(Rotate 1 bit first to begin rotation)
   int  mstp  = (Offs >= 4)?(-1):(1);
   unsigned int  NOffs = Offs;
   for(unsigned int ctr=4;ctr && (mval & (1 << NOffs));ctr--)NOffs = (unsigned int)((int)NOffs + mstp);  // NOTE: Cannot unroll, condition depends on result  // Scan left or right from Offs position
   if(NOffs > Offs)bval = RotR(bval, NOffs - Offs);
   else bval = RotL(bval, Offs - NOffs);
  }
   else
    {
     if(!(((bval >> Offs) ^ Val) & 1))return Val;  // No change
     bval = ~bval;
    }
 this->Data[uidx] = (Unit & ~(0xFF << bofs))|(bval << bofs);
 return Val;
}
//------------------------------------------------------------------------------------
constexpr _finline bool ModBitAt(bool Val, SBitLogic::EBitOp Op, unsigned int Idx, unsigned int Offs)
{
 return this->SetBitAt(SBitLogic::ModBit(Op,this->GetBitAt(Idx, Offs),Val), Idx, Offs);
}
//------------------------------------------------------------------------------------
// TODO: Shift/Rotate
//------------------------------------------------------------------------------------
// Retrns 1 if all bits are 1, zero if all bits are 0 and -1 otherwise
/*int State(void)
{
  return 0;
}
//------------------------------------------------------------------------------------
// Returns index of first set bit counting from low, -1(Biggar than Size) if none
unsigned int Find(void)
{
  return 0;
} */
//------------------------------------------------------------------------------------
// TODO: Add operations with another bit vector
};

//====================================================================================
// char[4]
// https://www.learncpp.com/cpp-tutorial/class-template-argument-deduction-ctad-and-deduction-guides/
// HOW TO PASS BS AND KEEP DEDUCTION OF T ?????????
// Class Template Argument Deduction (CTAD) ???
// Specialization?
//
// Not a BigNum, just a integer emulation. Takes its size from T
//
template<typename T=unsigned long long, template<unsigned int,class> class BS=CObfBitset, typename DP=unsigned int> class CBinNum
{
 using Self = CBinNum<T,BS,DP>;
 template<typename G, template<unsigned int,class> class XY, typename ZX> friend class CBinNum;  // Being forced to make it a friend with itself to access private members of another instances is just another stupid C++ thing
 static constexpr unsigned int SizeInBits = sizeof(T)*8;
 static constexpr bool IsSigned = (T(-1) < T(0));  // If try then high bit is reserved for a sign
 BS<SizeInBits, DP> bitv;

//------------------------------------------------------------------------------------
template<typename X> consteval static bool IsBinNumType(void){return sizeof(X) > 8;}     //    {return requires(const X& v) { sizeof(v.bitv); };}   // It CONSTANTLY evades this check!
//------------------------------------------------------------------------------------
template<typename X> consteval static unsigned int TypeSizeInBits(void){if constexpr(IsBinNumType<X>())return X::SizeInBits; else return sizeof(X)*8;}  // NOTE: Ref must be removed
//------------------------------------------------------------------------------------
static _finline bool GetBitAt(unsigned int Idx, const auto& Val)
{
 return Val & (1 << Idx);
}
//------------------------------------------------------------------------------------
static _finline void SetBitAt(bool Bit, unsigned int Idx, auto& Val)
{
 Val = (Val & ~(1 << Idx)) | ((T)Bit << Idx);
}
//------------------------------------------------------------------------------------
// Signed and unsigned
// TODO: Test with different sizes of V1 and V2
static _finline auto BinAdd(auto& Val1, const auto& Val2) // -> decltype(Val1)
{
 using TypeV1 = typename RemoveRef<decltype(Val1)>::T;
 using TypeV2 = typename RemoveRef<decltype(Val2)>::T;
 constexpr const bool V1IsBN = IsBinNumType<TypeV1>();
 constexpr const bool V2IsBN = IsBinNumType<TypeV2>();
 constexpr const int BitLen1 = TypeSizeInBits<TypeV1>();
 constexpr const int BitLen2 = TypeSizeInBits<TypeV2>();
 constexpr const int MBitLen = (BitLen1 < BitLen2)?BitLen1:BitLen2;     // Min bit size

 unsigned int Summ = 0;
 unsigned int bidx1, bidx2, boffs1, boffs2;
 if constexpr(V1IsBN){bidx1=0;boffs1=Val1.bitv.OffsForIdx(bidx1);}
 if constexpr(V2IsBN){bidx2=0;boffs2=Val2.bitv.OffsForIdx(bidx2);}
 for(int idx=0;idx < MBitLen;idx++)  // No shifts and because of that we can`t see if there are any nonzero bits left
  {
   char BitA, BitB;
   if constexpr(V1IsBN)BitA = Val1.bitv.GetBitAt(bidx1,boffs1);   // Dst, no increment
    else BitA = GetBitAt(idx, Val1);
   if constexpr(V2IsBN)BitB = Val2.bitv.GetNext(bidx2,boffs2);
    else BitB = GetBitAt(idx, Val2);
   Summ += (unsigned int)(BitA + BitB);
   char BitV = Summ & 1;
   if(BitA ^ BitV)
    {
     if constexpr(V1IsBN)Val1.bitv.SetNext(BitV, bidx1,boffs1);
       else SetBitAt(BitV, idx, Val1);    // Set if not same
    }
     else if constexpr(V1IsBN)Val1.bitv.StepNext(bidx1, boffs1);
   Summ -= (bool)Summ;
  }
 return Val1;
}
//------------------------------------------------------------------------------------
// Signed and unsigned
// TODO: Thirg argument as DST
static auto BinSub(auto& Val1, const auto& Val2) //-> decltype(Val1)
{
 using TypeV1 = typename RemoveRef<decltype(Val1)>::T;
 using TypeV2 = typename RemoveRef<decltype(Val2)>::T;
 constexpr const bool V1IsBN = IsBinNumType<TypeV1>();
 constexpr const bool V2IsBN = IsBinNumType<TypeV2>();
 constexpr const int BitLen1 = TypeSizeInBits<TypeV1>();
 constexpr const int BitLen2 = TypeSizeInBits<TypeV2>();
 constexpr const int MBitLen = (BitLen1 < BitLen2)?BitLen1:BitLen2;     // Min bit size

 char Carry = 0;
 unsigned int bidx1, bidx2, boffs1, boffs2;
 if constexpr(V1IsBN){bidx1=0;boffs1=Val1.bitv.OffsForIdx(bidx1);}
 if constexpr(V2IsBN){bidx2=0;boffs2=Val2.bitv.OffsForIdx(bidx2);}
 for(int idx=0;idx < MBitLen;idx++)  // No shifts and because of that we can`t see if there are any nonzero bits left
  {
   char BitA, BitB;
   if constexpr(V1IsBN)BitA = Val1.bitv.GetBitAt(bidx1,boffs1);   // Dst, no increment
    else BitA = GetBitAt(idx, Val1);
   if constexpr(V2IsBN)BitB = Val2.bitv.GetNext(bidx2,boffs2);
    else BitB = GetBitAt(idx, Val2);
   char Temp = (BitA - (BitB+Carry));
   char BitV = Temp & 1;
   Carry = (Temp & 2) >> 1;    // Carry
   if(BitA ^ BitV)
    {
     if constexpr(V1IsBN)Val1.bitv.SetNext(BitV, bidx1,boffs1);
       else SetBitAt(BitV, idx, Val1);    // Set if not same
    }
     else if constexpr(V1IsBN)Val1.bitv.StepNext(bidx1, boffs1);
  }
 return Val1;
}
//------------------------------------------------------------------------------------

public:
consteval CBinNum(void) {}    // Leave the value undefined by default (same as native C types)
constexpr _finline CBinNum(const T& Val) { this->bitv.SetBits(Val); }   // consteval    // NOTE: Make sure that constants like CBinNum(0x35) initialized at compile time!  // consteval constructor is always selected and causes errors
//_finline  CBinNum(volatile const T& Val){ this->bitv.SetBits(Val); }
//_finline  CBinNum(CBinNum& Val){ this->bitv.SetFrom(Val.bitv); }
_finline  CBinNum(const auto& Val){if constexpr(IsBinNumType<decltype(Val)>())this->bitv.SetFrom(Val.bitv); else this->bitv.SetBits(Val);}  //     {static_assert(IsBinNumType<decltype(Val)>()); this->bitv.SetFrom(Val.bitv); } //      if constexpr(IsBinNumType<decltype(Val)>())this->bitv.SetFrom(Val.bitv); else this->bitv.SetBits(Val);}
//------------------------------------------------------------------------------------
constexpr _finline operator auto () const {return this->bitv.template GetBits<T>();}     // Always return as base type      // explicit operator T ()

constexpr _finline operator void* () const {return (void*)this->bitv.template GetBits<T>();}
constexpr _finline operator char* () const {return (char*)this->bitv.template GetBits<T>();}
constexpr _finline operator unsigned char* () const {return (unsigned char*)this->bitv.template GetBits<T>();}
//------------------------------------------------------------------------------------
// Declating all operators as 'friend' and using 'auto&' for both argiments helps to define each operator only once:
//  NARNUM::operator+<int, NARNUM::CBinNum<int>>
//  NARNUM::operator+<NARNUM::CBinNum<int>, int>
// TODO: Make sure that operations are mede with an integers, when available (optimization)
//
constexpr _finline Self& operator~() {this->bitv.Invert(); return *this;}

constexpr _finline friend Self operator|(const auto& lhv, const auto& rhv) {Self tmp(lhv); if constexpr(IsBinNumType<decltype(rhv)>())tmp.bitv.ModWith(rhv.bitv, SBitLogic::boOr); else tmp.bitv.ModBits(rhv, SBitLogic::boOr); return tmp;}

constexpr _finline friend Self operator&(const auto& lhv, const auto& rhv) {Self tmp(lhv); if constexpr(IsBinNumType<decltype(rhv)>())tmp.bitv.ModWith(rhv.bitv, SBitLogic::boAnd); else tmp.bitv.ModBits(rhv, SBitLogic::boAnd); return tmp;}

constexpr _finline friend Self operator^(const auto& lhv, const auto& rhv) {Self tmp(lhv); if constexpr(IsBinNumType<decltype(rhv)>())tmp.bitv.ModWith(rhv.bitv, SBitLogic::boXOr); else {static_assert(sizeof(rhv) <= 8); tmp.bitv.ModBits(rhv, SBitLogic::boXOr);} return tmp;}

constexpr _finline friend Self operator+(const auto& lhv, const auto& rhv) {Self tmp(lhv); BinAdd(tmp, rhv); return tmp;}

constexpr _finline friend Self operator-(const auto& lhv, const auto& rhv) {Self tmp(lhv); BinSub(tmp, rhv); return tmp;}

constexpr _finline Self& operator|=(const auto& rhv) {if constexpr(IsBinNumType<decltype(rhv)>())this->bitv.ModWith(rhv.bitv, SBitLogic::boOr); else this->bitv.ModBits(rhv, SBitLogic::boOr); return *this;}

constexpr _finline Self& operator&=(const auto& rhv) {if constexpr(IsBinNumType<decltype(rhv)>())this->bitv.ModWith(rhv.bitv, SBitLogic::boAnd); else this->bitv.ModBits(rhv, SBitLogic::boAnd); return *this;}

constexpr _finline Self& operator^=(const auto& rhv) {if constexpr(IsBinNumType<decltype(rhv)>())this->bitv.ModWith(rhv.bitv, SBitLogic::boXOr); else this->bitv.ModBits(rhv, SBitLogic::boXOr); return *this;}

constexpr _finline Self& operator+=(const auto& rhv) {BinAdd(*this, rhv); return *this;}

constexpr _finline Self& operator-=(const auto& rhv) {BinSub(*this, rhv); return *this;}

//constexpr _finline Self& operator*=(const auto& rhv) {m_value = rhs.value() * m_value; return *this;}

constexpr _finline Self& operator=(const auto& rhv) {if constexpr(IsBinNumType<decltype(rhv)>())this->bitv.SetFrom(rhv); else this->bitv.SetBits(rhv); return *this;} // const?

constexpr _finline Self& operator++() {BinAdd(*this, 1); return *this;}      // Pre

constexpr _finline Self& operator--() {BinSub(*this, 1); return *this;}      // Pre

constexpr _finline Self operator++(int) {Self tmp(*this); BinAdd(tmp, 1); return tmp;}   // Post

constexpr _finline Self operator--(int) {Self tmp(*this); BinSub(tmp, 1); return tmp;}   // Post


//template <typename T> constexpr _finline bool operator==(const CNNInt<T>& lhv, const CNNInt<T>& rhv) {return lhs.value() == rhs.value();}
//template <typename T> constexpr _finline bool operator!=(const CNNInt<T>& lhv, const CNNInt<T>& rhv) {return !operator==(lhs, rhs);}
//template <typename T> constexpr _finline bool operator< (const CNNInt<T>& lhv, const CNNInt<T>& rhv) {return lhs.value() < rhs.value();}
//template <typename T> constexpr _finline bool operator> (const CNNInt<T>& lhv, const CNNInt<T>& rhv) {return operator<(rhs, lhs);}
//template <typename T> constexpr _finline bool operator<=(const CNNInt<T>& lhv, const CNNInt<T>& rhv) {return !operator>(lhs, rhs);}
//template <typename T> constexpr _finline bool operator>=(const CNNInt<T>& lhv, const CNNInt<T>& rhv) {return !operator<(lhs, rhs);}
//------------------------------------------------------------------------------------

/*
//------------------------------------------------------------------------------------
constexpr _finline Self& operator+=(auto rhs)
{
 //   *this = *this + rhs;
 return *this;
}
//------------------------------------------------------------------------------------
constexpr _finline Self& operator-=(auto rhs)
{
 //   *this = *this + rhs;
 return *this;
}
//------------------------------------------------------------------------------------
*/
/*
integer operator-(integer rhs)

// Bitwise Operators
  integer operator&(integer rhs)
integer operator|(integer rhs)
 integer operator^(integer rhs)

  integer operator&=(integer rhs){
    *this = *this & rhs;
    return *this;
  }

  integer operator|=(integer rhs){
    *this = *this | rhs;
    return *this;
  }

  integer operator^=(const integer rhs){
    *this = *this ^ rhs;
    return *this;
  }

  integer operator~(){

  // Bit Shift Operators
  // left bit shift. sign is maintained
 integer operator<<(uint64_t shift)
integer operator<<(integer shift)
  // right bit shift. sign is maintained
  integer operator>>(uint64_t shift)

  // Logical Operators
  bool operator!(){
    return !(bool) *this;
  }

  bool operator&&(integer rhs){
    return (bool) *this && (bool) rhs;
  }

  bool operator||(integer rhs){
    return ((bool) *this) || (bool) rhs;
  }

  // Comparison Operators
  bool operator==(integer rhs){
    return ((SIGN == rhs.SIGN) && (value == rhs.value));
  }

  bool operator!=(integer rhs){
    return !(*this == rhs);
  }

bool operator>(integer rhs)
 bool operator>=(integer rhs)
 bool operator<(integer rhs)
  bool operator<=(integer rhs)
 integer operator+(integer rhs)

 integer operator*(integer rhs)

  integer operator*=(integer rhs){
    *this = *this * rhs;
    return *this;
  }

 integer operator/(integer rhs)

  integer operator/=(integer rhs){
    *this = *this / rhs;
    return *this;
  }

 integer operator%(integer rhs)
  integer operator%=(integer rhs){
    *this = *this % rhs;
    return *this;
  }

  // Increment Operator
  integer & operator++(){  // Pre increment
    *this += 1;
    return *this;
  }

  integer operator++(int){  // Post increment
    integer temp(*this);
    ++*this;
    return temp;
  }

  // Decrement Operator
  integer & operator--(){
    *this -= 1;
    return *this;
  }

  integer operator--(int){
    integer temp(*this);
    --*this;
    return temp;
  }

  // Nothing done since promotion doesnt work here
  integer operator+(){
    return *this;
  }

  // Flip Sign
  integer operator-(){
    integer out = *this;
    if (out.value.size())
      out.SIGN ^= true;
    return out;
  }

*/
};
//====================================================================================


};

static_assert(sizeof(char)==1);
static_assert(sizeof(short)==2);
static_assert(sizeof(int)==4);
static_assert(sizeof(long long)==8);

using xbni8  = NARNUM::CBinNum<char,NARNUM::CObfBitset>;
using xbnu8  = NARNUM::CBinNum<unsigned char,NARNUM::CObfBitset>;
using xbni16 = NARNUM::CBinNum<short,NARNUM::CObfBitset>;
using xbnu16 = NARNUM::CBinNum<unsigned short,NARNUM::CObfBitset>;
using xbni32 = NARNUM::CBinNum<int,NARNUM::CObfBitset>;
using xbnu32 = NARNUM::CBinNum<unsigned int,NARNUM::CObfBitset>;
using xbni64 = NARNUM::CBinNum<long long,NARNUM::CObfBitset>;
using xbnu64 = NARNUM::CBinNum<unsigned long long,NARNUM::CObfBitset>;
using xbnptr = NARNUM::CBinNum<unsigned long,NARNUM::CObfBitset>;     // Same as size_t
