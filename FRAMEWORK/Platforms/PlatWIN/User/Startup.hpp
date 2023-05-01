
#pragma once

//------------------------------------------------------------------------------------------------------------
private:
//------------------------------------------------------------------------------------------------------------
// Compares sequences of stubs and finds most frequent match
static auto CalcSyscallStubInfo(vptr ModBase, uint32* HashArr, uint8** AddrArr)
{
 struct SRes {uint SCNOffs; uint StubSize; uint64 StubFB;};
 const  uint MaxStubLenA = 16;   // Max dist to syscall number
 const  uint MaxStubLenB = 64;   // Max syscall stub size

 NPE::SDosHdr*  DosHdr = (NPE::SDosHdr*)ModBase;
 NPE::SWinHdr*  WinHdr = (NPE::SWinHdr*)&((uint8*)ModBase)[DosHdr->OffsetHeaderPE];
 NPE::SDataDir* ExportDir = &WinHdr->OptionalHeader.DataDirectories.ExportTable;
 if(!ExportDir->DirectoryRVA || !ExportDir->DirectorySize)return SRes{0,0};		 // No export directory!
 NPE::SExpDir* Export  = (NPE::SExpDir*)&((uint8*)ModBase)[ExportDir->DirectoryRVA];  

 uint8*  PrevEntry = nullptr;
 uint8*  LastEntry = nullptr;
 uint32* NamePointers = (uint32*)&((uint8*)ModBase)[Export->NamePointersRVA];    
 uint32* AddressTable = (uint32*)&((uint8*)ModBase)[Export->AddressTableRVA];    
 uint16* OrdinalTable = (uint16*)&((uint8*)ModBase)[Export->OrdinalTableRVA];  
 uint16  MCount[MaxStubLenA] = {};    // Most of same dsizes is hopefully belongs to syscall stubs
 uint16  SCount[MaxStubLenB] = {};
 uint16  BCount[MaxStubLenB*2] = {};
 uint64  BVals[MaxStubLenB*2] = {};   // Keep first 8 bytes for each of stub sizes and swith to other half of the array on each mismatch
 uint    LastDist = 0;
 sint    BCntOffs = -1;
 for(uint ctr=0;ctr < Export->NamePointersNumber;ctr++)  // Find diff sizes and hash names
  {      
   uint32 nrva  = NamePointers[ctr];  
   if(!nrva)continue;  // No name, ordinal only (Not needed right now) 
   uint32 NameHash = NCTM::Crc32((achar*)&((uint8*)ModBase)[nrva]);       // TODO: Use fast Table/Hardware optimized version
   uint8* CurEntry = &((uint8*)ModBase)[AddressTable[OrdinalTable[ctr]]];
   for(uint idx=0;HashArr[idx];idx++){if(!AddrArr[idx] && (HashArr[idx] == NameHash))AddrArr[idx] = CurEntry;}   // Found an address for API name
   if(LastEntry)
    {
     if(CurEntry == LastEntry)continue;   // Same export
     uint CurDist = CurEntry - LastEntry;
     uint mlen = 0;
     for(uint idx=0;(idx <= MaxStubLenA)&&(LastEntry[idx]==CurEntry[idx]);idx++)mlen++;
     if(mlen && (mlen < MaxStubLenA))MCount[mlen]++;
     if((CurDist == LastDist)&&(CurDist < MaxStubLenB))
      {
       SCount[CurDist]++;
       if(BCntOffs < 0){BCntOffs = 0; BVals[BCntOffs + CurDist] = *(uint64*)CurEntry;}
       if(*CurEntry != *PrevEntry)   // mlen is 0    // Min is 1 byte for first syscall instruction
        {
         sint OldBCntOffs = BCntOffs;
         BCntOffs = BCntOffs ? 0 : MaxStubLenB;
         if(BCount[BCntOffs + CurDist] > BCount[OldBCntOffs + CurDist])  // Preserve in opposite slot if here counter is bigger than there   // Needed?
          {
           BCount[OldBCntOffs + CurDist] = BCount[BCntOffs + CurDist];
           BVals[OldBCntOffs + CurDist]  = BVals[BCntOffs + CurDist];
          }
         BCount[BCntOffs + CurDist] = 1;   // Start over   
         BVals[BCntOffs + CurDist]  = *(uint64*)CurEntry; 
        }    
         else BCount[BCntOffs + CurDist]++; 
       PrevEntry = CurEntry;
      }
     LastDist = CurDist;
    }
     else PrevEntry = CurEntry;
   LastEntry = CurEntry;
  }
 uint16 MaxLen = 0;
 uint BestOffs = 0;
 uint BestSize = 0;
 uint BestBIdx = 0;
 for(uint idx=0;idx < MaxStubLenA;idx++){if(MCount[idx] > MaxLen){MaxLen=MCount[idx];BestOffs=idx;}}  // Find most frequently encountered match size
 MaxLen = 0;
 for(uint idx=0;idx < MaxStubLenB;idx++){if(SCount[idx] > MaxLen){MaxLen=SCount[idx];BestSize=idx;}}  // Find most frequently encountered proc size
 MaxLen = 0;
 for(uint idx=0;idx < (MaxStubLenB*2);idx++){if(BCount[idx] > MaxLen){MaxLen=BCount[idx];BestBIdx=idx;}}  // Find most frequent (likely not hooked) first bytes of syscall stub
 return SRes{BestOffs, BestSize, BVals[BestBIdx]}; 
}
//------------------------------------------------------------------------------------------------------------
// NOTE: There is no gurantee that syscall numbers be in order
static sint FindNotHookedStub(uint8** CurStub, uint64 StubFB, uint SCOffs, uint StubLen, uint MaxDist)
{
 for(uint ctr=1;ctr <= MaxDist;ctr++)    // Scan forward
  {
   uint8* Ptr = *CurStub + (ctr * StubLen);
   bool Hooked = memcmp(Ptr, &StubFB, SCOffs);
   if(!Hooked){*CurStub = Ptr; return -ctr;}
  }
 for(uint ctr=1;ctr <= MaxDist;ctr++)    // Scan backward
  {
   uint8* Ptr = *CurStub - (ctr * StubLen);
   bool Hooked = memcmp(Ptr, &StubFB, SCOffs);
   if(!Hooked){*CurStub = Ptr; return ctr;}
  }
 return 0;
}
//------------------------------------------------------------------------------------------------------------
static sint ExtractSyscallNumbers(uint64 StubFB, uint SCOffs, uint StubLen, uint Total, uint8** AddrArr, uint32* SyscArr) 
{
 uint ResCtr = 0;
 for(uint ctr=0;ctr < Total;ctr++)
  {
   if(!AddrArr[ctr])continue;
   bool Hooked = memcmp(AddrArr[ctr], &StubFB, SCOffs);
   if(Hooked)
    {
     uint8* ptr = AddrArr[ctr];
     sint soffs = FindNotHookedStub(&ptr, StubFB, SCOffs, StubLen, 5);
     if(!soffs)continue;    // Failed to resolve
     SyscArr[ctr] = *(int32*)&ptr[SCOffs] + soffs;
    }
     else SyscArr[ctr] = *(uint32*)&AddrArr[ctr][SCOffs];
   ResCtr++;
  }
 return ResCtr;
}
//------------------------------------------------------------------------------------------------------------
static uint FindSyscallStubsBlock(uint8** SBlkPtrBeg, uint8** SBlkPtrEnd)
{
 uint8* PtrBeg = (uint8*)&SAPI::NtProtectVirtualMemory;
 uint8* PtrEnd = PtrBeg;
 uint8 tmpl[NSYSC::MaxStubSize];
 memcpy(tmpl, PtrBeg, sizeof(tmpl));
 *(uint32*)&tmpl[NSYSC::SYSCALLOFFS] = 0;

 for(;;PtrBeg -= NSYSC::MaxStubSize)
  {
   uint8 curr[NSYSC::MaxStubSize];
   memcpy(curr, &PtrBeg[-NSYSC::MaxStubSize], sizeof(curr));
   *(uint32*)&curr[NSYSC::SYSCALLOFFS] = 0;
   if(memcmp(curr, tmpl, NSYSC::MaxStubSize))break;
  }
 for(;;)
  {
   uint8 curr[NSYSC::MaxStubSize];
   PtrEnd += NSYSC::MaxStubSize;
   memcpy(curr, PtrEnd, sizeof(curr));
   *(uint32*)&curr[NSYSC::SYSCALLOFFS] = 0;
   if(memcmp(curr, tmpl, NSYSC::MaxStubSize))break;
  }
 *SBlkPtrBeg = PtrBeg;
 *SBlkPtrEnd = PtrEnd;
 return PtrEnd - PtrBeg;
}
//------------------------------------------------------------------------------------------------------------
// Variable length arrays:
//    Visual studio build supported only with Clang
// All path convertions on Windows require such memory allocations(UTF8 to WCHAR)
// -fms-extensions
// -mno-stack-arg-probe   // VLA always causes chkstk
// MSVC: /clang:-mno-stack-arg-probe
// LINK: /STACK:0x100000,0x100000 
//
static sint InitSyscalls(void)
{
 //static_assert(sizeof(SysApi) == (NSYSC::MaxStubSize * 26), "Inconsistent stub block size!");
// Calculate syscalls stubs size
 uint8* SBlkPtrBeg;  //= (uint8*)&SAPI::NtProtectVirtualMemory;
 uint8* SBlkPtrEnd;  //= (uint8*)&SAPI::NtUnloadDriver + NSYSC::MaxStubSize;
 uint   StubsLen    = FindSyscallStubsBlock(&SBlkPtrBeg, &SBlkPtrEnd);  //  SBlkPtrEnd - SBlkPtrBeg;          //NSYSC::MaxStubSize;
 uint8* BaseOfNtdll = (uint8*)NTX::GetBaseOfNtdll((wchar*)&fwsinf.SysDrive, 3);   // Take drive as 'C:\'
 uint   TotalRecs   = StubsLen / NSYSC::MaxStubSize; 
 const uint MaxRecs = 64;  // For recs in SAPI // sizeof does not work on SAPI
 if(!BaseOfNtdll)return -1;

 uint32 SyscArr[TotalRecs]; // = {};       // VLA is better than alloca. why extra pointer?
 uint32 HashArr[TotalRecs]; // = {};    
 uint8* ProcArr[TotalRecs]; // = {};
 memset(SyscArr,0,TotalRecs*sizeof(*SyscArr));
 memset(HashArr,0,TotalRecs*sizeof(*HashArr));
 memset(ProcArr,0,TotalRecs*sizeof(*ProcArr));

 for(uint ctr=0;ctr < TotalRecs;ctr++){HashArr[ctr] = *(uint32*)&SBlkPtrBeg[(ctr * NSYSC::MaxStubSize) + NSYSC::SYSCALLOFFS];}   // Gather all hashes (Not all of them may be for NTDLL)
 auto sinf = CalcSyscallStubInfo(BaseOfNtdll, HashArr, ProcArr);
// After all DLLs resolved
 ExtractSyscallNumbers(sinf.StubFB,sinf.SCNOffs, sinf.StubSize, TotalRecs, ProcArr, SyscArr); 
 auto pNtProtectVirtualMemory = SAPI::NtProtectVirtualMemory.GetPtr<uint32 (_scall*)(vptr, vptr*, size_t*, uint, uint*, uint32)>();

 vptr Addr    = SBlkPtrBeg;
 size_t Size  = StubsLen;
 uint OldProt = 0;
 uint resa = pNtProtectVirtualMemory(NT::NtCurrentProcess, &Addr, &Size, NT::PAGE_EXECUTE_READWRITE, &OldProt, SyscArr[0]);      // PAGE_EXECUTE_READWRITE may be forbidden, try PAGE_READWRITE but avoid making caller function nonexecutable

 if(!IsArchX64 && NTX::IsWow64())
  {
   // VERY COMPLICATED
  }
   else
    {
     for(uint ctr=0;ctr < TotalRecs;ctr++)
      {
       uint8* RecPtr = &SBlkPtrBeg[ctr * NSYSC::MaxStubSize];
#if defined(CPU_X86)
       *RecPtr = 0xB8;  // mov rax/eax
       *(uint32*)&RecPtr[1] = SyscArr[ctr];
       *(uint32*)&RecPtr[NSYSC::SYSCALLOFFS] = 0xCCCCCCCC;
#endif      
      }
    }
         
 resa = SAPI::NtProtectVirtualMemory(NT::NtCurrentProcess, &Addr, &Size, OldProt, &OldProt);  // All if SAPI is should be working by now            //pNtProtectVirtualMemory(NT::NtCurrentProcess, &Addr, &Size, OldProt, &OldProt, SyscArr[0]); 
 return StubsLen;
}
//------------------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------------------
enum ESFlags {sfInitialized=0x01, sfDynamicLib=0x02, sfLoadedByLdr=0x04};
struct SSINF
{
 vptr   ModBase;
 size_t ModSize;
 uint32 Flags;
 achar  SysDrive[8];

} static inline fwsinf = {};
//------------------------------------------------------------------------------------------------------------
static _finline size_t GetArgC(void){return 1;}   // On Windows should be always 1? 
static _finline const wchar* GetArgV(void){return NT::NtCurrentTeb()->ProcessEnvironmentBlock->ProcessParameters->CommandLine.Buffer;}
static _finline const wchar* GetEnvP(void){return NT::NtCurrentTeb()->ProcessEnvironmentBlock->ProcessParameters->Environment;} 

