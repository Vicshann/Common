
#pragma once

// Definitions for AppMain.cpp
//------------------------------------------------------------------------------------------------------------
#ifdef _APPENTRYPT    // Defined in AppMain.cpp if AppMain.hpp is present in a project that uses the Framework

// For application`s main executables (Class entry point)
//  __attribute__ ((section ("entry")))         // TODO: by cfg

_SYSENTRY MODERN_INIT sint _scall _ModStart(vptr ArgA, vptr ArgB, vptr ArgC) noexcept  // On Windows ArgA is PEB ptr
{
 NPTM::SFWCTX ctx;
 if(ctx.Initialize(GETSTKFRAME(), ArgA, ArgB, ArgC, NCFG::InitCon))return 0;  // The entry point has ben called already
 DBGMSG("Framework initialized");
 CAppMain app;           // Included somewhere above
 sint stat = -10000;
 if(app.Initialize(ArgA, ArgB, ArgC) >= 0)
  {
   DBGMSG("Application initialized");
   stat = app.Execute();
   DBGMSG("App execution completed");
   app.Finalize();
  }
 DBGMSG("Exiting");
 if(!NPTM::IsDynamicLib() && !NPTM::IsLoadedByLdr())NPTM::NAPI::exit_group((int)stat);  // Never do this for DLLs, only for main executable?
 return stat;
}
//--------------------------
/*_SYSENTRY MODERN_FINI sint _scall _ModExit(void) noexcept     // Dynamic libraries only
{
 return 0;   // Anything else?
}*/
//------------------------------------------------------------------------------------------------------------
#else
sint _scall ModuleMain(vptr ArgA, vptr ArgB, vptr ArgC);  // Forward declare

_SYSENTRY MODERN_INIT sint _scall _ModStart(vptr ArgA, vptr ArgB, vptr ArgC) noexcept  // On Windows ArgA is PEB ptr
{
 NPTM::SFWCTX ctx;
 if(ctx.Initialize(GETSTKFRAME(), ArgA, ArgB, ArgC, NCFG::InitCon))return 0; // The entry point has ben called already
 DBGMSG("Framework initialized");
 sint stat = ModuleMain(ArgA, ArgB, ArgC); // Call 'Main' which should be somewhere above
 DBGMSG("Exiting");
 if(!NPTM::IsDynamicLib() && !NPTM::IsLoadedByLdr())NPTM::NAPI::exit_group((int)stat);  // Never do this for DLLs, only for main executable?
 return stat;
}
//--------------------------
/*_SYSENTRY MODERN_FINI sint _scall _ModExit(void) noexcept     // Dynamic libraries only
{
 return 0;   // Anything else?
}*/
#endif
//------------------------------------------------------------------------------------------------------------
// Thread entry point
//------------------------------------------------------------------------------------------------------------
/* NOTES:
Predefined names on Linux:
  extern "C" void _init(void){LOGMSG("Init");}  // DT_INIT: init (argc, argv, env);
  extern "C" void _fini(void){LOGMSG("Fini");}  // DT_FINI

Looks like there is no other way to force the compiler to add DT_INIT in .dynamic section which is required by ld.so to call the function
Sections does not matter, records in DYNAMIC table do.
-Wl,-init,init -Wl,-fini,fini
// Make sure that the linker adds DT_INIT in .dynamic for this function  // It does not have to be in a separate section

// https://maskray.me/blog/2021-11-07-init-ctors-init-array
// https://refspecs.linuxbase.org/elf/gabi4+/ch5.dynamic.html
// https://cnlelema.github.io/memo/en/codegen/basic-elf/loading/init/
// Dynamic tags only (.dynamic) (Programs,sections?)
// NOTE: Pointers in SHT_INIT_ARRAY depend on RELA relocs
// NOTE: Value of DT_INIT is added to LoadBase so no relocs needed
// DT_INIT: This element holds the address of the initialization function, discussed in ``Initialization and Termination Functions'' below.
// DT_FINI: This element holds the address of the termination function, discussed in ``Initialization and Termination Functions'' below.

// DT_INIT_ARRAY: This element holds the address of the array of pointers to initialization functions, discussed in ``Initialization and Termination Functions'' below.
// DT_FINI_ARRAY: This element holds the address of the array of pointers to termination functions, discussed in ``Initialization and Termination Functions'' below.
// DT_INIT_ARRAYSZ: This element holds the size in bytes of the array of initialization functions pointed to by the DT_INIT_ARRAY entry. If an object has a DT_INIT_ARRAY entry, it must also have a DT_INIT_ARRAYSZ entry.
// DT_FINI_ARRAYSZ: This element holds the size in bytes of the array of termination functions pointed to by the DT_FINI_ARRAY entry. If an object has a DT_FINI_ARRAY entry, it must also have a DT_FINI_ARRAYSZ entry.

// https://chromium.googlesource.com/native_client/nacl-newlib/+/65e6baefeb2874011001c2f843cf3083e771b62f/newlib/libc/sys/linux/dl/dl-init.c

*/
