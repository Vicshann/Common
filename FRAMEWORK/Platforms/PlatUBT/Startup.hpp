
#pragma once

//------------------------------------------------------------------------------------------------------------
//private:
// TODO: Pack these fields in a structure and scramble them (OPTIONAL)
struct SSINF
{
 vptr            STInfo;  // Pointer to stack frame
 vptr            RetAddr; // Required for UBoot scanning
 vptr            UBootBase;
 vptr*           JmpTable;
 uint            ArgC;
 achar**         ArgV;

} static inline SInfo = {};

static const uint32 MaxUBootSize = 1024*1024;   // 1MB
//------------------------------------------------------------------------------------------------------------
// Return address of MAIN is somewhere inside UBOOT
// Look for UBOOT`s base in GD table
static vptr FindBaseOfUBOOT(vptr AddrInUboot)
{
 vptr* GTbl = GetGDPtr();
 size_t MinAddr = (size_t)AddrInUboot - MaxUBootSize;
 size_t MaxAddr = (size_t)AddrInUboot + MaxUBootSize;
 for(int idx=0;idx < 32;idx++)
  {
   size_t val = (size_t)GTbl[idx];
   if(val < MinAddr)continue;  // Too small
   if(val > MaxAddr)continue;  // Too big
   vptr* p = (vptr*)val;
   if(p[-1] || p[-2] || p[-3] || p[-4])continue; // All zeroes above must be
   return (vptr)val;
  }
 return nullptr;
}
//-----------------------------------------------------------------------------------------
// All members of global table are pointer-aligned
static vptr* FindJumpTable(vptr AddrInUboot)
{
 vptr* GTbl = GetGDPtr();
 size_t MinAddr = (size_t)AddrInUboot - MaxUBootSize;
 size_t MaxAddr = (size_t)AddrInUboot + MaxUBootSize;
 for(int idx=0;idx < 32;idx++)
  {
   size_t val = (size_t)GTbl[idx];
   if(val < 0x00010000)continue;  // Too small
   if(val > 0xFFFF0000)continue;  // Too big
   if((val < (size_t)GTbl)||(val > ((size_t)GTbl+0x100000)))continue; // Allocated in next block (separate malloc)   // NOTE: This check is unreliable
   size_t* TstTbl = (size_t*)val;
   for(int ctr=0;ctr < 24;ctr++)
    {
     size_t addr = TstTbl[ctr];
     if((addr < MinAddr)||(addr > MaxAddr)){TstTbl=nullptr; break;}  // Out of range
    }
   if(TstTbl)return (vptr*)TstTbl;   // Found it if not nulled
  }
 return nullptr;
}
//-----------------------------------------------------------------------------------------
// The idea is to detect commands that may come in different order. Tgt1 should be the first instruction
template<typename T> struct SExecCmdLineFinder_Base
{
 uint8* Ptr1 = nullptr;   // +0
 uint8* Ptr2 = nullptr;   // +1
 uint8* Ptr3 = nullptr;   // +3

uint8* Update(uint8* Addr, uint32 val)
{
// uint32 val = *(uint32*)Addr;
 if(!((val ^ T::Tgt1) & T::Msk1))Ptr1 = Addr;
 if(!((val ^ T::Tgt2) & T::Msk2))Ptr2 = Addr;
 if(!((val ^ T::Tgt3) & T::Msk3))Ptr3 = Addr;
// if(((size_t)Addr >= 0x0EF3D410) && ((size_t)Addr < 0x0EF3D41C)){UBTMSG("FindExecCmdLine: Addr=%p, val=%08X, Ptr1=%p, Ptr2=%p, Ptr3=%p", Addr, val, Ptr1, Ptr2, Ptr3);}
 if(!Ptr1 || !Ptr2 || !Ptr3)return nullptr;  // Not all is present yet
 if(NMATH::Abs(Ptr2-Ptr1) > T::Dist)return nullptr;  // Some matches are too far from each other
 if(NMATH::Abs(Ptr3-Ptr2) > T::Dist)return nullptr;
 return Ptr1;
}

};

