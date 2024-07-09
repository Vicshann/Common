

// Predefined lexical templates must be added first(Especially for scopes) to get an optimal tokenization

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


 Any UTF-8 code that is <= 0x20 is a whitespace (should be any nonprintable char in UNICODE32)   // Unicode code points use max 21 bit (11 bits are always zero)

 First you register whitespace ranges in UTF-32 encoding
 Anything else will be split by those whitespaces and added to stream of tokens
 Also currently accumulated token is checked (after each new added char) in lexer against all registered templates
 i.e. you have 'as' in current token and lexer have 'as':0 and 'asterisk':1 registered
    in this case tokenizer continues expecting 'as' to be 'asterisk' because it have higher priority than 'as'
    if it turns out to be 'ask' then 'as' will be stored as a token
   an ordinary sorting by size is enough here, if 'as' had higher priority then 'asterisk' would never be recognized
NOTE: It is very inefficient to split tokens after tokenization is done so the Tokenizer must use Lexer for splitting, beside ranges 
Ranges:
   Any ranges can be registered with a index(tag)
   Char from one range group will break token with chars of another range group (Several ranges can have same Group id)
   A range can be flagged to be ignored(whitespaces)
   UTF-8 is read char by char, converting to UTF32 and comparing with ranges
   Should unregistered ranges be whitespaces, invalids or as ordinary letters, used for identifiers?
   Ranges should be sorted by starting number(?) (Order would be: 00-20, 21-?? --- identifiers(letters) would be too far in the list) - Better to sort by group id to keep close something like ("{|}")
   Need an arbitrary subgroup id?
   Letters 'A' - 'Z' and 'a' - 'z' are two ranges with same group id.
   Chars from 0x21 to 0x2F may or may not be allowed to be in one range. May have individual joining properties and subgroup can already be assigned to some lexical meaning
   Need an optimized way to find a range for a char
   Looks like we cannot just split by different group id. We can have letters, and numbers, which need to have separate group ids but we can have identifiers like 'Hello35', but '32hello' in c++ is an invalid identifier
   Need to specify somehow can it split by left, right or both when placed beside a different group, and xor with same flags of that group
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
 uint16 SrcPos;  // Position on a line                        // https://stackoverflow.com/questions/36668466/bitfields-and-alignment
};

enum ERngType
{
  rtNormal     = 0, // Ordinary(letters for identifiers)
  rtInvalid    = 0   // Invalid(break parsing)
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


struct SStateRec   // Expected/Unexpected
{
 uint32 StateClr;    // Do first in all states
 uint32 StateSet;
};

/*
 Parsing 0xA3FD45 which is a hex number:
 0 is a gtDigit, which also adds gtNumHex(0x2C, 0X2C), gtNumOctal(043), gtNumBinary(0b101010)
 x is a gtName and tolerable to gtDigit. Removes gtNumOctal and gtNumBinary
 A is a gtName but all gtName is tolerable to gtNumHex
*/
/*struct SRange
{
 uint8  First;     // Sorted by this for fast search     // A note for 'struct-of-arrays' lovers(like Odin?). How cache friendly it would be to read 'First' from one memory page and then 'Last' from another?
 uint8  Last;
 uint8  Group;
 uint8  Flags; 
 uint32 Expected;
 uint32 UnExpected;
  */
/* uint16 StateClr;  // Order: 0
 uint16 StateSet;  // Order: 1
 uint16 StateXor;  // Order: 2   // This is needed for scopes that defined by same char, like strings ( '"' )

 uint16 SplitOn;   // Split if any of these states are set
 uint16 KeepOn;    // But keep anyway if any of these are set

 uint16 Group;     // Priority
 uint8  Flags;     // IncDepth, DecDepth, IncCounter, DecCounter, LowCaseASCII(Useful for hex numbers)
 uint8  Type;      // ERngType
 uint8  Ctr;       // Index of a counter (Max 256 counters) // Required to trace scope closing overflows (Closing brace without opening, or that there are unclosed braces left)
 uint8  Tag;   */
//};

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
  lfPairBeg = 0x04,   // i.e. '(' or '/*'
  lfPairEnd = 0x08,   // i.e. ')' or '*/'
  lfUnused  = 0x10,   // Unused as space or some word to ignore
};

