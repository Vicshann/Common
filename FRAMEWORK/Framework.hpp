
#pragma once

//#ifndef _FRAMEWORK_    // These macro are used to check which header file included
//#define _FRAMEWORK_    // Now we just use __has_include 	- a preprocessor operator to check whether an inclusion is possible. // C++17

// Must be outside any namespace. This is the only external dependancy. It is header-only and very compiler specific
//#include <intrin.h>         // Only for windows?  // Unfortunately <intrin.h> is a mess
//#include <stdarg.h>
//#include <emmintrin.h>        // for _mm_storeu_si128
//#include <immintrin.h>

/*
 Some useful links:
   https://graphics.stanford.edu/~seander/bithacks.html

*/
//------------------------------------------------------------------------------------------------------------
// Format: 'N'+'Four letters of namespace'
// NOTE: You cannot put a 'namespace' inside of a 'class'
namespace NFWK      // Must be a namespace because we are adding some configs in it
{
// NOTE: None of these files should include anything ( Unless it is a collection? )
// NOTE: Moving out of namespace should be done in the same file if required
#if !__has_include ("FrameworkCfg.hpp")
#include "Platforms/DefaultCfg.hpp"
#endif

#include "Platforms/Common.hpp"
#include "Platforms/CompileTime.hpp"
#include "Platforms/Platform.hpp"
//#include "MemUtils.hpp"
//#include "MemStorage.hpp"
//#include "StrStorage.hpp"
//#include "StrPool.hpp"

#include "Application.hpp"
};
//------------------------------------------------------------------------------------------------------------
using NFWK::NCTM::operator""_ps;

#include "Platforms/RTL.hpp"    // Must be in global namespace  // Comment this out if you linking with a default Run Time Library
//#endif