struct SExecCmdLineFinder_ARM32a: public SExecCmdLineFinder_Base<SExecCmdLineFinder_ARM32a>
{
/*
  04 00 11 E3     TST      R1, #4
  03 10 A0 03     MOVEQ    R1, #3       // May be MOVNE
  0B 10 A0 13     MOVNE    R1, #0xB     // May be MOVEQ
  EA CE FF EA     B        ExecListCmd
*/
 static constexpr uint32 Tgt1 = 0xE3110004;    // TST   R1, #4
 static constexpr uint32 Tgt2 = 0x03A01003;    // MOVEQ R1, #3
 static constexpr uint32 Tgt3 = 0x13A0100B;    // MOVNE R1, #0xB
 static constexpr uint32 Msk1 = ~0x0F0000u;    // Mask out registers field
 static constexpr uint32 Msk2 = ~0x00F000u;
 static constexpr uint32 Msk3 = ~0x00F000u;
 static constexpr uint32 Dist = 8;  // Should consider possible instruction reordering

};

struct SExecCmdLineFinder_ARM32b: public SExecCmdLineFinder_Base<SExecCmdLineFinder_ARM32b>   // Old U-BOOT
{
/*
  03 10 A0 E3    MOV   R1, #3
  E0 F7 FF EA    B     EP_ExecCmd
*/
 static constexpr uint32 Tgt1 = 0xE3A01003;    // MOV  R1, #3
 static constexpr uint32 Tgt2 = 0xEAFFF7E0;    // B EP_ExecCmd
 static constexpr uint32 Tgt3 = 0;             // Unused
 static constexpr uint32 Msk1 = -1;            // Exact match
 static constexpr uint32 Msk2 = 0xFF000000;    // Mask out call target
 static constexpr uint32 Msk3 = 0;             // Unused, always accept
 static constexpr uint32 Dist = 4;

};

