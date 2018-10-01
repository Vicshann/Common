
#pragma once

#include <intrin.h>

/*   // Such things increase compilation time
#ifdef _X64BIT
 static_assert((sizeof(N) == sizeof(char))&&(sizeof(N) == sizeof(short))&&(sizeof(N) == sizeof(long))&&(sizeof(N) == sizeof(long long)), "Operand size mismatch");
#else
 static_assert((sizeof(N) == sizeof(char))&&(sizeof(N) == sizeof(short))&&(sizeof(N) == sizeof(long)), "Operand size mismatch");
#endif
*/
//---------------------------------------------------------------------------
template<typename N> constexpr FINLINE N FCALLCNV InterlockedAdd(volatile N* Val, N Num)   // Should fail compilation because of missing return value if type is not of any specified sizes
{
#ifdef _ISGCC    // Else try MSVC or fail
 return __sync_fetch_and_add(Val, Num);     // Same name for all sizes
#else           // Else try MSVC or fail
//  if constexpr (sizeof(N) == 8)return _InterlockedExchangeAdd8((char*)Val, (char)Num);
//    else _InterlockedExchangeAdd((long*)Val, (long)Num); 
/* switch(sizeof(N))
  {
   case 8:  return _InterlockedExchangeAdd8((char*)Val, (char)Num);
   case 16: return _InterlockedExchangeAdd16((short*)Val, (short)Num);
   case 32: return _InterlockedExchangeAdd((long*)Val, (long)Num);
#ifdef _X64BIT
   case 64: return 0;
#endif
  }  */
#endif
 return 0;
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------

