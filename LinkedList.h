//---------------------------------------------------------------------------

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

#ifndef LinkedListH
#define LinkedListH

//---------------------------------------------------------------------------
template <class T> class CLinkedList
{
 struct SEntry
  {
   T  Element;
   SEntry *Prev;
   SEntry *Next;   

   void* operator new(size_t size, void *ptr){return HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,size);} 
   void  operator delete(void* mem){HeapFree(GetProcessHeap(),0,mem);}    // No normal placement delete are possible
//----------------------
  SEntry(void){this->Prev = this->Next = NULL;}
  ~SEntry(){this->Prev = this->Next = NULL;}
  }*List;

 UINT   ECount;
// HANDLE hHeap;
 static __inline SEntry* EntryFromElement(T* Element){return (SEntry*)Element;}     //{return (SEntry*)(((PBYTE)Element)-(PBYTE)&((SEntry*)(0))->Element);}

public:
 CLinkedList(void){this->ECount=0;this->List=NULL;/*this->hHeap=GetProcessHeap();*/}
 ~CLinkedList(){this->Clear();}
//----------------------
 SEntry* AllocEntry(void){return new ((void*)NULL) SEntry();}   //(SEntry*)HeapAlloc(this->hHeap,HEAP_ZERO_MEMORY,sizeof(SEntry));}    new ((void*)this->hHeap) SEntry();
//----------------------
 void FreeEntry(SEntry* Entry){delete(Entry);}
//----------------------
 UINT Count(void){return this->ECount;}
//----------------------
 void Clear(void){while(this->List && this->Remove(&this->List->Element));}
//----------------------
 T*   GetFirst(void){return (this->List)?(&this->List->Element):(NULL);}          // Don`t do checks?
 T*   GetLast(void){return (this->List)?(&this->List->Prev->Element):(NULL);}     // Don`t do checks?
 static T*   _GetNext(T* Element){return &this->EntryFromElement(Element)->Next->Element;}   // Works for ANY CLinkedList instance, but no check of end of the list
 static T*   _GetPrev(T* Element){return &this->EntryFromElement(Element)->Prev->Element;}   // Works for ANY CLinkedList instance, but no check of end of the list
//----------------------
 T*   GetNext(T* Element)            // Check List to NULL or GetHead is enough?
  {
   SEntry* entry = this->EntryFromElement(Element)->Next;  // Last is Tail
   if(entry == this->List)return NULL;    // Head again
   return &entry->Element;
  }
//----------------------
 T*   GetPrev(T* Element)            // Check List to NULL or GetTail is enough?
  {
   SEntry* entry = this->EntryFromElement(Element)->Prev; // Last is Head
   if(entry == this->List->Prev)return NULL;    // Tail again
   return &entry->Element;
  }
//----------------------
T*   GetNextCyc(T* Element)            // Check List to NULL or GetHead is enough?    // As in .NET:  NextCyclic
  {
   SEntry* entry = this->EntryFromElement(Element)->Next;  // Last is Tail
   return &entry->Element;
  }
//----------------------
 T*   GetPrevCyc(T* Element)            // Check List to NULL or GetTail is enough?  // As in .NET:  PreviousCyclic    
  {
   SEntry* entry = this->EntryFromElement(Element)->Prev; // Last is Head
   return &entry->Element;
  }
//----------------------

 T* Add(T* Element, PUINT Index=NULL){return this->Insert(Element,NULL,Index);}
 T* AddNew(PUINT Index=NULL){return this->Insert(NULL,Index);}
//----------------------
 bool MoveFirst(UINT Index)
  {
   if(!this->MoveAfter(Index, 0))return false;
   this->List = this->List->Next; // Set new entry as list head	 // The list is cycled
   return true;
  }
