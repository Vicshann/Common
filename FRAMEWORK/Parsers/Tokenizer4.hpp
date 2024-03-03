
/*
                             -------- --------
                           / |File  | |Extra |
                          /  |Token |+|Info  | File1
                         /   |Stream| |Stream|
                        /    -------- --------
                       /
 --------    ---------       -------- --------
 |String|    |Lixical|       |File  | |Extra |
 |      |----|       | ----- |Token |+|Info  | File2
 |Intern|    |List   |       |Stream| |Stream|
 --------    ---------       -------- --------
                       \
                        \    -------- --------
                         \   |File  | |Extra |
                          \  |Token |+|Info  | FileN
                           \ |Stream| |Stream|
                             -------- --------

 https://en.wikipedia.org/wiki/Argument-dependent_name_lookup
 https://probablydance.com/2015/02/16/ideas-for-a-programming-language-part-3-no-shadow-worlds/
 https://www.utf8-chartable.de/
 https://en.wikipedia.org/wiki/List_of_Unicode_characters
 https://en.wikipedia.org/wiki/UTF-32
 https://en.wikipedia.org/wiki/Plane_(Unicode)#Basic_Multilingual_Plane
*/

struct CTokenizer
{

struct STkn      // There will be great amount of this records, preferable in a contiguous memory for CPU cache sake
{
 uint16 StrIdx;  // Index of the token SStr in CStrIn
 uint16 Index;   // Index of a lexical template (SLex)  // 0 - undefined   // Required for scope tokens to produce a valid sequence  // If stays 0 when executing - report the UnknownToken error
 uint16 Extra;   // Extra info index (uint64 index, arbitrary sized records) (Scope size, target from, or jump to) For a first token - Total token records in this scope (To skip to a next scope without enumeration) (Includes subscopes) else - Scope depth (Controlled by scoping tokens (Must be defined first))   // May be too small (namespaces) // Is there are reason to skip a scope?
 uint8  Scope;   // 256 should be enough
 uint8  Flags;   // ??? (i.e. to disable the token)  //ScopeFirst, ScopeLast
};

struct SPos      // Optional. Associated with stream of STkn, must have same indexes  // Indeed, part of 'struct-of-arrays' concept but only because it may be optional and useful only for error reporting or an IDE  // Text related info for error reporting  (Move to optional array?)   // Moved out of STkn to make it more compack in cache (altough having some cache misses when writing but that happens only once)
{
 uint32 AbsPos;  // Useful for debugging
 uint16 SrcLin;  // Line of a source file      // One token stream per file
 uint16 SrcPos;  // Position on the line                        // https://stackoverflow.com/questions/36668466/bitfields-and-alignment
};

/*
enum ERngType
{
  rtNormal     = 0, // Ordinary(letters for identifiers)
  rtInvalid    = 0  // Invalid(break parsing)
                    // NewLine(need to report or count it)
                    // JoinLine(ignore next newline, like '\' in c for macros. NL is still counted)
                    // Whitespace(ignore)
};

// When a group or a single token is added it is specified to which groups it is tolerable (left size). if it is not, then it starts a new token
enum ERangeState  // The groups are arbitrary and defined here for convenience only   // This is kept as current state   // Token separation will depend on which groups is allowed in which current state
{
  rsName     = 0x0001,      // Any letter have this
  rsDigit    = 0x0002,      // 0-9 have this
  rsNumHex   = 0x0004,      // 'x'
  rsNumOct   = 0x0008,      
  rsNumBin   = 0x0010,      // 'b'
  rsComment  = 0x0020,      // Must skip tokenization of comments to avoid a lot of problems
  rsMLCmnt   = 0x0040,
  rsString   = 0x0080,      // Must skip tokenization of string contents to avoid a lot of problems [???]
  rsRawStr   = 0x0100,      // No escapes are processed
};

enum ETknFlg {
  tfNone     = 0,
  tfBaseLex  = 0x01,   // The Index is in BaseLex
  tfHaveLex  = 0x02,   // Have a resolved Lex at Index
  tfHaveExt  = 0x04,
  tfScopeBeg = 0x10,
  tfScopeEnd = 0x20,
};

enum ELexFlg {
  lfNone    = 0,
  lfSepBeg  = 0x01,   // Starts a new token
  lfSepEnd  = 0x02,   // Ends a token   // i.e. scope start/end tokens '(' and ')' should have lfDBeg+lfDEnd
  lfPairBeg = 0x04,   // i.e. '(' or '/$'
  lfPairEnd = 0x08,   // i.e. ')' or '$/'
  lfUnused  = 0x10,   // Unused as space or some word to ignore
};
*/
//------------------------------------------------------------------------------------------------------------
enum EParseFlags
{
 pfNone,
 pfKeepWS,       // Keep whitespace ranges as tokens
 pfTknStrings,   // Tokenize strings (Spams the Lexer)
 pfTknComments,  // Tokenize comments (Spams the Lexer)
};

enum ETLErrors
{
 tleBadChar=1,  // The char is declared inappropriate at this place
 tleUnexpChar,  // The char is in unregistered range
 tleUnexpEOF,   // Unexpected end of file (Incomplete token)
};

struct SErrCtx
{
 uint  Pos; 
 uint  Line; 
 uint  State;
 uint  Offset;
 uint8 Value;
}; 
//------------------------------------------------------------------------------------------------------------
SCVR uint MaxScope    = 0xFF;
SCVR uint MaxStrNum   = 0xFFFF;  // MAX_UINT16
SCVR uint MaxLexNum   = 0xFFFF;
SCVR uint MaxStates   = 32;      // For uint32
SCVR uint MaxTokenLen = 2048;

static inline SLOC* LS = &LocNone;       // Should it be global, in APP state?

//============================================================================================================
struct STokenLst           // Parsed from a file
{
 enum EFlg {flNone=0, flStorePos=0x01};

