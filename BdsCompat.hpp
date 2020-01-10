
#pragma once
 
#ifndef BdsCompH
#define BdsCompH
/*
  Copyright (c) 2018 Victor Sheinmann, Vicshann@gmail.com

  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), 
  to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
  and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. 
*/
 

//  An useful stuff for MSVC to work with BDS specific code
//====================================================================================

#pragma warning(push)
#pragma warning(disable:4731)     // frame pointer register 'ebp' modified by inline assembly code

//#pragma optimize(push)                         
//#pragma optimize("y", on)      // Ignored!
//#pragma optimize("2", on)      // Ignored!

// C++Builder _fastcall: The first three parameters are passed (from left to right) in EAX, EDX, and ECX, if they fit in the register. The registers are not used if the parameter is a floating-point or struct type. 
// ArgF: Stack
// ArgE: Stack
// ArgD: Stack
// ArgC: ECX
// ArgB: EDX
// ArgA: EAX
// RetAddr
//---------------------------------------------------------------------------
namespace BDS
{
template <typename R, typename... Types> constexpr int GetArgCount( R(_stdcall *f)(Types ...) ){ return sizeof...(Types); }   // GetArgCount( R(*f)(Types ...) )


template <typename Signature> struct CountArgs;

template <typename Ret, typename... Args> struct CountArgs<Ret(Args...)> 
{
 static constexpr size_t Value = sizeof...(Args);
};

#if !defined(_AMD64_)
//---------------------------------------------------------------------------
// Expose a proc as a BDS fastcall (MSVC proc must be declared as '_stdcall')
// UINT64 is two DWORDS on stack but counted as one argument!
//
#define BDSWRAP(proc) &BDS::BdsFWrap<proc, BDS::GetArgCount(proc)>      // There is some problem with GetArgCount !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
template<void* WProc, int Args> PVOID _stdcall BdsFWrap(void)   // [EAX,EDX,ECX,...] [EAX,EDX,ECX] [EAX,EDX] [EAX] []   // Use of specialization for 1 and 2 arg variants is somehow possible?
{
 static const PVOID Addr = WProc;         // Can`t be taken by an assembler code directly(Results in Null)
 static const int   acnt = Args;  
//  push        ebp       // Nothing will stop the compiler from inserting these!
//  mov         ebp,esp  
 __asm
 {
  pop   EBP               // Useless frame pointer         
  push  EDX               // Ret Addr stays and can be replaced with ECX later
  push  EAX
  mov   EAX, acnt         // Number of VAR arguments
  mov   EDX, 3
  cmp   EAX, EDX          // Only first 3 is placed in registers for BDS '_fastcall'
  cmova EAX, EDX                                          
  sub   EDX, EAX
  xchg  ECX, [ESP+8]      // Store ECX arg and get a RET addr
  lea   ESP, [ESP+EDX*4]  // Step back unused args number  // ESP -= (0,4,8,12)

// Very slow with 1,2 arguments!    // How to optimize this????   // movs will be faster? (Too many calcs for offsets?)
  test  EDX, EDX
  jz    Finish            // ArgCount >= 3, Placed OK already
  test  EAX, EAX
  jz    Finish            // No arguments, stack is unused
  dec   EAX
  jnz   ArgTw             // Two Args
  mov   EAX, [ESP-8]          
  mov   [ESP], EAX
  jmp  Finish
ArgTw:
  mov   EAX, [ESP]          
  mov   [ESP+4], EAX
  mov   EAX, [ESP-4]          
  mov   [ESP], EAX

Finish:
  push  ECX               // Return address
  jmp   [Addr]
 }
//  pop         ebp  
//  ret  
}
//---------------------------------------------------------------------------
// Call a BDS fastcall proc
// Not compatible with Floating Point and structs(UINT64?)?  // MSVC always uses stack for UINT64 (x32) even if a proc is _fastcall!
//                                                                                                                    -12  -8  -4      -8  -4       -4
template<typename R, typename... Args> R _stdcall BdsFCall(PVOID Addr, Args...)       // Can be made simple?  Stack: [EAX,EDX,ECX] or [EAX,EDX] or [EAX]
{
 static const unsigned int asize = (((sizeof...(Args)) < 3)?(sizeof...(Args)):(3));     // constexpr?
//  push        ebp       // Nothing will stop the compiler from inserting these!
//  mov         ebp,esp  
 __asm
 {
  pop   EBP               // Useless frame pointer         
  pop   EDX               // Return addr
  pop   ECX               // Target proc addr  
  mov   EAX, asize        // Size of VAR arguments (0-3)
  test  EAX, EAX
  jz    NoArgs            // No args
  dec   EAX
  jnz   More1Args
  pop   EAX               // First Arg
NoArgs:
  push  EDX               // Place a return address back
  jmp   ECX               // Call the prock
More1Args:
  dec   EAX
  jnz  ThreeArgs
  pop   EAX               // First Arg
  xchg  EDX, [ESP]        // Second Arg   // Replace with return address afterwards
  jmp   ECX               // Call the prock
ThreeArgs:
  pop   EAX               // First Arg
  xchg  ECX, [ESP]        // Second Arg   // Replace with Target proc addr afterwards
  xchg  EDX, [ESP+4]      // Third Arg    // Replace with Return addr afterwards
  xchg  ECX, EDX
  ret                     // Go to a target proc
 }
//  pop         ebp  
//  ret  
}
//---------------------------------------------------------------------------
#else
#define BDSWRAP(proc) proc
template<void* WProc, int Args> PVOID _stdcall BdsFWrap(void){return WProc;}
template<typename R, typename... Args> R _stdcall BdsFCall(PVOID Addr, Args...){return (R)Addr;}
#endif

static inline __int64 DateTimeToUnix(double dtDate)
{
 const double UnixStartDate = 25569; // 01/01/1970
 return ((dtDate - UnixStartDate) * 86400.0); // Round
}

static inline double UnixToDateTime(__int64 USec)
{
 const double UnixStartDate = 25569; // 01/01/1970
 return ((double)USec / 86400.0) + UnixStartDate;
}

static inline double Delphi_Now(void)
{
 return UnixToDateTime(GetTime64(true));
}
//---------------------------------------------------------------------------
static inline UINT& __fastcall RndGenSeed(void)
{
 static UINT Seed = 0;
 return Seed; 
}
//---------------------------------------------------------------------------
static inline void __fastcall DelphiRndSeed(UINT a1)
{
 RndGenSeed() = a1;
}
//------------------------------------------------------------------------------------------------------------
static inline int __fastcall DelphiRandom(UINT a1)
{
 RndGenSeed() = 0x8088405 * RndGenSeed() + 1;
 return (RndGenSeed() * (UINT64)a1) >> 32;
}
//------------------------------------------------------------------------------------------------------------


//---------------------------------------------------------------------------
};
#pragma warning(pop)

//====================================================================================
#endif