
#pragma once

#ifndef MiniStrH
#define MiniStrH
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


//#include <stdlib.h>
//===========================================================================================================
//extern "C" char* __cdecl _ultoa(unsigned long __value, char * __string, int __radix);

//extern DWORD _stdcall DecStrToDW(LPSTR String, UINT* Len);
//extern DWORD _stdcall HexStrToDW(LPSTR String, UINT Bytes);
//extern LPSTR _stdcall ConvertToDecDW(DWORD Value, LPSTR Number);
//extern LPSTR _stdcall ConvertToHexStr(UINT64 Value, int MaxDigits, LPSTR Number, bool UpCase);

class CMiniStr
{
 LPSTR  Data;
 UINT   SLength;
 //UINT   Allocated;


//----------------------
void movemem(PBYTE Dst, PBYTE Src, UINT Len) // C++Builder`s function is broken!!!
{
 if(Dst > Src)
  {
   Dst += Len; Src += Len;
   for(int ctr = Len/sizeof(long);ctr;ctr--)
	{
	 Dst -= sizeof(long);
	 Src -= sizeof(long);
	 *((long*)Dst) = *((long*)Src);
	}
   for(int ctr=Len%sizeof(long);ctr;ctr--){Dst--; Src--;*Dst = *Src;}
  }
   else
	{
	 for(int ctr=Len/sizeof(long);ctr;ctr--,Dst+=sizeof(long),Src+=sizeof(long))*Dst = *Src;
	 for(int ctr=Len%sizeof(long);ctr;ctr--,Dst++,Src++)*Dst = *Src;
	}
}
//----------------------
 LPSTR ResizeFor(UINT Len)    // Do not updates SLength
 {
  if(Len && this->Data)this->Data = (LPSTR)HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,this->Data,Len+8);
	else if(!this->Data)this->Data = (LPSTR)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,Len+8);
	  else if(!Len && this->Data){HeapFree(GetProcessHeap(),0,this->Data);this->Data=NULL;}

  /*if(Len >= this->Allocated)
   {
	this->Allocated = Len + ((this->Allocated)?(this->Allocated):(256));  // Reserve some space, initial prealloc 256
	if(!this->Data)this->Data = (LPSTR)malloc(this->Allocated);
	  else this->Data = (LPSTR)realloc(this->Data,this->Allocated);
	if(!this->Data)this->Allocated = 0;
   }
	else if(!Len){free(this->Data);this->Data=NULL;}*/
  return this->Data;
 }
//----------------------
 void Initialize(void)
 {
  this->Data      = NULL;
  this->SLength   = 0;
  //this->Allocated = 0;
 }
//----------------------
public:
 LPSTR sAssign(const CMiniStr &str)
 {
  if(!this->ResizeFor(str.Length())){this->Clear();return NULL;}  // +1 For a Zero Term char
  memcpy(this->Data,str.Data,str.SLength); 	  //strcpy(this->Data,str.c_str());
  this->SLength = str.SLength;
  this->Data[this->SLength] = 0;
  return this->Data;
 }
//----------------------
 LPSTR cAssign(const wchar_t* str, int Len=0)
 {
  if(!str/* || !str[0]*/){this->Clear();return NULL;}   // str[0] - Allow for: str = "";
  if(!Len)Len = lstrlenW(str);      // Will compiler inline this function and remove this condition if it known true (when default used)?
  if(!this->ResizeFor(Len)){this->Clear();return NULL;}  // +1 For a Zero Term char
  int ulen = WideCharToMultiByte(CP_ACP,0,str,Len,(LPSTR)this->Data,Len,NULL,NULL);
  if(ulen < Len)
   {
	Len = ulen;
	this->ResizeFor(Len);
	if(!this->Data){this->SLength = 0; return NULL;}
   }
  this->SLength = Len;
  this->Data[this->SLength] = 0;
  return this->Data;
 }
//----------------------
 LPSTR cAssign(const char* str, int Len=0)
 {
  if(!str/* || !str[0]*/){this->Clear();return NULL;}   // str[0] - Allow for: str = "";
  if(!Len)Len = lstrlen(str);      // Will compiler inline this function and remove this condition if it known true (when default used)?
  if(!this->ResizeFor(Len)){this->Clear();return NULL;}  // +1 For a Zero Term char
  memcpy(this->Data,str,Len); 	              // strcpy(this->Data,str);
  this->SLength = Len;
  this->Data[this->SLength] = 0;
  return this->Data;
 }