 CArr<STkn> Tokens;
 CArr<SPos> Positions;
 uint32 LastScope = 0;   // 0 is the root scope
 uint32 Flags = flNone;

//------------------------------------------------------
void Add(uint16 StrIdx, sint LexIdx, sint ExtIdx, uint8 slflg, uint32 AbsPos, uint16 SrcLin, uint16 SrcPos)
{
/* STkn tkn;
 if(slflg & tfScopeBeg)this->LastScope++;
 if(this->LastScope > MaxScope){LOGERR(LSTR("Token scope too deep at %u:%u:%u"), AbsPos, SrcLin, SrcPos);}

 tkn.StrIdx = StrIdx;
 tkn.Scope  = (uint8)this->LastScope;
 tkn.Flags  = slflg;   // tfBaseLex, tfScopeBeg, tfScopeEnd
 if(LexIdx >= 0){tkn.Index = LexIdx; tkn.Flags |= tfHaveLex;}   // If < 0 then the token is detected by default separation algo, not by Lex template
   else tkn.Index = 0;
 if(ExtIdx >= 0){tkn.Extra = ExtIdx; tkn.Flags |= tfHaveExt;}
   else tkn.Extra = 0;

 if(slflg & tfScopeEnd)this->LastScope--;
 this->Tokens.Append(&tkn, 1);

 if(this->Flags & flStorePos)
  {
   SPos ps {.AbsPos=AbsPos, .SrcLin=SrcLin, .SrcPos=SrcPos};
   this->Positions.Append(&ps, 1);
  }
*/
}
//------------------------------------------------------
};
//============================================================================================================
// Used mostly to gather number constants, including complex ones, like floats
// Not useful for scope counting because scopes can be defined by a simple words like 'begin' and 'end' in Pascal (Use Lexer for that)
//
enum ERangeFlags {
  rfNone    = 0, 
  rfIgnore  = 0x01,   // Ignore the char range, like ' in numbers. Do not change the state?
  rfBadChar = 0x02,   // Treat unspecified chars as an error instead of fallback to state 0
};

struct SRangeLst
{
 SCVR uint MaxRanges = 256;   // Max for all states

struct SState
{
 uint8 RangeMap[MaxRanges];   // 0 is for an empty slot (max 255 ranges)  // Range indexes for chars  // Max 256 ranges (Meaning that we can declare each char separatedly)
};

struct SRange
{
 uint8  First;    
 uint8  Last;
 uint8  State;      // Max 256 states, 64K memory(Allocated by 4K)
 uint8  Group;      // Useful for Scopes (Max 256 scope types)
 uint32 Flags;      // By default all unassigned char ranges cause fallback to state 0. Invalid char ranges must be defined to break parsing if necessary 
 uint32 TypeSet;    // Set accumumulated token flags for the Lexer (Low part)
 uint32 TypeClr;    // Clear accumumulated token flags for the Lexer (Low part)
 
