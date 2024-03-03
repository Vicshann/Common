
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

 NOTE: This is a context-free parser
 A complete AST building tool in a thousand lines of code, i hope:)
*/

struct CTokenizer
{

enum ETokenFlags {   // Tokenizer flags, not a token flags  // Token flags or range flags?   // Only 8 bits for token flags!   // This is for Range/Token records, not the token stream itself
  tfNone        = 0, 
  tfWhtspc      = 0x0001,   // This range is a whitespace
  tfIgnore      = 0x0002,   // Ignore the char range, like ' in numbers. Do not change the state?
  tfNoTerm      = 0x0004,   // The char range is a part of a token but cannot be last in it
  tfBadToken    = 0x0008,   // Treat unspecified chars as an error instead of fallback to state 0
  tfScopeOpn    = 0x0010,   // Both flags mean that same symbols wil open and close their scope (i.e.: ' " )   They will use a separate scope stack ???          //rfScopeOpn + rfScopeCse for counted scopes like ''' or '"'   // Note Do not allow scope close if it is not opened
  tfScopeCse    = 0x0020,   // Can we have nested scopes of same char?  NO!: "1 "2 "3 "?[We want to close the last one]    // So, the flat only: "O "C "O "C
  tfNumeric     = 0x0040,   // No lexing. Store in a separate list with associated struct of converted values to do conversion only once
  tfTknTerm     = 0x0080,   // This token terminates/separates an expression (i.e. ';')
  tfTknRSplit   = 0x0100,   // Split current token when encountered // Will become part of a next token, not the current one (Not same as tfTknTerm?)
  tfTknLSplit   = 0x0200,   // Split current token when encountered // Will become part of the current token
  tfRawString   = 0x0400,   // A string with no escape processing
  tfEscString   = 0x0800,   // An escaped string
  tfComment     = 0x1000,
  tfLSplitOnSDZ = 0x2000,   // Split if scope depth of GroupID becomes zero after tfScopeCse (Does tfTknLSplit)
  tfBRstIfSplit = 0x4000,   // Reset current state to base if split
  fScpTknMirror = 0x8000,   // This token is a mirrored when used as a scope marker (Store the token to compare on closing the scope)  // Format: [Prefix]anything[Postfix] Prefix/Postfix is controlled by ranges and 'anything' is stored on scope stack to compare on close   // C++: R "delimiter( raw_characters )delimiter"     // Delimiter is optional and it can be a character except () / or whitespaces
 // tfSLComment = 0x0400,   // A single-line comment string
 // tfMLComment = 0x0800,   // A multi-line comment string
};


struct STkn      // There will be great amount of this records, preferable in a contiguous memory for CPU cache sake
{
 uint16 StrIdx;  // Index of the token SStr in CStrIn
 uint16 Index;   // Index of a lexical template (SLex)  // 0 - undefined   // Required for scope tokens to produce a valid sequence  // If stays 0 when executing - report the UnknownToken error
 uint16 Extra;   // Extra info index (uint64 index, arbitrary sized records) (Scope size, target from, or jump to) For a first token - Total token records in this scope (To skip to a next scope without enumeration) (Includes subscopes) else - Scope depth (Controlled by scoping tokens (Must be defined first))   // May be too small (namespaces) // Is there are reason to skip a scope?
 uint8  Scope;   // 256 should be enough
 uint8  Flags;   // ??? (i.e. to disable the token)  //ScopeFirst, ScopeLast    // Created automatically from ETokenFlags
};

struct SPos      // Optional. Associated with stream of STkn, must have same indexes  // Indeed, part of 'struct-of-arrays' concept but only because it may be optional and useful only for error reporting or an IDE  // Text related info for error reporting  (Move to optional array?)   // Moved out of STkn to make it more compack in cache (altough having some cache misses when writing but that happens only once)
{
#ifdef _DEBUG
 uint32 Offset;  // Useful for debugging       // Ignore in release - takes too much memory
#endif
 uint16 Line;    // Line of a source file      // One token stream per file
 uint16 Pos;     // Position on the line       // NOTE: Limited to 64K text size if everything is in one line  // https://stackoverflow.com/questions/36668466/bitfields-and-alignment

_finline SPos(void){this->Set(0, 0, 0);}
_finline SPos(const SPos* Pos){this->Set(Pos);}
_finline SPos(uint Line, uint Pos, uint Offs){this->Set(Line, Pos, Offs);}

void _finline Set(const SPos* Pos){*this = *Pos;}
void _finline Set(uint line, uint pos, uint offs)
{
 this->Line   = (decltype(this->Line))line;
 this->Pos    = (decltype(this->Pos))pos;
#ifdef _DEBUG
 this->Offset = (decltype(this->Offset))offs;
#endif
}
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
enum EParseFlags              // Escape
{
 pfNone,
 pfKeepWhtspc  = 0x01,   // Keep whitespace ranges as tokens                // Not keeping those can save a lot of memory  // Whitespaces in strings and comments are always kept (when tokenizing them)
 pfKeepScpTkn  = 0x02,   // Keep scoping tokens (i.e. braces and brackets)  // Scoping tokens are useful only for printing. Each token record contains scope info   // Not keeping those can save a lot of memory
 pfKeepTrmTkn  = 0x04,   // Keep termination tokens (i.e. ';' in c++)       // Not keeping those can save some of memory
 pfTknStrings  = 0x08,   // Tokenize strings (Spams the Lexer)
 pfTknComments = 0x10,   // Tokenize comments (Spams the Lexer)
};

enum ETLErrors     // And warnings: tlw*
{
 tleBadChar=1,        // The char is declared inappropriate at this place
 tleUnexpChar,        // The char is in unregistered range
 tleUnexpEOF,         // Unexpected end of file (Incomplete token)
 tleScopeTooDeep,     // Max scope depth reached for a specfic scope type
 tleUnexpScpClse,     // Closing a scope that was not opened before                // Unexpected brace
 tleWrongScpClse,     // Closing a scope that was not opened at this depth level   // Wrong brace type
 tleUnclosedScpAtEOF, // There are some unclosed scopes at EOF
 tleChrCantBeLast,    // The char is forbidden to be last in the token
};

struct SErrCtx
{ 
 achar* Value;
 uint16 ValLen;
 uint8  State;
 uint8  Code;
 uint32 Extra;
 SPos   CurPos;
 SPos   PrvPos;  // For scopes

void Set(uint code, uint8 state, uint16 vlen, achar* val, SPos&& cpos, SPos&& ppos, uint32 extra=0)
{
 this->Code   = code;
 this->Extra  = extra;
 this->State  = state;
 this->Value  = val;
 this->ValLen = vlen;
 this->CurPos.Set(&cpos);
 this->PrvPos.Set(&ppos);
}

}; 
//------------------------------------------------------------------------------------------------------------
SCVR uint MaxScope    = 0xFF;
SCVR uint MaxStrNum   = 0xFFFF;  // MAX_UINT16
SCVR uint MaxLexNum   = 0xFFFF;
SCVR uint MaxStates   = 32;      // For uint32
SCVR uint MaxTokenLen = 2048;

//static inline SLOC* LS = &LocNone;       // Should it be global, in APP state?

//============================================================================================================
// Used mostly to gather number constants, including complex ones, like floats
// Not useful for scope counting because scopes can be defined by a simple words like 'begin' and 'end' in Pascal (Use Lexer for that)
//
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
 uint32 Flags;      // ETokenFlags  //By default all unassigned char ranges cause fallback to state 0. Invalid char ranges must be defined to break parsing if necessary 
 uint32 TypeSet;    // Set accumumulated token flags for the Lexer (Low part)
 uint32 TypeClr;    // Clear accumumulated token flags for the Lexer (Low part)
 
