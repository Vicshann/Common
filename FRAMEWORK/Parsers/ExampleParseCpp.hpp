
/*
https://stackoverflow.com/questions/610245/where-and-why-do-i-have-to-put-the-template-and-typename-keywords/613132#613132
https://stackoverflow.com/questions/776508/best-practices-for-circular-shift-rotate-operations-in-c
https://www.reddit.com/r/ProgrammingLanguages/comments/15o42wx/how_to_implement_an_order_free_parser_with_angle/
https://en.cppreference.com/w/cpp/language/escape
https://en.cppreference.com/w/cpp/language/charset
https://www.quut.com/c/ANSI-C-grammar-l-2011.html
*/


// C++ parsing example
class CParseCpp
{
 enum EChrStates    // Sorted by occurrence probability(may save some memory)   // 256 bytes per state record
  {
   csBase = 0,
   csWhitespc,
   csTSpecial,
   csInTkName,  
   csInNumExt,
//   csInNumSep,   // Just allow 0b1001101'; for now
   csInDecNum,
//   csInHexNum,
   csInHexVal,
   csInOctNum,
  // csInBinNum,  
   csInBinVal,
  // csInHexFlt,
   csInDecFlt,
   csInHFlVal,
   csInFltExp,
 //  csInFltExV,
   csInFlEVal,
   csInCmnBeg,
   csInCmntSL,
   csInCmntML,
   csInCmntBegML,
   csInCmntEndML,
   csInSQString,
   csInDQString,
   csInSQStrEsc,
   csInDQStrEsc,
   csInPossRStr,
   csInSQRawStr,
   csInDQRawStr,
   csInSQRawStrBeg,
   csInDQRawStrBeg,
   csInSQRawStrEnd,
   csInDQRawStrEnd,
  };
//-----------------------------------------------------------------------
 enum ETknType          // Passed to the Lexer
  {
   ttNone,
   ttTkName     = 0x000001,
   ttNumber     = 0x000002,  // Decimal by default (if 0)
   ttDecNum     = 0x000004,
   ttHexNum     = 0x000008,
   ttOctNum     = 0x000010,
   ttBinNum     = 0x000020,
   ttFltNum     = 0x000040,  // Dec or Hex
   ttFltExp     = 0x000080,  // Float number have exponent
   ttWhSpace    = 0x000100,
   ttSpecial    = 0x000200, 
   ttComment    = 0x000400, 
   ttSQString   = 0x000800,  // Packed string - converts to an integer or an array of integers. Packing is platform specific // Always attempted to be embedded in the code without rdata placement
   ttDQString   = 0x001000, 
   ttSQRawStr   = 0x002000,
   ttDQRawStr   = 0x004000,
   ttInCurlyBr  = 0x008000,  // ???
   ttInRoundBr  = 0x010000,  // ???
   ttInSquareBr = 0x020000,  // ???
  };

enum EScpGroup
{
 sgNone,
 sgCmntML,
 sgBrCurly,
 sgBrRound,
 sgBrSquare,
 sgSQString,
 sgDQString,
 sgSQRawStr,
 sgDQRawStr,
};
//------------------------------------------------------------------------------------------------------------
CTokenizer tkn;
//------------------------------------------------------------------------------------------------------------
void RegRanges_WS(void)
{
 // Whitespaces
 tkn.AddRange(csBase|(csWhitespc << 8), 0x01,0x20, csWhitespc, 0, ttWhSpace, ttNone, CTokenizer::tfWhtspc);  // Whitespace: 01-20, 7F  // No messing up text parsing/display - treat all special chars as whitespaces (If you put them in your source code you are responsible if some text editor(or a terminal emulator) will choke on it)
 tkn.AddRange(csBase|(csWhitespc << 8), 0x7F,0x7F, csWhitespc, 0, ttWhSpace, ttNone, CTokenizer::tfWhtspc);
}
//------------------------------------------------------------------------------------------------------------
void RegRanges_Names(void)
{
 // For names     // Must add to all prefixes and keywords that is handled
 tkn.AddRange(csBase|(csInTkName << 8)|(csInPossRStr << 16),   'a','z', csInTkName, 0, ttTkName);
 tkn.AddRange(csBase|(csInTkName << 8)|(csInPossRStr << 16),   'A','Z', csInTkName, 0, ttTkName);
 tkn.AddRange(csBase|(csInTkName << 8)|(csInPossRStr << 16),   '_','_', csInTkName, 0, ttTkName);
 tkn.AddRange(csBase|(csInTkName << 8)|(csInPossRStr << 16), 0x80,0xFF, csInTkName, 0, ttTkName);   // UTF-8 multi-byte chars  // Too much to manage - just allow any of this to be in names as 'a - z'  // Means no special chars in extended codepoints will be supported (Too slow to parse)  // Aliasing will solve this at the Lexer level
                                                   
 tkn.AddRange(csInTkName|(csInPossRStr << 8), '0','9', csInTkName, 0, ttNone  );
} 
//------------------------------------------------------------------------------------------------------------
void RegRanges_Specials(void)          // TODO: Use @ for aliasing by default
{
 // Specials (Split by Lexer)
 tkn.AddRange(csBase|(csTSpecial << 8), 0x21,0x2F, csTSpecial, 0, ttSpecial, ttNone);  // !"#$%&'()*+,-./
 tkn.AddRange(csBase|(csTSpecial << 8), 0x3A,0x40, csTSpecial, 0, ttSpecial, ttNone);  // :;<=>?@ 
 tkn.AddRange(csBase|(csTSpecial << 8), 0x5B,0x5E, csTSpecial, 0, ttSpecial, ttNone);  // [\]^
 tkn.AddRange(csBase|(csTSpecial << 8), 0x60,0x60, csTSpecial, 0, ttSpecial, ttNone);  // ` 
 tkn.AddRange(csBase|(csTSpecial << 8), 0x7B,0x7E, csTSpecial, 0, ttSpecial, ttNone);  // {|}~
                                                  
// Scopes
 tkn.AddRange(csBase|(csTSpecial << 8), '{','{', csBase, sgBrCurly,  ttInCurlyBr,  ttNone, CTokenizer::tfScopeOpn|CTokenizer::tfTknRSplit|CTokenizer::tfTknLSplit);
 tkn.AddRange(csBase|(csTSpecial << 8), '}','}', csBase, sgBrCurly,  ttInCurlyBr,  ttNone, CTokenizer::tfScopeCse|CTokenizer::tfTknRSplit|CTokenizer::tfTknLSplit);

 tkn.AddRange(csBase|(csTSpecial << 8), '(','(', csBase, sgBrRound,  ttInRoundBr,  ttNone, CTokenizer::tfScopeOpn|CTokenizer::tfTknRSplit|CTokenizer::tfTknLSplit);
 tkn.AddRange(csBase|(csTSpecial << 8), ')',')', csBase, sgBrRound,  ttInRoundBr,  ttNone, CTokenizer::tfScopeCse|CTokenizer::tfTknRSplit|CTokenizer::tfTknLSplit);

 tkn.AddRange(csBase|(csTSpecial << 8), '[','[', csBase, sgBrSquare, ttInSquareBr, ttNone, CTokenizer::tfScopeOpn|CTokenizer::tfTknRSplit|CTokenizer::tfTknLSplit);
 tkn.AddRange(csBase|(csTSpecial << 8), ']',']', csBase, sgBrSquare, ttInSquareBr, ttNone, CTokenizer::tfScopeCse|CTokenizer::tfTknRSplit|CTokenizer::tfTknLSplit);
  
// Comments  (Multiline comments are nestable)
 tkn.AddRange(csBase|(csTSpecial << 8),           '/','/', csInCmnBeg,           0, ttNone,     ttNone, CTokenizer::tfTknRSplit);    // Always starts a new token (Div or a comment)
 tkn.AddRange(csInCmnBeg,                         '/','/', csInCmntSL,           0, ttComment,  ttNone, CTokenizer::tfComment); 
 tkn.AddRange(csInCmnBeg|(csInCmntBegML << 8),    '*','*', csInCmntML,    sgCmntML, ttComment,  ttNone, CTokenizer::tfComment|CTokenizer::tfScopeOpn);    // Increase scope depth and continue the comment
                                                                                                
 tkn.AddRange(csInCmntSL,                       0x01,0xFF, csInCmntSL,           0, ttComment,  ttNone, CTokenizer::tfComment);
 tkn.AddRange(csInCmntSL,                       '\n','\n', csWhitespc,           0, ttComment,  ttNone, CTokenizer::tfWhtspc|CTokenizer::tfTknRSplit);    // Done at EOL   // '\r' (if present) will still be left at the end of a comment
                                                                                                
 tkn.AddRange(csInCmntML|(csInCmntEndML << 8),  0x01,0xFF, csInCmntML,           0, ttComment,  ttNone, CTokenizer::tfComment); 
 tkn.AddRange(csInCmntML,                         '/','/', csInCmntBegML,        0, ttComment,  ttNone, CTokenizer::tfComment); 
                                                                                                
 tkn.AddRange(csInCmntML,                         '*','*', csInCmntEndML,        0, ttComment,  ttNone, CTokenizer::tfComment); 
 tkn.AddRange(csInCmntEndML,                      '/','/', csInCmntML,    sgCmntML, ttComment,  ttNone, CTokenizer::tfComment|CTokenizer::tfScopeCse|CTokenizer::tfLSplitOnSDZ|CTokenizer::tfBRstIfSplit);  

// In C/C++ \ is used as line continuation mark so defining macros in multi-line is possible. It is also possible to change a single line comment to multi-line with it. And any tokens can be split too.
// Just consume and ignore anything up to '\n' includin it. Anything on a next line will continue the token
// This would mean that we must add such behaviour in EVERY state(Every name, number and every composed token) and it MUST be an unique state for proper continuation!
// I am too lazy to implement that ugly nonsence!    // May be allow a char to be assigned globally to skip rest of the line(Same as '\n' is works globally)?
}                                            
//------------------------------------------------------------------------------------------------------------
void RegRanges_Strings(void) 
{
// Strings: "" '' raw        // Parse escapes here or later?      // Store quotes as separate tokens?
 tkn.AddRange(csBase|(csTSpecial << 8),         '\'','\'', csInSQString, sgSQString, ttSQString, ttNone, CTokenizer::tfEscString|CTokenizer::tfScopeOpn|CTokenizer::tfTknLSplit);   // Raw format for this type of string too?
 tkn.AddRange(csInSQString,                     0x01,0xFF, csInSQString, sgSQString, ttSQString, ttNone, CTokenizer::tfEscString);
 tkn.AddRange(csInSQString,                     '\\','\\', csInSQStrEsc, sgSQString, ttSQString, ttNone, CTokenizer::tfEscString);   // Escape a char (including ')
 tkn.AddRange(csInSQString,                     '\'','\'', csBase,       sgSQString, ttSQString, ttNone, CTokenizer::tfEscString|CTokenizer::tfScopeCse|CTokenizer::tfTknRSplit|CTokenizer::tfTknLSplit);   // Close the scope
 tkn.AddRange(csInSQStrEsc,                     0x01,0xFF, csInSQString, sgSQString, ttSQString, ttNone, CTokenizer::tfEscString);   // Consume a char and return to the string state

 tkn.AddRange(csBase|(csTSpecial << 8),         '\"','\"', csInDQString, sgDQString, ttDQString, ttNone, CTokenizer::tfEscString|CTokenizer::tfScopeOpn|CTokenizer::tfTknLSplit);   // Raw format for this type of string too?
 tkn.AddRange(csInDQString,                     0x01,0xFF, csInDQString, sgDQString, ttDQString, ttNone, CTokenizer::tfEscString);
 tkn.AddRange(csInDQString,                     '\\','\\', csInDQStrEsc, sgDQString, ttDQString, ttNone, CTokenizer::tfEscString);   // Escape a char (including ')
 tkn.AddRange(csInDQString,                     '\"','\"', csBase,       sgDQString, ttDQString, ttNone, CTokenizer::tfEscString|CTokenizer::tfScopeCse|CTokenizer::tfTknRSplit|CTokenizer::tfTknLSplit);   // Close the scope
 tkn.AddRange(csInDQStrEsc,                     0x01,0xFF, csInDQString, sgDQString, ttDQString, ttNone, CTokenizer::tfEscString);   // Consume a char and return to the string state

// Raw strings prefix
 tkn.AddRange(csBase,            'R','R', csInPossRStr,             0, ttNone,     ttNone);      // NOTE: Slows down parsing of every 'R' 

// Raw single-quoted string
 tkn.AddRange(csInPossRStr,    '\'','\'', csInSQRawStrBeg, sgSQRawStr, ttSQRawStr, ttNone, CTokenizer::tfScpMrkBegin); 
 tkn.AddRange(csInSQRawStrBeg, 0x01,0xFF, csInSQRawStrBeg, sgSQRawStr, ttSQRawStr, ttNone);
 tkn.AddRange(csInSQRawStrBeg,   '(','(', csInSQRawStr,    sgSQRawStr, ttNone,     ttNone, CTokenizer::tfRawString|CTokenizer::tfTknLSplit|CTokenizer::tfScopeOpn|CTokenizer::tfScpMrkEnd);
 tkn.AddRange(csInSQRawStr,    0x01,0xFF, csInSQRawStr,    sgSQRawStr, ttSQRawStr, ttNone, CTokenizer::tfRawString);

 tkn.AddRange(csInSQRawStr,      ')',')', csInSQRawStrEnd, sgSQRawStr, ttSQRawStr, ttNone, CTokenizer::tfRawString|CTokenizer::tfTknMemStore|CTokenizer::tfScpMrkBegin);    // Remember a possible finished string
 tkn.AddRange(csInSQRawStrEnd, 0x01,0xFF, csInSQRawStrEnd, sgSQRawStr, ttSQRawStr, ttNone, CTokenizer::tfRawString);                                // The string continues
 tkn.AddRange(csInSQRawStrEnd,   ')',')', csInSQRawStrEnd, sgSQRawStr, ttSQRawStr, ttNone, CTokenizer::tfRawString|CTokenizer::tfTknMemStore|CTokenizer::tfScpMrkBegin);    // Repeat // Remember a possible scope closing token
// tkn.AddRange(csInSQRawStrEnd, '\\','\\', csInSQRawStrEnd,    sgSQRawStr, ttSQRawStr, ttNone,  CTokenizer::tfBadToken);      // Disabled: we can`t be sure if it is actually in a closing marker now
// tkn.AddRange(csInSQRawStrEnd, '/','/',   csInSQRawStrEnd,    sgSQRawStr, ttSQRawStr, ttNone,  CTokenizer::tfBadToken); 
 tkn.AddRange(csInSQRawStrEnd, '\'','\'', csBase,          sgSQRawStr, ttSQRawStr, ttNone, CTokenizer::tfRawString|CTokenizer::tfScopeCse|CTokenizer::tfLSplitOnSDZ|CTokenizer::tfMemSplitOnSDZ|CTokenizer::tfScpMrkEnd|CTokenizer::tfOChStateOnSDZ); 

// Raw double-quoted string
 tkn.AddRange(csInPossRStr,    '\"','\"', csInDQRawStrBeg, sgDQRawStr, ttDQRawStr, ttNone, CTokenizer::tfScpMrkBegin); 
 tkn.AddRange(csInDQRawStrBeg, 0x01,0xFF, csInDQRawStrBeg, sgDQRawStr, ttDQRawStr, ttNone);
 tkn.AddRange(csInDQRawStrBeg,   '(','(', csInDQRawStr,    sgDQRawStr, ttNone,     ttNone, CTokenizer::tfRawString|CTokenizer::tfTknLSplit|CTokenizer::tfScopeOpn|CTokenizer::tfScpMrkEnd);
 tkn.AddRange(csInDQRawStr,    0x01,0xFF, csInDQRawStr,    sgDQRawStr, ttDQRawStr, ttNone, CTokenizer::tfRawString);

 tkn.AddRange(csInDQRawStr,      ')',')', csInDQRawStrEnd, sgDQRawStr, ttDQRawStr, ttNone, CTokenizer::tfRawString|CTokenizer::tfTknMemStore|CTokenizer::tfScpMrkBegin);    // Remember a possible finished string
 tkn.AddRange(csInDQRawStrEnd, 0x01,0xFF, csInDQRawStrEnd, sgDQRawStr, ttDQRawStr, ttNone, CTokenizer::tfRawString);                                // The string continues
 tkn.AddRange(csInDQRawStrEnd,   ')',')', csInDQRawStrEnd, sgDQRawStr, ttDQRawStr, ttNone, CTokenizer::tfRawString|CTokenizer::tfTknMemStore|CTokenizer::tfScpMrkBegin);    // Repeat // Remember a possible scope closing token
// tkn.AddRange(csInDQRawStrEnd, '\\','\\', csInDQRawStrEnd,    sgDQRawStr, ttDQRawStr, ttNone,  CTokenizer::tfBadToken);      // Disabled: we can`t be sure if it is actually in a closing marker now
// tkn.AddRange(csInDQRawStrEnd, '/','/',   csInDQRawStrEnd,    sgDQRawStr, ttDQRawStr, ttNone,  CTokenizer::tfBadToken); 
 tkn.AddRange(csInDQRawStrEnd, '\"','\"', csBase,          sgDQRawStr, ttDQRawStr, ttNone, CTokenizer::tfRawString|CTokenizer::tfScopeCse|CTokenizer::tfLSplitOnSDZ|CTokenizer::tfMemSplitOnSDZ|CTokenizer::tfScpMrkEnd|CTokenizer::tfOChStateOnSDZ); 
}
//------------------------------------------------------------------------------------------------------------
//Check that you can have inputs like +++-+++-+-+-++3+-+-+-+5 and they are valid. 
// Unary operators should be given more precedence than the binary arithmetic ones, 
// to allow expressions like 5 * - 3 or -5 * +3 (to be interpreted as 5 * (-3) and (-5) * (+3)) or at least more than the adding operators + and - (in that case, de second -5 * +3 would be incorrect)
// And do not forget -(123) is same as -123 bit it is known later
// Ex: something-5    // Since a language can be whitespace free, does this mean that -5 is an argument to 'something' or it is an expression like 'int x = something - 5' ?
// 
void RegRanges_Numbers(void)  // NOTE: It is diffucult to determine negative numbers!   // TODO: Is it possible to attach a leading sign without actually knowing if it an operator or a part of the number?
{
// For all numbers
 tkn.AddRange(csBase,        '0','0', csInNumExt, 0, ttNumber, ttNone, CTokenizer::tfNumeric);    // 0OCTAL, 0x,0b    // Decimal 0, if alone
 tkn.AddRange(csBase,        '1','9', csInDecNum, 0, ttDecNum, ttNone, CTokenizer::tfNumeric);    // Or float
 tkn.AddRange(csInDecNum,    '0','9', csInDecNum, 0, ttDecNum, ttNone, CTokenizer::tfNumeric);    // May be turned into float
 tkn.AddRange(csInDecNum,  '\'','\'', csInDecNum, 0, ttNone,   ttNone, CTokenizer::tfIgnore|CTokenizer::tfNoTerm);     // Just ignore those, do not store in the number string
// Turning dec numbers into dec floats
 tkn.AddRange(csInDecNum,    '.','.', csInDecFlt, 0, ttFltNum);    // Can have dec floats like 'float x = 34.;'
 tkn.AddRange(csInDecFlt,    '0','9', csInDecFlt, 0, ttNone,   ttNone, CTokenizer::tfNumeric);
 tkn.AddRange(csInDecFlt,  '\'','\'', csInDecFlt, 0, ttNone,   ttNone, CTokenizer::tfIgnore|CTokenizer::tfNoTerm);    // Just ignore those, do not store in the number string
 tkn.AddRange(csInDecFlt,    'e','e', csInFltExp, 0, ttFltExp, ttNone, CTokenizer::tfNoTerm);    
 tkn.AddRange(csInDecFlt,    'E','E', csInFltExp, 0, ttFltExp, ttNone, CTokenizer::tfNoTerm);
// Exponent (Dec/Hex floats)                                                 
 tkn.AddRange(csInFltExp,    '0','9', csInFlEVal, 0, ttNone,   ttNone, CTokenizer::tfNumeric);    // E5
 tkn.AddRange(csInFltExp,    '+','+', csInFlEVal, 0, ttNone,   ttNone, CTokenizer::tfNoTerm );    // E+5   // 3.250000e+004
 tkn.AddRange(csInFltExp,    '-','-', csInFlEVal, 0, ttNone,   ttNone, CTokenizer::tfNoTerm );    // E-5
 tkn.AddRange(csInFlEVal,    '0','9', csInFlEVal, 0, ttNone,   ttNone, CTokenizer::tfNumeric); 
 tkn.AddRange(csInFlEVal,  '\'','\'', csInFlEVal, 0, ttNone,   ttNone, CTokenizer::tfIgnore|CTokenizer::tfNoTerm);    // Just ignore those, do not store in the number string
// Octal numbers                                                  
 tkn.AddRange(csInNumExt,    '0','8', csInOctNum, 0, ttOctNum, ttNone, CTokenizer::tfNumeric);    // 00067
 tkn.AddRange(csInOctNum,    '0','8', csInOctNum, 0, ttNone,   ttNone, CTokenizer::tfNumeric);    // Only 0-8 is valid
 tkn.AddRange(csInOctNum,  '\'','\'', csInOctNum, 0, ttNone,   ttNone, CTokenizer::tfIgnore|CTokenizer::tfNoTerm);    // Just ignore those, do not store in the number string
// Binary numbers                                                   
 tkn.AddRange(csInNumExt,    'b','b', csInBinVal, 0, ttBinNum, ttNone, CTokenizer::tfNoTerm);    
 tkn.AddRange(csInNumExt,    'B','B', csInBinVal, 0, ttBinNum, ttNone, CTokenizer::tfNoTerm); 
 tkn.AddRange(csInBinVal,    '0','1', csInBinVal, 0, ttNone,   ttNone, CTokenizer::tfNumeric);    // This can be separated
 tkn.AddRange(csInBinVal,  '\'','\'', csInBinVal, 0, ttNone,   ttNone, CTokenizer::tfIgnore|CTokenizer::tfNoTerm);    // Just ignore those, do not store in the number string
// Hex numbers                                                       
 tkn.AddRange(csInNumExt,    'x','x', csInHexVal, 0, ttHexNum, ttNone, CTokenizer::tfNoTerm);     // Hex/Float
 tkn.AddRange(csInNumExt,    'X','X', csInHexVal, 0, ttHexNum, ttNone, CTokenizer::tfNoTerm);     // Hex/Float
 tkn.AddRange(csInHexVal,    '0','9', csInHexVal, 0, ttNone,   ttNone, CTokenizer::tfNumeric);
 tkn.AddRange(csInHexVal,    'a','f', csInHexVal, 0, ttNone,   ttNone, CTokenizer::tfNumeric);
 tkn.AddRange(csInHexVal,    'A','F', csInHexVal, 0, ttNone,   ttNone, CTokenizer::tfNumeric);
 tkn.AddRange(csInHexVal,  '\'','\'', csInHexVal, 0, ttNone,   ttNone, CTokenizer::tfIgnore|CTokenizer::tfNoTerm);    // Just ignore those, do not store in the number string
// Turning hex numbers into hex floats                                                 
 tkn.AddRange(csInHexVal,    '.','.', csInHFlVal, 0, ttFltNum, ttNone, CTokenizer::tfNoTerm);     // Different state to allow only one '.'     (Cannot have hex floats like 'float x = 0x34.;'  - Why?)                                                 
 tkn.AddRange(csInHFlVal,    '0','9', csInHFlVal, 0, ttNone,   ttNone, CTokenizer::tfNumeric);
 tkn.AddRange(csInHFlVal,    'a','f', csInHFlVal, 0, ttNone,   ttNone, CTokenizer::tfNumeric);
 tkn.AddRange(csInHFlVal,    'A','F', csInHFlVal, 0, ttNone,   ttNone, CTokenizer::tfNumeric);
 tkn.AddRange(csInHFlVal,  '\'','\'', csInHFlVal, 0, ttNone,   ttNone, CTokenizer::tfIgnore|CTokenizer::tfNoTerm);    // Just ignore those, do not store in the number string                                                 
 tkn.AddRange(csInHFlVal,    'p','p', csInFltExp, 0, ttFltExp, ttNone, CTokenizer::tfNoTerm);     // Same format of exponent as dec float
 tkn.AddRange(csInHFlVal,    'P','P', csInFltExp, 0, ttFltExp, ttNone, CTokenizer::tfNoTerm);
}
//------------------------------------------------------------------------------------------------------------
void RegRanges_Scopes(void)
{

}
//------------------------------------------------------------------------------------------------------------
public:
void Initialize(void)
{
 //tkn.Initialize();
 this->RegRanges_WS();
 this->RegRanges_Specials();  // Must be after WS and before everything else
 this->RegRanges_Strings();
 this->RegRanges_Scopes();
 this->RegRanges_Names();
 this->RegRanges_Numbers();
}
//------------------------------------------------------------------------------------------------------------
sint ParseFile(const achar* Path)
{
 CArr<achar> Text;
 Text.FromFile(Path);
 if(Text.Length() < 1)return -9;
 CTokenizer::SErrCtx ectx{};
 this->Initialize();
 tkn.Init(Text.Data(), Text.Size(), 0);
 sint pres = tkn.Parse(&ectx);
 if(pres < 0)
  {
   LOGMSG("Parsing failed: %i, State=%u, Val=%02X('%.*s') at {Line=%u, Pos=%u} in {Line=%u, Pos=%u}",-pres, ectx.State, *ectx.Value, ectx.ValLen, ectx.Value,  ectx.CurPos.Line+1, ectx.CurPos.Pos+1,  ectx.PrvPos.Line+1, ectx.PrvPos.Pos+1);
   return pres;
  }
   else {LOGMSG("Parsing OK: %i",pres);} 
 return pres;
}
//------------------------------------------------------------------------------------------------------------
};