 bool IsValid(void){return (First|Last);}  // 0 char is invalid
};

 CArr<SRange> RangeArr;    // TODO: Optimize allocations
 CArr<SState> StateArr; 
//------------------------------------------------------
SRangeLst(void)
{
 SRangeLst::SRange rec{};
 this->RangeArr.Append(&rec);  // Add an empty zero-idx range which will mark entire range as unexpected chars 
}
//------------------------------------------------------
sint Add(SRange& rec, uint8 TgtState)      // Returns range index(Can change after next add because of sorting)
{
 if(this->RangeArr.Count() >= (MaxRanges-1))return -1;
 sint res = this->RangeArr.Count();
 this->RangeArr.Append(&rec);
 SRange* Ranges = this->RangeArr.Data();
 if(uint cnt=this->StateArr.Count();TgtState >= cnt)
  {
   uint num = (TgtState - cnt) + 1;
   this->StateArr.Append(nullptr, num);
   memset(&this->StateArr.Data()[cnt],0,num*sizeof(SState));
 }
 uint8* RangeMap = this->StateArr.Data()[TgtState].RangeMap;
 for(uint idx=rec.First;idx <= rec.Last;idx++)    // Add in array and update char map (Shorter ranges overlap larger ones)
  {   
   if(uint ridx = RangeMap[idx];ridx)   // Already belongs to some range   // We can write ranges inside of ranges but without overlapped borders
    {
     SRange* Rng = &Ranges[ridx];
     if(rec.First < Rng->First)continue;    // Do not overwrite ranges with intersecting borders
     if(rec.Last  > Rng->Last )continue;       
    }
   RangeMap[idx] = res;   // This slot is free
  }
 return res;
}
//------------------------------------------------------
SRange* Get(uint8 chr, uint8 State)  // Returns true if this char should start a new token
{
// if(State >= this->StateArr.Count())return nullptr;     // Slow but safe?
 uint8* RangeMap = this->StateArr.Data()[State].RangeMap;
 return &this->RangeArr.Data()[RangeMap[chr]];
}
//------------------------------------------------------
};
//============================================================================================================
SRangeLst Ranges;     
STokenLst Tokens;

//------------------------------------------------------------------------------------------------------------
// Type is flags that accumulated by 'Parse' loop for current token
//
sint AddRange(uint8 TgtState, uint8 First, uint8 Last, uint8 NextState, uint8 Group=0, uint32 TypeSet=0, uint32 TypeClr=0, uint32 Flags=0)
{
 SRangeLst::SRange rng{.First=First, .Last=Last, .State=NextState, .Group=Group, .Flags=Flags, .TypeSet=TypeSet, .TypeClr=TypeClr};
 return this->Ranges.Add(rng, TgtState);
}
//------------------------------------------------------------------------------------------------------------
// NOTE: ErrPos and ErrLine are counted from 0
//
sint Parse(const achar* Data, uint Size, uint Flags=0, SErrCtx* ErrCtx=nullptr)
{
 uint8 TokenBuf[MaxTokenLen];        // Allocate?

 const achar* EndPtr = &Data[Size];
 const achar* CurPtr = Data;
 const achar* CurTkn = CurPtr;
 uint   TknLine = 0;  // Line on which the current token is
 uint   TknLPos = 0;  // Position of current token on the line
 uint   TknSize = 0;
 uint   TknType = 0;  // Bit mask
 uint8  State   = 0;  // Initial state is always 0
 uint8  ChrLen  = 0;
 uint32 LineNum = 0;  // Line number
 uint32 LineNxt = 0;  // Next position on the line
 uint32 LinePos = 0;  // Position on the line
 for(;CurPtr < EndPtr;)        // Skip BOM if any?
  {
   uint8 Val = *CurPtr;   // Have to decode UTF-8 to know exact token position on a line if there are some UTF-8 chars on it     // TODO: No need to check for NULL char?
   if(Val == '\n')        // '\r' does not matter
    {
     LineNum++;
     LineNxt = 0;
     ChrLen  = 1;
    }
   else if(!ChrLen)
    {
     ChrLen = NUTF::GetCharSize(Val);
     LineNxt++;   // Next position on the line
    }
Repeat:       
   if(SRangeLst::SRange* Rng = this->Ranges.Get(Val, State);!Rng->IsValid())   // Fallback and split        // NOTE: It is possible to use ranges to parse some multibyte UTF-8 chars
    {
     if(!State)   // Nowhere to fallback, already in state 0 (All usable ASCII ranges must be defined in state 0)
      {
       if(ErrCtx){ErrCtx->Pos=LinePos; ErrCtx->Line=LineNum; ErrCtx->State=State; ErrCtx->Offset=CurPtr-Data; ErrCtx->Value=Val;}
       return -tleUnexpChar;
      }
     State = 0;
     if(TknSize)   // Skip whitespace tokens (Optionally)
      {
       DBGMSG("Token: '%.*s'",TknSize, &TokenBuf);
     //STokenLst::ADD
      }
     TknSize = TknType = 0;         // Reset the token
     TknLine = LineNum;
     TknLPos = LinePos;
     goto Repeat;    // Repeat from base state   // Would be useful to have in C++ ability to reevaluate a conditional scope from whithin (Like 'repeat')
    }
    else  // Change state or break if the range is marked as bad       // TODO: Gather range flags for lexer 
     {
      if(Rng->Flags & rfBadChar)    // This range in this state is marked as inappropriate at this point (Saves the Lexer from processing some bad number pieces like '0b1010120101;' which would be just split as '0b10101','20101;' otherwise - just add range 2-9 as bad for binary numbers, and 'a-z' too, probably)
       {
        if(ErrCtx){ErrCtx->Pos=LinePos; ErrCtx->Line=LineNum; ErrCtx->State=State; ErrCtx->Offset=CurPtr-Data; ErrCtx->Value=Val;}
        return -tleBadChar;
       }
       else 
        {
         State    = Rng->State;
         TknType &= ~Rng->TypeClr;  // First clear the flags
         TknType |= Rng->TypeSet;   // then set them
        }
      if(!(Rng->Flags & rfIgnore))TokenBuf[TknSize++] = Val;   // Accumulate a token 
     }  
   LinePos = LineNxt;
   CurPtr++;
   ChrLen--;
  }
 if(TknSize)  // Add a last token // Skip whitespace tokens (Optionally)     // Only if fallback is allowed at this point else report an unexpected EOF
  {
   DBGMSG("Last Token: '%.*s'",TknSize, &TokenBuf);
 //STokenLst::ADD
  }
 return CurPtr - Data;   // Return size of processed text
}
//------------------------------------------------------------------------------------------------------------

};

