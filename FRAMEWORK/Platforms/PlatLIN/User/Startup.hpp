
#pragma once

//------------------------------------------------------------------------------------------------------------
private:
// TODO: Pack these fields in a structure and scramble them (OPTIONAL)
struct SSINF
{
 void*            STInfo;  // Pointer to stack frame info, received from kernel  // PEB on windows?
 syschar**        CLArgs;  // Points to ARGV array  // Not split on windows!
 syschar**        EVVars;  // Points to EVAR array (cannot cache this pointer?)
 ELF::SAuxVecRec* AuxInf;  // Auxilary vector // LINUX/BSD: ELF::SAuxVecRec*
 vptr             NullPtr; // For any pointer that should point to NULL

 void*  TheModBase;
 size_t TheModSize;
 achar  SysDrive[8];
 sint32 UTCOffs;      // In seconds
 sint32 HaveLoader;   // TODO: Flags
 NTHD::SThCtx MainTh;    // Main(Init/Entry) thread // A thread from which the framework is initialized at main entry point for a module/app (For modules this is NOT the app`s process main thread)
 NTHD::SThInf* ThreadInfo;  // For additional threads (Null if only entry thread is used)

// Exe path (Even if this module is a DLL)
// Current directory(at startup)   // Required?
// Working dir (Not have to be same as current dir)
// Temp path
} static inline fwsinf = {};