//----------------------
 LPSTR iAssign(const int val)
 {
  char Tmpb[64];		
 // _ltoa(val,(LPSTR)&Tmpb[0],10);
  return this->cAssign(DecNumToStrS(val, (LPSTR)&Tmpb));  // No signs //this->cAssign((LPSTR)&Tmpb[0]);
 }
//----------------------
 LPSTR sAppend(const CMiniStr &str)
 {
  if(!this->ResizeFor(str.Length()+this->SLength))return NULL;
  memcpy(&this->Data[this->SLength],str.Data,str.SLength);     //  strcpy(&this->Data[this->SLength],str.c_str());
  this->SLength += str.SLength;
  this->Data[this->SLength] = 0;
  return this->Data;
 }
//----------------------
 LPSTR cAppend(const wchar_t* str, int Len=0)
 {
  if(!str /*|| !str[0]*/)return NULL; // str[0] - Allow for: str = "";
  if(!Len)Len = lstrlenW(str);      // Will compiler inline this function and remove this condition if it known true (when default used)?
  if(!this->ResizeFor(Len+this->SLength))return NULL;
  int ulen = WideCharToMultiByte(CP_ACP,0,str,Len,(LPSTR)&this->Data[this->SLength],Len,NULL,NULL);
  if(ulen < Len)
   {
	Len = ulen;
	this->ResizeFor(Len+this->SLength);
	if(!this->Data){this->SLength = 0; return NULL;}
   }
  this->SLength += Len;
  this->Data[this->SLength] = 0;
  return this->Data;
 }
//----------------------
 LPSTR cAppend(const char* str, int Len=0)
 {
  if(!Len && str)Len = lstrlen(str);      // Will compiler inline this function and remove this condition if it known true (when default used)?
  if(!Len)return this->Data; 
  if(!this->ResizeFor(Len+this->SLength))return NULL;
  LPSTR Dst = &this->Data[this->SLength];
  if(str)memcpy(Dst,str,Len);    //       strcpy(&this->Data[this->SLength],str);
  this->SLength += Len;
  this->Data[this->SLength] = 0;
  return Dst;    // Returns ptr to beginning of an appended string
 }
//----------------------

 LPSTR iAppend(const int val)
 {
  BYTE Tmpb[64];
 // _ltoa(val,(LPSTR)&Tmpb[0],10);
  return this->cAppend(DecNumToStrS(val, (LPSTR)&Tmpb)); // No signs //this->cAppend((LPSTR)&Tmpb[0]);
 }
//----------------------
 LPSTR uAppend(const unsigned int val)
 {
  BYTE Tmpb[64];
 // _ltoa(val,(LPSTR)&Tmpb[0],10);
  return this->cAppend(DecNumToStrS(val, (LPSTR)&Tmpb)); // No signs //this->cAppend((LPSTR)&Tmpb[0]);
 }
//----------------------

 LPSTR iAppend(const unsigned long val)
 {
  char Tmpb[64];
  char* buf = &Tmpb[30];
  char* end = buf;
  if(val)
   {
    unsigned long Val = val;
    for(buf--;Val;buf--){*buf = (Val % 10) + '0'; Val /= 10;}
    buf++;
  }
   else {*buf = '0'; end++;}
  return this->cAppend(buf,end-buf);
 }
//----------------------
 LPSTR cInsert(const char* str, UINT Pos, UINT Len=0)
 {
  if(Pos >= this->SLength)return this->cAppend(str, Len);   // Or just return a NULL instead?
  if(!Len && str)Len = lstrlen(str);      // Will compiler inline this function and remove this condition if it known true (when default used)?
  if(!Len)return this->Data; 
  if(!this->ResizeFor(Len+this->SLength))return NULL;
  movemem((PBYTE)&this->Data[Pos+Len],(PBYTE)&this->Data[Pos],(this->SLength-Pos)+1);  // +1 for a Zero term char   // memmove
  LPSTR Dst = &this->Data[Pos];
  if(str)memmove(Dst,str,Len);
  this->SLength += Len;
  return Dst;  // Returns ptr to beginning of an inserted string
 }
