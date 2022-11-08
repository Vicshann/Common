
#pragma once

//---------------------------------------------------------------------------
// TODO: Implement obfuscation
namespace NPTFM
{

//#if defined(PLT_NIX_USR) || defined(PLT_NIX_USR)
namespace NPRIVATE
{
#if defined(PLT_NIX_USR)
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
_codesec  static unsigned char syscall_mprotect_tmpl[] =   // Used as a template for all syscalls
#if defined(_CPU_ARM)
#if defined(_ARCH_X64)
   {0};
#else     // Not _ARCH_X64
   {0x04,0x70,0x2D,0xE5,  0x7D,0x70,0xA0,0xE3,  0x00,0x00,0x00,0xEF,  0x04,0x70,0x9D,0xE4,  0x1E,0xFF,0x2F,0xE1};  //  PUSH {R7};  MOV R7, #0x7D;  SVC 0;  POP {R7};  BX LR
static const uint SYSCALLSIZE = 4;
static const uint SYSCALLMASK = 0xFFFFF000;
static const uint SYSCALLOFFS = 4;

// enum class is ugly but this pollutes NPRIVATE namespace!
enum class ESysCNum: int { exit=1, exit_group=0xF8, open=5, close=6, creat=8, read=3, readv=0x91, write=4, writev=0x92, lseek=0x13, mkdir=0x27, rmdir=0x28, unlink=0x0A, rename=0x26, readlink=0x55, mprotect=0x7D, mmap=9, mmap2=0xC0, munmap=0x5B, madvise=0xDC, stat=0x6A, fstat=0x6C, access=21, stat64=0xC3, fstat64=0xC5, llseek=0x8C };

#endif    // _ARCH_X64
#elif defined(_CPU_X86)
#if defined(_ARCH_X64)
   {0x48,0xB8, 10,0,0,0,0,0,0,0, 0x49,0x89,0xCA, 0x0F,0x05, 0xC3};   // movabs rax, 10; mov r10, rcx; syscall; ret
static const uint SYSCALLSIZE = 8;
static const uint SYSCALLMASK = 0;
static const uint SYSCALLOFFS = 2;

// NOTE: Will differ for different architectures
// NOTE: Causes InitSyscallStub to use auto instead of int as an index argument
enum class ESysCNum: int { exit=60, exit_group=231, open=2, close=3, creat=85, read=0, readv=19, write=1, writev=20, lseek=8, mkdir=83, rmdir=84, unlink=87, rename=82, readlink=89, mprotect=10, mmap=9, mmap2=192, munmap=11, madvise=28, stat=4, fstat=5, access=21, stat64=195, fstat64=197, llseek=140 };

#else   // Not _ARCH_X64
   {0};
#endif   // _ARCH_X64
#else
#error "Unimplemented syscall template!"
#endif

static const uint SYSCALLSTUBLEN = sizeof(syscall_mprotect_tmpl);
#endif  // PLT_NIX_USR

//===========================================================================
#if defined(PLT_WIN_USR)

static const uint SYSCALLSIZE = 8;
static const uint SYSCALLMASK = 0;
static const uint SYSCALLOFFS = 2;

static const uint SYSCALLSTUBLEN = sizeof(32);
#endif  // PLT_WIN_USR

}
//---------------------------------------------------------------------------

template<class> struct SFuncStub;
template<class TRet, class... TPar> struct SFuncStub<TRet(TPar...)>
{
 using TFuncPtr = TRet (*)(TPar...);
 using IdxType  = TypeForSizeU<NPRIVATE::SYSCALLSIZE>::T;   //   TSW<(NPRIVATE::SYSCALLSIZE < 8), TSW<NPRIVATE::SYSCALLSIZE < 4, TSW<NPRIVATE::SYSCALLSIZE < 2, uint8, uint16>::T, uint32>::T, uint64>::T;
 static const int ArgNum   = sizeof...(TPar);
 static const int StubSize = NPRIVATE::SYSCALLSTUBLEN;   // Let it be copyable with __m256 ? Why?
 static const int StubFill = 0xFF;
 uint8 Stub[StubSize];     // TFuncPtr ptr;  // TReturn (*ptr)(TParameter...);
 consteval SFuncStub(void) {for(int ctr=0;ctr < StubSize;ctr++)Stub[ctr]=StubFill;}  // TODO: Fill with random if PROTECT is enabled?
 _finline TRet operator()(TPar... params){return ((TFuncPtr)&Stub)(params...);}     //return ptr(params...);

//-------------------------
void Init(auto Idx)        // NPRIVATE::ESysCNum  // Use auto?  // Restrict by concept? // On Windows syscall numbers only known at runtime
{
 memcpy(&this->Stub, &NPRIVATE::syscall_mprotect_tmpl, sizeof(NPRIVATE::syscall_mprotect_tmpl));
 *(IdxType*)&this->Stub[NPRIVATE::SYSCALLOFFS] = (*(IdxType*)&this->Stub[NPRIVATE::SYSCALLOFFS] & NPRIVATE::SYSCALLMASK) | (uint)Idx;
}
//-------------------------
bool IsEqual(void* data)
{
 size_t* PtrThis = (size_t*)this;
 size_t* PtrData = (size_t*)data;
 for(uint ctr=StubSize/sizeof(size_t);ctr;ctr++)
  {
   if(PtrThis[ctr] != PtrData[ctr])return false;
  }
 return true;
}
//-------------------------
void* FindStubsArea(size_t* Size)
{
 uint8* StubsBeg = (uint8*)this;     // All entries are initially same, filled with same value which may be random by itself
 uint8* StubsEnd = StubsBeg;     // NOTE: Unreliable to just assume that this is the first entry in NAPI memory block
 do
  {
   StubsEnd += StubSize;
  }
   while(this->IsEqual(StubsEnd));   // Find end of NAPI memory block   !memcmp(StubsEnd, Entry, StubSize)
 do
  {
   StubsBeg -= StubSize;
  }
   while(this->IsEqual(StubsBeg));   // Find beginning of NAPI memory block  !memcmp(StubsBeg, Entry, StubSize)
 StubsBeg += StubSize;
 *Size  = StubsEnd - StubsBeg;
 return StubsBeg;
}

};
//------------------------------------------------------------------------------------------------------------

}
//---------------------------------------------------------------------------

