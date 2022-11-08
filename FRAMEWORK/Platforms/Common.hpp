
#pragma once

// NOTE: This module does not declare its own name space
// NOTE: The Framework is not compatible with MSVC compiler because it uses some GCC style assebbler

//#pragma warning(disable:4800)   // forcing value to bool 'true' or 'false' (performance warning)

// template<[template-parameter-list]> using [your-alias] = [original-type];


// MSVC(_MSC_VER), CLANG(__clang__), GCC(__GNUC__), ICC(__INTEL_COMPILER), ICX(__INTEL_LLVM_COMPILER)
//------------------------------------------------------------------------------------------------------------

#ifdef __GNUC__              // CLANG defines it too
#define COMP_AGCC __GNUC__   // May be GCC, ICC, or something else
#endif
#ifdef _MSC_VER
#define COMP_MSVC _MSC_VER
#undef COMP_AGCC
#endif
#ifdef __clang__              // Must be last here!
#define COMP_CLNG __clang__   // May define __GNUC__ or _MSC_VER
#undef COMP_AGCC
#endif


#ifdef COMP_MSVC                              // __has_cpp_attribute
#define  _finline __forceinline               // At least '/Ob1' is still required
#define  _naked __declspec(naked)
#pragma code_seg(".text")
#define _codesec _declspec(allocate(".text"))
#define _codesecn(n) _declspec(allocate(".text"))
#else
#define  _finline __attribute__((always_inline))    // __attribute__((flatten)) is also useful
#define  _naked   __attribute__((naked))
#define _codesec __attribute__ ((section (".text")))     // NOTE: For PE exe sections any static inline member will go into a separate data section named as '.text'
#define _codesecn(n) __attribute__ ((section (".text." #n )))   // ELF section format: 'secname.index' goes to secname
#endif


#if defined(__DEBUG)
#define _DBGBUILD
#endif

// https://abseil.io/docs/cpp/platforms/macros
// https://stackoverflow.com/questions/152016/detecting-cpu-architecture-compile-time
//#if defined(i386) || defined(__i386__) || defined(__i386) || defined(_M_IX86) || defined(__X86__) || defined(_X86_) || defined(__i486__) || defined(__i586__) || defined(__i686__)    // None of those is recognized by CLANG
#if defined(__arm__) || defined(__aarch64__) || defined(_M_ARM64)
#define _CPU_ARM
#pragma message("CPU is ARM")
#elif defined(__x86_64__) || defined(_M_X64) || defined(__amd64__) || defined(__amd64) || defined(__i386__) || defined(_M_X86)
#define _CPU_X86
#pragma message("CPU is X86")
#else
#pragma message("CPU is UNKNOWN")
#endif

#if defined(__aarch64__) || defined(_M_ARM64) || defined(__x86_64__) || defined(_M_X64) || defined(__amd64__) || defined(__amd64)
#define _ARCH_X64
#pragma message("Platform is X64")
#else
#define _ARCH_X32
#pragma message("Platform is X32")
#endif

//#define _CPU_X86
//#define _CPU_ARM
//#define _ARCH_X32
//#define _ARCH_X64

#if defined(_CPU_X86) && defined(_ARCH_X32)
#define _STDC  __stdcall
#else
#define _STDC       // Not required
#endif

#if defined(_WIN32) || defined(__WIN32__) || defined(_WIN64) || defined(__WINDOWS__) || defined(__NT__)  // Clang, GCC mode at least   // MSVC: _WIN64 - Defined as 1 when the compilation target is 64-bit ARM (Windows only?)or x64.
#pragma message("OS is Windows")
#endif

#ifdef __ANDROID__        // Implies Linux, so check it first
#pragma message("OS is  Android")
#endif

#ifdef __linux__    // Clang, GCC mode at least
#pragma message("OS is Linux")
#endif

#if defined(unix) || defined(__unix__) || defined(__unix)    // Linux too  // Clang, GCC mode at least
//#error "We have Unix!"   // Linux too!
#endif

#if defined(__APPLE__) && defined(__MACH__)    // __APPLE__  // Clang, GCC mode at least
#error "We have MacOS X!"
#endif

#ifdef __FreeBSD__        // FreeBSD, NetBSD, OpenBSD, DragonFly BSD
#error "We have FreeBSD!"
#endif



#if defined(PLT_WIN_USR) || defined(PLT_WIN_KRN)

#ifdef _CPU_X86

#ifdef _ARCH_X64
#define SYSDESCPTR ((void*)__readgsqword(0x60))
#elif defined(_ARCH_X32)
#define SYSDESCPTR ((void*)__readfsdword(0x30))
#else
#error "SYSDESCPTR Architecture Unsupported"
#endif