/*
  0  NUL (null)                      32  SPACE     64  @         96  `
  1  SOH (start of heading)          33  !         65  A         97  a
  2  STX (start of text)             34  "         66  B         98  b
  3  ETX (end of text)               35  #         67  C         99  c
  4  EOT (end of transmission)       36  $         68  D        100  d
  5  ENQ (enquiry)                   37  %         69  E        101  e
  6  ACK (acknowledge)               38  &         70  F        102  f
  7  BEL (bell)                      39  '         71  G        103  g
  8  BS  (backspace)                 40  (         72  H        104  h
  9  TAB (horizontal tab)            41  )         73  I        105  i
 10  LF  (NL line feed, new line)    42  *         74  J        106  j
 11  VT  (vertical tab)              43  +         75  K        107  k
 12  FF  (NP form feed, new page)    44  ,         76  L        108  l
 13  CR  (carriage return)           45  -         77  M        109  m
 14  SO  (shift out)                 46  .         78  N        110  n
 15  SI  (shift in)                  47  /         79  O        111  o
 16  DLE (data link escape)          48  0         80  P        112  p
 17  DC1 (device control 1)          49  1         81  Q        113  q
 18  DC2 (device control 2)          50  2         82  R        114  r
 19  DC3 (device control 3)          51  3         83  S        115  s
 20  DC4 (device control 4)          52  4         84  T        116  t
 21  NAK (negative acknowledge)      53  5         85  U        117  u
 22  SYN (synchronous idle)          54  6         86  V        118  v
 23  ETB (end of trans. block)       55  7         87  W        119  w
 24  CAN (cancel)                    56  8         88  X        120  x
 25  EM  (end of medium)             57  9         89  Y        121  y
 26  SUB (substitute)                58  :         90  Z        122  z
 27  ESC (escape)                    59  ;         91  [        123  {
 28  FS  (file separator)            60  <         92  \        124  |
 29  GS  (group separator)           61  =         93  ]        125  }
 30  RS  (record separator)          62  >         94  ^        126  ~
 31  US  (unit separator)            63  ?         95  _        127  DEL
 */