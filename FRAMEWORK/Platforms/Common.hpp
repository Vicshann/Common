
#pragma once

// NOTE: This module does not declare its own name space
// NOTE: The Framework is not compatible with MSVC compiler because it uses some GCC style assebbler

//#pragma warning(disable:4800)   // forcing value to bool 'true' or 'false' (performance warning)

// template<[template-parameter-list]> using [your-alias] = [original-type];

// https://github.com/graphitemaster/incbin/blob/main/incbin.h

// MSVC(_MSC_VER), CLANG(__clang__), GCC(__GNUC__), ICC(__INTEL_COMPILER), ICX(__INTEL_LLVM_COMPILER)

// REF: https://github.com/chakra-core/ChakraCore/blob/master/lib/Common/CommonPal.h

// https://gcc.gnu.org/onlinedocs/gcc/Other-Builtins.html
// __builtin_bit_cast
// __builtin_trap (void)
//------------------------------------------------------------------------------------------------------------

#ifdef __GNUC__              // CLANG defines it too
#define COMP_AGCC __GNUC__   // May be GCC, ICC, or something else
#endif
#ifdef __clang__              // Must be last here!
#define COMP_CLNG __clang__   // May define __GNUC__ or _MSC_VER
#pragma message(">>> Compiler is CLANG")
#undef COMP_AGCC
#endif
#ifdef _MSC_VER
#define COMP_MSVC _MSC_VER
#pragma message(">>> Compiler is MSVC")
#undef COMP_CLNG   // CLANG pretends to be MSVC
#undef COMP_AGCC
#endif


// https://abseil.io/docs/cpp/platforms/macros
// https://stackoverflow.com/questions/152016/detecting-cpu-architecture-compile-time
//#if defined(i386) || defined(__i386__) || defined(__i386) || defined(_M_IX86) || defined(__X86__) || defined(_X86_) || defined(__i486__) || defined(__i586__) || defined(__i686__)    // None of those is recognized by CLANG
#if defined(__arm__) || defined(__aarch64__) || defined(_M_ARM64)
#define CPU_ARM
#pragma message(">>> CPU is ARM")
static constexpr bool IsCpuARM = true;
static constexpr bool IsCpuX86 = false;
#elif defined(__x86_64__) || defined(_M_X64) || defined(__amd64__) || defined(__amd64) || defined(__i386__) || defined(_M_X86)
static constexpr bool IsCpuARM = false;
#define CPU_X86
#pragma message(">>> CPU is X86")
static constexpr bool IsCpuX86 = true;
#else
#pragma message(">>> CPU is UNKNOWN")
static constexpr bool IsCpuARM = false;
static constexpr bool IsCpuX86 = false;
#endif

#if defined(__aarch64__) || defined(_M_ARM64) || defined(__x86_64__) || defined(_M_X64) || defined(__amd64__) || defined(__amd64)
#define ARCH_X64
#pragma message(">>> Architecture is X64")
static constexpr bool IsArchX64 = true;
#else
#define ARCH_X32
#pragma message(">>> Architecture is X32")
static constexpr bool IsArchX64 = false;
#endif

// https://stackoverflow.com/questions/3378560/how-to-disable-gcc-warnings-for-a-few-lines-of-code
// TODO: Implement

//#define CPU_X86
//#define CPU_ARM
//#define ARCH_X32
//#define ARCH_X64

#if defined(ARCH_X32) && !defined(CPU_ARM)
#define _scall  __stdcall
#define _ccall  __cdecl
#define _fcall  __fastcall
#define PXCALL  __cdecl
#else
#define _scall       // Not required
#define _ccall
#define _fcall
#define PXCALL
#endif


