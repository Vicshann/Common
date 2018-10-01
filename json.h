

#pragma once

#ifndef JSONH
#define JSONH
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

#include "MiniString.h"

class CJSonItem;

enum TJSArray  {jsArray=1000};
enum TJSObject {jsObject=1001};
enum EJSonType {jstEmpty,jstNULL,jstBool,jstInt,jstHexInt,jstFloat,jstString,jstObject,jstArray,jstAny=-1};  // Null is not a type AT ALL - it is a VALUE representation. We either must get rid of it or assume that an entities can stay typeless
//enum EJSonType {jstEmpty=0,jstBool,jstInt,jstHexInt,jstFloat,jstString,jstObject,jstArray,jstAny=-1}; // Max: 00 - 0F for binary  // TODO: JSON Null must mean an Object or array with no members
enum ECnvOpts  {coNone=0, coFormat=1, coWrUsrData=2, coNoUniCnv=4};
enum EBFlags   {bfNamePresent=0x10,bfDataPresent=0x20,bfValuePresent=0x40,bfBoolValue=0x80};
/* InMemory:
BYTE Flags;
// [Field Presence flags]:4,[Type Index]:4
0x10 - Name  Present   // A named object
0x20 - Data  Present   // User data  // Data size is sizeof(PVOID) // Do not store on x64 there some flags more than sizeof(DWORD) if planned to use it on x32
0x40 - Value Present   // Else this is NULL object (except BOOL type)  // Use also for a initially ZERO numberic values? - NO!
0x80 - BOOL type value
*/

/* InBinaryFile: // Unnamed BOOL=1 byte, unnamed int=1 byte(0 value), 3 bytes(0-255 value),...
 BYTE Flags
 Name:[BYTE Size, CHAR[]] // No terminating NULL, 0 = 1 char, max 256
 DATA:[BYTE(DataSize:4,ValueSize:4), BYTE[]]    // Size Field also used for specifying size of Value
 VALUE:[BYTE(DataSize:4,ValueSize:4), BYTE[]]
 // Extended: NumArray: DWORD ElemCount, BYTE ElemSize,
*/

class CJSonItem     // OPTIMIZE: Align Name sizes to CPU data size (x86/x64) and compare ad these data blocks
{
static const DWORD BINSIG = 'NSIB';   // '\0NSB'
union CJSonVal
{
 bool 		BoolVal;
 double		FloatVal;   // 64 bit
 __int64	IntVal;     // 64 bit    // Any pointers can be safely stored here  // Add UIntVal?
 LPSTR      StringVal;  // 32/64 bit
 CJSonItem* Items;      // 32/64 bit // Array/Object collection of values
};
public:
 PVOID      Data;          // User`s data  // TODO: Make optional somehow

private:
 EJSonType  Type;
 LPSTR      Name;         // Only for Object`s items  // TODO: Replace with PWSTR or support UTF8?  // TODO: Store type here for unnamed objects(low bit as a mark?)  // Optionally use a name manager to avoid duplicates
 UINT       mCount;       // TODO: Needed only for Object and Array and can be stored there // Also need support of memory framework to select between fast enumeration(including simple access by index) and fast expanding using memory chunks
// CJSonItem* pParent;    // !!! Memory PTR becomes invalalid after reallocation !!!
 CJSonVal   Value;

// CJSonItem& operator = (const CJSonItem &) = delete;      // Conflicts with other 'operator ='
//--------------
 void Cleanup(bool KeepName=false)
  {
   if(!KeepName && this->Name){this->Name -= sizeof(DWORD);ReAllocBuf(&this->Name,0); this->Name = NULL;}
   if((this->Type == jstString)&&this->Value.StringVal){this->Value.StringVal -= sizeof(DWORD);ReAllocBuf(&this->Value.StringVal,0);}
	 else if(((this->Type == jstArray)||(this->Type == jstObject))&&this->Value.Items)
	  {
	   for(UINT ctr=0;ctr < this->mCount;ctr++)this->Value.Items[ctr].Cleanup();  // Destroy all children items
	   ReAllocBuf(&this->Value.Items,0);
       this->Value.Items = NULL;
	  }
   this->mCount = 0;	  
  }
//--------------
 void Nullify(void)
  {
   this->Type     = jstEmpty;
   this->mCount   = 0;
   this->Value.Items = NULL;
   this->Name     = NULL;
   this->Data     = NULL;
//   this->pParent = NULL;
  }
//--------------
 template<typename T>static void ReAllocBuf(T* buf, UINT Size)
  {
   if(!Size){if(*buf)HeapFree(GetProcessHeap(),0,*buf);*buf=NULL;return;}
   if(!*buf)*buf = (T)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,Size+5);
	 else *buf = (T)HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,*buf,Size+5);
  }
//--------------
 static void SetString(LPSTR* str, const CMiniStr& val)
  {
   SetString(str, (const char*)val.c_data(), val.Length());
  }
//--------------
 static void SetString(LPSTR* str, const char* Value, UINT Len) // str is &data[4]
  {
   if(!Value){ReAllocBuf(str, 0);return;} // Free the string
   if(!Len)Len = lstrlen(Value);
   if(*str)(*str) -= sizeof(DWORD);
   ReAllocBuf(str, Len+sizeof(DWORD)+1);  // +1 for ZeroTerm  // +1 Allows an empty stringa
   *((PDWORD)*str) = Len;
   (*str) += sizeof(DWORD);
   if(Len)memcpy(*str,Value,Len);
   (*str)[Len] = 0;
  }
//--------------
 static void BinEncrypt(PBYTE data, UINT Size, BYTE Key)
  {
   BYTE Key2 = (Size & 0xFF) * ((Size >> 8) & 0xFF);
   for(UINT ctr=0;ctr<Size;ctr++)
	{
     BYTE VSu = ((ctr & 0x7E) * Key2)+ctr;
	 BYTE val = data[ctr];
	 val  = ~(val+Key) ^ Key2;
     val  = (ctr & 1)?(val-VSu):(val+VSu);
	 data[ctr] = val;
	}
  }
