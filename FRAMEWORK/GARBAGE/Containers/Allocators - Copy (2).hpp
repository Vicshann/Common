
/*
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
enum EAllocFlags {
 afNone        = 0,
 afNoMove      = 0x0010,   // For afSequential but delete is forbidden(How to error on it?). Allocate only from an initially reserved single block of memory(BlkLen). No moving is attempted (delete, extend) and pointers are always preserved. An attempt to grow may fail.
 afSequential  = 0x0020,   // The data in memory must always be sequential to be accessed by a raw pointer. Will use reallocation and copy, cannot guarantee pointer preservation. No free slot map, just a used/allocated sizes
 afObjAlloc    = 0x0040,   // Allocate by objects of specific size(Pow2 only, specified by AlnLen). Will keep a list of blocks if no afArena or afSequential is specified.
 afObjDeAlloc  = 0x0080,   // Will have a list of free objects and then will try to allocate in them first. Can be used only with afObjAlloc
 afThreadSafe  = 0x0100,   // TODO! Thread safe operations only (No moving of internal structures like BlkList or BitMap)
 afSinglePtr   = 0x0200,   // Use only single pointer to reference the memory (Store any associated info elsewhere).  // Sometimes an array is a member of some struct that is allocated in vast numbers but not all of them will have data
 afBlkMetadata = 0x0400,   // Store some metadata at beginning of each block (Like the allocator pointer, and some user pointer)

// Prealloc/Grow modes ?
 afGrowMask    = 0x000F,
 afGrowNone    = 0x0000,   // Size
 afGrowLin     = 0x0001,   // LastSize+Size
 afGrowExp     = 0x0002,   // LastSize * 2

};
//============================================================================================================
// NOTE: Not a malloc type allocator. Use only for specialized data structures. The memory is not always shrinkable (Even free objects keep the holes reserved)
// NOTE: A default allocator. Should respect all generic policy states at compile time
// NOTE: BlkLen, Align will have different meaning dependant on Flags
// MaxLen: Max block size / max number of objects
// MinLen: Min number of objects expected/ Min Block size
// AlnLen: Alignment/size of an object
// Allocations up to MaxLen will not fail
//
template<size_t MaxLen, size_t MinLen, uint32 Flg, typename T=uint8> class CAllocator
{
 SCVR size_t ObjLen = sizeof(T);
 STASRT(MaxLen >= MinLen);
 STASRT(ObjLen <= MinLen);
//------------------------------------------------------------------------------------------------------------
template<size_t MaxLen, size_t MinLen, size_t ObjLen, uint32 Flg> struct SMemAlloc   // Arbitrary memory allocator. Single block. Single linear range.
{
 vptr Allocate(size_t Size){return nullptr;}
 vptr Delete(size_t At, size_t Size){return nullptr;}  // If afNoMove is not present
};
//------------------------------------------------------------------------------------------------------------
template<size_t MaxLen, size_t MinLen, size_t ObjLen, uint32 Flg> struct SObjAlloc   // Same sized chunks allocator. Multiple blocks. Multiple linear ranges.
{
 vptr Allocate(size_t Size){return nullptr;}
 vptr Delete(size_t At, size_t Size){return nullptr;}  // If afObjDeAlloc is present
};
//------------------------------------------------------------------------------------------------------------
 using TA = TSW<(Flg & afSequential), SMemAlloc<MaxLen,MinLen,ObjLen,Flg>, SObjAlloc<MaxLen,MinLen,ObjLen,Flg> >::T;
 TA Alloc;
//------------------------------------------------------------------------------------------------------------
public:
CAllocator(void){}
~CAllocator(){}      // this->Release();
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

//------------------------------------------------------------------------------------------------------------
//============================================================================================================