struct SExecCmdLineFinder_ARM64a: public SExecCmdLineFinder_Base<SExecCmdLineFinder_ARM64a>   // Old U-BOOT
{
/*
  21 00 1E 12   AND    W1, W1, #4
  62 00 80 52   MOV    W2, #3
  3F 00 1F 6B   CMP    W1, WZR
  61 01 80 52   MOV    W1, #0xB
  41 00 81 1A   CSEL   W1, W2, W1, EQ
  A7 60 FF 17   B      ExecListCmd
*/
 static constexpr uint32 Tgt1 = 0x121E0021;    // AND W1, W1, #4
 static constexpr uint32 Tgt2 = 0x52800062;    // MOV W2, #3
 static constexpr uint32 Tgt3 = 0x52800161;    // MOV W1, #0xB
 static constexpr uint32 Msk1 = ~0x03FFu;      // Mask out registers field
 static constexpr uint32 Msk2 = ~0x001Fu;
 static constexpr uint32 Msk3 = ~0x001Fu;
 static constexpr uint32 Dist = 8;

};
//-----------------------------------------------------------------------------------------
static vptr FindExecCmdLine(vptr BaseAddr, size_t Size)
{
// UBTMSG("FindExecCmdLine: %p, %08X",BaseAddr,Size);
#if defined(CPU_ARM)
 static constexpr uint32 Step = 4;
#if defined(ARCH_X64)
 SExecCmdLineFinder_ARM64a fnd1;
#else
#ifdef FWK_OLD_UBOOT
SExecCmdLineFinder_ARM32b fnd1;
#else
SExecCmdLineFinder_ARM32a fnd1;
#endif
#endif
#elif defined(CPU_X86)
 static constexpr uint32 Step = 1;
 // Not implemented yet
#endif

// SExecCmdLineFinder_ARM32a fnd1;
// SExecCmdLineFinder_ARM32b fnd2;
// SExecCmdLineFinder_ARM64a fnd3;

 uint8* CurPtr = (uint8*)((size_t)BaseAddr & ~(16-1));
 uint8* EndPtr = &CurPtr[Size-16];
 uint8* Ptr = nullptr;
 for(;CurPtr < EndPtr;CurPtr += Step)
  {
   uint32 val = *(uint32*)CurPtr;
   if(Ptr=fnd1.Update(CurPtr, val))break;
//   if(Ptr=fnd2.Update(CurPtr, val))break;
//   if(Ptr=fnd3.Update(CurPtr, val))break;
  }
// if(Ptr){UBTMSG("FindExecCmdLine: Found at %p", Ptr);}
//   else {UBTMSG("FindExecCmdLine: Nothing found!");}
 return Ptr;
}
//-----------------------------------------------------------------------------------------
// ARM Cortex A8 (A9 ?)
_finline static void ClearCachesARMv7(void)
{
//#ifdef INLINECACHECTRL
// ARM Data transfer instructions from processor registers to coprocessor registers ( Write to coprocessor register )
// The operation of a storage system is usually performed by a coprocessor CP15 which contain 16 individual 32 Bit register (0-15)
// MCR{cond} p15,<Opcode_1>,<Rd>,<CRn>,<CRm>,<Opcode_2>
//
// cond? Condition code for instruction execution . When cond When ignored, the instruction is executed unconditionally .
// Opcode_1? Coprocessor specific opcodes . about CP15 For example ,opcode1=0
// Rd? As a source register ARM register , Its value will be transferred to the coprocessor register , Or transfer the value of coprocessor register to the register , Usually it is R0
// CRn? Coprocessor registers as target registers , Its number is C~C15.
// CRm? An additional destination register or source operand register in the coprocessor . If you don't need to set additional information , take CRm Set to c0, Otherwise, the result is unknown
// Opcode_2? Optional coprocessor specific opcodes .( To distinguish different physical registers of the same number , When additional information is not required , Designated as 0)
//
// CR7: Cache and write cache control
/*
#if !CONFIG_IS_ENABLED(SYS_ICACHE_OFF)
	mcr	p15, 0, r0, c7, c5, 0	@ invalidate icache
	mcr     p15, 0, r0, c7, c10, 4	@ DSB
	mcr     p15, 0, r0, c7, c5, 4	@ ISB
#endif
*/

/*
.set DC_ON, (0x1<<2)
.set IC_ON, (0x1<<12)
.set CTRL_M_BIT,  (1 << 0)
.set CTRL_C_BIT,  (1 << 2)
.set CTRL_B_BIT,  (1 << 7)
.set CTRL_I_BIT, (1 << 12)

ASM_FUNC(ArmEnableInstructionCache)
  ldr     R1,=IC_ON
  mrc     p15,0,R0,c1,c0,0      @Read control register configuration data
  orr     R0,R0,R1              @Set I bit
  mcr     p15,0,r0,c1,c0,0      @Write control register configuration data
  dsb
  isb
  bx      LR

ASM_FUNC(ArmDisableInstructionCache)
  ldr     R1,=IC_ON
  mrc     p15,0,R0,c1,c0,0      @Read control register configuration data
  bic     R0,R0,R1              @Clear I bit.
  mcr     p15,0,r0,c1,c0,0      @Write control register configuration data
  dsb
  isb
  bx LR

ASM_FUNC(ArmDisableCachesAndMmu)
  mrc   p15, 0, r0, c1, c0, 0           @ Get control register
  bic   r0, r0, #CTRL_M_BIT             @ Disable MMU
  bic   r0, r0, #CTRL_C_BIT             @ Disable D Cache
  bic   r0, r0, #CTRL_I_BIT             @ Disable I Cache
  mcr   p15, 0, r0, c1, c0, 0           @ Write control register
  dsb
  isb
  bx LR
*/
#ifndef __aarch64__
 __asm__ __volatile__ ("MCR p15,0,%0,c7,c5, 0    \n\t"     // @Invalidate entire instruction cache [ArmInvalidateInstructionCache]   // Invalidate Instruction TLB
                       "MCR p15,0,%0,c7,c5, 6    \n\t"     // @Invalidate Branch predictor array    // Invalidate Instruction TLB Single Entry ???
//                       "MCR p15,0,%0,c8,c7, 0    \n\t"  // Invalidate entire Unified Main TLB
                       "MCR p15,0,%0,c7,c10,4    \n\t"     // Data sync barrier (formerly drain write buffer)   // Clean Data Cache Line (using Index)
                       "MCR p15,0,%0,c7,c5, 4    \n\t"     // Instruction sync barrier  // Invalidate Instruction TLB Entry on ASID match
//                       "DSB SY\n\t"
//                       "ISB SY\n\t"
                       ::"r"(0));
#endif
/*#else
 void* (*pProc_CacheProcA)(void)  = SYSPROC_CacheProcA;
 void* (*pProc_CacheProcB)(void*) = SYSPROC_CacheProcB;
 void  (*pProc_CacheProcC)(void*) = SYSPROC_CacheProcC;

// SLOGTXT("CC1a\n"_es);
 void* v0 = pProc_CacheProcA();
// SLOGTXT("CC1b\n"_es);
 void* v1 = pProc_CacheProcB(v0);
// SLOGTXT("CC1c\n"_es);
 pProc_CacheProcC(v1);
// SLOGTXT("CC1d\n"_es);
#endif    */
}
//-----------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------