//--------------
 static void BinDecrypt(PBYTE data, UINT Size, BYTE Key)
  {
   BYTE Key2 = (Size & 0xFF) * ((Size >> 8) & 0xFF);
   for(UINT ctr=0;ctr<Size;ctr++)
	{
     BYTE VSu = ((ctr & 0x7E) * Key2)+ctr;
	 BYTE val = data[ctr];
     val  = (ctr & 1)?(val+VSu):(val-VSu);
     val  = ~(val ^ Key2) - Key;
	 data[ctr] = val;
	}
  }
//--------------
 template<typename T> static T ValueFromBytes(PBYTE data, UINT size)
  {
   UINT64 val = 0;
   for(UINT ctr=0;ctr<size;ctr++)((PBYTE)&val)[ctr] = data[ctr];
   return *((T*)&val);
  }
//--------------
 template<typename T> static int CountValueBytes(T* value)
  {
   int num = 0;
//   int tst = sizeof(*value);
   UINT32* dat = (UINT32*)value;
   if((sizeof(*value) > 4) && dat[1]){num += 4; dat++;} // Take HIGH part
   for(UINT32 val = *dat;val;val >>= 8)num++;
   return num;
  }
//--------------
 bool IsItemBelongs(CJSonItem* Item)
  {
   if(Item <  this->Value.Items)return false;
   if(Item >= &this->Value.Items[this->mCount])return false;
   return true;
  }
//---------------------------------------------------------------------------
/*static long _fastcall CharToHex(BYTE CharValue) 
{
 if((CharValue >= 0x30)&&(CharValue <= 0x39))return (CharValue - 0x30);		 // 0 - 9
 if((CharValue >= 0x41)&&(CharValue <= 0x46))return (CharValue - (0x41-10)); // A - F
 if((CharValue >= 0x61)&&(CharValue <= 0x66))return (CharValue - (0x41-10)); // a - f
 return -1;
}
//---------------------------------------------------------------------------
static DWORD _fastcall DecStrToDW(LPSTR String)   // Fast, but do not safe
{
 long  StrLength = 0;
 DWORD Result    = 0;
 DWORD DgtPow    = 1;
 BYTE  Symbol;

 for(int ctr=0;((BYTE)(String[ctr]-0x30)) <= 9;ctr++)StrLength++;     // Break on any non digit
 for(long ctr=1;ctr<=StrLength;ctr++)
  {
   Symbol  = (String[StrLength-ctr]-0x30);
   Result += (DgtPow*Symbol);
   DgtPow  = 1;
   for(long num = 0;num < ctr;num++)DgtPow = DgtPow*10;
  }            
 return Result;
}
//---------------------------------------------------------------------------
static DWORD _fastcall HexStrToDW(LPSTR String, UINT Bytes)   // Fast, but do not safe
{
 long  StrLength = 0;
 DWORD Result    = 0;
 DWORD DgtPow    = 1;
 BYTE  Symbol;

 for(int ctr=0;CharToHex(String[ctr]) >= 0;ctr++)StrLength++;     // Break on any non hex digit
 if(Bytes)StrLength = ((StrLength > (Bytes*2))?(Bytes*2):(StrLength))&0xFE;
 for(long ctr=1;(ctr<=8)&&(ctr<=StrLength);ctr++)
  {
   Symbol  = (String[StrLength-ctr]-0x30);
   if(Symbol > 9)Symbol  -= 7;
   if(Symbol > 15)Symbol -= 32;
   Result += (DgtPow*Symbol);
   DgtPow  = 1;
   for(long num = 0;num < ctr;num++)DgtPow = DgtPow*16;
  }
 return Result;
}      */
//---------------------------------------------------------------------------
/*static LPSTR _fastcall ConvertToHexStr(UINT64 Value, int MaxDigits, LPSTR Number, bool UpCase)
{
 const int cmax = sizeof(UINT64)*2;
 char  HexNums[] = "0123456789ABCDEF0123456789abcdef";
 int   DgCnt;
 DWORD Case;

 if(MaxDigits <= 0)MaxDigits = (Value > 0xFFFFFFFF)?(16):(8);
 for(int ctr=0;ctr<(int)MaxDigits;ctr++){Number[ctr]='0';Number[ctr+1]=0;}
 if(UpCase)Case = 0;
   else Case = 16;
 for(DgCnt = cmax;DgCnt > 0;DgCnt--)
  {
   ((char *)Number)[DgCnt-1] = HexNums[((Value & 0x0000000F)+Case)];
   Value = Value >> 4;
  }
 for(DgCnt = 0;Number[DgCnt] == '0';DgCnt++);
 if((MaxDigits > cmax) || (MaxDigits == 0))MaxDigits = cmax;
 if((cmax-DgCnt) < MaxDigits)DgCnt = cmax - MaxDigits;
 return (Number + DgCnt);
}*/    
//---------------------------------------------------------------------------
 CJSonItem* DuplicateTmpAt(CJSonItem* Dst)  // Apply this ONLY to a temporary objects (Owner of items` memory will be changed)
  {
   memcpy(Dst,this,sizeof(CJSonItem)); //::new ((void *)Ptr) CJSonItem(Item);
   this->Nullify();    // Invalidate original (Prevents the memory from freeing)
//   Dst->pParent = this;
   return Dst;
  }


public:
 ~CJSonItem()
  {
   this->Cleanup();
  }