 bool IsValid(void){return (First|Last);}  // 0 char is invalid
};

 CArr<SRange> RangeArr;    // TODO: Optimize allocations
 CArr<SState> StateArr;    // A map to find SRange by char index and not by search in RangeArr by State+First+Last // TODO: Optionally disable to save some memory (RangeArr needs to be sorted by State)
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
// FIFO is used to catch something like this: ({)}
// Some scopes closed by same char (use \ to hide)
//
/*enum EScopeFlags
{
 sfNone = 0,
 sfFlat = 0x01,   // Not nestable, same symbol for opening and for closing (Uses FlatState)
}; */

struct SScopeLst
{
 SCVR uint MaxScopes = 256;   // uint8 sized DepthState
 SCVR uint MaxGroups = 256;   // Max group indexes for scopes (tokens)

struct SScope
{
// uint32 TknIndex;  // Index of the Scope opening token
 SPos   Pos;       // 4/8 bytes
 uint16 Unused;
 uint8  GroupId;
 uint8  Flags;     // From ETokenFlags
};

 uint NextPosInFIFO;
 CArr<SScope> ScopeArr;         // Used as stack for nestable scopes
 uint8 DepthState[MaxGroups];   // Max depth is 256 for each scope type  // States of a flat scope markers (Like ' or ")  // TODO: Use bits to merk the states (Will take 32 bytes instead of 256)    // 1-Opened scope, 0-Closed scope // Transitions: 0->1, 1->0;  1->1 or 0->0 is an error
//------------------------------------------------------
SScopeLst(void)
{
 this->NextPosInFIFO = 0;
 memset(&DepthState,0,sizeof(DepthState));
}
//------------------------------------------------------
sint Upd(uint8 GroupId, uint8 Flags, uint Line, uint Pos, uint Offs, SPos* ScOpnPos=nullptr)
{
 uint8* DepthPtr = &this->DepthState[GroupId];
 if((Flags & (tfScopeOpn|tfScopeCse)) == (tfScopeOpn|tfScopeCse))   // Flat scope Open or Close
  {
   if(*DepthPtr)Flags &= ~tfScopeOpn;  // Already opened, close it
  }
 if(Flags & tfScopeOpn)
  {
   if(*DepthPtr == (MaxScopes-1))return -tleScopeTooDeep;
   this->NextPosInFIFO++;
   (*DepthPtr)++;
   if(this->NextPosInFIFO > this->ScopeArr.Count())this->ScopeArr.Append(nullptr);  // Allocate an additional FIFO entry 
   SScope* Last  = this->ScopeArr.Get(this->NextPosInFIFO-1);
   Last->Pos.Set(Line, Pos, Offs);
   Last->GroupId = GroupId;
   Last->Flags   = Flags;
   return *DepthPtr;
  }
 if(Flags & tfScopeCse)
  {
   if(!this->NextPosInFIFO)return -tleUnexpScpClse;   // The FIFO is empty!
   SScope* Last = this->ScopeArr.Get(this->NextPosInFIFO-1);
   if(Last->GroupId != GroupId)
    {
     if(ScOpnPos)ScOpnPos->Set(&Last->Pos); // Useful for the error context 
     return -tleWrongScpClse;
    }
   this->NextPosInFIFO--;
   if(!*DepthPtr)return -tleUnexpScpClse;   // The depth counter is 0!
   (*DepthPtr)--;
  }
 return *DepthPtr;
}
//------------------------------------------------------
};
//============================================================================================================
struct STokenLst           // Parsed from a file
{
// enum EFlg {flNone=0, flStorePos=0x01};

