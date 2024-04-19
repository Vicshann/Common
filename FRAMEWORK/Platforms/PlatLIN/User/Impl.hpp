
#pragma once

#include "../PlatDef.hpp"
//============================================================================================================
// All "members" are placed sequentially in memory but their order may change
// NOTE: Do not expect that the memory can be executable and writable at the same time! There is 'maxprot' values may be defined in EXE header which will limit that or it may be limited by the system
// It MUST be Namespace, not Struct to keep these in code segment (MSVC linker, even with Clang compiler) // FIXED: declared as _codesec SysApi
// NOTE: Unreferenced members will be optimized out!
// NOTE: Do not use these directly, not all of them are portable even between x32/x64 on the same cpu
//
struct SAPI  // POSIX API implementation  // https://docs.oracle.com/cd/E19048-01/chorus4/806-3328/6jcg1bm05/index.html
{
DECL_SYSCALL(NSYSC::ESysCNum::exit,       PX::exit,       exit       )
DECL_SYSCALL(NSYSC::ESysCNum::exit_group, PX::exit_group, exit_group )
#if !defined(SYS_BSD) && !defined(SYS_MACOS)
#  if defined(CPU_X86) && defined(ARCH_X64)
DECL_SYSCALL(NSYSC::ESysCNum::clone,      PX::cloneB0,    clone      )  // Linux only
#  else
DECL_SYSCALL(NSYSC::ESysCNum::clone,      PX::cloneB1,    clone      )  // Linux only
#  endif
#endif
#if !defined(CPU_ARM) || !defined(ARCH_X64)
DECL_SYSCALL(NSYSC::ESysCNum::fork,       PX::fork,       fork       )
DECL_SYSCALL(NSYSC::ESysCNum::vfork,      PX::vfork,      vfork      )
#endif
DECL_SYSCALL(NSYSC::ESysCNum::execve,     PX::execve,     execve     )
//DECL_SYSCALL(NSYSC::ESysCNum::ptrace,     PX::ptrace,     ptrace    )
DECL_SYSCALL(NSYSC::ESysCNum::process_vm_readv,     PX::process_vm_readv,     process_vm_readv     )
DECL_SYSCALL(NSYSC::ESysCNum::process_vm_writev,    PX::process_vm_writev,    process_vm_writev    )

DECL_SYSCALL(NSYSC::ESysCNum::wait4,      PX::wait4,      wait4      )
DECL_SYSCALL(NSYSC::ESysCNum::futex,      PX::futex,      futex      )

DECL_SYSCALL(NSYSC::ESysCNum::gettimeofday, PX::gettimeofday, gettimeofday  )
DECL_SYSCALL(NSYSC::ESysCNum::settimeofday, PX::settimeofday, settimeofday  )

DECL_SYSCALL(NSYSC::ESysCNum::gettid,     PX::gettid,     gettid     )
DECL_SYSCALL(NSYSC::ESysCNum::getpid,     PX::getpid,     getpid     )
DECL_SYSCALL(NSYSC::ESysCNum::getppid,    PX::getppid,    getppid    )
//DECL_SYSCALL(NSYSC::ESysCNum::getpgrp,    PX::getpgrp,    getpgrp    )   // Not present on ARM64, reimplemented with getpgid(0)
DECL_SYSCALL(NSYSC::ESysCNum::getpgid,    PX::getpgid,    getpgid    )
DECL_SYSCALL(NSYSC::ESysCNum::setpgid,    PX::setpgid,    setpgid    )

#if !defined(ARCH_X32)
DECL_SYSCALL(NSYSC::ESysCNum::mmap,       PX::mmap,       mmap       )
#endif

DECL_SYSCALL(NSYSC::ESysCNum::munmap,     PX::munmap,     munmap     )
DECL_SYSCALL(NSYSC::ESysCNum::madvise,    PX::madvise,    madvise    )
DECL_SYSCALL(NSYSC::ESysCNum::mprotect,   PX::mprotect,   mprotect   )
DECL_SYSCALL(NSYSC::ESysCNum::msync,      PX::msync,      msync      )

DECL_SYSCALL(NSYSC::ESysCNum::truncate,   PX::truncate,   truncate   )
DECL_SYSCALL(NSYSC::ESysCNum::ftruncate,  PX::ftruncate,  ftruncate  )
DECL_SYSCALL(NSYSC::ESysCNum::getdents,   PX::getdents64, getdents   )     // getdents64 on x32 and x64
DECL_SYSCALL(NSYSC::ESysCNum::fstat,      PX::fstat,      fstat      )     // Struct?
DECL_SYSCALL(NSYSC::ESysCNum::fcntl,      PX::fcntl,      fcntl      )     // Too specific to put in NAPI?
DECL_SYSCALL(NSYSC::ESysCNum::ioctl,      PX::ioctl,      ioctl      )
DECL_SYSCALL(NSYSC::ESysCNum::flock,      PX::flock,      flock      )
DECL_SYSCALL(NSYSC::ESysCNum::fsync,      PX::fsync,      fsync      )
DECL_SYSCALL(NSYSC::ESysCNum::fdatasync,  PX::fdatasync,  fdatasync  )
DECL_SYSCALL(NSYSC::ESysCNum::pipe2,      PX::pipe2,      pipe2      )
DECL_SYSCALL(NSYSC::ESysCNum::dup3,       PX::dup3,       dup3       )
DECL_SYSCALL(NSYSC::ESysCNum::dup,        PX::dup,        dup        )     // Does not allow to pass any flags (O_CLOEXEC), can be replaced with fcntl

#if !defined(CPU_ARM) || !defined(ARCH_X64)
//DECL_SYSCALL(NSYSC::ESysCNum::stat,       PX::stat,       stat       )     // Struct?
DECL_SYSCALL(NSYSC::ESysCNum::open,       PX::open,       open       )
DECL_SYSCALL(NSYSC::ESysCNum::mknod,      PX::mknod,      mknod      )     // Too specific to put in NAPI?
DECL_SYSCALL(NSYSC::ESysCNum::mkdir,      PX::mkdir,      mkdir      )
DECL_SYSCALL(NSYSC::ESysCNum::rmdir,      PX::rmdir,      rmdir      )
DECL_SYSCALL(NSYSC::ESysCNum::unlink,     PX::unlink,     unlink     )
DECL_SYSCALL(NSYSC::ESysCNum::rename,     PX::rename,     rename     )
DECL_SYSCALL(NSYSC::ESysCNum::readlink,   PX::readlink,   readlink   )
DECL_SYSCALL(NSYSC::ESysCNum::access,     PX::access,     access     )
#endif
// Bunch of *at functions (Useful together with 'getdents')
DECL_SYSCALL(NSYSC::ESysCNum::openat,     PX::openat,     openat     )
DECL_SYSCALL(NSYSC::ESysCNum::mknodat,    PX::mknodat,    mknodat    )
DECL_SYSCALL(NSYSC::ESysCNum::mkdirat,    PX::mkdirat,    mkdirat    )
DECL_SYSCALL(NSYSC::ESysCNum::unlinkat,   PX::unlinkat,   unlinkat   )
DECL_SYSCALL(NSYSC::ESysCNum::renameat,   PX::renameat,   renameat   )
DECL_SYSCALL(NSYSC::ESysCNum::readlinkat, PX::readlinkat, readlinkat )
DECL_SYSCALL(NSYSC::ESysCNum::faccessat,  PX::faccessat,  faccessat  )
DECL_SYSCALL(NSYSC::ESysCNum::fstatat,    PX::fstatat,    fstatat    )     // fstatat64 on x32 and newfstatat on x64  (original FStat struct is unreliable anyway)

DECL_SYSCALL(NSYSC::ESysCNum::close,      PX::close,    close        )
DECL_SYSCALL(NSYSC::ESysCNum::read,       PX::read,     read         )
DECL_SYSCALL(NSYSC::ESysCNum::write,      PX::write,    write        )
DECL_SYSCALL(NSYSC::ESysCNum::readv,      PX::readv,    readv        )
DECL_SYSCALL(NSYSC::ESysCNum::writev,     PX::writev,   writev       )
DECL_SYSCALL(NSYSC::ESysCNum::lseek,      PX::lseek,    lseek        )     // Offsets are same size as the architecture

#if defined(ARCH_X32)
DECL_SYSCALL(NSYSC::ESysCNum::mmap2,      PX::mmap2,    mmap2        )
//DECL_SYSCALL(NSYSC::ESysCNum::stat64,     PX::stat64,   stat64       )
//DECL_SYSCALL(NSYSC::ESysCNum::fstat64,    PX::fstat64,  fstat64      )
DECL_SYSCALL(NSYSC::ESysCNum::llseek,     PX::llseek,   llseek       )     // Definition is different from lseek
#endif


} static constexpr inline SysApi alignas(16);   // Declared to know exact address(?), its size is ALWAYS 1
//============================================================================================================
#include "../../UtilsFmtELF.hpp"
//============================================================================================================
// In fact, this is LINUX, not POSIX API emulation
//   FUNC_WRAPPER(PX::cloneB0,    clone   *** MAKES THE CODE 1.5k BIGGER ***   ) {return [&]<typename T=SAPI>() _finline {if constexpr(IsArchX64&&IsCpuX86)return T::clone(args...); else return T::clone(GetParFromPk<0>(args...),GetParFromPk<1>(args...),GetParFromPk<2>(args...),GetParFromPk<4>(args...),GetParFromPk<3>(args...));}();}  // Linux-specifix, need implementation for BSD
//
struct NAPI    // https://docs.oracle.com/cd/E19048-01/chorus4/806-3328/6jcg1bm05/index.html
{
#include "../../UtilsNAPI.hpp"

FUNC_WRAPPERFI(PX::exit,       exit       ) {return SAPI::exit(args...);}
FUNC_WRAPPERFI(PX::exit_group, exit_group ) {return SAPI::exit_group(args...);}
FUNC_WRAPPERFI(PX::cloneB0,    clone      ) { CALL_IFEXISTRPC(clone,clone,(IsArchX64&&IsCpuX86),(args...),(flags,newsp,parent_tid,tls,child_tid),(uint32 flags, vptr newsp, int32* parent_tid, int32* child_tid, vptr tls)) }
FUNC_WRAPPERFI(PX::fork,       fork       ) { CALL_IFEXISTRN(fork,clone,NAPI,(args...),(PX::SIGCHLD, nullptr, nullptr, nullptr, nullptr)) }
FUNC_WRAPPERFI(PX::vfork,      vfork      ) { CALL_IFEXISTRN(vfork,clone,NAPI,(args...),(PX::CLONE_VM | PX::CLONE_VFORK | PX::SIGCHLD, nullptr, nullptr, nullptr, nullptr)) }   // SIGCHLD makes the cloned process work like a "normal" unix child process
FUNC_WRAPPERFI(PX::execve,     execve     ) {return SAPI::execve(args...);}

FUNC_WRAPPERFI(PX::process_vm_readv,     process_vm_readv     ) {return SAPI::process_vm_readv(args...);}
FUNC_WRAPPERFI(PX::process_vm_writev,    process_vm_writev    ) {return SAPI::process_vm_writev(args...);}
//------------------------------------------------------------------------------------------------------------
FUNC_WRAPPERNI(PX::gettimeofday,  gettimeofday  )     // TODO: Prefer VDSO
{
 PX::timeval*  tv = GetParFromPk<0>(args...);
 PX::timezone* tz = GetParFromPk<1>(args...);
 int res = SAPI::gettimeofday(tv, tz);
 if(res < 0)return res;
 if(tz)        // No caching, always update. Use tz only if you expect the 'localtime' file to be changed
  {
   PX::timeval tvb = {};
   if(!tv)
    {
     int resi = SAPI::gettimeofday(&tvb, tz);
     if(resi < 0)return res;
     tv = &tvb;
    }
   tz->dsttime = 0;
   if(tz->utcoffs == -1)
    {
     int resi = UpdateTZOffsUTC(tv->sec);
     if(resi < 0){tz->utcoffs = 0; return res;}
    }
   tz->utcoffs = GetTZOffsUTC();
  }
 return res;
}
//------------------------------------------------------------------------------------------------------------
FUNC_WRAPPERNI(PX::settimeofday,  settimeofday  ) {return SAPI::settimeofday(args...);}
//------------------------------------------------------------------------------------------------------------
FUNC_WRAPPERFI(PX::futexGD,    futex      ) {return SAPI::futex(args..., nullptr, 0);}
FUNC_WRAPPERFI(PX::wait4,      wait       ) {return SAPI::wait4(args...);}
FUNC_WRAPPERFI(PX::gettid,     gettid     ) {return SAPI::gettid(args...);}
FUNC_WRAPPERFI(PX::getpid,     getpid     ) {return SAPI::getpid(args...);}     // VDSO?
FUNC_WRAPPERFI(PX::getppid,    getppid    ) {return SAPI::getppid(args...);}
FUNC_WRAPPERFI(PX::getpgrp,    getpgrp    ) {return SAPI::getpgid(0);}
FUNC_WRAPPERFI(PX::getpgid,    getpgid    ) {return SAPI::getpgid(args...);}
FUNC_WRAPPERFI(PX::setpgid,    setpgid    ) {return SAPI::setpgid(args...);}
//------------------------------------------------------------------------------------------------------------
FUNC_WRAPPERFI(PX::mmapGD,     mmap       ) { CALL_IFEXISTRPC(mmap,mmap2,(IsArchX64),(args...),(addr,length,prot,flags,fd,uint32(offset>>12)),(vptr addr, size_t length, uint prot, uint flags, int fd, uint64 offset)) }
FUNC_WRAPPERFI(PX::munmap,     munmap     ) {return SAPI::munmap(args...);}
FUNC_WRAPPERFI(PX::madvise,    madvise    ) {return SAPI::madvise(args...);}
FUNC_WRAPPERFI(PX::mprotect,   mprotect   ) {return SAPI::mprotect(args...);}
FUNC_WRAPPERFI(PX::msync,      msync      ) {return SAPI::msync(args...);}
//------------------------------------------------------------------------------------------------------------
FUNC_WRAPPERFI(PX::close,      close      ) {return SAPI::close(args...);}
FUNC_WRAPPERFI(PX::read,       read       ) {return SAPI::read(args...);}
FUNC_WRAPPERFI(PX::write,      write      ) {return SAPI::write(args...);}
FUNC_WRAPPERFI(PX::readv,      readv      ) {return SAPI::readv(args...);}
FUNC_WRAPPERFI(PX::writev,     writev     ) {return SAPI::writev(args...);}
//------------------------------------------------------------------------------------------------------------
// How the file descriptor connects llseek to underlying devices?
// loff_t mtdchar_lseek(struct file *file, loff_t offset, int orig)  // This matches lseek, not llseek on x32
// Is llseek on x32 cannot actually replace lseek, at least for devices?  (We must always use SAPI::lseek for them?)
FUNC_WRAPPERFI(PX::lseekGD,    lseek      )
{
 if constexpr (IsArchX32)
  {
   sint64 rofs = 0;
   sint64 offs = GetParFromPk<1>(args...);
   int res = [](int fd, uint32 offset_high, uint32 offset_low, sint64* result, int whence) _finline {return Y::llseek((PX::fdsc_t)fd, offset_high, offset_low, result, (PX::ESeek)whence);} (GetParFromPk<0>(args...), (uint32)(offs >> 32), (uint32)offs, &rofs, GetParFromPk<2>(args...));  // Lambdas are delayed and can reference an nonexistent members while used in constexpr
   if(res < 0)return res;  // How to return the error code?  // > 0xFFFFF000
   return rofs;
  }
   else return SAPI::lseek(args...);   // Compatible
}
//------------------------------------------------------------------------------------------------------------
// Complicated
FUNC_WRAPPERFI(PX::mkfifo,     mkfifo     ) { CALL_IFEXISTR(mknod,mknodat,(GetParFromPk<0>(args...), PX::S_IFIFO|GetParFromPk<1>(args...), 0),(PX::AT_FDCWD, GetParFromPk<0>(args...), PX::S_IFIFO|GetParFromPk<1>(args...), 0)) }
FUNC_WRAPPERFI(PX::mkdir,      mkdir      ) { CALL_IFEXISTR(mkdir,mkdirat,(args...),(PX::AT_FDCWD, args...)) }
FUNC_WRAPPERFI(PX::rmdir,      rmdir      ) { CALL_IFEXISTR(rmdir,unlinkat,(args...),(PX::AT_FDCWD, args..., PX::AT_REMOVEDIR)) }
FUNC_WRAPPERFI(PX::unlink,     unlink     ) { CALL_IFEXISTR(unlink,unlinkat,(args...),(PX::AT_FDCWD, args..., 0)) }
FUNC_WRAPPERFI(PX::rename,     rename     ) { CALL_IFEXISTRP(rename,renameat,(args...),(PX::AT_FDCWD,oldpath,PX::AT_FDCWD,newpath),(achar* oldpath, achar* newpath)) }
FUNC_WRAPPERFI(PX::readlink,   readlink   ) { CALL_IFEXISTR(readlink,readlinkat,(args...),(PX::AT_FDCWD, args...)) }
FUNC_WRAPPERFI(PX::access,     access     ) { CALL_IFEXISTR(access,faccessat,(args...),(PX::AT_FDCWD, args..., 0)) }
//------------------------------------------------------------------------------------------------------------
FUNC_WRAPPERFI(PX::fstatat,    fstatat    )
{
 int res = SAPI::fstatat(args...);
// DBGDBG("res %i:\r\n%#*.32D",res,256,GetParFromPk<2>(args...));
 if(res >= 0){vptr buf = GetParFromPk<2>(args...); PX::ConvertToNormalFstat((PX::SFStat*)buf, buf);}
 return res;
}
//------------------------------------------------------------------------------------------------------------
FUNC_WRAPPERFI(PX::stat,       stat       )
{
 int res = SAPI::fstatat(PX::AT_FDCWD, args..., 0);     //  CALL_RIFEXISTR(stat,fstatat,(args...),(PX::AT_FDCWD, args..., 0))
// DBGDBG("res %i:\r\n%#*.32D",res,256,GetParFromPk<1>(args...));
 if(res >= 0){vptr buf = GetParFromPk<1>(args...); PX::ConvertToNormalFstat((PX::SFStat*)buf, buf);}
 return res;
}
//------------------------------------------------------------------------------------------------------------
FUNC_WRAPPERFI(PX::fstat,      fstat      )
{
 int res = SAPI::fstat(args...);
// DBGDBG("res %i:\r\n%#*.32D",res,256,GetParFromPk<1>(args...));
 if(res >= 0){vptr buf = GetParFromPk<1>(args...); PX::ConvertToNormalFstat((PX::SFStat*)buf, buf);}
 return res;
}
//------------------------------------------------------------------------------------------------------------
/*
https://stackoverflow.com/questions/52329604/how-to-get-the-file-desciptor-of-a-symbolic-link
https://man7.org/linux/man-pages/man7/symlink.7.html
*/
FUNC_WRAPPERFI(PX::open,       open       ) { CALL_IFEXISTR(open,openat,(args...),(PX::AT_FDCWD, args...)) }
FUNC_WRAPPERFI(PX::pipe2,      pipe       ) {return SAPI::pipe2(args...);}
//------------------------------------------------------------------------------------------------------------
FUNC_WRAPPERFI(PX::getdentsGD, getdents    )
{
 size_t len = GetParFromPk<2>(args...);
 if((ssize_t)len < 0)     // Retrieve real info about links (file/dir)  // FRMWK extension
  {
//   DBGDBG("Following links");
   PX::fdsc_t dfd = GetParFromPk<0>(args...);
   vptr buf = GetParFromPk<1>(args...);
   len = -(ssize_t)len;
   int res = SAPI::getdents(dfd,buf,len);
   if(res <= 0)return res;
   for(int pos=0;pos < res;)
    {
     PX::SDirEnt* ent = (PX::SDirEnt*)((uint8*)buf + pos);
     pos += ent->reclen;
     if(ent->type != PX::DT_LNK)continue;
//     DBGDBG("Link is: %s", &ent->name);
     PX::SFStat sti;
     int sres = NAPI::fstatat(dfd, ent->name, &sti, 0);
//     DBGDBG("fstatat for the link: %i", sres);
     if(sres < 0)continue;   // Or skip the entire loop?
//     DBGDBG("SDirEnt:\r\n%#*.32D",sizeof(sti),&sti);
     if((sti.mode & PX::S_IFREG) == PX::S_IFREG)ent->type = PX::DT_REG;
     else if((sti.mode & PX::S_IFDIR) == PX::S_IFDIR)ent->type = PX::DT_DIR;
    }
   return res;
  }
  else return SAPI::getdents(args...);
}
FUNC_WRAPPERFI(PX::flock,      flock       ) {return SAPI::flock(args...);}
FUNC_WRAPPERFI(PX::fsync,      fsync       ) {return SAPI::fsync(args...);}
FUNC_WRAPPERFI(PX::fdatasync,  fdatasync   ) {return SAPI::fdatasync(args...);}

FUNC_WRAPPERFI(PX::dup3,       dup        )
{
 if(GetParFromPk<1>(args...) < 0)  // if newfd is -1 then behave like dup but with flags
  {
   if(GetParFromPk<2>(args...) & PX::O_CLOEXEC)return SAPI::fcntl(GetParFromPk<0>(args...),PX::F_DUPFD_CLOEXEC,0);
     else return SAPI::dup(GetParFromPk<0>(args...));
  }
   else return SAPI::dup3(args...);
}
//------------------------------------------------------------------------------------------------------------
//FUNC_WRAPPER(PX::stat,       stat       ) {return SAPI::stat(args...);}
//FUNC_WRAPPER(PX::fstat,      fstat      ) {return SAPI::fstat(args...);}
//---------------------------------------------------------------------------
// First, it returns from 'clone' into a child process if succeeds.
// Parent process is suspended by call to 'clone' with CLONE_VFORK
// If child exits with 'exit' or address space is replaced with 'execve' parent precess gets resumed
// Without CLONE_VFORK it will return immediately to parent process from 'clone'
// NOTE: You should pass file descriptors to siofd opened with O_CLOEXEC because there will be no attempt to close them before exec
// https://stackoverflow.com/questions/2535989/what-are-the-calling-conventions-for-unix-linux-system-calls-and-user-space-f
// X86-X64: %rdi, %rsi, %rdx, %rcx, %r8 and %r9
// X86-X32: on stack
// Stack:
//   Temps
//   Locals
//   RetAddr (Just a local on ARM)
//
// https://developers.redhat.com/blog/2015/08/19/launching-helper-process-under-memory-and-latency-constraints-pthread_create-and-vfork
/* Block all signals in the parent before calling vfork. This is for the safety of the child which inherits signal dispositions and
handlers. The child, running in the parent's stack, may be delivered a signal. For example on Linux a killpg call delivering a signal to a
process group may deliver the signal to the vfork-ing child and you want to avoid this. The easy way to do this is via: sigemptyset,
sigaction, and then undo this when you return to the parent. To be completely correct the child should set all non-SIG_IGN signals to
SIG_DFL and then restore the original signal mask, thus allowing the vforking child to receive signals that were actually intended for it, but
without executing any handlers the parent had setup that could corrupt state. When using glibc and Linux these functions i.e. sigemtpyset,
sigaction, etc. are safe to use after vfork. */
//
FUNC_WRAPPERNI(PX::spawn,       spawn       )
{
static constexpr const uint32 NotCloneFlg = PX::O_CLOEXEC;
// TODO: Use 'access' to check if the file exist and is executable?
 volatile int ExecRes = 0;   // If in parent process, we see this to be nonzero when execve has failed    // __asm__ __volatile__("" :: "m" (ExecRes));
 volatile PX::pid_t pid = NAPI::clone(PX::CLONE_VM | PX::CLONE_VFORK | PX::SIGCHLD | ((uint32)GetParFromPk<4>(args...) & ~NotCloneFlg), nullptr, nullptr, nullptr, nullptr);  // vfork     // Same stack, no copy-on-write
 if(pid)   // Not in child (Resumed after execve or exit) (Error Child create error if negative)
  {
   volatile int tmp = ExecRes;  // Some extra to prevent optimization
   if(tmp)return tmp;  // We need the result of execv if it failed  // if exec has failed then pid have no meaning because the child should been exited by now
   return pid;    // Success or vfork has failed
  }
// Only a child gets here
 {
//#if defined(CPU_X86) && defined(ARCH_X64)  // On ARM32 the stack is corrupted too! // Only X86-X64 suffers from overwriting return address from clone by execve or exit so we need to move stack pointer to have more space for child to overwrite
  volatile size_t* padd = (volatile size_t*)StkAlloc(size_t((pid >> 24)+64));   // Some trick with volatile var to avoid optimizations // alloca must be called at the block scope
  *padd = 0;     // Some extra to prevent optimization
//#endif
  volatile int32* fdarr = GetParFromPk<3>(args...);
  if(fdarr)
   {
    uint32 flg = ((uint32)GetParFromPk<4>(args...) & NotCloneFlg);
    for(;;)       // May be it is a bit clumsy format for defining fd list but it is extensible and consistent with argv and envp lists
     {
      int32 oldfd = *(fdarr++);
      if(oldfd < 0)break;
      int32 newfd = *(fdarr++);
      if(newfd < 0)break;         // Useless to dup into an unknown value    // If you need to discard stdout then assign it to /dev/null
      SAPI::dup3(oldfd, newfd, (int32)flg);
//      SAPI::close(oldfd);  // Other pipe`s end must be closed too. So, just always use CLOSEONEXEC  // Do not assume that it have CLOSEONEXEC. Should not left any excess pipe handles open or piping will not work as expected
     }
   }
  // TODO: Inherit all EVARS and update them from the passed array (As default on Windows)
  ExecRes = NAPI::execve(GetParFromPk<0>(args...), GetParFromPk<1>(args...), GetParFromPk<2>(args...));  // Should not return on success    // Is it possible to just drop last argument?
//#if defined(CPU_X86) && defined(ARCH_X64)   // On ARM32 the stack is corrupted too!
  NAPI::exit(ExecRes + (int)*padd);    // Exit the child thread only  // Resume parent on exit  // NOTE: Child enters here and overwrites parent`s return address from 'clone' and skips return value assignment from 'clone'
//#else
//  NAPI::exit(ExecRes);   // Or ESRCH ?
//#endif
 }
 return pid;
}
//------------------------------------------------------------------------------------------------------------
// sysdeps/unix/sysv/linux/createthread.c)        pthread_create
// If CLONE_THREAD is set, the child is placed in the same thread group as the calling process.
// CLONE_SYSVSEM is set, then the child and the calling process share a single list of System V semaphore adjustment (semadj) values
// ARCH_CLONE (&start_thread, STACK_VARIABLES_ARGS, clone_flags, pd, &pd->tid, tp, &pd->tid
// Who will free the stack if the thread suddenly terminated?
// No CREATE_SUSPENDED on Linux
// LIBC-X32: unsigned int pthread_self(){return __readgsdword(8u);}   // Cannot use same approach with GS to avoid conflicts with PTHREADS (LibC may be loaded and initialized from app`s main thread(i.e. by loading some other library dynamically))
// Calculation of TLS space of different libs by the Loader is a MESS!
// TODO: Use ThreadID as some index in a special memory area to find address of a thread`s context frame (SFWCTX)
// No way to disable FramePointer for this function?
// " In fact, CLONE_THREAD implies cloning parent process ID (PPID), just like CLONE_PARENT, and this way children threads are not
//    actually children of the thread that issued clone(), but of its parent. And that's why my wait() calls failed with ECHILD - there were no children. "
// Either remove CLONE_PARENT and make the new thread a child of the one that creates it or use ThreadID to wait for a futex.
// A new thread created with CLONE_THREAD has the same parent process as the caller of clone() (i.e., like CLONE_PARENT)
// NOTE: it is the parent process, which is signaled when the child terminates
// Do not forget about 'wait' and 'zombies'
//
// Stack:
// Rest of the stack
// SThCtx
// User Data
// TLS Block
//
FUNC_WRAPPERNI(NTHD::thread,       thread       )
{
 static constexpr const uint32 CloneFlg    = (PX::CLONE_VM | PX::CLONE_FS | PX::CLONE_FILES | PX::CLONE_SYSVSEM | PX::CLONE_SIGHAND | PX::CLONE_PARENT_SETTID | PX::CLONE_CHILD_CLEARTID | PX::CLONE_THREAD);   // | PX::CLONE_PARENT CLONE_PTRACE  CLONE_SETTLS |  CLONE_PARENT_SETTID // CLONE_CHILD_CLEARTID (? Thread pools? Other systems?)    // CLONE_PARENT is probably not needed with CLONE_THREAD
 static constexpr const uint32 NotCloneFlg = PX::O_CLOEXEC;   // Excluded user-specified flags         // Pass SIGCHLD to inform parent of the thread termination? (NOTE: The parent is noth the caller of 'clone', its the parent of the caller)
 auto   ThProc  = GetParFromPk<0>(args...);
 vptr   ThData  = GetParFromPk<1>(args...);
 if(!ThProc)return PXERR(ENOEXEC);     // Nothing to execute
 size_t DatSize = GetParFromPk<2>(args...);
 size_t StkSize = GetParFromPk<3>(args...);   // NOTE: As StkSize is aligned to a page size, there will be at least one page wasted for ThreadContext struct (Assume it always available for some thread local data?)
 size_t TlsSize = GetParFromPk<4>(args...);   // Slots is at least of pointer size

 size_t* StkFrame = nullptr;
 NTHD::SThCtx* ThrFrame = InitThreadRec((vptr)ThProc, ThData, StkSize, TlsSize, DatSize, &StkFrame);
 if(uint err=MMERR(ThrFrame);err)return -err;
 if constexpr (IsCpuX86)
  {
   if constexpr (IsArchX32)    // NOTE: must match with syscall stub which uses 'popad' on exit: EDI, ESI, EBP, EBX, EDX, ECX, EAX    // ESP is not loaded(ignored) from stack and just incremented
    {                 // Stack: clone_args, clone_ret_addr, pushad_8regs
     StkFrame[-1] = (size_t)&ThProcCallStub;      // Just return there, no args needed
     for(uint idx=2;idx <= 9;idx++)StkFrame[-idx] = (size_t)ThrFrame;     // All popped registers(including EBP) will point to the thread desc (And stack will be considered above it)
     StkFrame    -= 9;      // Number of registers to 'popd'  // 8 regs and ret addr
    }
   else
    {
     StkFrame[-1] = (size_t)&ThProcCallStub;      // Just return there, no args needed
     StkFrame    -= 1;    // Ret addr
    }
  }
 else {} // ???
 DBGMSG("StkFrame=%p, ThrFrame=%p",StkFrame,ThrFrame);
 DBGMSG("Info %p: Rec0=%p, Rec1=%p, Rec2=%p, Rec3=%p, Rec4=%p",NPTM::GetThDesc()->ThreadInfo,(vptr)NPTM::GetThDesc()->ThreadInfo->Recs[0],(vptr)NPTM::GetThDesc()->ThreadInfo->Recs[1],(vptr)NPTM::GetThDesc()->ThreadInfo->Recs[2],(vptr)NPTM::GetThDesc()->ThreadInfo->Recs[3],(vptr)NPTM::GetThDesc()->ThreadInfo->Recs[4]);

// register NTHD::SThCtx* ThFrm __asm__("5") = ThrFrame;                  // Allocate a register by index  // NOTE: no free registers on X32-X64
// __asm__ __volatile__("" : "=r"(ThFrm) : "r"(ThFrm));    // This will preserve the register
// Because the call is not inlined we have to have a return address on the new stack (X86)
 PX::pid_t pid = NAPI::clone(CloneFlg | ((uint32)GetParFromPk<5>(args...) & ~NotCloneFlg), StkFrame, (PX::PINT32)&ThrFrame->ThreadID, (PX::PINT32)&ThrFrame->ThreadID, nullptr);  // vfork     // Same stack, no copy-on-write   // pid saved to [ebp-XXXh] on X86 with O0
 if(!pid)    // In the provided stack, all required values MUST be in registers already!   // ARM32(LR), ARM64(X30), X86-X64(Stack), X86-X32(Stack)(watch out for cdecl stack release)
  {
   ThProcCallStub();   // Only on ARM it will get here  // Will try to get GETSTKFRAME inside   // X86-X32: GOT will be reread into EBX from [ebp-XXXh] (O0)
  }
 return pid;  // Not in a new thread - return an error code
}
//------------------------------------------------------------------------------------------------------------
FUNC_WRAPPERNI(NTHD::thread_sleep,      thread_sleep     )     // Sleep self  (Until timeout or a signal)      // sys_pause ?
{
 uint64 time = GetParFromPk<0>(args...);
 NTHD::SThCtx* tinf = GetThreadSelf();
 if(!tinf)return PXERR(EBADF);
 PX::timespec ts;
 PX::PTiSp tsp = nullptr;
 if(time != (uint64)-1)
  {
   // TODO: fill TS
  }
 sint32 res = NAPI::futex((uint32*)&tinf->ThreadID, PX::FUTEX_WAIT|PX::FUTEX_PRIVATE_FLAG, tinf->ThreadID, tsp);
 if(res == -PX::ENOSYS)res = NAPI::futex((uint32*)&tinf->ThreadID, PX::FUTEX_WAIT, tinf->ThreadID, tsp);
 return res;
}
//------------------------------------------------------------------------------------------------------------
// Doing this from another thread so we must find its context by its ID
//
FUNC_WRAPPERNI(NTHD::thread_wait,       thread_wait      )
{
 uint64 time = GetParFromPk<1>(args...);
 NTHD::SThCtx* tinf = GetThreadByID(GetParFromPk<0>(args...));
 if(!tinf)return PXERR(EBADF);   // i.e. the thread is already finished

 PX::timespec ts;
 PX::PTiSp tsp = nullptr;
 if(time != (uint64)-1)
  {
   // TODO: fill TS
  }
 DBGMSG("Waiting for: %u",tinf->ThreadID);
// Returns 0 if the caller was woken up. callers should always conservatively assume that a return value of 0 can mean a spurious wake-up (Why?)
 sint32 res = NAPI::futex((uint32*)&tinf->ThreadID, PX::FUTEX_WAIT, tinf->ThreadID, tsp);   // Will not work with FUTEX_PRIVATE_FLAG - infinite waiting (Why?)  // FUTEX_CLOCK_REALTIME ???
 return res;
}
//------------------------------------------------------------------------------------------------------------
/*
 Only a parent process can wait and that is not the one that spawned the thread with CLONE_THREAD
*/
FUNC_WRAPPERNI(NTHD::thread_status,   thread_status      )     // Get the thread return code (Works on finished threads with unavailable SThCtx)
{
 sint tid = GetParFromPk<0>(args...);
 NTHD::SThCtx* ThCtx = nullptr;
 NTHD::STDesc* ThDsc = NPTM::GetThDesc();
 if(tid != ThDsc->MainTh.ThreadID)
  {
   if(!ThDsc->ThreadInfo)return PXERR(ENOMEM); // No more threads
   NTHD::SThCtx** ptr = ThDsc->ThreadInfo->FindOldThreadByTID(tid);
   if(!ptr)return PXERR(ENOENT);
   ThCtx = NTHD::ReadRecPtr(ptr);
  }
   else ThCtx = &ThDsc->MainTh;
 if(!ThCtx)return PXERR(EBADF);
 DBGMSG("Status: %08X",ThCtx->ExitCode);
 return ThCtx->ExitCode;
}
//------------------------------------------------------------------------------------------------------------
FUNC_WRAPPERNI(NTHD::thread_exit,       thread_exit      )
{
 sint status = GetParFromPk<0>(args...);   // If this var is on stack, the stack may become deallocated (probably - Even marked records should be checked for zero TID)
 NTHD::SThCtx* tinf = GetThreadSelf();
 if(tinf && tinf->SelfPtr)
  {
   tinf->LastThrdID = tinf->ThreadID;
   tinf->ExitCode   = status;
   NTHD::ReleaseRec((NTHD::SThCtx**)tinf->SelfPtr);
 }
 return NAPI::exit(status);
}
//------------------------------------------------------------------------------------------------------------
};
//============================================================================================================