public:
//------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------
static _finline size_t GetArgC(void){return (size_t)SInfo.ArgC;}   // On Windows should be always 1? // Be careful with access to SInfo using casts. Clang may optimize out ALL! code because of it
static _finline const achar** GetArgV(void){return (const achar**)SInfo.ArgV;}
static _finline const achar** GetEnvP(void){return nullptr;}
//------------------------------------------------------------------------------------------------------------
static _finline PX::fdsc_t GetStdIn(void)  {return PX::STDIN; }  // 0
static _finline PX::fdsc_t GetStdOut(void) {return PX::STDOUT;}  // 1
static _finline PX::fdsc_t GetStdErr(void) {return PX::STDERR;}  // 2
//------------------------------------------------------------------------------------------------------------

static _finline bool IsLoadedByLdr(void) {return true;}    // Always return to the loader
static _finline bool IsDynamicLib(void) {return false;}
//------------------------------------------------------------------------------------------------------------
static inline vptr* GetGDPtr(void)       // UBOOT`s GD ptr (new)
{
	vptr* gd_ptr;
if constexpr (IsArchX64)      // (sizeof(void*) > 4)
{
 __asm__ volatile("mov %0, x18\n" : "=r" (gd_ptr));
} else
{
	__asm__ volatile("mov %0, r9\n" : "=r" (gd_ptr));    // NOTE: Cland may not preserve R9 without modification!
}
	return gd_ptr;
}
//-----------------------------------------------------------------------------------------
static inline void SaveOldCtx(void)
{
 if constexpr (IsArchX32)
 {
  __asm__ volatile("mov r9, r8\n");
 }
}
//-----------------------------------------------------------------------------------------
static inline void RestoreOldCtx(void)
{
 if constexpr (IsArchX32)
 {
  __asm__ volatile("mov r8, r9\n");
 }
}
//-----------------------------------------------------------------------------------------
// %rdi, %rsi, %rdx, %rcx, %r8 and %r9
// DescrPtr must be set to 'ELF Auxiliary Vectors' (Stack pointer at ELF entry point)
//
/*
argv[0] is not guaranteed to be the executable path, it just happens to be most of the time. It can be a relative path from the current
working directory, or just the filename of your program if your program is in PATH, or an absolute path to your program, or the name a
symlink that points to your program file, maybe there are even more possibilities. It's whatever you typed in the shell to run your program.
*/
// Copies a next param to user`s buffer
// A spawner process is responsible for ARGV
// Quotes are stripped by the shell and quoted args are kept intact
// NOTE: Do not expect the return value to be an argument index!
static uint GetCmdLineArg(sint& AOffs, achar* DstBuf, uint BufLen=-1)
{
 if(DstBuf)*DstBuf = 0;
 if(AOffs < 0)return -3;     // Already finished
 if(AOffs >= (sint)GetArgC())return -2;    // No such arg
 const achar* CurArg = GetArgV()[AOffs++];
 uint  ArgLen = 0;
 if(!DstBuf)
  {
   while(CurArg[ArgLen])ArgLen++;
   return ArgLen+1;   // +Space for terminating 0
  }
 for(;CurArg[ArgLen] && (ArgLen < BufLen);ArgLen++)DstBuf[ArgLen] = CurArg[ArgLen];
 DstBuf[ArgLen] = 0;
 if(AOffs >= (sint)GetArgC())AOffs = -1;
 return ArgLen;
}
//------------------------------------------------------------------------------------------------------------
template<typename T> static sint GetEnvVar(T* Name, T DstBuf, size_t BufCCnt)
{
 return -1;
}
//------------------------------------------------------------------------------------------------------------
// AppleInfo on MacOS
static sint GetSysInfo(uint InfoID, void* DstBuf, size_t BufSize)
{
 return -1;
}
//------------------------------------------------------------------------------------------------------------
//
static _finline void* GetModuleBase(void)
{
 return nullptr;//SInfo.TheModBase;
}
//------------------------------------------------------------------------------------------------------------
//
static _finline size_t GetModuleSize(void)
{
 return 0;//SInfo.TheModSize;
}
//------------------------------------------------------------------------------------------------------------
// Returns full path to current module and its name in UTF8
static size_t _finline GetModulePath(achar* DstBuf, size_t BufSize=-1)
{
 return 0;
}
//------------------------------------------------------------------------------------------------------------
//#define DJTAPI(name) *SysApi.name.GetPtrSC<volatile vptr*>() = (vptr)0xAABBCCDD; //SInfo.JmpTable[(int)NUBOOT::EApiIdx::name]
#define DJTAPI(name) *UnbindPtr(SAPI::name.GetPtrSC<vptr*>()) = SInfo.JmpTable[(int)NUBOOT::EApiIdx::name]