//--------------
// CJSonItem(CJSonItem& itm){itm.DuplicateTmpAt(this);} // Stealing move constructor 

 CJSonItem(LPSTR name=NULL)                  // jstNULL or jstEmpty
  {
   this->Nullify();
   if(name){SetString(&this->Name,name,0);/*this->Type = jstNULL;*/} 
   this->Type = jstObject;
  }
 CJSonItem(int num, LPSTR name=NULL)         // jstInt
  {
   this->Nullify();
   if(name)SetString(&this->Name,name,0);
   this->Type = jstInt;
   this->Value.IntVal = num;
  }
 CJSonItem(__int64 num, LPSTR name=NULL)     // jstInt
  {
   this->Nullify();
   if(name)SetString(&this->Name,name,0);
   this->Type = jstInt;
   this->Value.IntVal = num;
  }
 CJSonItem(UINT num, LPSTR name=NULL)         // jstInt
  {
   this->Nullify();
   if(name)SetString(&this->Name,name,0);
   this->Type = jstHexInt;
   this->Value.IntVal = num;
  }
 CJSonItem(UINT64 num, LPSTR name=NULL)     // jstInt
  {
   this->Nullify();
   if(name)SetString(&this->Name,name,0);
   this->Type = jstHexInt;
   this->Value.IntVal = num;
  }
 CJSonItem(bool flg, LPSTR name=NULL)        // jstBool
  {
   this->Nullify();
   if(name)SetString(&this->Name,name,0);
   this->Type = jstBool;
   this->Value.BoolVal = flg;
  }
 CJSonItem(double val, LPSTR name=NULL)       // jstFloat
  {
   this->Nullify();
   if(name)SetString(&this->Name,name,0);
   this->Type = jstFloat;
   this->Value.FloatVal = val;
  }
 CJSonItem(const char* str, LPSTR name=NULL)       // jstString
  {
   this->Nullify();
   if(name)SetString(&this->Name,name,0);
   this->Type = jstString;
   SetString(&this->Value.StringVal,str,0);
  }
 CJSonItem(TJSArray dummy, LPSTR name=NULL)  // jstArray
  {
   this->Nullify();
   if(name)SetString(&this->Name,name,0);
   this->Type = jstArray;      // Leave 'this->Value.Items' empty for now
  }
 CJSonItem(TJSObject dummy, LPSTR name=NULL) // jstObject
  {
   this->Nullify();
   if(name)SetString(&this->Name,name,0);
   this->Type = jstObject;    // Leave 'this->Value.Items' empty for now
  }
//--------------

 UINT Count(void){return this->mCount;}
 int IndexOf(CJSonItem* Item)
  {
   if(!this->IsItemBelongs(Item))return -1;
   return (Item - this->Value.Items);   // CJSonItem
  }
//--------------
 const CJSonItem& GetObj(LPSTR Name)
  {
   static const CJSonItem empty;  // Must not be modified
   for(CJSonItem* itm = this->First();itm;itm=this->Next(itm))
	{
	 if(itm->Name && (lstrcmpi(Name,itm->Name)==0))return *itm;  // TODO: Add a case sensitive/insensitive mode selection
	}
   //empty.Nullify();	
   return empty;
  }
//--------------
 CJSonItem* Get(LPSTR Name)
  {
   for(CJSonItem* itm = this->First();itm;itm=this->Next(itm))
	{
	 if(itm->Name && (lstrcmpi(Name,itm->Name)==0))return itm;  // TODO: Add a case sensitive/insensitive mode selection
	}
   return NULL;
  }
//--------------
 CJSonItem* Get(UINT Index)
  {
   if(Index >= this->mCount)return NULL;
   return &this->Value.Items[Index];
  }
//--------------
// CJSonItem* Parent(void){return this->pParent;}  // !!! Becomes invalid after parent`s parent memory reallocation
//--------------
 CJSonItem* First(void)            // No Array/Object check!
  {
   if(!this->mCount)return NULL;
   return this->Value.Items;
  }
 CJSonItem* Last(void)             // No Array/Object check!
  {
   if(!this->mCount)return NULL;
   return &this->Value.Items[this->mCount-1];
  }
 CJSonItem* Next(CJSonItem* Item)  // No Array/Object check!
  {
   Item++;
   if(!this->IsItemBelongs(Item))return NULL;
   return Item;
  }
 CJSonItem* Prev(CJSonItem* Item)  // No Array/Object check!
  {
   Item--;
   if(!this->IsItemBelongs(Item))return NULL;
   return Item;
  }
//--------------

 CJSonItem* Insert(CJSonItem& itm, CJSonItem* Before)  // Array items must be sorted? If so, how to sort Objects?
  {
   int idx = this->IndexOf(Before);
   if(idx < 0)return NULL;  // Or just add to end of the list?
   this->mCount++;
   ReAllocBuf(&this->Value.Items,(this->mCount*sizeof(CJSonItem)));   // May corrupt pointers to items in this container
   memmove(&this->Value.Items[idx+1],&this->Value.Items[idx],((this->mCount-idx-1)*sizeof(CJSonItem)));
   return itm.DuplicateTmpAt(&this->Value.Items[idx]);
  }
 CJSonItem* Add(CJSonItem& itm)  // No Array/Object check!
  {
   this->mCount++;
   ReAllocBuf(&this->Value.Items, (this->mCount*sizeof(CJSonItem)));  // May corrupt pointers to items in this container
   return itm.DuplicateTmpAt(&this->Value.Items[this->mCount-1]);
  }
 CJSonItem* AddFirst(CJSonItem& itm)  // No Array/Object check!
  {
   if(!this->mCount)return this->Add(itm);
   return this->Insert(itm, this->Value.Items);
  }
 bool Delete(UINT Index){return this->Delete(this->Get(Index));}
 bool Delete(CJSonItem* Item)    // Index
  {
   if(!this->IsItemBelongs(Item))return false;
   Item->Cleanup();
   if(this->mCount > 1)
	{
	 UINT cSize = (PBYTE)&this->Value.Items[this->mCount] - (PBYTE)&Item[1];  // &this->Value.Items[this->mCount] - &Item[1];
	 if(cSize)memcpy(Item,&Item[1],cSize);    // *sizeof(CJSonItem)
	}
   this->mCount--;
   ReAllocBuf(&this->Value.Items,(this->mCount*sizeof(CJSonItem)));    // May corrupt pointers to items in this container
   return true;
  }
//--------------
 CJSonItem* StealTemp(const CJSonItem& itm){return ((CJSonItem*)&itm)->DuplicateTmpAt(this);}

 CJSonItem* CopyFrom(const CJSonItem& itm)  // Copies data at pointers, NOT just pointers itself (Except user`s data)
  {
   this->Cleanup();
   this->Type   = itm.Type;
   this->Data   = itm.Data;
   this->mCount = itm.mCount;
   SetString(&this->Name, itm.Name, CountedStrLen(itm.Name));
   if(((this->Type == jstArray)||(this->Type == jstObject))&&this->mCount)
	{
	 this->Value.Items = NULL;
	 ReAllocBuf(&this->Value.Items, this->mCount*sizeof(CJSonItem));
	 memset(this->Value.Items,0,this->mCount*sizeof(CJSonItem));  // All pointers must be NULL
	 for(UINT ctr=0;ctr < this->mCount;ctr++)this->Value.Items[ctr].CopyFrom(itm.Value.Items[ctr]);
	}
	 else if(this->Type == jstString)SetString(&this->Value.StringVal, itm.Value.StringVal, CountedStrLen(itm.Value.StringVal));
	   else this->Value = itm.Value;
   return this;
  }
