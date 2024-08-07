
#pragma once

//------------------------------------------------------------------------------------------------------------
private:
//------------------------------------------------------------------------------------------------------------
// Compares sequences of stubs and finds most frequent match
[[clang::optnone]] _ninline static auto CalcSyscallStubInfo(vptr ModBase, uint32* HashArr, uint8** AddrArr) __attribute__ ((optnone))    // NOTE: optnone is ignored!!!  // NOTE: This function is randomly messed up even with -O1
{
 struct SRes {uint SCNOffs; uint StubSize; uint64 StubFB;};
 const  uint MaxStubLenA = 16;   // Max dist to syscall number
 const  sint MaxStubLenB = 64;   // Max syscall stub size

 NPE::SDosHdr*  DosHdr = UnbindPtr((NPE::SDosHdr*)ModBase);
 NPE::SWinHdr*  WinHdr = UnbindPtr((NPE::SWinHdr*)&((uint8*)ModBase)[DosHdr->OffsetHeaderPE]);
 NPE::SDataDir* ExportDir = UnbindPtr(&WinHdr->OptionalHeader.DataDirectories.ExportTable);
 if(!ExportDir->DirectoryRVA || !ExportDir->DirectorySize)return SRes{0,0,0};		 // No export directory!
 NPE::SExpDir* Export  = UnbindPtr((NPE::SExpDir*)&((uint8*)ModBase)[ExportDir->DirectoryRVA]);

 uint8*  PrevEntry = nullptr;
 uint8*  LastEntry = nullptr;
 uint32* NamePointers = UnbindPtr((uint32*)&((uint8*)ModBase)[Export->NamePointersRVA]);
 uint32* AddressTable = UnbindPtr((uint32*)&((uint8*)ModBase)[Export->AddressTableRVA]);
 uint16* OrdinalTable = UnbindPtr((uint16*)&((uint8*)ModBase)[Export->OrdinalTableRVA]);
 uint16  MCount[MaxStubLenA] = {};    // Most of same dsizes is hopefully belongs to syscall stubs
 uint16  SCount[MaxStubLenB] = {};
 uint16  BCount[MaxStubLenB*2] = {};
 uint64  BVals[MaxStubLenB*2] = {};   // Keep first 8 bytes for each of stub sizes and swith to other half of the array on each mismatch

 sint LastDist = 0;      
 sint BCntOffs = -1;     
 for(volatile uint ctr=0, tot=Export->NamePointersNumber;ctr < tot;ctr++)  // Find diff sizes and hash names  // Without 'volatile' the 'ctr' is comepletely broken with -O2 !!!
  {
   uint32 nrva = NamePointers[ctr];
   if(!nrva)continue;  // No name, ordinal only (Not needed right now)
   uint32 NameHash = NCRYPT::CRC32((achar*)&((uint8*)ModBase)[nrva]);      // TODO: Use fast Table/Hardware optimized version
   uint8* CurEntry = &((uint8*)ModBase)[AddressTable[OrdinalTable[ctr]]];
   for(uint idx=0;HashArr[idx];idx++){if(!AddrArr[idx] && (HashArr[idx] == NameHash))AddrArr[idx] = CurEntry;}   // Found an address for API name
   if(LastEntry)
    {
     if(CurEntry == LastEntry)continue;   // Same export
     sint CurDist = CurEntry - LastEntry;
     uint mlen = 0;
     for(uint idx=0;(idx <= MaxStubLenA)&&(LastEntry[idx]==CurEntry[idx]);idx++)mlen++;
     if(mlen && (mlen < MaxStubLenA))
        MCount[mlen]++;
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
 for(uint idx=0;idx < MaxStubLenA;idx++){if(MCount[idx] > MaxLen){MaxLen=MCount[idx];BestOffs=idx;}}  // Find most frequently encountered match size (From 1?)
 MaxLen = 0;
 for(uint idx=0;idx < MaxStubLenB;idx++){if(SCount[idx] > MaxLen){MaxLen=SCount[idx];BestSize=idx;}}  // Find most frequently encountered proc size
 MaxLen = 0;
 for(uint idx=1;idx < (MaxStubLenB*2);idx++){if(BCount[idx] > MaxLen){MaxLen=BCount[idx];BestBIdx=idx;}}  // Find most frequent (likely not hooked) first bytes of syscall stub
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
   if(!Hooked){*CurStub = Ptr; return -(sint)ctr;}
  }
 for(uint ctr=1;ctr <= MaxDist;ctr++)    // Scan backward
  {
   uint8* Ptr = *CurStub - (ctr * StubLen);
   bool Hooked = memcmp(Ptr, &StubFB, SCOffs);
   if(!Hooked){*CurStub = Ptr; return (sint)ctr;}
  }
 return 0;
}
//------------------------------------------------------------------------------------------------------------
static sint ExtractSyscallNumbers(uint64 StubFB, uint SCOffs, uint StubLen, uint Total, uint8** AddrArr, uint32* SyscArr)
{
 sint ResCtr = 0;
 for(uint ctr=0;ctr < Total;ctr++)
  {
   if(!AddrArr[ctr])continue;
   bool Hooked = memcmp(AddrArr[ctr], &StubFB, SCOffs);
   if(Hooked)
    {
     uint8* ptr = AddrArr[ctr];
     sint soffs = FindNotHookedStub(&ptr, StubFB, SCOffs, StubLen, 5);
     if(!soffs)continue;    // Failed to resolve
     SyscArr[ctr] = uint32(*(int32*)&ptr[SCOffs] + soffs);
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
 return uint(PtrEnd - PtrBeg);
}
//------------------------------------------------------------------------------------------------------------
// We cannot inline those stubs with a constant numbers
// It would be possible to inline a templated stubs which take syscall numbers from a table (
// Is having a table of syscalls in the module body a bad idea?
// Anyway, stubs are much easier to debug an they can be easily patched in a binary if arguments compatibility will be broken in future versions of OS
// None of this is required on Linux because syscall numbers are already known
//
// Variable length arrays:
//    Visual studio build supported only with Clang
// All path convertions on Windows require such memory allocations(UTF8 to WCHAR)
// -fms-extensions
// -mno-stack-arg-probe   // VLA always causes chkstk
// MSVC: /clang:-mno-stack-arg-probe    // -mstack-probe-size=100000
// LINK: /STACK:0x100000,0x100000
//
static sint InitSyscalls(void)
{
 //static_assert(sizeof(SysApi) == (NSYSC::MaxStubSize * 26), "Inconsistent stub block size!");
// Calculate syscalls stubs size
 uint8* SBlkPtrBeg;  //= (uint8*)&SAPI::NtProtectVirtualMemory;
 uint8* SBlkPtrEnd;  //= (uint8*)&SAPI::NtUnloadDriver + NSYSC::MaxStubSize;
 uint   StubsLen    = FindSyscallStubsBlock(&SBlkPtrBeg, &SBlkPtrEnd);  //  SBlkPtrEnd - SBlkPtrBeg;          //NSYSC::MaxStubSize;
 uint8* BaseOfNtdll = (uint8*)NTX::GetBaseOfNtdll((wchar*)&fwsinf.SysDrive, 3);   // Take drive as 'C:\'   // SharedUserData->NtSystemRoot ???
 uint   TotalRecs   = StubsLen / NSYSC::MaxStubSize;
// const uint MaxRecs = 64;  // For recs in SAPI // sizeof does not work on SAPI
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
 auto pNtProtectVirtualMemory = SAPI::NtProtectVirtualMemory.GetPtr<uint32 (_scall*)(size_t, vptr*, size_t*, uint32, uint32*, uint32)>();

 vptr Addr    = SBlkPtrBeg;
 size_t Size  = StubsLen;
 uint32 OldProt = 0;   // Always 32 bit?
 uint resa = pNtProtectVirtualMemory(NT::NtCurrentProcess, &Addr, &Size, NT::PAGE_EXECUTE_READWRITE, &OldProt, SyscArr[0]);   // NtProtectVirtualMemory is expected to be first in SysApi list     // PAGE_EXECUTE_READWRITE may be forbidden, try PAGE_READWRITE but avoid making caller function nonexecutable

 if(!IsArchX64 && NTX::IsWow64())   // TODO: Windows ARM
  {
   // TODO: VERY COMPLICATED      
  }
   else
    {
     for(uint ctr=0;ctr < TotalRecs;ctr++)
      {
       uint8* RecPtr = &SBlkPtrBeg[ctr * NSYSC::MaxStubSize];
#if defined(CPU_X86)    // X86-X64
       *RecPtr = 0xB8;  // mov rax/eax        // Restore our stub from its static state ( call by a passed syscall number )
       *(uint32*)&RecPtr[1] = SyscArr[ctr];
       *(uint32*)&RecPtr[NSYSC::SYSCALLOFFS] = 0xCCCCCCCC;  // Wipe out Hash of an API name
#endif
      }
    }

 resa = SAPI::NtProtectVirtualMemory(NT::NtCurrentProcess, &Addr, &Size, OldProt, &OldProt);  // SAPI is should be working by now            //pNtProtectVirtualMemory(NT::NtCurrentProcess, &Addr, &Size, OldProt, &OldProt, SyscArr[0]);
 return (sint)StubsLen;
}
//------------------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------------------
enum ESFlags {sfInitialized=0x01, sfDynamicLib=0x02, sfLoadedByLdr=0x04};
struct SSINF
{
 vptr   ModBase;
 size_t ModSize;
 vptr   TheModBase;
 size_t TheModSize;
 vptr   MainModBase;
 size_t MainModSize;
 achar  SysDrive[8];
 sint32 MemPageSize;
 sint32 MemGranSize;
 sint32 UTCOffs; // In seconds
 uint32 Flags;
 NTHD::STDesc thd;

 PX::fdsc_t DevNull;
 PX::fdsc_t DevRand;

} static inline fwsinf = {};
//------------------------------------------------------------------------------------------------------------
static _finline size_t GetArgC(void){return 1;}   // On Windows should be always 1?
static _finline const wchar* GetArgV(void){return NT::NtCurrentTeb()->ProcessEnvironmentBlock->ProcessParameters->CommandLine.Buffer;}     // Single string, space separated, args in quotes
static _finline const wchar* GetEnvP(void){return NT::NtCurrentTeb()->ProcessEnvironmentBlock->ProcessParameters->Environment;}            // Block of null-terminated strings, last is 0
static _finline bool IsInitialized(void){return  fwsinf.Flags & sfInitialized;}
static _finline void UpdateTZOffsUTC(void){fwsinf.UTCOffs = sint32(-NTX::GetTimeZoneBias() / NDT::SECS_TO_FT_MULT);}   // Number of 100-ns intervals in a second
static _finline NTHD::STDesc* GetThDesc(void){return &fwsinf.thd;}
//------------------------------------------------------------------------------------------------------------

public:

static _finline uint32 GetPageSize(void)  {return fwsinf.MemPageSize;}
static _finline uint32 GetGranSize(void)  {return fwsinf.MemGranSize;}
//------------------------------------------------------------------------------------------------------------
static _finline PX::fdsc_t GetStdIn(void)  {return (PX::fdsc_t)NT::NtCurrentTeb()->ProcessEnvironmentBlock->ProcessParameters->StandardInput; }
static _finline PX::fdsc_t GetStdOut(void) {return (PX::fdsc_t)NT::NtCurrentTeb()->ProcessEnvironmentBlock->ProcessParameters->StandardOutput;}
static _finline PX::fdsc_t GetStdErr(void) {return (PX::fdsc_t)NT::NtCurrentTeb()->ProcessEnvironmentBlock->ProcessParameters->StandardError; }

static _finline PX::fdsc_t GetStdNull(void) {return fwsinf.DevNull;}
static _finline PX::fdsc_t GetStdRand(void) {return fwsinf.DevRand;}
//------------------------------------------------------------------------------------------------------------
static _finline sint32 GetTZOffsUTC(void){return fwsinf.UTCOffs;}   // In seconds
static _finline bool   IsLoadedByLdr(void) {return fwsinf.Flags & sfLoadedByLdr;}  // If loaded by loader then should return to the loader   // OnWindows, Any DLL that loaded by loader
static _finline bool   IsDynamicLib(void) {return fwsinf.Flags & sfDynamicLib;}
//------------------------------------------------------------------------------------------------------------
static _finline vptr   GetModuleBase(void){return fwsinf.ModBase;}
static _finline size_t GetModuleSize(void){return fwsinf.ModSize;}
//------------------------------------------------------------------------------------------------------------
// Returns full path to current module and its name in UTF8
static sint _finline GetModulePath(achar* DstBuf, size_t BufSize=size_t(-1))
{
 sint aoffs = 0;
 return (size_t)GetCLArg(aoffs, DstBuf, BufSize);       // TODO TODO TODO !!!

// Search in loader list first

// Search by memory mapping

 return 0;
}
//------------------------------------------------------------------------------------------------------------
static sint InitStartupInfo(vptr StkFrame=nullptr, vptr ArgA=nullptr, vptr ArgB=nullptr, vptr ArgC=nullptr)
{
 DBGDBG("StkFrame=%p, ArgA=%p, ArgB=%p, ArgC=%p",StkFrame,ArgA,ArgB,ArgC);
 vptr AddrInTheMod = (vptr)&InitStartupInfo;
 fwsinf.ModBase = NTX::LdrGetModuleByAddr(AddrInTheMod, &fwsinf.ModSize);
 if(!fwsinf.ModBase)   // Not present in the loader list (Loaded by other means)
  {
   // TODO: Find by VirtuaQuery
  }
   else fwsinf.Flags |= sfLoadedByLdr;      // Normal processes`s entry point is called by some loader stub  // TODO: Support native process creation then there will be no such stub
 if((ArgA != NT::NtCurrentPeb())&&(((size_t)ArgA & ~(MEMGRANSIZE-1)) == ((size_t)fwsinf.ModBase & ~(MEMGRANSIZE-1))))fwsinf.Flags |= sfDynamicLib;   // System passes PEB as first argument to EXE`s entry point then it is safe to exit from entry point without calling 'exit'

 UpdateTZOffsUTC();
 fwsinf.MemPageSize = MEMPAGESIZE;
 fwsinf.MemGranSize = MEMGRANSIZE;
 fwsinf.Flags  |= sfInitialized;
 return 0;
}
//------------------------------------------------------------------------------------------------------------
// https://learn.microsoft.com/en-us/windows/win32/api/errhandlingapi/nf-errhandlingapi-seterrormode
// Windows 7:
// System default is to display all error dialog boxes.
// SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX|SEM_NOOPENFILEERRORBOX
// GetErrorMode: NtQueryInformationProcess, 4
// SetErrorMode: NtSetInformationProcess, 4
//
static sint SetErrorHandlers(void)
{

 return 0;
}
//------------------------------------------------------------------------------------------------------------
static void DbgLogStartupInfo(void)
{
 uint  alen = 0;
 sint  aoff = 0;
 LOGDBG("CArguments: ");
 for(uint idx=0;aoff >= 0;idx++)
  {
   const syschar* val = NPTM::SkipCLArg(aoff, &alen);
   LOGDBG("  Arg %u: %.*ls",idx,alen,val);     // NOTE: Terminal should support UTF8
  }
 LOGDBG("EVariables: ");
 aoff = 0;
 for(uint idx=0;aoff >= 0;idx++)
  {
   const syschar* val = NPTM::GetEnvVar(aoff, &alen);
   LOGDBG("  Var %u: %ls",idx,val);     // NOTE: Terminal should support UTF8
  }
 DBGDBG("Done!");
}
//------------------------------------------------------------------------------------------------------------
