

#pragma once

#ifndef MemChainH
#define MemChainH


//---------------------------------------------------------------------------
// Memory management primitives
/*
 - No linear items copy support
 - No Removing a separate items support, only an entire blocks
 + Pointers to an items are always pertained 
 + Fast allocation

 Best works with SAllocLL
 'CMemChain' is not suppose to be created on stack!
*/
template<typename Alloca=SAllocLL<>, typename D=char[0]> class CMemChain    // Add Block Descr and funt to get a block descr from any address in it     
{
typedef CMemChain<Alloca,D> TSlf;     // SMemChain<T,Alloca>

 TSlf* Prev;
 TSlf* Next;
 UINT  Size;   // Full size of this memory chunk

public:
  D    Data;   // User defined data (By default its size is 0)  // i.e. this is a struct which describes the buffer and have 'BYTE Data[0]' at the end

//---------------------------------------------------------------------------
static TSlf* Create(UINT BlkSize=1)  // First allocation can request more than MEMGRANSIZE
{
 TSlf* This = (TSlf*)Alloca::Allocate(BlkSize, &BlkSize);   
 This->Prev = This->Next = nullptr;
 This->Size = BlkSize;
 return This;
}
//---------------------------------------------------------------------------
TSlf* Remove(void)  // Returns pointer to a next block of the chain, NULL if this is an only block
{
 Self* Prv = this->Prev;
 Self* Nxt = this->Next;
 Alloca::Free(this);
 if(Nxt)Nxt->Prev = Prv;
 if(Prv)Prv->Next = Nxt;
 return Nxt;     
}
//---------------------------------------------------------------------------
void Destroy(void)     // Frees memory of entire chain
{
 for(TSlf* Blk = this->GetFirstBlock();Blk;)
  {
   TSlf* Nxt = Blk->Next;
   Alloca::Free(Blk);
   Blk = Nxt;
  }
}
//---------------------------------------------------------------------------
TSlf* InsertAfter(TSlf* Blk)
{
 Blk->Next  = this->Next;
 Blk->Prev  = this;
 this->Next = Blk;
 return Blk;
}
//---------------------------------------------------------------------------
TSlf* InsertBefore(TSlf* Blk)
{
 Blk->Next  = this;
 Blk->Prev  = this->Prev;
 this->Prev = Blk;
 return Blk;
}
//---------------------------------------------------------------------------
TSlf* BlkFirst(void)
{
 TSlf* Ptr = this;
 while(Ptr->Prev)Ptr = Ptr->Prev;
 return Ptr; 
}
//---------------------------------------------------------------------------
TSlf* BlkLast(void)
{
 TSlf* Ptr = this;
 while(Ptr->Next)Ptr = Ptr->Next;
 return Ptr;
}
//---------------------------------------------------------------------------
PUINT8 GetFirstAddr(void)           // In the block  // This two functions is useful if you use the block as an item array
{
 return &((PUINT8)this)[AlignFrwrd(sizeof(TSlf),sizeof(PVOID))];   // Aligned to the platform`s default
}
//---------------------------------------------------------------------------
PUINT8 GetLastAddr(UINT ItmSize=1)  // In the block
{
 return &this->GetFirstAddr()[(this->Size-AlignFrwrd(sizeof(TSlf),sizeof(PVOID))) / ItmSize];
}
//---------------------------------------------------------------------------
TSlf* BlkNext(void){return this->Next;}
TSlf* BlkPrev(void){return this->Prev;}
UINT  BlkSize(void){return this->Size;}
//----------------------
};

//---------------------------------------------------------------------------
//===========================================================================
/*
 Represents a chain of pages whthin a memory block

 NOTE: Use low level memory allocation to minimize memory usage and to use a specific features based on alignment
 NOTE: An most platforms only MEMPAGESIZE low level allocation base address alignment gurantied
 NOTE: On Each segment base there are pointer to a Block Base, and at each BlockBase there are UserData value(Align to a pointer size after UserData) 
 NOTE: An unused page have no base blk ptr on its beginning

  -Have no knowledge of deleted items
  -An item size is limited to size of MEMPAGESIZE-sizeof(CPageChain)
  -Any leftover from ItemSize at a page`s end will be wasted (Not much is an items are small)
  -All blocks are of same size that it can be aesily found their base
  -If an item not fits in rest of a block then that memory is wasted
  -For fast block`s base finding a block size can be 4096(0x1000),8192(0x2000),16384(0x4000),32768(0x8000)
  -Page cannot be released but can be reused
  +From any item`s address we can find its page base address and there a pointer to a block`s base
*/
template<typename C, typename D> struct SPBlkDesc{C PageCnt; D UsrDat;}; // This have meaning only for a first page in block  // C++: struct cannot be defined directly in a template param

