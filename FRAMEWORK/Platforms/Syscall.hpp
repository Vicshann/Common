
#pragma once

struct NSYSC
{
//===========================================================================
//                              SYSCALL TABLE
//---------------------------------------------------------------------------
// TODO: Implement obfuscation
//namespace NPRIVATE
//{
#if defined(PLT_LIN_USR) || defined(PLT_MAC_USR)

template<int x86_32, int x86_64, int arm_32, int arm_64, int BSD_MAC> struct DSC   // Can be used for Kernel too
{
 static constexpr int
#if defined(SYS_MACOS)
 V = BSD_MAC | (2 << 24)  // SYSCALL_CLASS_UNIX
#elif defined(_SYS_BSD)
 V = BSD_MAC
#else
#  if defined(CPU_ARM)
#    if defined(ARCH_X64)
 V = arm_64
#    else
 V = arm_32
#    endif
#  elif defined(CPU_X86)
#    if defined(ARCH_X64)
 V = x86_64
#    else
 V = x86_32
#    endif
#  endif
#endif  // SYS_MACOS or _SYS_BSD
;
};


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

// https://github.com/nemasu/asmttpd/issues/6    // ???????????????? func signatures?
//
// BSD: https://alfonsosiciliano.gitlab.io/posts/2021-01-02-freebsd-system-calls-table.html
// MAC: https://opensource.apple.com/source/xnu/xnu-1504.3.12/bsd/kern/syscalls.master
//
// https://stackoverflow.com/questions/55403236/why-is-the-open-syscall-supported-on-some-linux-systems-and-not-others
// https://chromium.googlesource.com/chromiumos/docs/+/master/constants/syscalls.md
// https://syscall.sh/
// NOTE: See as syscall deprecation is going on with arm_64
// NOTE: *at file syscalls are avoided on purpose. Use full paths
// NOTE: Keep only POSIX compatible syscalls. POSIX 2001, if not available then POSIX 2008

// NOTE: Causes InitSyscallStub to use auto instead of int as an index argument
enum class ESysCNum: int { //                   x86_32  x86_64  arm_32  arm_64  BSD/MacOS
//                   --- PROCESS/THREAD/DEBUG
                           exit =         DSC<  1,      60,     1,      93,     1         >::V,
                           exit_group =   DSC<  252,    231,    248,    94,     -1        >::V,   // Is 'exit' works as 'exit_group' on BSD?
                           fork =         DSC<  2,      57,     2,      -1,     2         >::V,   // Memory spaces are separate but memory contents is available to the child process as CopyOnWrite
                           vfork =        DSC<  190,    58,     190,    -1,     66        >::V,   // Same memory, suspended caller thread. // Linux: Clone: CLONE_VM | CLONE_VFORK | SIGCHLD  // It is a special case of 'clone'.  It is used to create new processes without copying the page tables of the parent process.
                           clone =        DSC<  120,    56,     120,    220,    -1        >::V,   // Linux specific   // Thread: CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGHAND | CLONE_PARENT | CLONE_THREAD | CLONE_IO   // BSD?: https://reviews.freebsd.org/D31473
                           execve =       DSC<  11,     59,     11,     221,    59        >::V,   // Can it be emulated?
                           ptrace =       DSC<  26,     101,    26,     117,    26        >::V,   // Not quite portable
// kill, tkill, tgkill
// syslog
// sigaction
// rt_sigaction
// getrandom
// nanosleep
// getitimer
// setitimer
// timer_create
// timer_gettime
// timer_settime
// timer_delete
// timer_getoverrun
// clock_settime
// clock_gettime
// clock_getres
// clock_nanosleep
// init_module, finit_module, delete_module
                           gettid =       DSC<  224,    186,    224,    178,    -2        >::V,   // Linux-specific
                           getpid =       DSC<  20,     39,     20,     172,    -2        >::V,
                           getppid =      DSC<  64,     110,    64,     173,    -2        >::V,
//                           getpgrp =      DSC<  65,     111,    65,     -1,     -2        >::V,   // Use getpgid(0) instead
                           getpgid =      DSC<  132,    121,    132,    155,    -2        >::V,
                           setpgid =      DSC<  57,     109,    57,     154,    -2        >::V,
                           wait4 =        DSC< 114,      61,   114,     260,    -2        >::V,
//waitid
//                getgid32      getgid     ?????? =       DSC< -1,      -1,     -1,     -1,     -1        >::V,     // User gropu to check if Root?
//                getegid32     getegid     ?????? =       DSC< -1,      -1,     -1,     -1,     -1        >::V,
//                   --- MEMORY
                           mmap =         DSC<  90,     9,      -1,     222,    197       >::V,   // Not present on arm32. Ignore it on any x32 system, use mmap2 instead   // BSD: new mmap (freebsd6, November 2005)
                           mmap2 =        DSC<  192,    -1,     192,    -1,     71        >::V,   // x32 only   // BSD: old, unimplemented on MacOS
                           munmap =       DSC<  91,     11,     91,     215,    73        >::V,
// mremap    // Linux-specific
                           madvise =      DSC<  219,    28,     220,    233,    75        >::V,
                           mprotect =     DSC<  125,    10,     125,    226,    74        >::V,
//                   --- NETWORK
                           socketcall =   DSC<  102,    -1,     -1,     -1,     -1        >::V,   // On x86-32, socketcall() was historically the only entry point for the sockets API. However, starting in Linux 4.3(Nov 1, 2015), direct systemcalls are provided on x86-32 for the sockets API.
                           socket =       DSC<  359,    41,     281,    198,    97        >::V,
                           connect =      DSC<  362,    42,     283,    203,    98        >::V,
                           bind =         DSC<  361,    49,     282,    200,    104       >::V,
                           accept =       DSC<  -1,     43,     285,    202,    30        >::V,   // Not present on x86_32 ( use accept4 instead )
                           accept4 =      DSC<  364,    288,    366,    242,    -1        >::V,   // Nonstandard Linux extension. For x86_32 only
                           listen =       DSC<  363,    50,     284,    201,    106       >::V,
                           shutdown =     DSC<  373,    48,     293,    210,    134       >::V,
                           getsockopt =   DSC<  365,    55,     295,    209,    118       >::V,
                           setsockopt =   DSC<  366,    54,     294,    208,    105       >::V,
//                         send =         DSC<  -1,     -1,     289,    -1,     -1        >::V,   // Only on arm_32?  // sendto(sockfd, buf, len, flags, NULL, 0);   // BSD/MAC: Old, unimplemented
                           sendto =       DSC<  369,    44,     290,    206,    133       >::V,
                           sendmsg =      DSC<  370,    46,     296,    211,    28        >::V,
//                         recv =         DSC<  -1,     -1,     291,    -1,     -1        >::V,   // Only on arm_32?  // recvfrom(fd, buf, len, flags, NULL, 0);     // BSD/MAC: Old, unimplemented
                           recvfrom =     DSC<  371,    45,     292,    207,    29        >::V,
                           recvmsg =      DSC<  372,    47,     297,    212,    27        >::V,
//                   --- FILE/DIRECTORY/SOCKET
                           open =         DSC<  5,      2,      5,      -1,     5         >::V,   // Not present on arm64, use openat
                           openat =       DSC<  295,    257,    322,    56,     -1        >::V,   // P2008, for Arm64 only      // BSD/MAC: uncertain
                           close =        DSC<  6,      3,      6,      57,     6         >::V,
//                         creat =        DSC<  8,      85,     8,      -1,     -1        >::V,   // A call to creat() is equivalent to calling open() with flags equal to O_CREAT|O_WRONLY|O_TRUNC   // BSD/MAC: old creat, unimplemented, was 6
                           read =         DSC<  3,      0,      3,      63,     3         >::V,
                           readv =        DSC<  145,    19,     145,    65,     120       >::V,
                           write =        DSC<  4,      1,      4,      64,     4         >::V,
                           writev =       DSC<  146,    20,     146,    66,     121       >::V,
                           lseek =        DSC<  19,     8,      19,     62,     199       >::V,   // Use llseek on X32 systems instead    // BSD/MAC: x64 offsets on x32?
                           llseek =       DSC<  140,    -1,     140,    -1,     -1        >::V,   // x32 only            // BSD/MAC: old lseek, unimplemented, was 19
                           mkdir =        DSC<  39,     83,     39,     -1,     136       >::V,   // mkdirat  on arm64
                           mkdirat =      DSC<  296,    258,    323,    34,     -1        >::V,   // P2008, for Arm64 only
                           mknod =        DSC<  14,     133,    14,     -1,     -2        >::V,   //  i.e. mknod("foobar", S_IFIFO|0666)  - create a named pipe
                           mknodat =      DSC<  297,    259,    324,    33,     -2        >::V,   // P2008, for Arm64 only
                           rmdir =        DSC<  40,     84,     40,     -1,     137       >::V,   // No rmdir on arm64, use unlinkat with AT_REMOVEDIR flag  // /proc/self/fd
                           unlink =       DSC<  10,     87,     10,     -1,     10        >::V,   // Remove file
                           unlinkat =     DSC<  301,    263,    328,    35,     -1        >::V,   // P2008, for Arm64 only: replaces unlink and rmdir
                           rename =       DSC<  38,     82,     38,     -1,     128       >::V,
                           renameat =     DSC<  302,    264,    329,    38,     -1        >::V,   // P2008, for Arm64 only
                           readlink =     DSC<  85,     89,     85,     -1,     58        >::V,
                           readlinkat =   DSC<  305,    267,    332,    78,     -1        >::V,   // P2008, for Arm64 only        // BSD/MAC: 473  ?
                           access =       DSC<  33,     21,     33,     -1,     33        >::V,   // Not present on arm64, use faccessat instead
                           faccessat =    DSC<  307,    269,    334,    48,     -1        >::V,   // P2008, for Arm64 only
// fcntl
                           stat =         DSC<  106,    4,      106,    -1,     188       >::V,   // Use fstatat on arm64 instead
                           stat64 =       DSC<  195,    -1,     195,    -1,     -1        >::V,   // x32 only  // BSD: old stat, unimplemented on MacOS, was 38
                           fstat =        DSC<  108,    5,      108,    80,     189       >::V,
                           fstat64 =      DSC<  197,    -1,     197,    -1,     -1        >::V,   // x32 only  // BSD: old fstat, unimplemented on MacOS, was 62
                           fstatat =      DSC<  300,    262,    327,    79,     -1        >::V,   // P2008, for Arm64 only: newfstatat, fstatat64 on X32  // Replaces stat, not fstat
                           pipe2 =        DSC<  331,    293,    359,    59,     -2        >::V,
                           flock =        DSC<  143,    73,     143,    32,     -2        >::V,
                           fsync =        DSC<  118,    74,     118,    82,     -2        >::V,
                           fdatasync =    DSC<  148,    75,     148,    83,     -2        >::V,
                           fcntl =        DSC<  55,     72,     55,     25,     -2        >::V,
                           dup3 =         DSC<  330,    292,    358,    24,     -2        >::V,   // was added to Linux in version 2.6.27
                           dup =          DSC<  41,     32,     41,     23,     -2        >::V,
//                   --- DATE/TIME
                           gettimeofday = DSC<  78,     96,     78,     169,    -2        >::V,
                           settimeofday = DSC<  79,     164,    79,     170,    -2        >::V,
};
#endif
//===========================================================================
//                                  STUBS
//---------------------------------------------------------------------------
static constexpr const int MaxStubSize =
#ifdef FWK_OLD_UBOOT
48;
#else
#ifdef FWK_OLD_ARM    // Less than ARMv7, no movw   // Can it be detected with one of __TARGET_ARCH_ARM macros?
20;
#else
16;
#endif
#endif
//---------------------------------------------------------------------------
//                              MacOS
//---------------------------------------------------------------------------
#if defined(PLT_MAC_USR)
/* https://embeddedartistry.com/blog/2019/05/20/exploring-startup-implementations-os-x/

#define SYSCALL_CLASS_NONE  0   // Invalid
#define SYSCALL_CLASS_MACH  1   // Mach
#define SYSCALL_CLASS_UNIX  2   // Unix/BSD
#define SYSCALL_CLASS_MDEP  3   // Machine-dependent
#define SYSCALL_CLASS_DIAG  4   // Diagnostics

#define SYSCALL_CLASS_SHIFT	24
#define SYSCALL_CLASS_MASK	(0xFF << SYSCALL_CLASS_SHIFT)
#define SYSCALL_NUMBER_MASK	(~SYSCALL_CLASS_MASK)
*/

// ABI of function calls is same as Linux (System-V):
// X86-64: Arguments are passed through the rdi, rsi, rdx, r10, r8 and r9 registers, respectively.
// The syscall number is in the rax register.
// int main(int argc, char *argv[], char *envp[])  // argv[-2] is normal stack frame as set by kernel
// Original LLVM`s LLD is unable to create LC_UNIXTHREAD entry points and always creates LC_MAIN+LC_LOAD_DYLINKER

// 00007FFC1CB80951 48:C7C0 55FFFF7F  mov rax,7FFFFF55  // Sign extended   // Ordinary 'mov eax, 4'  is zero extended so no need to use movabs for DWORD values

#  if defined(CPU_X86) && defined(ARCH_X64)
//_codesec      // Putting this into code section only needed if syscalls are encrypted or on Windows, when passing a syscall number as an argument to avoid calling ntdll directly(worth it?)
static constexpr inline uint8 syscall_tmpl[MaxStubSize] = {
                                                           0xB8, (uint)ESysCNum::mprotect, (uint)ESysCNum::mprotect >> 8, (uint)ESysCNum::mprotect >> 16, (uint)ESysCNum::mprotect >> 24,  // mov eax, ESysCNum::mprotect
                                                           0x49, 0x89, 0xCA,    // mov r10, rcx
                                                           0x0F, 0x05,          // syscall
                                                           0x73, 0x03,          // jnc to retn
                                                           0x48, 0xF7, 0xD8,    // neg rax  // Make error code negative, as in Linux syscalls
                                                           0xC3                 // retn
};
static constexpr inline uint SYSCALLOFFS = 1;
static constexpr inline uint SYSCALLSFTL = 0;
static constexpr inline uint SYSCALLMASK = 0;

#  else
#  error "Unsupported MacOS architecture(ARM or X32)!"
#  endif
//---------------------------------------------------------------------------
//                              Windows
//---------------------------------------------------------------------------
#elif defined(PLT_WIN_USR)
//static consteval uint64 MakeProcID(uint64 DllHash, auto&& ProcName){return (uint64)NCRYPT::CRC32(ProcName)|(DllHash << 32);}
#define WPROCID(lh,pn) ((uint64)NCRYPT::CRC32(pn)|((uint64)lh << 32))

#  if defined(CPU_X86)
#    if defined(ARCH_X64)

// 48 8B4424 30  mov rax,qword ptr ss:[rsp+30]  // NtProtectVirtualMemory(HANDLE ProcessHandle, PPVOID BaseAddress, PSIZE_T RegionSize, ULONG NewProtect, PULONG OldProtect, Syscall_Num);
// B8 50000000   mov eax,50
// 4C 8BD1       mov r10,rcx
// 0F 05         syscall
// CD 2E         int 2E  // Will cause AV (Works only under a virtualized kernel?)
// C3            ret
// NOTE: Control Flow Guard is only granular up to 16 bytes but we do not care about it here anyway. So, keep alignments to the arch size

//_codesec
static constexpr inline uint8 syscall_tmpl[MaxStubSize] = {
                                                           0x48,0x8B,0x44,0x24,0x30,          // mov rax, [rsp+30]    // NtProtectVirtualMemory syscall is unknown on Windows, pass it as an argument
                                                           0x4C,0x8B,0xD1,                    // mov r10,rcx
                                                           0x0F,0x05,                         // syscall
                                                           0xC3,                              // ret
                                                           0xCC,0xCC,0xCC,0xCC,0xCC           // int 3   //NOPs
};
static constexpr inline uint SYSCALLOFFS = 12;     // Store the function name hash in unused space
static constexpr inline uint SYSCALLSFTL = 0;
static constexpr inline uint SYSCALLMASK = 0;
#    else  // ARCH_X32
// In native x32 syscalls you call NTDLL::KiFastSystemCall and system returns to NTDLL::KiFastSystemCallRet:
//   KiFastSystemCall:
//       B800000000 mov eax, 0
//       8BD4       mov edx, esp
//       0F34       sysenter
//   KiFastSystemCallRet:
//       C3         ret
//
//
// ??? x86-32
static constexpr inline uint8 syscall_tmpl[MaxStubSize] = {
1,2,3,4
};
static constexpr inline uint SYSCALLOFFS = 1;     // Store the function name hash in unused space
static constexpr inline uint SYSCALLSFTL = 0;
static constexpr inline uint SYSCALLMASK = 0;
#    endif
#  else  // CPU_ARM (X64 only)      // TODO
#  error "Unsupported Windows architecture(ARM)!"
#  endif
//---------------------------------------------------------------------------
//                              Linux
//---------------------------------------------------------------------------
#elif defined(PLT_LIN_USR)
/* https://stackoverflow.com/questions/2535989/what-are-the-calling-conventions-for-unix-linux-system-calls-and-user-space-f?noredirect=1&lq=1
https://stackoverflow.com/questions/46087730/what-happens-if-you-use-the-32-bit-int-0x80-linux-abi-in-64-bit-code

In x86-32 parameters for Linux system call are passed using registers. %eax for syscall_number.
%ebx, %ecx, %edx, %esi, %edi, %ebp are used for passing 6 parameters to system calls.
The return value is in %eax. All other registers (including EFLAGS) are preserved across the int $0x80.

If there are more than six arguments, %ebx must contain the memory location where the list of arguments is stored -
but don't worry about this because it's unlikely that you'll use a syscall with more than six arguments.
*/

// WARNING: This template may happen to be right in tme middle of stubs block! (Even if all stubs are in one struct now ?????)
//_codesec
static constexpr inline uint8 syscall_tmpl[MaxStubSize] = {  // Used as a template for all syscalls
#  if defined(CPU_ARM)
#    if defined(ARCH_X64)
   (uint8)((uint8)ESysCNum::mprotect << 5)|0x08,(uint8)((uint)ESysCNum::mprotect >> 3),0x80,0xD2,   // MOV X8, #0    // D2 80 XX X?
   0x01,0x00,0x00,0xD4,   // SVC 0
   0xC0,0x03,0x5F,0xD6,   // RET
   0xC0,0x03,0x5F,0xD6    // RET
};   // Arm64
static constexpr inline uint SYSCALLOFFS = 0;  // Offset of uint32 in bytes
static constexpr inline uint SYSCALLSFTL = 5;  // Offset of value in bits (Left shift)
static constexpr inline uint SYSCALLMASK = 0xFFE0001F;  // SOOOOOOO OHH????? ???????? ???RRRRR
#    else       // Arm32
#ifdef FWK_OLD_ARM
   0x80,0x40,0x2D,0xE9,                       // PUSH {R7,LR}
   0x04,0x70,0x9F,0xE5,                       // LDR  R7, +12
   0x00,0x00,0x00,0xEF,                       // SVC 0
   0x80,0x80,0xBD,0xE8,                       // POP {R7,PC}
   (uint8)ESysCNum::mprotect,0,0,0
};

