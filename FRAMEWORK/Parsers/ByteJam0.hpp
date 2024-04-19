
// Used instead of a HashSet to get unique string IDs (Any byte sequences)
//------------------------------------------------------------------------------------------------------------
class CByteJam       // I am tired of naming things for now :)
{
static constexpr uint MaxDist = 0x3FFFFF;
enum EBFlags    
{
 flSeqEnd  = 0x80,  // This bit ends some word
 flExOffs  = 0x40,  // No offs if next rec is adjacent  // Of have offs ant it is 0 then this is an terminal rec (Offset is always from current rec by words)
 flOffsMsk = 0x3F,  // Max index is 4194303
};
//==================================================================================
struct SBitNode   // Bit node
{
struct SBit
{
 uint32 Offs;
 uint8  Flags;   // Includes offset bits
 uint8  Idx;

 bool IsTerm(void){return !this->Offs;}
 SBitNode* GetOwner(void){return (SBitNode*)(((SBit*)this) - this->Idx);}
} Bit[2];
    uint64 Pos;     //<<<Debugging
 SBitNode(void){this->Bit[0].Idx = 0; this->Bit[1].Idx = 1;}
};
using SBit = SBitNode::SBit;
//==================================================================================
struct SBits
{
 const uint8* Data;
 uint ByteLen;
 uint BitLen;
 uint BitOffs;

