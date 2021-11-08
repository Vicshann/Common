
#pragma once

//============================================================================================================
// All "members" are placed sequentially in memory but their orded may change
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
_codesec  static SFuncStub<decltype(PX::mmap)> mmap;
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
//#include "Impl_Mem.hpp"
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


// Linux shared library initialization, return from initialization to where? Do not terminate if returning from a shared library

// TODO: Detect Android/BSD
//#pragma code_seg()
//#pragma section(".text")       // NOTE: Only MSVC allows taking address of an array into its member, CLANG complains 'non-constant-expression cannot be narrowed'
//volatile static const inline char Data[] = {"HelloBello!"};
//static inline  decltype(TypeSwitch<IsBigEnd, uint32, uint64>()) Val = 0;
//static inline TSW<IsBigEnd, uint32, uint64>::T Val = 0;
// https://stackoverflow.com/questions/46087730/what-happens-if-you-use-the-32-bit-int-0x80-linux-abi-in-64-bit-code
// https://stackoverflow.com/questions/2535989/what-are-the-calling-conventions-for-unix-linux-system-calls-and-user-space-f
// https://blog.packagecloud.io/eng/2016/04/05/the-definitive-guide-to-linux-system-calls/
// https://www.cs.fsu.edu/~langley/CNT5605/2017-Summer/assembly-example/assembly.html
// https://jumpnowtek.com/shellcode/linux-arm-shellcode-part1.html
/*
RAX -> system call number
RDI -> first argument
RSI -> second argument
RDX -> third argument
R10 -> fourth argument  // Move it from RCX
R8  -> fifth argument
R9  -> sixth argument
*/
//
_codesec  static unsigned char syscall_mprotect_X86_X64[] = {0x48,0xB8, 10,0,0,0,0,0,0,0, 0x49,0x89,0xCA, 0x0F,0x05, 0xC3};   // movabs rax, 10; mov r10, rcx; syscall; ret
//------------------------------------------------------------------------------------------------------------
static _finline void InitSyscallStub(auto& Stub, uint idx)   // auto stub
{
// uint8* Dst = (uint8*)&Stub->Stub;
 memcpy(&Stub.Stub, &syscall_mprotect_X86_X64, sizeof(syscall_mprotect_X86_X64));   //for(uint ctr=0;ctr < sizeof(syscall_mprotect_X86_X64);ctr++)Dst[ctr] = syscall_mprotect_X86_X64[ctr];
 *(uint64*)&((uint8*)&Stub.Stub)[2] = idx;   // Set syscall number: x86_x64
}
//------------------------------------------------------------------------------------------------------------
//
static void* FindStubsArea(size_t* Size)
{
 uint8* Entry = (uint8*)&NAPI::exit;
 uint8 Filler = NAPI::exit.StubFill;
 uint  ESize  = NAPI::exit.StubSize;
 uint8* StubsBeg = nullptr;
 uint8* StubsEnd = nullptr;
 uint8* StubsPtr = Entry;
 do
  {
   StubsPtr += ESize;
  }
   while(!memcmp(StubsPtr, Entry, ESize));
 StubsEnd = StubsPtr;
 do
  {
   StubsPtr -= ESize;
  }
   while(!memcmp(StubsPtr, Entry, ESize));
 StubsBeg = StubsPtr + ESize;
 *Size  = StubsEnd - StubsBeg;
 return StubsBeg;
}
//------------------------------------------------------------------------------------------------------------