//--------------
 CJSonItem* AssignFrom(const CJSonItem& itm)  // Do not copies DATA/VALUE and NAME, only pointers
  {
   this->Cleanup();
   this->Type   = itm.Type;
   this->Data   = itm.Data;
   this->Name   = itm.Name;         // Only for Object`s items  // TODO: Replace with PWSTR
   this->mCount = itm.mCount;
   if(((this->Type == jstArray)||(this->Type == jstObject))&&this->mCount)
	{
	 this->Value.Items = NULL;
	 ReAllocBuf(&this->Value.Items, this->mCount*sizeof(CJSonItem));
	 memset(this->Value.Items,0,this->mCount*sizeof(CJSonItem));  // All pointers must be NULL
	 for(UINT ctr=0;ctr < this->mCount;ctr++)this->Value.Items[ctr].AssignFrom(itm.Value.Items[ctr]);
	}
	 else this->Value = itm.Value;
   return this;
  }
//--------------
 CJSonItem* Move(UINT CurIdx, UINT NewIdx)  // Moves children item  // TODO: Multi move
  {
   if((NewIdx >= this->mCount)||(CurIdx >= this->mCount))return NULL; // Out of bounds
   CJSonItem* citm = &this->Value.Items[CurIdx];
   CJSonItem* nitm = &this->Value.Items[NewIdx];
   if(CurIdx == NewIdx)return citm; // No moving needed
   BYTE ItemBuf[sizeof(CJSonItem)+4];
   memcpy(&ItemBuf,citm,sizeof(CJSonItem));   // Save moved item
   if(CurIdx > NewIdx)memmove(&this->Value.Items[NewIdx+1],&this->Value.Items[NewIdx],((CurIdx-NewIdx)*sizeof(CJSonItem)));
	 else memmove(&this->Value.Items[CurIdx],&this->Value.Items[CurIdx+1],((NewIdx-CurIdx)*sizeof(CJSonItem)));
   memcpy(nitm,&ItemBuf,sizeof(CJSonItem));
   return nitm;
  }
//--------------

 void  Clear(void){ this->Cleanup(true); }     // this->Nullify();}

 const LPSTR GetName(void){return this->Name;}
 const UINT  GetNameLen(void){return CountedStrLen(this->Name);}   // {return ((PDWORD)this->Name)[-1];}
 const UINT  GetValStrLen(void){return CountedStrLen(this->Value.StringVal);} 
 void  SetName(const LPSTR name, UINT Len=0){SetString(&this->Name,name,Len);}
 void  SetType(const EJSonType type){this->Type = type;}

 __int64 GetValInt(void){return this->Value.IntVal;}
 bool    GetValBol(void){return this->Value.BoolVal;}
 char*   GetValStr(void){return this->Value.StringVal;}
 double  GetValFlt(void){return this->Value.FloatVal;}

 bool  IsNull(void){return (jstNULL == this->Type);}
 bool  IsArray(void){return (jstArray == this->Type);}
 bool  IsObject(void){return (jstObject == this->Type);}
 bool  IsValInt(void){return (jstInt == this->Type);}
 bool  IsValBol(void){return (jstBool == this->Type);}
 bool  IsValFlt(void){return (jstFloat == this->Type);}
 bool  IsValStr(void){return (jstString == this->Type);}

 void  _SetValInt(const int     val){if(jstInt == this->Type)this->Value.IntVal = val;}
 void  _SetValInt(const __int64 val){if(jstInt == this->Type)this->Value.IntVal = val;}
 void  _SetValBol(const bool    val){if(jstBool == this->Type)this->Value.BoolVal = val;}
 void  _SetValFlt(const double  val){if(jstFloat == this->Type)this->Value.FloatVal = val;}
 void  _SetValStr(const char*   val){if(jstString == this->Type)SetString(&this->Value.StringVal,val,0);}

 void  SetValInt(const int     val){this->Value.IntVal = val;}     // TODO: Switch type on access?
 void  SetValInt(const __int64 val){this->Value.IntVal = val;}
 void  SetValBol(const bool    val){this->Value.BoolVal = val;}
 void  SetValFlt(const double  val){this->Value.FloatVal = val;}
 void  SetValStr(const char*   val, UINT Len=0){SetString(&this->Value.StringVal,val,Len);}

/* void  SetVal(const int     val){this->Value.IntVal = val;}
 void  SetVal(const __int64 val){this->Value.IntVal = val;}
 void  SetVal(const bool    val){this->Value.BoolVal = val;}
 void  SetVal(const double  val){this->Value.FloatVal = val;}
 void  SetVal(const char*   val){SetString(&this->Value.StringVal,val,0);}  */

 void  operator = (const int     val){this->SetValInt(val);}   
 void  operator = (const __int64 val){this->SetValInt(val);}
 void  operator = (const bool    val){this->SetValBol(val);}
 void  operator = (const double  val){this->SetValFlt(val);}
 void  operator = (const char*   val){this->SetValStr(val);}
 void  operator = (const unsigned int     val){this->SetValInt((int)val);}      
 void  operator = (const unsigned __int64 val){this->SetValInt((__int64)val);}

 operator   int()     {return this->GetValInt();}
 operator   __int64() {return this->GetValInt();}
 operator   bool()    {return this->GetValBol();}
 operator   double()  {return this->GetValFlt();}
 operator   char*()   {return this->GetValStr();}
