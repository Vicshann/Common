
//============================================================================================================
//
template<size_t MinLen, uint32 Flg, typename Ty=uint8, typename TCIfo=SEmptyType, typename TBIfo=SEmptyType, typename MP=SMemPrvBase, size_t MaxAlign=64> class SSeqAlloc   // Arbitrary memory allocator. Single block. Single linear range.   // Alignment 64 is enough even for AVX512
{
SCVR bool   EmptyMP   = sizeof(MP) <= 1;    // MP will have 1 size without any data members    // The MP size should be either 0 or > 1
SCVR bool   CtxInFBlk = Flg & afSinglePtr;  // The context is in the block               
SCVR uint32 StratType = Flg & afGrowMask;

struct SUHdrs: TCIfo, TBIfo {};  // For grouping only

struct SPtrH {vptr Blk;};

struct SFHdr: TSW<(CtxInFBlk), SEmptyType, SPtrH >::T 
{
 size_t Size;    // Current size of the block
 MP*    MProv;
 SUHdrs InfHdr;


//------------------------------------------------------------- 
inline size_t GetDataLen(void) const {return this->Size - AlFHdrLen;}    // In bytes
inline size_t GetDataMax(void) const {return (this->Size - AlFHdrLen) / AlUnitLen;}  // In units
inline Ty*    GetDataPtr(void)     
{
 size_t ptr = (size_t)this->GetBlockPtr();
 if constexpr (CtxInFBlk)ptr += AlFHdrLen;
 return (Ty*)ptr;
}
inline vptr  GetBlockPtr(void) 
{
 if constexpr (CtxInFBlk)return (vptr)this;
 return this->Blk;
}
//------------------------------------------------------------- 
};

// Is this the most appropriate alignment strategy?  
SCVR size_t AlUnitLen  = (Flg & afObjAlign)?((Flg & afAlignPow2)?(AlignToP2Up(sizeof(Ty))):(AlignP2Frwd(sizeof(Ty), sizeof(size_t)))):(sizeof(Ty));   // May align to nearest Pow@ or by pointer size, depending what is the best for the current strategy
SCVR size_t AlFHdrLen  = CtxInFBlk ? ((!(Flg & afLimitHdrAl) || (AlUnitLen <= MaxAlign))?(AlignFrwd(sizeof(SFHdr),AlUnitLen)):(AlignP2Frwd(sizeof(SFHdr),MaxAlign))) : 0;
SCVR size_t AlPageSize = AlignP2Frwd(Max(AlUnitLen+AlFHdrLen, MinLen), NPTM::MEMPAGESIZE);  

TSW<(CtxInFBlk), SFHdr*, SFHdr >::T Context;
//-------------------------------------------------------------
static bool IsCtxMP(SFHdr* ptr) {if constexpr (CtxInFBlk)return (size_t)ptr & 1; return false;}   // NOTE: High unused bits may be actually used by the system for some tagging
//-------------------------------------------------------------
inline MP* GetMP(void)
{
 if constexpr (!EmptyMP)
  {
   if constexpr (CtxInFBlk)
    {
     if((size_t)this->Context & 1)return (MP*)((size_t)this->Context - 1);
      else return this->Context->MProv;
    }
     else return ((SFHdr*)&this->Context)->MProv;
  }
   else return (MP*)nullptr;   // no data members
}
//-------------------------------------------------------------
_finline SFHdr* GetCtx(void)  // _ninline
{
 if constexpr (CtxInFBlk)return this->Context;     // May be NULL if not initialized yet (GrowBlkIdxArrFor will do that)      // Return nullptr if bit 1 is set (ptr to MP)?
 return (SFHdr*)&this->Context;     // NOTE: Clang will not detect correctly all exit points in constexpr if-else (when inlined) and will go insane
}
//-------------------------------------------------------------
_finline vptr GetBlockPtr(void)
{
 if constexpr (CtxInFBlk)return this->Context;
 return ((SFHdr*)&this->Context)->Blk;
}
//-------------------------------------------------------------
void SetBlockPtr(vptr ptr, size_t len)
{
 if constexpr (CtxInFBlk)
  {
   this->Context = ptr;
   this->Context->Size = len;
  }
   else
    {
     ((SFHdr*)&this->Context)->Blk  = ptr;
     ((SFHdr*)&this->Context)->Size = len;
    }
}
//-------------------------------------------------------------
static size_t CalcBlkSizeForOffset(size_t Offset) 
{
 if constexpr (StratType == afGrowLin)
  {
   size_t cbidx = (((size_t)NMATH::sqrt((8 * (Offset / AlPageSize)) + 1) - 1) / 2) + 1;    // Optimize?
   return (AlPageSize * ((cbidx * (cbidx + 1)) / 2));
  }
 else if constexpr (StratType == afGrowExp)return AlignToP2Up(Offset);
 return AlignFrwd(Offset, AlPageSize);
}
//-------------------------------------------------------------
static size_t CalcBlkSizeForElmIdx(size_t ElmIdx)
{
 size_t offs = ElmIdx * AlUnitLen;
 if constexpr (CtxInFBlk)offs += AlFHdrLen;
 return CalcBlkSizeForOffset(offs); 
}
//-------------------------------------------------------------
public:
//------------------------------------------------------------- 
SSeqAlloc(void) = delete;

inline SSeqAlloc(MP* mp=(MP*)nullptr)
{
 NMOPS::ZeroObj(this);
 if constexpr (!EmptyMP)
   if constexpr (CtxInFBlk)this->Context = (SFHdr*)((size_t)mp | 1);     // In most cases MP is zero    // NOTE: This will make the GetCtx to return an invalid context pointer until first call to AllocBlock
     else this->Context.MProv = mp;
}
inline ~SSeqAlloc(){this->Release();}
//------------------------------------------------------------- 
int Release(void)
{
 SFHdr* ctx = this->GetCtx();
 if constexpr (CtxInFBlk) {if(!ctx)return -0x10000000;}
 this->GetMP()->Free(this->GetBlockPtr(), ctx->Size);
 this->SetBlockPtr(nullptr, 0);
 return 0;
}
//------------------------------------------------------------- 
int Expand(size_t ElmIdx)    // Expand for, not "expand by"
{
 SFHdr* ctx = this->GetCtx();
 if constexpr (CtxInFBlk) {if(!ctx)return -0x10000000;}
 size_t NewLen = CalcBlkSizeForElmIdx(ElmIdx);
 if(NewLen <= ctx->Size)return 0;  // Nothing to do
 vptr OldPtr = this->GetBlockPtr();
 if constexpr (CtxInFBlk)
   if((size_t)OldPtr & 1)OldPtr = nullptr;    // MP, actual block ptr is NULL 
 MP*  mp = this->GetMP(); 
 vptr NewPtr = mp->ReAlloc(OldPtr, ctx->Size, NewLen, !(Flg & afNoMove));   
 if(!NewPtr)return -0x30000000;
 this->SetBlockPtr(NewPtr, NewLen);
 if constexpr (CtxInFBlk)this->GetCtx()->MProv = mp;      // Assign to actual MP pointer in case this is a first allocation 
 return 0;
}
//-------------------------------------------------------------
int Shrink(size_t ElmIdx) 
{
 SFHdr* ctx = this->GetCtx();
 if constexpr (CtxInFBlk) {if(!ctx)return -1;}
 size_t NewLen = CalcBlkSizeForElmIdx(ElmIdx);
 if(NewLen == ctx->Size)return 0;  // Nothing to do
 if(NewLen >= ctx->Size)return -0x20000000;  // Will not grow
 vptr NewPtr = this->GetMP()->ReAlloc(this->GetBlockPtr(), ctx->Size, NewLen, !(Flg & afNoMove));
 if(!NewPtr)return -0x30000000;
 this->SetBlockPtr(NewPtr, NewLen);
 return 0;
}
//-------------------------------------------------------------
inline bool IsElmExist(size_t ElmIdx)
{
 SFHdr* ctx = this->GetCtx();
 if constexpr (CtxInFBlk) {if(!ctx)return false;}
 size_t ExpLen = CalcBlkSizeForElmIdx(ElmIdx);
 return (ExpLen <= ctx->Size);
}
//------------------------------------------------------------- 
inline SUHdrs* GetCtxInfo(void)
{
 if constexpr(sizeof(SUHdrs))return &this->GetCtx()->InfHdr;
  else return nullptr;
}
//------------------------------------------------------------- 
inline Ty* GetDataPtr(void)       // Valid only after a first call to Expand!
{
 return this->GetCtx()->GetDataPtr();
}
//------------------------------------------------------------- 
size_t GetDataLen(void)       // In bytes
{
 SFHdr* ctx = this->GetCtx();
 if constexpr (CtxInFBlk) {if(!ctx)return 0;}
 return ctx->GetDataLen();
}
//------------------------------------------------------------- 
inline size_t GetElmMax(void)   // In Units
{
 SFHdr* ctx = this->GetCtx();
 if constexpr (CtxInFBlk) {if(!ctx)return 0;}
 return ctx->GetDataMax(); 
}
//------------------------------------------------------------- 

using CtxPtr = TSW<CtxInFBlk, SFHdr**, SFHdr*>;   // Pointer to pointer in case of relocation

class SIterator    // The iterator will try to be valid between relocations :: for (auto it = alc.begin(), end = alc.end(); it != end; ++it) {const auto i = *it; ... }
{
 size_t  COffs;                    
 size_t  BOffs;    // From block base (Includes AlFHdrLen)
 size_t  EOffs;    // Beyond the data
 CtxPtr  Ctx;      // Allocator`s context
 //------------------------------------------------------ 
 _finline SFHdr* GetCtx(void)
 {
  if constexpr (CtxInFBlk)return *this->Ctx;    // NOTE: the owner of this pointer may move! 
  return this->Ctx;
 }
 //------------------------------------------------------ 

public:

 inline SIterator(const SIterator& itr)   // Copy constructor
 {
  NMOPS::CopyObj(this, itr);
 }
 inline SIterator(SIterator&& itr)  // Move constructor
 {
  NMOPS::CopyObj(this, itr);
  NMOPS::ZeroObj(itr);
 }
 //------------------------------------------------------
 inline SIterator(CtxPtr ctx)       // NULL iterator constructor  
 {    
  this->COffs = this->BOffs = this->EOffs = (size_t)-1;
  this->Ctx   = ctx;
 }    
 //------------------------------------------------------     
 inline SIterator(size_t ElmIdx, CtxPtr ctx)   
 {
  this->Ctx      = ctx;
  SFHdr* octx    = this->GetCtx();
  size_t elmoffs = ElmIdx * AlUnitLen;
  size_t BlkLen  = octx->GetDataMax() * sizeof(AlUnitLen);
  if(elmoffs >= BlkLen){this->COffs = this->BOffs = this->EOffs = (size_t)-1; return;}  // Invalid range
  if constexpr (CtxInFBlk)this->BOffs = AlFHdrLen;
    else this->BOffs = 0; 
  this->EOffs = this->BOffs + BlkLen;
  this->COffs = this->BOffs + elmoffs;
 }
 //------------------------------------------------------ 
 inline void operator= (const SIterator& itr)
 {
  NMOPS::CopyObj(this, itr);
 }
 //------------------------------------------------------ 
 inline void operator= (size_t ElmIdx)    // Set the position in objects      // Fast if in boundaries of the current block
 {
  size_t offs = this->BOffs + (ElmIdx * AlUnitLen);
  if(offs < this->EOffs)this->COffs = offs;
    else this->COffs = (size_t)-1;  // Set to null
 }
 //------------------------------------------------------ 
 inline operator size_t(void)             // Get the position in objects    
 {
  return (this->COffs - this->BOffs) / AlUnitLen;
 }
 //------------------------------------------------------ 
 // Prefix 
 inline SIterator& operator++(void) 
 { 
  this->COffs += AlUnitLen * (this->COffs != (size_t)-1);   // Do nothing if the iterator is NULL
  if(this->COffs >= this->EOffs)this->COffs = (size_t)-1;      
  return *this; 
 }  
 //------------------------------------------------------   
 inline SIterator& operator--(void) 
 { 
  this->COffs -= AlUnitLen * (this->COffs != (size_t)-1);   // Do nothing if the iterator is NULL
  if(this->COffs < this->BOffs)this->COffs = (size_t)-1;
  return *this; 
 }  
 //------------------------------------------------------ 
 // Postfix 
 inline SIterator operator++(int) { SIterator tmp = *this; ++(*this); return tmp; }
 inline SIterator operator--(int) { SIterator tmp = *this; --(*this); return tmp; }     // Not used by ranged FOR (Clumsy, i don`t like them anyway)

 inline Ty& operator*(void) const { return *(Ty*)((size_t)(this->GetCtx()->GetBlockPtr()) + this->COffs); }    
 inline Ty* operator->(void) { return (Ty*)((size_t)(this->GetCtx()->GetBlockPtr()) + this->COffs); }          

 inline operator bool() const { return (bool)(this->COffs + 1); }    // Will be 0 if was (size_t)-1  // to check if this iterator is NULL   

 friend inline bool operator== (const SIterator& a, const SIterator& b) { return !((size_t)a.COffs ^ (size_t)b.COffs); }  
 friend inline bool operator!= (const SIterator& a, const SIterator& b) { return ((size_t)a.COffs ^ (size_t)b.COffs); }   
};
//-------------------------------------------------------------
inline SIterator ElmFrom(size_t ElmIdx)
{
 return SIterator(ElmIdx, &this->Context);   
}
//------------------------------------------------------------- 
inline SIterator ElmLast(void) 
{
 size_t LastIdx = this->GetDataMax();
 if(!LastIdx)return SIterator(&this->Context);
 return SIterator(LastIdx-1, &this->Context);  
}
//------------------------------------------------------------- 
inline SIterator ElmFirst(void) 
{
 return this->ElmFrom(0);
}
//------------------------------------------------------------- 
inline SIterator begin(void)     
{ 
 return this->ElmFirst(); 
}
//------------------------------------------------------------- 
inline SIterator end(void)        
{ 
 return SIterator(&this->Context);     // NULL iterator
} 
//------------------------------------------------------------- 
};