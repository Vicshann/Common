
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


//#include <Windows.h>
//#include "MiniString.h"


//----------------------------------------------------------------------------
class CMiniIni
{
enum EStates {stExpValNameNL,stExpSecBeg,stExpSecEnd,stExpValNameBeg,stExpValNameEnd,stExpValBeg,stExpValEnd};
struct SValRec
{
 LPSTR Name;
 LPSTR Value;
 UINT  NameLen;
 UINT  ValueLen;
 SValRec* Next; 
};

struct SSecRec
{
 LPSTR Name;
 UINT  Len;
 SValRec* Recs;
 SSecRec* Next;
};

 EStates  State;
 SSecRec* Secs;

//----------------------------------------------------------------------------
static void TrimSpaces(LPSTR& Str, int& Len)
{
 for(;Len && (*Str == ' ');Str++,Len--);
 for(;Len && (Str[Len-1] == ' ');Len--);
}
//----------------------------------------------------------------------------
// Keep offsets of SecName, ValName, Value  for updating and keeping comments 
// Note: Duplicates allowed
// Note: No names copying from a source string!
int AddValue(LPSTR SecName, LPSTR ValName, LPSTR Value, int SecNamLen, int ValNamLen, int ValStrLen)
{
 TrimSpaces(SecName, SecNamLen);
 TrimSpaces(ValName, ValNamLen);
 TrimSpaces(Value, ValStrLen);
 if(SecNamLen <= 0)return -1;
 if(ValNamLen <= 0)return 1;
 SSecRec* CurSec = NULL;
 if(this->Secs)
  {  
   SSecRec* LstSec;
   for(LstSec = this->Secs;;LstSec=LstSec->Next)
    {
     if((SecNamLen == LstSec->Len) && (memcmp(SecName,LstSec->Name,SecNamLen)==0)){CurSec = LstSec;break;}    // Found a section by name
     if(!LstSec->Next)break;
    }
   if(!CurSec)LstSec->Next = CurSec = (SSecRec*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(SSecRec));  // Big fragmentation but no preallocation 
  }
   else this->Secs = CurSec = (SSecRec*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(SSecRec));  // Big fragmentation but no preallocation
 SValRec* CurRec = NULL;
 if(!CurSec->Recs)
  {
   CurSec->Name = SecName;
   CurSec->Len  = SecNamLen;
   CurSec->Recs = CurRec = (SValRec*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(SValRec));  // Big fragmentation but no preallocation
  }
   else
    {
     for(CurRec = CurSec->Recs;CurRec->Next;CurRec=CurRec->Next);
     CurRec->Next = (SValRec*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(SValRec));  // Big fragmentation but no preallocation
     CurRec = CurRec->Next;
    }
 CurRec->Name  = ValName;
 CurRec->Value = Value;
 CurRec->NameLen  = ValNamLen;
 CurRec->ValueLen = ValStrLen;
 return 0;
}
//----------------------------------------------------------------------------


public:
CMiniIni(void)
{
 this->Secs  = NULL;
 this->State = stExpSecBeg;
}
//----------------------------------------------------------------------------
~CMiniIni()
{
 this->Clear();
}
//----------------------------------------------------------------------------
void Clear(void)
{
 for(SSecRec* CurSec = this->Secs;CurSec;)
  {
   for(SValRec* CurRec = CurSec->Recs;CurRec;CurRec=CurRec->Next)HeapFree(GetProcessHeap(),0,CurRec);
   SSecRec* DelSec = CurSec;
   CurSec = CurSec->Next;
   HeapFree(GetProcessHeap(),0,DelSec);
  }
}
//----------------------------------------------------------------------------
// Keep comments?
bool ParseString(LPSTR Str, UINT MaxLen=0, bool Update=false)
{
 LPSTR SecBeg, SecEnd, ValBeg, ValEnd, ValNamBeg, ValNamEnd; 
 
 if(!Update){this->Clear(); this->State = stExpSecBeg;}
 for(UINT Offs=0;*Str;Offs++,Str++)
  {
ReEnter:
   switch(this->State)  // {stExpSecBeg,stExpSecEnd,stExpValNameBeg,stExpValNameEnd,stValBeg,stValEnd};
    {
     case stExpSecBeg:
//      if(*Str == ' ')break;   // No Spaces allowed before a section name
      if(*Str != '[')continue;      // Reset a section search
      SecBeg = Str+1;
      this->State = stExpSecEnd;
     continue;
     case stExpSecEnd:
      if(*Str  < ' ')break;        // Reset a section search on a new line
      if(*Str != ']')continue;
      SecEnd = Str;
      this->State = stExpValNameNL;
     continue;
     case stExpValNameNL:
      if(*Str >= ' ')continue;     // Search for a NewLine
      this->State = stExpValNameBeg;
     continue;
     case stExpValNameBeg:
      if(*Str <= ' ')continue;     // No spaces at beginning of value names  // Empty lines between values allowed
      if(*Str == ';')continue;     // Just skip any comments for now
      if(*Str == '['){this->State = stExpSecBeg; goto ReEnter;}   // A new section ?  // Value names can`t start with  '[' ?
      ValNamBeg = Str; 
      this->State = stExpValNameEnd;
     continue;
     case stExpValNameEnd:
      if(*Str  < ' ')break;        // Unexpected NewLine
      if(*Str != '=')continue;
      ValNamEnd = Str;
      this->State = stExpValBeg;
     continue;
     case stExpValBeg:
      if(!*Str)break;    // Values may be empty: 'Val='
      ValBeg = Str--;    // Decrement in case the value is empty
      this->State = stExpValEnd;
     continue;
     case stExpValEnd:
      if(*Str  >= ' ')continue;      // Tabs is not allowed in values?
      ValEnd = Str;            
      if(this->AddValue(SecBeg, ValNamBeg, ValBeg, SecEnd-SecBeg, ValNamEnd-ValNamBeg, ValEnd-ValBeg) < 0)return false;
      this->State = stExpValNameBeg;
     continue;
    }
   this->State = stExpSecBeg;  // Reset
  }
 return true;
}
//----------------------------------------------------------------------------
int GetValue(LPSTR SecName, LPSTR ValName, LPSTR Buffer, UINT BufLen)
{
 UINT SecNamLen = lstrlen(SecName);
 UINT ValNamLen = lstrlen(ValName);
 for(SSecRec* CurSec = this->Secs;CurSec;CurSec=CurSec->Next)
  {
   if((SecNamLen != CurSec->Len) || (memcmp(SecName,CurSec->Name,SecNamLen)!=0))continue;
   for(SValRec* CurRec = CurSec->Recs;CurRec;CurRec=CurRec->Next)
    {
     if((ValNamLen != CurRec->NameLen) || (memcmp(ValName,CurRec->Name,ValNamLen)!=0))continue;
     BufLen = (BufLen < (CurRec->ValueLen+1))?(BufLen):(CurRec->ValueLen);
     lstrcpyn(Buffer,CurRec->Value,BufLen+1);
     return BufLen;
    }
  }
 return -1;
}
//----------------------------------------------------------------------------
};
//----------------------------------------------------------------------------

