
#ifndef FactoryH
#define FactoryH

#pragma once

// NOTE: Must be thread safe
// Optimal is Low Level allocator only
//---------------------------------------------------------------------------
template<typename T> class CFactory
{
struct SItem
{
 T Value;
 UINT16 RefCtr;
};

public:
//-------------------------------
CU8Str* AddStr(LPSTR str)
{
 return NULL;
}
//-------------------------------
CU8Str* AddStr(PWSTR str)
{
 return NULL;
}
//-------------------------------
void    DelStr(CU8Str* str)
{
 return NULL;
}
//-------------------------------
};
//---------------------------------------------------------------------------
#endif