//--------------

 int EntityFromString(CMiniStr &str, UINT Flags, int offset)  // 'this' represents an appropriate JSON entity // WARNING: Do not make it 'const CMiniStr &str' or 'operator []' will be messed up!
  {
   bool nouc   = (Flags & coNoUniCnv);
   for(;offset >= 0;)
	{
	 int tknpos = str.Pos(0x20,CMiniStr::ComparatorG,offset);
	 if(tknpos < 0)return -1; // No more tokens!
	 if(str[tknpos] == ']')return tknpos+1;  // End of Array
	 if(str[tknpos] == '}')return tknpos+1;  // End of Object
	 if(str[tknpos] == ',')  // Another Entity
	  {
	   offset = this->Add(CJSonItem())->EntityFromString(str, Flags, tknpos+1);  // Add a item as NULL item, its type will be assigned later
	   continue;
	  }
	 if((str[tknpos] == '{') || (str[tknpos] == '['))    // Convert current Entity to Array/Object
	  {
	   BYTE Clb;
	   //this->Cleanup(true);
	   if(str[tknpos] == '{'){this->Type = jstObject; Clb = '}';}
		 else {this->Type = jstArray; Clb = ']';}
	   int tknn = str.Pos(0x20,CMiniStr::ComparatorG,tknpos+1);
	   if(tknn < 0)return -1; // No more tokens!
	   if(str[tknn] == Clb){offset=tknn;continue;}        // An Empty Array/Object
	   offset = this->Add(CJSonItem())->EntityFromString(str, Flags, tknpos+1);  // Add a first item as NULL item, its type will be assigned later    // Process Children Entities
	   continue;
	  }
	 if(str[tknpos] == '\"')  // String VALUE or NAME:VALUE pair
	  {
	   int slen = JsonStrLen(&str[tknpos]);
	   int delm = str.Pos(0x20,CMiniStr::ComparatorG,tknpos+slen);  // ':' or ',' or ']'
	   if(delm < 0)return -1; // No more tokens! (This item will be lost!(Or else need parent checking to determine its type))
	   if((str[delm] == '}')||(str[delm] == ']')||(str[delm] == ',')){SetString(&this->Value.StringVal,JsonStrToNormalStr(&str[tknpos],slen,nouc));this->Type=jstString;return delm;} // This is a String Item of Array/Object, Set its Value and return to a parent Array/Object
	   if(str[delm] == ':'){SetString(&this->Name,JsonStrToNormalStr(&str[tknpos],slen,nouc));offset=delm+1;continue;}  // Continue to Value Processing
	   return -1; // Wrong Token! (Break parsing)
	  }

	// --- true, false, null, NUMBER ---
	 if(ICIsSameASCII("true", &str[tknpos]))
	  {
	   this->Type = jstBool;
	   this->Value.BoolVal = true;
	   return tknpos+4;
	  }
	 if(ICIsSameASCII("false", &str[tknpos]))
	  {
	   this->Type = jstBool;
	   this->Value.BoolVal = false;
	   return tknpos+5;
	  }
	 if(ICIsSameASCII("null", &str[tknpos]))
	  {
	   this->Type = jstObject;  // jstNULL   // Only objects can be null?  // And by doing that we probably broke the JSON compatibility :)
	   this->Value.IntVal = 0;
	   return tknpos+4;
	  }

	 int  nctr = 0;
	 bool nflt = false;
	 bool nhex = false;
	 BYTE Number[128];
	 if(str[tknpos] == '<')	  // Out of scoping concept here, need a separate function for a numberic value retrieval  // TODO: Sequence of some props?
	  {
	   bool started = false;
	   for(char val = str[++tknpos];val;val = str[++tknpos])
		{
		 if(!(((val >= '0')&&(val <= '9'))||((val >= 'a')&&(val <= 'z'))||((val >= 'A')&&(val <= 'Z')))){if(started)break; else continue;}
		   else started = true;
		 if(nctr < (sizeof(Number)-1))Number[nctr++] = val;
		}
	   for(char val = str[tknpos];val;val = str[++tknpos])if(val == '>'){tknpos++;break;}
	   Number[nctr] = 0;
	   this->Data = (PVOID)HexStrToDW((LPSTR)&Number,8);	  // User Data	// NOTE: Now used PVOID an trimmed to DWORD on x32
	  }

	 nctr = 0;
	 if(str[tknpos] == '$')
	  {
	   tknpos++;
	   nhex = true;
	   for(char val = str[tknpos];val;nctr++,val = str[tknpos+nctr])
		{
		 if(!(((val >= '0')&&(val <= '9'))||((val >= 'a')&&(val <= 'z'))||((val >= 'A')&&(val <= 'Z'))))break;
		 if(nctr < (sizeof(Number)-1))Number[nctr] = val;
		}
	  }
	   else
		{
	 for(char val = str[tknpos];val;nctr++,val = str[tknpos+nctr])
	  {
	   if(((val < '0')||(val > '9'))&&(val != '+')&&(val != '-'))
		{
		 if((val == '.')||(val == 'e')||(val == 'E'))nflt = true;
		  else break;
		}
	   if(nctr < (sizeof(Number)-1))Number[nctr] = val;
	  }
		}
	 if(!nctr)return -1;  // Wrong Token! (Break parsing)
	 /*if(nctr >= sizeof(Number))Number[sizeof(Number)-1] = 0;
	   else*/ Number[nctr] = 0;

	 if(nflt)
	  {
	   this->Type = jstFloat;
	 //  this->Value.FloatVal = atof((LPSTR)&Number);  //strtod((LPSTR)&Number,NULL); // atof((LPSTR)&Number);
	   return tknpos+nctr;
	  }
	   else
		{
		 if(nhex){this->Value.IntVal = HexStrToNum<UINT64>((LPSTR)&Number); this->Type = jstHexInt;}                    // Must support 64 bit
		   else {this->Value.IntVal = DecStrToNum<UINT64>((LPSTR)&Number); this->Type = jstInt;}   // TODO: Test signed numbers
		 return tknpos+nctr;
		}
   }         
   return -1;
  }

//--------------
 int FromString(const CMiniStr &str, UINT Flags=coNone, BYTE DecKey=0)
  {
   if(str.Length() < 3)return -1;
   this->Cleanup(true);
   int soffs = 0;
   int btype = IsBinaryEncrypted(str.c_data());
   if(btype >= 0)
	{
	 if(btype == 1)BinDecrypt(&str.c_data()[4], str.Length()-4, DecKey); // NOTE: Decrypts until end of the string
     soffs = EntityFromBinary(*(CMiniStr*)&str, 4);            // Const hack!
	}
	 else soffs = -this->EntityFromString(*(CMiniStr*)&str, Flags, 0);  // Const hack!
   return soffs;
  }
