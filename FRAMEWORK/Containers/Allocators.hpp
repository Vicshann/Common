
//============================================================================================================
struct NALC
{
enum EAllocFlags {
 afNone        = 0,
 afNoMove      = 0x0010,   // For afSequential. Allocate only from an initially reserved single block of memory(BlkLen). No moving is attempted (delete, extend) and pointers are always preserved. An attempt to grow may fail. // May be not enough memory to relocate (On Windows, especially x32)
 afSequential  = 0x0020,   // The data in memory must always be sequential to be accessed by a raw pointer. Will use reallocation and copy, cannot guarantee pointer preservation. No free slot map, just a used/allocated sizes
 afObjAlign    = 0x0040,   // Waste some memory by expanding (Pow2/Mul2) size of objects for better access perfomance
 afAlignPow2   = 0x0080,   // Align obj/hdr size to nearest Pow2 size
 afLimitHdrAl  = 0x0100,   // Use limited header alignment (64 bytes) instead of same as the object size (aligned)
 afSinglePtr   = 0x0200,   // Use only single pointer to reference the memory (Store any associated info elsewhere).  // Sometimes an array is a member of some struct that is allocated in vast numbers but not all of them will have some data
 afSmallMem    = 0x0400,   // Do not preallocate early and keep preallocation as small as possible (More conditions while allocating/accessing and more syscalls to grow the allocated block)
 afBlkTrcOwner = 0x0800,   // Store some metadata at beginning of each block (Like the allocator pointer, and some user pointer)
 afSparseIndex = 0x1000,   // Allocate blocks if their pointers are NULL else report errors on access by unallocateed index.

 afGrowMask    = 0x000F,
 afGrowUni     = 0x0000,   // NextSize = BaseSize               // Uniform allocation growth (All blocks of same size (BaseSize))
 afGrowLin     = 0x0001,   // NextSize = PrevSize + BaseSize    // Linear allocation growth
 afGrowExp     = 0x0002,   // NextSize = PrevSize + PrevSize    // Exponential allocation growth (Size = Size * 2)
};
//============================================================================================================
alignas(size_t) struct SMemPrvBase     // Base memory provider  // Min size is 4096 and min alignment is 4096  // TODO: Interface verification
{
 vptr Alloc(size_t len)  // For block allocations,   // May return a handle instead of actual memory
 {
  vptr BPtr = (vptr)NPTM::NAPI::mmap(nullptr, len, PX::PROT_READ|PX::PROT_WRITE, PX::MAP_PRIVATE|PX::MAP_ANONYMOUS, -1, 0);   // Executable?  // Some platforms may refuse allocating of RWX memory
  if(NPTM::GetMMapErrFromPtr(BPtr))return nullptr;
  return BPtr;
 }
 bool Free(vptr ptr, size_t len){NPTM::NAPI::munmap(ptr, len) >= 0;}    // Size is optional
 vptr Lock(vptr ptr, size_t len, size_t offs=0){return ptr;}            // Size is optional  // Returns actual memory pointer   // NOTE: Do not expect to store any contexts or headers in memory that requires this
 bool UnLock(vptr ptr, size_t len, size_t offs=0){return true;}         // Size is optional
 vptr ReAlloc(vptr optr, size_t olen, size_t nlen, bool maymove=true)   // May return a handle   // TODO: Implement mremap syscall  // NOTE: may fail if MayMove is false and ptr is not a handle
 {
  if(!optr)return this->Alloc(nlen);   // Should just allocate if ptr is NULL
  vptr BPtr = (vptr)NPTM::NAPI::mremap(optr, olen, nlen, !maymove?PX::MREMAP_FIXED:0, nullptr);
  if(NPTM::GetMMapErrFromPtr(BPtr))return nullptr;
  return nullptr; 
 } 
};
//============================================================================================================

#include "AllocatorSeq.hpp"
#include "AllocatorBlk.hpp"

//------------------------------------------------------------------------------------------------------------
// MinLen: In bytes. Will be rounded up to the system`s PageSize at least
// TCIfo: Per context (allocator) user defined info
// TBIfo: Per block user defined info
//
template<size_t MinLen, uint32 Flg, typename Ty=uint8, typename TCIfo=SEmptyType, typename TBIfo=SEmptyType, typename MP=SMemPrvBase> class CGenAlloc: protected TSW<(Flg & afSequential), SSeqAlloc<MinLen,Flg,Ty,TCIfo,TBIfo,MP>, SBlkAlloc<MinLen,Flg,Ty,TCIfo,TBIfo,MP> >::T
{
 using Base = TSW<(Flg & afSequential), SSeqAlloc<MinLen,Flg,Ty,TCIfo,TBIfo,MP>, SBlkAlloc<MinLen,Flg,Ty,TCIfo,TBIfo,MP> >::T;

//------------------------------------------------------------------------------------------------------------
public:

//------------------------------------------------------------------------------------------------------------
inline CGenAlloc(MP* mp=(MP*)nullptr): Base(mp) {}    
inline ~CGenAlloc(){}      
//------------------------------------------------------------------------------------------------------------
using Base::end;
using Base::begin;

using Base::ElmFrom;
using Base::ElmLast;
using Base::ElmFirst;
     
using Base::Expand;
using Base::Shrink;
using Base::Release;

using Base::GetCtxInfo;  // NOTE: Use 'auto' to assign
using Base::IsElmExist;
using Base::GetElmMax;
//-------------------------------------------------------------------
};

//============================================================================================================
//                                           TESTS
//------------------------------------------------------------------------------------------------------------
#ifdef DBGBUILD
template<typename S> static bool TestStrategyBlk(uint32 From=0)
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
   if(!BrtIdx && (str.BlkIdxToObjIdx(BrtBlk) != idx))   // Every first unit must match a calculated one
    {
     return false;
    }
  }
 return true; 
}
#endif
//------------------------------------------------------------------------------------------------------------

};
//------------------------------------------------------------------------------------------------------------
/*
{
   NALC::CGenAlloc<4096,4096,NALC::afObjAlloc|NALC::afSinglePtr> alc;
     bool t1 = alc.IsElmExist(56);
   volatile vptr xx = alc.GetBlock(5);
     bool t2 = alc.IsElmExist(67);
      xx = alc.GetBlock(8);
        alc.DeleteBlk(5);
        xx = alc.GetBlock(43);
    vptr yy = alc.FindBlock(nullptr);
    auto dd = alc.GetBlkData(8, 4);
    alc.SetBlkTag(8, 9); 
    uint rr = alc.GetBlkTag(8);
    uint TotalElm1 = 0;
    uint TotalElm2 = 0;
    uint TotalElm3 = 0;
    for (auto it = alc.begin(), end = alc.end(); it != end; ++it) 
     {
      *it = 6;
      TotalElm1++;
     }
    for (auto it = alc.begin(), end = alc.end(); it ; ++it) 
     {
      *it = 6;
      TotalElm2++;
     }
    for (auto& el : alc) 
     {
      el = 6;
      TotalElm3++;
     }
   //int v = AlignToP2Dn(5);
   //v++;
   return 11;
  }

*/