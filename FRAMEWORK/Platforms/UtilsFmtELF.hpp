
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
static vptr FindStartupInfo(vptr AddrOnStack)
{
 constexpr const size_t MaxArgC  = 0x1000;   // Max is 4k of arguments
 constexpr const size_t StkAlign = 0x1000;   // Max is 4k forward   // NOTE: Bottom of the stack is unknown
 size_t* CurPtr = (size_t*)AddrOnStack;
 size_t* EndPtr = (size_t*)AlignP2Frwd((size_t)AddrOnStack, StkAlign);
 enum EState {stArgC,stArgs,stEnvs,stAuxV,stDone};
 EState state = stArgC;
 if(((uint8*)EndPtr - (uint8*)CurPtr) < 0x100)EndPtr += StkAlign;  // Too small, another page must exist
 size_t* TblEndPtr = nullptr;
 size_t* InfoPtr = nullptr;
 size_t ArgC  = 0;    // Counts everything
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
         else {EndPtr += StkAlign; continue;} // Another page is expected   // DBGDBG("PageAdded: EndPtr=%p",EndPtr);
      }
       else break;
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
// Reads a string from a null-terminated string array such as Args or EVars
static sint GetStrFromArr(sint* AIndex, const achar** Array, achar* DstBuf, uint BufLen=-1)
{
 if(DstBuf)*DstBuf = 0;
 if(*AIndex < 0)return -3;     // Already finished
 if(!Array[*AIndex])return -2;    // No such arg   // (AOffs >= (sint)GetArgC())
 const achar* CurStr = Array[(*AIndex)++];  //  GetArgV()[AOffs++];
 uint  ArgLen = 0;
 if(!DstBuf)
  {
   while(CurStr[ArgLen])ArgLen++;
   return ArgLen+1;   // +Space for terminating 0
  }
 for(;CurStr[ArgLen] && (ArgLen < BufLen);ArgLen++)DstBuf[ArgLen] = CurStr[ArgLen];
 DstBuf[ArgLen] = 0;
 if(!Array[*AIndex])*AIndex = -1;    // (AOffs >= (sint)GetArgC())
 return ArgLen;
}
//------------------------------------------------------------------------------------------------------------

};

using UELF = NUFELF<size_t>;
//============================================================================================================
