
// Members: void (Observer::*)(const T&...)

// NOTE: Those functions may do nothing but must be implemented to make apps compilable for all supported platforms

//
// Startup helper functions
//
INTF_VALIDATE(GetStdIn,   sint32,  void)
INTF_VALIDATE(GetStdOut,  sint32,  void)
INTF_VALIDATE(GetStdErr,  sint32,  void)

INTF_VALIDATE(GetTZOffsUTC,   sint32,  void)
INTF_VALIDATE(IsLoadedByLdr,  bool,    void)
INTF_VALIDATE(IsDynamicLib,   bool,    void)

INTF_VALIDATE(GetCLArg,            sint,    sint&, achar*, uint)
INTF_VALIDATE(GetCLArg,   const syschar*,    sint&, uint*)

INTF_VALIDATE(GetEnvVar,   sint,    sint&, achar*, uint)
INTF_VALIDATE(GetEnvVar,   sint,    const achar*, achar*, uint)
INTF_VALIDATE(GetEnvVar,   const syschar*,    sint&, uint*)
INTF_VALIDATE(GetEnvVar,   const syschar*,    const achar*, uint*)

INTF_VALIDATE(GetAuxInfo,   sint,    uint, vptr, size_t)

INTF_VALIDATE(GetModuleBase,   vptr,    void)
INTF_VALIDATE(GetModuleSize,   size_t,  void)
INTF_VALIDATE(GetModulePath,   sint,    achar*, size_t)

INTF_VALIDATE(GetThreadSelf,   NTHD::SThCtx*,    void)
INTF_VALIDATE(GetThreadByID,   NTHD::SThCtx*,    uint)
INTF_VALIDATE(GetThreadByAddr, NTHD::SThCtx*,    vptr)
//INTF_VALIDATE(GetNextThread,   NTHD::SThCtx*,    ???)







// How to validate NAPI with all those templates and argument forwarding? // TODO: Get rid of the templates and just copy declarations from PX
