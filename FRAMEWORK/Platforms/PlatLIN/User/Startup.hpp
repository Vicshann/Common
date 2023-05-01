
#pragma once

//------------------------------------------------------------------------------------------------------------
private:
// TODO: Pack these fields in a structure and scramble them (OPTIONAL)
struct SSINF
{
 void*            STInfo;  // Pointer to stack frame info, received from kernel  // PEB on windows?
 achar**          CLArgs;  // Points to ARGV array  // Not split on windows!
 achar**          EVVars;  // Points to EVAR array (cannot cache this pointer?)
 achar**          AuxInf;  // Auxilary vector (Not for MacOS or Windows)     // LINUX/BSD: ELF::SAuxVecRec*   // MacOS: Apple info (Additional Name=Value list)

 void*  TheModBase;
 size_t TheModSize;
 achar  SysDrive[8];
 bool   HaveLoader;

// Exe path (Even if this module is a DLL)
// Current directory(at startup)   // Required?
// Working dir (Not have to be same as current dir)
// Temp path
} static inline SInfo = {};
//------------------------------------------------------------------------------------------------------------

#if defined(SYS_UNIX) || defined(SYS_ANDROID)  //|| defined(_SYS_BSD)
static ELF::SAuxVecRec* GetAuxVRec(size_t Type)
{
 for(ELF::SAuxVecRec* Rec=(ELF::SAuxVecRec*)SInfo.AuxInf;Rec->type != ELF::AT_NULL;Rec++)
  {
   if(Rec->type == Type)return Rec;
  }
 return nullptr;
}
#endif
public:
//------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------
static _finline size_t GetArgC(void){return (size_t)SInfo.CLArgs[-1];}   // On Windows should be always 1? // Be careful with access to SInfo using casts. Clang may optimize out ALL! code because of it
static _finline const char** GetArgV(void){return (const char**)SInfo.CLArgs;}
static _finline const char** GetEnvP(void){return (const char**)SInfo.EVVars;}
//------------------------------------------------------------------------------------------------------------
static _finline PX::fdsc_t GetStdIn(void)  {return PX::STDIN; }  // 0
static _finline PX::fdsc_t GetStdOut(void) {return PX::STDOUT;}  // 1
static _finline PX::fdsc_t GetStdErr(void) {return PX::STDERR;}  // 2
//------------------------------------------------------------------------------------------------------------