//--------------
 void EntityToString(CMiniStr &str, UINT Flags, int indent)
  {
   bool nouc   = (Flags & coNoUniCnv);
   bool format = (Flags & coFormat);
   bool scopedtp  = false;
   bool knowntype = true;
   if(format){if(str.Length())str += "\r\n";str.AddChars(0x20,indent);}
   if(this->Name){str += NormalStrToJsonStr(this->Name,0,nouc); str.AddChars(':');}   // Or check if Parent is an Object 
   switch(this->Type)
	{
	 case jstString:
	  str += NormalStrToJsonStr(this->Value.StringVal,0,nouc);
	  break;
	 case jstNULL:
	  str += "null";
	  break;
	 case jstBool:
	  if(this->Value.BoolVal)str += "true";
		else str += "false";
	  break;
	 case jstInt:
	  str += (int)this->Value.IntVal;
	  break;
	 case jstHexInt:
	  {
	   BYTE Buf[32];
	   LPSTR Val = ConvertToHexStr((UINT64)this->Value.IntVal, -1, (LPSTR)&Buf[1], true);
	   Val--;
	   *Val = '$';
	   Buf[17] = 0;
	   str += Val;
	  }
	  break;
	 case jstFloat:
	  {
	   BYTE Buf[128];
	 //  gcvt(this->Value.FloatVal,32,(LPSTR)&Buf);  // May skikp the '.'!
	   for(int ctr=0;Buf[ctr] != '.';ctr++){if(!Buf[ctr]){Buf[ctr]='.';Buf[ctr+1]='0';Buf[ctr+2]=0;break;}} // By presence of '.' we will detect type of this entry
	   str += (LPSTR)&Buf;
	  }
	   break;
	 case jstObject:
       if(!this->Value.Items){str += "null"; break;}     // ???????????????????????? Upd
	 case jstArray:
	  {
   if(this->Data && (Flags & coWrUsrData))	
    {
	 BYTE Buf[32];
	 LPSTR Val = ConvertToHexStr((UINT64)this->Data, -1, (LPSTR)&Buf[2], true);
	 Val -= 2;
	 *Val = '<';
	 Val[1] = '$';
	 Buf[18] = '>';
	 Buf[19] = 0;
	 str += Val;
	}
	   BYTE BOpn,BCls;
	   if(this->Type == jstArray){BOpn='[';BCls=']';}
		 else {BOpn='{';BCls='}';}
	   str.AddChars(BOpn);
	   indent++;
	   for(CJSonItem* itm = this->First();itm;itm=this->Next(itm))
		{
		 itm->EntityToString(str,Flags,indent);
		 if(itm != this->Last())str.AddChars(',');
		}
	   indent--;
	   if(format){str += "\r\n";str.AddChars(0x20,indent);}
	   str.AddChars(BCls);
	   scopedtp = true;
	  }
	   break;
	 default: knowntype = false;
	}
   if(this->Data && !scopedtp && knowntype && (Flags & coWrUsrData))	// TODO: For jstArray and jstObject place this prop before brackets - will look nicely	  // Do not output a NULL props? Anyway they are initialized to NULL
    {
	 BYTE Buf[32];
	 LPSTR Val = ConvertToHexStr((UINT64)this->Data, -1, (LPSTR)&Buf[3], true);
	 Val -= 3;
	 *Val = 0x20;
	 Val[1] = '<';
	 Val[2] = '$';
	 Buf[19] = '>';
	 Buf[20] = 0;
	 str += Val;
	}
  }
//--------------
 void ToString(CMiniStr &str, UINT Flags=coNone) // The resulting String is copied?
  {
   EntityToString(str,Flags,0);
  }
//---------------------------------------------------------------------------
 static int _fastcall IsBinaryEncrypted(PBYTE bindat)  // -1 = , 0 = Not Encrypted
  {
   if(*((PDWORD)bindat) == BINSIG)return 0;   // Not encrypted
   if((*((PDWORD)bindat) & 0x80808080)==0x80808080)return 1;  // Encrypted
   return -1; // Not a Binary
  }
