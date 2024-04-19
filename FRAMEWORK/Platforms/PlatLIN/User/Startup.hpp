
#pragma once

//------------------------------------------------------------------------------------------------------------
private:
// TODO: Pack these fields in a structure and scramble them (OPTIONAL)
enum ESFlags {sfInitialized=0x01, sfDynamicLib=0x02, sfLoadedByLdr=0x04};
struct SSINF
{
 vptr             STInfo;  // Pointer to stack frame info, received from kernel  // PEB on windows?
 syschar**        CLArgs;  // Points to ARGV array  // Not split on windows!
 syschar**        EVVars;  // Points to EVAR array (cannot cache this pointer?)
 ELF::SAuxVecRec* AuxInf;  // Auxilary vector // LINUX/BSD: ELF::SAuxVecRec*
 vptr             NullPtr; // For any pointer that should point to NULL
 PX::fdsc_t       DevNull;
 PX::fdsc_t       DevRand;

 vptr   TheModBase;
 size_t TheModSize;
 vptr   MainModBase;
 size_t MainModSize;
 achar  SysDrive[8];
 sint32 MemPageSize;
 sint32 MemGranSize;
 sint32 UTCOffs;      // In seconds
 uint32 Flags;
 NTHD::STDesc thd;

// Exe path (Even if this module is a DLL)
// Current directory(at startup)   // Required?
// Working dir (Not have to be same as current dir)
// Temp path
} static inline fwsinf = {};

static _finline NTHD::STDesc* GetThDesc(void){return &fwsinf.thd;}
//------------------------------------------------------------------------------------------------------------
static vptr FindMainBase(size_t* Size)   // Use something else  // TODO: Probably /proc/self/maps search for /proc/self/exe content match
{
 sint ExePtr = GetAuxInfo(ELF::AT_PHDR);
 if(ExePtr < 0)ExePtr = GetAuxInfo(ELF::AT_ENTRY);
 if(ExePtr > 0)
  {
   vptr ptr = UELF::FindElfByAddr((vptr)ExePtr, Size);
   DBGDBG("Found by ELF for %p: %p",(vptr)ExePtr, ptr);
   if(ptr)return ptr;
  }
 achar* PPtr = nullptr;
 sint   PLen = 0;
 alignas(sizeof(vptr)) achar pbuf[2048];
 alignas(sizeof(vptr)) uint8 tbuf[2048];
 *pbuf = 0;
 ExePtr = GetAuxInfo(ELF::AT_EXECFN);
 DBGDBG("Exe path auxv: %p",(vptr)ExePtr);
 if(ExePtr <= 0)
  {
   PLen = NAPI::readlink("/proc/self/exe", pbuf, sizeof(pbuf)-1);
   if(PLen >= 0){pbuf[PLen] = 0; PPtr = pbuf;}
  }
  else PPtr = (achar*)ExePtr;
 DBGDBG("Exe path: %s",((ExePtr > 0)?(achar*)ExePtr:""));

 if(Size)*Size = 0;
 if(!PPtr || !*PPtr)return nullptr;
 PLen = AlignP2Frwd(PLen, sizeof(vptr));
 SMemMap* MappedRanges = (SMemMap*)&pbuf;
 MappedRanges->TmpBufLen = sizeof(tbuf);
 MappedRanges->TmpBufPtr = tbuf;     // For reading from the system
 if(NPFS::FindMappedRangesByPath(-1, 0, PPtr, MappedRanges, sizeof(pbuf)) <= 0)return nullptr;
 if(!MappedRanges->RangesCnt)return nullptr;
 vptr   addr = (vptr)MappedRanges->Ranges[0].RangeBeg;
 size_t mlen = ELF::GetModuleSizeInMem(addr);   // Validate and get full size, including BSS
 if(!mlen)return nullptr;
 if(Size)*Size = mlen;
 DBGDBG("Found by mappings: %p",addr);
 return addr;
}
//------------------------------------------------------------------------------------------------------------
static _ninline vptr FindModuleBase(size_t* Size)   // Use a pointer inside
{
 return UELF::FindElfByAddr(&fwsinf, Size);
}
//------------------------------------------------------------------------------------------------------------
public:      // Do not hide platform dependant stuff!
//------------------------------------------------------------------------------------------------------------
static _finline size_t GetArgC(void){return (size_t)fwsinf.CLArgs[-1];}   // On Windows should be always 1? // Be careful with access to SInfo using casts. Clang may optimize out ALL! code because of it
static _finline const syschar** GetArgV(void){return (const syschar**)fwsinf.CLArgs;}
static _finline const syschar** GetEnvP(void){return (const syschar**)fwsinf.EVVars;}
//------------------------------------------------------------------------------------------------------------
static _finline uint32 GetPageSize(void)  {return fwsinf.MemPageSize;}
static _finline uint32 GetGranSize(void)  {return fwsinf.MemGranSize;}
//------------------------------------------------------------------------------------------------------------
static _finline PX::fdsc_t GetStdIn(void)  {return PX::STDIN; }  // 0
static _finline PX::fdsc_t GetStdOut(void) {return PX::STDOUT;}  // 1
static _finline PX::fdsc_t GetStdErr(void) {return PX::STDERR;}  // 2

