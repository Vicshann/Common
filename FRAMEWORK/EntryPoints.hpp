
#pragma once

//------------------------------------------------------------------------------------------------------------
#ifdef _APPENTRYPT
//  __attribute__ ((section ("entry")))         // TODO: by cfg
_SYSENTRY sint _scall AppEntryPoint(vptr ArgA, vptr ArgB, vptr ArgC) noexcept  // On Windows ArgA is PEB ptr
{
 NPTM::SFWCTX ctx;
 ctx.Initialize(GETSTKFRAME(), ArgA, ArgB, ArgC, NCFG::InitCon);
 CAppMain app;           // Included somewhere above
 sint stat = -10000;
 if(app.Initialize(ArgA, ArgB, ArgC) >= 0)
  {
   stat = app.Execute();
   app.Finalize();
  }
 if(!NPTM::IsDynamicLib() && !NPTM::IsLoadedByLdr())NPTM::NAPI::exit_group((int)stat);  // Never do this for DLLs, only for main executable?
 return stat;
}
//------------------------------------------------------------------------------------------------------------
#else
//extern sint ModuleMain(vptr ArgA, vptr ArgB, vptr ArgC);

_SYSENTRY sint _scall ModEntryPoint(vptr ArgA, vptr ArgB, vptr ArgC) noexcept  // On Windows ArgA is PEB ptr
{
 NPTM::SFWCTX ctx;
 ctx.Initialize(GETSTKFRAME(), ArgA, ArgB, ArgC, InitCon);  
 sint stat = ModuleMain(ArgA, ArgB, ArgC); // Call 'Main' which should be somewhere above
 if(!NPTM::IsDynamicLib() && !NPTM::IsLoadedByLdr())NPTM::NAPI::exit_group((int)stat);  // Never do this for DLLs, only for main executable?
 return stat;
}
#endif
//------------------------------------------------------------------------------------------------------------
// Thread entry point
//------------------------------------------------------------------------------------------------------------

