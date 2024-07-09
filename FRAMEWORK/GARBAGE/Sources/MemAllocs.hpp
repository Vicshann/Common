
#pragma once

//===========================================================================
// Just groups memory allocation/deallocation functions for use in templates
// NOTE: On Linux any allocated but untouched memory is reserved
// All allocations showld be SIMD fiendly here?
//
template<ESMemPage MAlgn=mp4K, ESMemPage SAlgn=mp4K, int MAllocGranMul=1> struct SAllocLL    // Low level memory allocator(High perfomance, High Reserve amount) // MReserGranMul and MAllocGranMul must not be 0 
{              // All of these functions and constants are mandatory
 static const SIZE_T SAlignment = SAlgn;      // Size alignment        // Pow2
 static const SIZE_T MAlignment = MAlgn;      // Address alignment     // Pow2  // If a platform`s alignment is smaller than specified alignment then some memory will be wasted on improvized alignment
 static const SIZE_T MAllocGran = MAllocGranMul * MEMPAGESIZE;  // Not have to be Pow2
// static const SIZE_T MaxBlkSize = AlignFrwrdPow2(MaxBLen, SAlignment);

//-----------------------------------------       // TODO: TEST!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
static inline PVOID FCALLCNV Reallocate(PVOID Mem, SIZE_T Size, SIZE_T* Allocated=nullptr)      // Pointer stays valid, but reallocation for a bigger saze may fail
{
 SIZE_T ASize = AlignFrwrdPow2(AlignFrwrd(Size, MAllocGran), SAlignment);   // Alignment applied to Base and Size
 if(ASize > RSize)RSize  = ASize;  // Can happen when MEMPAGESIZE is same as MEMGRANSIZE
 if(Allocated)*Allocated = ASize;  // Sets result to know on which size we failed    // Incorrect overwrite!!!!!!!!!!!
 if((RSize > MaxBlkSize)||(ASize > MaxBlkSize))return nullptr;
 PVOID  Ptr   = NPTM::AllocMemLL(Mem, *Allocated, ASize, RSize, MAlignment|MEMNORELOCATE);      // Commits a reserved pages and tries to commit more(Can fail!)
 return Ptr;
}
//-----------------------------------------
/*static inline SIZE_T FCALLCNV Resize(PVOID Mem, SIZE_T Size, SIZE_T Allocated)    // Expands allocated memory in a reserved range only
{
 if(Size > Allocated)Size = Allocated;
 SIZE_T ASize = AlignFrwrdPow2(AlignFrwrd(Size, MAllocGran), SAlignment);    // Alignment applied to Base and Size
 if(ASize)NPTM::AllocMemLL(Mem, 0, ASize, 0, MAlignment|MEMNORELOCATE);      // ASize allowed to be 0
 Allocated -= ASize;
 if(Allocated)Release(&((PUINT8)Mem)[ASize], Allocated);  // Decommit rest of the block and keep memory pages reserved
 return Size;
} */
//-----------------------------------------
static inline void FCALLCNV Release(PVOID Mem, SIZE_T Size)   // Release memory pages if possible
{
 NPTM::FreeMemLL(Mem,Size);    // Releases an entire page range but keeps the range reserved (accessible, no need to commit these pages again)
}
//-----------------------------------------
static inline void FCALLCNV Free(PVOID Mem)
{
 NPTM::FreeMemLL(Mem,0);
}
//-----------------------------------------
};
//===========================================================================
template<ESMemPage MAlgn=mpNO, ESMemPage SAlgn=mpNO, int MAllocGranMul=0> struct SAllocHL   // High level memory allocator(Low perfomance, Low Reserve amount) // MReserGranMul and MAllocGranMul have same meaning here
{
 static const SIZE_T SAlignment = SAlgn;
 static const SIZE_T MAlignment = MAlgn;
 static const SIZE_T MAllocGran = MAllocGranMul * 256;  
// static const SIZE_T MaxBlkSize = AlignFrwrdPow2(MaxBLen, SAlignment);

//-----------------------------------------   // TODO: TEST!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
static inline PVOID FCALLCNV Reallocate(PVOID Mem, SIZE_T Size, SIZE_T* Allocated=nullptr)     // Pointer may become invalid!  // When reallocating, Allocated contains old allocated size
{
 SIZE_T ASize = (MAllocGran)?(AlignFrwrd(Size, MAllocGran)):(Size);       // No Base and Size alignment used
 if(Allocated)*Allocated = ASize;             // Incorrect!!!!!!!!!!!!!!!!!!!!                      
 if(ASize > MaxBlkSize)return nullptr;
 PVOID  Ptr   = NPTM::AllocMemHL(Mem, *Allocated, ASize, 0, MAlignment);           // TODO: Use 'Expand' function if possible
 return Ptr;
}
//-----------------------------------------
static inline void FCALLCNV Release(PVOID Mem, SIZE_T Size)      // Accepts ONLY addr returned from AllocMemHL
{
 Free(Mem);
}
//-----------------------------------------
static inline void FCALLCNV Free(PVOID Mem)
{
 NPTM::FreeMemHL(Mem,0);
}
//-----------------------------------------
};
//===========================================================================

