
#pragma once
/*
  Copyright (c) 2018 Victor Sheinmann, Vicshann@gmail.com

  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), 
  to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
  and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. 
*/

//#define SIGSCANSAFE

template<int MaxStatSig=0> class CSigScan   // Anything more than MaxStatSig goes to dynamic memory
{
public:
enum EPOp {poDat=0x00,     // Next byte is a single raw byte    // Low half can be random(To prevent some detection)      // TODO: CompileTime signature obfuscator
           poLoH=0x10,     // Low half contains LoHalf of a data byte  // Terminates a byte  // Can be used instead of poDat for obfuscation to prevent some detection
           poHiH=0x20,     // Low half contains HiHalf of a data byte  // Combine with poLoH to make it terminate a byte  // Can be used instead of poDat for obfuscation to prevent some detection
           poRaw=0x40,     // Raw bytes, if (& 0x7F)==0 then next byte is an counter of raw bytes or else rest of bits is a byte counter(63 max)
           poSkp=0x80,     // Skip bytes, counter is same as poRaw; Combine with poRaw to continue in a variable range. Counter byte specifies max range to search for a pattern continuation
  };

enum ESigFlg {sfNone,sfBinary=1,sfBackward=2};

private:
struct SSigRec
{
 PVOID    FoundAddr;
 PBYTE    SigData;    // Signatures stored externally
 char*    SigName;    // Stored externally, if needed
 long     BoundIdx;   // If set then this signature tested only after resolved a sighature which it is bound to
 SSigRec* BoundPtr;   // Resolved from BoundIdx only while searching
 UINT32   MatchIdx;
 UINT16   SigSize;
 UINT16   Flags;
};
 UINT StSigNum;
 UINT DySigNum;
 SSigRec* DySigArr;
 SSigRec  StSigArr[MaxStatSig];
//---------------------------------------------------------------------------
SSigRec* AddSigRec(void)    // These two isolates templateness to prevent code bloat
{
 if(StSigNum >= MaxStatSig)   
  {               
   this->DySigNum++;
   if(!this->DySigArr)this->DySigArr = (SSigRec*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,this->DySigNum*sizeof(SSigRec));    // malloc(this->DySigNum*sizeof(SSigRec));
     else this->DySigArr = (SSigRec*)HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,this->DySigArr,this->DySigNum*sizeof(SSigRec));   //   realloc(DySigArr,this->DySigNum*sizeof(SSigRec));
   return &this->DySigArr[this->DySigNum-1];
  }
 return &this->StSigArr[this->StSigNum++];
}
//---------------------------------------------------------------------------
SSigRec* GetSigRec(UINT Idx)
{
 if(Idx >= this->GetSigCount())return nullptr;
 if(Idx < this->StSigNum)return &this->StSigArr[Idx];
 Idx -= this->StSigNum;
 return &this->DySigArr[Idx];
}
//---------------------------------------------------------------------------