//---------------------------------------------------------------------------
static UINT _fastcall CountedStrLen(LPSTR str){return (str && *str)?(((PDWORD)str)[-1]):(0);}
//---------------------------------------------------------------------------
static long _fastcall CharToHex(BYTE val)
{
 if((val >= 0x30)&&(val <= 0x39))return (val - 0x30);	   // 0 - 9
 if((val >= 0x41)&&(val <= 0x46))return (val - (0x41-10)); // A - F
 if((val >= 0x61)&&(val <= 0x66))return (val - (0x61-10)); // a - f
 return -1;
}
//--------------
static WORD _fastcall HexToChar(BYTE Value, bool upcase=false)
{
 BYTE ucse   = ((upcase)?(0):(0x20));
 WORD Result = 0;
 for(int ctr=1;ctr >= 0;ctr--)
  {
   BYTE Tmp = (Value & 0x0F);
   if(Tmp < 0x0A)((PBYTE)&Result)[ctr] |= 0x30+Tmp;
	 else ((PBYTE)&Result)[ctr] |= 0x37+ucse+Tmp;
   Value  >>= 4;
  }
 return Result;
}
//---------------------------------------------------------------------------
static bool _fastcall ICIsSameASCII(const char* StrA, const char* StrB) // StrB must fully contain StrA
{
 for(;*StrA && *StrB;StrA++,StrB++)if((*StrA|0x20)!=(*StrB|0x20))return false;
 return !*StrA;
}
//--------------
static int _fastcall JsonStrLen(const char* str) // Must be in quotes! // Parses all matching qoutes
{
 bool QStat = (*str == '\"'); // == 1 if the string in qoutes
 int  Len   = QStat;
 for(str++;*str && QStat;str++,Len++)
  {
   if((str[0] == '\"')&&(str[-1] != '\\'))QStat = !QStat;
  }
 return (QStat)?(0):(Len); // Return -1?
}
//--------------
// TODO: Make an optional JSON mode without usage of this slow function
//
static CMiniStr _fastcall JsonStrToNormalStr(LPCSTR str, int len=0, bool nouc=false) // Must be in quotes! // Parses all matching qoutes
{
 if(!len)len = lstrlen(str);
 if((len < 2)||(str[0] != '\"')||(str[len-1] != '\"'))return "";  // String must be in quotes
 len-=2; // Exclude quotes
 str++;
 PWSTR rwstr = (PWSTR)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,(len*sizeof(WCHAR))+16);
 int   Count = 0;
 for(;*str && (len > 0);str++,len--,Count++)
  {
   WCHAR Val = (BYTE)*str;
   if(Val == '\\')
	{
	 str++;
	 len--;
	 Val = (BYTE)*str;
	 if(!Val || (len <= 0))break;  // End of String
	 switch(Val)
	  {
	   case '\"':
	   case '\\':
	   case '/':
		break;
	   case 'b':
	     Val = '\b';
	    break;
	   case 'f':
		 Val = '\f';
	    break;
	   case 'n':
	     Val = '\n';
	    break;
	   case 'r':
	     Val = '\r';
	    break;
	   case 't':
	     Val = '\t';
	    break;
	   case 'u':
	    {
		 Val = 0;
		 long dA = CharToHex(str[1]);
		 if(dA < 0)break;
		 Val |= dA;
		 long dB = CharToHex(str[2]);
		 if(dB < 0)break;
		 Val <<= 4;
		 Val  |= dB;
		 long dC = CharToHex(str[3]);
		 if(dC < 0)break;
		 Val <<= 4;
		 Val  |= dC;
		 long dD = CharToHex(str[4]);
		 if(dD < 0)break;
		 Val <<= 4;
		 Val  |= dD;
		 str += 4;
		 len -= 4;
		}
		 break;
	   default: len = 0;  // Break 'for' loop
	  }
	}
//	 else if(Val >= 0x80)Val += 0x350;  // A hack to keep russian chars valid
   rwstr[Count] = Val;
  }
 CMiniStr Temps;
 Temps.AddChars(0,Count);
 rwstr[Count] = 0;
 if(!nouc)len = WideCharToMultiByte(CP_ACP,0,rwstr,Count,(LPSTR)Temps.c_data(),Temps.Length(),NULL,NULL);
   else for(len = 0;len<Count;len++)Temps.c_data()[len] = rwstr[len];   // Drops extra wide bytes!
 HeapFree(GetProcessHeap(),0,rwstr);
 return Temps;
}
//--------------
// TODO: Make an optional JSON mode without usage of this slow function
//
static CMiniStr _fastcall NormalStrToJsonStr(LPCSTR str, int len=0, bool nouc=false)
{
 if(!len)len = lstrlen(str);
 PWSTR rwstr = (PWSTR)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,(len*sizeof(WCHAR))+16);  
 LPSTR anstr = (LPSTR)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,(len*6)+16);   // As if all chars will be converted to numbers
 int wlen    = 0;
 int Count   = 0;
 if(!nouc)wlen = MultiByteToWideChar(CP_ACP,0,str,len,rwstr,len+1);     // It Should be UTF-8 ?      // TODO: Allow data as bytes and strings as UTF8  // Add Binary type as string
   else for(;wlen<len;wlen++)*((PWORD)&rwstr[wlen]) = (BYTE)str[wlen];  // Direct copy, no codepage conversion
 anstr[Count++] = '\"';   // Opening quote
 for(int ctr = 0;ctr < wlen;ctr++,Count++)
  {
   WCHAR Val = rwstr[ctr];
   switch(Val)
	{
	 case '\"':
	 case '\\':
	 case '/':
	   anstr[Count++] = '\\';
	   break;
	 case '\b':
	   Val = 'b';
	   anstr[Count++] = '\\';
	   break;
	 case '\f':
	   Val = 'f';
	   anstr[Count++] = '\\';
	   break;
	 case '\n':
	   Val = 'n';
	   anstr[Count++] = '\\';
	   break;
	 case '\r':
	   Val = 'r';
	   anstr[Count++] = '\\';
	   break;
	 case '\t':
	   Val = 't';
	   anstr[Count++] = '\\';
	   break;
	}
   if(Val < 0x20)Val = 0x20;
	 else if(Val > 0xFF)
	  {
	   anstr[Count++] = '\\';
	   anstr[Count++] = 'u';
	   *((PWORD)&anstr[Count]) = HexToChar(Val >> 8);
	   Count += 2;
	   *((PWORD)&anstr[Count]) = HexToChar(Val & 0x00FF);
	   Count += 1;  // +1 by 'for'
	   continue;
	  }
   anstr[Count] = Val;
  }
 anstr[Count]   = '\"';  // Closing quote
 anstr[++Count] = 0;
 HeapFree(GetProcessHeap(),0,rwstr);
 CMiniStr Temps = anstr; // COPY!!!
 HeapFree(GetProcessHeap(),0,anstr); 
 return Temps;
}
//--------------
 // Set 'offset' at position after BINSIG
 int EntityFromBinary(CMiniStr &str, int offset)  // 'this' represents an appropriate JSON entity // WARNING: Do not make it 'const CMiniStr &str' or 'operator []' will be messed up!
  {
   PBYTE BinDat = str.c_data();
   BYTE  BFlags = BinDat[offset++];
   BYTE  BSize  = 0;
   int   BLeft  = str.Length() - offset;
   this->Type   = EJSonType(BFlags & 0x0F);
   if(BFlags & bfNamePresent)
	{
	 UINT ssize = BinDat[offset++] + 1;
	 SetString(&this->Name,(const char*)&BinDat[offset],ssize);
	 offset += ssize;
	}
   if(BFlags & (bfDataPresent|bfValuePresent))BSize = BinDat[offset++];
   if(BFlags & bfDataPresent)
	{
	 UINT dsize = BSize >> 4;
	 this->Data = ValueFromBytes<PVOID>(&BinDat[offset], dsize);
	 offset += dsize;
	}
   switch(this->Type)
	{
	 case jstBool:
	   this->Value.BoolVal = BFlags & bfBoolValue;
	  break;
	 case jstInt:
	 case jstHexInt:
	  {
	   UINT dsize = (BSize & 0x0F);
	   this->Value.IntVal = ValueFromBytes<UINT64>(&BinDat[offset], dsize);
	   offset += dsize;
	  }
	  break;
	 case jstFloat:
	  {
	   UINT dsize = (BSize & 0x0F);
	   this->Value.FloatVal = ValueFromBytes<double>(&BinDat[offset], dsize);
	   offset += dsize;
	  }
	   break;
	 case jstArray:
	 case jstObject:
	  {
	   UINT dsize = (BSize & 0x0F);
	   UINT onumr = ValueFromBytes<UINT32>(&BinDat[offset], dsize);
	   offset += dsize;
	   for(;onumr;onumr--)offset = this->Add(CJSonItem())->EntityFromBinary(str, offset);
       Sleep(1);
	  }
	   break;
	 case jstString:
	  {
	   UINT dsize = (BSize & 0x0F);
	   UINT ssize = ValueFromBytes<UINT32>(&BinDat[offset], dsize);
	   offset += dsize;
	   SetString(&this->Value.StringVal,(const char*)&BinDat[offset],ssize);
	   offset += ssize;
	  }
	  break;
	}   
   return offset;
  }