// MacOS 'dyld main' args: ACnt, Args, EVars, AVars
// Without the -osx_min_version flag to ld, arguments are indeed passed on stack to _start ????????????????
//
static sint InitStartupInfo(void* StkFrame=nullptr, void* ArgA=nullptr, void* ArgB=nullptr, void* ArgC=nullptr)
{
 SInfo.STInfo    = StkFrame;  // Pointer to stack frame (No useful information there)
 SInfo.RetAddr   = ArgC;
 SInfo.ArgV      = (achar**)ArgB;
 SInfo.ArgC      = (uint)ArgA;
 SInfo.UBootBase = FindBaseOfUBOOT(SInfo.RetAddr);
 if(!SInfo.UBootBase)return -1;
 SInfo.JmpTable  = FindJumpTable(SInfo.RetAddr);
 if(!SInfo.JmpTable)return -2;

 DJTAPI(get_version);   // *SAPI::get_version.GetPtrSC<vptr>() = SInfo.JmpTable[(int)NUBOOT::EApiIdx::get_version];
 DJTAPI(getc);
 DJTAPI(tstc);
 DJTAPI(putc);
 DJTAPI(puts);
 DJTAPI(printf);
 DJTAPI(install_hdlr);
 DJTAPI(free_hdlr);
 DJTAPI(malloc);
 DJTAPI(free);
 DJTAPI(udelay);
 DJTAPI(get_timer);
 DJTAPI(vprintf);
 DJTAPI(do_reset);
 DJTAPI(getenv);
 DJTAPI(setenv);
 DJTAPI(simple_strtoul);
 DJTAPI(strict_strtoul);
 DJTAPI(simple_strtol);
 DJTAPI(strcmp);
 DJTAPI(i2c_write);
 DJTAPI(i2c_read);

 DBGDBG("StkFrame=%p, RetAddr=%p, ArgV=%p, ArgC=%u",StkFrame,ArgC,ArgA,ArgB);    // On Windows syscalls are not initialized yet at this point!
 DBGMSG("Base of UBOOT: %p",SInfo.UBootBase);
 DBGMSG("Jump Table: %p",SInfo.JmpTable);

 vptr pExecCmdLine = FindExecCmdLine((void*)((size_t)SInfo.JmpTable[(int)NUBOOT::EApiIdx::malloc] & ~0xFFFF), 0x20000);     // ((size_t)this->malloc & ~0xFFFF)
 *UnbindPtr(SAPI::ExecCmdLine.GetPtrSC<vptr*>()) = pExecCmdLine;

 DBGMSG("ExecCmdLine addr: %p",pExecCmdLine);  // DBGMSG   // NOTE: Framework`s logging will make the UBOOT module much bigger
 if(!pExecCmdLine)return -3;
 return 0;
}
//============================================================================================================
static void DbgLogStartupInfo(void)
{
 if(!SInfo.STInfo)return;
 // Log command line arguments
 if(SInfo.ArgV)
  {
   void*  APtr = nullptr;
   char** Args = SInfo.ArgV;
   uint ParIdx = 0;
   LOGDBG("CArguments: ");
   for(uint idx=0;idx < SInfo.ArgC;idx++)
    {
     LOGDBG("  Arg %u: %s",idx,APtr);
    }
  }
 DBGDBG("Done!");
}
//------------------------------------------------------------------------------------------------------------