static _finline PX::fdsc_t GetStdNull(void) {return fwsinf.DevNull;}
static _finline PX::fdsc_t GetStdRand(void) {return fwsinf.DevRand;}
//------------------------------------------------------------------------------------------------------------
static _finline sint32 GetTZOffsUTC(void)  {return fwsinf.UTCOffs;}   // In seconds   // TODO: Reread optionally?
static _finline bool   IsInitialized(void) {return fwsinf.Flags & sfInitialized;}
static _finline bool   IsLoadedByLdr(void) {return fwsinf.Flags & sfLoadedByLdr;}     // Loading and unloading is managed by a loader (No need to call Exit(group) from the entry point)
static _finline bool   IsDynamicLib(void)  {return fwsinf.Flags & sfDynamicLib;}      // Not a main executable
//------------------------------------------------------------------------------------------------------------
static _finline vptr   GetMainBase(void) {return fwsinf.MainModBase;}
static _finline size_t GetMainSize(void) {return fwsinf.MainModSize;}
static _finline vptr   GetModuleBase(void) {return fwsinf.TheModBase;}
static _finline size_t GetModuleSize(void) {return fwsinf.TheModSize;}
//------------------------------------------------------------------------------------------------------------
static sint GetMainPath(achar* DstBuf, size_t BufSize=uint(-1))  // NOTE: If returned length is BufSize-1 then the buffer is probably too small
{
 sint len    = 0;
 sint aoffs  = 0;
 sint ExePtr = GetAuxInfo(ELF::AT_EXECFN);
 if(ExePtr > 0)len = NSTR::StrCopy(DstBuf, (achar*)ExePtr, BufSize);
 if(len <= 0)len = NAPI::readlink("/proc/self/exe", DstBuf, BufSize-1);   // TODO: Embed the string in the code
 if(len <= 0)len = GetCLArg(aoffs, DstBuf, BufSize);   // First ard is USUALLY the exe path (But does not have to - the shell passes it)
 if(len <= 0)
  {
   SMemRange Range;
   Range.FPath    = DstBuf;
   Range.FPathLen = BufSize;
   len = NPFS::FindMappedRangeByAddr(-1, (size_t)fwsinf.MainModBase, &Range);  // MainModBase MUST be already found!
  }
 if(len <= 0)return 0;
 DstBuf[len] = 0;  // readlink won`t do that
 return len;
}
//------------------------------------------------------------------------------------------------------------
// Returns full path to current module and its name in UTF8
static sint _finline GetModulePath(achar* DstBuf, size_t BufSize=uint(-1))   // NOTE: Plugins should be mapped for this to work
{
 if(!IsDynamicLib())return GetMainPath(DstBuf, BufSize);   // Will be faster
 SMemRange Range;
 Range.FPath    = DstBuf;
 Range.FPathLen = BufSize;
 sint len = NPFS::FindMappedRangeByAddr(-1, (size_t)fwsinf.TheModBase, &Range);
 if(len <= 0)return 0;
 DstBuf[len] = 0;
 return len;
}
//------------------------------------------------------------------------------------------------------------
static sint InitStartupInfo(vptr StkFrame=nullptr, vptr ArgA=nullptr, vptr ArgB=nullptr, vptr ArgC=nullptr)  // Probably should be private but...
{
 DBGDBG("StkFrame=%p, ArgA=%p, ArgB=%p, ArgC=%p",StkFrame,ArgA,ArgB,ArgC);
 IFDBG{ DBGDBG("Stk[0]=%p, Stk[1]=%p, Stk[2]=%p, Stk[3]=%p, Stk[4]=%p", ((void**)StkFrame)[0], ((void**)StkFrame)[1], ((void**)StkFrame)[2], ((void**)StkFrame)[3], ((void**)StkFrame)[4]); }

// NOTE: LibC only (MUSL does not pass ArgC, ArcV, EnvP to Init proc)
 if(ArgB && ArgC && (((size_t*)ArgC) == (((size_t*)ArgB)+((size_t)ArgA)+1)) && (((size_t*)ArgB)[-1] == (size_t)ArgA))   // System entry point does not receive ArgC,ArgV,EnvP but LD passes those to _init
  {
   fwsinf.Flags |= sfLoadedByLdr;
   StkFrame = ((size_t*)ArgB)-1;  // Start from ArgC  // NOTE: Lets hope that it is not a copy and actually the original ArgC:ArgV:Env:Aux arrays
   DBGDBG("Loader detected(1)");
  }
 else if(StkFrame)
  {
  // vptr optr = StkFrame;
   size_t* Frame = (size_t*)UELF::FindStartupInfo(StkFrame);
   if(Frame && !Frame[0] && !Frame[1] && !Frame[2] && !Frame[3])
    {
     Frame = (size_t*)UELF::FindStartupInfoByAuxV(Frame);  // ArgC,ArgV,EnvP,AuxV are null - probbly incorrectly detected because of 4 null entries on stack
     if(Frame)
      {
       fwsinf.Flags |= sfLoadedByLdr;    // Too far and NULLs on stakd are put by a loader most likely
       DBGDBG("Loader detected(2)");
      }
    }
   StkFrame = Frame;
   DBGDBG("FoundInfoPtr: %p", StkFrame);
  // for(uint8* Ptr = (uint8*)optr;;Ptr += 8){LOGMSG("Stk %p: %p",Ptr,*(vptr*)Ptr);}  // <<< DBG
  }

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
 DBGDBG("STInfo=%p, CLArgs=%p, EVVars=%p, AuxInf=%p",fwsinf.STInfo,fwsinf.CLArgs,fwsinf.EVVars,fwsinf.AuxInf);
 sint PageSize = GetAuxInfo(ELF::AT_PAGESZ);
 if(PageSize > 0)fwsinf.MemPageSize = fwsinf.MemGranSize = PageSize;
   else fwsinf.MemPageSize = fwsinf.MemGranSize = MEMPAGESIZE;
 fwsinf.Flags  |= sfInitialized;

 fwsinf.DevNull = OpenDevNull();   // Needed for a module base search!
 fwsinf.DevRand = OpenDevRand();

 fwsinf.MainModBase = FindMainBase(&fwsinf.MainModSize);
 fwsinf.TheModBase  = FindModuleBase(&fwsinf.TheModSize);
 DBGDBG("MainMod={%p, %p}, ThisMod={%p, %p}",fwsinf.MainModBase,(vptr)fwsinf.MainModSize,  fwsinf.TheModBase,(vptr)fwsinf.TheModSize);
 bool KnownBases = fwsinf.MainModBase && fwsinf.TheModBase;
 if(KnownBases && (fwsinf.MainModBase != fwsinf.TheModBase))
  {
   fwsinf.Flags |= sfDynamicLib;
   DBGDBG("This is a dynamic lib");
  }
 if(!(fwsinf.Flags & sfLoadedByLdr))   // TODO: Compare module pointers
  {
   sint LdrBase = GetAuxInfo(ELF::AT_BASE);    // Our executables do not request the loader from the system
   if(LdrBase > 0)
    {
     fwsinf.Flags |= sfLoadedByLdr;
     if(!KnownBases)fwsinf.Flags |= sfDynamicLib;  // At least assume this
     DBGDBG("Loader detected(3)");
    }
  }
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



