
#pragma once

//============================================================================================================
// All "members" are placed sequentially in memory but their orded may change
// It MUST be Namespace, not Struct to keep these in code segment
namespace NAPI // POSIX API implementation  // https://docs.oracle.com/cd/E19048-01/chorus4/806-3328/6jcg1bm05/index.html
{
_codesec  static SFuncStub<decltype(PX::exit)> exit;
_codesec  static SFuncStub<decltype(PX::exit_group)> exit_group;

_codesec  static SFuncStub<decltype(PX::open)> open;
_codesec  static SFuncStub<decltype(PX::close)> close;
_codesec  static SFuncStub<decltype(PX::creat)> creat;

_codesec  static SFuncStub<decltype(PX::read)> read;
_codesec  static SFuncStub<decltype(PX::readv)> readv;

_codesec  static SFuncStub<decltype(PX::write)> write;
_codesec  static SFuncStub<decltype(PX::writev)> writev;

_codesec  static SFuncStub<decltype(PX::lseek)> lseek;
_codesec  static SFuncStub<decltype(PX::mkdir)> mkdir;
_codesec  static SFuncStub<decltype(PX::rmdir)> rmdir;
_codesec  static SFuncStub<decltype(PX::unlink)> unlink;
_codesec  static SFuncStub<decltype(PX::rename)> rename;
_codesec  static SFuncStub<decltype(PX::readlink)> readlink;

_codesec  static SFuncStub<decltype(PX::mprotect)> mprotect;
#ifdef _ARCH_X32
_codesec  static SFuncStub<decltype(PX::mmap)> internal_mmap;         // Superseded by mmap2 and should not be used directly
_codesec  static SFuncStub<decltype(PX::mmap2)> mmap2;
static void* PXCALL mmap(void* addr, size_t length, int prot, int flags, int fd, size_t offset){return mmap2(addr, length, prot, flags, fd, offset / 4096); } // MMAP2_PAGE_UNIT   // On ia64, the unit for offset is actually the system page size
#else
_codesec  static SFuncStub<decltype(PX::mmap)> mmap;
#endif
_codesec  static SFuncStub<decltype(PX::munmap)> munmap;
_codesec  static SFuncStub<decltype(PX::madvise)> madvise;

_codesec  static SFuncStub<decltype(PX::stat)> stat;
_codesec  static SFuncStub<decltype(PX::fstat)> fstat;
#ifdef _ARCH_X32
_codesec  static SFuncStub<decltype(NPOSIX<uint64>::stat)> stat64;    // Need to cast the NPOSIX<uint64>::SFStat to PX::SFStat
_codesec  static SFuncStub<decltype(NPOSIX<uint64>::fstat)> fstat64;
_codesec  static SFuncStub<decltype(NPOSIX<uint64>::llseek)> llseek;
#endif
_codesec  static SFuncStub<decltype(PX::access)> access;


// >>>>> MEMORY <<<<<
#include "Impl_Mem.hpp"
// >>>>> NETWORK <<<<<
//#include "Impl_Net.hpp"
// >>>>> FILE SYSTEM <<<<<
//#include "Impl_FS.hpp"
// >>>>> PROCESSES/THREADS <<<<<
//#include "Impl_PT.hpp"
};
//============================================================================================================
//private:
namespace NPRIVATE
{
static void*            STInfo = nullptr;     // TODO: Pack these fields in a structure and scramble them (OPTIONAL)
static char**           CLArgs = nullptr;
static char**           EVVars = nullptr;
static ELF::SAuxVecRec* AuxVec = nullptr;

static void*  TheModBase = nullptr;
static size_t TheModSize = 0;


//------------------------------------------------------------------------------------------------------------

static _finline int MakeTblWritable(void* StubsArr, size_t StubsLen, bool state)     // TODO: Unprotect entire '.text' section // Find ELF header
{
 decltype(PX::mprotect)* pmprotect = ((decltype(PX::mprotect)*)&syscall_mprotect_tmpl);
 StubsLen += ((size_t)StubsArr & (MEMPAGESIZE-1));          // Size don`t have to be page-aligned
 StubsArr  = (void*)((size_t)StubsArr & ~(MEMPAGESIZE-1));  // Addr must be page-aligned
 return pmprotect(StubsArr, StubsLen, PX::PROT_READ|PX::PROT_EXEC | (state?PX::PROT_WRITE:0));
}
//------------------------------------------------------------------------------------------------------------
static _finline size_t GetArgC(void){return *(size_t*)STInfo;}
static _finline char** GetArgV(void){return (char**)&((size_t*)STInfo)[1];}
//------------------------------------------------------------------------------------------------------------
static ELF::SAuxVecRec* GetAuxVRec(size_t Type)
{
 for(ELF::SAuxVecRec* Rec=AuxVec;Rec->type != ELF::AT_NULL;Rec++)
  {
   if(Rec->type == Type)return Rec;
  }
 return nullptr;
}
//------------------------------------------------------------------------------------------------------------

};
//public:

