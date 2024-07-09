
#pragma once

// Memory management primitives for a self managed containers
// Max PageSize is equal to AllocationGranularity
//=========================================================================== 
template<unsigned int MPSize=mp4K> struct alignas(ALIGNPLAIN) SMemPage      // We must know MPSize to find a page header by any address in it    // 'enum' passing is broken in C++? (Converted to 'int' randomly)
{
 static const SIZE_T MPageSize  = MPSize;            // Expected to be 4k that a page base address can be calculated by AND masking
 static const SIZE_T MPageSMsk  = MPageSize-1;       // For sizes
 static const SIZE_T MPageAMsk  = ~MPageSMsk;        // For addresses
 UINT32 HdrSize;   // Split into FLAGS field as two UINT16???
 UINT32 PageIdx;   // Unused for single blocks  // Without this we can`t find a parent block not wasting memory for Allocation Granularity // But with it we can`t allocate memory more than (MPSize-sizeof(SMemPage))   // A block can be of any number of pages // Will be useful for CPageChain 
 
SMemPage* PagePrev(void){return (SMemPage*)&((PUINT8)this)[-MPageSize];}  // Array-
SMemPage* PageNext(void){return (SMemPage*)&((PUINT8)this)[MPageSize];}   // Array+
static SMemPage* PageForAddr(void* Addr){return (SMemPage*)((SIZE_T)Addr & MPageAMsk);}
};
//===========================================================================
//
// Linked list of memory chunks.
// Max block size is 4GB 
// Best works with SAllocLL
// Is not supposed to be created on stack!
// Unused blocks can be deallocated if required
// All headers have SIMD compatible alignment
// Block is derived from a page because it is consists of pages
// Static Polymorphism: https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern
//
template<int Align, typename Alloca, typename T> class alignas(ALIGNPLAIN) CMemBlockF: public SMemPage<Alloca::SAlignment>  // x32:32; x64:
{
public:
 typedef T TSlf;   // Makes T accessible by derived classes 

protected:
 TSlf*  Prev;     // struct alignas(8) {TSlf* Prev;  };:  'struct alignas(8)' Makes pointers same size on x32/x64 for serialization compatibility
 TSlf*  Next;  
 TSlf*  First;    // For fast access in case some user data is stored there
 SIZE_T Size;     // Used size of this memory chunk
 SIZE_T Full;     // Full size of this memory chunk including reserved

//---------------------------------------------------------------------------
static void SetAsFirst(TSlf* Blk)
{
 for(TSlf* Ptr=Blk;Ptr;Ptr=Ptr->Next)Ptr->First = Blk;    // Update pointer to first block of all blocks 
}
//---------------------------------------------------------------------------

public:
static bool IsCrossesPages(SIZE_T Offset, SIZE_T Size){return ((Offset & (MPageSize-1)) != ((Offset+(Size-1)) & (MPageSize-1)));}    // ((Offset / MPSize) != ((Offset+(Size-1)) / MPSize))
//---------------------------------------------------------------------------
static TSlf* Create(UINT BlkSize, UINT UsrSize=0)  // First allocation can request more than MEMGRANSIZE
{
 SIZE_T FullSize = 0;
 SIZE_T DataSize = 0; 
 TSlf* This    = (TSlf*)Alloca::Allocate(BlkSize, &DataSize, &FullSize); 
 if(!This)return nullptr; 
 This->Prev    = This->Next = nullptr;
 This->Full    = FullSize; 
 This->Size    = DataSize;  
 This->First   = This;     // The only block until it inserted in a chain
 This->PageIdx = 0;
 This->HdrSize = (Align)?(AlignFrwrd(sizeof(TSlf) + UsrSize, Align)):(sizeof(TSlf) + UsrSize);
 return This;
}
//---------------------------------------------------------------------------
// Returns pointer to a next block of the chain, NULL if this is last block
// Do not forget to take a pointer to first block after this in case it is changed
//
TSlf* Delete(void)  
{
 TSlf* Prv = this->Prev;
 TSlf* Nxt = this->Next;
 Alloca::Free(this);
 if(Nxt)Nxt->Prev = Prv;
 if(Prv)Prv->Next = Nxt;
  else SetAsFirst(Nxt);     // This was the first block
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
// Does nothing if the block is full (4GB)
// Tries to resize this block inplace 
// Check BlkSize/BlkFullSize afterwards to confirm success
//
TSlf* Resize(SIZE_T BlkSize, bool CanReloc=false)        
{
 if(BlkSize > this->Full)     // Do not use this with PageChain
  {
   SIZE_T FullSize = 0; 
   SIZE_T DataSize = 0; 
   TSlf* This = (TSlf*)Alloca::Realloc(this, BlkSize, this->Size, &DataSize, &FullSize);   // Shrink or enlarge the committed block
   if(!This)  
    {
     if(!CanReloc)return nullptr;   // But no releasing the block
     This = TSlf::Create(BlkSize);      // Try again
     if(!This)return nullptr; 
     CopyMem(This->DataPtr(), this->DataPtr(), this->DataLen());
     this->BlkInsBefore(This);
     this->Delete();
     return This;
    }
    else if(This != (TSlf*)this)    // Reallocated to a different location
     {
      if(This->Next)This->Next->Prev = This;  // Update its pointers in LinkedList
      if(This->Prev)This->Prev->Next = This;
       else SetAsFirst(This);     // This was the first block
     } 
   this->Size = DataSize;
   this->Full = FullSize;  
   return This;
  }
 if(BlkSize < sizeof(TSlf))BlkSize = sizeof(TSlf);     // First page must be always present    // NOTE: Size is not consider any derived classes by this is not important here
 if(BlkSize != this->Size)
  {
   Alloca::Resize(this, BlkSize, this->Full); 
   this->Size = BlkSize;   // New size of this block
  }
 return static_cast<TSlf*>(this);
}
//---------------------------------------------------------------------------
TSlf* BlkInsAfter(TSlf* Blk)  // Inserts a single Blk(not a chain) after this block
{
 Blk->First = this->First;
 Blk->Next  = this->Next;
 Blk->Prev  = static_cast<TSlf*>(this);
 this->Next = Blk;
 return Blk;
}
//---------------------------------------------------------------------------
TSlf* BlkInsBefore(TSlf* Blk)  // Inserts a single Blk(not a chain) before this block   // Can change First block 
{
 Blk->First = this->First;
 Blk->Next  = static_cast<TSlf*>(this);
 Blk->Prev  = this->Prev;
 this->Prev = Blk;
 if(this->BlkIsFirst())SetAsFirst(Blk);   // 'this' was a first block
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
 TSlf* Ptr = static_cast<TSlf*>(this);
 while(Ptr->Next)Ptr = Ptr->Next;
 return Ptr;
}
//---------------------------------------------------------------------------
PUINT8 DataPtr(void)
{
 return &((PUINT8)this)[this->HdrSize];
}
//---------------------------------------------------------------------------
SIZE_T DataLen(void)
{
 return this->Size - this->HdrSize;
}
//---------------------------------------------------------------------------
PUINT8 UsrDataPtr(void)
{
 return &((PUINT8)this)[sizeof(TSlf)];
}
//---------------------------------------------------------------------------
SIZE_T UsrDataLen(void)   // Includes alignment bytes
{
 return (this->HdrSize - sizeof(TSlf));
}
//---------------------------------------------------------------------------
TSlf*  BlkNext(void){return this->Next;}
TSlf*  BlkPrev(void){return this->Prev;}
SIZE_T BlkSize(void){return this->Size;}
SIZE_T BlkFullSize(void){return this->Full;}
bool   BlkIsFirst(void){return static_cast<TSlf*>(this) == this->First;}
bool   BlkIsFull(void){return this->Size >= this->Full;}
//----------------------
};
//===========================================================================
// Simple chgainable memory block
//
template<int Align=ALIGNSIMD, typename Alloca=SAllocLL<> > class alignas(ALIGNPLAIN) CMemBlock: public CMemBlockF<Align, Alloca, CMemBlock<Align,Alloca> >
{
friend class CMemBlockF<Align, Alloca, CMemBlock<Align,Alloca> >;

};
//===========================================================================
// Smallest chunk size to allocate is 8 bytes
// Usage: Allocator for chunks of same size (Deletion of different sized chunks can cause Empty list to grow and slow down allocations)
// It is your responsibility to keep items SIMD aligned if required(Keep their size aligned to 16)
// Use only with SAllocLL (See alignment specifics there)
// Pointer Free: You can serialize entire block and load it later at different address if your data does not contain any pointers
// Biased toward data storage: More fragmented chunks are free slower allocation will be(Because of Empty list traversing)
// TODO: CacheSpread parameter ?  (
//      Copy one block into another when both are page aligned is not cache optimal. 
//      For such cases allocation of both arrays from a big block of common pool is more preferable because addresses of both arrays most likely end up in a different cache lines
//      But here we need an ability to find beginning of a block without wasting memory for some header for each allocation. So we separate memory block to pages.)
//                            
template<int Align=0, typename Alloca=SAllocLL<> > class alignas(ALIGNPLAIN) CMemPoolBlkS: public CMemBlockF<Align, Alloca, CMemPoolBlkS<Align,Alloca> >
{
public:
 typedef CMemPoolBlkS<Align,Alloca>  TSlf;   // Makes T accessible by derived classes   // In C++14 base class` TSlf was visible

private:
struct alignas(ALIGNPLAIN) SEmpty    // UINT32 limits block size to 4GB. Else it will be 16 bytes per alloc item on x64  // And allow each item to be not SIMD aligned?
{
 UINT32 Next;    // Offset
 UINT32 Size;  
};

 typedef CMemBlockF<Align, Alloca, CMemPoolBlkS<Align,Alloca> > TBase;   // 'Align' better to be multiply of 8 (16 for SIMD optimization)
 friend class TBase; 

 static const SIZE_T BAlignment  = (Align < sizeof(SEmpty))?(sizeof(SEmpty)):(Align);    // No point aligning headers to SIMDS when chunks have incompatible alignment
 static const SIZE_T MaxPageFree = MPageSize - AlignFrwrdPow2(sizeof(SMemPage<>), BAlignment);  // No wasted memory: Size and Base alignment is bound
 UINT32  Empty;  // Chain of empty chunks offsets across all pages   // Offsets instead of pointers used(Relative to BlkBase) - no fixing when the block is reallocated needed
 UINT32  EmptyPages;   // Counting unused pages adds too much overhead for Alloc/Free??? (Recalculation of Empty offset to PageBase and size check, see IsPageNowEmpty)

//---------------------------------------------------------------------------
// Add each page to Empty chain
//
void MakeBlkEmpty(UINT32 PageIdx=0, bool InitHdrs=false)
{
 for(SIZE_T COffs = PageIdx*MPageSize;;)
  {
   SMemPage* MPage = (SMemPage*)((UINT8*)this + COffs);
   if(InitHdrs)         // Done here to avoid passing data cache twice in a separete loops of InitHeaders and MakeBlkEmpty
    {
     if(COffs)     // Starting not from a first page
      {
       MPage->HdrSize = MPage->HdrSize = AlignFrwrdPow2(sizeof(SMemPage<>), BAlignment);     // Align Page HeaderSize to specified aalignment(Helps to avoid page leftovers and growing Empty list when you allocating only objects of specific size)
       MPage->PageIdx = COffs/MPageSize;
      } 
       else this->Empty = this->HdrSize;   // From first block, init first header Empty ptr
    }
   COffs += MPageSize;
   SEmpty* Curr = (SEmpty*)((UINT8*)MPage + MPage->HdrSize);
   Curr->Size = MPageSize - MPage->HdrSize;
   if(COffs >= this->Size){Curr->Next = 0; break;}  // No more pages
     else Curr->Next = ((SMemPage*)((UINT8*)this + COffs))->HdrSize;  // Point to a next page 
  }
}
//---------------------------------------------------------------------------
// Block`s allocated size has changed - Recalculate Empty list
void UpdateBlkEmpty(SIZE_T OldSize)
{
 if(this->Size == OldSize)return;  // Nothing changed
 if(this->Empty >= this->Size){this->Empty = 0; return;}   // No empty left (Possibly after shrinking the block)
 if(this->Size < OldSize)  // Shrinking - Trim Free list
  {
   if(!this->Empty)return;   // No empty
   SEmpty* Curr = (SEmpty*)((UINT8*)this + this->Empty);
   while(Curr->Next && (Curr->Next < this->Size))Curr = (SEmpty*)((UINT8*)this + Curr->Next);  // Find last one
   Curr->Next = 0;  // Make it Last one
   return;
  }

 UINT PIdx = (OldSize / MPageSize) + (bool)(OldSize % MPageSize);   // Has Grown - Init new pages as Free
 SIZE_T COffs = PIdx * MPageSize; 
 this->MakeBlkEmpty(PIdx, true);
 if(this->Empty)
  {
   SEmpty* Curr = (SEmpty*)((UINT8*)this + this->Empty);
   while(Curr->Next)Curr = (SEmpty*)((UINT8*)this + Curr->Next);  // Find last one
   Curr->Next = COffs + ((SMemPage*)((UINT8*)this + COffs))->HdrSize; 
  }
   else this->Empty = COffs + ((SMemPage*)((UINT8*)this + COffs))->HdrSize;
}
//---------------------------------------------------------------------------
static bool IsPageEmpty(void* Addr)  // Call this after 'Free' to determine if a memory page became unused  // Call it before Alloc to determine if a page was unused
{
 SMemPage* MPadr = PageForAddr(Addr);
 if(((UINT8*)MPadr + MPadr->HdrSize) != (UINT8*)Addr)return false;  // Not first Empty record
 return (((SEmpty*)Addr)->Size == (MPageSize - MPadr->HdrSize));
}
//---------------------------------------------------------------------------
SIZE_T ExAllocSize(void)
{
 return this->Size + Alloca::SAlignment;   
}
//---------------------------------------------------------------------------

public:
//---------------------------------------------------------------------------
static TSlf* Create(UINT BlkSize, UINT UsrSize=0)  // First allocation can request more than MEMGRANSIZE
{
 TSlf* This = (TSlf*)TBase::Create(BlkSize, UsrSize);
 if(This)
  {
   This->HdrSize = AlignFrwrdPow2(sizeof(TSlf) + UsrSize, BAlignment);
   This->EmptyPages = 0;     // ?????????????????????????????????
   This->MakeBlkEmpty(0, true);
  }
 return This;
}
//---------------------------------------------------------------------------
TSlf* Resize(SIZE_T BlkSize, bool CanReloc=false)
{
 SIZE_T PrvSize = this->Size;
 TSlf* Res = TBase::Resize(BlkSize, CanReloc);
 Res->UpdateBlkEmpty(PrvSize);
 return Res;
}
//---------------------------------------------------------------------------
// Registers all pages in Empty list
void Clear(void)
{
 this->Empty = this->HdrSize;
 this->MakeBlkEmpty(0, false);
}
//---------------------------------------------------------------------------
bool IsEmpty(void)
{
 return ((this->EmptyPages * MPageSize) >= this->Size);
}
//---------------------------------------------------------------------------
// Release last free pages
// NOTE: No point in doing this after each 'Free' (See 'Alloc-Free-Alloc' note)
//
bool Trim(void)
{      
 SIZE_T EmOffs = this->Empty;  // Initial offset
 SMemPage* FirstPage = nullptr;
 SMemPage* LastPage  = nullptr;
 UINT PageCtr = 0;
 for(;;)
  {
   SEmpty* Curr = (SEmpty*)((UINT8*)this + EmOffs);
   SMemPage* CurrPage = (SMemPage*)((UINT8*)this + (EmOffs & MPageAMsk));
   if(Curr->Size == (MPageSize - CurrPage->HdrSize))
    {
     if(!FirstPage){FirstPage = CurrPage; PageCtr++;} 
      else
       {
        if(CurrPage != LastPage->PageNext())
         {
          FirstPage = nullptr;
          PageCtr   = 0;
         }
          else PageCtr++;
       }   
    }
     else
      {
       FirstPage = nullptr;
       PageCtr   = 0;
      }
   if(!Curr->Next)break;  
   EmOffs   = Curr->Next;
   LastPage = CurrPage;
  }
 if(!FirstPage)return false;    // No free pages at end
 if(FirstPage == this){FirstPage = FirstPage->PageNext(); PageCtr--;}  // First page cant`t be released
 SIZE_T FirstOffs = (UINT8*)FirstPage - (UINT8*)this;
 if((FirstOffs+(MPageSize * PageCtr)) != this->Size)return false;   // Not last pages
 return (bool)this->Resize(FirstOffs, false);   // PERF: Empty list will be traversed again in UpdateBlkEmpty
}
//---------------------------------------------------------------------------
// Exclude Range from Empty list (Allocate chunk of memory)
// First page is BlockHdr and cannot fit MaxPageFree
// Min allocated size is 'sizeof(SEmpty)' (8 bytes)
// 6 times faster than HeapAlloc on Windows :)
// Most allocators put a great effort in preserving size of an allocated chunk. But when you free memory in your code you usually know what size it is
//
void* Alloc(SIZE_T Size)
{
 if(!Size)return nullptr;  // ???
 if(!this->Empty)   // No empty chunks  
  {
   if(!this->Resize(this->ExAllocSize(), false))return nullptr;    // After this we should have at least one free page added
  }  
 SIZE_T  FLen  = AlignFrwrdPow2(Size, BAlignment);     // Without alignment there are mem leftovers lost when left is '< sizeof(SEmpty)' in a chunk (Impossible to put in Empty list)  // Headers also aligned to this value (Min sizeof(SEmpty)) 
 if(FLen > MaxPageFree)return nullptr;   // No mem for this
 SEmpty* EPrv  = nullptr;
 SEmpty* ELst  = nullptr;
 SEmpty* PCur  = nullptr;
 SEmpty* ECur  = nullptr;
 SIZE_T LSize  = -1;
 SIZE_T EmOffs = this->Empty;
 for(;;)  // Find a smallest free chunk to fit 'Size'
  {
   ECur = (SEmpty*)((UINT8*)this + EmOffs);
   if((ECur->Size >= FLen) && (ECur->Size < LSize))   
    {
     if(ECur->Size == FLen) // Exact fit
      {
       if(EmOffs == this->Empty)this->Empty = ECur->Next;  // First    // 0 if this is the only
       else if(!ECur->Next)PCur->Next = 0;  // Last  // Expected to exist: 'EmOffs != this->Empty'  
            else PCur->Next = ECur->Next;  // Exclude
       return ECur;
      }
     ELst  = ECur;
     EPrv  = PCur;
     LSize = ECur->Size;
    }
   PCur   = ECur;
   if(!ECur->Next)break;   
   EmOffs = ECur->Next;
  }
 if(!ELst)  // No free space found   // PERF: Additional pass through Empty list
  {
   if(!this->Resize(this->ExAllocSize(), false))return nullptr;    // After this we should have at least one free page added
   EmOffs = LSize = PCur->Next;  // 'Next' should be updated to point to a newly added page
   ELst   = (SEmpty*)((UINT8*)this + EmOffs);
   EPrv   = PCur;
  }
   else EmOffs = LSize = (UINT8*)ELst - (UINT8*)this;
 EmOffs += FLen;
 ECur    = (SEmpty*)((UINT8*)this + EmOffs);
 if(LSize == this->Empty)this->Empty = EmOffs;   // First
   else EPrv->Next = EmOffs;
 ECur->Next = ELst->Next;
 ECur->Size = ELst->Size - FLen;
 return ELst;    // No Zeroing?
}
//---------------------------------------------------------------------------
// No wasting space for a Chunk size - you responsible for knowing it (It would require 16 bytes to keep SIMD alignment)  // Perfect for 'operator new'
// TODO: Optimize, if possible?
// It is big but avoids excess overwritings in CPU cache and 'IF' duplicates
// ???: Check if Ptr is in current block and that Ptr and Size are 'sizeof(SEmpty)' aligned?
// Blocks can allocate more pages sequentially, but can`t release them (To avoid last item Alloc-Free-Alloc cycle overhead)
//
bool Free(void* Ptr, SIZE_T Size)    // Ptr must be in current block   // Check that PTR and SIZE are 'sizeof(SEmpty)' aligned
{
 SIZE_T Offs = (UINT8*)Ptr - (UINT8*)this;         
 SIZE_T FLen = AlignFrwrdPow2(Size, BAlignment);  // Headers also aligned to this value (Min sizeof(SEmpty)) 
 if(!this->Empty)    // Init a first Empty
  {
   SEmpty* Curr = (SEmpty*)Ptr;
   Curr->Next   = 0;
   Curr->Size   = FLen;  // Curr->Size is not SIZE_T
   this->Empty  = Offs;
   return true;
  }

 SIZE_T EmOffs = this->Empty;  // Initial offset
 if(Offs < EmOffs)      // Replace 'this->Empty' // Only first record, others will go with '(Offs < Curr->Next)'
  {
   SEmpty* ENxt = (SEmpty*)Ptr;
   if((Offs + FLen) == this->Empty)      // Right before Next
    {
     SEmpty* ELst = (SEmpty*)((UINT8*)this + this->Empty);
     ENxt->Next = ELst->Next;
     ENxt->Size = FLen + ELst->Size;    // Combine with Prev
    }
     else
      {
       ENxt->Next = this->Empty;
       ENxt->Size = FLen;  // Curr->Size is not SIZE_T
      }
   this->Empty = Offs;
   return true;
  }

 for(;;)
  {
   SEmpty* Curr = (SEmpty*)((UINT8*)this + EmOffs);
   if(!Curr->Next)   // Add Last
    {
     if((EmOffs + Curr->Size) == Offs)Curr->Size += FLen;     // Right after Curr - enlarge it
       else
        {
         SEmpty* ENxt = (SEmpty*)Ptr;
         ENxt->Next   = 0;
         ENxt->Size   = FLen;  // Curr->Size is not SIZE_T
         Curr->Next   = Offs;
        }
     return true;
    }
   if(Offs < Curr->Next)  // Between Curr And Next
    {
     if((EmOffs + Curr->Size) == Offs)  // Right after Curr - enlarge it
      {      
       if((Offs + FLen) == Curr->Next)   // Chunk fits right in the gap between Curr and Next - Merge them
        {
         SEmpty* ENxt = (SEmpty*)((UINT8*)this + Curr->Next);
         Curr->Size  += FLen + ENxt->Size;
         Curr->Next   = ENxt->Next;
         return true;
        }
       Curr->Size += FLen;
       return true;
      }
     if((Offs + FLen) == Curr->Next)      // Right before Next - merge with it    // Not right after Curr, it is some gap there
      {
       SEmpty* ENxt = (SEmpty*)((UINT8*)this + Curr->Next);
       SEmpty* ENew = (SEmpty*)Ptr;
       ENew->Next   = ENxt->Next;
       ENew->Size   = FLen + ENxt->Size;  // Curr->Size is not SIZE_T
       Curr->Next   = Offs;
       return true;
      }
     SEmpty* ENxt = (SEmpty*)Ptr;     // Insert between Curr and Next
     ENxt->Next   = Curr->Next;
     ENxt->Size   = FLen;  // Curr->Size is not SIZE_T
     Curr->Next   = Offs;
     return true;
    }
   EmOffs = Curr->Next;
  }         
 return false;
}
//---------------------------------------------------------------------------

};
//===========================================================================
// Extends MemPoolBlk beyond a single block capacity
//
/*template<typename TPoolBlk=CMemPoolBlkS<> > class alignas(ALIGNPLAIN) CMemPool
{
 typedef CMemPool<TPoolBlk> TSlf;

TPoolBlk* GetBlkForPtr(void* Addr)    // Helps to avoid linked list travesing   // Fails for a block with different page size
{
 SMemPage* PPg = TPoolBlk::PageForAddr(Addr);
 return (TPoolBlk*)((UINT8*)PPg - (PPg->PageIdx * TPoolBlk::MPageSize));
}
//---------------------------------------------------------------------------

public:
//---------------------------------------------------------------------------
static TSlf* Create(void* UsrData=nullptr, UINT UsrSize=0, UINT InitBlkSize=TPoolBlk::MPageSize)
{
 return 0;
}
//---------------------------------------------------------------------------
void* Alloc(SIZE_T Size)
{
 return 0;
}
//---------------------------------------------------------------------------
bool Free(void* Ptr, SIZE_T Size)
{
 return 0;
}
//---------------------------------------------------------------------------
void* GetUserData(UINT* Size=nullptr)    // Only a First block contains it
{

}
//---------------------------------------------------------------------------

};  */   // Fwrite FreePagesBefore in a page header
//
// Keep sorted linked list of used pages. Then anything in between is unused
// Keep offset of some first unused page and counter of unused pages
// Then we can decommit an unused page but allocate it again as some page will be required
// Memory will be returned to the system, but addres space of a block stays reserved(Not a problem on x64)
// When some page will be required, we take a first free page, and find another free to be ready
// We cannot fulfill a request for more than one page of size class
//
//===========================================================================
//
//---------------------------------------------------------------------------
/*template<int APowMin=4, int APowMax=23, int HdrAlign=CPUCACHELNE, typename UDat=char[0]> CViAlloc
{
 typedef CViAlloc<APowMin,APowMax> TSlf;

 static const int AMinPow   = (APowMin < 4)?4:((APowMin > 23)?23:APowMin);   // Min cnunk size is 16 (2 ^ 4)
 static const int AMaxPow   = (APowMax > 23)?23:((APowMax < 4)?4:APowMax);
 static const int MaxPClass = (AMaxPow - AMinPow) + 1;   // Page sizes: 4096 - 2147483648
 static const int MaxSClass = 256;   // For each PClass

struct alignas(ALIGNPLAIN) SPageHdr   // Overlaps with ByteMap for PageSize <= 16384    // Keep alignment
{
 UINT16 PageIdx;   // Limits max block size to 268435456 for 4K pages (16MB is wasted on ByteMap for such big block)
 UINT8  HdrSize;   // In mul of 16
 UINT8  Flags;     // Including Lock bit
};
struct alignas(ALIGNPLAIN) SBlockHdr   // On a first page, follows after SPageHdr if it is not overlaps with ByteMap
{
 TSlf*  Prev;     // struct alignas(8) {TSlf* Prev;  };:  'struct alignas(8)' Makes pointers same size on x32/x64 for serialization compatibility
 TSlf*  Next;  
 TSlf*  Inst;     // For fast access user data
 UINT16 Size;     // In Pages // Initialized size (Registered pages)
 UINT16 Full;     // In Pages // Full size (Reserved or untouched pages). Will require initialization and OnDemand fetching (Soft failure)
};

// CViAlloc()
//---------------------------------------------------------------------------


}; */






//===========================================================================
//                        DON`T LOOK BELOW :)
//===========================================================================
/*
  Mark chunk`s offset in EmptyList as that chunk need a page commit(Was released after Free)
  On Linux we need immeadiately mmap after munmap to keep that address space reserved for us
  If no records on a page, then there is no page header retained and such case can be detected by EmptyChunc offset, aligned to PaseSize instead of just 16 bytes(Points to a Page beginning, not after its header)
  Unused memory more than a system page size (4K) is never wasted, just not allocated(Reserved/untouched).

  VirtualFree on Windows can do MEM_RELEASE only for a same base address which was returned by VirtualAlloc.
  This means that we can`t unmap unneeded pages as with munmap on Linux to keep alignment.
  Leaving these pages reserved will lead to severe address space fragmentation on x32 system.
  Because of this Object allocator can`t use pages more than 65536 bytes in size which is allocation granularity on windows

// Bitmap grows sooner
PAGE           BM    CHUNK     Pages
4096        <> 32    <> 16     1
8192        <> 64    <> 16     2
16384       <> 64    <> 32     4
32768       <> 128   <> 32     8
65536       <> 128   <> 64     16
131072      <> 256   <> 64     32
262144      <> 256   <> 128    64
524288      <> 512   <> 128
1048576     <> 512   <> 256
2097152     <> 1024  <> 256
4194304     <> 1024  <> 512
8388608     <>
16777216    <>
33554432    <>
67108864    <>
134217728   <>

// Memory wasted sooner
4096        <> 32   <> 16   
8192        <> 32   <> 32
16384       <> 64   <> 32
32768       <> 64   <> 64
65536       <> 128  <> 64
131072      <> 128  <> 128
262144      <> 256  <> 128
524288      <> 256  <> 256
1048576     <> 512  <> 256
2097152     <> 512  <> 512
4194304     <> 1024 <> 512
8388608     <> 1024 <> 1024     // Anything greater than 8MB will waste too much memory and processes bitmap too long
16777216    <>
33554432    <>
67108864    <>
134217728   <>

// Bytemap size is 256 bytes to define size of one max chunk that can fit on a page by multiplying it to its size class 
// No bit magic needed to access byte map and we can alocated store chunk sizes without breaking 16 byte alignment
// Consider this table when allocating your objects
// First 4 bytes of Bytemap are used as a page header
// If we used bits instead of bytes then there would be no way to distinguish one allocated chunk from another when doing 'Free' and standart 'free' function doesn`t accepts size along with a pointer
// MUL is used to compensate for BYTE limit
// Never put a chunk in a block which alignment is two blocks larger than a chunk c(i.e. 64 chunk into 256 block): - never put in a different block
// Chunks with size close to alignment put in corresponding block to keep small pages at minimum
// FirstFree, LastFree for comepletely free blocks
// If more than one block, last free will be released to system on next Free operation. But recent bleck whick become empty after Free will never immidiately returned to system because it may be required by a next allocation request
// Each page can be locked separatedly and SizeClass manipulation is atomic operation (8 byte exchange)
// Returning individual pages to the system may slowdown significately because of soft page faults on returning them.(MEM_RESERVED->MEM_COMMIT(again) on Windows may do the trick)
//
PAGE SIZE      CHUNK SIZE     USED    RANGES          FIRST
4096         / 256 = 16       16      1     <> 3840   -0       // Min page size and min chunk size   // Big overhead of ByteMap when too many pages (16MB for 256MB block)
8192         / 256 = 32       8       3841  <> 7936   -0       // Into iach next page size class come allocations that can`t fit on a previous one
16384        / 256 = 64       4       7936  <> 16128  -0       // Last when page header fits in unused space in bytemap    // <<< This page size is optimal for general use and very cache friendly                      
32768        / 256 = 128      2       16129 <> 32512  -16
65536        / 256 = 256      1       32513 <> 65536  -16      // TLSF may waste 8192 here! And minimal allocation.  But we wasting more memory from each smaller allocation by using fixed granularity and alignment (TLSF just requires blocks be >= 32 and multiple of 4 in size)                                           
131072       / 256 = 512      0                       -16 -256
262144       / 256 = 1024
524288       / 256 = 2048
1048576      / 256 = 4096           // From here we can have an entire unallocated chunks. Before chunks can cross their boundaries and we should mark a prev offset if deallocating a group for a page (Merge and mark as unallocated)
2097152      / 256 = 8192
4194304      / 256 = 16384          // Afer this you should consider to use a low level allocation function
8388608      / 256 = 32768
16777216     / 256 = 65536
33554432     / 256 = 131072
67108864     / 256 = 262144
134217728    / 256 = 524288
268435456    / 256 = 1048576        // This is max block size for 4K pages
536870912    / 256 = 2097152
1073741824   / 256 = 4194304
2147483648   / 256 = 8388608        // Max bit position for 32 bit SIZE_T on x32

Without Bytemap we may have allovation in any Page Size with size Mul16 but have to search in LinkedList for a FreeBlock 
 to fit when allocating and again, when doing Free and trying to merge free blocks. And unable to emulate 'free' for 'malloc' bacause size of allocation is tot stored anywhere(Avoid wasting memory for it to kee SIMD alignment)
Use min MinPagesToDecommit paraameter to reduce overhead if needed
From a first chunk on a page a size of metadata may be subtracted(If that chunk`s size is more than 16 bytes)
First page contains list of free list class offsets one for each multiply of a chunk size (256 in total) (For 4096 bytes page it is 16,32,48,64,...,4096)
By using ByteMap instead of BitMap we wasting 256 bytes instead of 32 but avoiding any complicated bit counting and keeping size of allocated block

APowMin=4 (16)
SPowMin=12 (4096)

APowMax=23 (8388608)
SPowMax=31 (2147483648)

*/

// recently freed blocks will be given out to new allocations first.
// In general - a general allocator is bad idea. Either ir fragments address space or keeps memory, allocated by some rare burst of request long ago and which is freed already


// Bitmap ca be used to validate chunk size on Free call and to find a prev free chunk to merge with it
// NOTE: It is bad when multiple CPU cores are writing to the same cache line. You should prefer using allocators locally
/*
TLSF:    // Memory wasting grows exponentially?
16 65536 + 0 * 8192      // Means that If you request 65537 bytes it will give you (65536+8192+N) from second level 1 instead of 0 to avoid searching in it   
           1 * 8192
           ...
           7 * 8192
*/
//===========================================================================
/*IDEA:
??? Each page`s header contains offset of prev free chunk on any prev page (of any size class)  ???
 Merging of chunks could only happen on a same page
 When a chunk merged or split it moved to a different size class.
 If a chunk is completely consumed by allocation then it just removed vrom its size class` linked list
 Page`s header contains bit map of free chunks. This bitmap is used to find a nearest free chink header when doing 'Free' in case of merging may be done
 If there is some Empty on a page that can be merged, then it is removed from its list and added with new size. If no empty there, then a new added to its size class as first
 No need to maintain linked list of all empty chunks across all pages since we can`t merge them across pages
 Each page`s header contains index of its chunk`s size class
 If next chunk is free can be easily determined by a bitmap

*/
//===========================================================================
// Smallest chunk size to allocate is 16 bytes (SIMD compatible)
// Usage: Generic allocator, for allocations of different size (Wastes some memory for Size Class array (1/4 of a first page))
// You can specify bigger page size for SAllocLL to have support for bigger allocations (Min 4K is enough in most cases or you can just use SAllocLL directly)
// Indexes go from big to small (255 -> 0)  // 1020 bytes per block (Max 4GB)
// No Empty list traversing is required, no loops is used except for best size class search in SCFreeArr size of which IS determined
// No data structures for allocated chunks are used - you get exactly memory you requested, aligned as you specified(min 16 bytes) // Extra data structures would requre additional 16 bytes per chunk to maintain SIMD alignment
// Memory wasting is linear and confirms to alignment (Min 16 bytes) // Means that if you request 20 bytes, 32 will be allocated
//
/*template<int Align=0, typename Alloca=SAllocLL<> > class alignas(ALIGNPLAIN) CMemPoolBlkF: public CMemBlockF<Align, Alloca, CMemPoolBlkF<Align,Alloca> >
{
class alignas(ALIGNPLAIN) SEmpty    // UINT32 limits block size to 4GB. Else it will be 32 bytes per alloc item on x64  
{                    
static constexpr const UINT32 FreeBitMsk = 0xF;  // All chunks are 16 byte aligned so low 4 bits of offset are unused
 UINT32 NextFree;    // Offset // Of sequential free list
 UINT32 PrevFree;    // Offset // Of sequential free list
 UINT32 NextClass;   // Offset // Of same size class
 UINT32 PrevClass;   // Offset // Of same size class
 
public:
 UINT32 GetNextFree(void) {return this->NextFree  & ~FreeBitMsk;}
 UINT32 GetPrevFree(void) {return this->PrevFree  & ~FreeBitMsk;}
 UINT32 GetNextClass(void){return this->NextClass & ~FreeBitMsk;}
 UINT32 GetPrevClass(void){return this->PrevClass & ~FreeBitMsk;}
 UINT16 GetSizeClass(void){return ((this->NextClass & FreeBitMsk) << 8)|((this->PrevFree & FreeBitMsk) << 4)|(this->NextFree & FreeBitMsk);}  // 12 bits is enough for max 64k page size (4095 indeces)
 UINT8  GetFlags(void){return this->PrevClass & FreeBitMsk;}

 void   SetNextFree(UINT32 offs) {this->NextFree  = (this->NextFree  & FreeBitMsk)|offs;}
 void   SetPrevFree(UINT32 offs) {this->PrevFree  = (this->PrevFree  & FreeBitMsk)|offs;}
 void   SetNextClass(UINT32 offs){this->NextClass = (this->NextClass & FreeBitMsk)|offs;}
 void   SetPrevClass(UINT32 offs){this->PrevClass = (this->PrevClass & FreeBitMsk)|offs;}
 void   SetSizeClass(UINT16 scls){this->NextClass = (this->NextClass & ~FreeBitMsk)|(scls >> 8);  this->PrevFree = (this->PrevFree & ~FreeBitMsk)|((scls >> 4) & FreeBitMsk);  this->NextFree = (this->NextFree & ~FreeBitMsk)|(scls & FreeBitMsk);}     // '((scls >> 8) & FreeBitMsk)' in case of OutOfRange?
 void   SetFlags(UINT8 flg){this->PrevClass = (this->PrevClass & ~FreeBitMsk)|flg;}
};

 typedef CMemBlockF<Align, Alloca, CMemPoolBlkF<Align,Alloca> > TBase;   // 'Align' better to be multiply of 8 (16 for SIMD optimization)
 friend class TBase; 

 static const SIZE_T BAlignment  = (Align < sizeof(SEmpty))?(sizeof(SEmpty)):(Align);    // No point aligning headers to SIMDS when chunks have incompatible alignment
 static const SIZE_T MaxPageFree = MPageSize - AlignFrwrdPow2(sizeof(SMemPage<>), BAlignment);  // No wasted memory: Size and Base alignment is bound
 static const UINT   SzClassNum  = MaxPageFree / BAlignment;

 UINT32 SCFreeArr[SzClassNum];   // OPT: Access to this array should not use MUL operations // First record is Chain of empty chunks offsets across all pages  UINT32  Empty; // Each entry represents multiply of 16 (sizeof SEmpty)  // Enumerated from END (Big chunks)  // 1020 bytes for 4K page size, 16380 for 64k page size (1/4 of a page size)
                            // Use bitmap to find a nearest valid Empty to current addr for Free (Only 32 bytes for 4K pages, 512 for 64K pages)
//---------------------------------------------------------------------------
static SIZE_T AlignSize(SIZE_T DSize){return AlignFrwrd(DSize, BAlignment);}
static UINT   SizeToSClass(SIZE_T DSize){return (DSize / BAlignment);}   // Class 0 is for EmptyOffs(With that we avoiding -1 here)  // DSize must be aligned to MinChunkLen  // No size range check here
//---------------------------------------------------------------------------
UINT GetEmptyOffs(void){return *this->SCFreeArr;}
UINT GetSClassOffs(UINT SClass){return this->SCFreeArr[SClass] & ~FreeBitMsk;}
UINT GetSClassFlags(UINT SClass){return this->SCFreeArr[SClass] & FreeBitMsk;}
void SetEmptyOffs(UINT32 Offs){*this->SCFreeArr = Offs;}
void SetSClassOffs(UINT SClass, UINT32 Offs){this->SCFreeArr[SClass] = this->GetSClassFlags(SClass)|Offs;}   // Optimized?
void SetSClassFlags(UINT SClass, UINT8 flg){this->SCFreeArr[SClass] = this->GetSClassOffs(SClass)|flg;}      // Optimized?   
//---------------------------------------------------------------------------
SEmpty* EmptyByOffset(UINT32 Offs){return (SEmpty*)((UINT8*)this + Offs);}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------

void InitEmptyArr(void)
{

}
//---------------------------------------------------------------------------
void GrowEmptyArr(UINT Pages)
{

}
//---------------------------------------------------------------------------
void ShrinkEmptyArr(UINT Pages)
{

}
//---------------------------------------------------------------------------
void ResizeEmptyArr(SIZE_T OldSize)
{


}
//---------------------------------------------------------------------------
int FindNearestFreeSClass(UINT SClass)  // May return Empty for a bigger size class if there is no free chunks for a requested one    
{
 for(UINT ctr=SClass;ctr < SzClassNum;ctr++){ if(this->SCFreeArr[ctr])return ctr; }   // First allocations for a small blocks have to go down to a biggest block (254 iterations for 4K pages). Should not cause any perfomance problems
 return -1;    // No free chunks for this size
}
//---------------------------------------------------------------------------
SEmpty* GetFirstEmptyForSClass(UINT SClass)
{
 UINT Offs = this->GetSClassOffs(SClass);
 return (Offs)?((SEmpty*)((UINT8*)this + Offs)):(nullptr);
}  
//---------------------------------------------------------------------------
// A Chunk consumes entire Empty slot
//
void RemoveFromEmptyList(SEmpty* Empt)
{
 UINT32 ENext = Empt->GetNextFree();
 UINT32 EPrev = Empt->GetPrevFree();
 if(ENext)this->EmptyByOffset(ENext)->SetPrevFree(EPrev);    // May be 0 if Empt is First
 if(EPrev)this->EmptyByOffset(EPrev)->SetNextFree(ENext);    // May be 0 if Empt is Last 
   else this->SetEmptyOffs(ENext);                             // No Prev Empt is First        
}
//---------------------------------------------------------------------------
// Empty is fully consumed or moved to a different size class
//
void RemoveFromClassList(SEmpty* Empt)
{
 UINT32 ENext = Empt->GetNextClass();
 UINT32 EPrev = Empt->GetPrevClass();
 if(ENext)this->EmptyByOffset(ENext)->SetPrevClass(EPrev);    // May be 0 if Empt is First 
 if(EPrev)this->EmptyByOffset(EPrev)->SetNextClass(ENext);    // May be 0 if Empt is Last 
   else this->SetSClassOffs(Empt->GetSizeClass(), ENext);     // No Prev Empt is First        
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

// Removes Empty from Empty size class
void RemoveEmptyFromSClass(UINT SClass)
{

}
//---------------------------------------------------------------------------
// From BIG to SMALL after Alloc (Splitting)
// From SMALL to BIG after Free (Merging)
// By FromSClass is first Empty taken found by FindNearestFreeSClass
//
void* MoveEmptySClass(UINT FromSClass, UINT ToSClass)
{

}
//---------------------------------------------------------------------------

public:
void* Alloc(SIZE_T Size)
{
 if(!Size)return nullptr;    // What exactly do you expect from ZERO allocation request?
 Size = AlignSize(Size);     // Without alignment there are mem leftovers lost when left is '< sizeof(SEmpty)' in a chunk (Impossible to put in Empty list)  // Headers also aligned to this value (Min sizeof(SEmpty)) 
 if(Size > MaxPageFree)return nullptr;   // Requested more than can fit on a page (You should use a larger pages in the allocator)
 long  SClass = SizeToSClass(Size);
 long  ASCls  = this->FindNearestFreeSClass(SClass);
 return (ASCls < 0)?(nullptr):(this->MoveEmptySClass(ASCls, SClass));
}
//---------------------------------------------------------------------------
bool Free(void* Ptr, SIZE_T Size)
{


}
//---------------------------------------------------------------------------

};  */


/*    // Seen-it-empty counter for empty blocks to NOT immediately free them  // Randomize size of a page header in range of 16 - 64 by 16?
class CMemMgr           // If a block to allocate is more than a first page of a newly created pool then create it in an next one if this is not last (64K)
{
 Mem Pools 4K-64K

};
*/

//
/*template<typename Alloca=SAllocLL<>, typename TSlf=int> class alignas(ALIGNSIMD) CPageBlock: public CMemBlock<Alloca, TSlf> 
{
typedef CPageBlock<Alloca> TSlf; 


};   */
//===========================================================================
//
// Represents a chain of pages whthin a memory block. Can be any number of CMemChain blocks
//
//  Use low level memory allocation to minimize memory usage and to use a specific features based on alignment
//  At most platforms only MEMPAGESIZE low level allocation base address alignment guranteed
//  On Each page base there is a pointer to a Block Base, and at each BlockBase there are UserData value reserved (Align to a pointer size after UserData) but present only in first block
//  An unused page have no base blk ptr on its beginning
//
/*template<typename UDat=char[0], ESMemPage MPSize=mp4K> class alignas(ALIGNSIMD) CPageChain  //: protected CMemChain<UDat, SAllocLL<MPLen>, MPLen>   // One page allocation mult  // All pages aligned to MEMPAGESIZE
{
typedef CPageChain<UDat,MPSize> TSlf;      // using TSlf = CPageChain<UDat,MPLen>;
typedef CMemChain<UDat, SAllocLL<MPSize>, MPSize> TBlk;
typedef SMemPage<MPSize>  TPage; 

UINT UsedPageCnt;       // Block will be deallocated if this becomes 0
UDat UserData;          // Reserved on each block but accessed from a first only

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
 Blk->BlkInitPages(0, PagesNum);     // Allocated pages are marked as Used now
 return Blk;
}
//---------------------------------------------------------------------------
// Always adds a sequentially new pages(As a new block). Use it if you need to keep them sequential or if ReusePages failed
//
TPage* AddNewPages(UINT PagesNum)    // TODO: Try to allocate pages in a reserved space of current block first
{    
 TSlf* Blk = Create(PagesNum);
 this->BlkLast()->BlkInsAfter(Blk);  // Insert new block into chain
 return Blk;
}
//---------------------------------------------------------------------------
// Tries to find unused pages in any block in chain
// Tries to find smallest range of unused pages to fit PCnt
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
 // TODO:
 return 0;
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
           if(FBlk && BCurr->BlkIsFirst())CopyMem(FBlk->GetUserData(),BCurr->GetUserData(),sizeof(UDat));   // Copy user data from a to be removed first block to a next
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
 return &((TSlf*)this->BlkFirst())->UserData;
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

/*
sliding problem. when your memory block is sliding in address space, losing ability to grow when encounters an another block on its path
you can easily build a size class based allocator with this
sequential fit strategy, best for allocation os save sized objects or allocation of pools of objects where is rare or no single object deletion
how to find a page base if page sizes are different for each block
return size of allocated memory. if you allocated a big chink for some array, then you may use some preallocated space, of the block alignment
blocks can release address space from their end and discard a memory pages from any place whitbin
Collocating large objects in shared memory regions may have
nother bene?cial impact because it tends to align them randomly,
hus reducing con?ict misses. For example, a program may fre-
uently access a particular ?eld of a number of large objects. When
bjects are always aligned similarly, as in PHKmalloc, accesses to
his ?eld will always con?ict in the cache. By avoiding ?xed ob-
ect alignment, Vam reduces the likelihood of this sort of con?ict
misses.

*/