// TODO: Check that we have compatible PLT_* definitions
#if defined(_WIN32) || defined(__WIN32__) || defined(_WIN64) || defined(__WINDOWS__) || defined(__NT__)  // Clang, GCC mode at least   // MSVC: _WIN64 - Defined as 1 when the compilation target is 64-bit ARM (Windows only?)or x64.
#pragma message(">>> OS is Windows")
#define SYS_WINDOWS
static constexpr bool IsSysWindows = true;
#else
static constexpr bool IsSysWindows = false;
#endif

#ifdef __ANDROID__        // Implies Linux, so check it first
#pragma message(">>> OS is Android")
#define SYS_ANDROID
static constexpr bool IsSysAndroid = true;
#else
static constexpr bool IsSysAndroid = false;
#endif

#ifdef __linux__    // Clang, GCC mode at least
#pragma message(">>> OS is Linux")
#define SYS_LINUX
static constexpr bool IsSysLinux = true;
#else
static constexpr bool IsSysLinux = false;
#endif

#if defined(unix) || defined(__unix__) || defined(__unix)    // Linux too  // Clang, GCC mode at least
#pragma message(">>> OS is Unix")   // Linux too!
#define SYS_UNIX
#endif

#if defined(__APPLE__) && defined(__MACH__)    // __APPLE__  // Clang, GCC mode at least
#pragma message(">>> OS is MacOS")
#define SYS_MACOS
static constexpr bool IsSysMacOS = true;
#else
static constexpr bool IsSysMacOS = false;
#endif

#ifdef __FreeBSD__        // FreeBSD, NetBSD, OpenBSD, DragonFly BSD
#pragma message(">>> OS is BSD")
#define SYS_BSD
static constexpr bool IsSysBSD = true;
#else
static constexpr bool IsSysBSD = false;
#endif


#if defined(__DEBUG) || defined(DEBUG) && !defined(NDEBUG)
#define DBGBUILD
#pragma message(">>> Building as DEBUG")
static constexpr bool IsDbgBuild = true;
#else
#pragma message(">>> Building as RELEASE")
static constexpr bool IsDbgBuild = false;
#endif

#define IFDBG if constexpr(IsDbgBuild)

#ifdef COMP_MSVC  // MSVC/CLANG_MSVC                            // __has_cpp_attribute
#define _NonVolatile
#pragma intrinsic(_ReturnAddress)
#pragma intrinsic(_AddressOfReturnAddress)  // Instead of #include <intrin.h>

#define GETSTKFRAME() _AddressOfReturnAddress()  // ARM? On ARM RetAddr is in LR register, not on stack // Will not include address of some previous stack frame (from 'push rbp' at proc addr)    // CLANG:MSVC also supports __builtin_frame_address
#define GETRETADDR() _ReturnAddress()
//#define SETRETADDR(addr) (*(void**)_AddressOfReturnAddress() = (void*)(addr))    // Not on ARM? LR is usually pushed on stack

#define  _fcompact __forceinline              // No 'flatten' with MSVC?
#define  _ninline _declspec(noinline)
#define  _finline __forceinline               // At least '/Ob1' is still required     [[msvc::forceinline]] ??
#define  _naked __declspec(naked)
#define  _used
//#pragma code_seg(".xtext")
#pragma section(".xtxt",execute,read)       // 'execute' will be ignored for a data declared with _codesec if section name is '.text', another '.text' section will be created, not executable
#define _codesec _declspec(allocate(".xtxt"))
#define _codesecn(n) _declspec(allocate(".xtxt"))
#pragma comment(linker,"/MERGE:.xtxt=.text")     // Without this SAPI struct won`t go into executable '.text' section
#else   // CLANG/GCC
#define _NonVolatile __restrict
// https://gcc.gnu.org/onlinedocs/gcc/Return-Address.html
// NOTE: __builtin_frame_address does not return the frame address as it was at a function entry
#define GETSTKFRAME() __builtin_frame_address(0)   // TODO: Rework! // On ARM there is no RetAddr on stack  // Should not include address of some previous stack frame (from 'push rbp' at proc addr)
#define GETRETADDR() __builtin_extract_return_addr(__builtin_return_address (0))
//#define SETRETADDR(addr) (*(void**)__builtin_frame_address(0) = __builtin_frob_return_addr((void*)(addr)))  // ARM?

