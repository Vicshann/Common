
#ifndef StrAllocH
#define StrAllocH

#pragma once

//
class CStrAlloc
{
 MemUtil::SAllocHL<> Allo;

 UINT8* StrAlloc(UINT Size){return 0;};
 void   StrFree(UINT8* Data){free(Data);};
};
//---------------------------------------------------------------------------
#endif