static _finline bool IsLoadedByLdr(void) {return SInfo.HaveLoader;}    // OnWindows, Any DLL that loaded by loader
static _finline bool IsDynamicLib(void) {return false;}
//------------------------------------------------------------------------------------------------------------
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
template<typename T> static sint GetCmdLineParam(sint AOffs, T DstBuf, uint* BufCCnt=nullptr)  //, unsigned int ParLen)  // Return size of Param string? // May overflow without 'ParLen'
{
 if(AOffs >= (sint)GetArgC())return -2;
 if(AOffs < 0)return -3;
 const achar* CurArg = GetArgV()[AOffs];
 uint  ArgLen = 0;
 if(!DstBuf)
  {
   while(CurArg[ArgLen])ArgLen++;
   ArgLen++;  // Space for terminating 0
   if(BufCCnt)*BufCCnt = ArgLen;
   return ArgLen;   // Returns arg size, not offset of a next arg
  }
 uint MaxLen = (BufCCnt)?(*BufCCnt - 1):(-1);  // -1 is MaxUINT
 for(;CurArg[ArgLen] && (ArgLen < MaxLen);ArgLen++)DstBuf[ArgLen] = CurArg[ArgLen];
 DstBuf[ArgLen] = 0;
 if(BufCCnt)*BufCCnt = ArgLen;
 if(CurArg[ArgLen])return -1;  // Buffer too small
 return ++AOffs;
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
 //GetAuxVRec(size_t Type)
 return -1;
}
//------------------------------------------------------------------------------------------------------------
//
static _finline void* GetModuleBase(void)
{
 return SInfo.TheModBase;
}
//------------------------------------------------------------------------------------------------------------
//
static _finline size_t GetModuleSize(void)
{
 return SInfo.TheModSize;
}
//------------------------------------------------------------------------------------------------------------
// Returns full path to current module and its name in UTF8
static size_t _finline GetModulePath(char* DstBuf, size_t BufSize)
{
 return 0;
}
//------------------------------------------------------------------------------------------------------------
// MacOS 'dyld main' args: ACnt, Args, EVars, AVars
// Without the -osx_min_version flag to ld, arguments are indeed passed on stack to _start ????????????????
//
static sint InitStartupInfo(void* StkFrame=nullptr, void* ArgA=nullptr, void* ArgB=nullptr, void* ArgC=nullptr)
{
 DBGDBG("StkFrame=%p, ArgA=%p, ArgB=%p, ArgC=%p",StkFrame,ArgA,ArgB,ArgC);    // On Windows syscalls are not initialized yet at this point!
 IFDBG{ DBGDBG("Stk[0]=%p, Stk[1]=%p, Stk[2]=%p, Stk[3]=%p, Stk[4]=%p", ((void**)StkFrame)[0], ((void**)StkFrame)[1], ((void**)StkFrame)[2], ((void**)StkFrame)[3], ((void**)StkFrame)[4]); }

 /*if(!APtr)  // Try to get AUXV from '/proc/self/auxv'
  {
   return -1;
  }
   // It may not be known if we are in a shared library
*/
 if constexpr(IsSysWindows)
  {


  }
 else if constexpr(IsSysMacOS)
  {
   if(StkFrame)
    {
     // ArgA and ArgB may be ArgC and ArgV from main if the exe is built with LC_MAIN and dyld
     if(((size_t*)StkFrame)[0] > 0xFFF)            // Return to dyld or argc   //   ArgB && ArgA && ((size_t)ArgA < 256) && ((size_t)ArgB > (size_t)StkFrame) && (((size_t)ArgB - (size_t)StkFrame) < 1024))
      {
       SInfo.HaveLoader = true;
       StkFrame = &((size_t*)ArgB)[-1];  // Make it point to 'argc' again  // Real stack frame as passed from the kernel
       SInfo.TheModBase = ((void**)StkFrame)[-1];  // Not for DLL, try something else
      }
       else SInfo.TheModBase = nullptr;  // Probably can get it from ArgA or ArgB, should be add somwhere in this module`s header
    }
     else return -1;  // For now, no DLL support
  }
 else if constexpr(IsSysLinux)
  {
   if(StkFrame)
    {
//   TODO: detect if this is a DLL loaded by loader, or if the EXE started from loader
     if constexpr(IsCpuARM)StkFrame = &((void**)StkFrame)[2]; // ARM: PUSH {R11,LR} - 0: RetAddr, 1:StackFramePtr   // __builtin_frame_address does not return the frame address as it was at the function entry
      else StkFrame = &((void**)StkFrame)[1];  // No ret addr on stack, only stack frame ptr
    }
  }

 if constexpr(!IsSysWindows)
  {
   if(StkFrame)
    {
     SInfo.STInfo = StkFrame;
     uint ArgNum  = ((size_t*)StkFrame)[0];
     SInfo.CLArgs = (char**)&((void**)StkFrame)[1];
     SInfo.EVVars = &SInfo.CLArgs[ArgNum+1];
     void*  APtr = nullptr;
     char** Args = SInfo.EVVars;
     uint ParIdx = 0;
     do{APtr=Args[ParIdx++];}while(APtr);
     SInfo.AuxInf = &Args[ParIdx];
    }
  }
 DBGDBG("STInfo=%p, CLArgs=%p, EVVars=%p, AuxInf=%p",SInfo.STInfo,SInfo.CLArgs,SInfo.EVVars,SInfo.AuxInf);
 return 0;
}
//============================================================================================================
static void DbgLogStartupInfo(void)
{
 if(!SInfo.STInfo)return;
 // Log command line arguments
 if(SInfo.CLArgs)
  {
   void*  APtr = nullptr;
   char** Args = SInfo.CLArgs;
   uint ParIdx = 0;
   LOGDBG("CArguments: ");
   for(uint idx=0;(APtr=Args[ParIdx++]);idx++)
    {
     LOGDBG("  Arg %u: %s",idx,APtr);
    }
  }
 // Log environment variables
 if(SInfo.EVVars)
  {
   void*  APtr = nullptr;
   char** Args = SInfo.EVVars;
   uint ParIdx = 0;
   LOGDBG("EVariables: ");
   while((APtr=Args[ParIdx++]))
    {
//LOGDBG("  VA: %p",APtr);
     LOGDBG("  Var: %s",APtr);
    }
  }
 // Log auxilary vector
 if(SInfo.AuxInf)
  {
   LOGDBG("AVariables : ");
   if constexpr(IsSysLinux)
    {
     for(ELF::SAuxVecRec* Rec=(ELF::SAuxVecRec*)SInfo.AuxInf;Rec->type != ELF::AT_NULL;Rec++)
      {
       LOGDBG("  Aux: Type=%.3u, Value=%p",Rec->type, (void*)Rec->val);
      }
    }
   else if constexpr(IsSysMacOS)
    {
     void*  APtr = nullptr;
     char** Args = SInfo.AuxInf;
     uint ParIdx = 0;
     while((APtr=Args[ParIdx++]))
      {
       LOGDBG("  AVar: %s",APtr);
      }
    }
  }
 DBGDBG("Done!");
}
//------------------------------------------------------------------------------------------------------------