 CArr<STkn> Tokens;
 CArr<SPos> Positions;
// uint32 LastScope = 0;   // 0 is the root scope
// uint32 Flags = flNone;

//------------------------------------------------------
// Need NoLex flag?
//
sint Add(uint8 CurState, uint8 Group, uint32 Type, uint32 Flags, uint TknLen, const achar* TknVal, SPos&& TknPos, SPos& PrvPos) //     uint16 StrIdx, sint LexIdx, sint ExtIdx, uint8 slflg, uint32 AbsPos, uint16 SrcLin, uint16 SrcPos)
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
SRangeLst Ranges;    
SScopeLst Scopes; 
STokenLst Tokens;

// Parsing context
uint8  ChrLen;  // Size of current UTF-8 char
uint32 LineNum;  // Line number
uint32 LineNxt;  // Next position on the line
uint32 LinePos;  // Position on the line
// Reset on each call to 'Parse'?
uint   TknLine;  //= LineNum;  // Line on which the current token is
uint   TknLPos;  //= LinePos;  // Position of current token on the line
uint   TknSize;  //= 0;
uint   TknOffs;  //= uint(CurPtr-BegPtr);  // Absolute offset in Data
uint   TknType;  //= 0;  // Bit mask
uint8  TknGroup; //= 0;  // Last one will be used
uint8  TknFlags; //= 0;  // Last one will be used

uint32 ParseFlg;      // TODO: Optionally split on change state to base (R or L ?)     // And split to group change to < or > or both
const achar* BegPtr;
const achar* EndPtr;
const achar* CurPtr;

// TODO: Remember last up to 256(?) states for error reporting (With position of state changes) as a circular buffer and a pointer in it
achar TokenBuf[MaxTokenLen];        // Allocate?  // One page?
//------------------------------------------------------------------------------------------------------------
void ResetToken(void)
{
 TknSize = TknType = TknGroup = TknFlags = 0;         // Reset the token
 TknLine = LineNum;
 TknLPos = LinePos;
 TknOffs = uint(CurPtr-BegPtr);
}
//------------------------------------------------------------------------------------------------------------
void UpdateToken(uint8 Chr, SRangeLst::SRange* Rng)
{
 TknGroup = Rng->Group;
 TknFlags = Rng->Flags;
 TknType &= ~Rng->TypeClr;  // First clear the flags
 TknType |= Rng->TypeSet;   // then set them
 if(!(Rng->Flags & tfIgnore))TokenBuf[TknSize++] = Chr;   // Accumulate a token 
}
//------------------------------------------------------------------------------------------------------------
sint ProcessToken(uint8 CurState, SPos& PrvPos)
{
// DBGMSG("Token: '%.*s'",TknLen, TknVal);
 if(!TknSize){this->ResetToken(); return 0;}

 PrvPos.Set(TknLine,TknLPos,TknOffs);
 if(TknFlags & tfNoTerm)
  return -tleChrCantBeLast;         // A last char brought this flag and it is not allowed to be last

 if((TknFlags & tfWhtspc) && !(this->ParseFlg & pfKeepWhtspc)){this->ResetToken(); return 0;}  // Ignore whitespaces
 DBGMSG("Token %u: '%.*s'",CurState, TknSize, &TokenBuf);
 //if(Flags & (tfScopeOpn|tfScopeCse))

 this->ResetToken(); 
 return 0;
}

public:
//------------------------------------------------------------------------------------------------------------
sint Init(const achar* Data, uint Size, uint Flags=0)
{
 this->BegPtr   = Data;
 this->EndPtr   = &Data[Size];
 this->CurPtr   = this->BegPtr;
 this->ChrLen   = 0;  // Size of current UTF-8 char
 this->LineNum  = 0;  // Line number
 this->LineNxt  = 0;  // Next position on the line
 this->LinePos  = 0;  // Position on the line

 this->TknLine  = 0;
 this->TknLPos  = 0;
 this->TknSize  = 0;
 this->TknOffs  = 0;
 this->TknType  = 0;
 this->TknGroup = 0;
 this->TknFlags = 0;

 this->ParseFlg = Flags;
 // TODO: Clear some lists
 if(((this->EndPtr - this->BegPtr) >= 3) && (this->BegPtr[0] == 0xEF) && (this->BegPtr[1] == 0xBB) && (this->BegPtr[2] == 0xBF))this->BegPtr += 3;   // Skip UTF-8 BOM
 if(this->EndPtr <= this->BegPtr)return -1;
 return 0;
}
//------------------------------------------------------------------------------------------------------------
// Type is flags that accumulated by 'Parse' loop for current token
// NOTE: State, Group, Type are arbitrary user defined values
// Can add additionally 3 nonbase target states
// Check return value explicitly as -1 in case of an error if using multiple target states
//
sint AddRange(uint32 TgtState, uint8 First, uint8 Last, uint8 NextState, uint8 Group=0, uint32 TypeSet=0, uint32 TypeClr=0, uint32 Flags=0)
{
 SRangeLst::SRange rng{.First=First, .Last=Last, .State=NextState, .Group=Group, .Flags=Flags, .TypeSet=TypeSet, .TypeClr=TypeClr};
 uint32 status = 0;
 do
  {
   sint res = this->Ranges.Add(rng, (uint8)TgtState);
   status <<= 8;     // Last return value will be lowest
   status  |= (uint8)res;
   if(res < 0)break;
   TgtState >>= 8;
  }
   while(TgtState);
 return (sint)status;
}
//------------------------------------------------------------------------------------------------------------
// AddToken()
//------------------------------------------------------------------------------------------------------------
// NOTE: ErrPos and ErrLine are counted from 0
//  Can continue to parse a next token(An error may break in a middle of current token, a warning will not)
// NOTE: In case of an error it is possible to extract expected char ranges from current state
//
sint Parse(SErrCtx* ErrCtx=nullptr)  // TODO: Continuable  // TODO: Break on warnings too
{
 SPos   PrvPos;     // For error reporting
 uint8  State = 0;  // Initial state is always 0  // Must reset on each call to 'Parse' ?
 uint PrvSplitFlg = 0;
 for(;CurPtr < EndPtr;)        // Skip BOM if any?
  {
   uint8 Val = *CurPtr;   // Have to decode UTF-8 to know exact token position on a line if there are some UTF-8 chars on it     // TODO: No need to check for NULL char?
//    if(Val == '/')
//      Val = Val;
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

   SRangeLst::SRange* Rng;
   uint SplitFlg = PrvSplitFlg;
   while((Rng=this->Ranges.Get(Val, State),!Rng->IsValid()))   // Detect and fallback to a base state 0 if there is no valid defined range for the char in current state    // If a range is not defined it is a empty record with NextState is 0 (Base state)
    {
     if(!State)   // Nowhere to fallback, already in state 0 (All usable ASCII ranges must be defined in state 0)
      {
       if(ErrCtx)ErrCtx->Set(tleUnexpChar, State, 1, (TokenBuf[TknSize] = Val, &TokenBuf[TknSize]), SPos{LineNum,LinePos,uint(CurPtr-BegPtr)}, SPos{TknLine,TknLPos,TknOffs});  
       return -tleUnexpChar;
      } 
     State     = 0;              // Rng->State (An invalid rec is empty and its state is 0 too)
     SplitFlg |= tfTknRSplit;    // The char will start a new token (Same as tfTknRSplit)
    }

// We have some valid range in Rng now         // !!! Need support for single chars splitted
   if(Rng->Flags & tfBadToken)    // This range in this state is marked as inappropriate at this point (Saves the Lexer from processing some bad number pieces like '0b1010120101;' which would be just split as '0b10101','20101;' otherwise - just add range 2-9 as bad for binary numbers, and 'a-z' too, probably)
    {
     if(ErrCtx)ErrCtx->Set(tleBadChar, State, 1, (TokenBuf[TknSize] = Val, &TokenBuf[TknSize]), SPos{LineNum,LinePos,uint(CurPtr-BegPtr)}, SPos{TknLine,TknLPos,TknOffs});  
     return -tleBadChar;
    }

   if(Rng->Flags & (tfScopeOpn|tfScopeCse))   // Can do scoping even in the middle of a token
    {
     if(sint res=this->Scopes.Upd(Rng->Group, Rng->Flags, LineNum, LinePos, uint(CurPtr-BegPtr), &PrvPos);res < 0)
      {
       if(ErrCtx)
        {
         if(Rng->Flags & tfTknRSplit){this->ProcessToken(State, PrvPos); this->UpdateToken(Val, Rng);}   // Store last token to update the error position
         ErrCtx->Set(res, State, TknSize, TokenBuf, SPos{LineNum,LinePos,uint(CurPtr-BegPtr)}, SPos{PrvPos});
        }
       return res;    // Scoping error
      }
       else if(!res && (Rng->Flags & (tfScopeCse|tfLSplitOnSDZ)) == (tfScopeCse|tfLSplitOnSDZ))
       SplitFlg |= tfTknLSplit;    // LSplit
    }

   SplitFlg |= Rng->Flags;          // RSplit first stores current token then inits a new one; LSplit stores adds the char, stores the token and then inits a new token  // LSplit is for scopes and usually set manually so should have higher priority
   if((SplitFlg & (tfTknLSplit|tfTknRSplit)) == (tfTknLSplit|tfTknRSplit))  // Magic combination to force next char to start a new token
    {
     PrvSplitFlg = tfTknRSplit;     // Split at next char
     SplitFlg  &= ~tfTknLSplit;
    }
     else PrvSplitFlg = 0;
   bool LSplit = SplitFlg & tfTknLSplit;
   if(LSplit)       // The current char will be last char of current token
    {
     this->UpdateToken(Val, Rng);
     if(sint res=this->ProcessToken(State, PrvPos);res < 0)     // Check size before the call?
      {
       if(ErrCtx)ErrCtx->Set(-res, State, TknSize, TokenBuf, SPos{LineNum,LinePos,uint(CurPtr-BegPtr)}, SPos{PrvPos});
       return res;
      }
    }
   else if(SplitFlg & tfTknRSplit)       // Store the accumulated token  // The current char will start a new token
    {
     if(sint res=this->ProcessToken(State, PrvPos);res < 0)     // Check size before the call?
      {
       if(ErrCtx)ErrCtx->Set(-res, State, TknSize, TokenBuf, SPos{LineNum,LinePos,uint(CurPtr-BegPtr)}, SPos{PrvPos});
       return res;
      }
    }
   if(!LSplit)  // Just update the current token
    {
     this->UpdateToken(Val, Rng);
    }
   if((SplitFlg & tfBRstIfSplit) && (SplitFlg & (tfTknLSplit|tfTknRSplit)))State = 0;
     else State = Rng->State; 

   LinePos = LineNxt;
   CurPtr++;
   ChrLen--;
  }

 if(sint res=this->ProcessToken(State, PrvPos);res < 0)
  {
   if(ErrCtx)ErrCtx->Set(-res, State, TknSize, TokenBuf, SPos{LineNum,LinePos,uint(CurPtr-BegPtr)}, SPos{PrvPos});
   return res;
  }

 if(this->Scopes.NextPosInFIFO)    // Check any unclosed scopes
  {
   if(ErrCtx)
    {
     *TokenBuf = 0;
     SPos pos2;
     if(this->Scopes.NextPosInFIFO > 1)pos2.Set(&this->Scopes.ScopeArr.Get(this->Scopes.NextPosInFIFO-2)->Pos);
     ErrCtx->Set(tleUnclosedScpAtEOF, 0, 0, TokenBuf, SPos{this->Scopes.ScopeArr.Get(this->Scopes.NextPosInFIFO-1)->Pos}, SPos{pos2}, this->Scopes.NextPosInFIFO);    // TODO: Take two last unclosed scope pos
    }
   return -tleUnclosedScpAtEOF;
  }
 return CurPtr - BegPtr;   // Return size of processed text
}
//------------------------------------------------------------------------------------------------------------

};    // TODO: arbitrary reverse token brace: A sequense opens a scope and same sequence in reverse must close it (Useful for raw strings)

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