public:
//---------------------------------------------------------------------------
CSigScan(void)
{
 memset(this,0,sizeof(CSigScan));
}
//---------------------------------------------------------------------------
~CSigScan()
{
 if(this->DySigArr)HeapFree(GetProcessHeap(),0,this->DySigArr);      // TODO: MiniRTL malloc/free ?
}
//---------------------------------------------------------------------------
UINT  GetSigCount(void){return this->StSigNum + this->DySigNum;}
//---------------------------------------------------------------------------
PVOID GetSigAddr(char* SigName, UINT Idx)    // By tag or just by index
{
 if(SigName)  // Idx is for same Tag only
  {
   if(!Idx)Idx++;   // From 1
   for(UINT ctr=0;;ctr++)
    {
     SSigRec* Rec = this->GetSigRec(ctr); 
     if(!Rec)return nullptr;
     if(!StrCompareSimple(SigName,Rec->SigName) && !--Idx)return Rec->FoundAddr;
    }
  }
 SSigRec* Rec = this->GetSigRec(Idx); 
 return (Rec)?(Rec->FoundAddr):(nullptr);
}
//---------------------------------------------------------------------------
void AddSignature(PBYTE SigData, UINT SigSize, long BoundIdx=-1, UINT MatchIdx=1, char* SigName=nullptr, UINT16 Flags=0)
{
 if(BoundIdx >= this->GetSigCount())BoundIdx = -1;
 SSigRec* Rec = this->AddSigRec();
 if(!MatchIdx)MatchIdx = 1;   // From 1
 Rec->FoundAddr = nullptr;
 Rec->SigData   = SigData;    // Signatures stored externally
 Rec->MatchIdx  = MatchIdx;
 Rec->BoundIdx  = BoundIdx;
 Rec->SigName   = SigName;
 Rec->SigSize   = SigSize;
 Rec->Flags     = Flags;
}
//---------------------------------------------------------------------------
// Backward('R') sigs are useless and unsafe for a range search?
// Cannot be repeated
UINT FindSignatures(PBYTE AddrLo, PBYTE AddrHi, long Step, bool SkipUnreadable=false)
{                                      
 if(!Step)Step = 1;                // Support backwards?
 UINT FoundCtr = 0;
 UINT TotalSig = this->GetSigCount();
        //         PBYTE TestAddr = AddrLo + 0x002F12C8 - 0x1000;
 while(AddrLo < AddrHi)
  {
//    if(AddrLo == TestAddr)
//      Sleep(100);

   UINT RdFail = 0;
   SIZE_T Size = AddrHi - AddrLo; 
   for(UINT Idx=0;;Idx++)
    {
     SSigRec* Rec = this->GetSigRec(Idx); 
     if(!Rec)break;    // No more signatures
     if(Rec->FoundAddr)continue;  // Already found
     if(Rec->BoundIdx >= 0)
      {
       if(!Rec->BoundPtr)Rec->BoundPtr = this->GetSigRec(Rec->BoundIdx);
       if(!Rec->BoundPtr->FoundAddr)continue;  // Parent is not yet found
      }
#ifdef SIGSCANSAFE
     __try
      {
#endif
       int Match = 0;
       if(Rec->Flags & sfBinary)Match = IsSignatureMatchBin(AddrLo, Size, Rec->SigData, Rec->SigSize, Rec->Flags & sfBackward);
         else Match = IsSignatureMatchStr(AddrLo, Size, (LPSTR)Rec->SigData, Rec->SigSize);
       if((Match == 1) && !--Rec->MatchIdx){FoundCtr++; Rec->FoundAddr = AddrLo; DBGMSG("Address is %p for %u: %s",AddrLo,Idx,(Rec->SigName)?(Rec->SigName):(""));}
#ifdef SIGSCANSAFE
      }
      __except(EXCEPTION_EXECUTE_HANDLER)
       {
        RdFail++;
       }
#endif
    }
#ifdef SIGSCANSAFE
   if(RdFail >= TotalSig)
    {
     AddrLo = (PBYTE)(((SIZE_T)AddrLo & ~0xFFF) + 0x1000);    // To a next page
    }
     else 
#endif
   AddrLo += Step;              
  }   
 LOGMSG("Found %u of %u",FoundCtr,TotalSig);
 return FoundCtr;
}
//---------------------------------------------------------------------------
// Returns 1 if a signature match, 0 if it is not and -1 if it is out of buffer
// Please don`t pass a malformed signatures here :)
static int IsSignatureMatchBin(PBYTE Address, SIZE_T Size, PBYTE BinSig, long SigLen, bool Backward)
{
 PBYTE Data = (PBYTE)Address; 
 long DataDir = (Backward)?(-1):(1);
 while(Size)     // NOTE: 'if' is more cache friendly than 'switch'
  {
   if(SigLen <= 0)return 1;   // End of signature without any mismatch
   BYTE Val = *BinSig; 
   BinSig++; SigLen--;           
   if(!(Val & 0xF0))  // poDat         // Conditions are sorted by Bit Order
    {
     if(*Data != Val)return 0;
     Size--; Data+=DataDir; BinSig++; SigLen--;
    }
   else if(Val & poSkp)  // poSkp   // Must come before poRaw (Bit ordering)
    { 
     SIZE_T Len = (Val == 0xFF)?(((UINT)*(BinSig++) << 7)|0x7F):(Val & 0x7F);   // If the small counter is full then use an extended one
     if(Len > Size)return -1;   // Out of buffer
     Size -= Len;     
     Data += (DataDir * Len);
    }
   else if(Val & poRaw)  // poRaw
    {
     SIZE_T Len = (Val == 0xFF)?(((UINT)*(BinSig++) << 7)|0x7F):(Val & 0x7F);   // If the small counter is full then use an extended one
     if((Len > Size)||((long)Len > SigLen))return -1;   // Out of buffer
     for(;Len;Len--,Size--,BinSig++,SigLen--,Data+=DataDir){if(*Data != *BinSig)return 0;}
    }
   else if(Val & poHiH)   // poHiH     // Must come before poLoH (Bit ordering)
    {
     if((*Data >> 4) != (Val & 0x0F))return 0;
     if(Val & poLoH){Size--; Data+=DataDir;}   // Goes to next byte     // 0x3x is a terminating HiHalf
    }
   else if(Val & poLoH)   // poLoH          // Always terminating
    {
     if((*Data & 0x0F) != (Val & 0x0F))return 0;
     Size--; Data+=DataDir;   // Goes to next byte 
    } 
  }
 return -1;   // Out of buffer
}
//---------------------------------------------------------------------------
// SigLen - number of chars in signature string (Can have a big block of signatures in one string and specify a separate one by offset and length)
// Please don`t pass a malformed signatures here :)
static int IsSignatureMatchStr(PVOID Address, SIZE_T Size, LPSTR Signature, UINT SigLen)
{
 PBYTE Data    = (PBYTE)Address; 
 BYTE  Value   = 0;
 long  SigMult = 1;
 if(!SigLen)SigLen--;         // Overflow the counter - removes size limit
 if('R' == *Signature)        // Reversed signature!  // Revise!
  {
   Signature++; 
   SigLen--;
   SigMult = -1;   
  } 
 for(;*Signature && !Value && (SigLen >= 2);Signature++,SigLen--) // Scan by Half byte
  {
   if(!Size)return -1;  // Out of buffer
   if(*Signature == ' ')continue;   // Skip spaces
   if(*Signature == ';')return 1;      // Start of a comments
   if(*Signature == '*')            // *SkipNum*    
    {
     UINT Len = 0;
     long Counter = DecStrToDW(++Signature, &Len);      // <<<<< Deprecated function!
     if(Counter > (long)Size)return -1;  // Out of buffer
     Signature += Len;
     Data += (Counter*SigMult);   // Skip N bytes
     Size -= Counter;
     continue;
    }       
   Value = *Data;
   long ValueH = CharToHex(Signature[0]);
   long ValueL = CharToHex(Signature[1]);
   if(ValueH < 0)ValueH = (Value >> 4);
   if(ValueL < 0)ValueL = (Value & 0x0F);
   Value  = (BYTE)(((ValueH << 4) | ValueL) ^ Value);
   Signature++;
   SigLen--;
   Size--;
   Data += SigMult;   
  }
 return !Value;
}   
//--------------------------------------------------------------------------- 
};

