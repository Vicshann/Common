
/*
https://stackoverflow.com/questions/610245/where-and-why-do-i-have-to-put-the-template-and-typename-keywords/613132#613132
https://stackoverflow.com/questions/776508/best-practices-for-circular-shift-rotate-operations-in-c
https://www.reddit.com/r/ProgrammingLanguages/comments/15o42wx/how_to_implement_an_order_free_parser_with_angle/
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
   csInHexNum,
   csInHexVal,
   csInOctNum,
   csInBinNum,  
   csInBinVal,
   csInHexFlt,
   csInDecFlt,
   csInHFlVal,
   csInFltExp,
   csInFltExV,
   csInFlEVal,
   csInCmnBeg,
   csInCmntSL,
   csInCmntML,
   csInCmntBegML,
   csInCmntEndML,
  };
//-----------------------------------------------------------------------
 enum ETknType          // Passed to the Lexer
  {
   ttNone,
   ttTkName     = 0x0001,
   ttNumber     = 0x0002,  // Decimal by default (if 0)
   ttDecNum     = 0x0004,
   ttHexNum     = 0x0008,
   ttOctNum     = 0x0010,
   ttBinNum     = 0x0020,
   ttFltNum     = 0x0040,  // Dec or Hex
   ttFltExp     = 0x0080,  // Float number have exponent
   ttWhSpace    = 0x0100,
   ttSpecial    = 0x0200, 
   ttComment    = 0x0400, 
   ttInCurlyBr  = 0x0800,  // ???
   ttInRoundBr  = 0x1000,  // ???
   ttInSquareBr = 0x2000,  // ???
  };

enum EScpGroup
{
 sgNone,
 sgCmntML,
 sgBrCurly,
 sgBrRound,
 sgBrSquare,
};
//------------------------------------------------------------------------------------------------------------
CTokenizer tkn;
//------------------------------------------------------------------------------------------------------------
void RegRanges_WS(void)
{
 // Whitespaces
 tkn.AddRange(csBase,      0x01,0x20, csWhitespc, 0, ttWhSpace, ttNone, CTokenizer::tfWhtspc);  // Whitespace: 01-20, 7F  // No messing up text parsing/display - treat all special chars as whitespaces (If you put them in your source code you are responsible if some text editor(or a terminal emulator) will choke on it)
 tkn.AddRange(csBase,      0x7F,0x7F, csWhitespc, 0, ttWhSpace, ttNone, CTokenizer::tfWhtspc);

 tkn.AddRange(csWhitespc,  0x01,0x20, csWhitespc, 0, ttWhSpace, ttNone, CTokenizer::tfWhtspc);
 tkn.AddRange(csWhitespc,  0x7F,0x7F, csWhitespc, 0, ttWhSpace, ttNone, CTokenizer::tfWhtspc);
}
//------------------------------------------------------------------------------------------------------------
void RegRanges_Names(void)
{
 // For names
 tkn.AddRange(csBase,        'a','z', csInTkName, 0, ttTkName);
 tkn.AddRange(csBase,        'A','Z', csInTkName, 0, ttTkName);
 tkn.AddRange(csBase,        '_','_', csInTkName, 0, ttTkName);
 tkn.AddRange(csBase,      0x80,0xFF, csInTkName, 0, ttTkName);   // UTF-8 multi-byte chars  // Too much to manage - just allow any of this to be in names as 'a - z'  // Means no special chars in extended codepoints will be supported (Too slow to parse)  // Aliasing will solve this at the Lexer level
                                                   
 tkn.AddRange(csInTkName,    '0','9', csInTkName, 0, ttNone  );
 tkn.AddRange(csInTkName,    'a','z', csInTkName, 0, ttNone  );
 tkn.AddRange(csInTkName,    'A','Z', csInTkName, 0, ttNone  );
 tkn.AddRange(csInTkName,    '_','_', csInTkName, 0, ttNone  );
 tkn.AddRange(csInTkName,  0x80,0xFF, csInTkName, 0, ttNone  );
} 
//------------------------------------------------------------------------------------------------------------
void RegRanges_Specials(void)          // TODO: Use @ for aliasing by default
{
 // Specials (Split by Lexer)
 tkn.AddRange(csBase|(csTSpecial << 8),      0x21,0x2F, csTSpecial, 0, ttSpecial, ttNone);  // !"#$%&'()*+,-./
 tkn.AddRange(csBase|(csTSpecial << 8),      0x3A,0x40, csTSpecial, 0, ttSpecial, ttNone);  // :;<=>?@ 
 tkn.AddRange(csBase|(csTSpecial << 8),      0x5B,0x5E, csTSpecial, 0, ttSpecial, ttNone);  // [\]^
 tkn.AddRange(csBase|(csTSpecial << 8),      0x60,0x60, csTSpecial, 0, ttSpecial, ttNone);  // ` 
 tkn.AddRange(csBase|(csTSpecial << 8),      0x7B,0x7E, csTSpecial, 0, ttSpecial, ttNone);  // {|}~
                                                  

// Scopes
 tkn.AddRange(csBase|(csTSpecial << 8), '{','{', csBase, sgBrCurly,  ttInCurlyBr,  ttNone, CTokenizer::tfScopeOpn|CTokenizer::tfTknRSplit|CTokenizer::tfTknLSplit);
 tkn.AddRange(csBase|(csTSpecial << 8), '}','}', csBase, sgBrCurly,  ttInCurlyBr,  ttNone, CTokenizer::tfScopeCse|CTokenizer::tfTknRSplit|CTokenizer::tfTknLSplit);

 tkn.AddRange(csBase|(csTSpecial << 8), '(','(', csBase, sgBrRound,  ttInRoundBr,  ttNone, CTokenizer::tfScopeOpn|CTokenizer::tfTknRSplit|CTokenizer::tfTknLSplit);
 tkn.AddRange(csBase|(csTSpecial << 8), ')',')', csBase, sgBrRound,  ttInRoundBr,  ttNone, CTokenizer::tfScopeCse|CTokenizer::tfTknRSplit|CTokenizer::tfTknLSplit);

 tkn.AddRange(csBase|(csTSpecial << 8), '[','[', csBase, sgBrSquare, ttInSquareBr, ttNone, CTokenizer::tfScopeOpn|CTokenizer::tfTknRSplit|CTokenizer::tfTknLSplit);
 tkn.AddRange(csBase|(csTSpecial << 8), ']',']', csBase, sgBrSquare, ttInSquareBr, ttNone, CTokenizer::tfScopeCse|CTokenizer::tfTknRSplit|CTokenizer::tfTknLSplit);
  
// Comments
 tkn.AddRange(csBase|(csTSpecial << 8),        '/','/', csInCmnBeg, 0,        ttNone,    ttNone, CTokenizer::tfTknRSplit);    // Always starts a new token (Div or a comment)
 tkn.AddRange(csInCmnBeg,    '/','/', csInCmntSL, 0,        ttComment, ttNone, CTokenizer::tfComment); 
 tkn.AddRange(csInCmnBeg,    '*','*', csInCmntML, sgCmntML, ttComment, ttNone, CTokenizer::tfComment|CTokenizer::tfScopeOpn);   

 tkn.AddRange(csInCmntSL,  0x01,0xFF, csInCmntSL, 0, ttComment, ttNone, CTokenizer::tfComment);
 tkn.AddRange(csInCmntSL,  '\n','\n', csWhitespc, 0, ttComment, ttNone, CTokenizer::tfTknRSplit|CTokenizer::tfWhtspc);   // Done at EOL   // '\r' (if present) will still be left at the end of a comment
  
 tkn.AddRange(csInCmntML,     0x01,0xFF, csInCmntML,     0, ttComment, ttNone, CTokenizer::tfComment); 
 tkn.AddRange(csInCmntML,       '/','/', csInCmntBegML,  0, ttComment, ttNone, CTokenizer::tfComment); 
 tkn.AddRange(csInCmntBegML,    '*','*', csInCmntML,     sgCmntML, ttComment, ttNone, CTokenizer::tfComment|CTokenizer::tfScopeOpn); // Increase scope depth and continue the comment

 tkn.AddRange(csInCmntML,       '*','*', csInCmntEndML, 0,     ttComment, ttNone, CTokenizer::tfComment);  // <<<
 tkn.AddRange(csInCmntEndML,  0x01,0xFF, csInCmntML,    0,     ttComment, ttNone, CTokenizer::tfComment); 
 tkn.AddRange(csInCmntEndML,    '/','/', csInCmntML, sgCmntML, ttComment, ttNone, CTokenizer::tfComment|CTokenizer::tfScopeCse|CTokenizer::tfLSplitOnSDZ|CTokenizer::tfBRstIfSplit);  //CTokenizer::tfTknLSplit);  // With this instead of scoping we get the same broken multiline comments as C++
}
//------------------------------------------------------------------------------------------------------------
void RegRanges_Numbers(void)
{
  // For numbers
 tkn.AddRange(csBase,        '0','0', csInNumExt, 0, ttNumber, ttNone, CTokenizer::tfNumeric);    // 0OCTAL, 0x,0b    // Decimal 0, if alone
 tkn.AddRange(csBase,        '1','9', csInDecNum, 0, ttDecNum, ttNone, CTokenizer::tfNumeric);    // Or float
 tkn.AddRange(csInDecNum,    '0','9', csInDecNum, 0, ttDecNum, ttNone, CTokenizer::tfNumeric);    // May be turned into float
 tkn.AddRange(csInDecNum,  '\'','\'', csInDecNum, 0, ttNone  , ttNone, CTokenizer::tfIgnore);     // Just ignore those, do not store in the number string
 tkn.AddRange(csInDecNum,    '.','.', csInDecFlt, 0, ttFltNum);    // Can have dec floats like 'float x = 34.;'
 tkn.AddRange(csInDecFlt,    '0','9', csInDecFlt, 0, ttNone  , ttNone, CTokenizer::tfNumeric);
 tkn.AddRange(csInDecFlt,  '\'','\'', csInDecFlt, 0, ttNone  , ttNone, CTokenizer::tfIgnore);    // Just ignore those, do not store in the number string
 tkn.AddRange(csInDecFlt,    'e','e', csInFltExp, 0, ttFltExp);    
 tkn.AddRange(csInDecFlt,    'E','E', csInFltExp, 0, ttFltExp);
                                                 
 tkn.AddRange(csInFltExp,  0x01,0xFF, csInFltExp, 0, ttNone  , ttNone, CTokenizer::tfBadToken);  // Fill entire range with BadChar first  (Float numbers cannot just end with E/P)
 tkn.AddRange(csInFltExp,    '0','9', csInFlEVal, 0, ttNone  , ttNone, CTokenizer::tfNumeric);    // E5
 tkn.AddRange(csInFltExp,    '+','+', csInFltExV, 0, ttNone  );    // E+5   // 3.250000e+004
 tkn.AddRange(csInFltExp,    '-','-', csInFltExV, 0, ttNone  );    // E-5
 tkn.AddRange(csInFltExV,  0x01,0xFF, csInFltExV, 0, ttNone  , ttNone, CTokenizer::tfBadToken);  // Fill entire range with BadChar first  (Float numbers cannot just end with +/-)
 tkn.AddRange(csInFltExV,    '0','9', csInFlEVal, 0, ttNone  , ttNone, CTokenizer::tfNumeric);  
 tkn.AddRange(csInFlEVal,    '0','9', csInFlEVal, 0, ttNone  , ttNone, CTokenizer::tfNumeric); 
 tkn.AddRange(csInFlEVal,  '\'','\'', csInFlEVal, 0, ttNone  , ttNone, CTokenizer::tfIgnore);    // Just ignore those, do not store in the number string
                                                   
 tkn.AddRange(csInNumExt,    '0','8', csInOctNum, 0, ttOctNum, ttNone, CTokenizer::tfNumeric);    // 00067
 tkn.AddRange(csInOctNum,    '0','8', csInOctNum, 0, ttNone  , ttNone, CTokenizer::tfNumeric);    // Only 0-8 is valid
 tkn.AddRange(csInOctNum,  '\'','\'', csInOctNum, 0, ttNone  , ttNone, CTokenizer::tfIgnore);    // Just ignore those, do not store in the number string
                                                   
 tkn.AddRange(csInNumExt,    'b','b', csInBinNum, 0, ttBinNum);    
 tkn.AddRange(csInNumExt,    'B','B', csInBinNum, 0, ttBinNum); 
 tkn.AddRange(csInBinNum,  0x01,0xFF, csInBinNum, 0, ttNone  , ttNone, CTokenizer::tfBadToken);  // Fill entire range with BadChar first  (Cannot have just 0b part)
 tkn.AddRange(csInBinNum,    '0','1', csInBinVal, 0, ttNone  , ttNone, CTokenizer::tfNumeric);
 tkn.AddRange(csInBinVal,    '0','1', csInBinVal, 0, ttNone  , ttNone, CTokenizer::tfNumeric);    // This can be separated
 tkn.AddRange(csInBinVal,  '\'','\'', csInBinVal, 0, ttNone  , ttNone, CTokenizer::tfIgnore);    // Just ignore those, do not store in the number string
                                                       
 tkn.AddRange(csInNumExt,    'x','x', csInHexNum, 0, ttHexNum);    // Hex/Float
 tkn.AddRange(csInNumExt,    'X','X', csInHexNum, 0, ttHexNum);    // Hex/Float
 tkn.AddRange(csInHexNum,  0x01,0xFF, csInHexNum, 0, ttNone  , ttNone, CTokenizer::tfBadToken);  // Fill entire range with BadChar first  (Cannot have just 0x part)
 tkn.AddRange(csInHexNum,    '0','9', csInHexVal, 0, ttNone  , ttNone, CTokenizer::tfNumeric);
 tkn.AddRange(csInHexNum,    'a','f', csInHexVal, 0, ttNone  , ttNone, CTokenizer::tfNumeric);
 tkn.AddRange(csInHexNum,    'A','F', csInHexVal, 0, ttNone  , ttNone, CTokenizer::tfNumeric);
                                                 
 tkn.AddRange(csInHexVal,    '0','9', csInHexVal, 0, ttNone  , ttNone, CTokenizer::tfNumeric);
 tkn.AddRange(csInHexVal,    'a','f', csInHexVal, 0, ttNone  , ttNone, CTokenizer::tfNumeric);
 tkn.AddRange(csInHexVal,    'A','F', csInHexVal, 0, ttNone  , ttNone, CTokenizer::tfNumeric);
 tkn.AddRange(csInHexVal,  '\'','\'', csInHexVal, 0, ttNone  , ttNone, CTokenizer::tfIgnore);    // Just ignore those, do not store in the number string
                                                 
 tkn.AddRange(csInHexVal,    '.','.', csInHexFlt, 0, ttFltNum);    // Different state to allow only one '.'   
 tkn.AddRange(csInHexFlt,  0x01,0xFF, csInHexFlt, 0, ttNone  , ttNone, CTokenizer::tfBadToken);  // Fill entire range with BadChar first  (Cannot have hex floats like 'float x = 0x34.;'  - Why?)
 tkn.AddRange(csInHexFlt,    '0','9', csInHFlVal, 0, ttNone  , ttNone, CTokenizer::tfNumeric);
 tkn.AddRange(csInHexFlt,    'a','f', csInHFlVal, 0, ttNone  , ttNone, CTokenizer::tfNumeric);
 tkn.AddRange(csInHexFlt,    'A','F', csInHFlVal, 0, ttNone  , ttNone, CTokenizer::tfNumeric);
                                                  
 tkn.AddRange(csInHFlVal,    '0','9', csInHFlVal, 0, ttNone  , ttNone, CTokenizer::tfNumeric);
 tkn.AddRange(csInHFlVal,    'a','f', csInHFlVal, 0, ttNone  , ttNone, CTokenizer::tfNumeric);
 tkn.AddRange(csInHFlVal,    'A','F', csInHFlVal, 0, ttNone  , ttNone, CTokenizer::tfNumeric);
 tkn.AddRange(csInHFlVal,  '\'','\'', csInHFlVal, 0, ttNone  , ttNone, CTokenizer::tfIgnore);    // Just ignore those, do not store in the number string
                                                   
 tkn.AddRange(csInHFlVal,    'p','p', csInFltExp, 0, ttFltExp);    // Same format of exponent as dec float
 tkn.AddRange(csInHFlVal,    'P','P', csInFltExp, 0, ttFltExp);
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
 this->RegRanges_Specials();
 this->RegRanges_Names();
 this->RegRanges_Numbers();
 this->RegRanges_Scopes();
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