static _finline bool IsInitialized(void){return  fwsinf.Flags & sfInitialized;}
//------------------------------------------------------------------------------------------------------------

public:
static _finline PX::fdsc_t GetStdIn(void)  {return (PX::fdsc_t)NT::NtCurrentTeb()->ProcessEnvironmentBlock->ProcessParameters->StandardInput; }  
static _finline PX::fdsc_t GetStdOut(void) {return (PX::fdsc_t)NT::NtCurrentTeb()->ProcessEnvironmentBlock->ProcessParameters->StandardOutput;} 
static _finline PX::fdsc_t GetStdErr(void) {return (PX::fdsc_t)NT::NtCurrentTeb()->ProcessEnvironmentBlock->ProcessParameters->StandardError; } 
//------------------------------------------------------------------------------------------------------------
static _finline bool IsLoadedByLdr(void) {return fwsinf.Flags & sfLoadedByLdr;}  // If loaded by loader then should return to the loader   // OnWindows, Any DLL that loaded by loader
static _finline bool IsDynamicLib(void) {return fwsinf.Flags & sfDynamicLib;} 
//------------------------------------------------------------------------------------------------------------
// Copies a next param to user`s buffer
// Set AOffs to 0 initially and expect it to be -1 when there are no more args
// NOTE: Do not expect the return value to be an argument index!
static uint GetCmdLineArg(sint& AOffs, achar* DstBuf, uint BufLen=-1)  
{
 char SFchB = '\"';
 char SFchE = '\"';
 const wchar* CLBase  = &GetArgV()[AOffs];
 const wchar* CmdLine = CLBase;
 while(*CmdLine && (*CmdLine <= 0x20))CmdLine++;  // Skip any spaces before
 if(!*CmdLine){AOffs=-1; return 0;}   // No more args
 if(*CmdLine == SFchB)CmdLine++;  // Skip opening quote
   else SFchE = 0x20;             // No quotes, scan until a first space
 if(!DstBuf)return SizeOfWStrAsUtf8(CmdLine, -1, SFchE) + 4;       // Calculate buffer size for current argument    // +Space for terminating 0

 uint SrcLen = -1;
 WStrToUtf8(DstBuf, CmdLine, BufLen, SrcLen, SFchE);
 DstBuf[BufLen] = 0;

 CmdLine = &CmdLine[SrcLen]; 
 if(*CmdLine)CmdLine++; // Skip last Quote or Space
 while(*CmdLine && (*CmdLine <= 0x20))CmdLine++; // Skip any spaces after it to point at next argument
 if(*CmdLine)AOffs += CmdLine - CLBase;
  else AOffs = -1;  // No more args
 return BufLen;
}
//---------------------------------------------------------------------------
template<typename T> static sint GetEnvVar(T* Name, T DstBuf, size_t BufCCnt)
{
 return -1;
}
//------------------------------------------------------------------------------------------------------------
// AppleInfo on MacOS
static sint GetSysInfo(uint InfoID, void* DstBuf, size_t BufSize)
{
 //GetAuxVRec(size_t Type)
 return -1;
}
//------------------------------------------------------------------------------------------------------------
static _finline vptr   GetModuleBase(void){return fwsinf.ModBase;}
static _finline size_t GetModuleSize(void){return fwsinf.ModSize;}
//------------------------------------------------------------------------------------------------------------
// Returns full path to current module and its name in UTF8
static size_t _finline GetModulePath(char* DstBuf, size_t BufSize=-1)
{
// Search in loader list first

// Search by memory mapping

 return 0;
} 
//------------------------------------------------------------------------------------------------------------
static sint InitStartupInfo(void* StkFrame=nullptr, void* ArgA=nullptr, void* ArgB=nullptr, void* ArgC=nullptr)
{
 DBGDBG("StkFrame=%p, ArgA=%p, ArgB=%p, ArgC=%p",StkFrame,ArgA,ArgB,ArgC);    
 vptr AddrInTheMod = &InitStartupInfo;
 fwsinf.ModBase = NTX::LdrGetModuleByAddr(AddrInTheMod, &fwsinf.ModSize);
 if(!fwsinf.ModBase)   // Not present in the loader list (Loaded by other means)
  {
   // TODO: Find by VirtuaQuery
  }
   else fwsinf.Flags |= sfLoadedByLdr;
 if((ArgA != NT::NtCurrentPeb())&&(((size_t)ArgA & ~(MEMGRANSIZE-1)) == ((size_t)fwsinf.ModBase & ~(MEMGRANSIZE-1))))fwsinf.Flags |= sfDynamicLib;   // System passes PEB as first argument to EXE`s entry point then it is safe to exit from entry point without calling 'exit'

 fwsinf.Flags |= sfInitialized;
 return 0;
}
//------------------------------------------------------------------------------------------------------------
static void DbgLogStartupInfo(void)
{
 uint  alen = 0;
 sint  aoff = 0;
 achar buf[2048];
 LOGDBG("CArguments: ");
 for(uint idx=0;aoff >= 0;idx++)
  {
   alen = NPTM::GetCmdLineArg(aoff, buf, sizeof(buf));
   LOGDBG("  Arg %u: %s",idx,&buf);     // NOTE: Terminal should support UTF8
  }


}
//------------------------------------------------------------------------------------------------------------