static _finline int MakeTblWritable(void* StubsArr, size_t StubsLen, bool state)     // TODO: Unprotect entire '.text' section // Find ELF header
{
 decltype(PX::mprotect)* pmprotect = ((decltype(PX::mprotect)*)&syscall_mprotect_X86_X64);
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


namespace NPRIVATE
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
 char** Args = CLArgs;
 uint ParIdx = 0;
 LogMsg("CArguments: ");
 for(uint idx=0;(APtr=Args[ParIdx++]);idx++)
  {
   LogMsg("Arg %u: %s",idx,APtr);
  }
 // Log environment variables
 Args = EVVars;
 LogMsg("EVariables: ");
 while((APtr=Args[ParIdx++]))
  {
   LogMsg("Var: %s",APtr);
  }
 // Log auxilary vector
 for(ELF::SAuxVecRec* Rec=AuxVec;Rec->type != ELF::AT_NULL;Rec++)
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
 NPRIVATE::STInfo = DescrPtr;
 void*  APtr = nullptr;
 char** Args = NPRIVATE::GetArgV();
 uint ParIdx = 0;

 NPRIVATE::CLArgs = Args;     // May point to NULL
 do{APtr=Args[ParIdx++];}while(APtr);
 NPRIVATE::EVVars = &Args[ParIdx];
 do{APtr=Args[ParIdx++];}while(APtr);
 NPRIVATE::AuxVec = (ELF::SAuxVecRec*)&Args[ParIdx];

 size_t StubsSize = 0;
 void* StubsPtr = NPRIVATE::FindStubsArea(&StubsSize);
 if(!StubsPtr || !StubsSize)return -1;
 if(NPRIVATE::MakeTblWritable(StubsPtr, StubsSize, true) < 0)return -2;

 NPRIVATE::InitSyscallStub(NAPI::exit, 60);
 NPRIVATE::InitSyscallStub(NAPI::exit_group, 231);
 NPRIVATE::InitSyscallStub(NAPI::open, 2);
 NPRIVATE::InitSyscallStub(NAPI::close, 3);
 NPRIVATE::InitSyscallStub(NAPI::creat, 85);

 NPRIVATE::InitSyscallStub(NAPI::read, 0);
 NPRIVATE::InitSyscallStub(NAPI::readv, 19);
 NPRIVATE::InitSyscallStub(NAPI::write, 1);
 NPRIVATE::InitSyscallStub(NAPI::writev, 20);

 NPRIVATE::InitSyscallStub(NAPI::lseek, 8);
 NPRIVATE::InitSyscallStub(NAPI::mkdir, 83);
 NPRIVATE::InitSyscallStub(NAPI::rmdir, 84);
 NPRIVATE::InitSyscallStub(NAPI::unlink, 87);
 NPRIVATE::InitSyscallStub(NAPI::rename, 82);
 NPRIVATE::InitSyscallStub(NAPI::readlink, 89);

 NPRIVATE::InitSyscallStub(NAPI::mprotect, 10);
 NPRIVATE::InitSyscallStub(NAPI::mmap, 9);
 NPRIVATE::InitSyscallStub(NAPI::munmap, 11);
 NPRIVATE::InitSyscallStub(NAPI::madvise, 28);

 NPRIVATE::InitSyscallStub(NAPI::stat, 4);
 NPRIVATE::InitSyscallStub(NAPI::fstat, 5);
 NPRIVATE::InitSyscallStub(NAPI::access, 21);

#ifdef _ARCH_X32
 NPRIVATE::InitSyscallStub(&NAPI::stat64, 195);
 NPRIVATE::InitSyscallStub(&NAPI::fstat64, 197);
 NPRIVATE::InitSyscallStub(&NAPI::llseek, 140);
#endif


 if(NPRIVATE::MakeTblWritable(StubsPtr, StubsSize, false) < 0)return -3;
     // NPRIVATE::LogStartupInfo();
 //int res = ((decltype(PX::mprotect)*)&syscall_mprotect_X86_X64)((void*)((uint)&NAPI::open & ~0xFFF), 0x1000, PX::PROT_READ|PX::PROT_WRITE|PX::PROT_EXEC);  // |PX::EMProt.WRITE|PX::EMProt.EXEC
// TODO: Validate presence of all required POSX functions implementation?
// SigWithTmplParam<NTSYSAPI::NtProtectVirtualMemory>();
 return 0;
}
//============================================================================================================