static constexpr inline uint SYSCALLOFFS = 16;
static constexpr inline uint SYSCALLSFTL = 0;
static constexpr inline uint SYSCALLMASK = 0;
#else
   0x80,0x40,0x2D,0xE9,                       // PUSH {R7,LR}
   (uint8)ESysCNum::mprotect,0x70,0x00,0xE3,  // MOVW R7, #0x7D     // Max encoded 0xFFFF : {0}{1} 7{2} 0{3} E3   // E3 0X 7X XX   // NOTE: ARMv7 and above
   0x00,0x00,0x00,0xEF,                       // SVC 0
   0x80,0x80,0xBD,0xE8                        // POP {R7,PC}
};

static constexpr inline uint SYSCALLOFFS = 4;
static constexpr inline uint SYSCALLSFTL = 0;
static constexpr inline uint SYSCALLMASK = 0xFF00F000;    // Max number is FFF
#endif
#    endif    // ARCH_X64
#  elif defined(CPU_X86)
#    if defined(ARCH_X64)     // X86-64
   0xB8, (uint8)ESysCNum::mprotect,0,0,0,  // mov eax, 10    // zero expanded
   0x49,0x89,0xCA,                         // mov r10, rcx
   0x0F,0x05,                              // syscall
   0xC3,                                   // ret
   0x90,0x90,0x90,0x90,0x90                // NOPs
};
static constexpr inline uint SYSCALLOFFS = 1;
static constexpr inline uint SYSCALLSFTL = 0;
static constexpr inline uint SYSCALLMASK = 0;
     #else   // X86-32
   1,2,3,4
};  // x86_32
static constexpr inline uint SYSCALLOFFS = 0;
static constexpr inline uint SYSCALLSFTL = 0;
static constexpr inline uint SYSCALLMASK = 0;
     #endif