//----------------------
 bool Equal(const CMiniStr &str)
  {
   return (strcmp(this->Data,str.c_str()) == 0);
  }
//----------------------
 bool Equal(const char* str)
  {
   return (strcmp(this->c_str(),(str)?(str):("")) == 0);
  }
//----------------------
 //LPSTR Assign(const CMiniStr &str){this->sAssign(str);}
 //LPSTR cAssign(LPSTR str){this->cAssign(str);}
 //LPSTR sAppend(const CMiniStr &str){this->sAppend(str);}
 //LPSTR cAppend(LPSTR str){this->cAppend(str);}
//----------------------

public:

//----------------------   // Instead of lambdas
typedef bool (_fastcall *COMPARATOR)(BYTE ChrA, BYTE ChrB);

 static bool _fastcall ComparatorE (BYTE ChrA, BYTE ChrB){return (ChrA == ChrB);}
 static bool _fastcall ComparatorL (BYTE ChrA, BYTE ChrB){return (ChrA  < ChrB);}
 static bool _fastcall ComparatorG (BYTE ChrA, BYTE ChrB){return (ChrA  > ChrB);}
 static bool _fastcall ComparatorEL(BYTE ChrA, BYTE ChrB){return (ChrA <= ChrB);}
 static bool _fastcall ComparatorEG(BYTE ChrA, BYTE ChrB){return (ChrA >= ChrB);}
 static bool _fastcall ComparatorNE(BYTE ChrA, BYTE ChrB){return (ChrA != ChrB);}
 //----------------------
							CMiniStr(void){this->Initialize();}
							CMiniStr(const CMiniStr& str)                      {this->Initialize();this->sAssign(str);}
							CMiniStr(const CMiniStr& str, const CMiniStr& app) {this->Initialize();this->sAssign(str);this->sAppend(app);}
 //template<class C>          CMiniStr(const C* str)                             {this->Initialize();this->Assign(str);}
							CMiniStr(const char* str)                          {this->Initialize();this->cAssign(str);}
							CMiniStr(const wchar_t* str)                       {this->Initialize();this->cAssign(str);}
//							CMiniStr(const unsigned char* str)                 {this->Initialize();this->cAssign(str);}

 template<class C>          CMiniStr(const C* str, const CMiniStr& app)        {this->Initialize();this->cAssign(str);this->sAppend(app);}
 template<class C>          CMiniStr(const CMiniStr& str, const C* app)        {this->Initialize();this->sAssign(str);this->cAppend(app);}
 //template<class C, class E> CMiniStr(const C* str, const E* app)               {this->Initialize();this->Assign(str);this->Append(app);}
							CMiniStr(const CMiniStr& str, const ULONG app){this->Initialize();this->sAssign(str);this->iAppend(app);}
							CMiniStr(const int val, const CMiniStr& app){this->Initialize();this->iAssign(val);this->sAppend(app);}
							CMiniStr(const int val){this->Initialize();this->iAssign(val);}
//----------------------
 ~CMiniStr()
 {
  this->ResizeFor(0); 
 }
//----------------------
 operator   const char*()    {return (char*)this->c_str();}
//----------------------
 void  Clear(void){this->ResizeFor(0); this->Initialize();}
//----------------------
 UINT  Length(void) const {return this->SLength;}
//----------------------
 PBYTE c_data(void) const {return (PBYTE)this->Data;}
//----------------------
 LPSTR c_str(void) const {if(this->SLength)return this->Data; return (LPSTR)&this->SLength;}    // No "", it generates a very ugly code with a global constant  // Data is always NULL when SLength is 0?
//----------------------
 void  operator = (const CMiniStr &str){this->sAssign(str);}
//----------------------
 void  operator = (const LPSTR str){this->cAssign(str);}
//----------------------
 CMiniStr& operator += (const CMiniStr &str){this->sAppend(str);return *this;}
//----------------------
 CMiniStr& operator += (const LPSTR str){this->cAppend(str);return *this;}
//----------------------
 CMiniStr& operator += (const PWSTR str){this->cAppend(str);return *this;}
//----------------------
 CMiniStr& operator += (int val){this->iAppend(val);return *this;}
//----------------------
 CMiniStr& operator += (unsigned int val){this->uAppend(val);return *this;}
//----------------------
 CMiniStr& operator += (unsigned long val){this->iAppend(val);return *this;}
