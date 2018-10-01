
#pragma once

//===========================================================================
template<ESMemPage MAlgn=mp4K, ESMemPage SAlgn=mp4K, SIZE_T MaxBLen=MAXMEMBLK, int MAllocGranMul=1, int MReserGranMul=1> struct SAllocLL    // Low level memory allocator(High perfomance, High Reserve amount) // MReserGranMul and MAllocGranMul must not be 0 
{              // All of these functions and constants are mandatory
 static const SIZE_T SAlignment = SAlgn;           // Pow2
 static const SIZE_T MAlignment = MAlgn;           // Pow2  // If a platform`s alignment is smaller than specified alignment then some memory will be wasted on improvized alignment
 static const SIZE_T MAllocGran = MAllocGranMul * MEMPAGESIZE;  // Not have to be Pow2
 static const SIZE_T MReserGran = MReserGranMul * MEMGRANSIZE;  // Not have to be Pow2
 static const SIZE_T MaxBlkSize = AlignFrwrdPow2(MaxBLen, SAlignment);

static inline PVOID FCALLCNV Allocate(SIZE_T Size, SIZE_T* Allocated, SIZE_T* Reserved)
{
 return Realloc(nullptr, Size, 0, Allocated, Reserved);
}
//-----------------------------------------
static inline PVOID FCALLCNV Realloc(PVOID Mem, SIZE_T Size, SIZE_T OldSize, SIZE_T* Allocated, SIZE_T* Reserved)      // Pointer stays valid, but reallocation for a bigger saze may fail
{
 SIZE_T ASize = AlignFrwrdPow2(AlignFrwrd(Size, MAllocGran), SAlignment);   // Alignment applied to Base and Size
 SIZE_T RSize = AlignFrwrdPow2(AlignFrwrd(Size, MReserGran), SAlignment);
 if(ASize > RSize)RSize  = ASize;  // Can happen when MEMPAGESIZE is same as MEMGRANSIZE
 if(Allocated)*Allocated = ASize;  // Sets result to know on which size we failed
 if(Reserved)*Reserved   = RSize;
 if((RSize > MaxBlkSize)||(ASize > MaxBlkSize))return nullptr;
 PVOID  Ptr   = NPTM::AllocMemLL(Mem, OldSize, ASize, RSize, MAlignment|MEMNORELOCATE);      // Commits a reserved pages and tries to commit more(Can fail!)
 return Ptr;
}
//-----------------------------------------
static inline SIZE_T FCALLCNV Resize(PVOID Mem, SIZE_T Size, SIZE_T Reserved)    // Expands allocated memory in a reserved range only
{
 if(Size > Reserved)Size = Reserved;
 SIZE_T ASize = AlignFrwrdPow2(AlignFrwrd(Size, MAllocGran), SAlignment);    // Alignment applied to Base and Size
 if(ASize)NPTM::AllocMemLL(Mem, 0, ASize, 0, MAlignment|MEMNORELOCATE);     // ASize allowed to be 0
 Reserved -= ASize;
 if(Reserved)Release(&((PUINT8)Mem)[ASize], Reserved);  // Decommit rest of the block and keep memory pages reserved
 return Size;
}
//-----------------------------------------
static inline void FCALLCNV Release(PVOID Mem, SIZE_T Size)
{
 NPTM::FreeMemLL(Mem,Size);    // Releases an entire pages
}
//-----------------------------------------
static inline void FCALLCNV Free(PVOID Mem)
{
 NPTM::FreeMemLL(Mem,0);
}
//-----------------------------------------
};
//===========================================================================
template<ESMemPage MAlgn=mpNO, ESMemPage SAlgn=mpNO, SIZE_T MaxBLen=MAXMEMBLK, int MAllocGranMul=0, int MReserGranMul=0> struct SAllocHL   // High level memory allocator(Low perfomance, Low Reserve amount) // MReserGranMul and MAllocGranMul have same meaning here
{
 static const SIZE_T SAlignment = SAlgn;
 static const SIZE_T MAlignment = MAlgn;
 static const SIZE_T MAllocGran = MAllocGranMul * 256; 
 static const SIZE_T MReserGran = MReserGranMul * 256;  
 static const SIZE_T MaxBlkSize = AlignFrwrdPow2(MaxBLen, SAlignment);

static inline PVOID FCALLCNV Allocate(UINT Size, SIZE_T* Allocated, SIZE_T* Reserved)
{
 return Realloc(nullptr, Size, 0, Allocated, Reserved);
}
//-----------------------------------------
static inline PVOID FCALLCNV Realloc(PVOID Mem, SIZE_T Size, SIZE_T OldSize, SIZE_T* Allocated, SIZE_T* Reserved)     // Pointer may become invalid!
{
 SIZE_T ASize = (MAllocGran)?(AlignFrwrd(Size, MAllocGran)):(Size);       // No Base and Size alignment used
 if(Allocated)*Allocated = ASize;                                   
 if(Reserved)*Reserved   = ASize;                                  // Not 'Reserved' but 'Allocated'
 if(ASize > MaxBlkSize)return nullptr;
 PVOID  Ptr   = NPTM::AllocMemHL(Mem, OldSize, ASize, 0, MAlignment);           // TODO: Use 'Expand' function if possible
 return Ptr;
}
//-----------------------------------------
static inline SIZE_T FCALLCNV Resize(PVOID Mem, SIZE_T Size, SIZE_T Reserved)    // Resizes an allocated memory within a reserved range  // Preserves memory block pointer // Can`t release unused memory pages!
{
 if(Size > Reserved)Size = Reserved;
 return Size;
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