/* ====================== LINUX/BSD ======================
Stack layout from startup code:
  argc
  argv pointers
  NULL that ends argv[]
  environment pointers
  NULL that ends envp[]
  ELF Auxiliary Table
  argv strings
  environment strings
  program name
  NULL
*/

// =========================== MACH-O system entry frame =======================
/*
 * C runtime startup for interface to the dynamic linker.
 * This is the same as the entry point in crt0.o with the addition of the
 * address of the mach header passed as an extra first argument.
 *
 * Kernel sets up stack frame to look like:
 *
 *  | STRING AREA | 'executable_path=' + All strings from fields below
 *  +-------------+
 *  |      0      |
*   +-------------+
 *  |  apple[n]   |
 *  +-------------+
 *         :
 *  +-------------+
 *  |  apple[0]   |  // Name=value or nulls
 *  +-------------+
 *  |      0      |
 *  +-------------+
 *  |    env[n]   |
 *  +-------------+
 *         :
 *         :
 *  +-------------+
 *  |    env[0]   |  // Name=value
 *  +-------------+
 *  |      0      |
 *  +-------------+
 *  | arg[argc-1] |
 *  +-------------+
 *         :
 *         :
 *  +-------------+
 *  |    arg[0]   |   // Value (split command line)
 *  +-------------+
 *  |     argc    |   // on x86-64 RSP  points directly to 'argc', not to some return address
 *  +-------------+
 *. |      mh     | <--- [NOT TRUE?]sp, address of where the a.out's file offset 0 is in memory (MACH-O header)
 *  +-------------+
 *
 *  Where arg[i] and env[i] point into the STRING AREA
 */

// NOTE: Looks like it is impossible to create a valid MACH-O executable(not dylib) which does not references dyld and receives control directly from Kernel

/*
Apple`s loader loads and run all initial functions from dynamically linked libraries. Due to this, we can create functions that run before the main binary is started.


Executable 0x2 (mh_execute/mh_executable) - Is not linked. Is used to create a launchable program - Application, App extension - Widget. Application target is a default setting
Bundle 0x8 (mh_bundle .bundle) - loadable bundle - run time linked. iOS now supports only Testing Bundle target where it is a default setting to generate a Loadable bundle.
System -> Testing Bundle -> tested binary. A location of Testing Bundle will be depended on target, static or dynamic binary...
Dynamic Library 0x6 (mh_dylib .dylib or none) - Load/run time linked.
With Framework target - Dynamic Library is a default setting to generate a Dynamic framework
Static Library(staticlib .a) - Compilation time(build time) linked.
With Static Library target - Static Library is a default setting to generate a Static library
With Framework target - Static Library to generate a Static framework
Relocatable Object File 0x1 (mh_object .o) - Compilation time(build time) linked. It is the simplest form. It is a basement to create executable, static or dynamic format. Relocatable because variables and functions don't have any specific address


 =========================== ELF system AUXV =======================
// http://articles.manugarg.com/aboutelfauxiliaryvectors.html

position            content                     size (bytes) + comment
  ------------------------------------------------------------------------
  stack pointer ->  [ argc = number of args ]     4
                    [ argv[0] (pointer) ]         4   (program name)
                    [ argv[1] (pointer) ]         4
                    [ argv[..] (pointer) ]        4 * x
                    [ argv[n - 1] (pointer) ]     4
                    [ argv[n] (pointer) ]         4   (= NULL)

                    [ envp[0] (pointer) ]         4
                    [ envp[1] (pointer) ]         4
                    [ envp[..] (pointer) ]        4
                    [ envp[term] (pointer) ]      4   (= NULL)

                    [ auxv[0] (Elf32_auxv_t) ]    8
                    [ auxv[1] (Elf32_auxv_t) ]    8
                    [ auxv[..] (Elf32_auxv_t) ]   8
                    [ auxv[term] (Elf32_auxv_t) ] 8   (= AT_NULL vector)

                    [ padding ]                   0 - 16

                    [ argument ASCIIZ strings ]   >= 0
                    [ environment ASCIIZ str. ]   >= 0

  (0xbffffffc)      [ end marker ]                4   (= NULL)

  (0xc0000000)      < bottom of stack >           0   (virtual)


//============================================================================================
X64: At start:
Stack is aligned to 8
rsp[0]: Ret addr to 'dyld'


@loader_path

ld:
     -execute    The default. Produce a mach-o main executable that has file
                 type MH_EXECUTE.

     -dylib      Produce a mach-o shared library that has file type MH_DYLIB.

     -bundle     Produce a mach-o bundle that has file type MH_BUNDLE.

     -dynamic    The default. Implied by -dylib, -bundle, or -execute

     -static     Produces a mach-o file that does not use the dyld. Only used
                 building the kernel.
*/


