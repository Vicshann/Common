
#pragma once

// TODO: Probably some generic flags

// This class is expected to be on stack in entry point proc or in dynamic memory
class CApplication   // Interface
{
public:
 sint Initialize(vptr ArgA=0, vptr ArgB=0, vptr ArgC=0){return 0;}   // Accepts some startup arguments
 sint Notify(vptr ArgA=0, vptr ArgB=0, vptr ArgC=0){return 0;}       // Used for some system callbacks
 sint Execute(void);   // Mandatory: Application`s entry point
 sint Finalize(void){return 0;}
//------------------------------------------------------------------------------------------------------------
//CApplication(void) {}  // Avoid constructors - CApplication instances are managed manually
//------------------------------------------------------------------------------------------------------------

};