/*
argv[0] is not guaranteed to be the executable path, it just happens to be most of the time. It can be a relative path from the current
working directory, or just the filename of your program if your program is in PATH, or an absolute path to your program, or the name a
symlink that points to your program file, maybe there are even more possibilities. It's whatever you typed in the shell to run your program.
*/
// Copies a next param to user`s buffer
// A spawner process is responsible for ARGV
// Quotes are stripped by a shell and quoted args are kept intact
// NOTE: Do not expect the return value to be an argument index!
template<typename T> static sint GetCmdLineParam(int cloffs, T DstBuf, uint* BufCCnt=nullptr, unsigned short Scope='\"\"')  //, unsigned int ParLen)  // Return size of Param string? // May overflow without 'ParLen'
{
 if(cloffs >= NPRIVATE::GetArgC())return -2;
 if(cloffs < 0)return -3;
 char* CurArg = NPRIVATE::GetArgV()[cloffs];
 uint  ArgLen = 0;
 if(!DstBuf)
  {
   while(CurArg[ArgLen])ArgLen++;
   if(BufCCnt)*BufCCnt = ArgLen;
     else return ArgLen;   // Returns arg size, not offset of a next arg
   return ++cloffs;
  }
 uint  MaxLen = (BufCCnt)?(*BufCCnt - 1):(-1);  // -1 is MaxUINT
 for(;CurArg[ArgLen] && (ArgLen < MaxLen);ArgLen++)DstBuf[ArgLen] = CurArg[ArgLen];
 DstBuf[ArgLen] = 0;
 if(BufCCnt)*BufCCnt = ArgLen;
 if(CurArg[ArgLen])return -1;  // Buffer too small
 return ++cloffs;
}
//---------------------------------------------------------------------------
template<typename T> static sint GetEnvVar(T* Name, T DstBuf, size_t BufCCnt)
{
 return -1;
}
//---------------------------------------------------------------------------
static sint GetSysInfo(uint InfoID, void* DstBuf, size_t BufSize)
{
 //GetAuxVRec(size_t Type)
 return -1;
}
//---------------------------------------------------------------------------
//
static _finline void* _finline GetModuleBase(void)
{
 return NPRIVATE::TheModBase;
}
//---------------------------------------------------------------------------
//
static _finline size_t _finline GetModuleSize(void)
{
 return NPRIVATE::TheModSize;
}
//---------------------------------------------------------------------------
// Returns full path to current module and its name in UTF8
static size_t _finline GetModulePath(char* DstBuf, size_t BufSize)
{
 return 0;
}
//---------------------------------------------------------------------------
static _finline int GetMemErrorCode(void* ptr)
{
 uint err = (uint)ptr & (MEMPAGESIZE - 1);
 if(!err)return 0;
 return -err & (MEMPAGESIZE - 1);
}
//---------------------------------------------------------------------------
__attribute__((__force_align_arg_pointer__))  static void LogMsg(const char* format, ...)  // NOTE: Temporary!
{
 va_list  args;
 va_start(args,format);
 alignas(16) char buf[512];
 int MSize = NSFMT::FormatToBuffer((char*)format, buf, sizeof(buf)-1, args);
 buf[MSize++] = '\n';
 va_end(args);
 NPTFM::NAPI::write(PX::STDOUT,buf,MSize);
}
//------------------------------------------------------------------------------------------------------------