#elif defined(_CPU_ARM)
#error "SYSDESCPTR Windows ARM Unsupported"
#else
#error "SYSDESCPTR CPU Unsupported"
#endif

#else
#define SYSDESCPTR  &((void**)__builtin_frame_address(0))[1]  // A pointer to 'ELF Auxiliary Vectors'  // First size_t is nullptr for a return addr
#endif

//#ifdef FWK_DEBUG
#define STASRT(...) static_assert(__VA_ARGS__)
//#else
//#define STASRT(cond,txt)
//#endif

// These three work at a call site
#define SRC_LINE __builtin_LINE()
#define SRC_FILE __builtin_FILE()        // Full path included
#define SRC_FUNC __builtin_FUNCTION()    // Only the name itself, no arguments or a return type (like __func__)

// To retrieve a function signature including all argument types
#if defined(__GNUC__) || (defined(__MWERKS__) && (__MWERKS__ >= 0x3000)) || (defined(__ICC) && (__ICC >= 600)) || defined(__ghs__)
#define FUNC_SIG __PRETTY_FUNCTION__
#elif defined(__DMC__) && (__DMC__ >= 0x810)
#define FUNC_SIG __PRETTY_FUNCTION__
#elif defined(__FUNCSIG__)
#define FUNC_SIG __FUNCSIG__
#elif (defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 600)) || (defined(__IBMCPP__) && (__IBMCPP__ >= 500))
#define FUNC_SIG __FUNCTION__
#elif defined(__BORLANDC__) && (__BORLANDC__ >= 0x550)
#define FUNC_SIG __FUNC__
#else
#define FUNC_SIG __func__   // Useless, because no types are provided  (Part of C++ 11 standart)
#endif


// NOTE: [no_unique_address] implies that a next member will take that space(Previous too, with some compilers). i.e. If four or less [no_unique_address] are followed by 'int' then the struct size will be 4.
//       Add another empty member and another alignment amount will be added to struct size
//       But ICC behaves differently and allocates space if there are more than one empty member in a sequence.
#if defined(_MSC_VER)
#define _NO_UNIQUE_ADDR [[msvc::no_unique_address]]
#else
#define _NO_UNIQUE_ADDR [[no_unique_address]]
#endif

class CEmptyType {};
using ETYPE = CEmptyType[0];    // This works plain and simple with ICC, GCC, CLANG (You can get a completely empty struct with such members and no annoying padding rules) but MSVC reports an error 'error C2229: struct has an illegal zero-sized array'
// MSVC: sizeof(ETYPE) : error C2070: 'ETYPE': illegal sizeof operand
// CLANG: With '--target=x86_64-windows-msvc' you cannot get an empty struct with such members, its size will be at least 1 byte

// Conclusion: [no_unique_address] feature is a mess :(  Forget about it and use ETYPE instead. At least it works somehow and it is predictable.
//------------------------------------------------------------------------------------------------------------
template<bool UseTypeA, typename A, typename B> struct TSW;                   // Usage: TSW<MyCompileTimeCondition, MyStructA, MyStructB>::T Val;   // Same as std::conditional
template<typename A, typename B> struct TSW<true, A, B> {using T = A;};
template<typename A, typename B> struct TSW<false, A, B> {using T = B;};

template<bool UseTypeA, typename A, typename B> constexpr static auto TypeSwitch(void)       // Usage: struct SMyStruct { decltype(TypeSwitch<MyCompileTimeCondition, MyStructA, MyStructB>()) Val; }
{
 if constexpr (UseTypeA) {A val{0}; return val;}
   else {B val{0}; return val;}
}
//------------------------------------------------------------------------------------------------------------
template<typename T> consteval static auto ChangeTypeSign(T Val=0)  // Should be compiled faster than a template specialization?
{
 if constexpr (T(-1) < T(0))   // IsSigned
  {
   if constexpr (1 == sizeof(T))return (unsigned char)Val;
   else if constexpr (2 == sizeof(T))return (unsigned short)Val;
   else if constexpr (4 == sizeof(T))return (unsigned int)Val;
   else if constexpr (8 == sizeof(T))return (unsigned long long)Val;
  }
   else
    {
     if constexpr (1 == sizeof(T))return (signed char)Val;
     else if constexpr (2 == sizeof(T))return (signed short)Val;
     else if constexpr (4 == sizeof(T))return (signed int)Val;
     else if constexpr (8 == sizeof(T))return (signed long long)Val;
    }
}