#  endif
//---------------------------------------------------------------------------
//                                U-BOOT
//---------------------------------------------------------------------------
#elif defined(PLT_UBOOT)
static constexpr inline uint8 syscall_tmpl[MaxStubSize] = {  // Used as a template for all syscalls
#  if defined(CPU_ARM)
#    if defined(ARCH_X64)
   0x4B,0x00,0x00,0x58,   // LDR  X11, [PC,8]
   0x60,0x01,0x1F,0xD6,   // BR   X11
   0,0,0,0,
   0,0,0,0
};   // Arm64
static constexpr inline uint SYSCALLOFFS = 8;  // Offset of uint32 in bytes
static constexpr inline uint SYSCALLSFTL = 0;  // Offset of value in bits (Left shift)
static constexpr inline uint SYSCALLMASK = 0;
#    else       // Arm32      // No safe temporary registers on Arm32?
#ifdef FWK_OLD_UBOOT           // An old U-BOOT uses R8 as its context register but Clang explicitly supports only -ffixed-r9
   0x24,0xE0,0x8F,0xE5,        //  STR             LR, off_48
   0x08,0xE0,0xA0,0xE1,        //  MOV             LR, R8
   0x09,0x80,0xA0,0xE1,        //  MOV             R8, R9
   0x0E,0x90,0xA0,0xE1,        //  MOV             R9, LR
   0x00,0xE0,0x8F,0xE2,        //  ADR             LR, loc_34
   0x0C,0xF0,0x9F,0xE5,        //  LDR             PC, =sub_0
   0x09,0xE0,0xA0,0xE1,        //  MOV             LR, R9
   0x08,0x90,0xA0,0xE1,        //  MOV             R9, R8
   0x0E,0x80,0xA0,0xE1,        //  MOV             R8, LR
   0x00,0xF0,0x9F,0xE5,        //  LDR             PC, =sub_0
   0,0,0,0,                    //  ProcPtr
   0,0,0,0,                    //  OrigLR
};
/*
__asm volatile (        // NOTE: This stub will not support hooks (recursion)
  "str LR, OrigLR\n"    // Save original return addr
  "mov LR, R8\n"        // Clang uses R8 for something and expects it to be preserved
  "mov R8, R9\n"        // UBOOT context
  "mov R9, LR\n"        // Original R8
  "adr LR, RetStub\n"   // R14
  "ldr PC, ProcPtr\n"   // R15
  "RetStub: mov LR, R9\n"
  "mov R9, R8\n"
  "mov R8, LR\n"
  "ldr PC, OrigLR\n"
  "ProcPtr: .word 0\n"
  "OrigLR:  .word 0\n"  // R14   // Any return stub will do so hooks will be OK
);
*/
#pragma message(">>> OLD U-BOOT!")
static constexpr inline uint SYSCALLOFFS = 40;
#else
   0x00,0xF0,0x9F,0xE5,      // LDR  R15, [PC,8]  // R15 is PC
   0,0,0,0,
   0,0,0,0,
   0,0,0,0,
};
static constexpr inline uint SYSCALLOFFS = 8;
#endif
static constexpr inline uint SYSCALLSFTL = 0;
static constexpr inline uint SYSCALLMASK = 0;
#    endif    // ARCH_X64
#  elif defined(CPU_X86)
#    if defined(ARCH_X64)     // X86-64
1,2,3,4
};
static constexpr inline uint SYSCALLOFFS = 0;
static constexpr inline uint SYSCALLSFTL = 0;
static constexpr inline uint SYSCALLMASK = 0;
     #else   // X86-32
