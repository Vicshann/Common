
//============================================================================================================
enum ECLAFlg
{
 cfNone     = 0,
 cfCaseSens = 1,
};

//------------------------------------------------------------------------------------------------------------
#if defined(PLT_WIN_USR)       // All arguments are in a single string
//------------------------------------------------------------------------------------------------------------
// Separate name reading to support composite arguments
// Set AOffs to 0 initially and expect it to be -1 when there are no more args
// NOTE: Only ASCII chars can be used as arg names (To be fast and avoid some case sensitivity problems)
//
static bool CheckCLArg(sint& AOffs, const achar* ArgName, uint32 Flags=0)
{
 const wchar* CLBase  = &NPTM::GetArgV()[AOffs];
 const wchar* CmdLine = CLBase;

 while(*CmdLine && (*CmdLine <= 0x20))CmdLine++;  // Skip any spaces before
 if(!*CmdLine){AOffs=-1; return 0;}   // No more args
 if(*CmdLine == '\"'){AOffs += CmdLine - CLBase; return false;}    // Names should not be in quotes

 sint match = (Flags & cfCaseSens)?(NSTR::MatchCS(CmdLine, ArgName)):(NSTR::MatchCI(CmdLine, ArgName));    // The name is used as a base string to match at its terminating 0
 if(match <= 0)return false;
 DBGTRC("Matching arg name at %u: %.*ls", (AOffs+(CmdLine-CLBase)), match, CmdLine);

 CmdLine = &CmdLine[match];
 while(*CmdLine && (*CmdLine <= 0x20))CmdLine++;  // Skip any spaces after
 if(*CmdLine)AOffs += CmdLine - CLBase;
  else AOffs = -1;  // No more args
 return true;
}
//------------------------------------------------------------------------------------------------------------
// Get the argument value until a separator
// Copies a next arg to user`s buffer
//
static sint GetCLArg(sint& AOffs, achar* DstBuf=nullptr, uint DstCCnt=uint(-1))    // Enumerate and copy     // NOTE: Copying is inefficient (The values may be very big)
{
 wchar SFchB = '\"';
 wchar SFchE = '\"';
 const wchar* CLBase  = &NPTM::GetArgV()[AOffs];
 const wchar* CmdLine = CLBase;
 if(DstBuf)*DstBuf = 0;
 while(*CmdLine && (*CmdLine <= 0x20))CmdLine++;  // Skip any spaces before
 if(!*CmdLine){AOffs=-1; return 0;}   // No more args
 if(*CmdLine == SFchB)CmdLine++;  // Skip opening quote
   else SFchE = 0x20;             // No quotes, scan until a first separator
 if(!DstBuf)return (sint)SizeOfWStrAsUtf8(CmdLine, uint(-1), SFchE) + 4;       // Calculate buffer size for current argument    // +Space for a terminating 0

 uint SrcLen = uint(-1);          // TODO: Replace with MaxUint or MAX_UINT
 WStrToUtf8(DstBuf, CmdLine, DstCCnt, SrcLen, SFchE);
 DstBuf[DstCCnt] = 0;
 DBGTRC("Extracted arg value at %u: %.*s", (AOffs+(CmdLine-CLBase)), DstCCnt, DstBuf);

 wchar LastCh = CmdLine[SrcLen];
 if((LastCh != SFchE) && LastCh)  // Dst is full, find end of the argument
  {
   sint pos = NSTR::ChrOffset(&CmdLine[SrcLen], SFchE);
   if(pos < 0){AOffs = -1; return (sint)DstCCnt;}  // No more args
   SrcLen += (uint)pos;
  }

 CmdLine = &CmdLine[SrcLen];
 if(*CmdLine)CmdLine++; // Skip last Quote or Space
 while(*CmdLine && (*CmdLine <= 0x20))CmdLine++; // Skip any spaces after it to point at next argument
 if(*CmdLine)AOffs += CmdLine - CLBase;
  else AOffs = -1;  // No more args
 return (sint)DstCCnt;
}
//------------------------------------------------------------------------------------------------------------
// NOTE: Only space-separated args
static const syschar* SkipCLArg(sint& AOffs, uint* Size=nullptr)       // Enumerate     // NOTE: Not null-terminated on Windows
{
 wchar SFchB = '\"';
 wchar SFchE = '\"';
 const wchar* CLBase  = &NPTM::GetArgV()[AOffs];
 const wchar* CmdLine = CLBase;
 while(*CmdLine && (*CmdLine <= 0x20))CmdLine++;  // Skip any spaces before
 if(!*CmdLine){AOffs=-1; return nullptr;}   // No more args
 if(*CmdLine == SFchB)CmdLine++;  // Skip opening quote
   else SFchE = 0x20;             // No quotes, scan until a first space

 uint SrcLen = 0;
 while((CmdLine[SrcLen] ^ SFchE) && CmdLine[SrcLen])SrcLen++;   // Optimize?
 DBGTRC("Skipping arg at %u: %.*ls", (AOffs+(CmdLine-CLBase)), SrcLen, CmdLine);

 const wchar* EndPtr = &CmdLine[SrcLen];
 if(*EndPtr)EndPtr++; // Skip last Quote or Space
 while(*EndPtr && (*EndPtr <= 0x20))EndPtr++; // Skip any spaces after it to point at next argument
 if(*EndPtr)AOffs += EndPtr - CLBase;
  else AOffs = -1;  // No more args
 if(Size)*Size = SrcLen;
 return CmdLine;
}
//------------------------------------------------------------------------------------------------------------
#elif defined(PLT_LIN_USR)    // BSD, MacOS ?
// %rdi, %rsi, %rdx, %rcx, %r8 and %r9
// DescrPtr must be set to 'ELF Auxiliary Vectors' (Stack pointer at ELF entry point)
//
//  argv[0] is not guaranteed to be the executable path, it just happens to be most of the time. It can be a relative path from the current
//   working directory, or just the filename of your program if your program is in PATH, or an absolute path to your program, or the name a
//   symlink that points to your program file, maybe there are even more possibilities. It's whatever you typed in the shell to run your program.
//
// Copies a next param to user`s buffer
// A spawner process is responsible for ARGV
// Quotes are stripped by the shell and quoted args are kept intact
// NOTE: Do not expect the return value to be an argument index!
//
//------------------------------------------------------------------------------------------------------------
static bool CheckCLArg(sint& AOffs, const achar* ArgName, uint32 Flags=0)
{
 if(AOffs < 0)return false;    // Already finished
 const achar** vars = NPTM::GetArgV();
 if(!vars)return false;
 const syschar* val = vars[AOffs & 0xFFFF];
 if(!val){AOffs = -1; return false;}  // End of list reached

 sint match = (Flags & cfCaseSens)?(NSTR::MatchCS(val, ArgName)):(NSTR::MatchCI(val, ArgName));    // The name is used as a base string to match at its terminating 0
 if(match <= 0)return false;
 DBGTRC("Matching arg name at %u: %.*s", (AOffs & 0xFFFF), match, val);
 if(val[match])AOffs = (AOffs & 0xFFFF) | ((match & 0x7FFF) << 16);   // A composite arg, remember offset in it
   else AOffs &= 0xFFFF;    // Just in case
 return true;
}
//------------------------------------------------------------------------------------------------------------
static sint GetCLArg(sint& AOffs, achar* DstBuf, uint DstCCnt=uint(-1))    // Enumerate and copy    // NOTE: Copying is inefficient (The values may be very big)
{
 if(AOffs < 0)return 0;    // Already finished
 const achar** vars = NPTM::GetArgV();
 if(!vars)return 0;
 const syschar* val = vars[AOffs & 0xFFFF];
 if(!val){AOffs = -1; return 0;}  // End of list reached
 val += (AOffs >> 16) & 0x7FFF;
 if(!DstBuf)return NSTR::StrLen(val) + 4;  // + space for a terminating 0
 size_t len = NSTR::StrCopy(DstBuf, val, DstCCnt);
 DBGTRC("Extracted arg value at %u: %.*s", (AOffs & 0xFFFF), len, DstBuf);
 return len;
}
//------------------------------------------------------------------------------------------------------------
static const syschar* SkipCLArg(sint& AOffs, uint* Size=nullptr)       // Enumerate     // NOTE: Not null-terminated on Windows
{
 if(AOffs < 0)return nullptr;    // Already finished
 const achar** vars = NPTM::GetArgV();
 if(!vars)return nullptr;
 const syschar* val = vars[AOffs & 0xFFFF];
 if(!val){AOffs = -1; return nullptr;}  // End of list reached
 if(Size)*Size = NSTR::StrLen(val);
 DBGTRC("Skipping arg at %u: %s", (AOffs & 0xFFFF), val);
 AOffs = (AOffs + 1) & 0xFFFF;   // Wiping out a name offset
 return val;
}
//------------------------------------------------------------------------------------------------------------
#else
static bool CheckCLArg(sint& AOffs, achar* ArgName, uint32 Flags=0){AOffs = -1; return false;}
static sint GetCLArg(sint& AOffs, achar* DstBuf=nullptr, uint DstCCnt=uint(-1)){AOffs = -1; return -1;}
static const syschar* SkipCLArg(sint& AOffs, uint* Size=nullptr){AOffs = -1; return nullptr;}
#endif
//------------------------------------------------------------------------------------------------------------
