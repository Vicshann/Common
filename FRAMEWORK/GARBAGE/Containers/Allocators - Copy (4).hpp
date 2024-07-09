
/*
// Both 64-bit Linux and 64-bit Windows support 48-bit addresses, which translates to 256TB of virtual address space
// Android will use memory tagging, so no pointer compression worth the implementation effort anyway.
 BlockChain:   // PRO: No space for list of blocks is needed.  CON: Too slow to walk the entire chain if many blocks are allocated. Metadata at beginning of each block. 
 BlockLinear:  // PRO: Fast random access, fast 'Next/Prev'. Use only an index of a block in the list.  CON: Block list must be stored somewhere. All blocks must be of same size.

 0000 0000
 0001 0000
 0002 0000
 0004 0000
 0008 0000
 0010 0000
 0020 0000
 0040 0000
 0080 0000
 0100 0000
 0200 0000
 0400 0000
 0800 0000
 1000 0000
 2000 0000
 4000 0000 1G
 8000 0000 2G

 Find blocks by end of obj to avoid alignment of each by 4,8,16,32,64,128,256,...
 GetRangeForIndex

 From a block of known size it is easy to get block base (To get some metadata by an object pointer: 'AllocatorMyObj::ObjWrapper* ptr;' - safe way to store such pointers(preserves flags))
 Allocating 64k for page index is not a problem on linux. On Windows it must be 64k or a memory hole will be created.
 Do a separate allocation for block index (Reallocate it if necessary, easy to replace) keep it NULL if only one block is allocated still

 Allocation types: 
 1a) Arbitrary single block + Grow if possible + No delete + Pointers preserved
 1b) Arbitrary linear single block + Move tail if deleted something + purge tail but keep reserved + Grow if possible, relocate otherwise + No pointer preservation
 2a) For Objects of same size only (fails if size is not same as in def?) + Pointers preserved + Grow by block list + No delete
 2b) For Objects of same size only + Pointers preserved + Grow by block list + Use FreeList for deallocated
*/
//------------------------------------------------------------------------------------------------------------
struct NALC
{
enum EAllocFlags {
 afNone        = 0,
 afNoMove      = 0x0010,   // For afSequential but delete is forbidden(How to error on it?). Allocate only from an initially reserved single block of memory(BlkLen). No moving is attempted (delete, extend) and pointers are always preserved. An attempt to grow may fail.
 afSequential  = 0x0020,   // The data in memory must always be sequential to be accessed by a raw pointer. Will use reallocation and copy, cannot guarantee pointer preservation. No free slot map, just a used/allocated sizes
 afObjAlloc    = 0x0040,   // Allocate by objects of specific size. Will keep a list of blocks if no afArena or afSequential is specified.
// afObjDeAlloc  = 0x0080,   // Will have a list of free objects and then will try to allocate in them first. Can be used only with afObjAlloc
// afObjLowFrag  = 0x0100,   // Try to keep fragmentation low by sorting removed objects in free list (Slower delete)
// afThreadSafe  = 0x0100,   // TODO! Thread safe operations only (No moving of internal structures like BlkList or BitMap)
 afSinglePtr   = 0x0200,   // Use only single pointer to reference the memory (Store any associated info elsewhere).  // Sometimes an array is a member of some struct that is allocated in vast numbers but not all of them will have data
 afSmallMem    = 0x0400,   // Do not preallocate early and keep preallocation as small as possible (More conditions while allocating/accessing and more syscalls to grow the allocated block)
 afBlkMetadata = 0x0800,   // Store some metadata at beginning of each block (Like the allocator pointer, and some user pointer)
 afSparseIndex = 0x1000,   // Allocate blocks if their pointers are NULL else report errors on access by unallocateed index.

// Prealloc/Grow modes ?
 afGrowMask    = 0x000F,
 afGrowNone    = 0x0000,   // Size = Size + BaseSize
 afGrowLin     = 0x0001,   // Size = Size + Size
 afGrowGeo     = 0x0002,   // Size = Size * 2

};
//============================================================================================================
//                                      Block access strategies
//------------------------------------------------------------------------------------------------------------
template<size_t ObjLen, size_t PageLen=NPTM::MEMPAGESIZE, size_t FHdrLen=0, size_t NHdrLen=0> struct SBASGeom   // Grow geometrically (geometric progression with base 2)
{
 SCVR size_t RangeNum  = 32;  // Should be enough for x32 and x64 even if base page size is 4096 (16TB max) (RangeNum times : PageSize = (PageSize * 2))
 SCVR size_t FHdrSize  = FHdrLen;   // To make them available from outside
 SCVR size_t NHdrSize  = NHdrLen;
 SCVR size_t ObjSize   = ObjLen;
 SCVR size_t PageSize  = AlignP2Up(((PageLen - Max(FHdrLen, NHdrLen)) < ObjSize)?(ObjSize+Max(FHdrLen, NHdrLen)):(PageLen));    // Base/First page size

//-------------------------------------------------------------
static inline size_t BlkIdxToHdrLen(uint32 BlkIdx)        // TODO: Branchless
{
 if constexpr (FHdrLen)
  {
   if(!BlkIdx)return FHdrLen;
    else if constexpr (NHdrLen)return NHdrLen;
  }
 else if constexpr (NHdrLen)
  {
   if(BlkIdx)return NHdrLen;
  }
 return 0;
} 
//------------------------------------------------------------- 
static inline uint64 BlkIdxToSize(uint32 BlkIdx) {return (uint64)PageSize << BlkIdx;}   // Force inline?   // NOTE: May be too much for x32  // BlkIdx &= (RangeNum - 1);  // Max is RangeNum
//-------------------------------------------------------------
static inline uint64 BlkIdxToDLen(uint32 BlkIdx) {return BlkIdxToSize(BlkIdx) - BlkIdxToHdrLen(BlkIdx);}      // TODO: Test it 
//------------------------------------------------------------- 
static inline size_t BlkIdxToObjNum(uint32 BlkIdx) {return BlkIdxToDLen(BlkIdx) / ObjSize;}
//-------------------------------------------------------------
static inline size_t BlkIdxToObjInf(uint32 BlkIdx, size_t& Offs, size_t& Size)
{
 uint64 BLen = BlkIdxToSize(BlkIdx);
 size_t HLen = BlkIdxToHdrLen(BlkIdx);
 Offs = HLen;
 Size = BLen;
 return (BLen - HLen) / ObjSize;
}
//-------------------------------------------------------------
//void BlkGetRange(size_t BlkIdx, size_t& ObjFrom, size_t& ObjTo) const {}
//-------------------------------------------------------------
static void CalcForIndex(size_t ObjIdx, size_t& Blk, size_t& Idx)      // TODO: Check if ObjIdx is outside of valid range? (For now this function is branchless)
{
 if constexpr (IsPowOfTwo(ObjSize) && !NHdrLen)   // Number of objects in a block is Pow2 as the block itself (No tail bytes, no headers in each block)  // If NHdrLen is not NULL then use of the table is faster than calculations 
  {
   SCVR size_t ObjsOnFPage   = PageSize / ObjSize;
   SCVR size_t ObjBaseBitIdx = ctz(ObjsOnFPage);
   if constexpr (FHdrLen)      // First block contains the header
    {
     SCVR size_t FPOLess = ObjsOnFPage - ((PageSize-FHdrLen) / ObjSize);   // How many objects we lose because of the header  // If FPOLess is 1 will its multiplication be optimized out?  
     ObjIdx += (ObjIdx >= (ObjsOnFPage - FPOLess)) * FPOLess;     // NPObjIdx // This one is probably better since FPOLess may happen to be Pow2 and optimized to aleft shift // !OPTIMIZE!: Will it be brancless on ARM too?   // if(Index >= (ObjsOnFPage - FPOLess))Index += FPOLess;  
    }
   size_t ratio = (ObjIdx >> ObjBaseBitIdx) + 1;   // ++ later?  // Reduce and round-up
   size_t BIdx  = BitWidth(ratio >> 1);  
   size_t ISub  = ((size_t)1 << (BIdx + ObjBaseBitIdx)) - ObjsOnFPage;  
   Idx = ObjIdx - ISub;     
   Blk = BIdx;
  }
 else  // Uncomputable number of objects per block (Wasted tails, headers in each block)
  {
   struct SApArr     // Is there another way to initialize an array at compile time by a consteval function?
   {
    size_t Arr[RangeNum];
    consteval SApArr(void) {for(size_t i=0,v=0,h=0;i < RangeNum;h = (i)?NHdrLen:FHdrLen, v += (((PageSize << i)-h)/ObjSize), i++)this->Arr[i] = v;}
   } SCVR RangeArr;    // Allocates in rdata section
   SCVR uint32 PageBitIdx = (BitSize<size_t>::V - clz(PageSize));  // - 1;  PageSize itself is index 0    // Must be done at compile time somehow // Initial page size will be block index 0
 
   uint64 ObjByteOffs  = ObjIdx * ObjSize;                  // As in a contigous memory block    
   uint32 ApproxGrpIdx = (BitSize<uint64>::V - clz(ObjByteOffs >> PageBitIdx));     // log2    // Actual page-aligned address   // Approximate (there are some wasted bytes at end of each block because an object cannot be split between blocks. But the index calculation is done as if for a single contiguous block of memory)
   ApproxGrpIdx += (ObjIdx >= RangeArr.Arr[ApproxGrpIdx + 1]);  // The error will not cross more than one block because each next block is twice as large than a previous one
   Idx = (ObjIdx - RangeArr.Arr[ApproxGrpIdx]);     // Sometimes means rereading from the same array position
   Blk = ApproxGrpIdx; 
  }
}
//-------------------------------------------------------------
#ifdef DBGBUILD
static void BruteForIndex(size_t ObjIdx, size_t& Blk, size_t& Idx)  // For testing
{
 size_t Ps   = 0;
 size_t HLen = FHdrLen;
 for(int idx=0;idx < RangeNum;idx++)
  {
   size_t Nx = (((PageSize << idx)-HLen)/ObjSize);
   size_t Lx = Ps;
   Ps += Nx;
   if(ObjIdx < Ps){Idx = ObjIdx-Lx; Blk = idx; return;}
   if constexpr (NHdrLen)HLen = NHdrLen;
     else HLen = 0;
  }
 Blk = Idx = -1;
}
#endif
//-------------------------------------------------------------
};  
//============================================================================================================

//============================================================================================================

#ifdef DBGBUILD
template<typename S> static bool TestStrategy(uint32 From=0)
{
 constexpr S str;   //  constexpr NALC::SBASGeom<128, 4096, 64, 0> str;  
 for(size_t idx=From;idx < 0xFFFFFFFF;idx++)
  {
   size_t ClcBlk=0, ClcIdx=0, BrtBlk=0, BrtIdx=0;
   str.CalcForIndex(idx, ClcBlk, ClcIdx);
   str.BruteForIndex(idx, BrtBlk, BrtIdx);
   if((ClcBlk != BrtBlk)||(ClcIdx != BrtIdx))
    {
     return false;
    }
//   size_t onum = str.BlkIdxToObjNum(ClcBlk);
//   onum = onum;
  }
 return true; 
}
#endif
//============================================================================================================
// NOTE: Not a malloc type allocator. Use only for specialized data structures. The memory is not always shrinkable (Even free objects keep the holes reserved)
// NOTE: A default allocator. Should respect all generic policy states at compile time
// NOTE: BlkLen, Align will have different meaning dependant on Flags
// MaxLen: Max block size / max number of objects
// MinLen: Min number of objects expected/ Min Block size
// AlnLen: Alignment/size of an object
// Allocations up to MaxLen will not fail
//
template<ssize_t MinLen, size_t MaxLen, uint32 Flg, uint32 HPad=sizeof(size_t), typename Ty=uint8, typename TCIfo=SEmptyType, typename TBIfo=SEmptyType> class SSeqAlloc   // Arbitrary memory allocator. Single block. Single linear range.
{
public:
 vptr Allocate(size_t Size){return nullptr;}
 vptr Delete(size_t At, size_t Size){return nullptr;}  // If afNoMove is not present
 void Release(void){}
};
//------------------------------------------------------------------------------------------------------------
template<size_t MinLen, size_t MaxLen, uint32 Flg, uint32 HPad=sizeof(size_t), typename Ty=uint32, typename TCIfo=SEmptyType, typename TBIfo=SEmptyType> class SBlkAlloc   // Same sized chunks allocator. Multiple blocks. Multiple linear ranges.
{
//SCVR bool HaveFHdr  = true;
//SCVR bool HaveNHdr  = true;    // If true then the main context is always in the first block (Bacause all blocks will store pointers to it and it should not be moved)
SCVR bool SepBlkIdx = true;    // Block index array is a separate allocation
SCVR bool CtxInFBlk = true;
SCVR bool MetaInBlk = true;    // Metadata is added to every block (To find the block owner)
SCVR bool SparseBlk = false;   // Blocks are allocated on first range request if NULL

// SCVR size_t CtxSize  = sizeof(vptr) * 8;   // Reserved size 32/64  // int512 on x64?   // Should be enough for any AVX alignment requirement
// SCVR size_t ObjSize  = ((Flg & afObjDeAlloc) && (sizeof(Ty) < sizeof(uint32)))?(sizeof(uint32)):(sizeof(Ty));   // Need uint32 for free slots index
// SCVR size_t MinSize  = AlignP2Frwd(MinLen, NPTM::MEMPAGESIZE);     // Grow dependant!!!
// SCVR size_t MaxSize  = AlignP2Frwd(MaxLen, NPTM::MEMGRANSIZE);     // Grow dependant!!!   // Index will be preallocated for this value?
// SCVR size_t GrowType = Flg & afGrowMask;

// STASRT(GrowType <= afGrowGeo);
 enum {dbfDeleted=1,dbfEmpty=2};

struct SBlkArrI    // Only for SBASGeom
{
 size_t BlkNum;
 vptr   BlkArr[SBASGeom<0>::RangeNum];   // TODO: Take size from strategy

 size_t inline GetArrLen(void) const {return sizeof(BlkArr)/sizeof(vptr);}
 vptr*  inline GetArrPtr(void) const {return BlkArr;}
};

struct SBlkArrP
{
 uint32 BlkNum;  // No allocated blocks is expected after this. But in the range some may be NULL
 uint32 BlkLen;  // Number of pointers total to fit in the array
 vptr*  BlkArr;  // Separate allocation to be able to grow. If afSmallMem is specified then allocated only when the first block is full 

 size_t inline GetArrLen(void) const {return BlkLen;}
 vptr*  inline GetArrPtr(void) const {return BlkArr;}
};

struct SMHdr { vptr Self; };  // Points to beginning of the block(This SHdr)  

struct SFHdr: TSW<(MetaInBlk), SMHdr, SEmptyType >::T     // For a first block // Cannot be part of the allocator if afBlkMetadata is specified because the allocator may be moved // Can be member of SObjAlloc, at beginning of block index or at beginning of the first block(bad idea). Should not be moved(Pointer to it may be stored in each block)
{
 TSW<(SepBlkIdx), SBlkArrI, SBlkArrP >::T Blocks; 
 TCIfo  Info;
};

struct SNHdr     // NOTE: Inheritance storage layout is unspecified. You must not make any assumptions about the class layout in memory.     
{
 vptr   Self;    // Points to beginning of the block(This SHdr)     // To find the block base in nonlinear allocation   
 SFHdr* Ctx;     // The allocator context pointer  
 TBIfo  Info;
};

TSW<(CtxInFBlk), SFHdr*, SFHdr >::T Context;

SBASGeom<sizeof(Ty), AlignP2Up(AlignP2Frwd(MinLen, NPTM::MEMPAGESIZE)), AlignP2Up(AlignP2Frwd(sizeof(SFHdr), AlignP2Up(HPad))), AlignP2Up(AlignP2Frwd(sizeof(SNHdr), AlignP2Up(HPad)))> Strat;
//-------------------------------------------------------------
/*static void ObjIdxToBlkIdx(size_t ObjIdx, size_t& BIdx, size_t& OIdx)   // Dependant on grow type
{
 // Calculate the largest k such that 2^k - 1 < unit. We want the smallest k such that 2^(k+1) - 1 >= unit
 if constexpr (GrowType == afGrowGeo)              // 1,1+1=2,2+2=4,4+4=8,...  //  1 + 2 + 4 + 8 = 15
  {                                                                            // 1-1,2-3,4-7,8-15

  }
 else if constexpr (GrowType == afGrowLin)      // Linear increase (Size = Size + MinSize)     // 1,1+1=2,2+1=3,3+1=4,...   //  1 + 2 + 3 + 4 = 10   // n^2 + n - 2 * uidx = 0
  {                                                                                                                         // 1-1,2-3,4-6,7-10
                                                                                                                            //  1,  3,  6,  10 
  }                                                                                                                         // +0, +1, +2,  +3
 else  // afGrowNone   // All blocks of same size (MinSize)  // NOTE: Will waste a lot of memory if the object size is more than 20% of MinSize
  {
   SCVR size_t BlkSize  = MinSize - CtxSize;
   SCVR size_t ObjInBlk = (BlkSize / ObjSize); 
   BIdx = ObjIdx / ObjInBlk;      // Optimize?   // Should be fast if CtxSize is 0 (no afSinglePtr)
   OIdx = ObjIdx % ObjInBlk;
  }
} */
//-------------------------------------------------------------
SFHdr* inline GetCtx(void)
{
 if constexpr (CtxInFBlk)return this->Context;
  else &this->Context;
}
//-------------------------------------------------------------
int inline GrowBlkIdxArrFor(size_t BlkIdx)
{
 if constexpr (SepBlkIdx)
  {

  }
   else return -1;
}
//-------------------------------------------------------------
/*static bool IsBlkPtrEmpty(vptr ptr)     // The block is still allocated but marked as empty    
{
 return (size_t)ptr & dbfEmpty;
}
//-------------------------------------------------------------
static bool IsBlkPtrDeleted(vptr ptr)   // The block is deleted and the pointer is actually an encoded index in chain of deleted blocks
{
 return (size_t)ptr & dbfDeleted;
}*/
//-------------------------------------------------------------
       // How do we work with blocks?
public:
// Allocates number of elements. !!! How to track how many free slots left in the block? Do we have to? Probably that is owner of the allocator should do.
int Alloc(size_t ElmNum)   // Can`t return a pointer - May be split in several blocks
{
 size_t ClcBlk=0, ClcIdx=0;
 //this->Strat.CalcForIndex(idx, ClcBlk, ClcIdx);
 return 0;
}
//-------------------------------------------------------------  // We must be able to mark blocks as unused without deallocation
// Deallocates a single block               // For a big allocation range a bigger block can be selected if no Sequential flag is specified (Object orded in memory is not important)
int Delete(size_t BlkIdx)                   // !!!>>> How to find free blocks faster than iteration? How to know how many items will fit in a block?  // XOR pointers? (Of prev and next free blk index)
{
 SFHdr* ctx = this->GetCtx();
 if(BlkIdx >= ctx->Blocks.BlkNum)return -1;  // Out of range
 vptr blk = ctx->Blocks.BlkArr[BlkIdx];
 if(!blk)return -2;       // Not allocated
 if constexpr (CtxInFBlk)  // The context is in first block
  {
   if(blk == (vptr)ctx)return -3;     // Cannot deallocate first the block 
  }
 if((BlkIdx+1) == ctx->Blocks.BlkNum)ctx->Blocks.BlkNum--;   // It is last of non NULL pointers
 ctx->Blocks.BlkArr[BlkIdx] = nullptr;
 size_t blen = this->Strat.BlkIdxToSize(BlkIdx);
 if(NPTM::NAPI::munmap(blk, blen) < 0)return -4;
 return 0;
}  
//-------------------------------------------------------------    // Optionally iterate over empty/unallocated blocks?
// Gets range for the element index. Returns 1 if the range is last one    // ??? Use this to allocate memory?   // MakeRange/AllocRange?
int GetRange(size_t ElmIdx, Ty*& Ptr, Ty*& End) // Return Indexes and the block????  // Used by iterators, should be branchless    // TODO: Separate allocation for index
{
 size_t BlkIdx, ObjIdx, Offs, Size;
 SFHdr* ctx = this->GetCtx();
 this->Strat.CalcForIndex(ElmIdx, BlkIdx, ObjIdx);   // NOTE: No ElmIdx check. CalcForIndex may return nonsense!
// if(BlkIdx >= ctx->Blocks.BlkNum)return -1;  // Out of range (The index part itself may be not allocated yet)
 vptr blk = ctx->Blocks.BlkArr[BlkIdx];   // Branchless: Just crash if the pointer is NULL or worse if the BlkIdx is out of range! (Anyone who uses this allocator should keep track of indexes)
// if(!blk)return -2;        // Not allocated
 size_t ObjNum = this->Strat.BlkIdxToObjInf(BlkIdx, Offs, Size);
 if constexpr (SparseBlk)  // Can preallocate here(may lead to allocation loop untill no free memory is left), iterator always points to the context and updates its end pointer
  {
   if(!blk)
    {
     vptr blk = (vptr)NPTM::NAPI::mmap(nullptr, Size, PX::PROT_READ|PX::PROT_WRITE, PX::MAP_PRIVATE|PX::MAP_ANONYMOUS, -1, 0);
     if(NPTM::GetMMapErrFromPtr(blk)){Ptr = End = nullptr; return -3;}
     ctx->Blocks.BlkArr[BlkIdx] = blk;
     if(BlkIdx >= ctx->Blocks.BlkNum)ctx->Blocks.BlkNum = BlkIdx+1;
    }
  }
 Ty* OBPtr = (Ty*)((uint8*)blk + Offs);
 Ptr = &OBPtr[ObjIdx];
 End = &OBPtr[ObjNum];
 return ((BlkIdx+1) == ctx->Blocks.BlkNum);    // Never last one if sparse
}     
//-------------------------------------------------------------
// Will allocate if there is unallocated block for this index 
// It will not allocate for the entire range up to ElmIdx, only the block range that contains ElmIdx
//
int EnsureRange(size_t ElmIdx, Ty*& Ptr, Ty*& End) 
{
 size_t BlkIdx, ObjIdx, Offs, Size;
 SFHdr* ctx = this->GetCtx();
 this->Strat.CalcForIndex(ElmIdx, BlkIdx, ObjIdx);   // NOTE: No ElmIdx check. CalcForIndex may return nonsense!
 if((BlkIdx >= ctx->Blocks.GetArrLen()) && (this->GrowBlkIdxArrFor(BlkIdx) < 0))return -2;    // Cannot grow
 vptr blk = ctx->Blocks.BlkArr[BlkIdx];   
// if(!blk)return -2;        // Not allocated
 size_t ObjNum = this->Strat.BlkIdxToObjInf(BlkIdx, Offs, Size);
 if constexpr (SparseBlk)  // Can preallocate here(may lead to allocation loop untill no free memory is left), iterator always points to the context and updates its end pointer
  {
   if(!blk)
    {
     vptr blk = (vptr)NPTM::NAPI::mmap(nullptr, Size, PX::PROT_READ|PX::PROT_WRITE, PX::MAP_PRIVATE|PX::MAP_ANONYMOUS, -1, 0);
     if(NPTM::GetMMapErrFromPtr(blk)){Ptr = End = nullptr; return -3;}
     ctx->Blocks.BlkArr[BlkIdx] = blk;
     if(BlkIdx >= ctx->Blocks.BlkNum)ctx->Blocks.BlkNum = BlkIdx+1;
    }
  }
 Ty* OBPtr = (Ty*)((uint8*)blk + Offs);
 Ptr = &OBPtr[ObjIdx];
 End = &OBPtr[ObjNum];
 return ((BlkIdx+1) == ctx->Blocks.BlkNum);    // Never last one if sparse
}                           
//-------------------------------------------------------------
// Free everything, including tables and metadata if any
int Release(void)
{
 SFHdr* ctx    = this->GetCtx();
 size_t BASize = ctx->Blocks.GetArrLen();
 vptr*  BAPtr  = ctx->Blocks.GetArrPtr();   // May be in a separate block
 for(sint bidx=ctx->Blocks.BlkNum-1;bidx >= 0;bidx--)  // Last block to free may contain the context
  {
   vptr   blk  = ctx->Blocks.BlkArr[bidx];
   if(!blk)continue;
   size_t blen = this->Strat.BlkIdxToSize(bidx);
   if(NPTM::NAPI::munmap(blk, blen) < 0)return -1;     
  }
 if constexpr(SepBlkIdx)
  {
   if(BAPtr && (NPTM::NAPI::munmap(BAPtr, BASize * sizeof(vptr)) < 0))return -2;  // The block index was a separate allocation
  }
 memset(&this->Context,0,sizeof(this->Context));    // May be pointer or the context itself
 return 0;
}
//-------------
};
//------------------------------------------------------------------------------------------------------------
// MinLen: Minimal expected number of (Ty) elements.
// MaxLen: Maximal expected number of (Ty) elements. Depending on flags, allocation of more elements may be not possible or inefficient
// TCIfo: Per context (allocator) user defined info
// TBIfo: Per block user defined info
// HPad:  Padding of headers to keep the data properly aligned
//
template<size_t MinLen, size_t MaxLen, uint32 Flg, uint32 HPad=sizeof(size_t), typename Ty=uint8, typename TCIfo=SEmptyType, typename TBIfo=SEmptyType> class CDynAlloc: public TSW<(Flg & afSequential), SSeqAlloc<MinLen,MaxLen,Flg,HPad,Ty,TCIfo,TBIfo>, SBlkAlloc<MinLen,MaxLen,Flg,HPad,Ty,TCIfo,TBIfo> >::T
{
// SCVR size_t ObjLen = sizeof(Ty);
 STASRT(MaxLen >= MinLen);
 STASRT(ObjLen <= MinLen);
//------------------------------------------------------------------------------------------------------------
public:

class Iterator    // I do not like clumsy ranged FOR but i need iterator object to hide iteration between blocks. :: for (auto it = integers.begin(), end = integers.end(); it != end; ++it) {const auto i = *it; ... }
{
 Ty*     Ptr;     // Current Block ptr (Points to first object, after a header if present)
 size_t  Idx;     // Current idx it points to in the block
 size_t  FIdx;    // Index of first obj in the block
 size_t  LIdx;    // Index of last obj in the block

public:
 // Iterator tags here...

 // Iterator constructors here...

 Ty& operator*() const { return *Ptr; }
 Ty* operator->() { return Ptr; }

 // Prefix 
 Iterator& operator++() { Ptr++; return *this; }       // Need also decrement operators for backtracing
 Iterator& operator--() { Ptr--; return *this; }  

 // Postfix 
 Iterator operator++(int) { Iterator tmp = *this; ++(*this); return tmp; }
 Iterator operator--(int) { Iterator tmp = *this; --(*this); return tmp; }     // Not used by ranged FOR (Clumsy, i don`t like them anyway)

 friend bool operator== (const Iterator& a, const Iterator& b) { return a.Ptr == b.Ptr; };
 friend bool operator!= (const Iterator& a, const Iterator& b) { return a.Ptr != b.Ptr; };     
};
//------------------------------------------------------------------------------------------------------------

CDynAlloc(void){}
~CDynAlloc(){this->Release();}    
//------------------------------------------------------------------------------------------------------------
Iterator begin(void)     // NOTE: No way avoid iteration over deleted objects!
{ 
 return Iterator(&m_data[0]); 
}
//------------------------------------------------------------------------------------------------------------
Iterator end(void)       // Iterates up until last non NULL block (NULL blocks are skipped)
{ 
 return Iterator(&m_data[200]); 
}
//------------------------------------------------------------------------------------------------------------


/*
 SCVR bool HaveBlkList  = (Flg & afObjAlloc) && !(Flg & (afNoMove|afSequential));    // Must have a block list if the data is not in a single block
 SCVR bool HaveFreeBM   = (Flg & afObjDeAlloc) && !(Flg & afSequential);
 SCVR bool SinglePtr    = (Flg & afSinglePtr);
// SCVR bool MayGrow      = (Flg & afSequential) && !(Flg & afNoMove);   // afNoMove is allowed to grow inplace but allowed to fail   // Not useful for BitMap

 SCVR size_t Flags      = Flg;
 SCVR size_t AlnSize    = AlignP2Up(AlnLen);   // Must be Pow2   // afObjAlloc: sizeof(obj) can be passed directly. MaxLen, MinLen is in number of objects

 SCVR size_t MemMinSize = (Flg & afObjAlloc)?(AlignP2Frwd(MinLen*AlnSize, NPTM::MEMGRANSIZE)):(AlignP2Frwd(Max(MinLen,AlnSize), NPTM::MEMGRANSIZE));
 SCVR size_t MemMaxSize = (Flg & afObjAlloc)?(AlignP2Frwd(MaxLen*AlnSize, NPTM::MEMGRANSIZE)):(AlignP2Frwd(Max(MaxLen,AlnSize), NPTM::MEMGRANSIZE));

//-------------------------------------------------------------------
// Each block can be of 'BlkMinSize', 'LastBlkSize + BlkMinSize', 'LastBlkSize * 2' until total MemMaxSize is reached
//
static consteval uint BlkList_CalcSize(void)  // Calculate base size for BlockList (Cached)
{
 switch(Flags & afGrowMask)
  {
   case afGrowNone: return 0;
   case afGrowLin:  return 0;
   case afGrowExp:  return 0;
  }
 return 0;
}
//-------------------------------------------------------------------
static consteval uint FBitMap_CalcSize(void)  // Calculate base size for BitMap of free objects (Cached)
{
 return 0;
}
//-------------------------------------------------------------------
static consteval uint CalcContextSize(void)  // Calculate base size for info memory (Cached)
{
 return 0;
}
//-------------------------------------------------------------------
 SCVR size_t BlkLstSize = BlkList_CalcSize();    // In pointers
 SCVR size_t BitMapSize = FBitMap_CalcSize();
 SCVR size_t CtxMemSize = CalcContextSize();     // Unable to grow if limited by it
//-------------------------------------------------------------------
 struct SBlkLst
 {

 };
//-------------------------------------------------------------------
 struct SBitMap
 {

 };
//-------------------------------------------------------------------
 struct SCtx
 {
  SCVR size_t CtxLen = AlignP2Frwd(sizeof(SCtx),AlnSize); 
  //SCVR size_t InfLen = AlignP2Frwd(CtxLen+,NPTM::MEMGRANSIZE); 

  vptr MemPtr;    // Memory block/List of memory blocks
  uint MemLen;

//------------------------------------
  vptr GetPtr(void)
  {
   if constexpr (SinglePtr)
    {
     return (vptr)((size_t)this + CtxMemSize);    // It is faster to add a constant than read the actual pointer from memory
    }
     else return this->MemPtr;
  }
//------------------------------------
 };

 typename TSW<SinglePtr, SCtx*, SCtx>::T  ctx;       // The context as a pointer or right there


 //typename TSW<HaveFreeBM, vptr, ETYPE>::T  FreeBM;
 //typename TSW<HaveFreeBM, uint, ETYPE>::T  FreeBMSize;

 //typename TSW<HaveBlkList, vptr, ETYPE>::T  BlkList;
 //typename TSW<HaveBlkList, uint, ETYPE>::T  BlkListSize;
//-------------------------------------------------------------------
SCtx* GetCtx(void)
{
 if constexpr (SinglePtr)     // Not in all cases can be a part of same memory block
  {
   return this->ctx;    // Allocate if not done yet
  }
  else return &this->ctx;
}
//-------------------------------------------------------------------
vptr BlkList_AllocBlk(uint Index)
{
// SCVR BlkListSize =      // Must cover at least BlkMaxSize * 2
 return nullptr;
}

//-------------------------------------------------------------------
public:
vptr Alloc(size_t ForLen)    // Base
{
 if constexpr (HaveBlkList)
  {
   if constexpr (HaveFreeBM)   // Try to find a free slot first
    {
     // ??? Return a free slot if found
    }
   return this->Alloc_BlkList(ForLen);
  }
 else
  {
   if constexpr (HaveFreeBM)   // Try to find a free slot first
    {

    }
   // ???
  }
}
//-------------------------------------------------------------------
vptr ReAlloc(size_t ForLen, size_t At)  // Base       // Grow/shrink the entire memory. 'At' is a hint
{
 return nullptr;
}
//-------------------------------------------------------------------
void DeAlloc(size_t ForLen, size_t At)  // Base
{

}
//-------------------------------------------------------------------
void Release(void)
{

}
*/
//-------------------------------------------------------------------
};

};
//------------------------------------------------------------------------------------------------------------
//============================================================================================================