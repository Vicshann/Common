
//============================================================================================================
//                                      Block access strategies
//------------------------------------------------------------------------------------------------------------
template<size_t PageLen, size_t ObjLen, size_t FHdrLen, size_t NHdrLen, size_t RMin, size_t RMax, typename Der> struct SBASBase
{

}; 
//============================================================================================================
// Size = Size   // Uniform allocation
//
template<size_t ObjLen, size_t PageLen=NPTM::MEMPAGESIZE, size_t FHdrLen=0, size_t NHdrLen=0> struct SBASUni  
{

};
//============================================================================================================
// Size = Size + BaseSize    // Linear allocation growth
//
template<size_t ObjLen, size_t PageLen=NPTM::MEMPAGESIZE, size_t FHdrLen=0, size_t NHdrLen=0> struct SBASLin  
{

}; 
//============================================================================================================
// Size = Size + Size  // Grow geometrically (geometric progression with base 2)
//
template<size_t ObjLen, size_t PageLen=NPTM::MEMPAGESIZE, size_t FHdrLen=0, size_t NHdrLen=0> struct SBASExp: SBASBase<AlignP2Up(((PageLen - Max(FHdrLen, NHdrLen)) < ObjLen)?(ObjLen+Max(FHdrLen, NHdrLen)):(PageLen)), ObjLen, FHdrLen, NHdrLen, 32, 32, SBASExp<ObjLen,PageLen,FHdrLen,NHdrLen> >  
{
 SCVR size_t TagMask   = ((size_t)NPTM::MEMPAGESIZE - 1);   // Do not use PageSize to keep it consistent with other strategies
 SCVR size_t PtrMask   = ~TagMask; 
 SCVR size_t RangeMin  = 32;  // Should be enough for x32 and x64 even if base page size is 4096 (16TB max) (RangeNum times : PageSize = (PageSize * 2))
 SCVR size_t RangeMax  = 32;
 SCVR size_t FHdrSize  = FHdrLen;   // To make them available from outside
 SCVR size_t NHdrSize  = NHdrLen;
 SCVR size_t ObjSize   = ObjLen;
 SCVR size_t PageSize  = AlignP2Up(((PageLen - Max(FHdrLen, NHdrLen)) < ObjSize)?(ObjSize+Max(FHdrLen, NHdrLen)):(PageLen));    // Base/First page size 

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
//                                           The allocator
//------------------------------------------------------------------------------------------------------------
// MaxLen is not used. Allocation may fail depending on available memory and address space.
//
template<size_t MinLen, size_t MaxLen, uint32 Flg, uint32 HPad=sizeof(size_t), typename Ty=uint32, typename TCIfo=SEmptyType, typename TBIfo=SEmptyType> class SBlkAlloc   // Multiple linear ranges
{
using TBStrat = SBASExp<0>;    // To get access to constants
SCVR bool SepBlkIdx = TBStrat::RangeMax != TBStrat::RangeMin;    // Block index array is a separate allocation (For strategies that require large indexes for blocks)
SCVR bool MetaInBlk = (Flg & afBlkTrcOwner);    // Metadata is added to every block (To find the block owner)  // Forces context to be in the first block
SCVR bool CtxInFBlk = MetaInBlk || (Flg & afSinglePtr);          // The context is in the first block (Forced by MetaInBlk)

struct SBlkArrI    // Only for SBASExp
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
 vptr*  BlkArr;  // Separate allocation to be able to grow. If afSmallMem is specified then allocated only when the first block is full 
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

// TODO: FHDR is only differs for first block if context in first block!!!!!!!!!!!!!!!!!!!!!!! 
using Strat = SBASExp<sizeof(Ty), AlignP2Up(AlignP2Frwd(MinLen, NPTM::MEMPAGESIZE)), AlignP2Up(AlignP2Frwd(sizeof(SFHdr), AlignP2Up(HPad))), AlignP2Up(AlignP2Frwd(sizeof(SNHdr), AlignP2Up(HPad)))>;

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
//============================================================================================================
//                                           TESTS
//------------------------------------------------------------------------------------------------------------
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
//------------------------------------------------------------------------------------------------------------