namespace NPDBG
{
/*
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
static void LogStartupInfo(void)
{
 // Log command line arguments
 void*  APtr = nullptr;
 char** Args = NPRIVATE::CLArgs;
 uint ParIdx = 0;
 LogMsg("CArguments: ");
 for(uint idx=0;(APtr=Args[ParIdx++]);idx++)
  {
   LogMsg("Arg %u: %s",idx,APtr);
  }
 // Log environment variables
 Args = NPRIVATE::EVVars;
 LogMsg("EVariables: ");
 while((APtr=Args[ParIdx++]))
  {
   LogMsg("Var: %s",APtr);
  }
 // Log auxilary vector
 for(ELF::SAuxVecRec* Rec=NPRIVATE::AuxVec;Rec->type != ELF::AT_NULL;Rec++)
  {
   LogMsg("Aux: Type=%u, Value=%p",Rec->type, (void*)Rec->val);
  }
}
//------------------------------------------------------------------------------------------------------------

}

//============================================================================================================
// %rdi, %rsi, %rdx, %rcx, %r8 and %r9
// DescrPtr must be set to 'ELF Auxiliary Vectors' (Stack pointer at ELF entry point)
//
static int Initialize(void* DescrPtr)
{
 size_t StubsSize = 0;
 void* StubsPtr = NAPI::exit.FindStubsArea(&StubsSize);
 if(!StubsPtr || !StubsSize)return -1;
 if(NPRIVATE::MakeTblWritable(StubsPtr, StubsSize, true) < 0)return -2;

 NAPI::exit.Init(NPRIVATE::ESysCNum::exit);
 NAPI::exit_group.Init(NPRIVATE::ESysCNum::exit_group);
 NAPI::open.Init(NPRIVATE::ESysCNum::open);
 NAPI::close.Init(NPRIVATE::ESysCNum::close);
 NAPI::creat.Init(NPRIVATE::ESysCNum::creat);

 NAPI::read.Init(NPRIVATE::ESysCNum::read);
 NAPI::readv.Init(NPRIVATE::ESysCNum::readv);
 NAPI::write.Init(NPRIVATE::ESysCNum::write);
 NAPI::writev.Init(NPRIVATE::ESysCNum::writev);

 NAPI::lseek.Init(NPRIVATE::ESysCNum::lseek);
 NAPI::mkdir.Init(NPRIVATE::ESysCNum::mkdir);
 NAPI::rmdir.Init(NPRIVATE::ESysCNum::rmdir);
 NAPI::unlink.Init(NPRIVATE::ESysCNum::unlink);
 NAPI::rename.Init(NPRIVATE::ESysCNum::rename);
 NAPI::readlink.Init(NPRIVATE::ESysCNum::readlink);

 NAPI::mprotect.Init(NPRIVATE::ESysCNum::mprotect);
 NAPI::munmap.Init(NPRIVATE::ESysCNum::munmap);
 NAPI::madvise.Init(NPRIVATE::ESysCNum::madvise);

 NAPI::stat.Init(NPRIVATE::ESysCNum::stat);
 NAPI::fstat.Init(NPRIVATE::ESysCNum::fstat);
 NAPI::access.Init(NPRIVATE::ESysCNum::access);

#ifdef _ARCH_X32
 NAPI::stat64.Init(NPRIVATE::ESysCNum::stat64);
 NAPI::fstat64.Init(NPRIVATE::ESysCNum::fstat64);
 NAPI::llseek.Init(NPRIVATE::ESysCNum::llseek);
 NAPI::mmap2.Init(NPRIVATE::ESysCNum::mmap2);
 NAPI::internal_mmap.Init(NPRIVATE::ESysCNum::mmap);
#else
 NAPI::mmap.Init(NPRIVATE::ESysCNum::mmap);
#endif


 if(NPRIVATE::MakeTblWritable(StubsPtr, StubsSize, false) < 0)return -3;

 if(!DescrPtr)  // Try to get AUXV from '/proc/self/auxv'
  {

  }
 if(DescrPtr)   // It may not be known if we are in a shared library
  {
   NPRIVATE::STInfo = DescrPtr;
   void*  APtr = nullptr;
   char** Args = NPRIVATE::GetArgV();
   uint ParIdx = 0;

   NPRIVATE::CLArgs = Args;     // May point to NULL
   do{APtr=Args[ParIdx++];}while(APtr);
   NPRIVATE::EVVars = &Args[ParIdx];
   do{APtr=Args[ParIdx++];}while(APtr);
   NPRIVATE::AuxVec = (ELF::SAuxVecRec*)&Args[ParIdx];
  }

     // NPRIVATE::LogStartupInfo();
 //int res = ((decltype(PX::mprotect)*)&syscall_mprotect_X86_X64)((void*)((uint)&NAPI::open & ~0xFFF), 0x1000, PX::PROT_READ|PX::PROT_WRITE|PX::PROT_EXEC);  // |PX::EMProt.WRITE|PX::EMProt.EXEC
// TODO: Validate presence of all required POSX functions implementation?
// SigWithTmplParam<NTSYSAPI::NtProtectVirtualMemory>();
 return 0;
}
//============================================================================================================