1,2,3,4
};  // x86_32
static constexpr inline uint SYSCALLOFFS = 0;
static constexpr inline uint SYSCALLSFTL = 0;
static constexpr inline uint SYSCALLMASK = 0;
     #endif
#  endif
//---------------------------------------------------------------------------
#else
#error "Unimplemented platform!"
#endif

static constexpr inline uint SYSCALLSTUBLEN = sizeof(syscall_tmpl);

//}   // NPRIVATE
//---------------------------------------------------------------------------
#ifdef FWK_SC_USED_ONLY
#define SC_STUB_DEF _codesec static constexpr inline
#else
#define SC_STUB_DEF _codesec _used static constexpr inline
#endif

#define SYSC_FILL 0xFF      // TODO: Change at compile time
#define DECL_SYSCALL(id,Func,Name) SC_STUB_DEF NSYSC::SFuncStub<(uint64)id,decltype(Func)> Name alignas(16);
#define DECL_SYSCALLVA(id,Func,Name) SC_STUB_DEF NSYSC::SFuncStubVA<(uint64)id,decltype(Func)> Name alignas(16);

// No nesting:(
// https://cplusplus.com/forum/general/87429/
// https://stackoverflow.com/questions/24527395/compiler-error-when-initializing-constexpr-static-class-member
// https://stackoverflow.com/questions/8108314/error-using-a-constexpr-as-a-template-parameter-within-the-same-class/13775154#13775154

