
#pragma once

// NOTE: This module is separate from Common.hpp because of type visibility issues
// NOTE:  Dump here any implementations that should be accessible early and have no personal HPP yet >>>

//------------------------------------------------------------------------------------------------------------
// Some type traits

template<typename T, typename U> struct SameTypes {enum { value = 0 };};   // Cannot be a member of a local class or a function
template<typename T> struct SameTypes<T, T> {enum { value = 1 };};
template<typename A, typename B> constexpr _finline static bool IsSameTypes(A ValA, B ValB)
{
 return SameTypes<A, B>::value;
}

template<typename T> struct Is_Pointer { static const bool value = false; };
template<typename T> struct Is_Pointer<T*> { static const bool value = true; };
//------------------------------------------------------------------------------------------------------------

template<typename T> constexpr _finline static T SwapBytes(T Value)  // Unsafe with optimizations?
{
 uint8* SrcBytes = (uint8*)&Value;     // TODO: replace the cast with __builtin_bit_cast because it cannot be constexpr if contains a pointer cast
 uint8  DstBytes[sizeof(T)];
 for(uint idx=0;idx < sizeof(T);idx++)DstBytes[idx] = SrcBytes[(sizeof(T)-1)-idx];
 return *(T*)&DstBytes;
}
//------------------------------------------------------------------------------------------------------------
template<typename T> constexpr _finline static int32 AddrToRelAddr(T CmdAddr, unsigned int CmdLen, T TgtAddr){return -((CmdAddr + CmdLen) - TgtAddr);}         // x86 only?
template<typename T> constexpr _finline static T     RelAddrToAddr(T CmdAddr, unsigned int CmdLen, int32 TgtOffset){return ((CmdAddr + CmdLen) + TgtOffset);}  // x86 only?

template <typename T> constexpr _finline static T RotL(T Value, unsigned int Shift){constexpr unsigned int MaxBits = sizeof(T) * 8U; return (Value << Shift) | (Value << ((MaxBits - Shift)&(MaxBits-1)));}
template <typename T> constexpr _finline static T RotR(T Value, unsigned int Shift){constexpr unsigned int MaxBits = sizeof(T) * 8U; return (Value >> Shift) | (Value << ((MaxBits - Shift)&(MaxBits-1)));}

template<typename N, typename M> constexpr _finline static M NumToPerc(N Num, M MaxVal){return (((Num)*100)/(MaxVal));}               // NOTE: Can overflow!
template<typename P, typename M> constexpr _finline static M PercToNum(P Per, M MaxVal){return (((Per)*(MaxVal))/100);}               // NOTE: Can overflow!          

template<class N, class M> constexpr _finline static M AlignFrwd(N Value, M Alignment){return (Value/Alignment)+(bool(Value%Alignment)*Alignment);}    // NOTE: Slow but works with any Alignment value
template<class N, class M> constexpr _finline static M AlignBkwd(N Value, M Alignment){return (Value/Alignment)*Alignment;}                            // NOTE: Slow but works with any Alignment value

// 2,4,8,16,...
template<typename N> constexpr _finline static bool IsPowerOf2(N Value){return Value && !(Value & (Value - 1));}
template<typename N> constexpr _finline static N AlignP2Frwd(N Value, unsigned int Alignment){return (Value+((N)Alignment-1)) & ~((N)Alignment-1);}    // NOTE: Result is incorrect if Alignment is not power of 2
template<typename N> constexpr _finline static N AlignP2Bkwd(N Value, unsigned int Alignment){return Value & ~((N)Alignment-1);}                       // NOTE: Result is incorrect if Alignment is not power of 2
//------------------------------------------------------------------------------------------------------------

// The pointer proxy to use inside of a platform dependant structures (i.e. NTDLL)
template<typename T, typename H=uint> struct alignas(H) SPTR
{
 STASRT(SameTypes<H, uint>::value || SameTypes<H, uint32>::value || SameTypes<H, uint64>::value, "Unsupported pointer type!");   
 H Value;
 _finline void operator= (H val){this->Value = val;} 
 _finline void operator= (T* val){this->Value = (H)val;}    // May truncate or extend the pointer
 _finline void operator= (SPTR<T,H> val){this->Value = val.Value;}
 _finline operator T* (void){return (T*)this->Value;}    // Must be convertible to current pointer type
 _finline operator H (void){return this->Value;}    // Raw value
};
//using SPTRN  = SPTR<uint>;
//using SPTR32 = SPTR<uint32>;
//using SPTR64 = SPTR<uint64>;
//------------------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------------------
