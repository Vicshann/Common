
#pragma once


struct NMATH
{

//======================================================================================================================
// https://forum.arduino.cc/t/divmod10-a-fast-replacement-for-10-and-10-unsigned
// NOTE: Generic UDiv10, much slower than Mult with magic constant
// Checking X at runtime will make it slower?  ( X may take less bytes and rest of 'q = (q>>8) + x' will be redundant )
// Best solution on ARM?
//
template<typename T> constexpr _finline static T UDiv10(T in) requires (IsPositive<T>::V)
{
 T x = (in|1) - (in>>2); // div = in/10 <~~> div = 0.75*in/8
 T q = (x>>4) + x;
 if constexpr ((T)-1 > 0xFF)      // NOTE: sizeof(T) may incorrectly detect size of a class as larger if it contains anything besides the type itself
  {
   x = q;
   q = (q>>8) + x;
   q = (q>>8) + x;
   if constexpr ((T)-1 > 0xFFFF)
    {
     q = (q>>8) + x;
     q = (q>>8) + x;
     if constexpr ((T)-1 > 0xFFFFFFFF)
      {
       q = (q>>8) + x;
       q = (q>>8) + x;
       q = (q>>8) + x;
       q = (q>>8) + x;
      }
    }
  }
 return q >> 3;     // We need to lose 3 low bits
}
//----------------------------------------------------------------------------------------------------------------------
template<typename T> constexpr _finline static T UDivMod10(T in, XRef<T> mod) requires (IsPositive<T>::V)
{
 T d = UDiv10(in);
 mod = in - ((d << 3) + (d << 1));
 return d;
}
//----------------------------------------------------------------------------------------------------------------------
template<typename T> constexpr _finline static T UModDiv10(T in, XRef<T> div) requires (IsPositive<T>::V)
{
 T d = div = UDiv10(in);
 return in - ((d << 3) + (d << 1));
}
//----------------------------------------------------------------------------------------------------------------------
template<typename T> constexpr _finline static T UMod10(T in) requires (IsPositive<T>::V)
{
 return UModDiv10(T());
}
//----------------------------------------------------------------------------------------------------------------------

};