//----------------------
 bool  operator == (const CMiniStr &ent) {return this->Equal((const char *)ent.Data);}
//----------------------
 bool  operator == (char* ent) {return this->Equal(ent);}
//----------------------
 bool  operator != (const CMiniStr &ent) {return !this->Equal((const char *)ent.Data);}
//----------------------
 bool  operator != (char* ent) {return !this->Equal(ent);}
//----------------------
 char& operator [] (const int idx){return this->Data[idx];} // 'this->Data' May be NULL!!!
//----------------------


const CMiniStr& SetLength(UINT NewLen, char fill=0x20)   // NOTE: Generates a huge and ugly code!!!
{
 if(NewLen >  this->SLength)return this->AddChars(fill,(NewLen - this->SLength));
 if(NewLen != this->SLength)
  {
   this->ResizeFor(NewLen);
   this->SLength = NewLen;
   if(this->Data)this->Data[NewLen] = 0;
  }
 return *this;
}
//----------------------
const CMiniStr& AddChars(char val, int Count=1)     // return CONST?
{
 LPSTR str = this->cAppend(LPSTR(0), Count);
 memset(str,val,Count);
 return *this;
}
//----------------------
const CMiniStr& InsertChars(char val, int Pos, int Count=1)     // return CONST?
{
 LPSTR str = this->cInsert(LPSTR(0), Pos, Count);
 memset(str,val,Count);
 return *this;
}
//----------------------
const CMiniStr& ConcatLines(void)
{
 int dpos = 0;
 for(UINT end=this->Length(),spos=0;spos < end;spos++)
  {
   BYTE val = this->Data[spos];
   if((val == '\r')||(val == '\n'))continue;
   this->Data[dpos++] = val;
  }
 return this->SetLength(dpos);
}
//----------------------
const CMiniStr& SplitLines(UINT Width, LPSTR Delim="\r\n")
{
 CMiniStr temps = *this;
 int dellen = lstrlen(Delim);
 this->SetLength(temps.SLength + ((temps.SLength / Width)*dellen));
 for(UINT spos = 0, dpos=0;spos < temps.SLength;)
  {
   if(spos && !(spos % Width)){memcpy(&this->Data[dpos],Delim,dellen);dpos+=dellen;}
   this->Data[dpos++] = temps[spos++];
  }
 return *this; 
}
//----------------------
bool SameAt(UINT pos, const char* str, UINT len=0)    // Slow comparision(not a 'memcmp', but do not requires a string`s length)
{
 UINT idx = 0;
 if(len > (this->Length() + pos))return false;   // The String does not fit!
 //  LPSTR From = (LPSTR)&this->Data[pos];     // For testing only!
 for(UINT end=this->Length();pos < end;pos++,idx++){if(this->Data[pos] != str[idx])return !(str[idx]);}  // (str[idx] == Zero) means a Full Match
 return !(str[idx]);  // If Zero, then Full Match
}
//----------------------
int Pos(const char* str, int from=0, UINT len=0, UINT Maxoffs=0)
{
 if(!len)len = lstrlen(str);   // That faster than comparing strings until a last char
 if(from < 0)   // Search backwards
  {
   from = -from;
   if((from >= (int)this->Length())||(len > this->Length()))return -1;   // Out of the string   // Incorrect with Maxoffs
   for(int idx=from;idx >= Maxoffs;idx--){if(this->SameAt(idx,str,len))return idx;}
  }
   else for(int end=Maxoffs?Maxoffs:this->Length(),idx=from;(idx+(int)len) <= end;idx++){if(this->SameAt(idx,str,len))return idx;}
 return -1;
}
//---------------------
int Pos(BYTE chr, COMPARATOR cmpr=ComparatorE, int from=0)
{
 if(from < 0)   // Search backwards
  {
   from = -from;
   if(from >= (int)this->Length())return -1;   // Out of the string
   for(int idx=from;idx >= 0;idx--){if(cmpr(this->Data[idx],chr))return idx;}
  }
   else for(int end=this->Length(),idx=from;idx < end;idx++){if(cmpr(this->Data[idx],chr))return idx;}
 return -1;
}
//---------------------
int Count(const char* str, int from=0)
{
 int ctr = 0;
 int len = lstrlen(str);
 if(from < 0)
  {
   for(;;from = -(from-len))     // 'from-len' - No overlapping
	{
	 from = this->Pos(str, from, len);
	 ctr += (bool)(from>=0);
	 if(from < len)break;
	}
  }
   else
	{
	 for(;;from+=len,ctr++)      // 'from+=len' - No overlapping   // "AAAAAAAAAAAAAAAA" contains "AAAA" many times
	  {
	   from = this->Pos(str, from, len);
	   if(from < 0)break;
	  }
	}
 return ctr;
}
//----------------------
int Count(BYTE chr, int from=0)
{
 int ctr = 0;
 if(from < 0)
  {
   for(;;from = -(from-1))
	{
	 from = this->Pos(chr, ComparatorE, from);
	 ctr += (bool)(from>=0);
	 if(from <= 0)break;
	}
  }
   else
	{
	 for(;;from++,ctr++)
	  {
	   from = this->Pos(chr, ComparatorE, from);
	   if(from < 0)break;
	  }
	}
 return ctr;
}
//----------------------
 const CMiniStr& Delete(UINT offs, UINT cnt)
  {
   if(offs >= this->SLength)return *this; // Nothing to delete
   if((offs+cnt) > this->SLength){this->SetLength(offs);return *this;}
   memmove(&this->Data[offs],&this->Data[offs+cnt],(this->SLength-(offs+cnt))+1);  // +1 for a Zero term char
   this->SLength -= cnt;
   this->ResizeFor(this->SLength);
   return *this;
  }