//------------------------------------------------------------------------------------------------------------
SCVR uint MaxScope    = 0xFF;
SCVR uint MaxStrNum   = 0xFFFF;  // MAX_UINT16
SCVR uint MaxLexNum   = 0xFFFF;
SCVR uint MaxStates   = 32;      // For uint32
SCVR uint MaxTokenLen = 2048;

static inline SLOC* LS = &LocNone;       // Should it be global, in APP state?

//------------------------------------------------------------------------------------------------------------
struct STokenLst           // Parsed from a file
{
 enum EFlg {flNone=0, flStorePos=0x01};

 CArr<STkn> Tokens;
 CArr<SPos> Positions;
 uint32 LastScope = 0;   // 0 is the root scope
 uint32 Flags = flNone;

void Add(uint16 StrIdx, sint LexIdx, sint ExtIdx, uint8 slflg, uint32 AbsPos, uint16 SrcLin, uint16 SrcPos)
{
 STkn tkn;
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
}

};
//============================================================================================================
// Used mostly to gather number constants, including complex ones, like floats
// Not useful for scope counting because scopes can be defined by a simple words like 'begin' and 'end' in Pascal (Use Lexer for that)
//
struct SRangeLst
{
 SCVR uint MaxRanges = 256;   // Max for all states
 enum EFlags {rfNone  = 0, 
           rfUnkIsErr = 0x0001,   // Treat unspecified chars as an error instead of fallback to state 0
 };

struct SState
{
 uint8 RangeMap[MaxRanges];   // 0 is for an empty slot (max 255 ranges)  // Range indexes for chars  // Max 256 ranges (Meaning that we can declare each char separatedly)
};

struct SRange
{
 uint8  First;    
 uint8  Last;
 uint8  Group;
 uint8  State;
 uint16 Type;    // For lexer (Low part)
 uint16 Flags;
};

 CArr<SRange> RangeArr;    // TODO: Optimize allocations
 CArr<SState> StateArr; 
//------------------------------------------------------
sint Add(SRange& rec, uint8 TgtState)      // Returns range index(Can change after next add because of sorting)
{
 if(this->RangeArr.Count() >= (MaxRanges-1))return -1;
 sint res = this->RangeArr.Append(&rec);
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
     SRange* Rng = &Ranges[ridx-1];
     if(rec.Group <= Rng->Group)              // A higher group will always overwrite
      {
       if(rec.First < Rng->First)continue;    // Do not overwrite ranges with intersecting borders
       if(rec.Last  > Rng->Last )continue; 
      }
    }
   RangeMap[idx] = res + 1;   // This slot is free
  }
 return res;
}
//------------------------------------------------------
SRange* Get(uint8 chr, uint8 State)  // Returns true if this char should start a new token
{
// if(State >= this->StateArr.Count())return nullptr;     // Slow but safe?
 uint8* RangeMap = this->StateArr.Data()[State].RangeMap;
 return &this->RangeArr.Data()[RangeMap[chr] - 1];
}
//------------------------------------------------------
};
//============================================================================================================
SRangeLst Ranges;     
STokenLst Tokens;

//------------------------------------------------------------------------------------------------------------
// Type is flags that accumulated by 'Parse' loop for current token
//
sint AddRange(uint8 TgtState, uint8 First, uint8 Last, uint8 NextState=0, uint8 Group=0, uint16 Type=0, uint16 Flags=0)
{
 SRangeLst::SRange rng{.First=First, .Last=Last, .Group=Group, .State=NextState, .Type=Type, .Flags=Flags};
 return this->Ranges.Add(rng, TgtState);
}
//------------------------------------------------------------------------------------------------------------
sint Parse(const achar* Data, uint Size)
{
 uint8 TokenBuf[MaxTokenLen];
 const achar* EndPtr = &Data[Size];
 const achar* CurPtr = Data;
 const achar* CurTkn = CurPtr;
 uint TknOffs = 0;
 uint8 State  = 0;        // Initial state is always 0
 for(;CurPtr < EndPtr;CurPtr++)        // Skip BOM if any?
  {
   uint8 Val = *CurPtr;       // Have to decode UTF-8 to know exact token position on a line if there are some UTF-8 chars on it
   SRangeLst::SRange* Rng = this->Ranges.Get(Val, State);


//   States &= ~Rng->StateClr;
   // Split if zero?
//   States |= Rng->StateSet;
//   States ^= Rng->StateXor;
   // Split if zero?
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