/*
Path	Base	Size
FrameworkTest.dylib	0000000100000000	0000000000003648
/usr/lib/dyld	0000000100004000	00000000000B8000
/usr/lib/system/libsystem_blocks.dylib	00007FF80B03B000	0000000000002000
/usr/lib/system/libxpc.dylib	00007FF80B03D000	000000000003C000
/usr/lib/system/libsystem_trace.dylib	00007FF80B079000	0000000000019000
/usr/lib/system/libcorecrypto.dylib	00007FF80B092000	0000000000092000
/usr/lib/system/libsystem_malloc.dylib	00007FF80B124000	000000000002C000
/usr/lib/system/libdispatch.dylib	00007FF80B150000	0000000000047000
/usr/lib/libobjc.A.dylib	00007FF80B197000	000000000003A000
/usr/lib/system/libsystem_featureflags.dylib	00007FF80B1D1000	0000000000003000
/usr/lib/system/libsystem_c.dylib	00007FF80B1D4000	0000000000089000
/usr/lib/libc++.1.dylib	00007FF80B25D000	0000000000059000
/usr/lib/libc++abi.dylib	00007FF80B2B6000	0000000000016000
/usr/lib/system/libsystem_kernel.dylib	00007FF80B2CC000	0000000000038000
/usr/lib/system/libsystem_pthread.dylib	00007FF80B304000	000000000000C000
/usr/lib/system/libdyld.dylib	00007FF80B310000	000000000000C000
/usr/lib/system/libsystem_platform.dylib	00007FF80B31C000	000000000000A000
/usr/lib/system/libsystem_info.dylib	00007FF80B326000	000000000002B000
/usr/lib/system/libsystem_darwin.dylib	00007FF80D946000	000000000000A000
/usr/lib/system/libsystem_notify.dylib	00007FF80DD6C000	000000000000F000
/usr/lib/system/libsystem_networkextension.dylib	00007FF810240000	0000000000017000
/usr/lib/system/libsystem_asl.dylib	00007FF8102A5000	0000000000017000
/usr/lib/system/libsystem_symptoms.dylib	00007FF811AF3000	0000000000008000
/usr/lib/system/libsystem_containermanager.dylib	00007FF813C77000	000000000001D000
/usr/lib/system/libsystem_configuration.dylib	00007FF8149CE000	0000000000004000
/usr/lib/system/libsystem_sandbox.dylib	00007FF8149D2000	0000000000006000
/usr/lib/system/libquarantine.dylib	00007FF81571A000	0000000000003000
/usr/lib/system/libsystem_coreservices.dylib	00007FF815DD1000	0000000000005000
/usr/lib/system/libsystem_m.dylib	00007FF816037000	0000000000061000
/usr/lib/system/libmacho.dylib	00007FF816099000	0000000000006000
/usr/lib/system/libcommonCrypto.dylib	00007FF8160BB000	000000000000C000
/usr/lib/system/libunwind.dylib	00007FF8160C7000	000000000000B000
/usr/lib/liboah.dylib	00007FF8160D2000	0000000000008000
/usr/lib/system/libcopyfile.dylib	00007FF8160DA000	000000000000A000
/usr/lib/system/libcompiler_rt.dylib	00007FF8160E4000	0000000000008000
/usr/lib/system/libsystem_collections.dylib	00007FF8160EC000	0000000000005000
/usr/lib/system/libsystem_secinit.dylib	00007FF8160F1000	0000000000003000
/usr/lib/system/libremovefile.dylib	00007FF8160F4000	0000000000002000
/usr/lib/system/libkeymgr.dylib	00007FF8160F6000	0000000000001000
/usr/lib/system/libsystem_dnssd.dylib	00007FF8160F7000	0000000000008000
/usr/lib/system/libcache.dylib	00007FF8160FF000	0000000000005000
/usr/lib/libSystem.B.dylib	00007FF816104000	0000000000002000
/usr/lib/system/libsystem_product_info_filter.dylib	00007FF81C374000	0000000000001000
*/

