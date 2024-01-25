 
#pragma once

/*
  Copyright (c) 2024 Victor Sheinmann, Vicshann@gmail.com

  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), 
  to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
  and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. 
*/

/*
 https://docs.python.org/3/library/configparser.html

 Nesting is specified by '.' in beginning of a section name and represents level of nesting from root section.
 If a last section have one '.' or none then current section will be its child if have its name with one more '.'

 All whitespaces and line breaks are preserved.
 Comment lines and inline comments are preserved too.

 A line starting with a space(tab, any whitespace char) considered to be a continuation of a previous line`s value(section indentation excluded) (comments are ignored) (single leading whitespace is removed)

 Names can contain spaces, but leading and trailing ones are trimmed when creating or changing names
 All names are case sensitive, there is no handling of duplicates but you can specify index of a match on request (0 by default)

 Comments start with ';' or '#'
 Value assignment is done with '=' or ':'

 Values can be outside of any section ("" or Null section owns them)
---------------------------

true	1, t, y, on, yes, enabled, true
false	0, f, n, off, no, disabled, false

Note: Keys must have a value separator or they won`t be visible for enumeration
Note: Setting a value with line breaks('\n') will save it as multiline and it could only be retrieved line by line
---------------------------
TODO: 1) Encryption (With custom/build key)
      2) Binary save/load
      3) Add terminating 0 to every meaningful string (DSize+1) ???
      4) Extend support for duplicate names ???
*/
//----------------------------------------------------------------------------
class CMiniIni       // template<typename DS>   // DS may be a std::array     
{
enum EType  {etNone=0, etComment=0x01, etName=0x02, etValue=0x04, etSection=0x08};    // Order is important
enum EFlag  {efNone=0, efMultiline=0x01};
public:
enum EFlags {
  efUseFirstMrk      = 0x01,   // Use first found markers instead of statistic when parsing a file
  efUseDisabledSec   = 0x02, 
  efUseDisabledVal   = 0x04, 
  efNameCaseSensSec  = 0x08, 
  efNameCaseSensVal  = 0x10,
  efAllowInlineCmnt  = 0x20,   // Makes '#' and ';' inappropriate value chars
  efIgnoreSecNesting = 0x40, 
};
enum EFormat {      // Format when adding new records
  efSecAfterNL       = 0x01,
  efSecBeforeNL      = 0x02,
  efSecSpacedNames   = 0x04,
  efSecIndentNested  = 0x08,
  efNameValSpacing   = 0x10,
  efNameValAlign     = 0x20,
  efCmntSpacing      = 0x40,
  efCmntIndenting    = 0x80,
}; 

// Linux conf files
static const achar  CfgMrkEOL  = '\n';   // 0x0A
static const achar  CfgMrkCMN  = '#';
static const achar  CfgMrkNVS  = ':';

// Windows ini files
static const achar  IniMrkEOL  = achar(('\n' << 4) | '\r');   // 0x0D,0x0A    // Extract by >> 4 shift
static const achar  IniMrkCMN  = ';';
static const achar  IniMrkNVS  = '=';

static const achar  SecNestMrk = '.';
static const achar  IndentChar = ' ';

private:
static constexpr uint64 NoRecMrk  = (uint64)-1;     // sizeof(SRecHdr)
static constexpr int    MinIniLen = 3;
static constexpr int    MaxTxtLen = 0xFFFF;
static constexpr int    NestSecIndent = 1;
static constexpr int    MLineValIndnt = 2;
static constexpr int    SecNameSpaces = 1;  // between section name and '[', ']'
static constexpr int    NameValSpaces = 1;  // Between name, '=' and a value
static constexpr int    CmntValSpaces = 1;  // Between ';' and the value itself
static constexpr int    CmntInlSpaces = 8;  // Base indent of inline comments

 CArr<achar> Data;
 uint16 LastRecSize = 0;  // ASize of last record

#ifdef SYS_WINDOWS             // For creation of new files
 achar  DefEOL  = IniMrkEOL;   
 achar  DefCMN  = IniMrkCMN;
 achar  DefNVS  = IniMrkNVS;
#else
 achar  DefEOL  = CfgMrkEOL;   
 achar  DefCMN  = CfgMrkCMN;
 achar  DefNVS  = CfgMrkNVS;
#endif

public:
 achar  Flags   = efUseFirstMrk;
 achar  Format  = efSecIndentNested|efSecSpacedNames|efSecBeforeNL|efNameValSpacing|efCmntSpacing;

private:
struct SRecHdr   // All records are aligned to 8  (8 byte SRecHdr should be faster than bitmasking Type And size every time during iterations)
{
 using T = uint64;
// uint32 DSize: 15;   // Size of actual data
// uint32 FSize: 15;   // Size to a next record
// uint32 Type:  2;    // EType   // High 2 bits
 uint16 ASize;   // Aligned size (includes size of header)
 uint16 DSize;   // Size of the text
 uint16 FSize;   // Full size (Includes anything that not the text value itself)
 uint8  Flags;
 uint8  Type;    // Type/Flags
 achar  Data[0];

 static uint ReadLineSizeAt(const achar* At){return (*At << 8) | At[1];}   // Big-endian

