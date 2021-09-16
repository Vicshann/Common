
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
#else
#define  _finline __attribute__((always_inline))    // __attribute__((flatten)) is also useful
#define  _naked   __attribute__((naked))
#endif

//#ifdef FWK_DEBUG
#define STASRT(...) static_assert(__VA_ARGS__)
//#else
//#define STASRT(cond,txt)
//#endif

// These three work at a call site 
#define SRC_LINE __builtin_LINE()
#define SRC_FILE __builtin_FILE()        // Full path included
#define SRC_FUNC __builtin_FUNCTION()    // Only the name itself, no arguments or a return type

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
namespace NGenericTypes   // You should do 'using' for it yourselves if you want to bring these types to global name space
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

 // Add definitions for floating point types?
};

using namespace NGenericTypes;   // For 'Framework.hpp'   // You may do the same in your code if you want
//------------------------------------------------------------------------------------------------------------


//------------------------------------------------------------------------------------------------------------