constexpr static bool Is64BitBuild(void){return sizeof(void*) == 8;}   // To be used in constexpr expressions instead of __amd64__ macro
//------------------------------------------------------------------------------------------------------------
// NOTE: You must use these types if you want code randomization to be applied
namespace NGenericTypes   // You should do 'using' for it yourselves if you want to bring these types to global name space   // If this is not Namespace than this would not be possible: 'using namespace NFWK::NGenericTypes;'
{
#ifdef FWK_DEBUG
 static_assert(1 == sizeof(unsigned char), "Unsupported size of char!");
 static_assert(2 == sizeof(unsigned short), "Unsupported size of short!");
 static_assert(4 == sizeof(unsigned int), "Unsupported size of int!");
 static_assert(8 == sizeof(unsigned long long), "Unsupported size of int64!");
 static_assert(4 == sizeof(float), "Unsupported size of float!");
 static_assert(8 == sizeof(double), "Unsupported size of double!");
 static_assert(sizeof(void*) == sizeof(decltype(sizeof(void*))), "Unsupported size of size_t!");
#endif

/* https://unix.org/version2/whatsnew/lp64_wp.html
Datatype	LP64	ILP64	LLP64	ILP32	LP32
char	    8	    8	    8	    8	    8
short	    16	    16	    16	    16	    16
_int32		32
int	        32	    64	    32	    32	    16
long	    64	    64	    32	    32	    32
long long			64
pointer	    64	    64	    64	    32	    32
*/

// Trying to sort out the whole history of type mess (If some platform does not support any of these, its compiler should implement missing operations)
 using achar  = char;      // Since C++11: u8"Helloto define a UTF-8 string of chars
 using wchar  = wchar_t;   // Different platforms may use different sizes for it
 using charb  = char8_t;   // u8"" // cannot be signed or unsigned
 using charw  = char16_t;  // u""  // cannot be signed or unsigned
 using chard  = char32_t;  // U""  // cannot be signed or unsigned

 using uint8  = unsigned char;      // 'char' can be signed or unsigned by default
 using uint16 = unsigned short int;
 using uint32 = unsigned int;       // Expected to be 32bit on all supported platforms  // NOTE: int is 32bit even on x64 platforms, meaning that using 'int' everywhere is not architecture friendly
 using uint64 = unsigned long long; // 'long long unsigned int' or 'long unsigned int' ???  // See LP64, ILP64, ILP32 data models on different architectures

 using int8   = signed char;
 using int16  = signed short int;
 using int32  = signed int;
 using int64  = signed long long;   // __int64_t

 using uint   = decltype(sizeof(void*));   // These 'int' are always platform-friendly (same size as pointer type, replace size_t) // "The result of sizeof and sizeof... is a constant of type std::size_t"
 using sint   = decltype(ChangeTypeSign(sizeof(void*)));

 using size_t    = uint;  // To write a familiar type convs
 using ssize_t   = sint;
 using nullptr_t = decltype(nullptr);


 // Add definitions for floating point types?
};

using namespace NGenericTypes;   // For 'Framework.hpp'   // You may do the same in your code if you want
//------------------------------------------------------------------------------------------------------------

static constexpr unsigned int StkAlign = Is64BitBuild()?8:4;   // X86,ARM

#ifndef va_start
typedef __builtin_va_list va_list;        // Requires stack alignment by 16
#if defined __has_builtin    // GCC/CLANG
#  if __has_builtin (__builtin_va_start)
#    define va_start(ap, param) __builtin_va_start(ap, param)
#    define va_end(ap)          __builtin_va_end(ap)
#    define va_arg(ap, type)    __builtin_va_arg(ap, type)
#  endif
#endif

#endif
//------------------------------------------------------------------------------------------------------------

// NOTE:  Dump here any implementations that should be accessible early and have no personal HPP yet >>>

//------------------------------------------------------------------------------------------------------------
// Some type traits

template<typename T, typename U> struct SameTypes {enum { value = 0 };};   // Cannot be a member of a local class or a function
template<typename T> struct SameTypes<T, T> {enum { value = 1 };};
template<typename A, typename B> constexpr _finline static bool IsSameTypes(A ValA, B ValB)
{
 return SameTypes<A, B>::value;
}

template<typename T> struct IsPointer { static const bool value = false; };
template<typename T> struct IsPointer<T*> { static const bool value = true; };

template<typename T> struct IsPositive { static const bool V = (T(-1) >= 0); };     // template<typename T> constexpr _finline static bool IsPositive(void){return (T(-1) >= 0);}

//------------------------------------------------------------------------------------------------------------
template<typename Ty> struct RemoveRef { using T = Ty; };
template<typename Ty> struct RemoveRef<Ty&> { using T = Ty; };
template<typename Ty> struct RemoveRef<Ty&&> { using T = Ty; };
//------------------------------------------------------------------------------------------------------------
template <typename Ty> struct TyIdent { using T = Ty; };

