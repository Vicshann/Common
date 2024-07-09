
#ifndef StrUtf8H
#define StrUtf8H

#pragma once

// UTF8 is simple amall and fast :)
// Inplace storing(The strings will be stuffed together in a memory buffer)
// Size is limited to 65535 Bytes/Chars
// Cannot use global operator 'new' because it will lead to a heap fragmentation
//
class CU8Str
{
struct SDesc
{
 UINT32 RefCtr;    // Even 32 bits can be not enough (Long listing of small repeating text fragments stored in StrCache as one CU8Str)
 UINT16 ByteCnt;   // Number of bytes of UTF-8 encoding
 UINT16 CharCnt;   // Actual number of characters to display
 UINT8  Data[1];   // Zero terminated and at least 1 char in size
}*Descr;


public:
//-----------------------------------




void Initialize(PBYTE Str)  // LPSTR or PWSTR will be detected
{

}
//-----------------------------------
static CU8Str* Create(LPSTR Str)
{

}
//-----------------------------------
void AssignFrom(CU8Str* str)   // Steals the Pointer
{

}
//---------------------------------------------------------------------------
static UINT _fastcall CalcStrSize(char* Str)
{
 return 0;
}
//---------------------------------------------------------------------------
static UINT _fastcall CalcStrSize(wchar_t* Str)
{
 return 0;
}
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------

};
//---------------------------------------------------------------------------
#endif
