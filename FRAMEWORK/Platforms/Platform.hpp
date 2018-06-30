
#pragma once

#ifndef _PLATFORM_
#define _PLATFORM_

#include "Platforms\Windows\PlatWin.h" 

namespace NPTM
{
PVOID _fastcall AllocMemLL(SIZE_T AllocSize, PVOID Mem=0, SIZE_T ReserveSize=0, SIZE_T Align=MEMPAGESIZE);
void  _fastcall FreeMemLL(PVOID Mem, SIZE_T Size=0);

PVOID _fastcall AllocMemHL(SIZE_T AllocSize, PVOID Mem=0, SIZE_T ReserveSize=0, SIZE_T Align=MEMPAGESIZE);    // EMemAlign is not supported!
void  _fastcall FreeMemHL(PVOID Mem, SIZE_T Size=0);
}
//---------------------------------------------------------------------------
#endif