// NOTE: Use this only to pass an unused temporaries as unneeded return values of a function
// EXAMPLE: ARef<typename RemoveRef<typename TyIdent<T>::T>::T> res
template <typename Ref> struct ARef
{
 Ref &&ref;

 constexpr _finline ARef(Ref&& arg) : ref((typename RemoveRef<Ref>::T&&)arg) { }   // RValue     // Using the class` type Ref leaves us wuthout Universal Reference. But making the constructor template will break type conversion on assignment
 constexpr _finline ARef(Ref& arg) : ref((typename RemoveRef<Ref>::T&&)arg) { }    // LValue
 constexpr _finline ARef(volatile Ref& arg) : ref((typename RemoveRef<Ref>::T&&)arg) { }    // LValue for a volatile storage (Can removing the 'volatile' break some use cases of it bacause of optimization?)  // Can we pass a type to ARef without losing its volatility?

 constexpr _finline Ref& operator=(ARef<Ref> const& v){ref = v; return ref; };
 constexpr _finline operator Ref& () const & { return ref; }
 constexpr _finline operator Ref&& () const && { return (typename RemoveRef<Ref>::T&&)ref; }
 constexpr _finline Ref& operator*() const { return ref; }
 constexpr _finline Ref* operator->() const { return &ref; }
};

// EXAMPLE: XRef<T> res
template<typename T> using XRef = ARef<typename RemoveRef<typename TyIdent<T>::T>::T>;    // Template Alias

template <int Size> struct TypeForSizeU
{
 using T = typename TSW<(Size < 8), typename TSW<Size < 4, typename TSW<Size < 2, uint8, uint16>::T, uint32>::T, uint64>::T;    // without 'typename' the compiler refuses to look for TSW type 
};
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

template <typename T> constexpr _finline static T RotL(T Value, unsigned int Shift){constexpr unsigned int MaxBits = sizeof(T) * 8U; return (Value << Shift) | (Value >> ((MaxBits - Shift)&(MaxBits-1)));}
template <typename T> constexpr _finline static T RotR(T Value, unsigned int Shift){constexpr unsigned int MaxBits = sizeof(T) * 8U; return (Value >> Shift) | (Value << ((MaxBits - Shift)&(MaxBits-1)));}

template<typename N, typename M> constexpr _finline static M NumToPerc(N Num, M MaxVal){return (((Num)*100)/(MaxVal));}               // NOTE: Can overflow!
template<typename P, typename M> constexpr _finline static M PercToNum(P Per, M MaxVal){return (((Per)*(MaxVal))/100);}               // NOTE: Can overflow!

template<typename N> constexpr _finline static N AlignFrwd(N Value, N Alignment){return ((Value/Alignment)+(bool(Value%Alignment)))*Alignment;}    // NOTE: Slow but works with any Alignment value
template<typename N> constexpr _finline static N AlignBkwd(N Value, N Alignment){return (Value/Alignment)*Alignment;}                              // NOTE: Slow but works with any Alignment value

// 2,4,8,16,...
template<typename N> constexpr _finline static bool IsPowerOf2(N Value){return Value && !(Value & (Value - 1));}
template<typename N> constexpr _finline static N AlignP2Frwd(N Value, unsigned int Alignment){return (Value+((N)Alignment-1)) & ~((N)Alignment-1);}    // NOTE: Result is incorrect if Alignment is not power of 2
template<typename N> constexpr _finline static N AlignP2Bkwd(N Value, unsigned int Alignment){return Value & ~((N)Alignment-1);}                       // NOTE: Result is incorrect if Alignment is not power of 2
//------------------------------------------------------------------------------------------------------------

template<typename T> consteval size_t countof(T& a){return (sizeof(T) / sizeof(*a));}         // Not for array classes or pointers!  // 'a[0]' ?


