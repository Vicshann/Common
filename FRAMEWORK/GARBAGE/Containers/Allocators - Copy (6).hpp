
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
 afBlkTrcOwner = 0x0800,   // Store some metadata at beginning of each block (Like the allocator pointer, and some user pointer)
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
 SCVR size_t RangeMin  = 32;  // Should be enough for x32 and x64 even if base page size is 4096 (16TB max) (RangeNum times : PageSize = (PageSize * 2))
 SCVR size_t RangeMax  = 32;
 SCVR size_t FHdrSize  = FHdrLen;   // To make them available from outside
 SCVR size_t NHdrSize  = NHdrLen;
 SCVR size_t ObjSize   = ObjLen;
 SCVR size_t PageSize  = AlignP2Up(((PageLen - Max(FHdrLen, NHdrLen)) < ObjSize)?(ObjSize+Max(FHdrLen, NHdrLen)):(PageLen));    // Base/First page size
 SCVR size_t TagMask   = ((size_t)NPTM::MEMPAGESIZE - 1);   // Do not use PageSize to keep it consistent with other strategies
 SCVR size_t PtrMask   = ~TagMask;   

//-------------------------------------------------------------
static inline uint BlkIdxToHdrLen(uint32 BlkIdx)        // TODO: Branchless
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
static uint CalcForIndex(size_t ObjIdx, size_t& Idx)      // TODO: Check if ObjIdx is outside of valid range? (For now this function is branchless)
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
   return BIdx;   // Block index
  }
 else  // Uncomputable number of objects per block (Wasted tails, headers in each block)
  {
   struct SApArr     // Is there another way to initialize an array at compile time by a consteval function?
   {
    size_t Arr[RangeMin];
    consteval SApArr(void) {for(size_t i=0,v=0,h=0;i < RangeMin;h = (i)?NHdrLen:FHdrLen, v += (((PageSize << i)-h)/ObjSize), i++)this->Arr[i] = v;}
   } SCVR RangeArr;    // Allocates in rdata section
   SCVR uint32 PageBitIdx = (BitSize<size_t>::V - clz(PageSize));  // - 1;  PageSize itself is index 0    // Must be done at compile time somehow // Initial page size will be block index 0
 
   uint64 ObjByteOffs  = ObjIdx * ObjSize;                  // As in a contigous memory block    
   uint32 ApproxGrpIdx = (BitSize<uint64>::V - clz(ObjByteOffs >> PageBitIdx));     // log2    // Actual page-aligned address   // Approximate (there are some wasted bytes at end of each block because an object cannot be split between blocks. But the index calculation is done as if for a single contiguous block of memory)
   ApproxGrpIdx += (ObjIdx >= RangeArr.Arr[ApproxGrpIdx + 1]);  // The error will not cross more than one block because each next block is twice as large than a previous one
   Idx = (ObjIdx - RangeArr.Arr[ApproxGrpIdx]);     // Sometimes means rereading from the same array position
   return ApproxGrpIdx;  // Block index
  }
}
//-------------------------------------------------------------
#ifdef DBGBUILD
static uint BruteForIndex(size_t ObjIdx, size_t& Idx)  // For testing
{
 size_t Ps   = 0;
 size_t HLen = FHdrLen;
 for(int idx=0;idx < RangeMin;idx++)
  {
   size_t Nx = (((PageSize << idx)-HLen)/ObjSize);
   size_t Lx = Ps;
   Ps += Nx;
   if(ObjIdx < Ps){Idx = ObjIdx-Lx; return idx;}
   if constexpr (NHdrLen)HLen = NHdrLen;
     else HLen = 0;
  }
 Idx = -1;
 return -1;
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
 for(uint idx=From;idx < 0xFFFFFFFF;idx++)
  {
   size_t ClcIdx, BrtIdx;
   size_t ClcBlk = str.CalcForIndex(idx, ClcIdx);
   size_t BrtBlk = str.BruteForIndex(idx, BrtIdx);
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
template<size_t MinLen, size_t MaxLen, uint32 Flg, uint32 HPad=sizeof(size_t), typename Ty=uint8, typename TCIfo=SEmptyType, typename TBIfo=SEmptyType> class SSeqAlloc   // Arbitrary memory allocator. Single block. Single linear range.
{
public:
 vptr Allocate(size_t Size){return nullptr;}
 vptr Delete(size_t At, size_t Size){return nullptr;}  // If afNoMove is not present
 void Release(void){}
};
//------------------------------------------------------------------------------------------------------------
template<size_t MinLen, size_t MaxLen, uint32 Flg, uint32 HPad=sizeof(size_t), typename Ty=uint32, typename TCIfo=SEmptyType, typename TBIfo=SEmptyType> class SBlkAlloc   // Same sized chunks allocator. Multiple blocks. Multiple linear ranges.
{
using TBStrat = SBASGeom<0>;    // To get access to constants
SCVR bool SepBlkIdx = TBStrat::RangeMax != TBStrat::RangeMin;    // Block index array is a separate allocation (For strategies that require large indexes for blocks)
SCVR bool MetaInBlk = (Flg & afBlkTrcOwner);    // Metadata is added to every block (To find the block owner)  // Forces context to be in the first block
SCVR bool CtxInFBlk = MetaInBlk || (Flg & afSinglePtr);          // The context is in the first block (Forced by MetaInBlk)

struct SBlkArrI    // Only for SBASGeom
{
 size_t BlkNum;
 vptr   BlkArr[TBStrat::RangeMin];  

 inline void Init(void){}
 static inline uint GetArrLen(void) {return sizeof(BlkArr)/sizeof(vptr);}
 inline vptr*  GetArrPtr(void) {return BlkArr;}
};

struct SBlkArrP
{
 uint32 BlkNum;  // No allocated blocks is expected after this. But in the range some may be NULL
 uint32 BlkLen;  // Number of pointers total to fit in the array
 vptr*  BlkArr;  // Separate allocation to be able to grow. If afSmallMem is specified then allocated only when the first block is full    /// !!! Should not allocate index if only the first block exis
 vptr   Local[TBStrat::RangeMin];   // BlkArr Should point here initially

 inline void  Init(void){BlkArr = Local; BlkLen = TBStrat::RangeMin;}
 inline uint  GetArrLen(void) const {return BlkLen;}
 inline vptr* GetArrPtr(void) {return BlkArr;}
};

struct SMFHdr { vptr Self; };  // Points to beginning of the block(This SHdr)   

struct SFHdr: TSW<(MetaInBlk), SMFHdr, SEmptyType >::T     // For a first block // Cannot be part of the allocator if afBlkMetadata is specified because the allocator may be moved // Can be member of SObjAlloc, at beginning of block index or at beginning of the first block(bad idea). Should not be moved(Pointer to it may be stored in each block)
{
 TSW<(SepBlkIdx), SBlkArrI, SBlkArrP >::T Blocks; 
 TCIfo  CtxInfo;
};

struct SFHdrEx: SFHdr  // If the context is in the same block
{
 TBIfo  BlkInfo;
};

struct SMNHdr: SMFHdr { SFHdr* Ctx; };
struct SNHdr: TSW<(MetaInBlk), SMNHdr, SEmptyType >::T     // NOTE: Inheritance storage layout is unspecified. You must not make any assumptions about the class layout in memory.     
{
 TBIfo  BlkInfo;
};

using Strat = SBASGeom<sizeof(Ty), AlignP2Up(AlignP2Frwd(MinLen, NPTM::MEMPAGESIZE)), AlignP2Up(AlignP2Frwd(sizeof(SFHdr), AlignP2Up(HPad))), AlignP2Up(AlignP2Frwd(sizeof(SNHdr), AlignP2Up(HPad)))>;

TSW<(CtxInFBlk), SFHdr*, SFHdr >::T Context;
//-------------------------------------------------------------
inline SFHdr* GetCtx(void)
{
 if constexpr (CtxInFBlk)return this->Context;     // May be NULL if not initialized yet (GrowBlkIdxArrFor will do that)
  else &this->Context;
}
//-------------------------------------------------------------
     
public:
//-------------------------------------------------------------
inline SBlkAlloc(void){memset(this,0,sizeof(*this));}
inline ~SBlkAlloc(){this->Release();}
//------------------------------------------------------------- 
int Release(void)               // Free everything, including tables and metadata if any
{
 SFHdr* ctx    = this->GetCtx();
 if constexpr (CtxInFBlk) { if(!ctx)return 0; }     // Not initialized
 size_t BASize = ctx->Blocks.GetArrLen();
 vptr*  BAPtr  = ctx->Blocks.GetArrPtr();   // May be in a separate block
 for(sint bidx=ctx->Blocks.BlkNum-1;bidx >= 0;bidx--)  // Last block to free may contain the context
  {
   vptr   blk  = BAPtr[bidx];
   if(!blk)continue;
   size_t blen = Strat::BlkIdxToSize(bidx);
   if(NPTM::NAPI::munmap(blk, blen) < 0)return -1;     
  }
 if constexpr(SepBlkIdx)
  {
   if(BAPtr && (ctx->Blocks.GetArrLen() > Strat::RangeMin) && (NPTM::NAPI::munmap(BAPtr, BASize * sizeof(vptr)) < 0))return -2;  // The block index was a separate allocation
  }
 memset(&this->Context,0,sizeof(this->Context));    // May be pointer or the context itself
 return 0;
}
//-------------------------------------------------------------              
int DeleteBlk(size_t BlkIdx)     // Deallocates a single block               
{
 SFHdr* ctx = this->GetCtx();
 if constexpr (CtxInFBlk) { if(!ctx)return -1; }
 if(BlkIdx >= ctx->Blocks.BlkNum)return -2;  // Out of range
 vptr*  BAPtr = ctx->Blocks.GetArrPtr(); 
 vptr blk = BAPtr[BlkIdx];
 if(!blk)return -3;        // Not allocated
 if constexpr (CtxInFBlk)  // The context is in first block
  {
   if(blk == (vptr)ctx)return -4;     // Cannot deallocate first the block 
  }
 if((BlkIdx+1) == ctx->Blocks.BlkNum)ctx->Blocks.BlkNum--;   // It is last of non NULL pointers
 BAPtr[BlkIdx] = (vptr)this->GetBlkTag(BlkIdx);     // Leave the tag intact
 size_t blen = this->Strat.BlkIdxToSize(BlkIdx);
 if(NPTM::NAPI::munmap(blk, blen) < 0)return -5;
 return 0;
}  
//------------------------------------------------------------- 
vptr GetBlock(size_t BlkIdx)   // Allocates the block if it is not allocated yet
{
 if constexpr (!CtxInFBlk)     // The context is always available
  {
//   if(BlkIdx >= SBlkArrI::GetArrLen())return nullptr;     // Out of bounds
   if(vptr ptr=this->GetBlkPtr(BlkIdx);ptr)return ptr;      // Already allocated  // Useless if GrowBlkIdxArrFor returns 0 (Has grown) which is rare
  }
 else if(this->GetCtx())    // May be not initialized yet
  {
   if(vptr ptr=this->GetBlkPtr(BlkIdx);ptr)return ptr; 
  }
 size_t BSize = (size_t)this->Strat.BlkIdxToSize(BlkIdx);
 vptr   BPtr  = (vptr)NPTM::NAPI::mmap(nullptr, BSize, PX::PROT_READ|PX::PROT_WRITE, PX::MAP_PRIVATE|PX::MAP_ANONYMOUS, -1, 0);   // Executable?  // Some platforms may refuse allocating of RWX memory
 if(NPTM::GetMMapErrFromPtr(BPtr))return nullptr;
 if constexpr (CtxInFBlk)   
  {
   if(!this->GetCtx())   // No context is allocated yet
    {
     if(BlkIdx)          // Must make sure that the first block exist  // No need to allocate index for block 0 anyway (Stored locally)
      {
       if(!this->GetBlock(0))return nullptr;   
      }
       else this->Context = (SFHdr*)BPtr;  
    }
  }
 SFHdr* ctx = this->GetCtx();   // Will be available
 if constexpr (SepBlkIdx)   // Init/Grow the index array if required
  {
   if(!ctx->Blocks.GetArrPtr())ctx->Blocks.Init(); 
   if(BlkIdx >= ctx->Blocks.GetArrLen())
    {
     uint NMax = AlignP2Frwd(BlkIdx, NPTM::MEMPAGESIZE/sizeof(vptr)); 
     vptr IPtr = (vptr)NPTM::NAPI::mmap(nullptr, NMax*sizeof(vptr), PX::PROT_READ|PX::PROT_WRITE, PX::MAP_PRIVATE|PX::MAP_ANONYMOUS, -1, 0);   
     if(NPTM::GetMMapErrFromPtr(IPtr)){ NPTM::NAPI::munmap(BPtr, BSize); return nullptr; }    // Tolal failure, especially on the first block

     vptr   OPtr = ctx->Blocks.BlkArr;
     size_t OLen = ctx->Blocks.BlkLen*sizeof(vptr);
     memcpy(IPtr, OPtr, OLen);

     ctx->Blocks.BlkArr = IPtr;
     ctx->Blocks.BlkLen = NMax;
     NPTM::NAPI::munmap(OPtr, OLen);
    }
  }
 if constexpr(MetaInBlk)    // MetaInBlk Forces CtxInFBlk
  {
   if(BlkIdx)
    {
     SNHdr* hdr = (SNHdr*)BPtr;
     hdr->Self = BPtr;
     hdr->Ctx  = ctx;
    }
     else ctx->Self = vptr((size_t)BPtr | 1);    // First block, mark it
  }
 vptr*  Adr = &ctx->Blocks.GetArrPtr()[BlkIdx];
 size_t Tag = (size_t)*Adr & Strat::TagMask;
 *Adr = vptr((size_t)BPtr | Tag);     // Store the block ptr in the index array
 if(BlkIdx >= ctx->Blocks.BlkNum)ctx->Blocks.BlkNum = BlkIdx+1;
 return BPtr;   // Return Offset to beginning or to data?
}
//-------------------------------------------------------------
vptr FindBlock(vptr ElmAddrInBlk)   // NOTE: More grown the block - Slower this function!    // The element must know type of its container   // NOTE: Will crash if an invalid address is passed!
{
 if constexpr(MetaInBlk) 
  {
   for(size_t pbase=AlignP2Bkwd((size_t)ElmAddrInBlk, Strat::PageSize);;pbase -= Strat::PageSize)
    {
     SMFHdr* hdr = (SMFHdr*)pbase;
     if(((size_t)hdr->Self & Strat::PtrMask) == pbase)return (vptr)pbase;     // Nothing else to check here
    }
  }
   else return nullptr;
}
//-------------------------------------------------------------
Ty* GetBlkData(size_t BlkIdx, size_t BElmIdx)   
{
 size_t Ptr = (size_t)this->GetBlock(BlkIdx); // May allocate the block (do not pass some random numbers as BlkIdx)
 if(!Ptr)return nullptr;                      // The block is not allocated
  
 size_t EOffs = BElmIdx * this->Strat.ObjSize;
 size_t HSize = this->Strat.BlkIdxToHdrLen(BlkIdx);
 size_t DSize = (size_t)this->Strat.BlkIdxToSize(BlkIdx) - HSize;   // NOTE: May trim the size on x32, no checks
 if(EOffs >= DSize)return nullptr;            // The element is out of range for the block
 return (Ty*)(Ptr + HSize + EOffs);
}
//-------------------------------------------------------------
// ElmIdx: In=Index of an element, Out=Index of that element in the block
// Size: Full size of the block. Offs: Offset to first element in the block
//
static inline uint GetRange(size_t ElmIdx, size_t& BlkIdx, size_t& Size, size_t& Offs)   // Used by iterators, should be branchless  
{
 BlkIdx = Strat::CalcForIndex(ElmIdx, ElmIdx);   // NOTE: No ElmIdx check. CalcForIndex may return nonsense!
// if(BlkIdx >= ctx->Blocks.BlkNum)return -1;  // Out of range (The index part itself may be not allocated yet)
 Size = (size_t)Strat::BlkIdxToSize(BlkIdx);    // NOTE: May trim the size on x32, no checks
 Offs = Strat::BlkIdxToHdrLen(BlkIdx);
 return ElmIdx;         
}     
//-------------------------------------------------------------
static inline uint GetBlkSize(size_t BlkIdx) {return (size_t)Strat::BlkIdxToSize(BlkIdx);}  // NOTE: May trim the size on x32, no checks
static inline uint GetBlkOffs(size_t BlkIdx) {return Strat::BlkIdxToHdrLen(BlkIdx);}
static inline uint GetBlkEIdx(size_t& ElmIdx) {size_t BlkIdx = Strat::CalcForIndex(ElmIdx, ElmIdx); return BlkIdx;}
//-------------------------------------------------------------
inline bool   IsBlkInRange(size_t BlkIdx) const {return BlkIdx < this->GetCtx()->Blocks.BlkNum;}  // If the block in the index range   // For iterators
inline vptr   GetBlkPtr(size_t BlkIdx) const {return vptr((size_t)this->GetCtx()->Blocks.GetArrPtr()[BlkIdx] & Strat::PtrMask);}
inline uint32 GetBlkTag(size_t BlkIdx) const {return (size_t)this->GetCtx()->Blocks.GetArrPtr()[BlkIdx] & Strat::TagMask;}
inline void   SetBlkTag(size_t BlkIdx, uint32 Tag)      // It is important to have no checks if it is NULL or not
{
 vptr*  Ptr  = &this->GetCtx()->Blocks.GetArrPtr()[BlkIdx];
 size_t Addr = (size_t)*Ptr & Strat::PtrMask;
 *Ptr = vptr(Addr | (Tag & Strat::TagMask));
}
//-------------------------------------------------------------
inline bool IsElmExist(size_t ElmIdx)     // Checks if a block for the element is allocated
{
 if(!this->GetCtx())return false;
 size_t BlkIdx = Strat::CalcForIndex(ElmIdx, ElmIdx);
 return this->GetBlkPtr(BlkIdx);
}
//-------------------------------------------------------------
inline TCIfo* GetCtxInfo(void)
{
 if constexpr(sizeof(TCIfo))return &this->GetCtx()->CtxInfo;
  else return nullptr;
}
//------------------------------------------------------------- 
static inline TCIfo* GetCtxInfo(vptr BlkBase)
{
 if constexpr(sizeof(TCIfo) && MetaInBlk && CtxInFBlk)
  {
   if((size_t)((SMFHdr*)BlkBase)->Self & 1)return &((SFHdr*)BlkBase)->CtxInfo;
     else return &((SMNHdr*)BlkBase)->Ctx->CtxInfo;
  }
  else return nullptr;
}
//------------------------------------------------------------- 
inline TBIfo* GetBlkInfo(size_t BlkIdx)
{
 if constexpr(sizeof(TBIfo))
  {
   if constexpr(CtxInFBlk)
    {
     if(!BlkIdx)return &((SFHdrEx*)this->GetCtx())->BlkInfo;
    }
   return &((SNHdr*)this->GetBlkPtr(BlkIdx))->BlkInfo;
  }
  else return nullptr;
}
//------------------------------------------------------------- 
static inline TBIfo* GetBlkInfo(vptr BlkBase)
{
 if constexpr(sizeof(TBIfo) && MetaInBlk && CtxInFBlk)
  {
   if((size_t)((SMFHdr*)BlkBase)->Self & 1)return &((SFHdrEx*)BlkBase)->BlkInfo;
     else return &((SMNHdr*)BlkBase)->BlkInfo;
  }
  else return nullptr;
}
//------------------------------------------------------------- 
class SIterator    // I do not like clumsy ranged FOR but i need iterator object to hide iteration between blocks. :: for (auto it = integers.begin(), end = integers.end(); it != end; ++it) {const auto i = *it; ... }
{
 Ty*     CPtr;     // Current Block ptr (Points to first object, after a header if present)
 Ty*     BPtr; 
 Ty*     EPtr;
 SFHdr*  Ctx;
 size_t  BIdx;        // Index of current block;

 //------------------------------------------------------ 
 void NextBlock(void)
 {
  vptr*  APtr = this->Ctx.Blocks.GetArrPtr();
  for(sint idx=(sint)this->BIdx+1,tot=this->Ctx.Blocks.BlkNum; idx < tot; idx++)  // Iterate in case null blocks are present
   {
    size_t blk = (size_t)APtr[idx] & Strat::PtrMask;
    if(blk)
     {
      this->BIdx = idx;
      this->EPtr = (Ty*)(blk + (size_t)Strat::BlkIdxToSize(idx));     // NOTE: May trim the size on x32, no checks
      this->CPtr = this->BPtr = (Ty*)(blk + Strat::BlkIdxToHdrLen(idx));
      return *this; 
     }
   }
  this->BIdx = -1;    // reset to END
  this->EPtr = (Ty*)-1;
  this->CPtr = this->BPtr = nullptr;
 }
 //------------------------------------------------------ 
 void PrevBlock(void)
 {
  vptr* APtr = this->Ctx.Blocks.GetArrPtr();
  for(sint idx=(sint)this->BIdx-1; idx <= 0; idx--)  // Iterate in case null blocks are present
   {
    size_t blk = (size_t)APtr[idx] & Strat::PtrMask;
    if(blk)
     {
      this->BIdx = idx;        
      this->BPtr = (Ty*)(blk + Strat::BlkIdxToHdrLen(idx));
      this->CPtr = this->EPtr = (Ty*)(blk + (size_t)Strat::BlkIdxToSize(idx));     // NOTE: May trim the size on x32, no checks
      this->CPtr--;
      return *this; 
     }
   }
  this->BIdx = -1;    // reset to END
  this->CPtr = this->BPtr = this->EPtr = nullptr;
 }
 //------------------------------------------------------ 
public:
 // Iterator tags here...

 // Iterator constructors here...
 SIterator(SFHdr* ctx)
 {    
  this->BIdx = -1;    // reset to END
  this->EPtr = (Ty*)-1;
  this->CPtr = this->BPtr = nullptr;
  this->Ctx  = ctx;
 }    
      
 SIterator(vptr BlkPtr, size_t BlkIdx, size_t ElmDOffs, SFHdr* ctx)
 {
  size_t BSize = (size_t)Strat::BlkIdxToSize(BlkIdx);     // NOTE: May trim the size on x32, no checks
  size_t HSize = Strat::BlkIdxToHdrLen(BlkIdx);
  this->BIdx = BlkIdx;    // reset to END
  this->BPtr = (Ty*)((size_t)BlkPtr + HSize);
  this->EPtr = (Ty*)((size_t)BlkPtr + BSize);
  this->CPtr = (Ty*)((size_t)BlkPtr + HSize + ElmDOffs);
  this->Ctx  = ctx;
 }
 //------------------------------------------------------ 
 // Prefix 
 inline SIterator& operator++(void) 
 { 
  this->CPtr += (bool)this->CPtr;   // Do nothing if the iterator is NULL
  if(this->CPtr >= this->EPtr)this->NextBlock();      
  return *this; 
 }  
 //------------------------------------------------------   
 inline SIterator& operator--(void) 
 { 
  this->CPtr -= (bool)this->CPtr;   // Do nothing if the iterator is NULL
  if(this->CPtr < this->BPtr)this->PrevBlock();
  return *this; 
 }  
 //------------------------------------------------------ 
 // Postfix 
 inline SIterator operator++(int) { SIterator tmp = *this; ++(*this); return tmp; }
 inline SIterator operator--(int) { SIterator tmp = *this; --(*this); return tmp; }     // Not used by ranged FOR (Clumsy, i don`t like them anyway)

 inline Ty& operator*(void) const { return *this->CPtr; }
 inline Ty* operator->(void) { return this->CPtr; }

 inline operator bool() const { return (bool)this->CPtr; } //operator bool  to check if this iterator is NULL

 friend inline bool operator== (const SIterator& a, const SIterator& b) { return !((a.BIdx ^ b.BIdx)|((size_t)a.CPtr ^ (size_t)b.CPtr)); }  // Do not compare pointers, +1 pointer may be in a next block with a random element index
 friend inline bool operator!= (const SIterator& a, const SIterator& b) { return ((a.BIdx ^ b.BIdx)|((size_t)a.CPtr ^ (size_t)b.CPtr)); }     
};
//------------------------------------------------------------- 
// Iterates in allocated ranges (Not aware if any valid data is present there or not) // Unallocated blocks are skipped
SIterator ElmFrom(size_t ElmIdx)   
{
 SFHdr* ctx = this->GetCtx();
 if(!ctx)return SIterator(ctx); 
 size_t BlkIdx  = Strat::CalcForIndex(ElmIdx, ElmIdx);
 size_t ElmOffs = ElmIdx * Strat::ObjSize;
 for(uint idx=BlkIdx,tot=ctx->Blocks.BlkNum;idx < tot;idx++)
  {
   vptr ptr = this->GetBlkPtr(BlkIdx);
   if(ptr)return SIterator(ptr, BlkIdx, ElmOffs, ctx);
   ElmOffs = 0;   // Any of next blocks will start from 0
  }
 return SIterator(ctx); 
}
//------------------------------------------------------------- 
SIterator ElmLast(void)   // Backward only from here
{
 SFHdr* ctx = this->GetCtx();
 if(!ctx)return SIterator(ctx); 
 for(sint idx=ctx->Blocks.BlkNum-1;idx >= 0;idx--)
  {
   vptr ptr = this->GetBlkPtr(idx);
   if(ptr)return SIterator(ptr, idx, (size_t)Strat::BlkIdxToSize(idx) - Strat::ObjSize, ctx);   
  }
 return SIterator(ctx); 
}
//------------------------------------------------------------- 
inline SIterator ElmFirst(void)   // Forward only from here
{
 return this->ElmFrom(0);
}
//------------------------------------------------------------- 
inline SIterator begin(void)     // NOTE: No way avoid iteration over deleted objects!
{ 
 return this->ElmFirst(); 
}
//------------------------------------------------------------- 
inline SIterator end(void)       // Iterates up until last non NULL block (NULL blocks are skipped)
{ 
 return SIterator(this->GetCtx());     // NULL iterator (There is no possible valid comparable END pointer for blocks)
}
//------------------------------------------------------------- 
};
//------------------------------------------------------------------------------------------------------------
// MinLen: Minimal expected number of (Ty) elements.
// MaxLen: Maximal expected number of (Ty) elements. Depending on flags, allocation of more elements may be not possible or inefficient
// TCIfo: Per context (allocator) user defined info
// TBIfo: Per block user defined info
// HPad:  Padding of headers to keep the data properly aligned
//
template<size_t MinLen, size_t MaxLen, uint32 Flg, uint32 HPad=sizeof(size_t), typename Ty=uint8, typename TCIfo=SEmptyType, typename TBIfo=SEmptyType> class CGenAlloc: public TSW<(Flg & afSequential), SSeqAlloc<MinLen,MaxLen,Flg,HPad,Ty,TCIfo,TBIfo>, SBlkAlloc<MinLen,MaxLen,Flg,HPad,Ty,TCIfo,TBIfo> >::T
{
// SCVR size_t ObjLen = sizeof(Ty);
// STASRT(MaxLen >= MinLen);
// STASRT(ObjLen <= MinLen);
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

CGenAlloc(void){}
~CGenAlloc(){this->Release();}    
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