//----------------------
 bool ConvertCodePage(UINT FromCP, UINT ToCP)
  {
   UINT  TmpLen = (this->SLength*sizeof(WCHAR));   // twice as size of current string
   PBYTE TmpBuf = (PBYTE)HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,this->Data,(TmpLen*2)+64);
   if(!TmpBuf)return false;
   int wlen = MultiByteToWideChar(FromCP,0,this->Data,this->SLength,(PWSTR)TmpBuf,(TmpLen/2));
   if(wlen <= 0){HeapFree(GetProcessHeap(),0,TmpBuf); return false;}
   int ulen = WideCharToMultiByte(ToCP,0,(PWSTR)TmpBuf,wlen,(LPSTR)&TmpBuf[TmpLen],TmpLen,NULL,NULL);
   if(ulen <= 0){HeapFree(GetProcessHeap(),0,TmpBuf); return false;}
   this->ResizeFor(ulen); // Don`t subtract 1 from 'ulen', the manual is WRONG
   this->SLength = ulen;
   memcpy(this->Data,&TmpBuf[TmpLen],ulen); // lstrcpy(this->Data,(LPSTR)&TmpBuf[TmpLen]);
   this->Data[this->SLength] = 0;
   HeapFree(GetProcessHeap(),0,TmpBuf); 
   return true;
  }
//----------------------
 bool FromFile(PVOID FileName)
  {
   HANDLE hFile;
   if(!((PBYTE)FileName)[1])hFile = CreateFileW((PWSTR)FileName,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL|FILE_FLAG_SEQUENTIAL_SCAN,NULL);
     else hFile = CreateFileA((LPSTR)FileName,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL|FILE_FLAG_SEQUENTIAL_SCAN,NULL);
   if(hFile == INVALID_HANDLE_VALUE)return false;
   DWORD Result   = 0;
   DWORD FileSize = GetFileSize(hFile,NULL);
   this->SetLength(FileSize);
   if(FileSize)ReadFile(hFile,this->Data,FileSize,&Result,NULL);
   CloseHandle(hFile);
   return (Result == FileSize);
  }
//----------------------
 bool ToFile(PVOID FileName)
  {
   HANDLE hFile;
   if(!((PBYTE)FileName)[1])hFile = CreateFileW((PWSTR)FileName,GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL|FILE_FLAG_SEQUENTIAL_SCAN,NULL);
     else hFile = CreateFileA((LPSTR)FileName,GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL|FILE_FLAG_SEQUENTIAL_SCAN,NULL);
   if(hFile == INVALID_HANDLE_VALUE)return false;
   DWORD Result   = 0;
   WriteFile(hFile,this->Data,this->SLength,&Result,NULL);
   CloseHandle(hFile);
   return (Result == this->SLength);
  }


};
//-----------------------------------------------------------------------------
//  static - To keep only one symbol instance of functions in *.h
//
 static CMiniStr operator + (const CMiniStr &str1, const CMiniStr &str2) {return CMiniStr(str1,str2);}
 static CMiniStr operator + (const char*     str1, const CMiniStr &str2) {return CMiniStr(str1,str2);}
 static CMiniStr operator + (const wchar_t*  str1, const CMiniStr &str2) {return CMiniStr(str1,str2);}

 static CMiniStr operator + (const int val1, const CMiniStr &str2) {return CMiniStr(val1,str2);}
 static CMiniStr operator + (const CMiniStr &str2, const int val1) {return CMiniStr(str2,val1);} // Why BCC confuses it with 'operator []' ?