 SRecHdr* Next(void){return (SRecHdr*)((uint8*)this + this->ASize);}
//---------------------------------------
 uint TextSize(void)    // Actual text size (To save)
 {
  if(this->Flags & efMultiline)
   {
    uint ExtraLen = 0;
    achar* EndPtr = &this->Data[this->DSize]; 
    for(achar* CurPtr=this->Data;CurPtr < EndPtr;)
     {
      uint LineLen = ReadLineSizeAt(CurPtr);
      CurPtr   += LineLen;
      ExtraLen += sizeof(uint16);
     }
    return this->FSize - ExtraLen;
   }
  return this->FSize;
 }
 //---------------------------------------
 const achar* NextLine(const achar* PrvLine, uint* Size)    // NOTE: Returns meaningfull text only, not entire string of FSize 
 {
  if(this->Flags & efMultiline)
   {
    if(!PrvLine)
     {
      if(Size)*Size = ReadLineSizeAt(this->Data);
      return &this->Data[sizeof(uint16)];
     }
    achar* EndPtr = &this->Data[this->DSize];  
    uint   PrvLen = ReadLineSizeAt(&PrvLine[-sizeof(uint16)]);
    const achar* CurPtr = PrvLine + PrvLen;
    if(CurPtr >= EndPtr)return nullptr;  // No more lines
    if(Size)*Size = ReadLineSizeAt(CurPtr);
    return &CurPtr[sizeof(uint16)];
   }
  if(PrvLine)return nullptr;    // Only a first line
  if(Size)*Size = this->DSize;
  return this->Data;
 }

};

static_assert((sizeof(SRecHdr)==sizeof(uint32))||(sizeof(SRecHdr)==sizeof(uint64)));

//===============================================================
struct SBaseRec
{
 friend class CMiniIni;   // Does not give us ability to init this object as POD if its members are pritected or private
//protected:
 CMiniIni* Owner   = nullptr;
 uint32    Offset  = 0;      // May become invalid if the Owner is modified from another Rec
 uint32    PrvOffs = 0;      // TODO: Calculate?

protected:
 SRecHdr* GetThisRec(void) const {return (SRecHdr*)&this->Owner->GetBufPtr()[this->Offset];}
 SRecHdr* GetPrevRec(void) const {return (SRecHdr*)&this->Owner->GetBufPtr()[this->PrvOffs];}      // Useful?
 SRecHdr* GetNextRec(void) const {return (SRecHdr*)&this->Owner->GetBufPtr()[this->Offset + this->GetThisRec()->ASize];}

public:
 bool IsValid(void) const {return (bool)this->Owner;}
 bool Remove(void) {bool res = this->Owner->RemoveRec(this); this->Owner = nullptr; this->PrvOffs = this->Offset = 0; return res;}
};

//===============================================================
template<typename T> struct SToolRec: public SBaseRec
{
// Index: 0 = on the same line, +N a line below, -N a line above
 const achar* GetComment(sint cmntidx, uint* ValLen) {return Owner->GetComment(this, cmntidx, ValLen);}
 void SetComment(sint cmntidx, achar* src, uint srcsize=0) {return Owner->SetComment(this, cmntidx, src, srcsize);}

};

public:
//===============================================================
struct SCmtRec: public SToolRec<SCmtRec>     // Contains functions for section manipulations
{

};
//===============================================================
struct SValRec: public SToolRec<SValRec>     // Contains functions for Name/Value manipulations    // Offset points  to name
{
protected:

public:
    // GetValXX  // To be able to chain value requests from a section object
//---------------------------------------
const achar* GetName(uint& Size)     // Return of the size is mandatory(strings are not 0-terminated)
{
 SRecHdr* rec = this->GetThisRec();
 Size = rec->DSize;
 return rec->Data;
}
//---------------------------------------
const achar* GetValue(uint& Size, const achar* PrvLine=nullptr)    // Pointer to the data for fast read-only access
{
 SRecHdr* rec = this->GetNextRec();
 return rec->NextLine(PrvLine, &Size);    
}
//---------------------------------------
bool AsBool(void)           // 1(Any nonzero number), on, [y]es, [t]rue, enabled 
{
 SRecHdr* rec  = this->GetNextRec();
 bool NegNum   = false;
 bool HexNum   = false;
 uint Size     = 0;
 uint ctr_ws   = 0;
 uint ctr_num  = 0;  // Nonzero digits count
 uint ctr_let  = 0;
 uint ctr_hex  = 0;
 uint ctr_on   = 0;
 uint ctr_yes  = 0;
 uint ctr_true = 0;
 uint ctr_enbl = 0;   // enable/enabled

 for(achar* CurPtr=rec->Data,*EndPtr=&rec->Data[rec->DSize];CurPtr < EndPtr;CurPtr++,Size++)
  {
   achar val = *CurPtr;
   bool IsWS = (val <= 0x20);
   if(ctr_ws && !IsWS)return false;     // Should be a single token
   if(IsWS){ctr_ws++; continue;}    // No whitespaces in the beginning, parser skips them    // TODO: Make sure that when editing
   if(!Size)
    {
     if(val == '-'){NegNum=true; continue;}    // Sign is allowed as a first char
     if(val == '$'){HexNum=true; continue;}                                                       // $???
     if(val == '0' && (rec->DSize > 1) && ((CurPtr[1] | 0x20) == 'x')){HexNum=true; CurPtr++; Size++; continue;}    // 0x???
    }  
   if((val <= '9') && (val >= '0'))
    {
     ctr_num += (val != '0'); 
     continue;
    }
   if(val == '.')continue;  // Ignore // TODO: Check floats? 
   val |= 0x20;  // ASCII to lower case
   if((val < 'a')||(val > 'z'))return false;  // Not a letter or a digit
   if(HexNum){if(val > 'f')return false; ctr_hex++;}  
   if(Size < 4)
    {
     if(Size < 1)   // 0
      {
       if(val == 'o')ctr_on++;
       else if(val == 'y')ctr_yes++;
       else if(val == 't')ctr_true++;
       else if(val == 'e')ctr_enbl++;
      }
     else if(Size < 2)  // 1
      {
       if(val == 'n'){ctr_on++; ctr_enbl++;}
       else if(val == 'e')ctr_yes++;
       else if(val == 'r')ctr_true++;
      }
     else if(Size < 3)  // 2
      {
       if(val == 's')ctr_yes++;
       else if(val == 'u')ctr_true++;
       else if(val == 'a')ctr_enbl++;
      }
     else  // 3
      {
       if(val == 'e')ctr_true++;
       else if(val == 'b')ctr_enbl++;
      }
    }
   else if(ctr_enbl && (Size < 7))     // 7 is size of 'enabled'
    {
     if((Size < 5) && (val == 'l'))ctr_enbl++;
     else if((Size < 6) && (val == 'e'))ctr_enbl++;
      else if(val == 'd')ctr_enbl++;
    }
   else if(Size > 8)break;      // No more chars is expected
   ctr_let++;
  }
 if(ctr_let)
  {
   if((2 == ctr_on)   && (ctr_let == ctr_on))return true;     // on
   if((6 == ctr_enbl) && (ctr_let == ctr_enbl))return true;   // enable
   if((7 == ctr_enbl) && (ctr_let == ctr_enbl))return true;   // enabled
   if((1 == ctr_yes)  && (ctr_let == ctr_yes))return true;    // y
   if((3 == ctr_yes)  && (ctr_let == ctr_yes))return true;    // yes
   if((1 == ctr_true) && (ctr_let == ctr_true))return true;   // t
   if((4 == ctr_true) && (ctr_let == ctr_true))return true;   // true
  }
 else if(ctr_num || ctr_hex)return true;    // Some nonzero number

 return false;
}
//---------------------------------------
sint64 AsInt(void)
{
 SRecHdr* rec = this->GetNextRec();
 if(!rec->DSize)return 0;  // Empty
 achar* BPtr = rec->Data;
 achar val = *BPtr;
 sint64 res;
 if(val == '$')res = NCNV::HexStrToNum<decltype(res)>(&BPtr[1]); 
 else if((rec->DSize > 2) && (val == '0') && ((BPtr[1] | 0x20) == 'x'))res = NCNV::HexStrToNum<decltype(res)>(&BPtr[2]); 
 else res = NCNV::DecStrToNum<decltype(res)>(BPtr);
 return res;
}
//---------------------------------------
uint64 AsUInt(void)
{
 SRecHdr* rec = this->GetNextRec();
 if(!rec->DSize)return 0;  // Empty
 achar* BPtr = rec->Data;
 achar val = *BPtr;
 uint64 res;
 if(val == '$')res = NCNV::HexStrToNum<decltype(res)>(&BPtr[1]); 
 else if((rec->DSize > 2) && (val == '0') && ((BPtr[1] | 0x20) == 'x'))res = NCNV::HexStrToNum<decltype(res)>(&BPtr[2]); 
 else res = NCNV::DecStrToNum<decltype(res)>(BPtr);
 return res;
}
//---------------------------------------
//double AsFloat(void){}
//---------------------------------------
void AsBool(bool val)
{
 if(val)
  {
   achar val[] = {'t','r','u','e'};
   this->Owner->ChangeValue(this, val, sizeof(val));
  }
  else
   {
    achar val[] = {'f','a','l','s','e'};
    this->Owner->ChangeValue(this, val, sizeof(val));
   }
}
//---------------------------------------
void AsInt(sint64 val)
{
 achar buf[64];
 uint Len = 0;
 achar* ptr = NCNV::DecNumToStrS(val, buf, &Len);
 this->Owner->ChangeValue(this, ptr, Len);
}
//---------------------------------------
void AsUInt(uint64 val)
{
 achar buf[64];
 uint Len = 0;
 achar* ptr = NCNV::DecNumToStrU(val, buf, &Len); 
 this->Owner->ChangeValue(this, ptr, Len);
}
//---------------------------------------
//void AsFloat(double val){}
//---------------------------------------
void AsBytes(vptr ptr, uint size)   // Set
{
 this->Owner->ChangeValue(this, nullptr, size << 1);
 SRecHdr* rec = this->GetThisRec();
 NCNV::ByteArrayToHexStr(ptr, rec->Data, size, true);
}
//---------------------------------------
vptr AsBytes(vptr ptr, uint* size)    // Get
{
 SRecHdr* rec = this->GetThisRec();
 uint BytesInStr = rec->DSize >> 1;  //Div 2
 if(BytesInStr > *size)BytesInStr = *size;
 *size = NCNV::HexStrToByteArray(ptr, rec->Data, BytesInStr);
 return ptr;
}
//---------------------------------------
// TODO: AsBase64 ???
};

struct SSecRec: public SToolRec<SSecRec>     // Contains functions for section manipulations
{
uint GetNestLvl(void) const
{
 SRecHdr* rec = this->GetThisRec();
 uint NstCtr = 0;
 this->Owner->GetSecInfoFromName(rec, NstCtr, 0);
 return NstCtr;
}
//---------------------------------------
SValRec GetValue(const achar* Name, uint Len=0) {return this->Owner->FindValue(this, Name, Len);}
//SSecRec GetSection(const achar* Name, uint Len=0) {return this->Owner->FindSection(this, Name, Len);}
SValRec SetValue(const achar* name, const achar* value, uint vlen=0, uint nlen=0){return this->Owner->SetValue(name, value, this, vlen, nlen);} 
SSecRec GetSection(const achar* name, uint len=0) {return this->Owner->GetSection(name, this, len);}     // SetSection
//---------------------------------------
};
//===============================================================

private:
//----------------------------------------------------------------------------
uint   GetBufLen(void) const {return this->Data.Size();}
uint8* GetBufPtr(void) const {return (uint8*)this->Data.Data();}
uint8* GetBufEnd(void) const {uint len = this->GetBufLen(); return (len >= sizeof(SRecHdr))?(&this->GetBufPtr()[len - sizeof(SRecHdr)]):(nullptr);}
uint   GetSizeOfEOL(void) const {return ((bool)(this->DefEOL >> 4)) + 1;}
static bool IsLastRecHdr(const SRecHdr* rec) {return (*(uint64*)((uint8*)rec + rec->ASize) == NoRecMrk);}
static _finline SRecHdr* NextRecHdr(const SRecHdr* rec) {return (SRecHdr*)((size_t)rec + rec->ASize);}
SRecHdr* GetLastRecHdr(void) const {uint len = this->LastRecSize + sizeof(SRecHdr); return (this->GetBufLen() >= len)?((SRecHdr*)&this->GetBufPtr()[this->GetBufLen()-len]):nullptr;}              //(this->GetBufLen() >= (sizeof(SRecHdr))*2)?(SRecHdr*)&this->GetBufPtr()[this->GetBufLen()-sizeof(SRecHdr)]:nullptr;}   // NOTE: GetBufLen must be of actual data size

//----------------------------------------------------------------------------
static uint BufferedWriteFile(const void* Data, uint Size, vptr Buf, uint BufSize, uint BufOffs, uint& TotalWr, int fd, const achar* FilePath)  // Returns updated BufOffs    // TODO: File stream class with buffering
{
 if(Data)TotalWr += Size;      // Statistic, unaffected by fails
 if((BufOffs + Size) >= BufSize)   // Need flushing
  {
   sint wlen = NPTM::NAPI::write(fd, Buf, BufOffs);
   if(wlen <  BufOffs){DBGERR("Error: Failed to write(flush) the file(%i): %s!",wlen,FilePath); return 0;}  // NOTE: Returning without even trying to save current data
   if(Size >= BufSize)   // Does not fits in the cache - Just save the data directly
    {
     if(Data)   // Size of BufSize can be used to just flush the buffer
      {
       sint wlen = NPTM::NAPI::write(fd, Data, Size);
       if(wlen < BufOffs){DBGERR("Error: Failed to write(save) the file(%i): %s!",wlen,FilePath);}  
      }
     return 0;  // Saved - no caching needed
    }
  }
 if(Data && Size)memmove((uint8*)Buf + BufOffs, Data, Size);
 return BufOffs + Size;
}
//----------------------------------------------------------------------------
// In the Windows environment a line is terminated with the two characters \r\n. The \r character represents a carriage return, and \n represents a newline. In Linux, only the \n character is used to terminate a line.
static uint CheckLineBreak(const achar* Ptr)          // TODO: Move to ParseUtils.hpp
{
 uint Tot = 0;
 achar V0 = Ptr[0];
// if(V0 >= 0x20)return 0;      // Makes the check faster?
 if(!V0)return 1;     // Check this separatedly if needed
 achar V1 = Ptr[1];
 if(V0 == '\n')
  {
   Tot++;        // Most likely Linux EOL
   if(V1 == '\r')Tot++;  // Needed???  Some wrong EOL marker on Windows ("\n\r" instead of "\r\n")
  }
 else if(V0 == '\r')
  {
   Tot++;        // Most likely Windows EOL
   if(V1 == '\n')Tot++;   // Windowws EOL
  }
 return Tot;
}
//----------------------------------------------------------------------------
static uint GetSecInfoFromName(SRecHdr* rec, auto&& NstCtr, auto&& CmnCtr)   // Disable and nest markers are part of a section name    // TODO: Optimize!!!!!
{
 achar* cptr = &rec->Data[0];
 achar* eptr = &cptr[rec->DSize];
 while((cptr < eptr) && (*cptr <= 0x20))cptr++;   // Skip any spaces before (Should be none)
 while((cptr < eptr) && ((*cptr == CfgMrkCMN) || (*cptr == IniMrkCMN))){cptr++; CmnCtr++;}   // Skip any number of comment markers
 while((cptr < eptr) && (*cptr <= 0x20))cptr++;   // Allowed to be spaces
 while((cptr < eptr) && (*cptr == SecNestMrk)){cptr++; NstCtr++;}
 while((cptr < eptr) && (*cptr <= 0x20))cptr++;   // There are a spaces allowed after nesting markers
 return cptr - &rec->Data[0];   // Returns actual name offset
}
//----------------------------------------------------------------------------
static SRecHdr* FindPrevRec(uint8* CurPtr, uint8* EndPtr)  // Not 100% reliable
{
 uint8* ThisRec = CurPtr;
 for(;CurPtr >= EndPtr;CurPtr -= sizeof(SRecHdr))
  {
   SRecHdr* rec = (SRecHdr*)CurPtr;
   if(!rec->ASize)continue;
   if(rec->Type > 0xFF)continue;
   if(rec->DSize > rec->FSize)continue;
   if(rec->ASize < rec->FSize)continue;
   if(&CurPtr[rec->ASize] == ThisRec)return rec;
  }
 return nullptr;
}
//----------------------------------------------------------------------------
static uint GetRecIndent(SRecHdr* rec)    // All indentation belongs to a prev record (beyond its actual text)
{
 achar* bptr = &rec->Data[rec->DSize];
 achar* cptr = &rec->Data[rec->FSize-1];
 uint IndCtr = 0;
 for(;cptr >= bptr;cptr--)  // In tail range
  {
   uint8 val = *cptr; 
   if(val == CfgMrkEOL)break;  // EOL
   if(val > 0x20)IndCtr = 0;   // Reset on non WS
     else IndCtr++;
  }
 return IndCtr;
} 
//----------------------------------------------------------------------------
static sint GetRecIndent(SBaseRec* rec)      // NOTE: Indents belong to a previous record (Meaningless for values)
{
 if((uint8*)rec == rec->Owner->GetBufPtr())return -1;   // No prev record(nothing before)!
 SRecHdr* hdr = rec->GetThisRec();
 if(hdr->Type == etValue)return -1;  // Meaningless for values
 return GetRecIndent(rec->GetPrevRec());
}
//----------------------------------------------------------------------------
achar* WriteEOL(achar* At)
{
 achar val = this->IniMrkEOL;
 *At = val & 0x0F;
 val >>= 4;
 if(val){At++; *At = val & 0x0F;}
 return ++At;
}
//----------------------------------------------------------------------------
SRecHdr* GetNextRec(SRecHdr* rec) const   // No checks if GetBufPtr() returns NULL     // More checks than NextRecHdr()
{
 uint8* EndPtr = this->GetBufEnd();
 uint8* NxtPtr = (uint8*)rec + rec->ASize;
 if(NxtPtr >= EndPtr)return nullptr;
 return (SRecHdr*)NxtPtr;
}
//----------------------------------------------------------------------------
SRecHdr* GetPrevRec(SRecHdr* rec) const   // Not 100% reliable
{
 uint8* EndPtr = this->GetBufPtr();
 return FindPrevRec((uint8*)rec, EndPtr); 
}
//----------------------------------------------------------------------------
uint AppendRecord(uint DstOffs, uint SrcOffs, const achar* SrcPtr, uint dlen, uint flen, uint& ValLnCnt, EType type)     // From a source file
{
 const achar* dptr = &SrcPtr[SrcOffs];
 DBGDBG("Type=%u, DstOffs=%08X, SrcOffs=%08X, dlen=%08X, flen=%08X:\r\n%#*.32D",(int)type, DstOffs, SrcOffs, dlen, flen, flen, dptr);
 uint ExtrLen = 0;
 if(ValLnCnt > 1)ExtrLen += CalcMultilineValueInfo(dptr, dlen, 0, nullptr);    // Indent chars is already there
 uint alsize  = AlignP2Frwd(flen+ExtrLen, sizeof(SRecHdr)) + sizeof(SRecHdr); 
 uint ResOffs = DstOffs + alsize; 
 uint CurSize = this->GetBufLen();
 if(ResOffs >= CurSize)this->Data.SetSize(ResOffs + (CurSize/4) + 8);    // NOTE: No separate Size/Prealloc in CArray yet
 SRecHdr* Rec = (SRecHdr*)&this->GetBufPtr()[DstOffs];
 Rec->Type  = type;
 Rec->Flags = 0;    // !!! TODO: Multiline
 Rec->DSize = dlen+ExtrLen;    // NOTE: sizes are truncated to uint16
 Rec->FSize = flen+ExtrLen;
 Rec->ASize = alsize;
 if(ValLnCnt > 1)
  {
   Rec->Flags |= efMultiline;
   achar* ptr = CopyTextAsMultiline(Rec->Data, dptr, dlen, 0);
   memcpy(ptr, &dptr[dlen], flen-dlen);   // Copy the tail
  }
   else memcpy(&Rec->Data, dptr, flen);
 ValLnCnt = 0;
 return ResOffs;
}
//----------------------------------------------------------------------------
sint ParseIniData(const achar* SrcData, uint SrcSize)                    // Comment index 0 is at same line, -1 a line above, +1 a line below    // Multiline values can be read only as a string, receiving min and max line widths as optional
{
 enum EState {stExpSecBeg=0x001, stExpSecEnd=0x002, stExpNVSep=0x004, stExpCmnt=0x008, stExpMultiline=0x010, stInSecName=0x020, stInValue=0x040, stInComnt=0x080, stExpSecName=0x100, stInMultiline=0x200}; 

 uint   Offs = 0;
 sint   CtrLbWS = -1;        // From beginning of the line (indent)
 sint   FirstWS = -1;        // Can span lines 
 sint   FirstNonWS = -1;     // Cannot span lines
 sint   ValEndOffs = -1;
 sint   LastCtrLbWS = -1;
 sint   LastCmntOffs = -1;
 uint   LastLineOffs = 0;
 uint   DstOffs = 0;
 uint   Expect  = stExpSecBeg|stExpNVSep|stExpCmnt;        // List of unnamed values are supported too
 uint   LstVlLnCnt = 0;
 uint   LstNameInd = 0;
 uint   PrvRecOffs = 0;   // Beginning of the record
 sint   PrvRecType = 0;
 uint16 PrvRecDLen = 0; // Size of actual record`s data
 uint   CtrCfgEOL = 0;  // Statistic
 uint   CtrCfgCMN = 0;  // Statistic
 uint   CtrCfgNVS = 0;  // Statistic
 uint   CtrIniEOL = 0;  // Statistic   // Windows (2 chars - 'rn')
 uint   CtrIniCMN = 0;  // Statistic   // Windows
 uint   CtrIniNVS = 0;  // Statistic   // Windows
 bool UseFirstMrk = this->Flags & efUseFirstMrk;   // Use only a first encountered marker   // Useful only on Windows if values may contain '#'
 uint ExpctInlCmt = (this->Flags & efAllowInlineCmnt)?stExpCmnt:0;     // Allows inline comment parsing to be turned off
 for(;Offs <= SrcSize;Offs++)
  {
   achar const* CurPtr = &SrcData[Offs];   //(Offs < SrcSize)?(&SrcData[Offs]):(&Nulls[0]);   // The source array may not contain final EOL or a terminating 0 
   achar fc = *CurPtr;
   if(fc > 0x20)    // Is whitespace?
    {
     if(CtrLbWS < 0)CtrLbWS = (Offs - LastLineOffs);      // Required for multiline values
     if(FirstNonWS < 0){FirstNonWS = Offs; FirstWS = -1;}    // Start counting untill next whitespace
     if(Expect & stExpMultiline)     // Check this separately and before anything else     // TODO: fill multiline separation with separation chars with -9 
      {
       if((CtrLbWS > LstNameInd) && (fc != '['))Expect = stInValue|stInMultiline;      // Continue the multiline value     // Indented deeper and NOT a some nested section        // NOTE: Have to store prev rec at begining of a new rec to have full len
         else Expect &= ~stExpMultiline;      // Not a multiline value            
      }
     if((Expect & stExpSecBeg) && (fc == '['))       // Note: '[' and ']' are not part of the section name and in case of section detetion must be found again
      {
       Expect     = stExpSecEnd;   
       FirstNonWS = FirstWS = -1;    // Reset to get the section name later
      }
     else if((Expect & stExpSecEnd) && (fc == ']'))    // Skipping leading and trailing whitespaces   // It is possible to a change section`s name by adding any or a comment char to "disable" it
      {       
       DstOffs    = AppendRecord(DstOffs, PrvRecOffs, SrcData, PrvRecDLen, FirstNonWS-PrvRecOffs, LstVlLnCnt, (EType)PrvRecType);    // Store some previous record (or emptiness)
       PrvRecType = etSection;           // Init the section record
       PrvRecOffs = (uint)FirstNonWS;     
       PrvRecDLen = FirstWS-FirstNonWS;  
       Expect     = ExpctInlCmt;         // Only comments allowed at the same line with a section name
      }
     else if((Expect & stExpNVSep) && ((fc == CfgMrkNVS)||(fc == IniMrkNVS)))    // Take anything from beginning of the line as a Name   // Valyes can be unnamed
      {
       if(fc == CfgMrkNVS)CtrCfgNVS++;
        else CtrIniNVS++;
       DstOffs    = AppendRecord(DstOffs, PrvRecOffs, SrcData, PrvRecDLen, FirstNonWS-PrvRecOffs, LstVlLnCnt, (EType)PrvRecType);    // Store some previous record (or emptiness)
       PrvRecType = etName;                  // Init the name record
       PrvRecOffs = (uint)FirstNonWS;     
       PrvRecDLen = (FirstWS >= 0)?(FirstWS-FirstNonWS):(Offs-FirstNonWS);    // May be no whitespace between a name and its value  
       Expect     = stInValue|ExpctInlCmt;  // Values can be empty but records for them MUST be added anyway  // Inline comments will terminate multiline values
       FirstNonWS = -1;  // Reset to get a value without leading whitespaces
      }
     else if((Expect & stExpCmnt) && ((fc == CfgMrkCMN)||(fc == IniMrkCMN)))   // Comments start from last marker   // Marker is part of a comment string(first char) to be able to add/remove it easily  (Comment size is always > 0)
      {
       if(UseFirstMrk)   // Some CMNT marker was already encountered
        {
         if(CtrIniCMN && (fc != IniMrkCMN)){FirstWS = -1; continue;}
         if(CtrCfgCMN && (fc != CfgMrkCMN)){FirstWS = -1; continue;}
        }
       if(fc == CfgMrkCMN)CtrCfgCMN++;
         else CtrIniCMN++;
       LastCmntOffs = Offs;  // Pointing to CMNT marker to be able to remove the comment entirely
       ValEndOffs   = (FirstWS >= 0)?FirstWS:Offs;  // End of the value
      }
     FirstWS = -1;   // Reset WS index
    }
   else 
    {
     uint eol = CheckLineBreak(CurPtr); 
     if(FirstWS < 0)FirstWS = Offs;    // First line is empty or a line with no whitespaces
     if(eol)
      {
       if(eol > 1)CtrIniEOL++;
         else CtrCfgEOL++;
       if(Expect & stExpSecEnd)return -Offs;  // ERR: Section names cannot span multiple lines
       if(Expect & stInValue)
        {
         if(!(Expect & stInMultiline))   // Single/First line
          {
           DstOffs    = AppendRecord(DstOffs, PrvRecOffs, SrcData, PrvRecDLen, FirstNonWS-PrvRecOffs, LstVlLnCnt, (EType)PrvRecType);    // Store previous name record   
           PrvRecType = etValue;           // Init the section record
           PrvRecOffs = (uint)FirstNonWS;  
           LstVlLnCnt = 1;   // Start counting value lines
           if(LastCmntOffs < 0)  // No inline comments (May be multiline)   // Any leading or trailing whitespaces not considered to be a part of the value but preserved as they were in the file
            {  
             LstNameInd = LastCtrLbWS;
             PrvRecDLen = FirstWS-FirstNonWS; 
             Expect |= stExpMultiline;
            }
             else 
              {
               PrvRecDLen = ValEndOffs-FirstNonWS; 
               DstOffs    = AppendRecord(DstOffs, PrvRecOffs, SrcData, PrvRecDLen, LastCmntOffs-PrvRecOffs, LstVlLnCnt, (EType)PrvRecType);   // Store the value immediately (Its end is known here)
               PrvRecOffs = LastCmntOffs;    // To avoid writing it again below (before adding a comment)          
              }
          }
           else    // Continuing multiline
            {
             Expect    |= stExpMultiline;
             PrvRecDLen = FirstWS - PrvRecOffs;
             LstVlLnCnt++;
            }
         FirstWS = -1;     
        }
       if(LastCmntOffs >= 0)   // Comments end with EOL, any whitesspace becomes part of the comment
        {
         if(LastCmntOffs > PrvRecOffs)DstOffs = AppendRecord(DstOffs, PrvRecOffs, SrcData, PrvRecDLen, LastCmntOffs-PrvRecOffs, LstVlLnCnt, (EType)PrvRecType);    // Store some previous record (or emptiness)
         PrvRecType = etComment;             // Init the comment record
         PrvRecOffs = (uint)LastCmntOffs;      
         PrvRecDLen = Offs - LastCmntOffs;   // Comments end with EOL
         FirstWS    = -1;      // Next line won`t be part of the comment  
        }
       Offs        += eol  - 1;
       LastLineOffs = Offs + 1;
       LastCtrLbWS  = CtrLbWS;
       FirstNonWS   = LastCmntOffs = CtrLbWS = -1;    // Comments cannot span multiple lines
       Expect       = (Expect & stExpMultiline)|stExpSecBeg|stExpNVSep|stExpCmnt;     // Reset, preserve only stExpMultiline
      }
    }
  }
 
 uint EndOffs = AppendRecord(DstOffs, PrvRecOffs, SrcData, PrvRecDLen, SrcSize-PrvRecOffs, LstVlLnCnt, (EType)PrvRecType);   // Store last record
 this->LastRecSize = EndOffs - DstOffs; 
 *(SRecHdr::T*)&this->GetBufPtr()[EndOffs] = NoRecMrk;   // No more records marker

 if(CtrCfgEOL > CtrIniEOL)this->DefEOL = CtrCfgEOL;
  else if(CtrCfgEOL < CtrIniEOL)this->DefEOL = CtrIniEOL;
//    else this->DefEOL = MrkEOL;    // Leave as it is

 if(CtrCfgCMN > CtrIniCMN)this->DefCMN = CtrCfgCMN;
  else if(CtrCfgCMN < CtrIniCMN)this->DefCMN = CtrIniCMN;
//    else this->DefCMN = MrkCMN;    // Leave as it is

 if(CtrCfgNVS > CtrIniNVS)this->DefNVS = CtrCfgNVS;
  else if(CtrCfgNVS < CtrIniNVS)this->DefNVS = CtrIniNVS;
//    else this->DefNVS = MrkNVS;   // Leave as it is

 this->SetChanged(false);
 return SrcSize;   // Number of bytes parsed
}
//----------------------------------------------------------------------------
// NOTE: Complicated here so that record enumeration would be faster because all garbage put in tails instead of separate records
uint InsertNewRecord(SRecHdr* PrvRec, const achar* TextA, const achar* TextB, uint SizeA, uint SizeB, uint ExtraA, uint ExtraB, EType type)    
{
 if(type == etValue) 
  {
   if(TextA && SizeA)NormalizeTextForRec(TextA, SizeA, etName);    
   if(TextB && SizeB)NormalizeTextForRec(TextB, SizeB, etValue);   
  }
  else if(TextA && SizeA)NormalizeTextForRec(TextA, SizeA, type);    // etValue

 uint EolLen  = this->GetSizeOfEOL();
 uint NRLast  = sizeof(SRecHdr) + SizeA;    // last new record data size
 uint NRMdLen = 0;
 uint NRLenAl = 0;     // For a name record
 uint NIndLen = 0;
 uint NValSpc = 0;
 uint SecNSpc = 0;
 uint CmnIndt = 0;
 uint CmnSpac = 0;
 uint ExtPrev = 0;     // Will contain size of SRecHdr if there is no PrvRec
 uint PrvSize = 0;     // Size of data that is kept by a prev record
 uint TailLen = 0;     // The tail to move to the end of a new record from a prev record
 uint PrefLen = 0;     // Size for a prev record (After DSize)
 uint PostLen = 0;     // Size for a new record
 uint NewBLen = 0;     // Size for a terminating record if the buffer is empty 
 uint PSlkSpc = 0;     // Extra alignment space in a prev record
 uint CtrValN = 0;
 bool NoSSpcB = false;

 uint8* BPtr = this->GetBufPtr();
 if(!BPtr)NewBLen = sizeof(SRecHdr);  // Need one for a terminating record // Need a first record, even if empty
 uint PrevRecOffs, AtOffs;
 if(PrvRec)    // PrvRec should be null only when BPtr is null too   // PrvRec can fit some prefix data in its moved+alignment space (Data after a NL in PrvRec is moved to end of a new record)
  {
   if(PrvRec->Type == etName)PrvRec = PrvRec->Next();   // The tail is in etValue
   achar* DPtr = PrvRec->Data;
   achar* cptr = &DPtr[PrvRec->FSize-1];
   achar* eptr = &DPtr[PrvRec->DSize];
   for(;cptr >= eptr;cptr--){if(*cptr == CfgMrkEOL)break;}  // Find first '\n' // In tail range
   cptr++;
   AtOffs   = cptr - (achar*)BPtr;         // Where new text should be written
   TailLen += &DPtr[PrvRec->FSize] - cptr;     // This text must be added to tail of a new record
   PrvSize += cptr - DPtr;    // This text must be kept where it is   // This is the offset at which bytes of PrefLen must be written to
   PrevRecOffs = (uint8*)PrvRec - BPtr;
   PSlkSpc = ((achar*)PrvRec + PrvRec->ASize) - &DPtr[PrvRec->FSize];  // Extra space
   if(PrvSize /*&& ((PrvRec->Type == type) || (type == etSection) || (PrvRec->Type == etSection))*/)     // An attempt to save formatting of extra new lines
    {
     achar* cur = cptr;      // After the sequence of EOLs
     achar* prv = nullptr;   // Prev EOL
     achar* mid = nullptr;   // One empty line is usually intended
     for(cptr -= 2;cptr >= eptr;cptr--)   // Find first '\n' in the sequence
      {
       achar val = *cptr;
       if(val == CfgMrkEOL){mid = prv; prv = cptr + 1;}
       else if(val != '\r')break;   // Allow only '\r' additionally, just in case
      } 
     if(mid)prv = mid;
     NoSSpcB = (bool)prv;    // No empty line before is needed
     bool ForceSl = false;
     if((type == etComment) && SizeB){prv = cptr + 1; ForceSl = true;}    // Same line comment, steal all EOLs
     if(prv && (((mid || (PrvRec->Type != etSection)) && !(!mid && (type == etSection) && (this->Format & efSecBeforeNL))) || ForceSl))    // First () is for efSecAfterNL, last () is for efSecBeforeNL   // NOTE!!!: Inline comments will confuse all checks of PrvRec->Type  // TODO: use only EOL count somehow
      {
       uint ext = cur - prv;      // Move extra EOLs to the tail 
       AtOffs  -= ext;
       TailLen += ext;
       PrvSize -= ext;
      }
    }
  }
   else {ExtPrev += sizeof(SRecHdr); PrevRecOffs = AtOffs = 0;}   // No actual data in a prev rec (No PrvSize or TailLen)  (No records in the buffer)      

 if(type == etValue)    // Name+Value   // SizeA is SizeOfName; SizeB is SizeOfValue, ExtraA is name indent, ExtraB is max name size to format all '=' 
  {
   uint ExtraValLen = CalcMultilineValueInfo(TextB, SizeB, ExtraA+MLineValIndnt, &CtrValN); 
   PrefLen += ExtraA;       // Name indent
   PostLen  = 1;   // 1 for '='  
   if(this->Format & efNameValSpacing){NValSpc = NameValSpaces * 2; PostLen += NValSpc;}  // ' = '
   if((this->Format & efNameValAlign) && (SizeA < ExtraB)){NIndLen = ExtraB - SizeA; PostLen += NIndLen;}    // Alignment space  // ExtraB is max indent of '='
   NRMdLen  = SizeA + PostLen;     // For FSize of the name record
   NRLenAl += AlignP2Frwd(sizeof(SRecHdr) + NRMdLen, sizeof(SRecHdr));    // NRLast is size of name + header
   NRLast   = sizeof(SRecHdr) + SizeB + ExtraValLen;    // Value is last
   PostLen  = EolLen; 
  }
 else if(type == etSection)    
  {
   if(this->Format & efSecAfterNL)PostLen += EolLen;   // TODO: FIX!!!! Will add extra NL if the section is last!!!!!
   if(PrvRec && !NoSSpcB && (this->Format & efSecBeforeNL))PrefLen += EolLen;   // NL before a section if it is not a first one     // FIX: Will add extra NL even if there is one already from efSecAfterNL !!!!!!!!!!!
   PrefLen += ExtraA + 1;   // 1 for [     // ExtraA is section indent
   PostLen += EolLen + 1;   // 1 for ]
   NRLast  += ExtraB;       // The section nesting
   if(this->Format & efSecSpacedNames){SecNSpc = SecNameSpaces; PrefLen += SecNameSpaces; PostLen += SecNameSpaces;}
  }
 else if(type == etComment) 
  {
   PrefLen += 1;  // 1 for Cmnt marker
   CmnIndt += ExtraB?ExtraB:((SizeB)?CmntInlSpaces:0); // At least 1 indent
   if(!PrvRec || !SizeB)PostLen += EolLen;       // First record or a new line  (If on same line then should steal EOL from a record above)  // SizeB is a SameLine flag
   if(this->Format & efCmntSpacing){CmnSpac = CmntValSpaces; PrefLen += CmntValSpaces;}    // After ';'
   if(!SizeB && (this->Format & efCmntIndenting))CmnIndt += ExtraA;     // Keep the block indent if required
   PrefLen += CmnIndt;
  }
 else  // Empty lines (Adding to a prev record)
  {
   PrefLen += ExtraA * EolLen;
   PrefLen += TailLen;      // Tail stays where it is  
   TailLen  = NRLast = 0;   // No body
  }

 uint AlgPrvLen = AlignP2Frwd(ExtPrev+PrvSize+PrefLen, sizeof(SRecHdr));       // No need to allocate Bytes for PrvSize, only to align with them 
 uint AlgLstLen = AlignP2Frwd(NRLast+TailLen+PostLen, sizeof(SRecHdr));        // Accounts for alignment WITH TailLen but does not allocates space for TailLen   // Name for Name/Value must be already aligned
 uint SizeToAdd = (AlgPrvLen + AlgLstLen + NRLenAl + NewBLen) - (PrvSize + TailLen + PSlkSpc);
 this->Data.Insert(nullptr, SizeToAdd, AtOffs);          // Inserting right before and old tail

 BPtr = this->GetBufPtr();      // Refresh the pointer
 PrvRec = (SRecHdr*)&BPtr[PrevRecOffs];
 if(!AtOffs)  // Fill in a new prv record
  {  
   PrvRec->ASize = AlgPrvLen;   // PrvSize was 0
   PrvRec->DSize = 0;  
   PrvRec->FSize = PrefLen;  
   PrvRec->Type  = etNone;   
  }
   else  // Update prev rec
    {
     PrvRec->ASize = AlgPrvLen + sizeof(SRecHdr);  // ExtPrev was 0
     PrvRec->FSize = PrvSize + PrefLen;
     if(TailLen)memcpy((uint8*)PrvRec + PrvRec->ASize + NRLenAl + NRLast + PostLen, &PrvRec->Data[PrvSize], TailLen);   // Move old tail // Must be done before NewRec header
    }
 SRecHdr* NewRec = (SRecHdr*)((uint8*)PrvRec + PrvRec->ASize);    // Name if this is Name/Value
 SRecHdr* RetRec = NewRec;    // For Name/Value should return the Name rec
 if(type == etValue)
  {
   achar* DPtr = &PrvRec->Data[PrvSize];
   for(achar* EndPtr=&DPtr[ExtraA];DPtr < EndPtr;DPtr++)*DPtr = IndentChar; 

   NewRec->ASize = NRLenAl;   
   NewRec->DSize = SizeA;  
   NewRec->FSize = NRMdLen;  
   NewRec->Flags = 0;
   NewRec->Type  = etName; 
   DPtr = NewRec->Data;
   memcpy(DPtr, TextA, SizeA);
   NValSpc >>= 1;   // Div 2 (Same amount before and after)
   DPtr += SizeA;
   for(achar* EndPtr=&DPtr[NIndLen+NValSpc];DPtr < EndPtr;DPtr++)*DPtr = IndentChar;
   *DPtr = DefNVS; DPtr++;
   for(achar* EndPtr=&DPtr[NValSpc];DPtr < EndPtr;DPtr++)*DPtr = IndentChar;   // Spaces after '='

   NewRec = (SRecHdr*)((uint8*)NewRec + NRLenAl);
   NewRec->ASize = AlgLstLen;   
   NewRec->DSize = NRLast - sizeof(SRecHdr);  
   NewRec->FSize = NewRec->DSize + PostLen + TailLen;
   NewRec->Flags = efNone;  
   NewRec->Type  = etValue; 
   DPtr = NewRec->Data;
   if(CtrValN){DPtr = CopyTextAsMultiline(DPtr, TextB, SizeB, ExtraA+MLineValIndnt); NewRec->Flags |= efMultiline;}
     else {memcpy(DPtr, TextB, SizeB); DPtr += SizeB;}
   DPtr = this->WriteEOL(DPtr);   // Write EOLs before the tail  // Tail is already moved there
  }
 else if(type == etSection) 
  {
   achar* DPtr = &PrvRec->Data[PrvSize];
   if(AtOffs && !NoSSpcB &&(this->Format & efSecBeforeNL))DPtr = this->WriteEOL(DPtr);  // Exlra new line before the section is requested
   for(achar* EndPtr=&DPtr[ExtraA];DPtr < EndPtr;DPtr++)*DPtr = IndentChar; 
   *DPtr = '['; DPtr++;
   for(achar* EndPtr=&DPtr[SecNSpc];DPtr < EndPtr;DPtr++)*DPtr = IndentChar; 

   NewRec->ASize = AlgLstLen;   
   NewRec->DSize = SizeA + ExtraB;  
   NewRec->FSize = NewRec->DSize + PostLen + TailLen;
   NewRec->Flags = 0;
   NewRec->Type  = etSection; 
   DPtr  = NewRec->Data;
   for(achar* EndPtr=&DPtr[ExtraB];DPtr < EndPtr;DPtr++)*DPtr = SecNestMrk;                    
   memcpy(DPtr, TextA, SizeA);
   DPtr += SizeA;
   for(achar* EndPtr=&DPtr[SecNSpc];DPtr < EndPtr;DPtr++)*DPtr = IndentChar; 
   *DPtr = ']';
   DPtr  = this->WriteEOL(++DPtr);   // Write EOLs before the tail   // Tail is already moved there
   if(this->Format & efSecAfterNL)this->WriteEOL(DPtr);
  }
 else if(type == etComment) 
  {
   achar* DPtr = &PrvRec->Data[PrvSize];
   for(achar* EndPtr=&DPtr[CmnIndt];DPtr < EndPtr;DPtr++)*DPtr = IndentChar; 
   *DPtr = DefCMN; DPtr++;
   for(achar* EndPtr=&DPtr[CmnSpac];DPtr < EndPtr;DPtr++)*DPtr = IndentChar; 

   NewRec->ASize = AlgLstLen;   
   NewRec->DSize = SizeA;  
   NewRec->FSize = SizeA + PostLen + TailLen;  
   NewRec->Flags = 0;
   NewRec->Type  = etComment; 
   DPtr = NewRec->Data;
   memcpy(DPtr, TextA, SizeA);
   if(!AtOffs || !SizeB)this->WriteEOL(&DPtr[SizeA]);    // Tail is already moved there   // Write EOL if not first rec or at the same line
  }
 else  // Empty lines
  {
   achar* DPtr = &PrvRec->Data[PrvSize];
   for(uint ctr=0;ctr < ExtraA;ctr++)DPtr = this->WriteEOL(DPtr);
   RetRec = NewRec = PrvRec;
  }

 uint8* NextPtr = (uint8*)NewRec + NewRec->ASize;
 if(NewBLen)*(uint64*)NextPtr = NoRecMrk;  // Adding a terminating rec
 if(*(uint64*)NextPtr == NoRecMrk)this->LastRecSize = NewRec->ASize;      // Must be also updated when deleting
 this->SetChanged(true);
 return (uint8*)RetRec - BPtr;
}
//----------------------------------------------------------------------------
static void NormalizeTextForRec(const achar*& Txt, uint& Len, uint type)
{
 const achar* TxtPtr = Txt;
 uint TxtLen = Len;
 if(type & (etName|etValue|etSection))  // No leading whitespaces allowed 
  {
   for(const achar* EndPtr=&TxtPtr[TxtLen];TxtPtr < EndPtr;TxtPtr++,TxtLen--)
    if(*TxtPtr > 0x20)break;
  }
 if(type & (etName|etValue|etSection))  // No trailing whitespaces allowed      // For values too? For now the Parser does not considers trailing WS as a part of value 
  {
   for(const achar* EndPtr=TxtPtr,*CurPtr=&TxtPtr[TxtLen-1];CurPtr >= EndPtr;CurPtr--,TxtLen--)
    if(*CurPtr > 0x20)break;
  }
 Txt = TxtPtr;
 Len = TxtLen;
}
//----------------------------------------------------------------------------
static achar* CopyTextAsMultiline(achar* Dst, const achar* Txt, uint Len, uint Indent)
{
 uint LenLstNL = 0;
 for(const achar* EndPtr=&Txt[Len],*CurPtr=Txt,*LstPtr=Txt;;CurPtr++)
  {
   bool EndOfData = CurPtr >= EndPtr;
   if(EndOfData || (*CurPtr == '\n'))   // Starting from a second line spaces mean indent and preserved but not considered as part of the line`s value   // First: CTR+Text;  Next: CTR+NL+Indent+Text
    {
     uint DOffs = 2; 
     uint LenCurNL;
     uint LineLen = CurPtr - LstPtr;     
     if(!EndOfData){LenCurNL = (CurPtr[-1] == '\r')?2:1; LineLen -= (LenCurNL-1);}   // Need to exclude EOL, it will be part of a next line
       else LenCurNL = 0;    
     uint FullLen = LineLen; 
     if(LenLstNL)    // Not the first line
      {
       if(LenLstNL > 1)Dst[DOffs++] = '\r';
       Dst[DOffs++] = '\n';
       for(uint EndOffs=DOffs+Indent;DOffs < EndOffs;DOffs++)Dst[DOffs] = IndentChar;
       FullLen += LenLstNL + Indent;
      }
     Dst[0] = FullLen >> 8; // Hi 
     Dst[1] = FullLen;      // Lo
     memmove(&Dst[DOffs], LstPtr, LineLen);
     Dst += DOffs + LineLen;
     if(EndOfData)break;
     LstPtr = CurPtr + 1;
     LenLstNL = LenCurNL;
    }
  }
 return Dst;
}
//----------------------------------------------------------------------------
static uint CalcMultilineValueInfo(const achar* Txt, uint Len, uint Indent, uint* NLCount=nullptr)   // Returns number of extra bytes needed to store line sizes and indents    // Must use line sizes (uint16) instead of line breaks ('\n' or '\r\n')
{
 uint CtrValNL = 0;
 uint ExtraValLen = 0;
 for(const achar* EndPtr=&Txt[Len],*CurPtr=Txt;CurPtr < EndPtr;CurPtr++)     // NL+Indent chars will be in beginning of each line except first one
   if(*CurPtr == '\n')CtrValNL++;
 if(CtrValNL)ExtraValLen  += ((sizeof(uint16)+Indent) * CtrValNL) + sizeof(uint16);  
 if(NLCount)*NLCount = CtrValNL;
 return ExtraValLen;
}
//----------------------------------------------------------------------------
sint EditRecTextAt(uint RecOffs, uint PrvRecOffs, const achar* NewTxt, uint TxtSize=0)     // Shrinked records won`t be moved, only their sizes will be updated     // TODO: thread spaces depending on a record type. also process multiline values
{
 if(!TxtSize && NewTxt)TxtSize = NSTR::StrLen(NewTxt);
 SRecHdr* Rec = (SRecHdr*)&this->GetBufPtr()[RecOffs];
 uint ExtraValLen  = 0;
 uint CtrValNL     = 0;
 uint Indnt        = 0;
 if(TxtSize && NewTxt)
  {
   NormalizeTextForRec(NewTxt, TxtSize, Rec->Type);
   if(Rec->Type & etValue)
    {                                         
     SRecHdr* PrvRec = (SRecHdr*)&this->GetBufPtr()[PrvRecOffs];   //this->GetPrevRec((SRecHdr*)&this->GetBufPtr()[PrvRecOffs]);   // For Value rec PrvRecOffs points to a rec before the Name rec
     Indnt = (PrvRec?GetRecIndent(PrvRec):0) + MLineValIndnt;
     ExtraValLen = CalcMultilineValueInfo(NewTxt, TxtSize, Indnt, &CtrValNL);   
    }
  }
 uint RecDLen = Rec->DSize;
 uint RecFLen = Rec->FSize;
 uint RecALen = Rec->ASize;  // Should include header size   // Now does
 uint TailLen = RecFLen - RecDLen;
 uint DataLen = TxtSize + ExtraValLen;
 uint NewALen = AlignP2Frwd(TailLen + DataLen, sizeof(SRecHdr));      // New text may fit and no memmove will be required
 if(NewALen > RecALen)       // Record grows
  {
   bool LstRec = IsLastRecHdr(Rec);
   uint ExLen  = NewALen - RecALen;    
   this->Data.Insert(nullptr, ExLen, RecOffs + Rec->ASize);   // Grow the array (Insert at the record`s end) 
   if(LstRec)this->LastRecSize = NewALen;
   Rec = (SRecHdr*)&this->GetBufPtr()[RecOffs];   // Refresh the pointer
   Rec->ASize  = NewALen;
  }
 Rec->Flags &= ~efMultiline;
 if(RecDLen != DataLen)memmove(&Rec->Data[DataLen], &Rec->Data[RecDLen], TailLen);  // Move tail data if the text is not of same size
 if(TxtSize && NewTxt)
  {
   if(CtrValNL){CopyTextAsMultiline(Rec->Data, NewTxt, TxtSize, Indnt); Rec->Flags |= efMultiline;}
     else memmove(&Rec->Data, NewTxt, TxtSize); 
  }
 Rec->DSize = DataLen;
 Rec->FSize = DataLen + TailLen;
 this->SetChanged(true);
 return DataLen;  
} 
//----------------------------------------------------------------------------
SRecHdr* GetRecByIndex(SRecHdr* Rec, sint Index, uint TypeMsk, bool BrkOnNonseq=false)
{
 SRecHdr* prv = Rec;
 if(!Index)        // From the same rec
  {
   if(prv->Type & TypeMsk)return prv;
     else return nullptr;
  }
 uint idx = (Index < 0)?(-Index):Index;  // Starts from +1/-1
 for(uint ctr = 1;ctr <= idx;)
  {
   SRecHdr* hdr = (Index < 0)?(this->GetPrevRec(prv)):(this->GetNextRec(prv));
   if(!hdr)break;
   if(hdr->Type & TypeMsk)
    {  
     if(ctr == idx)return hdr;
     ctr++;
    }
     else if(BrkOnNonseq)return nullptr;   // Records are not in sequense 
   prv = hdr;    
  }
 return nullptr;
}
//----------------------------------------------------------------------------
void SetChanged(bool val){if(this->GetBufLen())this->GetBufPtr()[this->GetBufLen()] = val;}   // Use ONLY after insert! (Not saved, outside of array in extra bytes)
//----------------------------------------------------------------------------

public:
CMiniIni(void){}
//----------------------------------------------------------------------------
~CMiniIni(){}
//----------------------------------------------------------------------------
bool IsChanged(void){return this->GetBufLen() && this->GetBufPtr()[this->GetBufLen()];}
//----------------------------------------------------------------------------
sint Load(const achar* FilePath)  // TODO: Support for UTF-16 INI files (Convert entire file?)   // NOTE: No need to skip BOM marker of UTF-8 files - it will be ignored and stored in an unused record
{
 CArr<achar> Src;
 sint res = Src.FromFile(FilePath);
 if(res < 0)return res;
 return this->Load(Src);
}
//----------------------------------------------------------------------------
sint Load(const CArr<achar>& SrcData)
{
 if(SrcData.Size() < MinIniLen)return PX::ENODATA;
 uint SrcSize = SrcData.Size();
 this->Data.SetSize(SrcSize + (SrcSize / 2) + 256);  // Preallocate one half of initial size for metadata  // Extra izes to consider: Rec headers, alignments, multiline counters
 SrcData.Data()[SrcSize] = 0;   // CArr always have extra sizeof(size_t) at its end and a terminating 0 is reqiured for ParseIniData
 return this->ParseIniData(SrcData.Data(), SrcSize);
}
//----------------------------------------------------------------------------
sint Save(CArr<achar>& DstData)
{
 if(this->GetBufLen() <= sizeof(SRecHdr))return 0;  // No data
 uint FullSize = 0;
 for(SRecHdr* rec = (SRecHdr*)this->GetBufPtr();;rec=NextRecHdr(rec))FullSize += rec->TextSize();    // TODO: Multiline line size
 DstData.SetSize(FullSize);    // Must set exact known size     // this->RawSize
 SRecHdr* rec = (SRecHdr*)this->GetBufPtr(); 
 achar*  DPtr = DstData.Data();
 uint TotalWr = 0;            
 for(;;rec=NextRecHdr(rec))
  {  
   if(*(SRecHdr::T*)rec == NoRecMrk)break; 
   uint DoneSize = 0;
   if(rec->Flags & efMultiline)
    {
     const achar* PrvLine = nullptr;
     for(;;)
      {
       uint linelen = 0;  
       PrvLine = rec->NextLine(PrvLine, &linelen);
       if(!PrvLine)break;
       memcpy(DPtr, PrvLine, linelen);    // Save with NLs and indents
       DPtr += linelen;
      }
     DoneSize = rec->DSize;   
    }
   uint reclen = rec->FSize - DoneSize;   
   memcpy(DPtr, &rec->Data[DoneSize], reclen);
   DPtr += reclen;  
  }
 this->SetChanged(false);
 return TotalWr;
}
//----------------------------------------------------------------------------
sint Save(const achar* FilePath) 
{
 achar outbuf[4096];
 if(this->GetBufLen() <= sizeof(SRecHdr))return 0;  // No data
 int df = NPTM::NAPI::open(FilePath,PX::O_CREAT|PX::O_WRONLY|PX::O_TRUNC, 0666);   // O_TRUNC  O_EXCL   // 0666
 if(df < 0){ DBGERR("Error: Failed to create the output data file(%i): %s!",df,FilePath); return (sint)df; }
 SRecHdr* rec = (SRecHdr*)this->GetBufPtr(); 
 uint TotalWr = 0;  
 uint BufOffs = 0;        
 for(;;rec=NextRecHdr(rec))
  {  
   if(*(SRecHdr::T*)rec == NoRecMrk)break;
   if(rec->Flags & efMultiline)
    {
     const achar* PrvLine = nullptr;
     for(;;)                     
      {
       uint linelen = 0;  
       PrvLine = rec->NextLine(PrvLine, &linelen);
       if(!PrvLine)break;
       BufOffs = BufferedWriteFile(PrvLine, linelen, &outbuf, sizeof(outbuf), BufOffs, TotalWr, df, FilePath);    // Save the line with NLs and indents
      }
     BufOffs = BufferedWriteFile(&rec->Data[rec->DSize], rec->FSize - rec->DSize, &outbuf, sizeof(outbuf), BufOffs, TotalWr, df, FilePath);  // Write rest of the text
    }
     else BufOffs = BufferedWriteFile(&rec->Data, rec->FSize, &outbuf, sizeof(outbuf), BufOffs, TotalWr, df, FilePath); 
  }
 BufferedWriteFile(nullptr, sizeof(outbuf), &outbuf, sizeof(outbuf), BufOffs, TotalWr, df, FilePath);    // Flush the buffer
 NPTM::NAPI::close(df);
 this->SetChanged(false);
 return TotalWr;
}
//----------------------------------------------------------------------------
bool ChangeRecord(SBaseRec* Rec, const achar* txt, uint len=0)    // NOTE: Only changes name of SValRec Name+Value pair
{
 if(!Rec->IsValid())return false;
 this->EditRecTextAt(Rec->Offset, Rec->PrvOffs, txt, len);      // SBaseRec Should not poin to etValue itself
 return true;
}
//----------------------------------------------------------------------------
bool ChangeValue(SValRec* Rec, const achar* txt, uint len=0)
{
 if(!Rec->IsValid())return false;
 SRecHdr* hdr = Rec->GetThisRec();
 uint voffs = Rec->Offset + hdr->ASize;  // Offset of associated etValue
 this->EditRecTextAt(voffs, Rec->PrvOffs, txt, len); 
 return true;
} 
//----------------------------------------------------------------------------
const achar* GetComment(SBaseRec* Rec, sint cmntidx, uint* ValLen)   // index is only in sequence of comments. Any noncomment record will stop the search   // NOTE: No checks if the Rec belongs to another Ini instance
{
 if(!Rec->IsValid())return nullptr;
 SRecHdr* hdr = this->GetRecByIndex(Rec->GetThisRec(), cmntidx, etComment, true);    // Keep enumerating nonsequent comments?
 if(!hdr)return nullptr;
 if(ValLen)*ValLen = hdr->DSize;
 return hdr->Data;
}
//----------------------------------------------------------------------------
const SCmtRec SetComment(SBaseRec* Rec, sint cmntidx, const achar* src, uint srcsize=0)       // TODO: Iterators of meaningful recs (By type mask)
{
 if(!Rec->IsValid())return SCmtRec{};
 SRecHdr* hdr = this->GetRecByIndex(Rec->GetThisRec(), cmntidx, etComment, true);    // Keep enumerating nonsequent comments?
 if(!hdr)return SCmtRec{};
 uint RecOffs = (uint8*)hdr - this->GetBufPtr();
 this->EditRecTextAt(RecOffs, 0, src, srcsize);  
 hdr = (SRecHdr*)&this->GetBufPtr()[RecOffs];   // Update the pointer
 return SCmtRec{this,(uint32)RecOffs,(uint32)-1};
}
//----------------------------------------------------------------------------
const SBaseRec NextRecord(const SBaseRec* Rec, uint TypeMsk)             // Disabled and Nesting level are ignored    // Those two ... (VS lost this comment)
{
 if(!Rec->IsValid())return SBaseRec{};
 uint8*  BPtr = this->GetBufPtr();
 SRecHdr* prv = Rec->GetThisRec();
 for(;;)
  {
   SRecHdr* hdr = this->GetNextRec(prv);
   if(!hdr)break;
   if(hdr->Type & TypeMsk)return SBaseRec{this, (uint32)((uint8*)hdr - BPtr), (uint32)((uint8*)prv - BPtr)};
   prv = hdr; 
  }
 return SBaseRec{};
}
//----------------------------------------------------------------------------
const SBaseRec PrevRecord(const SBaseRec* Rec, uint TypeMsk) 
{
 if(!Rec->IsValid())return SBaseRec{};
 uint8*  BPtr = this->GetBufPtr();
 SRecHdr* prv = Rec->GetThisRec();
 for(;;)
  {
   SRecHdr* hdr = this->GetPrevRec(prv);
   if(!hdr)break;
   if(hdr->Type & TypeMsk)return SBaseRec{this, (uint32)((uint8*)hdr - BPtr), (uint32)((uint8*)prv - BPtr)};
   prv = hdr;
  }
 return SBaseRec{};
}
//----------------------------------------------------------------------------
const SValRec _finline NextValue(const SBaseRec* Rec) {SBaseRec val = this->NextRecord(Rec, etName); return SValRec{val.Owner,val.Offset,val.PrvOffs};}       // SValRec points to Name rec
const SSecRec _finline NextSection(const SBaseRec* Rec) {SBaseRec val = this->NextRecord(Rec, etSection); return SSecRec{val.Owner,val.Offset,val.PrvOffs};}
const SCmtRec _finline NextComment(const SBaseRec* Rec) {SBaseRec val = this->NextRecord(Rec, etComment); return SCmtRec{val.Owner,val.Offset,val.PrvOffs};}

const SValRec _finline PrevValue(const SBaseRec* Rec) {SBaseRec val =  this->PrevRecord(Rec, etName); return SValRec{val.Owner,val.Offset,val.PrvOffs};}      // SValRec points to Name rec
const SSecRec _finline PrevSection(const SBaseRec* Rec) {SBaseRec val = this->PrevRecord(Rec, etSection); return SSecRec{val.Owner,val.Offset,val.PrvOffs};}
const SCmtRec _finline PrevComment(const SBaseRec* Rec) {SBaseRec val = this->PrevRecord(Rec, etComment); return SCmtRec{val.Owner,val.Offset,val.PrvOffs};}
//----------------------------------------------------------------------------
void AddEmptyLines(uint num=1, SBaseRec* BaseRec=nullptr)
{
 if(!num)return;
 SRecHdr* prec;
 if(BaseRec)prec = BaseRec->GetThisRec();
  else prec = (SRecHdr*)this->GetLastRecHdr();   // Indent is unknown because PrevRec is unknown
 this->InsertNewRecord(prec, nullptr, nullptr, 0, 0, num, 0, etNone);
}
//----------------------------------------------------------------------------
SCmtRec AddComment(const achar* txt, uint len=0, SBaseRec* BaseRec=nullptr, bool SameLine=false, uint indents=0)    // Adds after the BaseRec or at the same line with it
{
 sint Indent = 0;    // May be manually indented
 SRecHdr* prec;
 if(!len && txt)len = NSTR::StrLen(txt);
 if(BaseRec)
  { 
   prec   = BaseRec->GetThisRec();
   Indent = GetRecIndent(BaseRec);  // From a prev rec
  }
   else 
    {
     prec = (SRecHdr*)this->GetLastRecHdr();   // Indent is unknown because PrevRec is unknown
     if(prec && (this->Format & efCmntIndenting))
      if(SRecHdr* rec=FindPrevRec((uint8*)prec, this->GetBufPtr());rec)
       {
        if(rec->Type == etName)rec = FindPrevRec((uint8*)rec, this->GetBufPtr());
        if(rec)Indent = GetRecIndent(rec);   // Try to avoid requirement of passing BaseRec to insert an indented comments
       }
    }
 if(Indent < 0)Indent = 0;
 if(SameLine && !(this->Flags & efAllowInlineCmnt))SameLine = false;
 uint PrvOffs = (uint8*)prec - this->GetBufPtr();
 uint ROffs   = this->InsertNewRecord(prec, txt, nullptr, len, SameLine, Indent, indents, etComment);  
 return SCmtRec{this, (uint32)ROffs, (uint32)PrvOffs};
}
//----------------------------------------------------------------------------
SSecRec AddSection(const achar* txt, uint len=0, SSecRec* BaseSec=nullptr)  
{
 sint NstLvl = -1;   // Current nesting level (Root)
 sint Indent = 0;    // May be manually indented
 SRecHdr* rec;
 SRecHdr* prec;
 if(!len && txt)len = NSTR::StrLen(txt);
 if(BaseSec)
  {
   prec = BaseSec->GetThisRec();
   GetSecInfoFromName(prec, ++NstLvl, 0);
   if(this->Format & efSecIndentNested)Indent = GetRecIndent(BaseSec) + NestSecIndent;   // One char deeper from a parent
   rec  = BaseSec->GetNextRec();
  }
   else {rec = nullptr; prec = (SRecHdr*)this->GetLastRecHdr();}   
 if(rec)
  {
   for(;;prec=rec,rec=NextRecHdr(rec))  // Find last place
    {  
     if(*(SRecHdr::T*)rec == NoRecMrk)break;  // No more records
     if(rec->Type != etSection)continue;      // Ignore anything but sections (Adding as last, no insertion is supported yet)
     sint Nestl = 0;
     GetSecInfoFromName(rec, Nestl, 0);
     if(Nestl <= NstLvl)break;   // This is a parent or a sibling section (Never breaks if BaseSec is NULL)
    }
  }
 uint PrvOffs = (uint8*)prec - this->GetBufPtr();
 uint ROffs   = this->InsertNewRecord(prec, txt, nullptr, len, 0, Indent, NstLvl+1, etSection);   
 return SSecRec{this, (uint32)ROffs, (uint32)PrvOffs};
}
//----------------------------------------------------------------------------
SValRec AddValue(const achar* name, const achar* value, uint vlen=0, uint nlen=0, SSecRec* BaseSec=nullptr)   // Values should be added at section indent
{
 sint Indent = 0;    // May be manually indented
 sint MaxNamLen = 0; 
 SRecHdr* rec;
 SRecHdr* prec;
 if(!nlen && name)nlen = NSTR::StrLen(name);
 if(!vlen && value)vlen = NSTR::StrLen(value);
 if(BaseSec)
  { 
   Indent = GetRecIndent(BaseSec);
   prec   = BaseSec->GetThisRec();
   rec    = BaseSec->GetNextRec();
  }
   else {rec = nullptr; prec = (SRecHdr*)this->GetLastRecHdr();} 
 if(rec)
  {
   for(;;prec=rec,rec=NextRecHdr(rec))  // Find last place in the section (Adding as last, no insertion is upported yet)
    {  
     if(*(SRecHdr::T*)rec == NoRecMrk)break;  // No more records
     if(rec->Type == etSection)break;         // Don`t go into other sections
     if(rec->Type == etName)
      {
       if(rec->DSize > MaxNamLen)MaxNamLen = rec->DSize;
      }
    }
  }
 uint PrvOffs = (uint8*)prec - this->GetBufPtr();
 uint ROffs   = this->InsertNewRecord(prec, name, value, nlen, vlen, Indent, MaxNamLen, etValue);  
 return SValRec{this, (uint32)ROffs, (uint32)PrvOffs};
}
//----------------------------------------------------------------------------
SSecRec GetSection(const achar* name, SSecRec* From=nullptr, uint len=0) 
{
 SSecRec sec = this->FindSection(From, name, len);
 if(sec.IsValid())return sec;
 return this->AddSection(name, len, From);
}
//----------------------------------------------------------------------------
// These two are slow and inefficient
const achar* GetValue(const achar* SecName, const achar* ValName, const achar* DefVal, uint* ValLen, SSecRec* BaseSec=nullptr, uint LineIdx=0, uint DefValLen=0, uint SecNamLen=0, uint ValNamLen=0)   // No nested sections support, only Root/BaseSec
{
 SValRec  rec = this->GetValue(SecName, ValName, DefVal, BaseSec, DefValLen, SecNamLen, ValNamLen); 
 SRecHdr* ptr = rec.GetThisRec();
 uint  StrLen = 0;
 const achar* str  = ptr->NextLine(nullptr, &StrLen);  // NOTE: Only first line of a multiline value 
 if(ValLen)*ValLen = StrLen;    
 return str; 
}
//----------------------------------------------------------------------------
SValRec GetValue(const achar* SecName, const achar* ValName, const achar* DefVal, SSecRec* BaseSec=nullptr, uint DefValLen=0, uint SecNamLen=0, uint ValNamLen=0)   // No nested sections support, only Root/BaseSec  // NOTE: No multiline retrieving here
{
 SSecRec sec = this->FindSection(BaseSec, SecName, SecNamLen);
 if(!sec.IsValid())sec = this->AddSection(SecName, SecNamLen, BaseSec);
 SValRec rec = this->FindValue(&sec, ValName, ValNamLen);
 if(!rec.IsValid())rec = this->AddValue(ValName, DefVal, DefValLen, ValNamLen, &sec);
 return rec; 
}
//----------------------------------------------------------------------------
SValRec SetValue(const achar* name, const achar* value, SSecRec* BaseSec=nullptr, uint vlen=0, uint nlen=0)
{
 SValRec rec = this->FindValue(BaseSec, name, nlen);
 if(rec.IsValid())
  {
   this->ChangeValue(&rec, value, vlen);
   return rec;
  }
 return this->AddValue(name, value, vlen, nlen, BaseSec);
} 
//----------------------------------------------------------------------------
const achar* FindValue(SBaseRec* From, const achar* ValName, uint* ValLen, uint ValNamLen=0, uint MatchIdx=0)    // NOTE: No multiline retrieving here
{
 SValRec  rec = this->FindValue(From, ValName, ValNamLen, MatchIdx);
 if(!rec.IsValid())return nullptr;
 SRecHdr* ptr = rec.GetThisRec();
 uint  StrLen = 0;
 const achar* str  = ptr->NextLine(nullptr, &StrLen);  // NOTE: Only first line of a multiline value 
 if(ValLen)*ValLen = StrLen;    
 return str;  
}
//----------------------------------------------------------------------------
SValRec FindValue(SBaseRec* From, const achar* ValName, uint ValNamLen=0, uint MatchIdx=0)
{
 uint MatchCtr = 0;
 uint8* BPtr = this->GetBufPtr();
 SRecHdr* rec, *prec;
 if(From && From->IsValid())
  {
   prec = (SRecHdr*)&BPtr[From->Offset];
   rec  = NextRecHdr(prec);
  }
   else prec = rec = (SRecHdr*)BPtr;

 if(!ValNamLen && ValName)ValNamLen = NSTR::StrLen(ValName);    // Allowed non 0-terminated strings
 bool Unnamed  = !ValNamLen || !ValName || !*ValName;   
 bool CaseSens = (this->Flags & efNameCaseSensVal);         
 for(;;prec=rec,rec=NextRecHdr(rec))
  {  
   if(*(SRecHdr::T*)rec == NoRecMrk)break;
   if(rec->Type == etSection)break;     // Another section begins
   if(rec->Type != etName)continue;
   achar* NPtr = (achar*)&rec->Data;
   uint nsize  = rec->DSize;
   uint noffs  = 0; 
   bool Disbl  = (*NPtr == CfgMrkCMN) || (*NPtr == IniMrkCMN);    // Disabled
   noffs += Disbl; 
   if(Disbl && !(this->Flags & efUseDisabledVal))continue;     // Skip a disabled value 
   if(Unnamed)return SValRec{this, uint32((uint8*)rec - BPtr), uint32((uint8*)prec - BPtr)};     // Return just a next sibling section
   if(nsize != ValNamLen)continue;
   if((CaseSens && NSTR::IsStrEqualCS(&NPtr[noffs], ValName, nsize)) || NSTR::IsStrEqualCI(&NPtr[noffs], ValName, nsize))
    {
     if(MatchCtr >= MatchIdx)return SValRec{this, uint32((uint8*)rec - BPtr), uint32((uint8*)prec - BPtr)};   // Section match found
     MatchCtr++;
    }
  }
 return SValRec{};
}
//----------------------------------------------------------------------------
SSecRec FindSection(SBaseRec* From, const achar* SecName, uint SecNamLen=0, uint MatchIdx=0)   // Without IgnoreNLvl it should check only sections that are at same nesting level as a first encountered section or the specified section (If SBaseRec is a section)
{ 
 sint CurNestLvl = -1;
 uint MatchCtr = 0;
 uint8* BPtr = this->GetBufPtr();
 if(!BPtr)return SSecRec{};
 SRecHdr* rec = (SRecHdr*)BPtr;
 SRecHdr* prec = rec;
 bool RspSecNst = !(this->Flags & efIgnoreSecNesting);
 if(From)
  {
   prec = (SRecHdr*)&BPtr[From->Offset];
   if(RspSecNst && (prec->Type == etSection)){GetSecInfoFromName(prec, ++CurNestLvl, 0); CurNestLvl++;}   // It may be disabled
   rec = NextRecHdr(prec);
  }
 if(!SecNamLen && SecName)SecNamLen = NSTR::StrLen(SecName);    // Allowed non 0-terminated strings
 bool Unnamed  = !SecNamLen || !SecName || !*SecName; 
 bool CaseSens = (this->Flags & efNameCaseSensSec);       
 for(;;prec=rec,rec=NextRecHdr(rec))
  {  
   if(*(SRecHdr::T*)rec == NoRecMrk)break;
   if(rec->Type != etSection)continue;    // Ignore anything but sections
   sint CmnCtr = 0;
   sint Nestl  = 0;
   uint noffs  = GetSecInfoFromName(rec, Nestl, CmnCtr);
   if(CurNestLvl >= 0)   // 'From' is a section
    {
     if(RspSecNst)
      {
       if(Nestl < CurNestLvl)return SSecRec{};  // No more sibling sections Return invalid section
       if(Nestl > CurNestLvl)continue;          // Do not go into child sections      
      }
    }
     else CurNestLvl = Nestl;        // Lock to this nesting level from now on
   if((CmnCtr > 0) && !(this->Flags & efUseDisabledSec))continue;     // Skip a disabled section   (Check AFTER nesting level checks)
   if(Unnamed)return SSecRec{this, uint32((uint8*)rec - BPtr), uint32((uint8*)prec - BPtr)};     // Just return a next sibling section
   uint nsize = rec->DSize - noffs;  // Skipping any markers (Disabled, nesting)
   if(nsize  != SecNamLen)continue;
   if((CaseSens && NSTR::IsStrEqualCS(&rec->Data[noffs], SecName, nsize)) || NSTR::IsStrEqualCI(&rec->Data[noffs], SecName, nsize))
    {
     if(MatchCtr >= MatchIdx)return SSecRec{this, uint32((uint8*)rec - BPtr), uint32((uint8*)prec - BPtr)};   // Section match found
     MatchCtr++;
    }
  }
 return SSecRec{};
}
//----------------------------------------------------------------------------
SSecRec _finline FindSection(const achar* SecName, uint SecNamLen=0, uint MatchIdx=0)
{
 return this->FindSection(nullptr, SecName, SecNamLen, MatchIdx);
}
//----------------------------------------------------------------------------
bool RemoveRec(SBaseRec* Rec)
{
 if(!Rec->IsValid())return false;
 SRecHdr* CurHdr = (SRecHdr*)&this->GetBufPtr()[Rec->Offset];
 SRecHdr* PrvHdr = (SRecHdr*)&this->GetBufPtr()[Rec->PrvOffs];
 if((uint8*)CurHdr == this->GetBufPtr())return false;  // Should not happen!
 uint PrvOffs = PrvHdr->FSize;
 if(CurHdr->Type == etSection)   // Remove the section and all its values
  {
   uint DelSize = CurHdr->ASize;
   sint BaseNstLvl = 0;
   GetSecInfoFromName(CurHdr, BaseNstLvl, 0);
   for(SRecHdr* rec=CurHdr;;)  // Find last place
    {  
     rec = NextRecHdr(rec);
     if(*(SRecHdr::T*)rec == NoRecMrk)break;  // No more records
     if(rec->Type == etSection)      // Deleting any records inside
      {
       sint Nestl = 0;
       GetSecInfoFromName(rec, Nestl, 0);
       if(Nestl <= BaseNstLvl)break;   // This is a parent or a sibling section 
      }
     DelSize += rec->ASize;
     CurHdr   = rec;      // If it is last then its tail will be required
    }
   PrvHdr->ASize += DelSize;
   for(achar* EndPtr=&PrvHdr->Data[PrvHdr->DSize],*CurPtr=&PrvHdr->Data[PrvOffs-1];CurPtr > EndPtr;CurPtr--,PrvOffs--)
     if(*CurPtr == CfgMrkEOL)break;       // Remove '[' and any indenting
   if(CurHdr->Type == etSection)   // Remove ']'
    {
     for(achar* EndPtr = &CurHdr->Data[CurHdr->FSize],*CurPtr=&CurHdr->Data[CurHdr->DSize];CurPtr < EndPtr;CurPtr++,CurHdr->DSize++)
       if(*CurPtr == ']'){CurHdr->DSize++; break;}      
    }
  }
 else if(CurHdr->Type == etName)   // Remeve Name+Value recs
  {
   SRecHdr* NxtHdr = NextRecHdr(CurHdr);
   PrvHdr->ASize  += CurHdr->ASize + NxtHdr->ASize;
   CurHdr = NxtHdr;
  }
 else PrvHdr->ASize += CurHdr->ASize;     // Just expand prev rec, no deallocation is made
 uint TailLen = CurHdr->FSize - CurHdr->DSize;
 memmove(&PrvHdr->Data[PrvOffs], &CurHdr->Data[CurHdr->DSize], TailLen);   // Add the tail
 PrvHdr->FSize = PrvOffs + TailLen;
 if(IsLastRecHdr(PrvHdr))this->LastRecSize = PrvHdr->ASize;   // It became a last record
 this->SetChanged(true);
 return true;
}
//----------------------------------------------------------------------------
bool RemoveSection(SSecRec* BaseSec, const achar* SecName, uint SecNamLen=0)
{
 SSecRec sec = this->FindSection(BaseSec, SecName, SecNamLen);
 if(!sec.IsValid())return false;
 return this->RemoveRec(&sec);
}
//----------------------------------------------------------------------------
bool RemoveValue(const achar* SecName, const achar* ValName, SSecRec* BaseSec=nullptr, uint SecNamLen=0, uint ValNamLen=0)
{
 SSecRec sec = this->FindSection(BaseSec, SecName, SecNamLen);
 if(!sec.IsValid())return false;
 SValRec rec = this->FindValue(&sec, ValName, ValNamLen);
 if(!rec.IsValid())return false;
 return this->RemoveRec(&rec);
}
//----------------------------------------------------------------------------
// TODO: Remove comment ???

};

using CIni = CMiniIni;
//----------------------------------------------------------------------------
