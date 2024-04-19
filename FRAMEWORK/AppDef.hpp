
#pragma once

// TODO: Probably some generic flags

// This class is expected to be on stack in an entry point proc or in dynamic memory
class CApplication   // Interface
{
public:
 sint Initialize(vptr ArgA=nullptr, vptr ArgB=nullptr, vptr ArgC=nullptr){UNUSED(ArgA); UNUSED(ArgB); UNUSED(ArgC); return 0;}   // Accepts some startup arguments
 sint Notify(vptr ArgA=nullptr, vptr ArgB=nullptr, vptr ArgC=nullptr){UNUSED(ArgA); UNUSED(ArgB); UNUSED(ArgC); return 0;}       // Used for some system callbacks
 sint Execute(void);   // Mandatory: Application`s entry point
 sint Finalize(void){return 0;}
//------------------------------------------------------------------------------------------------------------
//CApplication(void) {}  // Avoid constructors - CApplication instances are managed manually
//------------------------------------------------------------------------------------------------------------

};
