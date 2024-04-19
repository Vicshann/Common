
#pragma once

//============================================================================================================
template<typename PHT=PTRCURRENT> struct NUFELF: public NFMTELF<PHT>    // NUFmtELF
{
//------------------------------------------------------------------------------------------------------------
// AddrOnStack is any address on stack because __builtin_frame_address(0) is unreliable
// NOTE: ArgC may be 0
// NOTE: This is not 100% reliable but should help with finding the startup info even if the process started by an loader
// TEST: Test it with 0 ArgC
//
// Detect:
//   ArgC
//   ArgPtrs...
//   Null
//   EVarPtrs...
//   Null
//
//
//
static vptr FindStartupInfo(vptr AddrOnStack)
{
 constexpr const size_t MaxArgC  = 0x1000;   // Max is 4k of arguments
 constexpr const size_t StkAlgn  = 0x1000;   // Max is 4k forward   // NOTE: Bottom of the stack is unknown
 size_t* CurPtr = (size_t*)AddrOnStack;
 size_t* EndPtr = (size_t*)AlignP2Frwd((size_t)AddrOnStack, StkAlgn);
 enum EState {stZero,stArgC,stArgs,stEnvs,stAuxV,stDone};
 EState state = stZero;
 if(((uint8*)EndPtr - (uint8*)CurPtr) < 0x100)EndPtr += StkAlgn;  // Too small, another page must exist
 size_t* TblEndPtr = nullptr;
 size_t* InfoPtr = nullptr;
 size_t ArgC  = 0;    // Counts everything
 size_t ZeroC = 0;
// DBGDBG("Scanning range: %p - %p",CurPtr,EndPtr);

 for(;;)
  {
   if(CurPtr >= EndPtr)
    {
     if(state != stArgC)    // Stop only on ArgC search
      {
       if(ArgC < 8)    // Min reliable counter
        {
         state  = stArgC;
         CurPtr = InfoPtr;
        }
         else {EndPtr += StkAlgn; continue;} // Another page is expected   // DBGDBG("PageAdded: EndPtr=%p",EndPtr);
      }
       else break;
    }
   else if(state == stZero)     // There may be only zeroes pushed on stack, they should not be assumed as zero args and evars
    {
     size_t val = *CurPtr;
     if(val)
      {
//       DBGDBG("Nonzero at: %p - %p",CurPtr, (vptr)val);
       if((val > MaxArgC) && (ZeroC >= 2))CurPtr -= 2;    // In case of actually zero ArgC     // Empty ArgV is unlikely
       state = stArgC;
       continue;      // Avoid CurPtr++
      }
       else ZeroC++;
    }
   else if(state == stArgC)
    {
     ArgC = *CurPtr;
     if(ArgC < MaxArgC)
      {
       state = stArgs;
       InfoPtr = CurPtr;
       TblEndPtr = &CurPtr[ArgC+1];    // Expected to contain NULL but may happen to be outside of our EndPtr or completely wrong if the ArgC is a mistake
//       DBGDBG("ArgC at: %p",CurPtr);
      }
    }
   else if(state == stArgs)
    {
     if(ArgC)
      {
//       DBGDBG("ArgRec at: %p",CurPtr);
       if((uint8*)*CurPtr <= (uint8*)TblEndPtr){state = stArgC; CurPtr = InfoPtr;}    // The string address is wrong - reset    // Pointers should point forward (The strings are closer to the stack`s top)
        else ArgC--;
      }
     else if(!*CurPtr)state = stEnvs;
           else {state = stArgC; CurPtr = InfoPtr;}     // Unexpected non-null - reset
    }
   else if(state == stEnvs)
    {
//     DBGDBG("EnvRec at: %p",CurPtr);
     if(!*CurPtr)state = stAuxV;
     else if((uint8*)*CurPtr <= (uint8*)TblEndPtr){state = stArgC; CurPtr = InfoPtr;}  // The string address is wrong - reset    // TblEndPtr points not at the end of ENVs but should be OK
          else ArgC++;
    }
   else if(state == stAuxV)   // Only ways out of this state is success or reaching EndPtr
    {
//     DBGDBG("AuxRec at: %p",CurPtr);
     ELF::SAuxVecRec* Rec = (ELF::SAuxVecRec*)CurPtr;
     if(Rec->type == ELF::AT_NULL){state = stDone; break;}
      else {CurPtr++; ArgC++;}
    }
   CurPtr++;
  }
// DBGDBG("Finished with: %u",state);
 if(state != stDone)return nullptr;   // Not found
 return InfoPtr;
}
//------------------------------------------------------------------------------------------------------------
static vptr FindStartupInfoByAuxV(vptr AddrOnStack)
{
 constexpr const size_t MaxArgC  = 0x1000;   // Max is 4k of arguments
 constexpr const size_t StkAlgn  = 0x1000;   // Max is 4k forward   // NOTE: Bottom of the stack is unknown
 size_t* CurPtr = (size_t*)AddrOnStack;
 size_t* EndPtr = (size_t*)AlignP2Frwd((size_t)AddrOnStack, StkAlgn);
 size_t* AuxEndNull = nullptr;
// DBGDBG("Starting from: %p",CurPtr);
 for(uint ARep=0;!AuxEndNull && (ARep < 2);ARep++)  // Alignment correction
  {
 for(sint MatchCtr=0,Idx=ARep,pidx=-((bool)!((size_t)CurPtr & (StkAlgn-1)));(Idx < 256)&&(pidx < 2);Idx+=2)
  {
   size_t num = CurPtr[Idx];
   size_t val = CurPtr[Idx+1];
 //  DBGDBG("Aux %p: %p %p, %u",&CurPtr[Idx],(vptr)num,(vptr)val, MatchCtr);
   if(!((size_t)&CurPtr[Idx] & (StkAlgn-1)))pidx++;
    else if(!((size_t)&CurPtr[Idx+1] & (StkAlgn-1)))pidx++;
   if(!num)
    {
     if(MatchCtr > 6){AuxEndNull=&CurPtr[Idx]; break;}  // Probably end of AuxV
     MatchCtr = 0;
     continue;
    }
// Most popular AUXV pointer values
   if((num == ELF::AT_PHDR)&&(val >= StkAlgn))MatchCtr++;
   else if((num == ELF::AT_BASE)&&(val >= StkAlgn))MatchCtr++;
   else if((num == ELF::AT_ENTRY)&&(val >= StkAlgn))MatchCtr++;
   else if((num == ELF::AT_EXECFN)&&(val >= StkAlgn))MatchCtr++;
   else if((num == ELF::AT_RANDOM)&&(val >= StkAlgn))MatchCtr++;
   else if((num == ELF::AT_SYSINFO_EHDR)&&(val >= StkAlgn))MatchCtr++;
// Most popular AUXV  integer values
   else if((num == ELF::AT_PAGESZ)&&(val <= StkAlgn))MatchCtr++;
   else if((num == ELF::AT_PHNUM)&&(val <= StkAlgn))MatchCtr++;
   else if((num == ELF::AT_PHENT)&&(val <= StkAlgn))MatchCtr++;
   else if((num == ELF::AT_UID)&&(val <= StkAlgn))MatchCtr++;
   else if((num == ELF::AT_GID)&&(val <= StkAlgn))MatchCtr++;
   else if((num == ELF::AT_EUID)&&(val <= StkAlgn))MatchCtr++;
   else if((num == ELF::AT_EGID)&&(val <= StkAlgn))MatchCtr++;
  }
  }
// DBGDBG("AuxVec end: %p",AuxEndNull);
 if(!AuxEndNull)return nullptr;
 size_t* VPtr = AuxEndNull;
 bool NullTbl = false;
 for(;;) // Count AuxV
  {
   VPtr -= 2;
   if(!VPtr[0] && !VPtr[1]){NullTbl=true;break;}  // End of the AuxV table (Prev tables is null)  // NOTE: No way to know what tables are null there (Most likely EnvP)
   if(!VPtr[1] && (VPtr[0] >= StkAlgn) && (VPtr[0] >= (size_t)AuxEndNull))break;   // The ID is actually a valid pointer in a table above
  }
// DBGDBG("Ptr AuxV: %p ",VPtr);
 if(!NullTbl)
  {
 for(;;)  // Count EnvP
  {
   VPtr--;
   if(!*VPtr)break;  // End of the EnvP table
   if(*VPtr < StkAlgn)return nullptr; // Not a pointer
   if((size_t*)*VPtr <= AuxEndNull)return nullptr;   // The data is not after the table
  }
  }
// DBGDBG("Ptr EnvP: %p ",VPtr);
 for(;;)  // Count ArgV
  {
   VPtr--;
   if(!*VPtr)break;  // End of the ArgV table (No records)
   if(*VPtr < StkAlgn)break; // Probably ArgC is reached
   if((size_t*)*VPtr <= AuxEndNull)return nullptr;   // The data is not after the table
  }
// DBGDBG("Ptr Final: %p ",VPtr);
 return VPtr;
}
//------------------------------------------------------------------------------------------------------------
// Why Linux loads VDSO ELF between Code and Data/BSS of the executable?    (.dynamic + 0x10000 bytes of a hole)
// Looks like segment alignment is 64K by default(ARM) in case of 64K memory pages (VDSO happily maps in this hole bacause it always take only one page anyway)
// NOTE: There are not mapped pages in alignment holes!
// ElfHdr-.rodata-|?|-.text-|?|-.data-.bss
// Why first fragment of the main EXE is in a hole between .rodata and .text ?
// NOTE: Very inefficient way to determine own base address!
// ARM: 'Bus Error' inside a mapped module on an read-only pages which belong to alignment holes (msync fails to detect that! 'write' also have no problem accessing that memory; process_vm_readv works) // Range,Mode,INode is defined
//
static vptr FindElfByAddr(vptr Addr, size_t* ModSize, bool SafeAddr=true)   // Unsafe!  Pass only an address in code - less likely to have a hole of not mapped pages
{
 static constexpr size_t MaxPages4K = 16;
 size_t PageLen = GetPageSize();
 size_t ptr = AlignP2Bkwd((size_t)Addr, PageLen);
 size_t LastElfSize = 0;
 size_t LastElfAddr = 0;
 size_t MaxElfDist  = MaxPages4K * PageLen;  // In case of 64K pages
 size_t NoPageCnt   = 0;
// DBGDBG("Starting from %p",(vptr)ptr);
 for(size_t fpg=ptr;;ptr -= PageLen)   // TODO: Check if the address is valid somehow. For now - just crash
  {
   if(!SafeAddr)    // Initial address mabe safe (By coming from PHDR addr in AUX or any addr inside the module)
    {
     if(!IsValidMemPtr((vptr)ptr, PageLen))
      {
       if(++NoPageCnt > MaxPages4K)break;  // Max 64K alignment holes (Should do more?)
       if(LastElfAddr && (LastElfAddr - ptr) >= MaxElfDist)break;
//       DBGDBG("Skip=%p",(vptr)ptr);
       continue;
      }
     NoPageCnt = 0;
//     DBGDBG("Trying=%p",(vptr)ptr);
      {   // On some systems those hole pages in a file mapping are read-inly but cause Bus Error when read
       uint8 buf[16];
       PX::iovec vec_l;
       PX::iovec vec_r;
       vec_r.base = (vptr)ptr;
       vec_l.base = &buf;
       vec_l.size = vec_r.size = sizeof(buf);
       sint res2 = NAPI::process_vm_readv(NAPI::getpid(), &vec_l,1, &vec_r,1, 0);  // Move to IsValidMemPtr? // Any faster way to test the readability?
       if(res2 == -PX::EFAULT)continue;   // Any other error probably means that process_vm_readv is not implemented right
      }
    }
     else SafeAddr = false;
   size_t size = ELF::GetModuleSizeInMem((vptr)ptr);
//   DBGDBG("Addr=%p, Size=%p",(vptr)ptr,(vptr)size);
//   DBGDBG("\r\n%#*.32D",256,(vptr)ptr);
   if(!size)continue;
   if((ptr+size) < (size_t)Addr)continue;  // Skip VDSO which could be mapped in a hole
   if(LastElfAddr && memcmp((vptr)ptr,(vptr)LastElfAddr,128))break;   // Already encountered some valid ELF header  // This ELF is different, take a previous one // Hope there will be no random ELFs in the segment gaps, just VDSO and a piece of the same which is mapped here
   LastElfAddr = ptr;
   LastElfSize = size;
   if(ptr == fpg)break;  // Match in the same page - most likekty the addr was in EHDR or PHDR
  }
 if(ModSize)*ModSize = LastElfSize;
 return (vptr)LastElfAddr;
}
//------------------------------------------------------------------------------------------------------------
// Reads a string from a null-terminated string array such as Args or EVars
static sint GetStrFromArr(sint* AIndex, const achar** Array, achar* DstBuf, uint BufLen=uint(-1))
{
 if(DstBuf)*DstBuf = 0;
 if(*AIndex < 0)return -3;     // Already finished
 if(!Array[*AIndex])return -2;    // No such arg   // (AOffs >= (sint)GetArgC())
 const achar* CurStr = Array[(*AIndex)++];  //  GetArgV()[AOffs++];
 uint ArgLen = 0;
 if(!DstBuf)
  {
   while(CurStr[ArgLen])ArgLen++;
   return sint(ArgLen+1);   // +Space for terminating 0
  }
 for(;CurStr[ArgLen] && (ArgLen < BufLen);ArgLen++)DstBuf[ArgLen] = CurStr[ArgLen];
 DstBuf[ArgLen] = 0;
 if(!Array[*AIndex])*AIndex = -1;    // (AOffs >= (sint)GetArgC())
 return sint(ArgLen);
}
//------------------------------------------------------------------------------------------------------------

};

// https://github.com/jhector/armhook-core/blob/master/ELF.cpp

using UELF = NUFELF<size_t>;
//============================================================================================================
/*
  SMemRange range;
   memset(&range,0,sizeof(range));
   sint res = NPFS::FindMappedRangeByAddr(-1, ptr, &range);
   DBGDBG("Range: Res=%i, Beg=%p, End=%p, Mode=%08X, INode=%u",res,(vptr)range.RangeBeg,(vptr)range.RangeEnd,range.Mode,range.INode);
*/