#define  _fcompact __attribute__((flatten))         // Inlines everything inside the marked function
#define  _ninline __attribute__((noinline))
#define  _finline __attribute__((always_inline))
#define  _naked   __attribute__((naked))
#define  _used __attribute__((used))       // Without it Clang will strip every reference to syscall stubs when building for ARM
#ifdef SYS_MACOS
#define _codesec __attribute__ ((section ("__TEXT,__text")))
#define _codesecn(n) __attribute__ ((section ("__TEXT,__text" #n )))
#else
#define _codesec __attribute__ ((section (".text")))     // NOTE: For PE exe sections any static inline member will go into a separate data section named as '.text'
#define _codesecn(n) __attribute__ ((section (".text." #n )))   // ELF section format: 'secname.index' goes to secname
#endif
#endif

// On x86 compiler expects return address on stack and calculates stack alignment based on this
// But system entry point(from kernel)does not stores any return address and keeps the stack aligned to 16
//
#ifdef CPU_X86
#define _SYSENTRY extern "C" __attribute__((force_align_arg_pointer))
#else
#define _SYSENTRY extern "C"
#endif

#define SCVR static constexpr const

#define StkAlloc(x) __builtin_alloca(x)
/*
#if defined(PLT_WIN_USR) || defined(PLT_WIN_KRN)

#ifdef CPU_X86

#ifdef ARCH_X64
#define SYSDESCPTR ((void*)__readgsqword(0x60))   // PEB
#elif defined(ARCH_X32)
#define SYSDESCPTR ((void*)__readfsdword(0x30))   // PEB
#else
#error "SYSDESCPTR Architecture Unsupported"
#endif

#elif defined(CPU_ARM)
#error "SYSDESCPTR Windows ARM Unsupported"
#else
#error "SYSDESCPTR CPU Unsupported"
#endif

#else
#define SYSDESCPTR  &((void**)__builtin_frame_address(0))[1]  // A pointer to 'ELF Auxiliary Vectors'  // First size_t is nullptr for a return addr
#endif
*/
//#define STKFRAMEPTR  &((void**)__builtin_frame_address(0))[1]  // A pointer to 'ELF Auxiliary Vectors'  // First size_t is nullptr for a return addr


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

// Allowed to force inline
#ifdef FWK_INLINE_ALL
#define FWMAYINL _finline
#define FWICALL _finline
#else
#define FWICALL
#define FWMAYINL
#endif

// NOTE: [no_unique_address] implies that a next member will take that space(Previous too, with some compilers). i.e. If four or less [no_unique_address] are followed by 'int' then the struct size will be 4.
//       Add another empty member and another alignment amount will be added to struct size
//       But ICC behaves differently and allocates space if there are more than one empty member in a sequence.
#if defined(_MSC_VER)
#define NO_UNIQUE_ADDR [[msvc::no_unique_address]]
#else
#define NO_UNIQUE_ADDR [[no_unique_address]]
#endif


