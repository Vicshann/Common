
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
 afObjAlloc    = 0x0040,   // Allocate by objects of specific size(Pow2 only, specified by AlnLen). Will keep a list of blocks if no afArena or afSequential is specified.
 afObjDeAlloc  = 0x0080,   // Will have a list of free objects and then will try to allocate in them first. Can be used only with afObjAlloc
 afObjLowFrag  = 0x0100,   // Try to keep fragmentation low by sorting removed objects in free list (Slower delete)
 afThreadSafe  = 0x0100,   // TODO! Thread safe operations only (No moving of internal structures like BlkList or BitMap)
 afSinglePtr   = 0x0200,   // Use only single pointer to reference the memory (Store any associated info elsewhere).  // Sometimes an array is a member of some struct that is allocated in vast numbers but not all of them will have data
 afSmallMem    = 0x0400,   // Do not preallocate early and keep preallocation as small as possible (More conditions while allocating/accessing and more syscalls for mem grow)
 afBlkMetadata = 0x0800,   // Store some metadata at beginning of each block (Like the allocator pointer, and some user pointer)
 afSparseIndex = 0x1000,   // Allocate blocks if their pointers are NULL else report errors on access by unallocateed index.

// Prealloc/Grow modes ?
 afGrowMask    = 0x000F,
 afGrowNone    = 0x0000,   // Size
 afGrowLin     = 0x0001,   // LastSize+Size
 afGrowGeo     = 0x0002,   // LastSize * 2

};
//============================================================================================================
//                                      Block access strategies
//------------------------------------------------------------------------------------------------------------
enum EStrategyFlags 
{
 sfHdrInFirstBlkOnly = 0x01,  // The header is only present in first block (Main header)
 sfRoundObjLenToPow2 = 0x02,  // Add not more than half of the object size to it to be perfectly fit in memory blocks (No effect if headers are present in each block)
};
//------------------------------------------------------------------------------------------------------------
template<size_t ObjLen, size_t HdrLen=0, size_t PageLen=NPTM::MEMPAGESIZE, size_t Flags=0, size_t ArrLen=32> struct SBASGeom   // Grow geometrically (geometric progression with base 2)
{
private:
 SCVR size_t ObjLenP2  = AlignP2Up(ObjLen);
 SCVR size_t PageLenP2 = AlignP2Up(PageLen);
 SCVR bool   ObjMayExt = (ObjLenP2 != ObjLen) && ((ObjLenP2 - ObjLen) <= (ObjLen / 2));   // If the object may expand to Pow2
 SCVR bool   NoHdrInEB = ((Flags & sfHdrInFirstBlkOnly) || !HdrLen);   // The header is in first block only or not present at all
public:
 SCVR size_t ObjSize   = ((Flags & sfRoundObjLenToPow2) && NoHdrInEB && ObjMayExt)?(ObjLenP2):(ObjLen);
 SCVR size_t PageSize  = ((PageLenP2 - HdrLen) < ObjSize)?(ObjLenP2 << 1):(PageLenP2);    // Base/First page size

//-------------------------------------------------------------
size_t BlkIdxToSize(size_t Index) const {return 0;}   // TODO: From base page size
//-------------------------------------------------------------
// Get range for block?
// 
//-------------------------------------------------------------
void CalcForIndex(size_t Index, size_t& Blk, size_t& Idx) const
{
 if constexpr (IsPowOfTwo(ObjSize) && NoHdrInEB)   // Number of objects in a block is Pow2 as the block itself (No tail bytes, no headers in each block)    // May be needed to exclude size of first header
  {
   SCVR size_t ObjsOnFPage   = PageSize / ObjSize;
   SCVR size_t ObjBaseBitIdx = ctz(ObjsOnFPage);
   if constexpr (HdrLen)      // First block contains the header
    {
     SCVR size_t FPOLess = ObjsOnFPage - ((PageSize-HdrLen) / ObjSize);   // How many objects we lose because of the header  // If FPOLess is 1 will its multiplication be optimized out?  
     Index += (Index >= (ObjsOnFPage - FPOLess)) * FPOLess;  // This one is probably better since FPOLess may happen to be Pow2 and optimized to aleft shift // !OPTIMIZE!: Will it be brancless on ARM too?   // if(Index >= (ObjsOnFPage - FPOLess))Index += FPOLess;  
    }
   size_t ratio = (Index >> ObjBaseBitIdx) + 1;   // ++ later?  // Reduce and round-up
   size_t BIdx  = BitWidth(ratio >> 1);  
   size_t ISub  = ((size_t)1 << (BIdx + ObjBaseBitIdx)) - ObjsOnFPage;  
   Idx = Index - ISub;     
   Blk = BIdx;
  }
 else  // Uncomputable number of objects per block (Wasted tails, headers in each block)
  {
   struct SApArr     // Is there another way to initialize an array at compile time by a consteval function?
   {
    size_t Arr[ArrLen];
    consteval SApArr(void)        
     {
      for(size_t i=0,v=0,h=HdrLen;i < ArrLen;v+=(((PageSize << i)-h)/ObjSize),i++)
       {
        this->Arr[i] = v;
        if constexpr(Flags & sfHdrInFirstBlkOnly)h = h * !i;   // If (i >= 1) then h should be 0
       }
     }
   } SCVR RangeArr;    // Allocates in rdata section
   SCVR uint32 PageBitIdx = (BitSize<size_t>::V - clz(PageSize));  // - 1;  PageSize itself is index 0    // Must be done at compile time somehow // Initial page size will be block index 0
 
   uint64 ObjByteOffs  = Index * ObjSize;                  // As in a contigous memory block    
   uint32 ApproxGrpIdx = (BitSize<uint64>::V - clz(ObjByteOffs >> PageBitIdx));     // log2    // Actual page-aligned address   // Approximate (there are some wasted bytes at end of each block because an object cannot be split between blocks. But the index calculation is done as if for a single contiguous block of memory)
   
   uint32 IdxEx = ApproxGrpIdx + 1;                // The error will not cross more than one block because each next block is twice as large than a previous one
   if(Index >= RangeArr.Arr[IdxEx])ApproxGrpIdx = IdxEx;  // Likely   // cmov?
   Idx = (Index - RangeArr.Arr[ApproxGrpIdx]);     // Sometimes means rereading from the same array position
   Blk = ApproxGrpIdx; 
  }
}
//-------------------------------------------------------------
#ifdef DBGBUILD
void BruteForIndex(size_t Index, size_t& Blk, size_t& Idx) const  // For testing
{
 size_t Ps   = 0;
 size_t HLen = HdrLen;
 for(int idx=0;idx < ArrLen;idx++)
  {
   size_t Nx = (((PageSize << idx)-HLen)/ObjSize);
   size_t Lx = Ps;
   Ps += Nx;
   if(Index < Ps){Idx = Index-Lx; Blk = idx; return;}
   if constexpr (HdrLen && (Flags & sfHdrInFirstBlkOnly))HLen = 0;
  }
 Blk = -1;
 Idx = -1;
}
#endif
//-------------------------------------------------------------
};  
//============================================================================================================

