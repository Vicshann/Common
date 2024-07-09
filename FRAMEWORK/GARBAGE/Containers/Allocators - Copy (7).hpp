
/*
// Both 64-bit Linux and 64-bit Windows support 48-bit addresses, which translates to 256TB of virtual address space
// Android will use memory tagging, so no pointer compression worth the implementation effort anyway.
 BlockChain:   // PRO: No space for list of blocks is needed.  CON: Too slow to walk the entire chain if many blocks are allocated. Metadata at beginning of each block. 
 BlockLinear:  // PRO: Fast random access, fast 'Next/Prev'. Use only an index of a block in the list.  CON: Block list must be stored somewhere. All blocks must be of same size.

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
 afObjAlign    = 0x0040,   // Waste some memory by expanding (Pow2/Mul2) size of objects for better access speed
 afAlignPow2   = 0x0080,   // Align obj/hdr size to nearest Pow2 size
 afLimitHdrAl  = 0x0100,   // Use limited header alignment (64 bytes) instead of same as the object size (aligned)
// afObjDeAlloc  = 0x0080,   // Will have a list of free objects and then will try to allocate in them first. Can be used only with afObjAlloc
// afObjLowFrag  = 0x0100,   // Try to keep fragmentation low by sorting removed objects in free list (Slower delete)
// afThreadSafe  = 0x0100,   // TODO! Thread safe operations only (No moving of internal structures like BlkList or BitMap)
 afSinglePtr   = 0x0200,   // Use only single pointer to reference the memory (Store any associated info elsewhere).  // Sometimes an array is a member of some struct that is allocated in vast numbers but not all of them will have some data
 afSmallMem    = 0x0400,   // Do not preallocate early and keep preallocation as small as possible (More conditions while allocating/accessing and more syscalls to grow the allocated block)
 afBlkTrcOwner = 0x0800,   // Store some metadata at beginning of each block (Like the allocator pointer, and some user pointer)
 afSparseIndex = 0x1000,   // Allocate blocks if their pointers are NULL else report errors on access by unallocateed index.
 //afSAddrAlign  = 0x2000,   // Align allocation base address by allocation size (Pow2)

// Prealloc/Grow modes ?
 afGrowMask    = 0x000F,
 afGrowUni     = 0x0000,   // NextSize = BaseSize               // Uniform allocation growth (All blocks of same size (BaseSize))
 afGrowLin     = 0x0001,   // NextSize = PrevSize + BaseSize    // Linear allocation growth
 afGrowExp     = 0x0002,   // NextSize = PrevSize + PrevSize    // Exponential allocation growth (Size = Size * 2)

};
//------------------------------------------------------------------------------------------------------------
alignas(size_t) struct SMemProvBase     // Base memory provider  // Min size is 4096 and min alignment is 4096  // TODO: Interface verification
{
 vptr Alloc(size_t len)  // For block allocations,   // May return a handle instead of actual memory
 {
  vptr BPtr = (vptr)NPTM::NAPI::mmap(nullptr, len, PX::PROT_READ|PX::PROT_WRITE, PX::MAP_PRIVATE|PX::MAP_ANONYMOUS, -1, 0);   // Executable?  // Some platforms may refuse allocating of RWX memory
  if(NPTM::GetMMapErrFromPtr(BPtr))return nullptr;
  return BPtr;
 }
 bool Free(vptr ptr, size_t len){NPTM::NAPI::munmap(ptr, len) >= 0;}      // Size is optional
 vptr Lock(vptr ptr, size_t len, size_t offs=0){return ptr;}       // Size is optional  // Returns actual memory pointer   // NOTE: Do not expect to store any contexts or headers in memory that requires this
 bool UnLock(vptr ptr, size_t len, size_t offs=0){return true;}    // Size is optional
 vptr ReAlloc(vptr ptr, size_t olen, size_t nlen){return ptr;}     // May return a handle   // TODO: Implement mremap syscall
};
//------------------------------------------------------------------------------------------------------------
#include "AllocatorSeq.hpp"
#include "AllocatorBlk.hpp"

//------------------------------------------------------------------------------------------------------------
// MinLen: Minimal expected number of (Ty) elements.
// MaxLen: Maximal expected number of (Ty) elements. Depending on flags, allocation of more elements may be not possible or inefficient
// TCIfo: Per context (allocator) user defined info
// TBIfo: Per block user defined info
// HPad:  Padding of headers to keep the data properly aligned
//
template<size_t MinLen, size_t MaxLen, uint32 Flg, typename Ty=uint8, typename TCIfo=SEmptyType, typename TBIfo=SEmptyType, typename MP=SMemProvBase> class CGenAlloc: public TSW<(Flg & afSequential), SSeqAlloc<MinLen,MaxLen,Flg,Ty,TCIfo,TBIfo,MP>, SBlkAlloc<MinLen,Flg,Ty,TCIfo,TBIfo,MP> >::T
{
// SCVR size_t ObjLen = sizeof(Ty);
// STASRT(MaxLen >= MinLen);
// STASRT(ObjLen <= MinLen);
//------------------------------------------------------------------------------------------------------------
public:

//------------------------------------------------------------------------------------------------------------
inline CGenAlloc(MP* mp=(MP*)nullptr){}     // TODO: Pass mp to base class
inline ~CGenAlloc(){}   //this->Release();}    
//------------------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------------------

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