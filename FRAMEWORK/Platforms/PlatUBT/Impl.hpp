
#pragma once

#include "./PlatDef.hpp"
#include "./uboot.hpp"

#define UBTMSG(msg,...) NPTM::SAPI::printf(msg "\n" __VA_OPT__(,) __VA_ARGS__)        //  __func__ "->" msg "\n"

#if defined(DBGBUILD) || defined(FCFG_FORCE_DBGMSG)
#define UBTDBG UBTMSG
#endif
//============================================================================================================
// All "members" are placed sequentially in memory but their order may change
// NOTE: Do not expect that the memory can be executable and writable at the same time! There is 'maxprot' values may be defined in EXE header which will limit that or it may be limited by the system
// It MUST be Namespace, not Struct to keep these in code segment (MSVC linker, even with Clang compiler) // FIXED: declared as _codesec SysApi
// NOTE: Unreferenced members will be optimized out!
// NOTE: Do not use these directly, not all of them are portable even between x32/x64 on the same cpu
//
struct SAPI  // POSIX API implementation  // https://docs.oracle.com/cd/E19048-01/chorus4/806-3328/6jcg1bm05/index.html
{
DECL_SYSCALL(0,       NUBOOT::get_version,     get_version    )
DECL_SYSCALL(0,       NUBOOT::getc,            getc           )
DECL_SYSCALL(0,       NUBOOT::tstc,            tstc           )
DECL_SYSCALL(0,       NUBOOT::putc,            putc           )
DECL_SYSCALL(0,       NUBOOT::puts,            puts           )
DECL_SYSCALLVA(0,     NUBOOT::printfNV,        printf         )
DECL_SYSCALL(0,       NUBOOT::install_hdlr,    install_hdlr   )
DECL_SYSCALL(0,       NUBOOT::free_hdlr,       free_hdlr      )
DECL_SYSCALL(0,       NUBOOT::malloc,          malloc         )
DECL_SYSCALL(0,       NUBOOT::free,            free           )
DECL_SYSCALL(0,       NUBOOT::udelay,          udelay         )
DECL_SYSCALL(0,       NUBOOT::get_timer,       get_timer      )
DECL_SYSCALL(0,       NUBOOT::vprintf,         vprintf        )
DECL_SYSCALL(0,       NUBOOT::do_reset,        do_reset       )
DECL_SYSCALL(0,       NUBOOT::getenv,          getenv         )
DECL_SYSCALL(0,       NUBOOT::setenv,          setenv         )
DECL_SYSCALL(0,       NUBOOT::simple_strtoul,  simple_strtoul )
DECL_SYSCALL(0,       NUBOOT::strict_strtoul,  strict_strtoul )
DECL_SYSCALL(0,       NUBOOT::simple_strtol,   simple_strtol  )
DECL_SYSCALL(0,       NUBOOT::strcmp,          strcmp         )
DECL_SYSCALL(0,       NUBOOT::i2c_write,       i2c_write      )
DECL_SYSCALL(0,       NUBOOT::i2c_read,        i2c_read       )

DECL_SYSCALL(0,       NUBOOT::ExecCmdLine,     ExecCmdLine    )

} static constexpr inline SysApi alignas(16);   // Declared to know exact address(?), its size is ALWAYS 1
//============================================================================================================
// NOTE: Do not rely on this in any way! It is implemented mostly for the compile consistency
struct NAPI
{
FUNC_WRAPPERFI(PX::exit,       exit       ) {return;}  // Just returns to loader
FUNC_WRAPPERFI(PX::exit_group, exit_group ) {return 0;}  // Just returns to loader
FUNC_WRAPPERFI(PX::open,       open       ) {return 0;}
FUNC_WRAPPERFI(PX::read,       read       ) {return 0;}
FUNC_WRAPPERFI(PX::write,      write      ) { SAPI::puts((achar*)GetParFromPk<1>(args...)); return 0;}  // Only for console output to work
FUNC_WRAPPERFI(PX::flock,      flock      ) {return 0;}
FUNC_WRAPPERFI(PX::lseek,      lseek      ) {return 0;}
FUNC_WRAPPERFI(PX::close,      close      ) {return 0;}
FUNC_WRAPPERFI(PX::access,     access     ) {return 0;}
FUNC_WRAPPERFI(PX::mkdir,      mkdir      ) {return 0;}

FUNC_WRAPPERFI(PX::mmapGD,     mmap       ) { return SAPI::malloc(GetParFromPk<1>(args...)); }     // Very approximate!
FUNC_WRAPPERFI(PX::munmap,     munmap     ) { SAPI::free(GetParFromPk<0>(args...)); return 0; }
FUNC_WRAPPERFI(PX::mprotect,   mprotect   ) {return 0;}
FUNC_WRAPPERFI(PX::msync,      msync      ) {return 0;}

FUNC_WRAPPERFI(PX::getpgrp,    getpgrp    ) {return 0;}
FUNC_WRAPPERFI(PX::getpid,     getpid     ) {return 0;} 
//---------------------------------------------------------------------------

};
//============================================================================================================

#include "Startup.hpp"
#ifdef CPU_ARM
#include "HookSMC.hpp"
#endif
//============================================================================================================
struct SFWCTX
{
 // Some thread context data here (to be stored on stack)

static sint Initialize(void* StkFrame=nullptr, void* ArgA=nullptr, void* ArgB=nullptr, void* ArgC=nullptr, uint InitConLog=0)    // _finline ?
{
//#if defined(ARCH_X32) && defined(FWK_OLD_UBOOT)   // Not here, in EntryPoint only
// if(InitConLog > 1)SaveOldCtx();
//#endif
// if(IsInitialized())return 1;
 if(!NLOG::CurrLog)NLOG::CurrLog = &NLOG::GLog;  // Will be set with correct address, relative to the Base
 if(InitConLog)   // On this stage file logging is not possible yet (needs InitStartupInfo)
  {
   NPTM::NLOG::GLog.LogModes   = NPTM::NLOG::lmCons;
   NPTM::NLOG::GLog.ConsHandle = NPTM::GetStdErr();
  }
 // NOTE: Init syscalls before InitStartupInfo if required
 sint res = InitStartupInfo(StkFrame, ArgA, ArgB, ArgC);
 IFDBG{DbgLogStartupInfo();}
 return res;
}

};
//============================================================================================================
