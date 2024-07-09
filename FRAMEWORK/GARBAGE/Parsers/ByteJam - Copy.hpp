
// TODO: Word size (in bits, 8). Optional tag for each word (uint32)
// Used instead of a HashSet to get unique string IDs (Any byte sequences)
// Consumes ~8 times more memory than original data(Without tags)!  ( But it is sparse and very fast at finding matches )  (Have no limit on a word size and consumes much less memory than a char table would do)
//------------------------------------------------------------------------------------------------------------
// Unit len is in bits, pow2: 0,2,4,8,16,32,64,...  // If not 0, uint32 tag is added for each unit
//
template<int UnitLen=0> class CByteJamBase       // I am tired of naming things :)
{
enum EBFlags         // Distance can be 268435455 to 1073741820 bytes for a nonterminal bit
{                                                 
 flWordEnd  = 0x80000000,  // This bit ends some word
 flHaveIdx  = 0x40000000,  // The offset value is actuallyindex in array of indexes
 flOffsMsk  = 0x0FFFFFFF,  
 flFlagMsk  = 0xC0000000,
 flExLenMsk = (flHaveIdx | (flHaveIdx >> 2))
};
//==================================================================================
struct SBitNode   // Bit node
{
 struct SBit
 {
  uint32 Offs;
  uint32 Flags;   // Includes offset bits
  uint8  Idx;
 
  bool IsTerm(void){return !this->Offs;}
  SBitNode* GetOwner(void){return (SBitNode*)(((SBit*)this) - this->Idx);}
 } Bit[2];
 uint32 MaxOffs;
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
uint GetBitPos(void){return this->BitOffs;}    // Of a next bit
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

CArr<uint32> Stream;   // Should be multithreaded, could be resized or pages added?    // NOTE: Awfully dumb allocator!!!
//------------------------------------------------------------------------------------------------------------
uint32 ReadBitNode(uint32 At, SBitNode* bitn)   // Returns index of a next bit (if any)
{  
 uint32 Desc = this->Stream.Data()[At++]; 
 bool  ExLen = ((Desc & flExLenMsk) == flExLenMsk);  // For a terminal bit position (Both bits require big offsets)
 bitn->Bit[0].Flags = Desc & flFlagMsk; 
 bitn->Bit[1].Flags = (Desc << 2) & flFlagMsk; 
 if(ExLen)  // Both bits have flHaveIdx set 
  {
   uint32 OffsBit1   = this->Stream.Data()[At++]; 
   uint32 OffsBit0   = (Desc & flOffsMsk) | ((OffsBit1 & flFlagMsk) >> 2);  // 2 high bits of extra offset belong to bit`s 0 offset
   bitn->Bit[1].Offs = OffsBit1 & ~flFlagMsk;
   bitn->Bit[0].Offs = OffsBit0;
   bitn->MaxOffs = ~flFlagMsk;
  }
  else  // Only one bit requires offset, the other one is adjacent
   {
    if(Desc & flHaveIdx){bitn->Bit[0].Offs = Desc & flOffsMsk; bitn->Bit[1].Offs = 1;} // The offs belongs to bit 0
      else {bitn->Bit[1].Offs = Desc & flOffsMsk; bitn->Bit[0].Offs = 1;}
    bitn->MaxOffs = flOffsMsk;
   }
 return At;      
}
//------------------------------------------------------------------------------------------------------------
uint32 WriteBitNode(uint32 At, SBitNode* bitn)          // Use only to store new nodes   // TODO: Multithread safety    // flExOffs must be already set accordingly (Only terminal bits and not adjacent ones will have it)
{
 uint32 Desc = bitn->Bit[0].Flags | (bitn->Bit[1].Flags >> 2); 
 bool  ExLen = ((Desc & flExLenMsk) == flExLenMsk);
 if(ExLen)
  {
   uint32 OffsBit1  = bitn->Bit[1].Offs & ~flFlagMsk;
   uint32 OffsBit0  = bitn->Bit[0].Offs & ~flFlagMsk;
   this->Stream.Data()[At++] = (OffsBit0 & flOffsMsk) | Desc;
   this->Stream.Data()[At++] = OffsBit1 | ((OffsBit0 << 2) & flFlagMsk);     // + 2 highs bits for bit 0 offset
  }
  else
   {
    if(Desc & flHaveIdx)this->Stream.Data()[At++] = Desc | (bitn->Bit[0].Offs & flOffsMsk);
      else this->Stream.Data()[At++] = Desc | (bitn->Bit[1].Offs & flOffsMsk);
   }
 return At;
}
//------------------------------------------------------------------------------------------------------------
void UpdateBitFlags(uint32 At, SBit* bit)      // Use to update bits in nodes  // Used only to set flWordEnd 
{
 uint32 CfgOr = (bit->Flags & flFlagMsk);   
 CfgOr >>= (bit->Idx << 1);                
 this->Stream.Data()[At] |= CfgOr;          // TODO: Thread safety (Interlocked OR ?)   
} 
//------------------------------------------------------------------------------------------------------------
void UpdateBitOffset(uint32 At, SBit* bit)     // Original offset expected to be 0  
{
 SBitNode* bitn = (SBitNode*)(bit - bit->Idx);
 if((bitn->Bit[0].Flags & bitn->Bit[1].Flags) & flHaveIdx)   // Have extended offs fields
  {
   if(bit->Idx)this->Stream.Data()[At+1] |= (bit->Offs & ~flFlagMsk);
    else {this->Stream.Data()[At] |= (bit->Offs & flOffsMsk); this->Stream.Data()[At+1] |= ((bit->Offs << 2) & flFlagMsk);}     // Or just ignore the four bits and use max 0x0FFFFFFF offsets ?  // TODO: OR as UINT64 for thread safety
  }
  else this->Stream.Data()[At] |= (bit->Offs & flOffsMsk);    // Assuming that this is a terminal bit which owns the offs field 
}
//------------------------------------------------------------------------------------------------------------
sint8 MatchWordAt(uint32& At, SBits& Word, SBitNode* Bitn, SBit** LastBit)    // NOTE: Do not call on an empty stream or an empty word!   // Returns: 1 if the word is longer than a path; -1 if the word is shorter or equal the path but no end marker; 0 if match 
{
 SBit*   bv = nullptr;
 uint WBits = Word.BitLen;
 Word.Reset();       
 for(;;)      
  {
   sint8 val = Word.Next();
   this->ReadBitNode(At, Bitn);
   bv = &Bitn->Bit[val];
   WBits--;           // Counting the bits is faster 
   if(!WBits)break;   // This bit is last in the word
   if(bv->IsTerm()){*LastBit = bv; return 1;}     // This bit is last on the path  // The word is longer than the path     
   At += bv->Offs; 
  }
 *LastBit = bv;
 if(bv->Flags & flWordEnd)return 0;  // Match - No more bits in the word and the end marker is present
 return -1;  // No end marker
} 
//------------------------------------------------------------------------------------------------------------
sint8 StoreWordAt(uint32& At, SBits& Word)    // Just returns same offset if the word was already stored   
{
 SBitNode Bitn;
 SBit* bv = nullptr;
 if(this->Stream.Count())
  {
   sint8 mv = this->MatchWordAt(At, Word, &Bitn, &bv);
   if(!mv)return 0;       // Already present
   if((mv < 0) && bv)     // Just set the end marker
    {
     bv->Flags |= flWordEnd;
     this->UpdateBitFlags(At, bv);     // Failed to write, the bit has been changed by another thread - must repeat StoreWordAt
     return 1; 
    }
  }
// Continue storing the word at updated by MatchWordAt position (The word is longer than the path) 
 SBitNode bit;
 uint   WBits = Word.GetBitLen() - Word.GetBitPos();   // Number of bits left to store
 uint32 NewAt = this->Stream.Count();
 if(bv)    // Last present bit is terminal and distance to a next bit must be set
  { 
   uint32 NextOffs = NewAt - At;   // Relative to current bit record
   if(NextOffs > Bitn.MaxOffs){LOGERR("Offset is too big!"); return -2;} 
   bv->Offs = NextOffs;
   this->UpdateBitOffset(At, bv);    // Failed to write, the bit has been changed by another thread - must repeat StoreWordAt
  }
 this->Stream.Append(nullptr, WBits+1);   // TODO: Thread safety     // +1 for terminal bit`s extended offset
 while(WBits-- > 0)    // Store new bits
  {
   At = NewAt;
   sint8 val = Word.Next();
   SBit* bv  = &bit.Bit[val];
   SBit* bz  = &bit.Bit[!val];  // Not on the path
   bz->Flags = flHaveIdx;
   bv->Flags = (WBits)?(0):(flHaveIdx|flWordEnd);   // This bit position is terminal else 0
   bv->Offs  = bz->Offs = 0;   // No next bit or it is adjacent
   NewAt = this->WriteBitNode(At, &bit); 
  }
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
 uint32 At = 0;
 SBit*  bv = nullptr;
 SBits wrd(Data, Size);
 return (this->MatchWordAt(At, wrd, &Bitn, &bv) == 0);
}
//------------------------------------------------------------------------------------------------------------
sint8 StoreBytes(const uint8* Data, uint Size, uint32* EndAt=nullptr)
{
 if(!Size)return false;
 uint32 At = 0;
 SBits wrd(Data, Size);
 sint8 res = this->StoreWordAt(At, wrd); 
 if(EndAt)*EndAt = At;     // This value is unique for each unique string
 return res;
}
//------------------------------------------------------------------------------------------------------------
// TODO: Extract list of possible paths for autocomplete
//------------------------------------------------------------------------------------------------------------
#ifdef _DEBUG
// https://github.com/dwyl/english-words
//
// Results:
//   ~10mb per 100000 words
//
static void DoTest(const achar* SrcFile)
{
 CByteJam jam;
 CArr<achar> arr;      // TODO: Make some decent allocator!
 arr.FromFile(SrcFile);
 if(arr.Size() < 1){DBGMSG("Empty or failed to load: %s",SrcFile); return;}
 DBGMSG("Loaded: %s",SrcFile);
 achar* WrdList = arr.Data();
 achar* WBeg = WrdList;
 uint WrdIdx = 0;
 for(;;WrdList++)
  {
   if(*WrdList < 0x20)
    {
     if(!*WrdList)break;
     if(!WBeg)continue;  
     uint32 EndAt;
     uint WLen = WrdList - WBeg;
     sint8 res = jam.StoreBytes((uint8*)WBeg, WLen, &EndAt);
     uint recs = jam.Stream.Count();
     DBGMSG("Res=%i, WrdIdx=%08X, Count=%08X, Size=%08X, EndAt=%08X: %.*s",res,WrdIdx,recs,recs*sizeof(uint32),EndAt,WLen,WBeg);
     WBeg = nullptr;
     WrdIdx++;
    }
     else if(!WBeg)WBeg = WrdList;  
  }
 achar dmpbuf[512];
 NSTR::StrCopy(dmpbuf, SrcFile);
 NSTR::StrCopy(NPTM::GetFileExt(dmpbuf), "jam");
 jam.SaveToFile(dmpbuf);
 DBGMSG("Saved: %s",&dmpbuf);
}
#endif
//------------------------------------------------------------------------------------------------------------
};
//------------------------------------------------------------------------------------------------------------
// TODO: Policy based memory allocator