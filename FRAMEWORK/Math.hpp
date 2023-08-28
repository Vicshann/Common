
#pragma once


struct NMATH
{

//======================================================================================================================
// https://forum.arduino.cc/t/divmod10-a-fast-replacement-for-10-and-10-unsigned
// NOTE: Generic UDiv10, much slower than Mult with magic constant
// Checking X at runtime will make it slower?  ( X may take less bytes and rest of 'q = (q>>8) + x' will be redundant )
// Best solution on ARM?
//
template<typename T> constexpr _finline static T Div10U(T in) requires (IsUnsigned<T>::V)
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
template<typename T> constexpr _finline static T DivMod10U(T in, XRef<T> mod) requires (IsUnsigned<T>::V)
{
 T d = Div10U(in);
 mod = in - ((d << 3) + (d << 1));
 return d;
}
//----------------------------------------------------------------------------------------------------------------------
template<typename T> constexpr _finline static T ModDiv10U(T in, XRef<T> div) requires (IsUnsigned<T>::V)
{
 T d = div = Div10U(in);
 return in - ((d << 3) + (d << 1));
}
//----------------------------------------------------------------------------------------------------------------------
template<typename T> constexpr _finline static T Mod10U(T in) requires (IsUnsigned<T>::V)
{
 return ModDiv10U(T());
}
//----------------------------------------------------------------------------------------------------------------------
//======================================================================================================================
// https://github.com/OP-TEE/optee_os/blob/master/lib/libutils/isoc/arch/arm/
// Software DIV
template<typename T> constexpr _ninline static T DivModU(T num, T den, XRef<T> mod) requires (IsUnsigned<T>::V)  // Returns quotient. remainder is in 'mod'
{
 T i = 1, q = 0;
 if(!den){mod=0; return 0;}  // What to return?             //{this->r = (T)-1;	return;} // division by 0

 while(!(den >> ((sizeof(T)*8)-1)))  // 31/63 // Until highest bit set // Very slow on X32! // TODO: Optimize with hardware
  {
   i = i << 1;	  // count the max division steps
   den = den << 1;   // increase p until it has maximum size
  }

 while(i > 0)
  {
   q = q << 1;	 // write bit in q at index (size-1)
   if(num >= den)
    {
     num -= den;
     q++;
    }
   den = den >> 1; 	// decrease p
   i = i >> 1; 	// decrease remaining size in q
  }
 mod = num;
 return q;
}
//----------------------------------------------------------------------------------------------------------------------
template<typename T> constexpr _finline static T ModDivU(T num, T den, XRef<T> div) requires (IsUnsigned<T>::V)   // Returns remainder. quotient is in 'div'
{
 T rem;
 div = UDivMod(num, den, rem);
 return rem;
}
//----------------------------------------------------------------------------------------------------------------------
template<typename T> constexpr _ninline static T DivModS(T num, T den, XRef<T> mod) requires (!IsUnsigned<T>::V)
{
 using M = decltype(TypeToUnsigned<T>());
 bool qs = 0, rs = 0;
 if(((num < 0) && (den > 0)) || ((num > 0) && (den < 0)))qs = true;	// quotient shall be negate
 if(num < 0){num = -num; rs = true;} 	// remainder shall be negate
 if(den < 0)den = -den;
 M r;  // Unsigned T
 T q = DivModU<M>(num, den, r);
 if(qs)q = -q;
 if(rs)r = -r;
 mod = r;
 return q;
}
//----------------------------------------------------------------------------------------------------------------------
template<typename T> constexpr _finline static T ModDivS(T num, T den, XRef<T> div) requires (!IsUnsigned<T>::V)   // Returns remainder. quotient is in 'div'
{
 T rem;
 div = DivModS(num, den, rem);
 return rem;
}
//----------------------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------
// https://stackoverflow.com/questions/664852/which-is-the-fastest-way-to-get-the-absolute-value-of-a-number
// Check for *MAX undefined behaviour?
template<typename T> constexpr _finline static T Abs(T num)
{
 if(num < 0)num = -num;
 return num;
}
//----------------------------------------------------------------------------------------------------------------------


};


/*
 sint32 r = 99999;
 sint32 q = NFWK::NMATH::DivModS(4, 54, r);
   sint32 q1 = 4 / 54;
   sint32 r1 = 4 % 54;
 return q+q1+r1;



*/
