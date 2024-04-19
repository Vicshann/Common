
// Members: void (Observer::*)(const T&...)

// NOTE: Those functions may do nothing but must be implemented to make apps compilable for all supported platforms

//
// Startup helper functions
//
API_VALIDATE(GetStdIn,   PX::fdsc_t,  void)
API_VALIDATE(GetStdOut,  PX::fdsc_t,  void)
API_VALIDATE(GetStdErr,  PX::fdsc_t,  void)

API_VALIDATE(GetTZOffsUTC,   sint32,  void)
API_VALIDATE(IsLoadedByLdr,  bool,    void)
API_VALIDATE(IsDynamicLib,   bool,    void)

API_VALIDATE(CheckCLArg,            bool,    sint&, const achar*, uint32)
API_VALIDATE(GetCLArg,              sint,    sint&, achar*, uint)
API_VALIDATE(SkipCLArg,   const syschar*,    sint&, uint*)

API_VALIDATE(GetEnvVar,   sint,    sint&, achar*, uint)
API_VALIDATE(GetEnvVar,   sint,    const achar*, achar*, uint)
API_VALIDATE(GetEnvVar,   const syschar*,    sint&, uint*)
API_VALIDATE(GetEnvVar,   const syschar*,    const achar*, uint*)

API_VALIDATE(GetAuxInfo,   sint,    uint, vptr, size_t)

API_VALIDATE(GetModuleBase,   vptr,    void)
API_VALIDATE(GetModuleSize,   size_t,  void)
API_VALIDATE(GetModulePath,   sint,    achar*, size_t)

API_VALIDATE(GetThreadSelf,   NTHD::SThCtx*,    void)
API_VALIDATE(GetThreadByID,   NTHD::SThCtx*,    uint)
API_VALIDATE(GetThreadByAddr, NTHD::SThCtx*,    vptr)
//API_VALIDATE(GetNextThread,   NTHD::SThCtx*,    ???)

API_VALIDATE(InitStartupInfo,   sint,  vptr, vptr, vptr, vptr)





// How to validate NAPI with all those templates and argument forwarding? // TODO: Get rid of the templates and just copy declarations from PX
