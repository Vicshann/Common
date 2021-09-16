
#pragma once

//#include "Common.hpp"

struct NPTFM
{
// Anything that may have a different set of system API is put here
#ifdef PLT_EFI

#elif  PLT_WEBASM

#elif  PLT_NIX_USR

#elif  PLT_NIX_KRN

#elif  PLT_MAC_USR

#elif  PLT_MAC_KRN

#elif  PLT_WIN_USR
#include "PlatWIN/PlatWin.h"
#include "PlatWIN/User/Impl.hpp"
#elif  PLT_WIN_KRN

#endif








//static uint32 MyProc(uint val){return val+1;}

/*extern "C"        // GetPageSize
{
PVOID _fastcall AllocMemLL(PVOID Mem, SIZE_T Size, SIZE_T AllocSize, SIZE_T ReserveSize=0, SIZE_T Align=MEMPAGESIZE);
bool  _fastcall FreeMemLL(PVOID Mem, SIZE_T Size=0);

PVOID _fastcall AllocMemHL(PVOID Mem, SIZE_T Size, SIZE_T AllocSize, SIZE_T ReserveSize=0, SIZE_T Align=MEMPAGESIZE);    // EMemAlign is not supported!
bool  _fastcall FreeMemHL(PVOID Mem, SIZE_T Size=0);
}*/
};
//------------------------------------------------------------------------------------------------------------