//---------------------------------------------------------------------------
static int _stdcall ReplaceParamXML(CMiniStr& XmlStr, LPCSTR ParName, LPSTR ParValue, bool Single)
{
 int ReplCnt = 0;
 int NamLen  = lstrlenA(ParName);
 for(int offs=0,spos=0;;)
  {
   if((spos = XmlStr.Pos(ParName, offs)) < 0)break;
   spos += NamLen;
   if(spos >= XmlStr.Length())break;  
   bool ExCont = false;
   offs = spos+1;
   for(;XmlStr.c_str()[spos] != '=';spos++){if(XmlStr.c_str()[spos] > 0x20){ExCont=true; break;}}   // Not a param
   if(ExCont)continue;
   spos++;
   ExCont = false;
   offs = spos+1;
   for(;XmlStr.c_str()[spos] != '\"';spos++){if(XmlStr.c_str()[spos] > 0x20){ExCont=true; break;}}   // Not a param
   if(ExCont){continue;}
   spos++;
   offs = spos;  // + 1;         // Start of param
   if((spos = XmlStr.Pos('"', CMiniStr::ComparatorE, offs )) < 0)break;
   int plen = spos - offs;  // End of param

   XmlStr.Delete(offs,plen);  // Some special TextSegmentReplace proc will be faster than this double reallocation
   XmlStr.cInsert(ParValue,offs);
   ReplCnt++;
   if(Single)break;
  }
 return ReplCnt;
}
//------------------------------------------------------------------------------------
static bool _fastcall IsCharReservedURI(BYTE chr)
{
 BYTE ChArr[] = {'!', '#', '$', '&', '\'', '(', ')', '*', '+', ',', '/', ':', ';', '=', '?', '@', '[', ']', '%'};
 for(int ctr=0;ctr < sizeof(ChArr);ctr++){if(ChArr[ctr] == chr)return true;}
 return false;
}
//------------------------------------------------------------------------------------
static void _fastcall EncodeURI(CMiniStr& str)
{
 CMiniStr res;
 int ResLen = 0;
 int SrcLen = str.Length();
 res.SetLength(SrcLen*3);   // As if an entire string will be encoded
 LPSTR Dst  = res.c_str(); 
 LPSTR Src  = str.c_str(); 
 for(;SrcLen > 0;SrcLen--,Src++,Dst++,ResLen++)
  {
   BYTE Val = *Src;
   if(IsCharReservedURI(Val))
    {
     *(Dst++) = '%';
     *(PWORD)(Dst++) = HexToChar(Val, false);
     ResLen  += 2;
    }
     else *Dst = Val;
  }
 str.cAssign(res.c_str(), ResLen);
}
//------------------------------------------------------------------------------------
static void _fastcall DecodeURI(CMiniStr& str)
{
 CMiniStr res;
 int ResLen = 0;
 int SrcLen = str.Length();
 res.SetLength(SrcLen);   // As if an entire string will be encoded
 LPSTR Dst  = res.c_str(); 
 LPSTR Src  = str.c_str(); 
 for(;SrcLen > 0;SrcLen--,Src++,Dst++,ResLen++)
  {
   BYTE Val = *Src;
   if(Val == '%')
    {
     int ByteHi  = CharToHex(*(++Src));
     int ByteLo  = CharToHex(*(++Src));
     if((ByteHi  < 0)||(ByteLo < 0))Val = 0;  // Not a HEX char!
     Val = (ByteHi << 4)|ByteLo;
     SrcLen -= 2;
    }
   *Dst = Val;
  }
 str.cAssign(res.c_str(), ResLen);
}
//------------------------------------------------------------------------------------

//===========================================================================================================
#endif
