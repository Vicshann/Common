
//------------------------------------------------------------------------------------------------------------
enum EMemMode {mmNone=0, mmShared=0x08, mmExec=0x04, mmWrite=0x02, mmRead=0x01};

struct SMemRange
{
 size_t RangeBeg;
 size_t RangeEnd;
 size_t FMOffs;
 size_t INode;   // FileID
 uint32 Mode;    // rwxp
 uint32 DevH;
 uint32 DevL;
 uint32 FPathLen;
 achar* FPath;     // ProcfsParseMMapLine sets it pointing inside the Line that has been parsed 

 SMemRange(void){FPathLen=0;}
};

struct SMemMap
{
 size_t    NextAddr;      // 0 if  no more
 size_t    RangesCnt;
 ssize_t   TmpBufOffs;    // Must be set to 0 initially
 size_t    TmpBufLen;
 uint8*    TmpBufPtr;     // For reading from the system
 SMemRange Ranges[0];

 SMemMap(void){NextAddr=RangesCnt=TmpBufOffs=0;}
};
//------------------------------------------------------------------------------------------------------------
// NOTE: Range->FPath and Range->FPathLen must be set to actual buffer address and size or NULL if not needed
//
static sint FindMappedRangeByAddr(sint ProcId, size_t Addr, SMemRange* Range)   
{
 return 0;
}
//------------------------------------------------------------------------------------------------------------
static sint FindMappedRangesByPath(sint ProcId, size_t Addr, const achar* ModPath, SMemMap* MappedRanges, size_t BufSize)
{
 return 0;
}
//------------------------------------------------------------------------------------------------------------
static sint ReadMappedRanges(sint ProcId, size_t AddrFrom, size_t AddrTo, SMemMap* MappedRanges, size_t BufSize)  // Windows: QueryVirtualMemory; Linux: ProcFS; BSD:?; MacOS:?
{
 return 0;
}
//------------------------------------------------------------------------------------------------------------
static bool IsValidMemPtr(vptr ptr, size_t len)
{
 size_t PageLen = GetPageSize();
 len += (size_t)ptr & (PageLen - 1);
 ptr  = (vptr)AlignP2Bkwd((size_t)ptr, PageLen);
 len  = AlignP2Frwd(len, PageLen);
 return !NAPI::msync(ptr, len, PX::MS_ASYNC);   // Returns ENOMEM if the memory (or part of it) was not mapped
}
//------------------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------------------