
#pragma once

// TODO: Probably some generic flags

class CApplication   // Interface
{
public:
 sint Initialize(uint ArgA=0, uint ArgB=0, uint ArgC=0){return 0;}   // Accepts some startup arguments
 sint Notify(uint ArgA=0, uint ArgB=0, uint ArgC=0){return 0;}     // Used for some system callbacks
 sint Execute(void);   // Mandatory: Application`s entry point
 sint Finalize(void){return 0;}
//------------------------------------------------------------------------------------------------------------
CApplication(void)
{
 NPTFM::Initialize();

}
//------------------------------------------------------------------------------------------------------------

};