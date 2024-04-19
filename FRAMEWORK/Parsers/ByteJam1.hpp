
// Used instead of a HashSet to get unique string IDs (Any byte sequences)
//------------------------------------------------------------------------------------------------------------
class CByteJam       // I am tired of naming things :)
{
enum EBFlags    
{
 flSeqEnd  = 0x8000,  // This bit ends some word
 flOIndex  = 0x4000,  // The offset value is actuallyindex in array of indexes
 flOffsMsk = 0x3FFF,  // Max index is 4194303
};
//==================================================================================
struct SBitNode   // Bit node
{
 struct SBit
 {
  uint16 Offs;
  uint16 Flags;   // Includes offset bits
  uint8  Idx;
 
  bool IsTerm(void){return !this->Offs;}
  SBitNode* GetOwner(void){return (SBitNode*)(((SBit*)this) - this->Idx);}
 } Bit[2];

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

CArr<uint32> Stream;   // Should be multithreaded, could be resized or pages added?
CArr<uint32> Offset;   // Extended offsets
//------------------------------------------------------------------------------------------------------------
uint64 ReadBitNode(uint64 At, SBitNode* bit)   // Returns index of a next bit (if any)
{  
 uint32 Desc = this->Stream.Data()[At++]; 
 bit->Bit[0].Flags = Desc & ~flOffsMsk; 
 bit->Bit[0].Offs  = Desc & flOffsMsk; 
 Desc >>= 16;
 bit->Bit[1].Flags = Desc & ~flOffsMsk; 
 bit->Bit[1].Offs  = Desc & flOffsMsk; 
 return At;      
}
//------------------------------------------------------------------------------------------------------------
uint64 WriteBitNode(uint64 At, SBitNode* bitn)          // Use only to store new nodes   // TODO: Multithread safety    // flExOffs must be already set accordingly (Only terminal bits and not adjacent ones will have it)
{
 uint32 Desc = bitn->Bit[1].Flags | bitn->Bit[1].Offs; 
 Desc <<= 16;
 Desc  |= bitn->Bit[0].Flags | bitn->Bit[0].Offs; 
 this->Stream.Data()[At++] = Desc;
 return At;
}
//------------------------------------------------------------------------------------------------------------
bool UpdateBitFlags(uint64 At, SBit* bit)      // Use to update bits in nodes   
{
 uint32 CfgOr = (bit->Flags & (flSeqEnd|flOIndex));    // Only flSeqEnd|flOIndex and only set is supported
 CfgOr <<= (bit->Idx << 4);                 // * 16    // Shift CfgOr by 0 or 16
 this->Stream.Data()[At] |= CfgOr;          // TODO: Thread safety (Interlocked OR ?)   // NOTE: uint16 aligned 
 return true;
} 
//------------------------------------------------------------------------------------------------------------
bool UpdateBitOffset(uint64 At, SBit* bit)    // NOTE: Done only once for a terminal bit     // Original offset expected to be 0  // NOTE: flExOffs must be already set
{
 uint32 CfgOr = (bit->Offs & flOffsMsk);
 CfgOr <<= (bit->Idx << 4);           // * 16  // Shift CfgOr by 0 or 16
 this->Stream.Data()[At] |= CfgOr;    // TODO: Thread safety
 return true;
}
//------------------------------------------------------------------------------------------------------------
sint8 MatchByteAt(uint64& At, uint8 Byte, SBitNode* Bitn, SBit** LastBit)    // NOTE: Do not call on an empty stream or an empty word!   // Returns: 1 if the word is longer than a path; -1 if the word is shorter or equal the path but no end marker; 0 if match 
{
 SBit* bv = nullptr;      
 for(sint8 Bits=7;Bits >= 0;)  // Counting the bits is faster     
  {
   this->ReadBitNode(At, Bitn);
   bv = &Bitn->Bit[bool(Byte & (1 >> Bits))];
   Bits--;
   if(bv->IsTerm())     // This bit is last on the path
     {    
      if(Bits < 0)break;   // This bit is last in the word 
      *LastBit = bv;
      return 1;     // The word is longer than the path (Need storing)
     }
   if(bv->Flags & flOIndex)At += this->Offset.Data()[bv->Offs - 1];
     else At += bv->Offs; 
  }
 *LastBit = bv;
 if(bv->Flags & flSeqEnd)return 0;  // Match - No more bits in the byte and the end marker is present
 return -1;  // No end marker (And the byte is shorter than the path)
} 
//------------------------------------------------------------------------------------------------------------
sint8 StoreByteAt(uint64& At, SBits& Word)    // Just returns same offset if the word was already stored    // NOTE: Do not call on an empty stream or an empty word!
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
   uint64 NextOffs = NewAt - At;   // Relative to current bit record
   if(NextOffs > 0xFFFFFFFF){LOGERR("Offset is too big!"); return -3;}
   if((uint32)NextOffs > flOffsMsk)          // Failed to store (Should store elsewhere)
    {
     this->Offset.Append((uint32*)&NextOffs); // NOTE: The offset is trimmed to uint32
     NextOffs = this->Offset.Count();         // Must not be 0 so taken after
     if((uint32)NextOffs >= flOffsMsk){LOGERR("Offset index overflow!"); return -2;}
   //  DBGTRC("Distance overflow - using index: %08X", (uint32)NextOffs);
     bv->Flags |= flOIndex;
     if(!this->UpdateBitFlags(At, bv))return -1;     // TODO: Merge with UpdateBitOffset
    }  
   bv->Offs = (uint32)NextOffs;
  // DBGMSG("Offs: %08X",NextOffs);
   if(!this->UpdateBitOffset(At, bv))return -1;    // Failed to write, the bit has been changed by another thread - must repeat StoreWordAt
  }
// uint FullCnt = WBits * sizeof(uint32);  //  ((WBits-1) * 2) + 3;      // Last node in a word takes 2 extended offsets
 this->Stream.Append(nullptr, WBits);   // TODO: Thread safety
// LOGMSG("MaxAt: %u, +%u",(uint32)this->Stream.Count(),FullCnt);
 At = NewAt;
 while(WBits > 0)    // Store new bits
  {
   sint8 val = Word.Next();
   SBit* bv  = &bit.Bit[val];
   SBit* bz  = &bit.Bit[!val];  // Not on the path
   bv->Flags = 0;   
   bz->Flags = 0;        // Reserve full extended offset
   bz->Offs  = 0;
   WBits--;
   if(!WBits)bv->Offs = 0;   // Word Terminal bit positions should have an extended offsets on both paths (End of batch)  (Storing longest words first may save some space)
     else bv->Offs = 1;    // Flags + 1 offset ex      // Min distance - this bits is adjacent
//   LOGMSG("At: %u",(uint32)At);
   At = this->WriteBitNode(At, &bit); 
  }
// LOGMSG("EndAt: %u",(uint32)At);
 return 2;
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
 uint WrdIdx = 0;
 for(;;WrdList++)
  {
   if(*WrdList < 0x20)
    {
     if(!*WrdList)break;
     if(!WBeg)continue;  
     uint WLen = WrdList - WBeg;
     sint8 res = jam.StoreBytes((uint8*)WBeg, WLen);
     uint recs = jam.Stream.Count();
     uint indx = jam.Offset.Count();
     DBGMSG("Res=%i, WrdIdx=%08X, Count=%08X, Size=%08X, ICount=%08X, ISize=%08X: %.*s",res,WrdIdx,recs,recs*sizeof(uint16),indx,indx*sizeof(uint32),WLen,WBeg);
     WBeg = nullptr;
     WrdIdx++;
    }
     else if(!WBeg)WBeg = WrdList;  
  }
 DBGMSG("Done");
}
#endif
//------------------------------------------------------------------------------------------------------------
};
//------------------------------------------------------------------------------------------------------------
// TODO: Policy based memory allocator