/* GPtr: ARM X64
0de9ee28: 0de9ef68 00000000 00000383 00000000    h...............
          baudrate
0de9ee38: 0001c200 00000000 00000000 00000000    ................
0de9ee48: 00000000 00000000 00000000 00000000    ................
0de9ee58: 00000000 00000000 00000001 00000000    ................
          evarbuf1
0de9ee68: 01099908 00000000 00000001 00000000    ................
                            ubootbase
0de9ee78: 10000000 00000000 0feaf000 00000000    ................
                            Code?
0de9ee88: 10000000 00000000 001405e8 00000000    ................
          ???
0de9ee98: 0de9ee10 00000000 0de9ee10 00000000    ................
          Zero              ThisGDat
0de9eea8: 0eeaf000 00000000 0de9ee28 00000000    ........(.......
          JmpTbl?
0de9eeb8: 0de9f050 00000000 00000000 00000000    P...............
          ???               ???
0de9eec8: 0de9f110 00000000 0de9f030 00000000    ........0.......
0de9eed8: 00000000 00000000 00000000 00000000    ................
                            *JmpTbl*
0de9eee8: 00000000 00000000 0deb1c50 00000000    ........P.......
0de9eef8: 32353131 00003030 00000000 00000000    115200..........
0de9ef08: 00000000 00000000 00000000 00000000    ................
0de9ef18: 00000000 00000000 04940433 00000000    ........3.......
0de9ef28: 00000000 00000000 00000000 00000000    ................
0de9ef38: 00000000 00000000 00000000 00000000    ................
*/

/* GPtr: ARM X32
                            baudrate
0db01ef0: 0db01fb0 00002383 0001c200 27bc86a4    .....#.........'
0db01f00: 00000000 00000000 00000000 00000001    ................
          evarbuf1                   ubootbase
0db01f10: 0405c40f 00000001 0f000000 0ef22000    ............. ..
0db01f20: 0f000000 000cde9c 0db01ee0 0db01ed0    ................
0db01f30: 0af22000 0db01ef0 0db02028 ffff0958    . ......( ..X...
0db01f40: 0db023b8 0db02018 0ef8f8c0 00000000    .#... ..........
                   *JmpTbl*
0db01f50: 00000000 0db076e8 32353131 00003030    .....v..115200..
0db01f60: 00000000 00000000 00000000 00000000    ................
0db01f70: 00000000 00000000 00000000 002749c2    .............I'.
0db01f80: ffff0940 00000600 000003fc 0db02228    @...........("..
0db01f90: 0013de43 00000000 00000000 003255ba    C............U2.
0db01fa0: 00000000 00000000 0eff0000 00004000    .............@..
0db01fb0: 00000000 00000000 00000000 00000000    ................
0db01fc0: 00000000 00000000 00000000 0000029a    ................
0db01fd0: 00000000 00000215 00000000 00000000    ................
0db01fe0: 00000000 00000000 00000000 00000000    ................
*/
