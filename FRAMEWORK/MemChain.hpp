
#pragma once

#ifndef MemChainH
#define MemChainH


// Memory management primitives for a self managed containers
//---------------------------------------------------------------------------
template<ESMemPage MPLen=mp4K> class alignas(sizeof(void*)) SMemPage   
{
public:
static const unsigned int MPageSize = MPLen;            // Expected to be 4k that a page base address can be calculated by AND masking
static const unsigned int MPageAMsk = MPageSize-1;
 SMemPage* Block;    // Points to a first page of the block // A block can be of any number of pages // Will be useful for CPageChain

bool      IsPageValid(void){return (bool)this->Block;}
void      PageInvalidate(void){this->Block = nullptr;}   // But still will stay in array of pages
SMemPage* BlkPagePrev(void){return (SMemPage*)&((PUINT8)this)[-MPageSize];}  // Array-
SMemPage* BlkPageNext(void){return (SMemPage*)&((PUINT8)this)[MPageSize];}   // Array+
};
//===========================================================================
//
// Linked list of memory chunks.
// Best works with SAllocLL
// Is not supposed to be created on stack!
//
template<typename Alloca=SAllocLL<>, ESMemPage MPLen=mp4K> class alignas(sizeof(void*)) CMemChain: protected SMemPage<MPLen> 
{
typedef CMemChain<Alloca,MPLen> TSlf;    

 TSlf*   Prev;
 TSlf*   Next;
 TSlf*   First;  // For fast access in case some user data is stored there
 SIZE_T  Size;   // Used size of this memory chunk
 SIZE_T  Full;   // Full size of this memory chunk including reserved

public:
//---------------------------------------------------------------------------
static TSlf* Create(UINT BlkSize)  // First allocation can request more than MEMGRANSIZE
{
 SIZE_T FullSize = 0;
 TSlf* This  = (TSlf*)Alloca::Allocate(BlkSize, &FullSize);   
 This->Prev  = This->Next = nullptr;
 This->Full  = FullSize; 
 This->Size  = BlkSize;  
 This->Block = nullptr;  // Not needed in this class
 This->First = This;     // The only block until it inserted in a chain
 return This;
}
//---------------------------------------------------------------------------
// Returns pointer to a next block of the chain, NULL if this is last block
// Do not forget to take a pointer to first block after this in case it is changed
//
TSlf* Remove(void)  
{
 Self* Prv = this->Prev;
 Self* Nxt = this->Next;
 Alloca::Free(this);
 if(Nxt)Nxt->Prev = Prv;
 if(Prv)Prv->Next = Nxt;
  else      // This was a first block
   {   
    for(Self* Blk=Nxt;Blk;Blk=Blk->Next)Blk->First = Nxt;    // Update pointer to first block of all blocks 
   }
 return Nxt;     
}
//---------------------------------------------------------------------------
void Destroy(void)     // Frees memory of entire chain
{
 for(TSlf* Blk = this->BlkFirst();Blk;)
  {
   TSlf* Nxt = Blk->Next;
   Alloca::Free(Blk);
   Blk = Nxt;
  }
}
//---------------------------------------------------------------------------
TSlf* Resize(SIZE_T BlkSize)      // Try to resize this block inplace
{
 this->Size = BlkSize;
 if(BlkSize < sizeof(TSlf))BlkSize = sizeof(TSlf);  // First page must be always present    // NOTE: Size is not consider any derived classes by this is not important here
 Alloca::Resize(this, BlkSize, this->Full); 
 return this;
}
//---------------------------------------------------------------------------
TSlf* BlkInsAfter(TSlf* Blk)  // Inserts a single Blk(not a chain) after this block
{
 Blk->First = this->First;
 Blk->Next  = this->Next;
 Blk->Prev  = this;
 this->Next = Blk;
 return Blk;
}
//---------------------------------------------------------------------------
TSlf* BlkInsBefore(TSlf* Blk)  // Inserts a single Blk(not a chain) before this block   // Can change First block 
{
 Blk->First = this->First;
 Blk->Next  = this;
 Blk->Prev  = this->Prev;
 this->Prev = Blk;
 if(this->BlkIsFirst())   // 'this' was a first block
  {
   for(Self* Ptr=Blk;Ptr;Ptr=Ptr->Next)Ptr->First = Blk;    // Update pointer to first block of all blocks 
  }
 return Blk;
}
//---------------------------------------------------------------------------
TSlf* BlkFirst(void)     // Always Fast
{
 return this->First; 
}
//---------------------------------------------------------------------------
TSlf* BlkLast(void)      // May be Slow
{
 TSlf* Ptr = this;
 while(Ptr->Next)Ptr = Ptr->Next;
 return Ptr;
}
//---------------------------------------------------------------------------
PUINT8 DataPtr(void)
{
 return &((PUINT8)this)[sizeof(TSlf)];
}
//---------------------------------------------------------------------------
SIZE_T DataLen(void)
{
 return this->BlkSize() - sizeof(TSlf);
}
//---------------------------------------------------------------------------
TSlf*  BlkNext(void){return this->Next;}
TSlf*  BlkPrev(void){return this->Prev;}
SIZE_T BlkSize(void){return this->Size;}
SIZE_T BlkReserved(void){return this->Full;}
bool   BlkIsFirst(void){return this == this->First;}
//----------------------
};
//===========================================================================
//
// Represents a chain of pages whthin a memory block. Can be any number of CMemChain blocks
//
//  Use low level memory allocation to minimize memory usage and to use a specific features based on alignment
//  At most platforms only MEMPAGESIZE low level allocation base address alignment gurantied
//  On Each page base there are pointer to a Block Base, and at each BlockBase there are UserData value reserved (Align to a pointer size after UserData) but present only in first block
//  An unused page have no base blk ptr on its beginning
//
template<typename UDat=char[0], ESMemPage MPLen=mp4K> class alignas(sizeof(void*)) CPageChain: protected CMemChain<SAllocLL<MPLen>, MPLen>   // One page allocation mult  // All pages aligned to MEMPAGESIZE
{
typedef CPageChain<UDat> TSlf; 
typedef SMemPage<MPLen>  TPage; 

UINT UsedPageCnt;       // Block will be deallocated if this becomes 0
UDat UserData;          // Reserved on each block but accessed from first only

protected:
//---------------------------------------------------------------------------
bool IsFirstPageOfBlk(TPage* Page)     
{
 return ((TSlf*)Page == this);  
}
//---------------------------------------------------------------------------
bool IsPageInBlk(TPage* Page)
{
 return (((PUINT8)Page >= (PUINT8)this) && ((PUINT8)Page < ((PUINT8)this)[this->BlkSize()]));
}
//---------------------------------------------------------------------------
UINT PageHdrSize(TPage* Page)
{
 if(this->IsFirstPageOfBlk(Page))return sizeof(TSlf);
 return sizeof(SMemPage);   // A not first page inside block (Or page not of this block)
}
//---------------------------------------------------------------------------
TPage* BlkPageNext(TPage* Page)       // In current block only!
{
 TPage* Nxt = Page->PageNext();             
 TSlf*  Blk = this->GetBlock();
 if(Nxt >= ((PUINT8)Blk)[Blk->BlkSize()])return nullptr;
 return Nxt;
}
//---------------------------------------------------------------------------
TPage* BlkPagePrev(TPage* Page)      // In current block only, checks limit!
{
 TPage* Prv = Page->PagePrev();           
 if(Prv < this->GetPageBlk())return nullptr;
 return Prv;
}
//---------------------------------------------------------------------------
TPage* BlkLastPage(void)     // First page is 'this' pointer 
{
 return (TPage*)&((PUINT8)this)[(this->BlkTotalPages()-1) * MPageSize];
}
//---------------------------------------------------------------------------
TPage* BlkGetPage(UINT Idx)         // In current block only, checks limit!
{
// if(Idx >= this->BlkTotalPages())return nullptr;
 return (TPage*)&((PUINT8)this)[Idx * MPageSize];
}
//---------------------------------------------------------------------------
int BlkIndexOfPage(TPage* Page)  // Without check if a page in the block
{
 return (((PUINT8)Page - (PUINT8)this) / MPageSize);
}
//---------------------------------------------------------------------------
// Total pages in current block(including unused)
UINT BlkTotalPages(void)  // Page size is not user defined
{
 return (this->GetPageBlk()->Size / MPageSize);
}
//---------------------------------------------------------------------------
// Try to find first 'Count' of contiguous unused pages from current block (contiguous for enumeration, not for direct data storage)
// Returns pointer to a first unused page and optional number of unused pages starting from it
//
TSlf* BlkGetUnusedPages(UINT Count=1, TPage* Page=nullptr, PUINT Unused=nullptr)
{
 UINT PNCnt  = 0;   
 UINT Total  = this->BlkTotalPages();
 TSlf* PBase = nullptr; 
 if(!Page)Page = this;
 for(UINT ctr=this->BlkIndexOfPage(Page),pctr=0;ctr < Total;ctr++,Page=Page->PageNext())
  {
   if(Page->IsPageValid())
    {
     if(PNCnt >= Count)break;   // Enough pages found
     PNCnt = 0;                 // Reset counter and continuuue the search
     continue;
    }
   PNCnt++;
   if(!PBase)PBase = Page;   // Save ptr to first unused page    
  }
 if((PNCnt < Count)){PBase = nullptr; PNCnt = 0;}
 if(Unused)*Unused = PNCnt;
 return PBase;
}
//---------------------------------------------------------------------------
void BlkInitPages(UINT From, UINT Cnt)
{
 TPage* Page = this;
 for(UINT ctr=0;ctr < Cnt;ctr++){Page->Block = this; Page = Page->BlkPageNext();}   // 'Block' pointer in beginning of each page points to its block
 this->UsedPageCnt += Cnt;
}
//---------------------------------------------------------------------------


public:
//---------------------------------------------------------------------------
// Creates a new block of pages
//
static TSlf* Create(UINT PagesNum)
{ 
 TSlf* Blk = (TSlf*)CMemChain::TSlf::Create(PagesNum*MPageSize);
 Blk->UsedPageCnt = 0;
 Blk->BlkInitPages(0, PagesNum);     // Allocated pages are marked as Used
 return Blk;
}
//---------------------------------------------------------------------------
// Always adds a sequentially new pages  // Use if you need to keep them sequential or if ReusePages failed
//
TPage* AddNewPages(UINT PagesNum)    // TODO: Try to allocate pages in a reserved space of current block first
{    
 TSlf* Blk = Create(PagesNum);
 this->BlkLast()->BlkInsAfter(Blk);  // Insert new block into chain
 return Blk;
}
//---------------------------------------------------------------------------
// Tries to find a unused pages in any block in chain
// Tries to find a smallest range of unused pages to fit PCnt
//
TPage* ReusePages(UINT PCnt, bool CanEnlarge)
{
 UINT   FullCnt = -1;  // Max
 TSlf*  PGBlk = nullptr;
 TPage* Pages = nullptr;
 for(TSlf* FBlk=this->BlkFirst();FBlk;FBlk=FBlk->BlkNext())
  {
   for(;;)    // Enum all unused page ranges with not less than PCnt pages
    {
     UINT   PNum = 0;
     TPage* PPtr = FBlk->BlkGetUnusedPages(PCnt, nullptr, &PNum);
     if(!PPtr)break;   // No more unused page ranges in this block
     if(PNum < FullCnt){FullCnt = PNum; Pages = PPtr; PGBlk = FBlk;}    // Remember smaller rage
    }
  }
 if(FBlk)FBlk->BlkInitPages(FBlk->BlkIndexOfPage(Pages), PCnt); 
 return Pages;
}
//---------------------------------------------------------------------------
// Searches for a block that can be enlarged for PCnt more pages
//
TPage* EnlargeForPages(UINT PCnt)    // TODO: Merge with ReusePages because the meaning is same?
{


}
//---------------------------------------------------------------------------
// Remove pages across blocks. Can free block`s memory or just mark part of its pages as unused
// Unused pages will stay in memory until there is no more used pages in the block
// Can change which block is first!
// Returns ptr to first block
//
TSlf* RemovePages(UINT PFrom, UINT PCnt)
{
 TSlf* Self = this;
 for(TSlf* FBlk=this->BlkFirst();FBlk;FBlk=FBlk->BlkNext())
  {
   UINT PCnt = FBlk->Size / MPageSize;
   if(PFrom < PCnt)
    {
     for(TPage* PPtr = FBlk->BlkGetPage(PIdx);PPtr && PCnt;PCnt--)
      {
       FBlk->UsedPageCnt--;
       PPtr->PageInvalidate();
       PPtr = FBlk->BlkPageNext(PPtr);
       if(!PPtr)    // No more pages in this block
        {
         TSlf* BCurr = FBlk;
         FBlk = FBlk->BlkNext();
         if(!BCurr->UsedPageCnt)   // No more used pages in this block - free it and go to next
          {
           if(Self == BCurr)Self = nullptr;
           if(FBlk && BCurr->BlkIsFirst())memcpy(FBlk->GetUserData(),BCurr->GetUserData(),sizeof(UDat));   // Copy user data from a to be removed first block to a next
           BCurr->Remove();
          }  
         if(!Self)Self = FBlk;     // Must preserve ptr to some block to get a first block in the end
         PPtr = FBlk;
        }
      }
     break;
    }
   PFrom -= PCnt;
  }
 return ((Self)?(Self->BlkFirst()):(nullptr));
}
//---------------------------------------------------------------------------
// This data is in first page of first block but space for it is reserved in a first page of each block in case if first block will be removed and then it will be copied
//
UDat* GetUserData(void)
{
 return &this->BlkFirst()->UserData;
}
//---------------------------------------------------------------------------
TPage* FirstPage(void)
{
 return this->BlkFirst();
}
//---------------------------------------------------------------------------
TPage* NextPage(TPage* Page)
{
 if(!Page)return this->FirstPage();
 Page = this->BlkPageNext(Page);
 if(!Page)Page = this->BlkNext();     // First page of next block or NULL
 return Page;
}
//---------------------------------------------------------------------------
TPage* PrevPage(TPage* Page)
{
 if(!Page)return this->BlkLast()->BlkLastPage();
 Page = this->BlkPagePrev(Page);
 if(!Page)Page = this->BlkPrev();  // First page of prev block or NULL
 return Page;
}
//---------------------------------------------------------------------------
TPage* GetPage(UINT PIdx)
{
 for(TSlf* FBlk=this->BlkFirst();FBlk;FBlk=FBlk->BlkNext())
  {
   UINT PCnt = FBlk->Size / MPageSize;
   if(PIdx < PCnt)return FBlk->BlkGetPage(PIdx);
   PIdx -= PCnt;
  }
 return nullptr;
}
//---------------------------------------------------------------------------
UINT PageDataSize(TPage* Page)   
{
 return MPageSize - this->PageHdrSize(Page);
}
//---------------------------------------------------------------------------
PUINT8 PageDataPtr(TPage* Page)
{
 return &((PUINT8)Page)[this->PageHdrSize(Page)];
}
//---------------------------------------------------------------------------

};
//===========================================================================









/*
////  -Have no knowledge of deleted items
////  -An item size is limited to size of MEMPAGESIZE-sizeof(CPageChain)
////  -Any leftover from ItemSize at a page`s end will be wasted (Not much is an items are small)
////  -If an item not fits in rest of a block then that memory is wasted
////  +From any item`s address we can find its page base address and there a pointer to a block`s base



Memory is not sequential, each page have a pointer to first chunk in beginning
that from any stored data item in that page a owner ptr can be easily calculated by masking out a page boundary.


 - No linear items copy support
 - No Removing a separate items support, only an entire blocks
 + Pointers to an items are always pertained 
 + Fast allocation

 typename D=char[0],

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