// This helps to create a function wrapper without copying its declared signature (If signatures is changed frequently during redesign it is a big hassle to keep the interfaces in sync)
// Setting SAPI here as default helps to get rid of lambdas which generate too much assignment code:
//   {return [&]<typename T=SAPI>() _finline {if constexpr(requires { typename T::unlink; })return T::unlink(args...); else return T::unlinkat(PX::AT_FDCWD, args..., 0);}();}
//   if constexpr(requires { Y::rename; })return Y::rename(args...); else return [](achar* oldpath, achar* newpath) _finline {return Y::renameat(PX::AT_FDCWD,oldpath,PX::AT_FDCWD,newpath);}(args...);
//
#define FUNC_WRAPPERFI(Func,Name) template<typename Y=SAPI, typename... Args> static _finline auto Name(Args ... args) -> decltype(((decltype(Func)*)nullptr)(args...))  // Name(Args&& ... args) ? - refs create too much code! // Argument errors is more verbose when using nullptr decltype instead of direct type here        // template<typename Func, typename... Args> auto Name(Args&& ... args) -> decltype(((Func*)nullptr)(args...)) {}
#define FUNC_WRAPPERNI(Func,Name) template<typename Y=SAPI, typename... Args> static _ninline auto Name(Args ... args) -> decltype(((decltype(Func)*)nullptr)(args...))  // Name(Args&& ... args) ? - refs create too much code! // Argument errors is more verbose when using nullptr decltype instead of direct type here        // template<typename Func, typename... Args> auto Name(Args&& ... args) -> decltype(((Func*)nullptr)(args...)) {}

#define CALL_IFEXISTR(fchk,falt,farg,faarg) if constexpr(requires { Y::fchk; })return Y::fchk farg; else return Y::falt faarg;
#define CALL_IFEXISTRN(fchk,falt,anams,farg,faarg) if constexpr(requires { Y::fchk; })return Y::fchk farg; else return anams::falt faarg;
#define CALL_IFEXISTRP(fchk,falt,farg,faarg,apar) if constexpr(requires { Y::fchk; })return Y::fchk farg; else return [] apar _finline {return Y::falt faarg;} farg;
#define CALL_IFEXISTRPC(fchk,falt,cond,farg,faarg,apar) if constexpr((cond))return Y::fchk farg; else return [] apar _finline {return Y::falt faarg;} farg;

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
 using achar  = char;      // Since C++11: u8"Hello" to define a UTF-8 string of chars
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
 using vptr   = void*;

 using size_t    = uint;  // To write a familiar type convs
 using ssize_t   = sint;
 using nullptr_t = decltype(nullptr);

 // Add definitions for floating point types?
};

using PTRTYPE64  = typename NGenericTypes::uint64;
using PTRTYPE32  = typename NGenericTypes::uint32;
using PTRCURRENT = typename NGenericTypes::uint;

using namespace NGenericTypes;   // For 'Framework.hpp'   // You may do the same in your code if you want
//------------------------------------------------------------------------------------------------------------

static constexpr unsigned int StkAlign = Is64BitBuild()?8:4;   // X86,ARM     // ARM is 8, x86 16(32 is better)
static constexpr unsigned int PtrBIdx  = Is64BitBuild()?3:2;

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

//template<typename T, typename U> struct SameTypes {enum { V = false };};   // Cannot be a member of a local class or a function
//template<typename T> struct SameTypes<T, T> {enum { V = true };};

template<typename T, typename U> struct SameTypes {static constexpr bool V = false;};   // Cannot be a member of a local class or a function
template<typename T> struct SameTypes<T, T> {static constexpr bool V = true;};

template<typename A, typename B> constexpr _finline static bool IsSameTypes(A ValA, B ValB)
{
 return SameTypes<A, B>::V;
}

template<typename T> struct IsPositive { static const bool V = (T(-1) >= 0); };     // template<typename T> constexpr _finline static bool IsPositive(void){return (T(-1) >= 0);}    // constinit

//------------------------------------------------------------------------------------------------------------
template<typename Ty> struct RemoveRef { using T = Ty; };
template<typename Ty> struct RemoveRef<Ty&> { using T = Ty; };
template<typename Ty> struct RemoveRef<Ty&&> { using T = Ty; };
//------------------------------------------------------------------------------------------------------------
template <typename Ty> struct TyIdent { using T = Ty; };

