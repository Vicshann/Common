
//------------------------------------------------------------------------------------------------------------
enum EAllocFlags {
 afNone       = 0,
 afNoMove     = 0x0010,   // Allocate only from an initially reserved single block of memory(BlkLen). No moving is attempted (delete, extend) and pointers are always preserved. An attempt to grow may fail.
 afSequential = 0x0020,   // The data in memory must always be sequential to be accessed by a raw pointer. Will use reallocation and copy, cannot guarantee pointer preservation. No free slot map, just a used/allocated sizes
 afObjAlloc   = 0x0040,   // Allocate by objects of specific size(Pow2 only, specified by AlnLen). Will keep a list of blocks if no afArena or afSequential is specified.
 afObjDeAlloc = 0x0080,   // Will have a list of free objects and then will try to allocate in them first. Can be used only with afObjAlloc
 afThreadSafe = 0x0100,   // Thread safe operations only (No moving of internal structures like BlkList or BitMap)
 afSinglePtr  = 0x0200,   // Use only single pointer to reference the memory (Store any associated info elsewhere)   // Sometime an array is a member of some struct that is allocated in vast numbers but not all of them will have members

// Prealloc/Grow modes ?
 afGrowMask   = 0x000F,
 afGrowNone   = 0x0000,   // Size
 afGrowLin    = 0x0001,   // LastSize+Size
 afGrowExp    = 0x0002,   // LastSize * 2

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
template<size_t MaxLen, size_t MinLen, size_t AlnLen, int Flg> class CAllocator
{
 STASRT(MaxLen >= MinLen);

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
//-------------------------------------------------------------------
};

//------------------------------------------------------------------------------------------------------------
//============================================================================================================