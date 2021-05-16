
#pragma once

// TODO: Probably some generic flags

class CApplication
{
public:
 virtual sint Initialize(uint ArgA=0, uint ArgB=0, uint ArgC=0) {return 0;}   // Accepts some startup arguments
 virtual sint Notify(uint ArgA=0, uint ArgB=0, uint ArgC=0) {return 0;}     // Used for some system callbacks
 virtual sint Execute(void) = 0;   // Application`s entry point
 virtual sint Finalize(void) {return 0;}
};