

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

#ifndef DynArrayH
#define DynArrayH

//---------------------------------------------------------------------------
// Do not calls constructors for objects
//
template<typename T, int PreAll=1> class CDynArray  // Stupid compiler cannot parse CDynBuffer<C, (PreAll*sizeof(T))> - the problem with sizeof(T) // template<typename T, int PreAll=1, int dummy=sizeof(T)> class CDynArray : public CDynBuffer<T, (PreAll*dummy)>
{
 T*   Data;
 UINT mCount;

public:
 CDynArray(const CDynArray &Arr){this->Assign(str);}
 CDynArray(void){Data=NULL;mCount=0;}   // Constructor of CDynBuffer will be called
 ~CDynArray(){this->ResizeFor(0);}      // Destructor of CDynBuffer will be called

//----------------------
 T* ResizeFor(UINT Cnt)
 {                                                
  if(Cnt && this->Data)this->Data = (T*)HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,this->Data,(Cnt*sizeof(T))+8);
	else if(!this->Data)this->Data = (T*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,(Cnt*sizeof(T))+8);
	  else if(!Cnt && this->Data){HeapFree(GetProcessHeap(),0,this->Data);this->Data=NULL;}
  this->mCount = Cnt;
  return this->Data;
 }
 //----------------------
 T* GetData(void){return this->Data;}
 T* First(void) {return (this->mCount)?(&this->Data[0]):(NULL);}                   // Uneffective for an Arrays
 T* Last(void)  {return (this->mCount)?(&this->Data[this->mCount-1]):(NULL);}      // Uneffective for an Arrays
 T* Next(T* ent){return (this->IsInArray(++ent))?(ent):(NULL);}                      // Uneffective for an Arrays		// Returns NULL at the End	of List
 T* Prev(T* ent){return (this->IsInArray(--ent))?(ent):(NULL);}                      // Uneffective for an Arrays		// Returns NULL at the End	of List
//----------------------
 T* Assign(const CDynArray &Arr, UINT Cnt=0)  // Can assign to Cnt elements fron array Arr of all content of Arr
 {
  if(!Cnt || (Cnt > Arr.mCount))Cnt = Arr.mCount;
  if(!this->ResizeFor(Cnt))return NULL;
  memcpy(this->Data, Arr.Data(), (sizeof(T)*Cnt));
  return this->Data;
 }
//----------------------
 T* Assign(const T* Arr, UINT Cnt=1)    // Pointer to N element of Array, 'arr' can be NULL
 {
  if(!this->ResizeFor(Cnt))return NULL;
  if(Arr)memcpy(this->Data, (PVOID)Arr, (sizeof(T)*Cnt));
  return this->Data;
 }
//----------------------
 T* Append(const CDynArray &Arr)  // TODO: redirect to 'Append(const T* Arr, UINT Cnt=1)'
 {
  UINT Count = this->mCount;
  if(!this->ResizeFor(Arr.mCount+Count))return NULL;
  T* start = &this->Data[Count];
  memcpy(&this->Data[Count], Arr.Data(), Arr.Size());  // Was (sizeof(T)*Arr.mCount)
  return start;
 }
//----------------------
 T* Append(const T* Arr, UINT Cnt=1)      // 'arr' can be NULL
 {
  UINT Count = this->mCount;
  if(!this->ResizeFor(Cnt+Count))return NULL;
  T* start = &this->Data[Count];
  if(Arr)memcpy(&this->Data[Count], (PVOID)Arr, (sizeof(T)*Cnt));
  return start;
 }
//----------------------
 T* Insert(const CDynArray &Arr, UINT Index)    // TODO: redirect to 'Insert(const T* Arr, UINT Index, UINT Cnt=1)'   // Inserts BEFORE the 'Index'
 {
  UINT Count = this->mCount;
  if(Index >= Count)return this->Append(Arr.GetData(),Arr.mCount);
  if(!this->ResizeFor(Arr.Count+this->mCount))return NULL;
  T* start = &this->Data[Index];
  memmove(&this->Data[Index+Arr->mCount], &this->Data[Index], (sizeof(T)*(Count-Index)));  // Get a space for new elements
  memcpy(&this->Data[Index], Arr.Data(), Arr.Size());    // Was (sizeof(T)*Arr.mCount)
  return start;
 }
//----------------------
 T* Insert(const T* Arr, UINT Index, UINT Cnt=1)      // 'arr' can be NULL     // Inserts BEFORE the 'Index'
 {
  UINT Count = this->mCount;
  if(Index >= Count)return this->Append(Arr,Cnt);
  if(!this->ResizeFor(Cnt+Count))return NULL;
  T* start = &this->Data[Index];
  memmove(&this->Data[Index+Cnt], &this->Data[Index], (sizeof(T)*(Count-Index)));  // Get a space for new elements
  if(Arr)memcpy(&this->Data[Index], (void*)Arr, (sizeof(T)*Cnt));
  return start;
 }
//----------------------
 void Delete(UINT Pos, UINT Cnt=1)
 {
  UINT Count = this->mCount;
  if(Pos >= Count)return;
  if((Pos+Cnt) >= Count)Cnt = (Count-Pos);   // Trim the Array`s tail
	else memmove(&this->Data[Pos],&this->Data[Pos+Cnt],(Count-(Pos+Cnt))*sizeof(T));   // Fix a hole in the array
  this->ResizeFor(Count-Cnt);   // Shink the used bytes/allocated bytes
 }
//---------------------
 UINT IndexOf(T* Itm)
  {
   if(!this->IsInArray(Itm))return -1;
   return (Itm - &this->Data[0]);  // Result is a Number of Objects, not a bytes!
  } 
//----------------------
 bool SwapItems(UINT IIdxA, UINT IIdxB)
  {
   if((IIdxA > this->mCount)||(IIdxB > this->mCount))return false;
   this->SwapMem(this->Data[IIdxA], this->Data[IIdxB], sizeof(T)); 
   return true;
  }
//----------------------
 template<typename S> bool SortByKeyProc(UINT IIdxA, UINT IIdxB, S IKeyA, S IKeyB)    // Sort by address
  {
   if((IKeyB < IKeyA) && this->SwapItems(IIdxA,IIdxB))return true;
   return false;
  }
//----------------------
 bool IsInArray(T* Itm){return ((Itm >= &this->Data[0])&&(Itm < &this->Data[this->mCount]));}
//----------------------
 void WriteElements(T* Dst, T* Src, UINT Cnt){memmove(Dst,Src,(sizeof(T)*Cnt));}
//----------------------
 void Clear(void){this->ResizeFor(0);}    // Do some more job here?
//----------------------
 UINT Count(void){return this->mCount;}   
//----------------------
 bool IsEmpty(void)  const {return !this->mCount;}
//----------------------
// bool IsEqual(const CDynArray &Arr)  const {return !this->mCount;} // in parent class!!!
//----------------------
 operator const T*() const {return this->Data;}
//----------------------
// void       operator  = (const CDynArray &Arr){this->Assign(&Arr);}    // !!!!!!!!!!!!!!    What is assigned (Array or an element?)
//----------------------
 CDynArray& operator += (const CDynArray &Arr){this->Append(&Arr);return *this;}
//----------------------
 T&         operator [] (int index){return this->Data[index];}   // No bounds check, work as static array !!!??? // What to do if OutOfBounds (Do not throw exceptions!)  // How to acces to Array of Arrays?
//----------------------
 bool       operator == (const CDynArray &Arr) {return this->IsEqual(&Arr);}
//----------------------

};
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
#endif
