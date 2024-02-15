

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
enum EGroupType  // The groups are arbitrary and defined here for convenience only   // This is kept as current state   // Token separation will depend at which groups is allowed in which current state
{
  gtName     = 0x0001,   // Any name, containing gigits or letters
  gtDigit    = 0x0002,
  gtNumHex   = 0x0004,
  gtNumOct   = 0x0008,
  gtNumBin   = 0x0010,
  gtComment  = 0x0020,      // Must skip tokenization of comments to avoid a lot of problems
  gtMLCmnt   = 0x0040,
  gtString   = 0x0080,      // Must skip tokenization of string contents to avoid a lot of problems
  gtRawStr   = 0x0100,      // No escapes are processed
};

/*
 Parsing 0xA3FD45 which is a hex number:
 0 is a gtDigit, which also adds gtNumHex(0x2C, 0X2C), gtNumOctal(043), gtNumBinary(0b101010)
 x is a gtName and tolerable to gtDigit. Removes gtNumOctal and gtNumBinary
 A is a gtName but all gtName is tolerable to gtNumHex
*/
struct SRange
{
 uint32 First;     // Sorted by this for fast search     // A note for 'struct-of-arrays' lovers(like Odin?). How cache friendly it would be to read 'First' from one memory page and then 'Last' from another?
 uint32 Last;

 uint16 StateClr;  // Order: 0
 uint16 StateSet;  // Order: 1
 uint16 StateXor;  // Order: 2   // This is needed for scopes that defined by same char, like strings ( '"' )

 uint16 SplitOn;   // Split of any of these states are set
 uint16 KeepOn;    // But keep anyway if any of these are set

 uint16 Group;     // Useful?
 uint8  Flags;     // IncDepth, DecDepth, IncCounter, DecCounter, LowCaseASCII(Useful for hex numbers)
 uint8  Type;      // ERngType
 uint8  Ctr;       // Index of a counter (Max 256 counters) // Required to trace scope closing overflows (Closing brace without opening, or that there are unclosed braces left)
 uint8  Tag;
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
  lfPairBeg = 0x04,   // i.e. '(' or '/*'
  lfPairEnd = 0x08,   // i.e. ')' or '*/'
  lfUnused  = 0x10,   // Unused as space or some word to ignore
};

//------------------------------------------------------------------------------------------------------------
static const uint MaxScope  = 0xFF;
static const uint MaxStrNum = 0xFFFF;  // MAX_UINT16
static const uint MaxLexNum = 0xFFFF;

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
//------------------------------------------------------------------------------------------------------------
struct SRangeLst
{
 CArr<SRange> RangeArr;

//------------------------------------------------------
sint Add(SRange& rec)
{
 return 0;
}
//------------------------------------------------------
bool Match(uint32 val)  // Returns true if this char should start a new token
{
 return false;
}
//------------------------------------------------------
};
//------------------------------------------------------------------------------------------------------------
SRangeLst Ranges;
STokenLst Tokens;
//------------------------------------------------------------------------------------------------------------
sint AddRange(uint32 First, uint32 Last, uint16 StateClr=0, uint16 StateSet=0, uint16 StateXor=0, uint16 SplitOn=0, uint16 KeepOn=0, uint16 Group=0, uint8 Flags=0, uint8 Type=0, uint8 Ctr=0, uint8 Tag=0)
{
 SRange rng{.First=First, .Last=Last, .StateClr=StateClr, .StateSet=StateSet, .StateXor=StateXor, .SplitOn=SplitOn, .KeepOn=KeepOn, .Group=Group, .Flags=Flags, .Type=Type, .Ctr=Ctr, .Tag=Tag};
 //
 // Any checks
 //
 return this->Ranges.Add(rng);
}
//------------------------------------------------------------------------------------------------------------
sint Parse(const achar* Data, uint Size)
{
 const achar* EndPtr = &Data[Size];
 const achar* CurPtr = Data;
 const achar* CurTkn = CurPtr;
 while(CurPtr < EndPtr)
  {
   uint32 Val;
   CurPtr += NUTF::ChrUtf8To32(&Val, CurPtr);
   if(!Val)return -(CurPtr - Data);         // Unvalid UTF-8 code
   if(this->Ranges.Match(Val))   // How to ignore whitespaces or anytthing?
    {
    // this->Tokens.Add(???);
    }
  }
 return CurPtr - Data;   // Return size of processed text
}
//------------------------------------------------------------------------------------------------------------


};
