
#pragma once

//------------------------------------------------------------------------------------------------------------
#ifdef _APPENTRYPT    // Defined in AppMain.cpp if AppMain.hpp is present
	
// For application`s main executables (Class entry point)
//  __attribute__ ((section ("entry")))         // TODO: by cfg

_SYSENTRY sint _scall AppEntryPoint(vptr ArgA, vptr ArgB, vptr ArgC) noexcept  // On Windows ArgA is PEB ptr
{
 NPTM::SFWCTX ctx;
 ctx.Initialize(GETSTKFRAME(), ArgA, ArgB, ArgC, NCFG::InitCon);
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
//------------------------------------------------------------------------------------------------------------
#else
// For dynamically loaded modules (Custom 'ModuleMain')
//extern sint ModuleMain(vptr ArgA, vptr ArgB, vptr ArgC);

_SYSENTRY sint _scall ModEntryPoint(vptr ArgA, vptr ArgB, vptr ArgC) noexcept  // On Windows ArgA is PEB ptr
{
 NPTM::SFWCTX ctx;
 ctx.Initialize(GETSTKFRAME(), ArgA, ArgB, ArgC, NCFG::InitCon);
 DBGMSG("Framework initialized");
 sint stat = ModuleMain(ArgA, ArgB, ArgC); // Call 'Main' which should be somewhere above
 DBGMSG("Exiting");
 if(!NPTM::IsDynamicLib() && !NPTM::IsLoadedByLdr())NPTM::NAPI::exit_group((int)stat);  // Never do this for DLLs, only for main executable?
 return stat;
}	
#endif
//------------------------------------------------------------------------------------------------------------
// Thread entry point
//------------------------------------------------------------------------------------------------------------