 const uint8* CurPtr;
 const uint8* EndPtr;

//----------------------------------------------------------------------------------
SBits(const uint8* Word, uint Len)
{
 this->Data    = Word;
 this->ByteLen = Len;
 this->BitLen  = Len * 8;
 this->EndPtr  = &this->Data[this->ByteLen];
 this->Reset();
}
//----------------------------------------------------------------------------------
uint GetBitPos(void){return this->BitOffs;}  // + ((this->CurPtr - this->Data) * 8);}  // Of a next bit
uint GetBitLen(void){return this->BitLen;}
void Reset(void){this->CurPtr = Data; this->BitOffs = 0;}
//----------------------------------------------------------------------------------
bool Next(void)       // Bit order: BYTE0{D0-D7}, BYTE1{D0-D7}, ...
{
 sint8 val = (*this->CurPtr >> (this->BitOffs++ & 7)) & 1;
 if(!(this->BitOffs & 7))   
  {
   if(++this->CurPtr >= this->EndPtr)this->Reset();   // No more bits - wrap around
  }
 return val;
}
//----------------------------------------------------------------------------------
};
//==================================================================================

CArr<uint16> Stream;   // Should be multithreaded, could be resized or pages added?
//------------------------------------------------------------------------------------------------------------
uint64 ReadBitNode(uint64 At, SBitNode* bit)   // Returns index of a next bit (if any)
{  
        bit->Pos = At;       // <<<<<
 uint16 Desc = this->Stream.Data()[At++]; 
 bit->Bit[0].Flags = (uint8)Desc; 
 if(Desc & flExOffs)bit->Bit[0].Offs = this->Stream.Data()[At++] | ((Desc & flOffsMsk) << 16);  
   else bit->Bit[0].Offs = (Desc & flOffsMsk); 

 Desc >>= 8;
 bit->Bit[1].Flags = (uint8)Desc; 
 if(Desc & flExOffs)bit->Bit[1].Offs = this->Stream.Data()[At++] | ((Desc & flOffsMsk) << 16); 
   else bit->Bit[1].Offs = (Desc & flOffsMsk); 

 return At;      
}
//------------------------------------------------------------------------------------------------------------
uint64 WriteBitNode(uint64 At, SBitNode* bitn)          // Use only to store new nodes   // TODO: Multithread safety    // flExOffs must be already set accordingly (Only terminal bits and not adjacent ones will have it)
{
 uint16 Desc = bitn->Bit[1].Flags; 
 if(Desc & flExOffs)Desc |= ((bitn->Bit[1].Offs >> 16) & flOffsMsk);
   else Desc |= (bitn->Bit[1].Offs & flOffsMsk);   // Full bit 1 offset fits in the desk byte
 Desc <<= 8;
 Desc |= bitn->Bit[0].Flags; 
 if(Desc & flExOffs)Desc |= ((bitn->Bit[0].Offs >> 16) & flOffsMsk);
   else Desc |= (bitn->Bit[0].Offs & flOffsMsk);   // Full bit 0 offset fits in the desk byte
 
 this->Stream.Data()[At++] = Desc;
 if(bitn->Bit[0].Flags & flExOffs)this->Stream.Data()[At++] = bitn->Bit[0].Offs;
 if(bitn->Bit[1].Flags & flExOffs)this->Stream.Data()[At++] = bitn->Bit[1].Offs;
 return At;
}
//------------------------------------------------------------------------------------------------------------
bool UpdateBitFlags(uint64 At, SBit* bit)      // Use to update bits in nodes   
{
 uint16 CfgOr = (bit->Flags & flSeqEnd);    // Only flSeqEnd and only set is supported
 CfgOr <<= (bit->Idx << 3);                 // * 16  // Shift CfgOr by 0 or 16
 this->Stream.Data()[At] |= CfgOr;          // TODO: Thread safety (Interlocked OR ?)   // NOTE: uint16 aligned 
 return true;
} 
//------------------------------------------------------------------------------------------------------------
bool UpdateBitOffset(uint64 At, SBit* bit)    // NOTE: Done only once for a terminal bit     // Original offset expected to be 0  // NOTE: flExOffs must be already set
{
 if(bit->Flags & flExOffs)
  {
   if(bit->Offs > 0xFFFF)    // Store extra offset bits in Flags byte
    {
     uint16 CfgOr = ((bit->Offs >> 16) & flOffsMsk);
     CfgOr <<= (bit->Idx << 3);           // * 16  // Shift CfgOr by 0 or 16
     this->Stream.Data()[At] |= CfgOr;    // TODO: Thread safety
    }
   if(bit->Idx && (bit[-1].Flags & flExOffs))this->Stream.Data()[At+2] = bit->Offs;   // Bits are in the array bit[2] so bit 0 is right above the bit 1
     else this->Stream.Data()[At+1] = bit->Offs;
  }
 else   // This branch is unlikely to happen (Offsets that updated are terminal offsets which have flExOffs set)
  {
   uint16 CfgOr = (bit->Offs & flOffsMsk);
   CfgOr <<= (bit->Idx << 3);           // * 16  // Shift CfgOr by 0 or 16
   this->Stream.Data()[At] |= CfgOr;    // TODO: Thread safety
  }
 return true;
}
//------------------------------------------------------------------------------------------------------------
sint8 MatchWordAt(uint64& At, SBits& Word, SBitNode* Bitn, SBit** LastBit)    // NOTE: Do not call on an empty stream or an empty word!   // Returns: 1 if the word is longer than a path; -1 if the word is shorter or equal the path but no end marker; 0 if match 
{
 SBit*   bv = nullptr;
 uint WBits = Word.BitLen;
 Word.Reset();
         uint LastPos = 0;    // <<<<<
 while(WBits > 0)  // Counting the bits is faster     
  {
   sint8 val = Word.Next();
   this->ReadBitNode(At, Bitn);
   bv = &Bitn->Bit[val];
   WBits--;
   if(bv->IsTerm())     // This bit is last on the path
     {
      *LastBit = bv;
      if(!WBits)return (bv->Flags & flSeqEnd);   // This bit is last in the word 
        else return 1;     // The word is longer than the path
     }
   At += bv->Offs; 
     /*if(At > 0x0039B33E)
       {
        DBGMSG("Pos: %08X",At);
       }
     if(At > 0x0039B385)
      {
       uint16* ptr = &this->Stream.Data()[Bitn->Pos]; 
       uint16* prv = &this->Stream.Data()[LastPos]; 
       At = 0;
      }  */
         LastPos = Bitn->Pos;  // <<<
  }
 *LastBit = bv;
 if(bv->Flags & flSeqEnd)return 0;  // Match - No more bits in the word and the end marker is present
 return -1;  // No end marker
} 
//------------------------------------------------------------------------------------------------------------
sint8 StoreWordAt(uint64& At, SBits& Word)    // Just returns same offset if the word was already stored    // NOTE: Do not call on an empty stream or an empty word!
{
 SBitNode Bitn;
 SBit* bv = nullptr;
 if(this->Stream.Count())
  {
   sint8 mv = this->MatchWordAt(At, Word, &Bitn, &bv);
   if(!mv)return 0;       // Already present
   if((mv < 0) && bv)     // Just set the end marker
    {
     bv->Flags |= flSeqEnd;
     if(!this->UpdateBitFlags(At, bv))return -1;     // Failed to write, the bit has been changed by another thread - must repeat StoreWordAt
     return 1; 
    }
  }
// Continue storing the word at updated by MatchWordAt position (The word is longer than the path) 
 SBitNode bit;
 uint   WBits = Word.GetBitLen() - Word.GetBitPos();   // Number of bits left to store
 uint64 NewAt = this->Stream.Count();
 if(bv)    // Last present bit is terminal and distance to a next bit must be set
  { 
   uint NextOffs = NewAt - At;   // Relative to current bit record
   if(NextOffs > MaxDist){LOGERR("Distance overflow!"); 
      return -2;}   // Failed to store (Should store elsewhere)
   bv->Offs = NextOffs;
  // DBGMSG("Offs: %08X",NextOffs);
   if(!this->UpdateBitOffset(At, bv))return -1;    // Failed to write, the bit has been changed by another thread - must repeat StoreWordAt
  }
 uint FullCnt = ((WBits-1) * 2) + 3;      // Last node in a word takes 2 extended offsets
 this->Stream.Append(nullptr, FullCnt);   // TODO: Thread safety
// LOGMSG("MaxAt: %u, +%u",(uint32)this->Stream.Count(),FullCnt);
 At = NewAt;
 while(WBits > 0)    // Store new bits
  {
   sint8 val = Word.Next();
   SBit* bv  = &bit.Bit[val];
   SBit* bz  = &bit.Bit[!val];  // Not on the path
   bv->Flags = 0;   
   bz->Flags = flExOffs;        // Reserve full extended offset
   bz->Offs  = 0;
   WBits--;
   if(!WBits){ bv->Offs = 0; bv->Flags |= flExOffs; }  // Word Terminal bit positions should have an extended offsets on both paths (End of batch)  (Storing longest words first may save some space)
     else bv->Offs = 2;    // Flags + 1 offset ex      // Min distance - this bits is adjacent
//   LOGMSG("At: %u",(uint32)At);
   At = this->WriteBitNode(At, &bit); 
  }
// LOGMSG("EndAt: %u",(uint32)At);
 return 1;
}
//------------------------------------------------------------------------------------------------------------

public:
CByteJam(void){}
//------------------------------------------------------------------------------------------------------------
void SaveToFile(const achar* path){this->Stream.IntoFile(path);}    
void LoadFromFile(const achar* path){this->Stream.FromFile(path);}
//------------------------------------------------------------------------------------------------------------
bool MatchBytes(const uint8* Data, uint Size)
{
 if(!Size || !this->Stream.Count())return false;
 SBitNode Bitn;
 uint64 At = 0;
 SBits wrd(Data, Size);
 SBit* bv = nullptr;
 return (this->MatchWordAt(At, wrd, &Bitn, &bv) == 0);
}
//------------------------------------------------------------------------------------------------------------
sint8 StoreBytes(const uint8* Data, uint Size)
{
 if(!Size)return false;
 uint64 At = 0;
 SBits wrd(Data, Size);
 return this->StoreWordAt(At, wrd); 
}
//------------------------------------------------------------------------------------------------------------
// TODO: Extract list of possible paths for autocomplete
//------------------------------------------------------------------------------------------------------------
#ifdef _DEBUG
// https://github.com/dwyl/english-words
//
static void DoTest(const achar* SrcFile)
{
 CByteJam jam;
 CArr<achar> arr;
 arr.FromFile(SrcFile);
 if(arr.Size() < 1){DBGMSG("Empty or failed to load: %s",SrcFile); return;}
 achar* WrdList = arr.Data();
 achar* WBeg = WrdList;
 for(;;WrdList++)
  {
   if(*WrdList < 0x20)
    {
     if(!*WrdList)break;
     if(!WBeg)continue;  
     uint WLen = WrdList - WBeg;
     sint8 res = jam.StoreBytes((uint8*)WBeg, WLen);
     uint recs = jam.UCount();
     DBGMSG("Res=%i, Count=%08X, Size=%08X: %.*s",res,recs,recs*2,WLen,WBeg);
     WBeg = nullptr;
    }
     else if(!WBeg)WBeg = WrdList;  
  }
}
#endif
//------------------------------------------------------------------------------------------------------------
};
//------------------------------------------------------------------------------------------------------------