//============================================================================================================


//============================================================================================================
// NOTE: Not a malloc type allocator. Use only for specialized data structures. The memory is not always shrinkable (Even free objects keep the holes reserved)
// NOTE: A default allocator. Should respect all generic policy states at compile time
// NOTE: BlkLen, Align will have different meaning dependant on Flags
// MaxLen: Max block size / max number of objects
// MinLen: Min number of objects expected/ Min Block size
// AlnLen: Alignment/size of an object
// Allocations up to MaxLen will not fail
//
template<size_t MaxLen, size_t MinLen, uint32 Flg, typename T=uint8> class SMemAlloc   // Arbitrary memory allocator. Single block. Single linear range.
{
public:
 vptr Allocate(size_t Size){return nullptr;}
 vptr Delete(size_t At, size_t Size){return nullptr;}  // If afNoMove is not present
 void Release(void){}
};
//------------------------------------------------------------------------------------------------------------
template<size_t MaxLen, size_t MinLen, uint32 Flg, typename T=uint32> class SObjAlloc   // Same sized chunks allocator. Multiple blocks. Multiple linear ranges.
{
 SCVR size_t CtxSize  = sizeof(vptr) * 8;   // Reserved size 32/64  // int512 on x64?   // Should be enough for any AVX alignment requirement
 SCVR size_t ObjSize  = ((Flg & afObjDeAlloc) && (sizeof(T) < sizeof(uint32)))?(sizeof(uint32)):(sizeof(T));   // Need uint32 for free slots index
 SCVR size_t MinSize  = AlignP2Frwd(MinLen, NPTM::MEMPAGESIZE);     // Grow dependant!!!
 SCVR size_t MaxSize  = AlignP2Frwd(MaxLen, NPTM::MEMGRANSIZE);     // Grow dependant!!!   // Index will be preallocated for this value?
 SCVR size_t GrowType = Flg & afGrowMask;

 STASRT(GrowType <= afGrowGeo);
  



struct SCtx  // Size is 32/40   // Cannot be part of the allocator if afBlkMetadata is specified because the allocator may be moved // Can be member of SObjAlloc, at beginning of block index or at beginning of the first block(bad idea). Should not be moved(Pointer to it may be stored in each block)
{
// enum EFlags {cfThisIsFirstBlk=0x01, cfNoBlkArrYet=0x02};       // If cfNoBlkArrYet then BlkArray points to the first block directly

 SCtx*   Self;      // Points to beginning of the block(This SHdr)  // To find the block base in nonlinear allocation   
 union
  {
   SCtx*   Ctx;       // The allocator context pointer   // BlkArray in first block
   vptr    FirstBlk;
   vptr**  BlkArray;  // Separate allocation to be able to grow. If afSmallMem is specified then allocated only when the first block is full
  };
 uint32  FSLnk;     // XorFB link to a first free slot in this block (Offset) // Max block size is 4gb
 uint32  FBlkLnk;   // XorFB link to blocks that contain some free slots (Index) (Faster to find free sequential slots in a single block)
 uint32  BlkArrCnt; // Only first block (Index for others)
 uint32  Flags;
 uint64  Tag;       // Some user-specified index or a pointer
};
//-------------------------------------------------------------
static void ObjIdxToBlkIdx(size_t ObjIdx, size_t& BIdx, size_t& OIdx)   // Dependant on grow type
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
}
//-------------------------------------------------------------

public:
 vptr Allocate(size_t Size){return nullptr;}
 vptr Delete(size_t From, size_t Cnt){return nullptr;}  // If afObjDeAlloc is present
 bool GetRange(size_t From, vptr* Beg, vptr* End){return false;}     // True if the last one
 void Release(void){}
};
//------------------------------------------------------------------------------------------------------------
template<size_t MaxLen, size_t MinLen, uint32 Flg, typename Ty=uint8> class CDynAlloc: public TSW<(Flg & afSequential), SMemAlloc<MaxLen,MinLen,Flg,Ty>, SObjAlloc<MaxLen,MinLen,Flg,Ty> >::T
{
 SCVR size_t ObjLen = sizeof(Ty);
 STASRT(MaxLen >= MinLen);
 STASRT(ObjLen <= MinLen);
//------------------------------------------------------------------------------------------------------------
public:
CDynAlloc(void){}
~CDynAlloc(){this->Release();}    
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