//============================================================================================================
/*
; Comment // Script input is PBYTE Data, UINT Size
+0x200 ; Offset from beginning // Optional, only in beginning 
-0x400 ; Offset from end       // Optional, only in beginning 

:PatchName: 006534t5634525255656245251235432564564625252353 ; Signature // If unnamed, applied all the the script whose signatures match // +/- HEXNUMBER is an offset instead of signature
+6: 84635235563     ; Sign must be present
-4: 565634252355    ; Offsets are from last signature/Base
-0: 563456245135135 ; Optional patches
*/
class CSigPatch 
{
public:
 enum EPatchFlg {pfNone,pfForce=0x10,pfProtMem=0x20,prSkipUnread=0x40};

private:
struct SCodeBlk
{
 long Offset;
 UINT Size;
 BYTE Data[0];
};

struct SPatchRec   // Must be initialized statically!   // Derive from it to save an original data?   
{
 PBYTE    Addr;   
 int      SigIdx;     // -1 if DirectBase
 LONG_PTR Offset;
 long     BoundIdx;
 UINT     FullSize;   // Useful for reallocation
 UINT     CodeBlkNum;
 BYTE     Name[64];
 BYTE     Data[0];  // Array of SCodeBlk
};

 UINT BrdLo;
 UINT BrdHi;
 UINT PatchCnt;
 PBYTE ScBuf;
 SPatchRec** Patches;
 CSigScan<> SigList;

public:
//---------------------------------------------------------------------------
CSigPatch(void)
{
 memset(this,0,sizeof(CSigPatch));
}
//---------------------------------------------------------------------------
~CSigPatch()
{
 if(this->Patches)
  {
   for(UINT ctr=0;ctr < this->PatchCnt;ctr++)HeapFree(GetProcessHeap(),0,this->Patches[ctr]); 
   HeapFree(GetProcessHeap(),0,this->Patches);      // TODO: MiniRTL malloc/free ?
  }
 if(this->ScBuf)HeapFree(GetProcessHeap(),0,this->ScBuf); 
}
//---------------------------------------------------------------------------
UINT GetPatchCount(void){return this->PatchCnt;}
//---------------------------------------------------------------------------
void SetBorders(UINT Lower, UINT Upper)
{
 this->BrdLo = Lower;
 this->BrdHi = Upper;
}
//---------------------------------------------------------------------------
UINT AddPatch(char* Name, LONG_PTR Offset, long BoundIdx=-1)   // Negative is from end of data
{
 if(BoundIdx >= this->GetPatchCount())BoundIdx = -1;
 this->PatchCnt++;
 if(!this->Patches)this->Patches = (SPatchRec**)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,this->PatchCnt*sizeof(PVOID));    // malloc(this->DySigNum*sizeof(SSigRec));
   else this->Patches = (SPatchRec**)HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,this->Patches,this->PatchCnt*sizeof(PVOID));   //   realloc(DySigArr,this->DySigNum*sizeof(SSigRec));
 SPatchRec*  Patch = (SPatchRec*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(SPatchRec));   // No code blocks for now
 Patch->Addr       = nullptr;
 Patch->Offset     = Offset;
 Patch->BoundIdx   = BoundIdx;
 Patch->SigIdx     = -1;     // -1 if DirectBase
 Patch->FullSize   = sizeof(SPatchRec);   // Useful for reallocation
 Patch->CodeBlkNum = 0;
 UINT NLen = 0;
 for(;Name[NLen] && (NLen < (sizeof(Patch->Name)-1));NLen++)Patch->Name[NLen] = Name[NLen];
 Patch->Name[NLen] = 0;
 this->Patches[this->PatchCnt-1] = Patch;
 return this->PatchCnt-1;
}
//---------------------------------------------------------------------------
UINT AddPatch(char* Name, PBYTE SigData, UINT SigSize, UINT SigMIdx=1, long BoundIdx=-1, UINT16 SigFlags=0)
{
 if(BoundIdx >= this->GetPatchCount())BoundIdx = -1;
 UINT PatchIdx = this->AddPatch(Name, 0, BoundIdx);
 SPatchRec* Patch = this->Patches[PatchIdx];
 Patch->SigIdx = this->SigList.GetSigCount();
 if(BoundIdx >= 0)
  {
   SPatchRec* BPatch = this->Patches[BoundIdx];
   BoundIdx = BPatch->SigIdx;    // Not have to be present   // ???????????????????????????????
  }
 this->SigList.AddSignature(SigData, SigSize, BoundIdx, SigMIdx, (char*)&Patch->Name, SigFlags);
 return PatchIdx;
}
//---------------------------------------------------------------------------
PVOID AddCodeBlock(UINT PatchIdx, long Offset, PVOID Data, UINT Size)
{
 if(PatchIdx >= this->PatchCnt)return nullptr;
 UINT ExSize = Size + sizeof(SCodeBlk);
 this->Patches[PatchIdx] = (SPatchRec*)HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,this->Patches[PatchIdx],this->Patches[PatchIdx]->FullSize+ExSize);
 SPatchRec* Patch = this->Patches[PatchIdx];
 Patch->FullSize += ExSize;
 SCodeBlk* Blk = (SCodeBlk*)&Patch->Data;
 for(UINT Idx=0;Idx < Patch->CodeBlkNum;Idx++)Blk = (SCodeBlk*)((PBYTE)Blk + Blk->Size + sizeof(SCodeBlk));
 Patch->CodeBlkNum++;
 Blk->Offset = Offset;
 Blk->Size   = Size;
 if(Data)memcpy(&Blk->Data,Data,Size);
 return &Blk->Data;
}
//--------------------------------------------------------------------------- 
int ApplyPatches(PVOID Data, SIZE_T Size, long Step=1, UINT Flags=0)        // bool Force=false, bool ProtMem=false     // TODO: Patch by a specified name only, or by names list (including all bound?)
{
 if(!this->Patches || !this->PatchCnt){LOGMSG("No patches loaded!"); return -1;}
 if((this->BrdLo+this->BrdHi) > Size){LOGMSG("No data in range!"); return -2;}
 PBYTE AddrLo = &((PBYTE)Data)[this->BrdLo]; 
 PBYTE AddrHi = &((PBYTE)Data)[Size - this->BrdHi]; 
 UINT  Total  = this->SigList.FindSignatures(AddrLo, AddrHi, Step, Flags & prSkipUnread);
 if((Total != this->SigList.GetSigCount()) && !(Flags & pfForce)){LOGMSG("Not all signatures found!"); return -3;}
 for(UINT ctr=0;ctr < this->PatchCnt;ctr++)
  {
   SPatchRec* Patch = this->Patches[ctr];
   if(!Patch->Addr) 
    {
     if(Patch->SigIdx < 0)
      {
       if(Patch->Offset < 0)Patch->Addr = &((PBYTE)Data)[(LONG_PTR)Size + Patch->Offset];      // Use bound values?
         else Patch->Addr = &((PBYTE)Data)[Patch->Offset];
      }
       else Patch->Addr = (PBYTE)this->SigList.GetSigAddr(nullptr,Patch->SigIdx);   // By signature
    }
   LOGMSG("Record %p [%p]: %s",Patch->Addr,(Patch->Addr-(PBYTE)Data),&Patch->Name);
   if(!Patch->Addr)continue;  // Not found
   SCodeBlk* Blk = (SCodeBlk*)&Patch->Data;
   for(UINT Idx=0;Idx < Patch->CodeBlkNum;Idx++)   // CodeBlkNum may be 0 if this is just a bound point
    {
     PBYTE Addr = &Patch->Addr[Blk->Offset];   // Positive or negative, no validation
     DWORD PrevProt = 0;
     LOGMSG("  Patch %u: %p [%p], %08X",Idx,Addr,(Addr-(PBYTE)Data),Blk->Size);
     if(Flags & pfProtMem){if(!VirtualProtectEx(GetCurrentProcess(),Addr,Blk->Size,PAGE_READWRITE,&PrevProt))return -4;}  
     memcpy(Addr,&Blk->Data,Blk->Size);
     if(Flags & pfProtMem){if(!VirtualProtectEx(GetCurrentProcess(),Addr,Blk->Size,PrevProt,&PrevProt))return -5;}  
     Blk = (SCodeBlk*)((PBYTE)Blk + Blk->Size + sizeof(SCodeBlk));
    }
  } 
 return 0;
}
//--------------------------------------------------------------------------- 
int LoadPatchScript(LPSTR Script, UINT ScriptSize=0)
{
 enum EParState {psNone, psComment, psBordNum, psOffsNum, psCodeBlk, psPatchName, psSigIndex, psPatchBase, psPatchData, psPatchSig};

 if(this->ScBuf)HeapFree(GetProcessHeap(),0,this->ScBuf); 
 if(!ScriptSize)ScriptSize = lstrlenA(Script);
 this->ScBuf = (PBYTE)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,ScriptSize+256); 
 memcpy(this->ScBuf, Script, ScriptSize);

 int  State     = psNone;
 int  CurPatch  = -1;
 bool PatchMode = false;
 bool BoundPtch = false;
 long CodeOffs  = 0;
 UINT SigIdx    = 0;
 UINT Bord[2]   = {0,0};  // +, -
 BYTE Name[64];
 for(;;)
  {
   while(*ScBuf && (*ScBuf <= ' '))ScBuf++;   // Skips any empty lines
   if(!*ScBuf)break;
   switch(State)   // Not fully checks for a Buffer Overflow!
    {
     case psNone:    // New line  
       if(*ScBuf == ';')State = psComment;
       else if(*ScBuf == '"')State = psPatchName;
       else if((*ScBuf == '+')||(*ScBuf == '-'))State = (PatchMode)?(psOffsNum):(psBordNum);
      break;
     case psComment:
       while(*ScBuf && (*ScBuf != '\r') && (*ScBuf != '\n'))ScBuf++;
       if(!ScBuf)break;
       State = psNone;
      break;
     case psBordNum:
       {
        int High = (*ScBuf == '-');
        ScBuf++;
        long Size = 0;
        if(*ScBuf == '$'){ScBuf++; Bord[High] = HexStrToNum<UINT>((LPSTR)ScBuf, &Size);}
          else Bord[High] = DecStrToNum<UINT>((LPSTR)ScBuf, &Size);
        ScBuf += Size;
        State = psNone;
       }
      break;
     case psOffsNum:
       {
        long Mult = (*ScBuf == '-')?(-1):(1);
        long Size = 0;
        ScBuf++;
        if(*ScBuf == '$'){ScBuf++; CodeOffs = HexStrToNum<long>((LPSTR)ScBuf, &Size);}
          else CodeOffs = DecStrToNum<long>((LPSTR)ScBuf, &Size);
        ScBuf += Size;
        while(*ScBuf && (*ScBuf != ':'))Size++;
        ScBuf++;
        CodeOffs *= Mult;
        State = psCodeBlk;
       }
      break;
     case psCodeBlk:     // Usual Str signature
       {
        UINT CodeLen = 0;
        for(;(ScBuf[CodeLen] >= ' ') && (ScBuf[CodeLen] != ';');CodeLen++);
        while(ScBuf[CodeLen-1] == ' ')CodeLen--;
        UINT ByteLen = CodeLen / 2;  // In bytes
        PBYTE Code = (PBYTE)this->AddCodeBlock(CurPatch, CodeOffs, nullptr, ByteLen);
        HexStrToByteArray(Code, (LPSTR)ScBuf, ByteLen);
        ScBuf += CodeLen;
        State  = psNone;
       }
      break;
     case psPatchName:
       {
        ScBuf++;
        UINT NamLen = 0;
        UINT IdxPos = 0;
        for(;ScBuf[NamLen] && (ScBuf[NamLen] != '"');NamLen++);
        for(UINT Ctr=0;(Ctr < NamLen) && (IdxPos < (sizeof(Name)-1));Ctr++){if(ScBuf[NamLen] > ' ')Name[IdxPos++] = ScBuf[Ctr];}  // Skip spaces
        Name[IdxPos] = 0;
        SigIdx = 0;
        ScBuf += NamLen+1; 
        State  = psPatchData;
        if(!PatchMode){PatchMode = true; this->SetBorders(Bord[0], Bord[1]);}   // Set borders
       }
      break;      
     case psSigIndex:
       {
        long Size = 0;
        SigIdx = DecStrToNum<UINT>((LPSTR)ScBuf, &Size);
        ScBuf += Size;
        while(*ScBuf && (*ScBuf != ':'))Size++;
        ScBuf++;
        State  = psPatchSig;
       }
      break;
     case psPatchBase:
       {
        long Mult = (*ScBuf == '-')?(-1):(1);
        long Size = 0;
        LONG_PTR Offset = 0;
        ScBuf++;
        if(*ScBuf == '$'){ScBuf++; Offset = HexStrToNum<LONG_PTR>((LPSTR)ScBuf, &Size);}
          else Offset = DecStrToNum<LONG_PTR>((LPSTR)ScBuf, &Size);
        Offset  *= Mult;
        CurPatch = this->AddPatch((char*)&Name, Offset, (BoundPtch?((int)this->GetPatchCount()-1):(-1))); 
        ScBuf   += Size;
        State    = psNone;      
       }
      break;
     case psPatchData:
       BoundPtch = false;
       if(*ScBuf == '='){ScBuf++; BoundPtch = true;}
       if(*ScBuf == '#'){ScBuf++; State = psSigIndex; break;}   // May be skipped
       if((*ScBuf == '+')||(*ScBuf == '-')){State = psPatchBase; break;}
       State = psPatchData;
      break;
     case psPatchSig:
       {
        UINT SigLen = 0;
        for(;(ScBuf[SigLen] >= ' ') && (ScBuf[SigLen] != ';');SigLen++);
        while(ScBuf[SigLen-1] == ' ')SigLen--;
        CurPatch = this->AddPatch((char*)&Name, ScBuf, SigLen, SigIdx, (BoundPtch?((int)this->GetPatchCount()-1):(-1)));    // Set Patch Index for next code blocks
        ScBuf   += SigLen;
        State    = psNone;
       }
      break;
    }
  }
 DBGMSG("Data offsets: Lo=%08X, Hi=%08X",Bord[0], Bord[1]);
 return 0;
}
//--------------------------------------------------------------------------- 

}; 
//============================================================================================================