// TODO: Try to avoid duplicates and use generic syscall code, and store only a jmp in SFuncStub

template<uint64 val> struct SStubBase
{
// TODO: On Windows X32 Calculate size of arguments and update last two bytes in the stub if last instr is retn (3 bytes, 0xC2)

 SCVR uint32 ID       = val;
 SCVR uint32 ExID     = val >> 32;        // Zero, if not needed
 SCVR int    StubSize = SYSCALLSTUBLEN;   // Let it be copyable with __m256 ? Why?
 SCVR uint8  StubFill = SYSC_FILL;
 alignas(16) uint8 Stub[StubSize];      // TFuncPtr ptr;  // TReturn (*ptr)(TParameter...);

//-------------------------
 consteval inline SStubBase(void)  // TODO: Fill with random if PROTECT is enabled?   // TODO: Spread 'val' bits across the stub (by 2 bits, from low, store in first and last)
 {
  static_assert((sizeof(*this) % 16) == 0, "Stub size is inappropriate!");
  if constexpr (NCFG::InitSyscalls)    // Not on Windows
   {
    for(uint ctr=0;ctr < StubSize;ctr++)this->Stub[ctr] = syscall_tmpl[ctr];  // memcpy(&this->Stub, &syscall_tmpl, StubSize);   // NOTE: Should be inlined
    uint32 IVal = uint32(this->Stub[SYSCALLOFFS]) | uint32(this->Stub[SYSCALLOFFS+1] << 8) | uint32(this->Stub[SYSCALLOFFS+2] << 16) | uint32(this->Stub[SYSCALLOFFS+3] << 24);        // NOTE: LE
    IVal &= SYSCALLMASK;   // Mask out some of previous bits
    IVal |= (ID << SYSCALLSFTL) & ~SYSCALLMASK;
    this->Stub[SYSCALLOFFS]   = uint8(IVal);
    this->Stub[SYSCALLOFFS+1] = uint8(IVal >> 8);
    this->Stub[SYSCALLOFFS+2] = uint8(IVal >> 16);
    this->Stub[SYSCALLOFFS+3] = uint8(IVal >> 24);
  //  *(uint32*)&this->Stub[SYSCALLOFFS] = (*(uint32*)&this->Stub[SYSCALLOFFS] & SYSCALLMASK) | ID;  // Write a syscall number as uint32  // FAIL: No casts in const
   }
    else
     {
      for(int ctr=0;ctr < StubSize;ctr++)Stub[ctr] = StubFill;
      this->StoreID(val);
     }
 }
//-------------------------
template<typename T> _finline T GetPtr(void) const {return (T)&Stub;}
template<typename T> _finline T GetPtrSC(void) const {return (T)&Stub[SYSCALLOFFS];}
//-------------------------
// On Windows it has to be called at runtime, after real syscall numbers are found
/* constexpr _finline void StoreID(uint32 id=val) const
 {
  Stub[0] = id;
  Stub[1] = id >> 8;
  Stub[2] = id >> 16;
  Stub[3] = id >> 24;
 }
//-------------------------
 constexpr _finline uint32 ReadID(void) const
 {
  return (Stub[3] << 24)|(Stub[2] << 16)|(Stub[1] << 8)|Stub[0];
 }
//-------------------------
// May be called in loop for all stubs
_finline void Init(void) const       // NPRIVATE::ESysCNum  // Use auto?  // Restrict by concept? // On Windows syscall numbers only known at runtime
{
 uint32 vID = this->ReadID();
 for(uint ctr=0;ctr < StubSize;ctr++)(char*)&this->Stub[ctr] = syscall_tmpl[ctr];  // memcpy(&this->Stub, &syscall_tmpl, StubSize);   // NOTE: Should be inlined
 *(uint32*)&this->Stub[SYSCALLOFFS] = (*(uint32*)&this->Stub[SYSCALLOFFS] & SYSCALLMASK) | vID;  // Write a syscall number as uint32
}
//-------------------------
// TODO: Use a separate allocated code memory to initialize syscalls if RWX mprotect fails on the 'text' segment
static _finline int ChangeStubsProt(void* StubsArr, size_t StubsLen, uint MemProt)   // TODO: Replace with generic memory protection function
{
 decltype(PX::mprotect)* pmprotect = ((decltype(PX::mprotect)*)&syscall_tmpl);   // On Windows, this syscall number must be found first
 StubsLen += ((size_t)StubsArr & (MEMPAGESIZE-1));          // Size don`t have to be page-aligned
 StubsArr  = (void*)((size_t)StubsArr & ~(MEMPAGESIZE-1));  // Addr must be page-aligned
 return pmprotect(StubsArr, StubsLen, PX::PROT_READ|MemProt);  // On Windows requires passing syscall number directly
}
//-------------------------
template<uint Size> static _finline void InitStubArray(void* StubsArr)
{
 static_assert((Size % sizeof(BType)) == 0, "Stub array size is inappropriate!");
 for (uint ctr=0,total=Size/sizeof(BType);ctr < total;ctr++)(((BType*)StubsArr)[ctr]).Init();
} */
//-------------------------
};
//---------------------------------------------------------------------------