public:      // Do not hide platform dependant stuff!
//------------------------------------------------------------------------------------------------------------
static _finline size_t GetArgC(void){return (size_t)fwsinf.CLArgs[-1];}   // On Windows should be always 1? // Be careful with access to SInfo using casts. Clang may optimize out ALL! code because of it
static _finline const syschar** GetArgV(void){return (const syschar**)fwsinf.CLArgs;}
static _finline const syschar** GetEnvP(void){return (const syschar**)fwsinf.EVVars;}
//------------------------------------------------------------------------------------------------------------
static ELF::SAuxVecRec* GetAuxVRec(size_t Type)
{
 for(ELF::SAuxVecRec* Rec=fwsinf.AuxInf;Rec->type != ELF::AT_NULL;Rec++)
  {
   if(Rec->type == Type)return Rec;
  }
 return nullptr;
}
//------------------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------------------
static _finline PX::fdsc_t GetStdIn(void)  {return PX::STDIN; }  // 0
static _finline PX::fdsc_t GetStdOut(void) {return PX::STDOUT;}  // 1
static _finline PX::fdsc_t GetStdErr(void) {return PX::STDERR;}  // 2
//------------------------------------------------------------------------------------------------------------
static _finline sint32 GetTZOffsUTC(void)  {return fwsinf.UTCOffs;}   // In seconds   // TODO: Reread optionally?
static _finline bool   IsLoadedByLdr(void) {return fwsinf.HaveLoader;}    // OnWindows, Any DLL that loaded by loader
static _finline bool   IsDynamicLib(void)  {return false;}
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
//
static sint GetCLArg(sint& AOffs, achar* DstBuf, uint DstCCnt=uint(-1))    // Enumerate and copy     // NOTE: Copying is inefficient!
{
 return UELF::GetStrFromArr(&AOffs, GetArgV(), DstBuf, DstCCnt);
}
//------------------------------------------------------------------------------------------------------------
static sint GetEnvVar(sint& AOffs, achar* DstBuf, uint DstCCnt=uint(-1))   // Enumerate and copy     // NOTE: Copying is inefficient!
{
 return UELF::GetStrFromArr(&AOffs, GetEnvP(), DstBuf, DstCCnt);
}
//------------------------------------------------------------------------------------------------------------
static _ninline sint GetEnvVar(const achar* Name, achar* DstBuf, uint DstCCnt=uint(-1))   // Find by name and copy   // NOTE: Copying is inefficient!  // NOTE: Broken on ARM64 if inlined! Why?
{
 const  achar** vars = GetEnvP();
 bool Unnamed = !Name || !*Name;
 for(;const achar* evar = *vars;vars++)
  {
   sint spos = NSTR::ChrOffset(evar, '=');
   if(spos < 0)continue;  // No separator!
   if((!spos && Unnamed) || NSTR::IsStrEqualCS(Name, evar, (uint)spos))return (sint)NSTR::StrCopy(DstBuf, &evar[spos+1], DstCCnt);
  }
 return -1;
}
//------------------------------------------------------------------------------------------------------------
static const syschar* GetCLArg(sint& AOffs, uint* Size=nullptr)       // Enumerate     // NOTE: Not null-terminated on Windows
{
 if(AOffs < 0)return nullptr;    // Already finished
 const achar** vars = GetArgV();
 const syschar* val = vars[AOffs++];
 if(!val){AOffs = -1; return nullptr;}  // End of list reached
 if(Size)*Size = NSTR::StrLen(val);
 return val;
}
//------------------------------------------------------------------------------------------------------------
static const syschar* GetEnvVar(sint& AOffs, uint* Size=nullptr)      // Enumerate
{
 if(AOffs < 0)return nullptr;    // Already finished
 const achar** vars = GetEnvP();
 const syschar* val = vars[AOffs++];
 if(!val){AOffs = -1; return nullptr;}  // End of list reached
 if(Size)*Size = NSTR::StrLen(val);
 return val;
}
//------------------------------------------------------------------------------------------------------------
static const syschar* GetEnvVar(const achar* Name, uint* Size=nullptr)          // Find by name
{
 const achar** vars = GetEnvP();
 bool Unnamed = !Name || !*Name;
 for(;const achar* evar = *vars;vars++)
  {
   sint spos = NSTR::ChrOffset(evar, '=');
   if(spos < 0)continue;  // No separator!
   if((!spos && Unnamed) || NSTR::IsStrEqualCS(Name, evar, (uint)spos))
    {
     if(Size)*Size = NSTR::StrLen(&evar[spos+1]);
     return &evar[spos+1];
    }
  }
 return nullptr;
}
//------------------------------------------------------------------------------------------------------------
// AppleInfo on MacOS
static sint GetAuxInfo(uint InfoID, vptr DstBuf, size_t BufSize)
{
 //GetAuxVRec(size_t Type)
 return -1;
}
//------------------------------------------------------------------------------------------------------------
//
static _finline vptr GetModuleBase(void)
{
 return fwsinf.TheModBase;
}
//------------------------------------------------------------------------------------------------------------
static _finline size_t GetModuleSize(void)
{
 return fwsinf.TheModSize;
}
//------------------------------------------------------------------------------------------------------------
// Returns full path to current module and its name in UTF8
static sint _finline GetModulePath(achar* DstBuf, size_t BufSize=uint(-1))
{
 sint aoffs = 0;
 return GetCLArg(aoffs, DstBuf, BufSize);       // Will work for now
}
//------------------------------------------------------------------------------------------------------------
static NTHD::SThCtx* GetThreadSelf(void)     // Probably can find it faster by scanning stack pages forward and testing for SThCtx at beginning
{
 return GetThreadByAddr(GETSTKFRAME());
}
//------------------------------------------------------------------------------------------------------------
static NTHD::SThCtx* GetThreadByID(uint id)
{
 if((id == (uint)-1)||(id == fwsinf.MainTh.ThreadID))return &fwsinf.MainTh;
 if(!fwsinf.ThreadInfo)return nullptr; // No more threads
 NTHD::SThCtx** ptr = fwsinf.ThreadInfo->FindThByTID(id);
 if(!ptr)return nullptr;
 return NTHD::ReadRecPtr(ptr);
}
//------------------------------------------------------------------------------------------------------------
static NTHD::SThCtx* GetThreadByAddr(vptr addr)   // By an address on stack
{
 if(((uint8*)addr >= (uint8*)fwsinf.MainTh.StkBase)&&((uint8*)addr < ((uint8*)fwsinf.MainTh.StkBase + fwsinf.MainTh.StkSize)))return &fwsinf.MainTh;
 if(!fwsinf.ThreadInfo)return nullptr; // No more threads
 NTHD::SThCtx** ptr = fwsinf.ThreadInfo->FindThByStack(addr);
 if(!ptr)return nullptr;
 return NTHD::ReadRecPtr(ptr);
}
//------------------------------------------------------------------------------------------------------------
/*static NTHD::SThCtx* GetNextThread(NTHD::SThCtx* th) // Get thread by index?   // Start from NULL    // Need to think
{
 return nullptr;
}*/
//------------------------------------------------------------------------------------------------------------
static sint InitStartupInfo(void* StkFrame=nullptr, void* ArgA=nullptr, void* ArgB=nullptr, void* ArgC=nullptr)  // Probably should be private but...
{
 DBGDBG("StkFrame=%p, ArgA=%p, ArgB=%p, ArgC=%p",StkFrame,ArgA,ArgB,ArgC);
 IFDBG{ DBGDBG("Stk[0]=%p, Stk[1]=%p, Stk[2]=%p, Stk[3]=%p, Stk[4]=%p", ((void**)StkFrame)[0], ((void**)StkFrame)[1], ((void**)StkFrame)[2], ((void**)StkFrame)[3], ((void**)StkFrame)[4]); }

  // LOGMSG("StkFrame=%p, ArgA=%p, ArgB=%p, ArgC=%p",StkFrame,ArgA,ArgB,ArgC);
 //  LOGMSG("Stk[0]=%p, Stk[1]=%p, Stk[2]=%p, Stk[3]=%p, Stk[4]=%p", ((void**)StkFrame)[0], ((void**)StkFrame)[1], ((void**)StkFrame)[2], ((void**)StkFrame)[3], ((void**)StkFrame)[4]);
 /*if(!APtr)  // Try to get AUXV from '/proc/self/auxv'
  {
   return -1;
  }
   // It may not be known if we are in a shared library
*/
 /*if(StkFrame) // NOTE: C++ Clang enforces the stack frame to contain some EntryPoints`s saved registers
  {
//   TODO: detect if this is a DLL loaded by loader, or if the EXE started from loader
   if constexpr (IsArchX64)
    {
     if constexpr(IsCpuARM)StkFrame = &((void**)StkFrame)[4]; // ARM 64: Stores X19,X29,X30 and aligns to 16   // __builtin_frame_address does not return the frame address as it was at the function entry
       else StkFrame = &((void**)StkFrame)[1];  // Untested  // No ret addr on stack, only stack frame ptr

    }
     else
      {
       if constexpr(IsCpuARM)StkFrame = &((void**)StkFrame)[2]; // ARM 32: PUSH {R11,LR} - 0: RetAddr, 1:StackFramePtr   // __builtin_frame_address does not return the frame address as it was at the function entry
         else StkFrame = &((void**)StkFrame)[1];  // No ret addr on stack, only stack frame ptr
      }
  }*/

 if(StkFrame){StkFrame = UELF::FindStartupInfo(StkFrame); DBGDBG("FoundInfoPtr: %p", StkFrame);}
 if(StkFrame)
  {
 //  LOGMSG("StkFrame %p:\r\n%#*.32D",StkFrame,128,StkFrame);
   fwsinf.STInfo = StkFrame;
   uint ArgNum   = ((size_t*)StkFrame)[0];           // Number of command line arguments
   fwsinf.CLArgs = (char**)&((void**)StkFrame)[1];   // Array of cammond line arguments
   fwsinf.EVVars = &fwsinf.CLArgs[ArgNum+1];         // Array of environment variables
   void*  APtr   = nullptr;
   char** Args   = fwsinf.EVVars;
   uint ParIdx   = 0;
   do{APtr=Args[ParIdx++];}while(APtr);  // Skip until AUX vector
   fwsinf.AuxInf = (ELF::SAuxVecRec*)&Args[ParIdx];

   /* if(ArgNum > 3)
     {
      ArgNum = 3;
      DBGDBG("Corrupted frame!!!");
     }  */

  }
 DBGDBG("Getting UTC offset...");
 //PX::timezone tz = {};
 //if(!NAPI::gettimeofday(nullptr, &tz))fwsinf.UTCOffs = tz.minuteswest;
 PX::timeval tv = {};
 if(int r = NAPI::gettimeofday(&tv, nullptr); r >= 0)
  {
   if(int t = UpdateTZOffsUTC(tv.sec);t < 0){DBGDBG("UpdateTZOffsUTC failed with %i", t);}    // Log any errors?
  }
   else {DBGDBG("gettimeofday failed with %i", r);}
 DBGDBG("UTC offset is %i seconds", fwsinf.UTCOffs); //LOGMSG("TZFILE offs: %i",tz.minuteswest);

 //  DbgLogStartupInfo();
 DBGDBG("STInfo=%p, CLArgs=%p, EVVars=%p, AuxInf=%p",fwsinf.STInfo,fwsinf.CLArgs,fwsinf.EVVars,fwsinf.AuxInf);
 return 0;
}
//============================================================================================================
static void DbgLogStartupInfo(void)
{
    //  return;
 if(!fwsinf.STInfo)return;
 // Log command line arguments
 if(fwsinf.CLArgs)
  {
   void*   APtr = nullptr;
   achar** Args = fwsinf.CLArgs;
   uint  ParIdx = 0;
   LOGDBG("CArguments: ");
   for(uint idx=0;(APtr=Args[ParIdx++]);idx++)
    {
     LOGDBG("  Arg %u: %s",idx,APtr);
    }
  }
 // Log environment variables
 if(fwsinf.EVVars)
  {
   void*   APtr = nullptr;
   achar** Args = fwsinf.EVVars;
   uint  ParIdx = 0;
   LOGDBG("EVariables: ");
   while((APtr=Args[ParIdx++]))
    {
//LOGDBG("  VA: %p",APtr);
     LOGDBG("  Var: %s",APtr);
    }
  }
 // Log auxilary vector
 if(fwsinf.AuxInf)
  {
   LOGDBG("AVariables: ");
   for(ELF::SAuxVecRec* Rec=(ELF::SAuxVecRec*)fwsinf.AuxInf;Rec->type != ELF::AT_NULL;Rec++)
    {
     LOGDBG("  Aux: Type=%.3u, Value=%p",Rec->type, (void*)Rec->val);
    }
  }
 DBGDBG("Done!");
}
//------------------------------------------------------------------------------------------------------------
/*
ABI specific?

ArmX64:
 EntryPoint:
  +00 NewSp
  +08
  +10 X29     GETSTKFRAME()
  +18 X30
  +20 X19
  +28
  +30 OrigSP  ArgNum (Stack before Entry)
*/

/* ====================== LINUX/BSD ======================
// NOTE: This is what a debugger shows
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
  NULL      // End of allocated stack area. Stack pointer starts from here
*/

/*
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

  (0xbffffffc)      [ end marker ]                4   (= NULL)      // Not present

  (0xc0000000)      < bottom of stack >           0   (virtual)     // Not present


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