// https://stackoverflow.com/questions/17644133/function-that-accepts-both-lvalue-and-rvalue-arguments
// NOTE: Use this only to pass an unused temporaries as unneeded return values of a function
// EXAMPLE: ARef<typename RemoveRef<typename TyIdent<T>::T>::T> res
template <typename Ref> struct ARef       // Universal ref wrapper
{
 Ref &&ref;

 constexpr _finline ARef(Ref&& arg) : ref((typename RemoveRef<Ref>::T&&)arg) { }   // RValue     // Using the class` type Ref leaves us wuthout Universal Reference. But making the constructor template will break type conversion on assignment
 constexpr _finline ARef(Ref& arg) : ref((typename RemoveRef<Ref>::T&&)arg) { }    // LValue
 constexpr _finline ARef(volatile Ref& arg) : ref((typename RemoveRef<Ref>::T&&)arg) { }    // LValue for a volatile storage (Can removing the 'volatile' break some use cases of it bacause of optimization?)  // Can we pass a type to ARef without losing its volatility?

 constexpr _finline Ref& operator=(ARef<Ref> const& v){ref = v; return ref; }
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
// https://stackoverflow.com/questions/46404503/can-i-implement-maxa-maxb-maxc-d-using-fold-expressions

template <unsigned int MinVal=sizeof(void*), typename ... Ts> constexpr _finline size_t SizeOfMaxInPPack(const Ts&... args)
{
 size_t ret { MinVal };
 if constexpr (sizeof...(args)){( (ret = (sizeof(Ts) > ret ? sizeof(Ts) : ret)), ... );}    // Max
 return ret;
}

template <unsigned int MinVal=sizeof(void*), typename ... Ts> constexpr _finline size_t SizeOfMaxInTPack(void)
{
 size_t ret { MinVal };
 return ( (ret = (sizeof(Ts) > ret ? sizeof(Ts) : ret)), ... );
}

template<int at, int idx=0> static constexpr _finline const auto GetParFromPk(const auto first, const auto... rest)    // NOTE: With auto& it generates more complicated code (Even when inlined there are a lot of taking refs into local vars)
{
 if constexpr(idx == at)return first;
  else return GetParFromPk<at,idx+1>(rest...);
}

/*static volatile __m128 me = {7,7,7,7};
static_assert(SizeOfMaxInPPack((int)1, (char)2, (double)3.0, (short)4, me) == 16);
static_assert(SizeOfMaxInPPack((int)1, (char)2, (double)3.0, (short)4) == 8);
static_assert(SizeOfMaxInPPack((char)1, (float)2) == sizeof(float));
static_assert(SizeOfMaxInPPack((int)1, (char)2) == 4); */

//------------------------------------------------------------------------------------------------------------
// https://stackoverflow.com/questions/3177686/how-to-implement-is-pointer

// These are useful with 'auto' arguments
//template <typename T> static consteval bool IsPointer(T const &t) {return false;}
//template <typename T> static consteval bool IsPointer(T *t) {return true;}
//template<typename T> struct IsPtrType { static constinit bool V = false; };
//template<typename T> struct IsPtrType<T*> { static constinit bool V = true; };


template <typename Ty> struct RemoveConst {typedef Ty T;};
template <typename Ty> struct RemoveConst<const Ty> {typedef Ty T;};
template <typename Ty> struct RemoveVolatile {typedef Ty T;};
template <typename Ty> struct RemoveVolatile<volatile Ty> {typedef Ty T;};
template <typename Ty> struct RemoveCV : RemoveConst<typename RemoveVolatile<Ty>::T> {};
template <typename Ty> struct IsUPtrType {enum { V = false };};
template <typename Ty> struct IsUPtrType<Ty*> {enum { V = true };};
template <typename Ty> struct IsPtrType : IsUPtrType<typename RemoveCV<Ty>::T> {};
template <typename Ty> static consteval bool IsPointer(const Ty&){return IsPtrType<Ty>::V;}   // Is this one better?


//------------------------------------------------------------------------------------------------------------
// Returns take address of an ref objects but returns pointers as is
// Needed because conditional expressions do not work with incompatible types which is required in folding: {((IsPtrType<RemoveRef<decltype(args)>::T>::V)?(args):(&args))...};
static constexpr _finline void* GetValAddr(auto&& Value)
{
 using VT = typename RemoveConst<typename RemoveRef<decltype(Value)>::T>::T;  // May be const
 if constexpr (IsPtrType<VT>::V)return (void*)Value;
 return (void*)&Value;
}

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
// TODO: Cast pointer types to size_t
template<typename N> constexpr _finline static N AlignP2Frwd(N Value, unsigned int Alignment){return (Value+((N)Alignment-1)) & ~((N)Alignment-1);}    // NOTE: Result is incorrect if Alignment is not power of 2
template<typename N> constexpr _finline static N AlignP2Bkwd(N Value, unsigned int Alignment){return Value & ~((N)Alignment-1);}                       // NOTE: Result is incorrect if Alignment is not power of 2
//------------------------------------------------------------------------------------------------------------

template<typename T> consteval size_t countof(T& a){return (sizeof(T) / sizeof(*a));}         // Not for array classes or pointers!  // 'a[0]' ?

//------------------------------------------------------------------------------------------------------------
// No way to return an array from a 'consteval' directly?
//
template<typename T, uint N> struct SDHldr
{
 T data[N];

// static_assert(sizeof(SDHldr<T,N>) == (sizeof(T)*N)); // incomplete type // Should work as array

 constexpr _finline operator T* () { return &this->data[0]; }
 constexpr _finline T& operator[] (const uint idx) {return this->data[idx];}              //lvalue
 constexpr _finline const T& operator[] (const uint idx) const {return this->data[idx];}  //rvalue
};
//------------------------------------------------------------------------------------------------------------
struct pchar
{
 union
  {
   achar* av;
   charb* bv;
   const achar* cav;
   const charb* cbv;
  } val;

_finline constexpr pchar(achar* v){this->val.av = v;}
_finline constexpr pchar(charb* v){this->val.bv = v;}
_finline constexpr pchar(const achar* v){this->val.cav = v;}
_finline constexpr pchar(const charb* v){this->val.cbv = v;}

_finline uint8 operator* () const {return *this->val.cav;}    // *Ptr ?

_finline operator achar* (void) const {return this->val.av;}
_finline operator charb* (void) const {return this->val.bv;}
};
//------------------------------------------------------------------------------------------------------------
// The pointer proxy to use inside of a platform dependant structures (i.e. NTDLL)
// Accepts a pointer to type T and stores it as type H
template<typename T, typename H=uint> struct alignas(H) SPTR
{
 using R = typename RemoveConst<typename RemoveRef<T>::T>::T;   // Base type
 STASRT(SameTypes<H, uint>::V || SameTypes<H, uint32>::V || SameTypes<H, uint64>::V, "Unsupported pointer type!");
 H Value;
 _finline constexpr SPTR(void) = default;    //{this->Value = 0;}          // Avoid default constructors in POD (SPTR will replace many members in POD structures)!
 _finline constexpr SPTR(H v){this->Value = v;}
 _finline constexpr SPTR(T* v) requires (!SameTypes<T, const char>::V) {this->Value = (H)v;}
 _finline constexpr SPTR(int v){this->Value = (H)v;}                   // For '0' values
 _finline constexpr SPTR(pchar v){this->Value = (H)v.val.av;}
 //_finline SPTR(unsigned int v){this->Value = (H)v;}          // For '0' values
 _finline constexpr SPTR(long long v){this->Value = (H)v;}             // For '0' values
 //_finline SPTR(unsigned long long v){this->Value = (H)v;}    // For '0' values
// template<typename X, int N> _finline  SPTR(const X(&v)[N]){this->Value = (H)v;}   // for arrays
 _finline constexpr SPTR(const char* v){this->Value = (H)v;}           // For string pointers
 _finline constexpr SPTR(nullptr_t v){this->Value = (H)v;}             // For 'nullptr' values
 template<typename X> _finline constexpr SPTR(X&& v) requires (!SameTypes<R, pchar>::V) {this->Value = (H)((T*)v);}   // For some classes that cannon convert themselves implicitly
 template<typename X> _finline constexpr SPTR(X&& v) requires (SameTypes<R, pchar>::V) {this->Value = (H)((const achar*)v);}

 _finline void operator= (H val){this->Value = val;}
 _finline void operator= (T* val){this->Value = (H)val;}     // May truncate or extend the pointer
// _finline void operator= (SPTR<T,H> val){this->Value = val.Value;}   // -Wdeprecated-copy-with-user-provided-copy
 template<typename X> _finline operator X* (void) const {return (X*)this->Value;}
// _finline operator auto* (void) const requires (!SameTypes<T, void>::V) {return (T*)this->Value;}
// _finline operator T* (void) const {return (T*)this->Value;}        // Must be convertible to current pointer type
 _finline operator H (void) const {return this->Value;}             // Raw value

 _finline T* operator* () const {return (T*)this->Value;}   // Should be T&
 _finline T* operator-> () const {return (T*)this->Value;}
};
//using SPTRN  = SPTR<uint>;
//using SPTR32 = SPTR<uint32>;
//using SPTR64 = SPTR<uint64>;
//------------------------------------------------------------------------------------------------------------
template <class T> T RevBits(T n)
{
 short bits = sizeof(n) * 8;
 T mask = ~T(0); // equivalent to uint32_t mask = 0b11111111111111111111111111111111;
 while (bits >>= 1)
  {
   mask ^= mask << (bits); // will convert mask to 0b00000000000000001111111111111111;
   n = (n & ~mask) >> bits | (n & mask) << bits; // divide and conquer
  }
 return n;
}

/*
template<typename T> T RevBits( T n )
{
    // we force the passed-in type to its unsigned equivalent, because C++ may
    // perform arithmetic right shift instead of logical right shift, depending
    // on the compiler implementation.
    typedef typename std::make_unsigned<T>::type unsigned_T;
    unsigned_T v = (unsigned_T)n;

    // swap every bit with its neighbor
    v = ((v & 0xAAAAAAAAAAAAAAAA) >> 1)  | ((v & 0x5555555555555555) << 1);

    // swap every pair of bits
    v = ((v & 0xCCCCCCCCCCCCCCCC) >> 2)  | ((v & 0x3333333333333333) << 2);

    // swap every nybble
    v = ((v & 0xF0F0F0F0F0F0F0F0) >> 4)  | ((v & 0x0F0F0F0F0F0F0F0F) << 4);
    // bail out if we've covered the word size already
    if( sizeof(T) == 1 ) return v;

    // swap every byte
    v = ((v & 0xFF00FF00FF00FF00) >> 8)  | ((v & 0x00FF00FF00FF00FF) << 8);
    if( sizeof(T) == 2 ) return v;

    // etc...
    v = ((v & 0xFFFF0000FFFF0000) >> 16) | ((v & 0x0000FFFF0000FFFF) << 16);
    if( sizeof(T) <= 4 ) return v;

    v = ((v & 0xFFFFFFFF00000000) >> 32) | ((v & 0x00000000FFFFFFFF) << 32);

    // explictly cast back to the original type just to be pedantic
    return (T)v;
}
*/

//---------------------------------------------------------------------------
// Makes the pointer 'arbitrary', like it came from some malloc
// Helps the optimizer 'forget' about where this pointer came from
auto _finline UnbindPtr(auto* ptr)
{
 volatile size_t tmp = (size_t)ptr;
 return (decltype(ptr))tmp;
}
//---------------------------------------------------------------------------
// NOTE: Will suppress force_inline
extern "C"
{
// TODO: Get rid of div and mult
// TODO: Align the addresses and use xmm for copy operations
void* memcpy(void* Dst, const void* Src, size_t Size)   // static
{
 size_t ALen = Size >> PtrBIdx;
// size_t BLen = _Size%sizeof(size_t);
 for(size_t ctr=0;ctr < ALen;ctr++)((size_t*)Dst)[ctr] = ((const size_t*)Src)[ctr];
 for(size_t ctr=(ALen << PtrBIdx);ctr < Size;ctr++)((char*)Dst)[ctr] = ((const char*)Src)[ctr];
 return Dst;
}

void* CopyMem(void* Dst, const void* Src, size_t Size) //__attribute__ ((optnone))  // static
{
 size_t ALen = Size >> PtrBIdx;
// size_t BLen = _Size%sizeof(size_t);
 for(size_t ctr=0;ctr < ALen;ctr++)((size_t*)Dst)[ctr] = ((const size_t*)Src)[ctr];
 for(size_t ctr=(ALen << PtrBIdx);ctr < Size;ctr++)((char*)Dst)[ctr] = ((const char*)Src)[ctr];
 return Dst;
}
//------------------------------------------------------------------------------------------------------------
// NOTE: There is a BUG in memmove part (loses last size_t at ALen-1)!!!
/*static void* memmove(void* Dst, const void* Src, size_t Size)     // Have a BUG
{
 if((char*)Dst <= (char*)Src)return memcpy(Dst,Src,Size);
 size_t ALen = Size >> PtrBIdx;
 size_t BLen = Size & (PtrBIdx-1);
 for(size_t ctr=Size-1;BLen > 0;ctr--,BLen--)((char*)Dst)[ctr] = ((const char*)Src)[ctr];
 for(size_t ctr=ALen-1;ALen > 0;ctr--,ALen--)((size_t*)Dst)[ctr] = ((const size_t*)Src)[ctr];
 return Dst;
} */
//---------------------------------------------------------------------------
void* memset(void* Dst, unsigned int Val, size_t Size)      // TODO: Aligned, SSE by MACRO
{
 size_t ALen = Size >> PtrBIdx;
// size_t BLen = _Size%sizeof(size_t);
 size_t DVal = Val & 0xFF;               // Bad and incorrect For x32: '(size_t)_Val * 0x0101010101010101;'         // Multiply by 0x0101010101010101 to copy the lowest byte into all other bytes
 if(DVal)
  {
   DVal = Val | (Val << 8) | (Val << 16) | (Val << 24);
#ifdef _AMD64_
   DVal |= DVal << 32;
#endif
  }
 for(size_t ctr=0;ctr < ALen;ctr++)((size_t*)Dst)[ctr] = DVal;
 for(size_t ctr=(ALen << PtrBIdx);ctr < Size;ctr++)((char*)Dst)[ctr] = (char)DVal;
 return Dst;
}
//---------------------------------------------------------------------------
// TODO: Need a function which returns number of matched bytes, not just diff of a last unmatched byte
int memcmp(const void* Buf1, const void* Buf2, size_t Size)   // '(*((ULONG**)&_Buf1))++;'
{
 const unsigned char* BufA = (const unsigned char*)Buf1;
 const unsigned char* BufB = (const unsigned char*)Buf2;
 for(;Size >= sizeof(size_t); Size-=sizeof(size_t), BufA+=sizeof(size_t), BufB+=sizeof(size_t))  // Enters here only if Size >= sizeof(ULONG)
  {
   if(*((const size_t*)BufA) != *((const size_t*)BufB))break;  // Have to break and continue as bytes because return value must be INT  // return (*((intptr_t*)BufA) - *((intptr_t*)BufB));  //  // TODO: Move everything to multiplatform FRAMEWORK
  }
 for(;Size > 0; Size--, BufA++, BufB++)  // Enters here only if Size > 0
  {
   if(*((const unsigned char*)BufA) != *((const unsigned char*)BufB)){return ((int)*BufA - (int)*BufB);}
  }
 return 0;
}
//---
};
//---------------------------------------------------------------------------