//------------------------------------------------------------------------------------------------------------
// The pointer proxy to use inside of a platform dependant structures (i.e. NTDLL)
// Accepts a pointer to type T and stores it as type H
template<typename T, typename H=uint> struct alignas(H) SPTR
{
 STASRT(SameTypes<H, uint>::value || SameTypes<H, uint32>::value || SameTypes<H, uint64>::value, "Unsupported pointer type!");
 H Value;
 _finline SPTR(void) = default;    //{this->Value = 0;}          // Avoid default constructors in POD (SPTR will replace many members in POD structures)!
 _finline SPTR(H v){this->Value = v;}
 _finline SPTR(T* v){this->Value = (H)v;}
 _finline SPTR(int v){this->Value = (H)v;}                   // For '0' values
 //_finline SPTR(unsigned int v){this->Value = (H)v;}          // For '0' values
 _finline SPTR(long long v){this->Value = (H)v;}             // For '0' values
 //_finline SPTR(unsigned long long v){this->Value = (H)v;}    // For '0' values
 _finline SPTR(const char* v){this->Value = (H)v;}           // For string pointers
 _finline SPTR(nullptr_t v){this->Value = (H)v;}             // For 'nullptr' values

 _finline void operator= (H val){this->Value = val;}
 _finline void operator= (T* val){this->Value = (H)val;}     // May truncate or extend the pointer
 _finline void operator= (SPTR<T,H> val){this->Value = val.Value;}
 _finline operator T* (void){return (T*)this->Value;}        // Must be convertible to current pointer type
 _finline operator H (void){return this->Value;}             // Raw value
};
//using SPTRN  = SPTR<uint>;
//using SPTR32 = SPTR<uint32>;
//using SPTR64 = SPTR<uint64>;
//------------------------------------------------------------------------------------------------------------




//---------------------------------------------------------------------------
// TODO: Get rid of div and mult
// TODO: Align the addresses and use xmm for copy operations
static void*  __cdecl memcpy(void* _Dst, const void* _Src, size_t _Size)
{
 size_t ALen = _Size/sizeof(size_t);
// size_t BLen = _Size%sizeof(size_t);
 for(size_t ctr=0;ctr < ALen;ctr++)((size_t*)_Dst)[ctr] = ((size_t*)_Src)[ctr];
 for(size_t ctr=(ALen*sizeof(size_t));ctr < _Size;ctr++)((char*)_Dst)[ctr] = ((char*)_Src)[ctr];
 return _Dst;
}
//------------------------------------------------------------------------------------------------------------
static void*  __cdecl memmove(void* _Dst, const void* _Src, size_t _Size)
{
 if((char*)_Dst <= (char*)_Src)return memcpy(_Dst,_Src,_Size);
 size_t ALen = _Size/sizeof(size_t);
 size_t BLen = _Size%sizeof(size_t);
 for(size_t ctr=_Size-1;BLen > 0;ctr--,BLen--)((char*)_Dst)[ctr] = ((char*)_Src)[ctr];
 for(size_t ctr=ALen-1;ALen > 0;ctr--,ALen--) ((size_t*)_Dst)[ctr] = ((size_t*)_Src)[ctr];
 return _Dst;
}
//---------------------------------------------------------------------------
static void*  __cdecl memset(void* _Dst, int _Val, size_t _Size)      // TODO: Aligned, SSE by MACRO
{
 size_t ALen = _Size/sizeof(size_t);
// size_t BLen = _Size%sizeof(size_t);
 size_t DVal =_Val & 0xFF;               // Bad and incorrect For x32: '(size_t)_Val * 0x0101010101010101;'         // Multiply by 0x0101010101010101 to copy the lowest byte into all other bytes
 if(DVal)
  {
   DVal = _Val | (_Val << 8) | (_Val << 16) | (_Val << 24);
#ifdef _AMD64_
   DVal |= DVal << 32;
#endif
  }
 for(size_t ctr=0;ctr < ALen;ctr++)((size_t*)_Dst)[ctr] = DVal;
 for(size_t ctr=(ALen*sizeof(size_t));ctr < _Size;ctr++)((char*)_Dst)[ctr] = DVal;
 return _Dst;
}
//---------------------------------------------------------------------------
static int __cdecl memcmp(const void* _Buf1, const void* _Buf2, size_t _Size) // '(*((ULONG**)&_Buf1))++;'
{
 unsigned char* BufA = (unsigned char*)_Buf1;
 unsigned char* BufB = (unsigned char*)_Buf2;
 for(;_Size >= sizeof(size_t); _Size-=sizeof(size_t), BufA+=sizeof(size_t), BufB+=sizeof(size_t))  // Enters here only if Size >= sizeof(ULONG)
  {
   if(*((size_t*)BufA) != *((size_t*)BufB))break;  // Have to break and continue as bytes because return value must be INT  // return (*((intptr_t*)BufA) - *((intptr_t*)BufB));  //  // TODO: Move everything to multiplatform FRAMEWORK
  }
 for(;_Size > 0; _Size--, BufA++, BufB++)  // Enters here only if Size > 0
  {
   if(*((unsigned char*)BufA) != *((unsigned char*)BufB)){return ((int)*BufA - (int)*BufB);}
  }
 return 0;
}
//---------------------------------------------------------------------------