#include "../../NixUtils.hpp"
#include "Startup.hpp"

//============================================================================================================
struct SFWCTX      // NOTE: Such alignment may waste some memory on main thread // Initialize should be called from each thread?    // CLONE_SETTLS should set to this somehow (Allows to avoid of separate TLS allocation)  // struct alignas(MEMPAGESIZE) SFWCTX - No need for now to store main thread ctx on the stack
{
 // Some thread context data here (to be stored on stack)

static sint Initialize(void* StkFrame=nullptr, void* ArgA=nullptr, void* ArgB=nullptr, void* ArgC=nullptr, bool InitConLog=false)    // _finline ?
{
 if(IsInitialized())return 1;
 if(!NLOG::CurrLog)NLOG::CurrLog = &NLOG::GLog;  // Will be set with correct address, relative to the Base
 if(InitConLog)   // On this stage file logging is not possible yet (needs InitStartupInfo)
  {
   NPTM::NLOG::GLog.LogModes   = NPTM::NLOG::lmCons;
   NPTM::NLOG::GLog.ConsHandle = NPTM::GetStdErr();
  }
 // NOTE: Init syscalls before InitStartupInfo if required
 InitStartupInfo(StkFrame, ArgA, ArgB, ArgC);
 IFDBG{DbgLogStartupInfo();}
 if(NTHD::SThCtx* MainTh=&NPTM::GetThDesc()->MainTh; !MainTh->Self)
  {
   MainTh->Self       = MainTh;     // For checks
   MainTh->SelfPtr    = nullptr;    // Not owned
   MainTh->TlsBase    = nullptr;    // Allocate somewhere on demand?
   MainTh->TlsSize    = 0;
   MainTh->StkBase    = nullptr;    // Get from ELF header or proc/mem ???
   MainTh->StkSize    = 0;          // StkSize; ??? // Need full size for unmap  // Can a thread unmap its own stack before calling 'exit'?
   MainTh->GroupID    = NAPI::getpgrp();   // pid
   MainTh->ThreadID   = NAPI::gettid();
   MainTh->ProcesssID = NAPI::getpid();
   MainTh->ThreadProc = nullptr;    // Get it from ELF or set from arg?
   MainTh->ThreadData = nullptr;
   MainTh->ThDataSize = 0;
   MainTh->Flags      = 0;    // ???
  }
 return 0;
}

};
//============================================================================================================
