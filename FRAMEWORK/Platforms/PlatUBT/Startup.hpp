
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
// Return address is somewhere inside UBOOT
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
static vptr FindExecCmdLine(vptr BaseAddr, size_t Size)
{
#if defined(CPU_ARM)
#if defined(ARCH_X64)
 static constexpr uint32 Tgt1 = 0x121E0021;    // AND W1, W1, #4
 static constexpr uint32 Tgt2 = 0x52800062;    // MOV W2, #3
 static constexpr uint32 Tgt3 = 0x52800161;    // MOV W1, #0xB
 static constexpr uint32 Msk1 = ~0x03FFu;      // Mask out registers field
 static constexpr uint32 Msk2 = ~0x001Fu;
 static constexpr uint32 Msk3 = ~0x001Fu;
#else   // X32
 static constexpr uint32 Tgt1 = 0xE3110004;    // TST   R1, #4
 static constexpr uint32 Tgt2 = 0x03A01003;    // MOVEQ R1, #3
 static constexpr uint32 Tgt3 = 0x13A0100B;    // MOVNE R1, #0xB
 static constexpr uint32 Msk1 = ~0x0F0000u;    // Mask out registers field
 static constexpr uint32 Msk2 = ~0x00F000u;
 static constexpr uint32 Msk3 = ~0x00F000u;
#endif
#elif defined(CPU_X86)
 // Not implemented yet
#endif
 uint32* Code = (uint32*)((size_t)BaseAddr & ~(4-1));
 Size = Size >> 2;   // div 4
 uint32 Offs1 = 0;   // +0
 uint32 Offs2 = 0;   // +1
 uint32 Offs3 = 0;   // +3
 for(uint32 ctr=0;ctr < Size;ctr++)
  {
   uint32 val = Code[ctr];
   if(!((val ^ Tgt1) & Msk1))Offs1 = ctr;
   if(!((val ^ Tgt2) & Msk2))Offs2 = ctr;
   if(!((val ^ Tgt3) & Msk3))Offs3 = ctr;
   if(!Offs1 || !Offs2 || !Offs3)continue;  // Not all is present yet
   if(((ctr-Offs1)+(ctr-Offs2)+(ctr-Offs3)) > 6)continue;
   return &Code[Offs1];
  }
 return nullptr;
}
//-----------------------------------------------------------------------------------------
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
 SInfo.STInfo    = StkFrame;  // Pointer to stack frame
 SInfo.RetAddr   = ArgC;
 SInfo.ArgV      = (achar**)ArgB;
 SInfo.ArgC      = (uint)ArgA;
 SInfo.UBootBase = FindBaseOfUBOOT(SInfo.RetAddr);
 SInfo.JmpTable  = FindJumpTable(SInfo.RetAddr);

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

 vptr pExecCmdLine = FindExecCmdLine((void*)((size_t)SInfo.JmpTable[(int)NUBOOT::EApiIdx::malloc] & ~0xFFFF), 0x20000);     // ((size_t)this->malloc & ~0xFFFF)
 *UnbindPtr(SAPI::ExecCmdLine.GetPtrSC<vptr*>()) = pExecCmdLine;

 DBGDBG("StkFrame=%p, RetAddr=%p, ArgV=%p, ArgC=%u",StkFrame,ArgC,ArgA,ArgB);    // On Windows syscalls are not initialized yet at this point!
 DBGMSG("Base of UBOOT: %p",SInfo.UBootBase);
 DBGMSG("ExecCmdLine addr: %p",pExecCmdLine);
 if(!pExecCmdLine)return -1;
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