template<typename U=char[0]> class CPageChain: protected CMemChain<SAllocLL<>, SPBlkDesc<int,U> >   // One page allocation mult  // All pages aligned to MEMPAGESIZE
{
typedef CPageChain<U> TSlf; 

protected:
static const int MPageSize = MEMPAGESIZE;


//---------------------------------------------------------------------------
bool IsValidPage(bool)   // But not first!
{
 return !*(TSlf**)this;
}
//---------------------------------------------------------------------------
void Invalidate(void)   // But not first!  // How to invaqlidate first page(Of first block?)?
{
 *(TSlf**)this = nullptr;
} 
//---------------------------------------------------------------------------
TSlf* GetBlock(void)      
{
 if(this->IsFirstPage())return this;
 return *(TSlf**)this;
}
//---------------------------------------------------------------------------
TSlf* BlkPageNext(void)       // In current block only!
{
 TSlf* Nxt = (TSlf*)&((PUINT8)this)[MPageSize];
 TSlf* Blk = this->GetBlock();
 if(Nxt >= ((PUINT8)Blk)[Blk->Size])return nullptr;
 return Nxt;
}
//---------------------------------------------------------------------------
TSlf* BlkPagePrev(void)      // In current block only!
{
 TSlf* Prv = (TSlf*)&((PUINT8)this)[-MPageSize];
 if(Prv < this->GetBlock())return nullptr;
 return Prv;
}
//---------------------------------------------------------------------------
// Next page in current block
TSlf* BlkGetPage(UINT Idx)
{
 if(Idx >= this->TotalPages())return NULL;
 return (TSlf*)&((PUINT8)this->GetBlock())[Idx*MPageSize];
}
//---------------------------------------------------------------------------
// Total pages in current block(including unused)
UINT BlkTotalPages(void)  // Page size is not user defined
{
 return (this->GetBlock()->Size / MPageSize);
}
//---------------------------------------------------------------------------
UINT BlkPageIndex(void)
{
 return ((PUINT8)this - (PUINT8)this->GetBlock()) / MPageSize; 
}
//---------------------------------------------------------------------------
// Count contiguous unused pages
// 
UINT BlkCountUnusedPages(UINT FromPage)
{
 UINT PNCnt  = 0;
 UINT Total  = this->BlkTotalPages();
 if(FromPage >= Total)return 0;
 TSlf*  Blk  = this->GetBlock();
 PUINT8 PPtr = &((PUINT8)Blk)[FromPage*MPageSize];
 for(UINT ctr=FromPage;ctr < Total;ctr++,PNCnt++,PPtr+=MPageSize)
  {
   if(((TSlf*)PPtr)->IsValidPage())break;
  }
 return PNCnt;
}
//---------------------------------------------------------------------------
// Will free the block if no more used pages left in it
// It is allowed to exclude already excluded paages as batch
// Returns number of used pages in the block
//
UINT BlkExcludePages(UINT Count=1)
{
 UINT PNext  = this->BlkTotalPages() - this->BlkPageIndex(); 
 PUINT8 PPtr = (PUINT8)this;
 TSlf*  Blk  = this->GetBlock();
 if(Count > PNext)Count = PNext;
 for(UINT ctr=0;(ctr < Count) && (Blk->Data.PageCnt > 0);ctr++,PPtr+=MPageSize)
  {
   if(((TSlf*)PPtr)->IsValidPage()){((TSlf*)PPtr)->Invalidate(); Blk->Data.PageCnt--;}
  }
 if(Blk->Data.PageCnt > 0)return Blk->Data.PageCnt;
 Blk->Remove();
 return 0;     // No page left - block removed
}
//---------------------------------------------------------------------------
// Try to reuse 'Count' of contiguous pages from current block (contiguous for enumeration, not for direct data storage)
// Not allowed to reuse already used pages
// Returns number of reused pages
//
TSlf* BlkReusePages(UINT Count=1, PUINT Reused=nullptr, bool FailIfNotEnough=true)
{
 UINT PNCnt  = 0;
 UINT Total  = this->BlkTotalPages();
 TSlf*  Blk  = this->GetBlock();
 TSlf*  Tmp  = nullptr; 
 TSlf* PBase = nullptr; 
 PUINT8 PPtr = (PUINT8)Blk;
 for(UINT ctr=0,pctr=0;ctr < Total;ctr++,PPtr+=MPageSize)
  {
   TSlf* pPage = (TSlf*)PPtr; 
   if(pPage->IsValidPage())
    {
     if(pctr > PNCnt){PNCnt = pctr; PBase = Tmp; Tmp = nullptr;}
     pctr = 0;
     continue;
    }
   pctr++;
   if(!Tmp)Tmp = pPage;
  }
 if((PNCnt < Count) && FailIfNotEnough){PBase = nullptr; PNCnt = 0;}
 if(Reused)*Reused = PNCnt;
 return PBase;
}
//---------------------------------------------------------------------------


public:
//---------------------------------------------------------------------------
// Creates a first block of pages
//
static TSlf* Create(UINT ItmNum=0, UINT ItmSize=1)
{

}
//---------------------------------------------------------------------------
// Allocates more pages
//
TSlf* AddPages(UINT ItmNum=1, UINT ItmSize=1)
{

}
//---------------------------------------------------------------------------

//void RemovePages()
//---------------------------------------------------------------------------
bool IsFirstPage(void)
{
 if(!this->Prev)return true;    // This page is first in block and this block is first in chain (Other pages always have first PVOID to point to their block)
 if((PVOID)this->Prev->Next == (PVOID)this)return true;  // The 'Prev' is really a Prev block of chain (For non first page 'Prev' will be its block base and 'Next' will be a next block)
 return false;
}
//---------------------------------------------------------------------------

UINT PageSize(void)    // Size af available data not including any service fields
{
 return (this->IsFirstPage())?(MEMPAGESIZE - sizeof(TSlf)):(MEMPAGESIZE-sizeof(PVOID));
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------


};
//---------------------------------------------------------------------------
/*

template<typename T, typename U=char[0]> class CPageChain: protected CMemChain<SAllocLL<>, SPBlkDesc<UINT,U> >   // One page for now  

{
//static const int MBlkSize = sizeof(T)
//static const int ItmInBlk = MEMPAGESIZE / sizeof(T);

T* GetFirstItm(void)        // In the page
{


 return (T*)&((PUINT8)this)[sizeof(PVOID)]; // PVOID at beginning of each page points to block address (first page)
}
//---------------------------------------------------------------------------
T* GetLastItm(void)         // In the block
{
 return this->GetFirstItm()[(this->Size / sizeof(T)) - 1];
}
//---------------------------------------------------------------------------

public:
static const UINT ERRNOSUCHITM = -1;  // -1(0xFFFFFFFF) is for items not in this Chain

//---------------------------------------------------------------------------
static TSlf* Create(UINT ItmCount=0)
{

}
//---------------------------------------------------------------------------
// Allocates space for 'Count' items
// Applicable to any block of the chain
// Returns a pointer only to a First Item, other is not gurantied to be on same block if no ForceFit was specified
// ForceFit will guaranty that Count will be allocated on same block even if a memory will be wasted(This can break a sequential item storing if ypu later try to allocate a number of iter which will fit in the block`s unused memory)
//
T* Allocate(UINT Count=1, bool ForceFit=false)        
{
 UINT CLeft = (this->FullSize / sizeof(T)) - this->ItmCount;
 if(Count <= CLeft)
  {
   CLeft = Count;
   Count = 0;
  }
   else Count -= CLeft;
 UINT Index = this->ItmCount;
 if(CLeft)
  {
   UINT Total = (this->ItmCount + CLeft) * sizeof(T);
   if(AlignFrwrd((this->ItmCount * sizeof(T)),Alloca::MAllocGran) != AlignFrwrd(Total,Alloca::MAllocGran))Alloca::Expand(this, Total, this->FullSize);   // Commit more pages in a reserved range
   this->ItmCount += CLeft;
  }
 if(Count)
  {
   Self* nptr = SMemChain<T,Alloca>::Create(Count);
   nptr->Prev = this;
   this->Next = nptr;
  }
 return &this->GetFirstItm()[Index];
}
//---------------------------------------------------------------------------

UINT IndexOf(T* Itm)    // Index of the item in MemChain
{
 UINT Idx = 0;
 for(Self* Ptr = this->GetFirstBlock();Ptr;Ptr=Ptr->Next)
  {
   T* First = Ptr->GetFirstItm();
   if((Itm >= First) && (Itm <= Ptr->GetLastItm()))   
    {
     Idx += (Itm - First);
     return Idx;
    }
     else Idx += Ptr->ItmCount;
  }
 return ERRNOSUCHITM; 
}
//---------------------------------------------------------------------------
T* Get(UINT Idx)
{
 for(TSlf* Ptr = this->GetFirstBlock();Ptr;Ptr=Ptr->Next)
  {
   if(Idx < Ptr->ItmCount)return &Ptr->Array[Idx];
     else Idx -= Ptr->ItmCount;
  }
 return nullptr; 
}
//---------------------------------------------------------------------------
T* GetFirst(void)
{
 return this->GetFirstBlock()->GetFirstItm();
}
//---------------------------------------------------------------------------
T* GetLast(void)
{
 return this->GetLastBlock()->GetLastItm();  // Allocated must never be 0!
}
//---------------------------------------------------------------------------
T* GetNext(T* Itm)      // Can be very slow because of 'GetBlockFor'
{
 TSlf* Blk = GetBlockFor(Itm);
 if(Itm < Blk->GetLastItm())return ++Itm;
 if(Blk->Next)return Blk->Next->GetFirstItm();
 return nullptr;
}
//---------------------------------------------------------------------------
T* GetPrev(T* Itm)      // Can be very slow because of 'GetBlockFor'
{
 TSlf* Blk = GetBlockFor(Itm);
 if(Itm > Blk->GetFirstItm())return --Itm;
 if(Blk->Prev)return Blk->Prev->GetLastItm();
 return nullptr;
}
//----------------------



}
 */


/*
// for(TSlf* Ptr = this->GetFirstBlock();Ptr;Ptr=Ptr->Next)   // Slow if there are many blocks!
//  {
//   if((Itm >= Ptr->GetFirstItm()) && (Itm <= Ptr->GetLastItm()))return Ptr;  // Checks if the item`s address in current block 
//  }
 return nullptr;
*/
//---------------------------------------------------------------------------
#endif
