
#pragma once

struct HSMC
{
// 0x82000000-0x8200FFFF : SMC32: SiP Service Calls
//
// R7 - ClientID (Optional)
// R6 - SessionID (Optional)
// R1-R5 - Parameters
// R0 - Function
 using THookProc = void (*)(void* Param, void** ArgList);
//---------------------------------------------------------------------------
// "Synchronous Abort" handler, esr 0xf2000000
// ELR:     Addr of the instruction which caused the exception
// LR:      Return addr from the function in which the exception occured
//
static void SetBreakpoint(size_t Addr)
{
 *((uint32*)Addr) = 0xd4200000;  // 0x00 0x00 0x20 0xd4 : BRK
}
//-----------------------------------------------------------------------------------------
// +/- 32 MB : B : XXXX XXNN  NNNN NNNN  NNNN NNNN  NNNN NNNN  : 0x14000000  : 0x03FFFFFF signed in DWORDs from branch instr
//
static uint32 CalcBranch(void* From, void* To)
{
 size_t offs = ((size_t)To - (size_t)From) >> 2;
 return 0x14000000 | (offs & 0x03FFFFFF);
}
//-----------------------------------------------------------------------------------------
// A Function Identifier is passed in register W0
// Arguments are passed in registers X1-X6  (Optional W7 for ClientID)
// Results are returned in X0-X3
// NOTE: Always reserve 8 slots for both, Args and Returns
//
__attribute__((naked)) static void CallSMC(void** ArgList)
{
#ifdef ARCH_X64
 asm volatile (
   "MOV x8, x0 ;"
   "STP X29, X8, [SP,#-0x10]! ;"    // Save SP(X29) and ArgList
   "ldp	x0, x1, [x8, #16 * 0] ;"
   "ldp	x2, x3, [x8, #16 * 1] ;"
   "ldp	x4, x5, [x8, #16 * 2] ;"
   "ldp	x6, x7, [x8, #16 * 3] ;"
   "SMC #0 ;"
   "LDP X29, X8, [SP],#0x10 ;"      // Restore SP and ArgList
   "stp	x0, x1, [x8, #16 * 0] ;"
   "stp	x2, x3, [x8, #16 * 1] ;"
   "stp	x4, x5, [x8, #16 * 2] ;"
   "stp	x6, x7, [x8, #16 * 3] ;"
   "RET ;"
   ::: "x0","x1","x2","x3","x4","x5","x6","x7","x8","x9"   // Clobbers X0-X9
 );
#else
 // TODO ?
#endif
}
//-----------------------------------------------------------------------------------------
// Expect X0-X7 as input for ArgList and X0-X7 as output for RetList
// This will be copied to an address above UBootBase
// r19-r29 and SP must be preserved
//
__attribute__((naked)) static void CallHook(void)  // Size = 0x48
{
#ifdef ARCH_X64
// Alloc space on stack for 8 Args and 8 Rets
// Keep stack aligned to 16
 asm volatile (                       // ; or \n\t
   "sub	sp, sp, #0x50 ;"  // Save LR,PC and arguments in X0-X9  // Order of registers is same in memory, sub their size from SP to get the pointer
   "stp	x0, x1, [sp, #16 * 0] ;"
   "stp	x2, x3, [sp, #16 * 1] ;"
   "stp	x4, x5, [sp, #16 * 2] ;"
   "stp	x6, x7, [sp, #16 * 3] ;"
   "stp	x8, LR, [sp, #16 * 4] ;"    // Save original LR (just in case)  // X8 is where to return from hook

   "MOV X0, X9 ;"      // Hook Proc Argument    "LDR X0, [PC,0x30] ;"
   "MOV X1, SP ;"      // Arg list
   "BLR X10 ;"

   "ldp	x0, x1, [sp, #16 * 0] ;"
   "ldp	x2, x3, [sp, #16 * 1] ;"
   "ldp	x4, x5, [sp, #16 * 2] ;"
   "ldp	x6, x7, [sp, #16 * 3] ;"
   "ldp	x8, LR, [sp, #16 * 4] ;"
   "add	sp, sp, #0x50 ;"
   "BR X8 ;"
   ::: "x0","x1","x2","x3","x4","x5","x6","x7","x8","x9"   // Clobbers X0-X9
 );
#else
 // TODO ?
#endif
}
//-----------------------------------------------------------------------------------------
// Long jump is placed above UBootBase
// Replaces all '03 00 00 D4 SMC #0' with Branch
// https://gcc.gnu.org/onlinedocs/gcc/Other-Builtins.html    // __builtin_assume_aligned
// NOTE: Compiler will ignore any other alignment hints and will try to copy memory by 16 byte blocks, craching CPU if Src or Dst addr is not aligned to 16
//
static _ninline uint32 HookCallsToSMC(void* HookParam, THookProc HookProc, void* UBootBase=SInfo.UBootBase)
{
 constexpr int BlkSize  = 0x200;
 constexpr int MaxStubs = (BlkSize / 16) - 2;  // 2 for header stub

 uint32* DstAddr = (uint32*)&((uint8*)UBootBase)[-BlkSize];
 bool HaveHooks = ((DstAddr[0] == 0x58000109) && (DstAddr[3] == 0xD61F0160));  //  May be already hooked, just update pointers then
 DBGMSG("Writing stubs at %p", DstAddr);
 DstAddr[0] = 0x58000109;  // "LDR X9, HookParam ;"
 DstAddr[1] = 0x580000AA;  // "LDR X10, HookProc ;"
 DstAddr[2] = 0x5800004B;  // "LDR X11, XProcAddr ;"
 DstAddr[3] = 0xD61F0160;  // "BR X11 ;"
 *(void**)&DstAddr[4] = (void*)CallHook;
 *(void**)&DstAddr[6] = (void*)HookProc;
 *(void**)&DstAddr[8] = HookParam;   // NOTE: Better not point this to Stack ot hooks will crash if called after the app returned to UBOOT
 if(HaveHooks){DBGMSG("Already hooked!"); return 0;}

 uint32 StubArrIdx = 10;
 uint32 Size = MaxUBootSize >> 2;  // div 4
 uint32* DataPtr = (uint32*)UBootBase;
 for(uint32 ctr=0,sidx=0;ctr < Size;ctr++)
  {
   if(DataPtr[ctr] != 0xD4000003)continue;    // SMC #0
   uint32* Addr = &DataPtr[ctr];
   DBGMSG("Hooking SMC at %p", Addr);

   DstAddr[StubArrIdx+0] = 0x58000048;  // "LDR X8, RetAddr ;"
   DstAddr[StubArrIdx+1] = CalcBranch(&DstAddr[StubArrIdx+1], DstAddr);
   *(void**)&DstAddr[StubArrIdx+2] = &Addr[1];

   *Addr = CalcBranch((void*)Addr, &DstAddr[StubArrIdx]);   // Set hook
   StubArrIdx += 4;
   sidx++;
   if(sidx >= MaxStubs){DBGMSG("Max stubs reached!"); break;}
  }
 DBGMSG("Done hooking!");
 return 0;
}
//-----------------------------------------------------------------------------------------
//---------------------------------------------------------------------------
};
