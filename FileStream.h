
#pragma once

/*
  Copyright (c) 2018 Victor Sheinmann, Vicshann@gmail.com

  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), 
  to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
  and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. 
*/

#ifndef FileStrH
#define FileStrH

#include <Windows.h>
#include <stdlib.h>
#include "MiniString.h"
//===========================================================================================================
class CFileStr
{
 HANDLE hFile;
 UINT64 SLength;

//----------------------
 void Initialize(void)
 {
  this->hFile   = INVALID_HANDLE_VALUE;
  this->SLength = 0;
 }
//----------------------
public:
 void sAssign(const CMiniStr &str)
 {
  this->cAssign(str.c_str(), str.Length());
 }
//----------------------
 void cAssign(const char* str, int Len)
 {
  this->Clear();
  this->cAppend(str, Len);
 }
//----------------------
 void iAssign(const int val)
 {
  this->Clear();
  this->iAppend(val);
 }
//----------------------
 void iAssign(const unsigned long val)
 {
  this->Clear();
  this->iAppend(val);
 }
//----------------------
 void sAppend(const CMiniStr &str)
 {
  this->cAppend(str.c_str(), str.Length());
 }
//----------------------
 void cAppend(const char* str, UINT Len)
 {
  DWORD Result;
  WriteFile(this->hFile,str,Len,&Result,NULL);
  this->SLength += Result;
 }
//----------------------
 void iAppend(const int val)
 {
  UINT len = 0;
  char Tmpb[64];
  char* res = DecNumToStrS(val, Tmpb, &len);
  this->cAppend(res,len);
 }
//----------------------
 LPSTR iAppend(const unsigned long val)
 {
  int  len = 0;
  char Tmpb[64];
  DecNumToStrU(val, Tmpb, &len);
  this->cAppend((LPSTR)&Tmpb,len);
 }
//----------------------
public:

//----------------------   // Instead of lambdas
CFileStr(void){this->Initialize();}
//----------------------
 ~CFileStr()
 {
  if(this->hFile != INVALID_HANDLE_VALUE)CloseHandle(this->hFile);
 }
//----------------------
 void  Clear(void)
 {
  SetFilePointer(this->hFile,0,NULL,FILE_BEGIN);
  SetEndOfFile(this->hFile);
  this->SLength = 0;
 }
//----------------------
 UINT  Length(void) const {return this->SLength;}
//----------------------
 void  operator = (const CMiniStr &str){this->sAssign(str);}
//----------------------
// void  operator = (const LPSTR str){this->cAssign(str,lstrlen(str));}
//----------------------
 CFileStr& operator += (const CMiniStr &str){this->sAppend(str);return *this;}
//----------------------
// CFileStr& operator += (const LPSTR str){this->cAppend(str,lstrlen(str));return *this;}
//----------------------
 CFileStr& operator += (int val){this->iAppend(val);return *this;}
//----------------------
 CFileStr& operator += (unsigned long val){this->iAppend(val);return *this;}
//----------------------
const CFileStr& AddChars(char val, int Count=1)     // return CONST?
{
 BYTE sadd[256];       // A little buffer :)
 int  left = (Count > sizeof(sadd))?(sizeof(sadd)):(Count); 
 memset(&sadd,val,left);
 for(;Count > 0;Count-=left)
  {
   left = (Count > sizeof(sadd))?(sizeof(sadd)):(Count); 
   this->cAppend((LPSTR)&sadd,left);  // Do not check if 'sadd' is empty?
  }
 return *this;
}
//----------------------
 bool SetFile(PVOID FileName)
  {
   if(!((PBYTE)FileName)[1])this->hFile = CreateFileW((PWSTR)FileName,GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL|FILE_FLAG_SEQUENTIAL_SCAN,NULL);
     else this->hFile = CreateFileA((LPSTR)FileName,GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL|FILE_FLAG_SEQUENTIAL_SCAN,NULL);
   if(this->hFile == INVALID_HANDLE_VALUE)return false;
   return true;
  }

};
//-----------------------------------------------------------------------------


//===========================================================================================================
#endif
