
#pragma once

//===========================================================================
// As array of u32 for consistency with x32/x64
// The may_alias attribute tells the compiler that __m128d* can alias other types the same way that char* can, for the purposes of optimization based on C++ strict-aliasing rules.
// MSVC?
using v128 = uint32 __attribute__((__vector_size__(16), __aligned__(16), __may_alias__));
using v256 = uint32 __attribute__((__vector_size__(32), __aligned__(32), __may_alias__));
using v512 = uint32 __attribute__((__vector_size__(64), __aligned__(64), __may_alias__));
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
namespace NINTR
{

};
//===========================================================================