//----------------------
 bool MoveAfter(UINT Index, UINT IndexAfter)
  {
   SEntry* After   = NULL;
   SEntry* Current = NULL;

   T* NE1 = this->GetEntry(IndexAfter, &After);
   if(!NE1)return false;
   T* NE2 = this->GetEntry(Index, &Current);
   if(!NE2)return false;

   Current->Prev->Next = Current->Next;
   Current->Next->Prev = Current->Prev;

   Current->Prev = After;
   Current->Next = After->Next;
   After->Next->Prev = Current;
   After->Next   = Current;
   return true;
  }
//----------------------
 T* InsertFirst(PUINT Index)
  {
   T* NEl = this->Insert(NULL,Index);
   if(NEl)this->List = this->List->Prev; // Set new entry as list head	 // The list is cycled
   return NEl;
  }
//----------------------
 T* InsertFirst(T* Element, PUINT Index)
  {
   T* NEl = this->Insert(NULL,Index);
   if(NEl)
	{
	 this->List = this->List->Prev; // Set new entry as list head	 // The list is cycled
	 memcpy(NEl,Element,sizeof(T));
	}
   return NEl;
  }
//----------------------
 T* Insert(T* Element, PVOID After, PUINT Index=NULL)
  {
   T* NEl = this->Insert(After,Index);
   if(NEl && Element)memcpy(NEl,Element,sizeof(T));
   return NEl;
  }
//----------------------
 T* InsertBefore(PVOID Before, PUINT Index=NULL)
  {
  // SEntry * ent1 = this->List;

   T* NEl = this->Insert(((SEntry*)Before)->Prev,Index);
  // SEntry * ent2 = this->List;

   if(this->List == Before)this->List = ((SEntry*)Before)->Prev;
   return NEl;
  }
//----------------------
 T* Insert(PVOID After, PUINT Index=NULL)
  {
   SEntry *NewEntry = this->AllocEntry();
   if(Index)*Index  = this->ECount;
   if(!this->List)
	{
	 this->List        = NewEntry;
	 this->List->Prev  = NewEntry;   // Set end of list
	 this->List->Next  = NewEntry;
	 this->ECount      = 1;
	}
	 else
	  {
	   SEntry *AfterEn = (After)?((SEntry*)After):(this->List->Prev); // After=NULL - Add to end of list
	   AfterEn->Next->Prev = NewEntry;
	   NewEntry->Next  = AfterEn->Next;
	   NewEntry->Prev  = AfterEn;
	   AfterEn->Next   = NewEntry;
	   this->ECount++;
	  }
   return &NewEntry->Element;
  }
//----------------------
 bool Remove(UINT Index){return this->Remove(this->GetEntry(Index));}
//----------------------
 bool Remove(T* Element)
  {
   if(!this->List || !Element)return false;
   SEntry* Removing     = (SEntry*)(((PBYTE)Element)-(PBYTE)&((SEntry*)(0))->Element);
   Removing->Prev->Next = Removing->Next;
   Removing->Next->Prev = Removing->Prev;
   if(Removing == this->List)
	{
	 if(Removing->Next == this->List)this->List = NULL;  // Removing one last entry (head)
	   else this->List = Removing->Next; // Set new head
	}
   this->FreeEntry(Removing);
   this->ECount--;
   return true;
  }
//----------------------
 T* FindEntry(T* Element, PUINT Index=NULL)
  {
   SEntry* CurEn = this->List;
   for(UINT ctr=0;CurEn;ctr++)
	{
	 if(RtlCompareMemory(&CurEn->Element,Element,sizeof(T))==sizeof(T)){if(Index)*Index=ctr;return CurEn->Element;}
	 CurEn = CurEn->Next;
	 if(CurEn == this->List)break;
	}
   return NULL;
  }
//----------------------
 T* GetEntry(UINT Index)
  {
   SEntry* CurEn = this->List;
   for(UINT ctr=0;CurEn;ctr++)
	{
	 if(ctr == Index)return &CurEn->Element;
	 CurEn = CurEn->Next;
	 if(CurEn == this->List)break;
	}
   return NULL;
  }

};
//-----------------------------------------------------------------------------



//---------------------------------------------------------------------------
#endif
