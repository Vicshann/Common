
#pragma once

#ifndef _FRAMEWORK_    // These macro are used to check which header file included
#define _FRAMEWORK_    // Now we just use __has_include 	- a preprocessor operator to check whether an inclusion is possible. // C++17  // __has_include requires include search directories to be passed to the compiler

// Must be outside any namespace. This is the only external dependancy. It is header-only and very compiler specific
//#include <intrin.h>         // Only for windows?  // Unfortunately <intrin.h> is a mess
//#include <stdarg.h>
//#include <emmintrin.h>        // for _mm_storeu_si128
//#include <immintrin.h>

/*
 Some useful links:
   https://graphics.stanford.edu/~seander/bithacks.html

*/

//#include <intrin.h>
//#include <xmmintrin.h>
//------------------------------------------------------------------------------------------------------------
// Some STD stuff
#if !__has_include (<initializer_list>)
#include "Platforms/InitList.hpp"
#endif

//extern "C" void*  __cdecl memmove(void* _Dst, const void* _Src, size_t _Size);
//extern "C" void*  __cdecl memset(void* _Dst, int _Val, size_t _Size)

// Format: 'N'+'Four letters of namespace'
// NOTE: It is impossible to put a 'namespace' inside of a 'class'
namespace NFWK      // Must be a namespace because we are adding some namespaces and configs in it
{
// NOTE: None of these files should include anything ( Unless it is a collection? )
// NOTE: Moving out of namespace should be done in the same file if required
#if __has_include ("AppCfg.hpp")
#include "AppCfg.hpp"
#elif __has_include ("../AppCfg.hpp")
#include "../AppCfg.hpp"
#elif __has_include ("../../AppCfg.hpp")
#include "../../AppCfg.hpp"
#else
#include "Platforms/DefaultCfg.hpp"
#pragma message(">>> Using default config")
#endif

#include "Platforms/Common.hpp"       // Contains type definitions, must be in namespace to allow their inclusion with 'using namespace'
#include "Platforms/BitOps.hpp"
#include "Platforms/MemOps.hpp"
namespace NCRYPT
{
#include "Crypto/Crc32.hpp"
}
#include "Platforms/CompileTime.hpp"  // Have access only to Common.hpp, everything else is 'invisible'
#include "Platforms/ArbiNum.hpp"

#include "Math.hpp"
#include "UTF.hpp"
#include "NumCnv.hpp"
#include "DateTime.hpp"
#include "StrUtils.hpp"
#include "StrFmt.hpp"
#include "Platforms/Platform.hpp"     // All API is defined here and inaccessible before!

using PX   = NPTM::PX;
using PX64 = NPTM::PX64;

// Now we have access to SAPI+NAPI




//#include "MemUtils.hpp"
//#include "MemStorage.hpp"
//#include "StrStorage.hpp"
//#include "StrPool.hpp"
#include "Arrays.hpp"

namespace NCRYPT     // https://github.com/abbbaf/Compile-time-hash-functions
{
#include "Crypto/TEA.hpp"
#include "Crypto/RC4.hpp"
#include "Crypto/SHA1.hpp"
#include "Crypto/Rijndael.hpp"
};

#include "AppDef.hpp"

};
//------------------------------------------------------------------------------------------------------------
using NFWK::NCTM::operator""_ps;
using NFWK::NCTM::operator""_es;

#include "Platforms/RTL.hpp"    // Must be in global namespace for the compiler to find it // Comment this out if you linking with a default Run Time Library

#endif