//template<class> struct SFuncStub;
template<uint64, class> struct SFuncStub;
template<uint64 val, class TRet, class... TPar> struct SFuncStub<val, TRet(TPar...)>: public SStubBase<val>
{
// using BType    = SFuncStub<0,int(int)>;  // Base type to access members
 using TFuncPtr  = TRet (*)(TPar...);
 SCVR int ArgNum = sizeof...(TPar);

//-------------------------
_finline TRet operator()(TPar... params) const {return ((const TFuncPtr)&this->Stub)(params...);}     //return ptr(params...);  // asm volatile ("nop" ::: "memory")
//-------------------------
};
//------------------------------------------------------------------------------------------------------------
template<uint64, class> struct SFuncStubVA;
template<uint64 val, class TRet, class... TPar> struct SFuncStubVA<val, TRet(TPar...)>: public SStubBase<val>
{
// using BType    = SFuncStub<0,int(int)>;  // Base type to access members
 using TFuncPtr  = TRet (*)(TPar..., ...);
 SCVR int ArgNum = sizeof...(TPar);

//-------------------------
template<typename... VA> _finline TRet operator()(TPar... params, VA... vp) const {return ((const TFuncPtr)&this->Stub)(params..., vp...);}     //return ptr(params...);  // asm volatile ("nop" ::: "memory")
//-------------------------
};

//using SATYPE = SFuncStub<0,int(int)>;
//------------------------------------------------------------------------------------------------------------
};