//--------------
 // DataSize is used for encryption
 // Set 'offset' at position after BINSIG
 int EntityToBinary(CMiniStr &str, int offset)
  {
   BYTE BFlags = this->Type;
   BYTE* PCtr;
   int ValLen;
   int NamLen = this->GetNameLen();
   int DatLen = CountValueBytes(&this->Data);
   if(this->Type != jstBool)ValLen = CountValueBytes(&this->Value.IntVal);
	else
	 {
	  ValLen = 0;
	  if(this->Value.BoolVal)BFlags |= bfBoolValue;
	 }
   int PreLen = DatLen+NamLen+((bool)NamLen)+((bool)(ValLen|DatLen))+1; // Except size of value itself

   str.SetLength(offset+PreLen);
   PBYTE BDPtr = str.c_data();  // Valid until next resize
   BYTE* Flags = &BDPtr[offset++];
   if(NamLen){BFlags |= bfNamePresent; BDPtr[offset++] = NamLen-1; memcpy(&BDPtr[offset],this->GetName(),NamLen); offset += NamLen;}
   if(DatLen || ValLen)
	{
	 PCtr  = &BDPtr[offset++];  // Size byte of Data/Value // Value size is used to define size of counters for String/Oblect/Array elements
	 *PCtr = 0;
	 if(DatLen){BFlags |= bfDataPresent; *PCtr |= (DatLen << 4); memcpy(&BDPtr[offset],&this->Data,DatLen); offset += DatLen;}  // Lowest bytes comes first in 'Data'!
	 if(ValLen)BFlags  |= bfValuePresent;
	}
   *Flags = BFlags;
   if(!ValLen)return offset;	// NULL or BOOL value

   switch(this->Type)
	{
	 case jstBool:
	   // Must never get here - BOOL value has been already assigned
	  break;
	 case jstInt:
	 case jstHexInt:
	  {
	   ValLen = CountValueBytes(&this->Value.IntVal);
	   *PCtr |= (ValLen & 0x0F);
	   str.SetLength(offset+ValLen);   // NOTE: Very small reallocation
	   BDPtr  = str.c_data();
	   memcpy(&BDPtr[offset],&this->Value.IntVal,ValLen); offset += ValLen;   // Write number of child objects
	  }
	  break;
	 case jstFloat:
	  {
	   ValLen = CountValueBytes(&this->Value.FloatVal);
	   *PCtr |= (ValLen & 0x0F);
	   str.SetLength(offset+ValLen);   // NOTE: Very small reallocation
	   BDPtr  = str.c_data();
	   memcpy(&BDPtr[offset],&this->Value.FloatVal,ValLen); offset += ValLen;   // Write number of child objects
	  }
	   break;
	 case jstArray:
	 case jstObject:
	  {
	   UINT32 SVLen = this->mCount;  // TODO: Move this counter to Items memory block
	   ValLen = CountValueBytes(&SVLen);
	   *PCtr |= (ValLen & 0x0F);
	   str.SetLength(offset+ValLen);   // NOTE: Very small reallocation
	   BDPtr  = str.c_data();
	   memcpy(&BDPtr[offset],&SVLen,ValLen); offset += ValLen;   // Write number of child objects
	   for(CJSonItem* itm = this->First();itm;itm=this->Next(itm))offset = itm->EntityToBinary(str, offset);
	  }
	   break;
	 case jstString:
	  {
	   UINT32 SVLen = CountedStrLen(this->Value.StringVal);   // Value size specifies size of string field
	   ValLen = CountValueBytes(&SVLen);
	   *PCtr |= (ValLen & 0x0F);
	   str.SetLength(offset+ValLen+SVLen);
	   BDPtr  = str.c_data();
	   memcpy(&BDPtr[offset],&SVLen,ValLen);                offset += ValLen;   // Write string size
	   memcpy(&BDPtr[offset],this->Value.StringVal,SVLen);  offset += SVLen;    // Write the string itself
	  }
	  break;
	}     
   return offset;
  }
//--------------
 UINT ToBinary(CMiniStr &str, BYTE EncKey=0)
  {
   UINT cptr = str.Length();
   str.AddChars(0,4);
   *((PDWORD)&str.c_data()[cptr]) = BINSIG;
   UINT FullSize = this->EntityToBinary(str, str.Length());
   if(EncKey)
	{
	 PBYTE dptr = str.c_data();
	 *(PDWORD)&dptr[cptr] = (DWORD)this | 0x80808080;  // Encryption flag on a random Signature
	 BinEncrypt(&dptr[cptr+4], str.Length()-(cptr+4), EncKey);
	}
   return FullSize;
  }
//--------------

};
//----------------------------------------------------------------------------
template <typename T> static CJSonItem* EnsureJsnParam(T val, LPSTR Name, CJSonItem* Owner)  // If parameter don`t exist - creates it and sets to 'Val' else just returns pointer to it
{
 CJSonItem* res = Owner->Get(Name);
 if(!res)res = Owner->Add(CJSonItem((T)val,Name));
 return res;
}
//---------------------------------------------------------------------------
template <typename T> static CJSonItem* SetJsnParamValue(T val, LPSTR Name, CJSonItem* Owner)
{
 CJSonItem* res = Owner->Get(Name);
 if(!res)res = Owner->Add(CJSonItem((T)val,Name));
  else (*res) = (T)val;
 return res;
}
//---------------------------------------------------------------------------
#endif
