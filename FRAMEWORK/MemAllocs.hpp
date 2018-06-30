
#ifndef MemAllocatorsH
#define MemAllocatorsH

#pragma once

/*
Hard faults are what you are thinking of - these are where data is not in RAM and has to pulled in from the swap file. 
They cripple performance (being 10 000's of times slower than RAM access for mechanical hard drives).

Soft faults, however, are triggered by pages that the program requests to be zero (demand zero pages), 
when a page is written to for the first time (if it was a copy on write page) or if the page is already in memory 
somewhere else (usually when shared between multiple processes). These are not so bad for performance.
*/
//===========================================================================
template<ESMemPage Algn=mp4K, int MAllocGranMul=1, int MReserGranMul=1> struct SAllocLL    // Low level memory allocator(High perfomance, High Reserve amount) // MReserGranMul and MAllocGranMul must not be 0 
{
 static const unsigned int MAlignment = Algn;
 static const unsigned int MAllocGran = MAllocGranMul * MEMPAGESIZE;
 static const unsigned int MReserGran = MReserGranMul * MEMGRANSIZE;

static PVOID Allocate(SIZE_T Size, SIZE_T* Reserved)
{
 return ReAlloc(nullptr, Size, Reserved);
}
//-----------------------------------------
static PVOID ReAlloc(PVOID Mem, SIZE_T Size, SIZE_T* Reserved)      // Pointer may become invalid!
{
 SIZE_T ASize = AlignFrwrd(Size, MAllocGran);
 SIZE_T RSize = AlignFrwrd(Size, MReserGran);
 PVOID  Ptr   = Platform::AllocMemLL(ASize, Mem, RSize, Algn);      // Commits a reserved pages and tries to commit more(Can fail!)
 if(Reserved)*Reserved = RSize;
 return Ptr;
}
//-----------------------------------------
static SIZE_T Resize(PVOID Mem, SIZE_T Size, SIZE_T Reserved)    // Expands an allocated memory in a reserved range
{
 if(Size > Reserved)Size = Reserved;
 SIZE_T ASize = AlignFrwrd(Size, MAllocGran);
 if(ASize)Platform::AllocMemLL(ASize, Mem, 0, Algn);    // ASize allowed to be 0
 Reserved -= ASize;
 if(Reserved)Release(&((PUINT8)Mem)[ASize], Reserved);  // Decommit rest of the block and keep memory pages reserved
 return Size;
}
//-----------------------------------------
static void Release(PVOID Mem, SIZE_T Size)
{
 Platform::FreeMemLL(Mem,Size);    // Releases an entire pages
}
//-----------------------------------------
static void Free(PVOID Mem)
{
 Platform::FreeMemLL(Mem,0);
}
//-----------------------------------------
};
//===========================================================================
template<ESMemPage Algn=mp4K, int MAllocGranMul=1, int MReserGranMul=1> struct SAllocHL   // High level memory allocator(Low perfomance, Low Reserve amount) // MReserGranMul and MAllocGranMul have same meaning here
{
 static const unsigned int MAlignment = Algn;
 static const unsigned int MAllocGran = MAllocGranMul * 256; 
 static const unsigned int MReserGran = MReserGranMul * 256;  

static PVOID Allocate(UINT Size, SIZE_T* Reserved)
{
 return ReAlloc(nullptr, Size, Reserved);
}
//-----------------------------------------
static PVOID ReAlloc(PVOID Mem, SIZE_T Size, SIZE_T* Reserved)     // Pointer may become invalid!
{
 SIZE_T ASize = (MAllocGran)?(AlignFrwrd(Size, MAllocGran)):(Size);
 PVOID  Ptr   = Platform::AllocMemHL(ASize, Mem, 0, Algn);           // TODO: Use 'Expand' function if possible
 if(Reserved)*Reserved = ASize;                                    // Not 'Reserved' but 'Allocated'
 return Ptr;
}
//-----------------------------------------
static SIZE_T Resize(PVOID Mem, SIZE_T Size, SIZE_T Reserved)    // Resizes an allocated memory within a reserved range  // Preserves memory block pointer // Can`t release unused memory pages!
{
 if(Size > Reserved)Size = Reserved;
 return Size;
}
//-----------------------------------------
static void Release(PVOID Mem, SIZE_T Size)      // Accepts ONLY addr returned from AllocMemHL
{
 Free(Mem);
}
//-----------------------------------------
static void Free(PVOID Mem)
{
 Platform::FreeMemHL(Mem,0);
}
//-----------------------------------------
};
//===========================================================================
#endif
