
#pragma once

//============================================================================================================
// All "members" are placed sequentially in memory but their order may change
// NOTE: Do not expect that the memory can be executable and writable at the same time! There is 'maxprot' values may be defined in EXE header which will limit that or it may be limited by the system
// It MUST be Namespace, not Struct to keep these in code segment (MSVC linker, even with Clang compiler) // FIXED: declared as _codesec SysApi
// NOTE: Unreferenced members will be optimized out!
struct SAPI  // POSIX API implementation  // https://docs.oracle.com/cd/E19048-01/chorus4/806-3328/6jcg1bm05/index.html
{
DECL_SYSCALL(NSYSC::ESysCNum::exit,     PX::exit,     exit     )
DECL_SYSCALL(NSYSC::ESysCNum::fork,     PX::fork,     fork     )
DECL_SYSCALL(NSYSC::ESysCNum::vfork,    PX::vfork,    vfork    )       // For thread creation on old MacOS?
DECL_SYSCALL(NSYSC::ESysCNum::execve,   PX::execve,   execve   )
//DECL_SYSCALL(NSYSC::ESysCNum::ptrace,   PX::ptrace, ptrace);

DECL_SYSCALL(NSYSC::ESysCNum::mmap,     PX::mmap,     mmap     )
DECL_SYSCALL(NSYSC::ESysCNum::munmap,   PX::munmap,   munmap   )
DECL_SYSCALL(NSYSC::ESysCNum::madvise,  PX::madvise,  madvise  )
DECL_SYSCALL(NSYSC::ESysCNum::mprotect, PX::mprotect, mprotect )

DECL_SYSCALL(NSYSC::ESysCNum::stat,     PX::stat,     stat     )       // Struct?
DECL_SYSCALL(NSYSC::ESysCNum::fstat,    PX::fstat,    fstat    )     // Struct?

DECL_SYSCALL(NSYSC::ESysCNum::open,     PX::open,     open     )
DECL_SYSCALL(NSYSC::ESysCNum::close,    PX::close,    close    )
DECL_SYSCALL(NSYSC::ESysCNum::read,     PX::read,     read     )
DECL_SYSCALL(NSYSC::ESysCNum::write,    PX::write,    write    )
DECL_SYSCALL(NSYSC::ESysCNum::readv,    PX::readv,    readv    )
DECL_SYSCALL(NSYSC::ESysCNum::writev,   PX::writev,   writev   )
DECL_SYSCALL(NSYSC::ESysCNum::lseek,    PX::lseek,    lseek    )  // Alwats uint64 offsets
DECL_SYSCALL(NSYSC::ESysCNum::mkdir,    PX::mkdir,    mkdir    )
DECL_SYSCALL(NSYSC::ESysCNum::rmdir,    PX::rmdir,    rmdir    )
DECL_SYSCALL(NSYSC::ESysCNum::unlink,   PX::unlink,   unlink   )
DECL_SYSCALL(NSYSC::ESysCNum::rename,   PX::rename,   rename   )
DECL_SYSCALL(NSYSC::ESysCNum::readlink, PX::readlink, readlink )
DECL_SYSCALL(NSYSC::ESysCNum::access,   PX::access,   access   )

// PTHREAD syscalls?

} static constexpr inline SysApi alignas(16);   // Declared to know exact address(?), its size is ALWAYS 1
//============================================================================================================
// In fact, this is LINUX, not POSIX API emulation
struct NAPI    // https://docs.oracle.com/cd/E19048-01/chorus4/806-3328/6jcg1bm05/index.html
{
FUNC_WRAPPER(PX::exit_group, exit_group ) {return SAPI::exit(args...);}
FUNC_WRAPPER(PX::fork,       fork       ) {return SAPI::fork(args...);}
FUNC_WRAPPER(PX::vfork,      vfork      ) {return SAPI::vfork(args...);}
FUNC_WRAPPER(PX::execve,     execve     ) {return SAPI::execve(args...);}

FUNC_WRAPPER(PX::mmap,       mmap       ) {return SAPI::mmap(args...);}
FUNC_WRAPPER(PX::munmap,     munmap     ) {return SAPI::munmap(args...);}
FUNC_WRAPPER(PX::madvise,    madvise    ) {return SAPI::madvise(args...);}
FUNC_WRAPPER(PX::mprotect,   mprotect   ) {return SAPI::mprotect(args...);}

FUNC_WRAPPER(PX::stat,       stat       ) {return SAPI::stat(args...);}
FUNC_WRAPPER(PX::fstat,      fstat      ) {return SAPI::fstat(args...);}

FUNC_WRAPPER(PX::open,       open       ) {return SAPI::open(args...);}
FUNC_WRAPPER(PX::close,      close      ) {return SAPI::close(args...);}
FUNC_WRAPPER(PX::read,       read       ) {return SAPI::read(args...);}
FUNC_WRAPPER(PX::write,      write      ) {return SAPI::write(args...);}
FUNC_WRAPPER(PX::readv,      readv      ) {return SAPI::readv(args...);}
FUNC_WRAPPER(PX::writev,     writev     ) {return SAPI::writev(args...);}
FUNC_WRAPPER(PX::lseek,      lseek      ) {return SAPI::lseek(args...);}
FUNC_WRAPPER(PX::mkdir,      mkdir      ) {return SAPI::mkdir(args...);}
FUNC_WRAPPER(PX::rmdir,      rmdir      ) {return SAPI::rmdir(args...);}
FUNC_WRAPPER(PX::unlink,     unlink     ) {return SAPI::unlink(args...);}
FUNC_WRAPPER(PX::rename,     rename     ) {return SAPI::rename(args...);}
FUNC_WRAPPER(PX::readlink,   readlink   ) {return SAPI::readlink(args...);}
FUNC_WRAPPER(PX::access,     access     ) {return SAPI::access(args...);}

FUNC_WRAPPER(PX::exit,       exit       ) {return 0;}   // TODO: POSIX_THREAD syscall?

};
//============================================================================================================
static sint Initialize(void* StkFrame=nullptr, void* ArgA=nullptr, void* ArgB=nullptr, void* ArgC=nullptr)    // _finline ?
{
 // NOTE: Init syscalls before InitStartupInfo if required
 InitStartupInfo(StkFrame, ArgA, ArgB, ArgC);
 LogStartupInfo();
 return 0;
}
//============================================================================================================
/*
pthread_create:

    proc_info
    bsdthread_create
    thread_selfid

pthread_join:

    __disable_threadsignal
    __semwait_signal


*/
