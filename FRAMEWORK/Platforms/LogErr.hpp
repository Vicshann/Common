
#pragma once

// See 'https://github.com/gabime/spdlog' for a decent independant logger

struct NLOG    // Can be instanciated if required
{
// Flags. Combined with ELogFlags
enum ELogModes {lmNone=0,
                lmFile        = 0x0010,    // Log to a file (The file handle is in 3 low bytes)
                lmCons        = 0x0020,    // Log to Terminal (STDOUT: 0; Prefer STDERR and leave STDOUT for some data streaming?)  // TODO: Should support ANSI escape codes
                lmProc        = 0x0040,    // Log to a user specified callback proc
                lmSyst        = 0x0080,    // Log to a system logging service [UNIMPLEMENTED]
                lmMask        = 0x00F0
};

// Flags. Passed with each message
enum ELogFlags {lfNone=0,
                lfLogName     = 0x0100,    // Log the function name
                lfLogTime     = 0x0200,    // Log an abstract time stamp  // TODO: Add option to format it in a normal time stamp
                lfLogThID     = 0x0400,    // Log the current thread ID
                lfLogLevel    = 0x0800,    // Log the messsage log level
                lfLogMsgIdx   = 0x1000,    // Log current message index (The index is global for all threads)
                lfLineBreak   = 0x2000,    // Add line break at the message end
                lfRawText     = 0x4000,    // Log the format string as an raw message, no argument processing (Additional arg is the text length)  // Puts the text at new line if any additional info is present
                lfColoring    = 0x8000,
                lfMask        = 0xFF00
               };

// Indeces. Combined with ELogFlags if required
// If a message passes any of these it will override default value in LDSC::LogModes    // Not all messages will need that
enum ELogLevel {llNone=0,                  // No level, always output
                llLogFail     = 0x0001,    // A thread or application itself is unable to continue
                llLogError    = 0x0002,    // Some operation failed but application will continue to operate
                llLogWarning  = 0x0003,    // Something unexpected has happened that may require attention but operation can continue
                llLogNote     = 0x0004,    // Some operational shoose has been made and an user must be informed about it
                llLogInfo     = 0x0005,    // Additional comments about current operation, hints and suggestions
                llLogDebug    = 0x0006,    // Everything that may be required to diagnose a problem during execution
                llLogTrace    = 0x0007,    // Log every step of operation or some complicated algorithm
                llLogMaxLevel = 0x0007,    // To get it all
                llLogBadLevel = 0x0008,    // For LDSC::MaxLevel - Log only unleveled messages (Do not pass this with messages!)
                llMask        = 0x0007
               };

/*
High 16 bits may be used for color:
  256 byte is range for 8 bit per channel
  5 bit max to store in uint16, this gives max 31 colors
  Convert 5 bit color value to percent from max 31 colors and then conver this percent to value in 256 color range
*/

struct LDSC
{
 void (_fcall *pCallback)(achar* Msg, uint MsgLen, uint MsgFlags, volatile LDSC* Logger);
 int32  FileHandle;  // Negative is invalid
 int32  ConsHandle;  // Negative is invalid  // Supports coloring
 uint32 MsgIndex;
 uint16 LogModes;    // Override(To disable) the ones which come with a message (By default, messages specify lmFile|lmCons|lmProc)   // Include base message flags
 uint16 MaxLevel;    // Override. llLogMaxLevel = Log everything  // 0 will log nothing but still form log messages (Why?)
};

static inline constexpr uint32 BaseMsgModes = (lmFile|lmCons|lmProc);                                 // LDSC::LogModes will override any of these passed with a message(Disable only)  // Exclude?   // Should be dicided per message
static inline constexpr uint32 BaseMsgFlags = (lfLineBreak|lfLogName|lfLogTime|lfLogThID|lfLogLevel); // If a message passes any of these it will override the values in LDSC::LogModes // Include?   // Some messages may require to completely replace these

static inline volatile LDSC GLog {nullptr, -1, -1, 0, 0, llLogMaxLevel};

// Pass NLOG::CurrLog by default in all logging functions.
// You can create your own instances of LDSC and switch them on runtime.
// Or pass pass your instances of LDSC to your definitions of logging functions
static inline volatile LDSC* CurrLog = nullptr;  // &GLog;  // No pointer assignment allowed: there is no loader to rebase such pointers

//------------------------------------------------------------------------------------------------------------
// Pass parameters by refs. Since most variables are already allocated on stack it is somehow more efficient to take their addresses than copying them (And some of them may even be objects)
// Reading arguments of wrong size will unlikely to break something
// Messing up argument order and interpreting something else as a pointer may still lead to crash
// Wrong argument number, which usually crash C-style variadic functions, is handled here
// Use this with a macro if default(with deduction guide) ones in not enough
//

// Is overloading possible when using marcos?
static _finline sint LogMessage(volatile LDSC* Ctx, uint32 EFlags, const char* ProcName, const achar* MsgFmt, auto&&... args)
{
 constexpr uint64 sbits = NFMT::PackSizeBits((int[]){sizeof(args)...,0});  // Last 0 needed in case of zero args number (No zero arrays allowed)
 constexpr size_t arnum = NFMT::MakeArgCtrVal(sizeof...(args), sbits);
 return LogProc(MsgFmt, (vptr[]){(vptr)arnum,(vptr)sbits,(NFMT::GetValAddr(args))...}, EFlags, Ctx, ProcName);
}

/*{
 static constexpr int NumbOfArgs = sizeof...(args);
 void* ArgList[NumbOfArgs] = {(NFWK::GetValAddr(args))...};         //{(ValSwitch<NFWK::IsPtrType<decltype(args)>::V>(args))...};  // Store pointers as is   // {&args...};  // *(void**)  // Always wasting memory on allocating the memory array
 return LogProc(MsgFmt, sizeof...(args), (void*[]){(NFWK::GetValAddr(args))...}, EFlags, Ctx, ProcName);
}*/

// Deduction guide: https://en.cppreference.com/w/cpp/language/class_template_argument_deduction#User-defined_deduction_guides
//template<uint32 BFlags=0, typename... Ts> struct DoLogMsg { constexpr _finline DoLogMsg(achar* Fmt, Ts&&... args, const char* ProcName=SRC_FUNC){ LogProc(Fmt, sizeof...(args), (void*[]){(NFWK::GetValAddr(args))...}, BFlags, CurrLog, ProcName); } };
//template<typename... Ts> DoLogMsg(achar* Fmt, Ts&&...) -> DoLogMsg<BaseMsgModes, Ts...>;   // The names must match. So it is impossible to reuse the struct with diferent flag values

// Waiting for C++25?: https://www.cppstories.com/2021/non-terminal-variadic-args/

#define LOGL(flg,msg,...) NFWK::NPTM::NLOG::LogMessage(NFWK::NPTM::NLOG::CurrLog, NFWK::NPTM::NLOG::BaseMsgModes|NFWK::NPTM::NLOG::BaseMsgFlags|NFWK::NPTM::NLOG::ll##flg,SRC_FUNC,msg __VA_OPT__(,) __VA_ARGS__)  // Custom leveled
#define LOGMSG(msg,...) NFWK::NPTM::NLOG::LogMessage(NFWK::NPTM::NLOG::CurrLog, NFWK::NPTM::NLOG::BaseMsgModes|NFWK::NPTM::NLOG::BaseMsgFlags,SRC_FUNC,msg __VA_OPT__(,) __VA_ARGS__)       // Unleveled
#define LOGFAL(msg,...) NFWK::NPTM::NLOG::LogMessage(NFWK::NPTM::NLOG::CurrLog, NFWK::NPTM::NLOG::BaseMsgModes|NFWK::NPTM::NLOG::BaseMsgFlags|NFWK::NPTM::NLOG::llLogFail,   SRC_FUNC,msg __VA_OPT__(,) __VA_ARGS__)
#define LOGERR(msg,...) NFWK::NPTM::NLOG::LogMessage(NFWK::NPTM::NLOG::CurrLog, NFWK::NPTM::NLOG::BaseMsgModes|NFWK::NPTM::NLOG::BaseMsgFlags|NFWK::NPTM::NLOG::llLogError,  SRC_FUNC,msg __VA_OPT__(,) __VA_ARGS__)
#define LOGWRN(msg,...) NFWK::NPTM::NLOG::LogMessage(NFWK::NPTM::NLOG::CurrLog, NFWK::NPTM::NLOG::BaseMsgModes|NFWK::NPTM::NLOG::BaseMsgFlags|NFWK::NPTM::NLOG::llLogWarning,SRC_FUNC,msg __VA_OPT__(,) __VA_ARGS__)
#define LOGNTE(msg,...) NFWK::NPTM::NLOG::LogMessage(NFWK::NPTM::NLOG::CurrLog, NFWK::NPTM::NLOG::BaseMsgModes|NFWK::NPTM::NLOG::BaseMsgFlags|NFWK::NPTM::NLOG::llLogNote,   SRC_FUNC,msg __VA_OPT__(,) __VA_ARGS__)
#define LOGINF(msg,...) NFWK::NPTM::NLOG::LogMessage(NFWK::NPTM::NLOG::CurrLog, NFWK::NPTM::NLOG::BaseMsgModes|NFWK::NPTM::NLOG::BaseMsgFlags|NFWK::NPTM::NLOG::llLogInfo,   SRC_FUNC,msg __VA_OPT__(,) __VA_ARGS__)
#define LOGDBG(msg,...) NFWK::NPTM::NLOG::LogMessage(NFWK::NPTM::NLOG::CurrLog, NFWK::NPTM::NLOG::BaseMsgModes|NFWK::NPTM::NLOG::BaseMsgFlags|NFWK::NPTM::NLOG::llLogDebug,  SRC_FUNC,msg __VA_OPT__(,) __VA_ARGS__)
#define LOGTRC(msg,...) NFWK::NPTM::NLOG::LogMessage(NFWK::NPTM::NLOG::CurrLog, NFWK::NPTM::NLOG::BaseMsgModes|NFWK::NPTM::NLOG::BaseMsgFlags|NFWK::NPTM::NLOG::llLogTrace,  SRC_FUNC,msg __VA_OPT__(,) __VA_ARGS__)

#define OUTMSG(msg,...) NFWK::NPTM::NLOG::LogMessage(NFWK::NPTM::NLOG::CurrLog, NFWK::NPTM::NLOG::BaseMsgModes|NFWK::NPTM::NLOG::lfLineBreak, nullptr,msg __VA_OPT__(,) __VA_ARGS__)
#define OUTTXT(msg,...) NFWK::NPTM::NLOG::LogMessage(NFWK::NPTM::NLOG::CurrLog, NFWK::NPTM::NLOG::BaseMsgModes, nullptr,msg __VA_OPT__(,) __VA_ARGS__)

#ifdef DBGBUILD    // Log only in debug build
#define DBGL    LOGL
#define DBGMSG  LOGMSG
#define DBGFAL  LOGFAL
#define DBGERR  LOGERR
#define DBGWRN  LOGWRN
#define DBGNTE  LOGNTE
#define DBGINF  LOGINF
#define DBGDBG  LOGDBG
#define DBGTRC  LOGTRC
#else
#define DBGL(flg,msg,...)
#define DBGMSG(msg,...)
#define DBGFAL(msg,...)
#define DBGERR(msg,...)
#define DBGWRN(msg,...)
#define DBGNTE(msg,...)
#define DBGINF(msg,...)
#define DBGDBG(msg,...)
#define DBGTRC(msg,...)
#endif
//------------------------------------------------------------------------------------------------------------
// ArgList may be RawTextSize if ArgArray is NULL and lfRawTextMsg is specified
_ninline static sint _fcall LogProc(const achar* Message, void** ArgList, uint32 MsgFlags=BaseMsgFlags, volatile LDSC* Ctx=CurrLog, const char* ProcName=SRC_FUNC)
{
#ifndef NOLOG
// static volatile ULONG  MsgIdx  = 0;
// static volatile UINT64 PrvTime = 0;
// NTSTATUS Status;

 if(!Message)return -1;

 uint MSize  = 0;
 uint LogLvl = MsgFlags & llMask;
 if(LogLvl)
  {
   if(Ctx->MaxLevel >= llLogBadLevel)return -2;   // Skip logging  // Log any unleveled messages only
   if(LogLvl > Ctx->MaxLevel)return -3;    // Excluded
  }

 achar* MPtr = nullptr;
 alignas(sizeof(void*)) achar TmpBuf[1024*2];   // avoid chkstk

 // Split flags:
 uint32 LogFlag = (MsgFlags & lfMask) | (Ctx->LogModes & lfMask);  // Add from LDSC (Usually 0 in LDSC)
 uint32 LogMode = (MsgFlags & lmMask) & (Ctx->LogModes & lmMask);  // LogModes in LDSC have higher priority than in MsgFlags   // Anything that is not enabled in LogModes should stay disabled

 if(!LogMode)return -4;   // NULL text string

 uint32 MIndex = 0;       //_InterlockedIncrement((long*)&MsgIdx);  Ctx->MsgIndex
 if(!(LogFlag & lfRawText))   // Format a normal message
  {
   MPtr = TmpBuf;
   uint Len = 0;
   if(LogFlag & lfLogTime)
    {
     uint64 PrvTime = 0;  // <<<<<<< TODO // NNTDLL::GetAbstractTimeStamp(&PrvTime);           // NNTDLL::GetTicks();
     NCNV::NumToHexStr(PrvTime, 16, TmpBuf, true, &Len); // Ticks time
     MSize += Len;
     TmpBuf[MSize++] = 0x20;
    }
   if(LogFlag & lfLogMsgIdx)
    {
     NCNV::NumToHexStr(MIndex, 8, &TmpBuf[MSize], true, &Len);  // Message Index (For message order detection)
     MSize += Len;
     TmpBuf[MSize++] = 0x20;
    }
   if(LogFlag & lfLogThID)
    {
     uint ThId = 0;  // NtCurrentThreadId()   // Should show PID too, in case of bad Fork?
     NCNV::NumToHexStr(ThId, 6, &TmpBuf[MSize], true, &Len);   // Thread ID
     MSize += Len;
     TmpBuf[MSize++] = 0x20;
    }
   if(LogFlag & lfLogLevel)
    {
     if(LogLvl==llLogFail)*(uint32*)&TmpBuf[MSize] = 'LIAF';  //FAIL      // Mind the byte order!    // NOTE: Cannot do that, alignment problem (ARM)
     else if(LogLvl==llLogError)*(uint32*)&TmpBuf[MSize] = 'RRRE';  //ERRR      // Mind the byte order!
     else if(LogLvl==llLogWarning)*(uint32*)&TmpBuf[MSize] = 'NRAW';  //WARN
     else if(LogLvl==llLogNote)*(uint32*)&TmpBuf[MSize] = 'ETON';  //NOTE
     else if(LogLvl==llLogInfo)*(uint32*)&TmpBuf[MSize] = 'OFNI';  //INFO
     else if(LogLvl==llLogDebug)*(uint32*)&TmpBuf[MSize] = 'GBED';  //DEBG
     else if(LogLvl==llLogTrace)*(uint32*)&TmpBuf[MSize] = 'ECRT';  //TRCE
     else *(uint32*)&TmpBuf[MSize] = 'ENON';  // NONE
     MSize += 4;
     TmpBuf[MSize++] = 0x20;
    }
   if(ProcName && (LogFlag & lfLogName))
    {
     for(;*ProcName;ProcName++,MSize++)TmpBuf[MSize] = *ProcName;
     TmpBuf[MSize++] = 0x20;
    }
   if(MSize)  // Have some logger info
    {
     TmpBuf[MSize++] = '-';
     TmpBuf[MSize++] = '>';
     TmpBuf[MSize++] = 0x20;
    }
   // TODO: Use split buffer (writev) instead of formatting everything to some buffer and dealing with overflows and allocations
   MSize += (uint)NFMT::FormatToBuffer((char*)Message, &TmpBuf[MSize], (sizeof(TmpBuf)-3)-MSize, ArgList);      // May return negative error code?    //    FormatToBuffer(Message, &TmpBuf[MSize], (sizeof(TmpBuf)-3)-MSize, args);  // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<
   if(LogFlag & lfLineBreak)
    {
     TmpBuf[MSize++] = '\r';
     TmpBuf[MSize++] = '\n';
    }
   TmpBuf[MSize]   = 0;  // User`s LogProc may want this
  }
   else  // Log raw text (No line break supported??????????????????????????)
    {
     //MSize = ProcName?((UINT)ProcName):(NSTR::StrLen(Message));  // ProcName is message size  // <<<<<<<<<<<<<<<<<<<<<< TODO !!!!!!!!!!!!!!!!!!!
     //MPtr  = Message;
    }

 if((LogMode & lmProc) && Ctx->pCallback)Ctx->pCallback(MPtr, MSize, LogFlag|LogMode|LogLvl, Ctx);    // pLogProc(MPtr, MSize, LogLvl);
 if(LogMode & lmFile)    // TODO: Scatter write   // TODO: Log update flag
  {
   NPTM::NAPI::write(Ctx->FileHandle, MPtr, MSize);   
  /* if(!hLogFile)  // NOTE: Without FILE_APPEND_DATA offsets must be specified to NtWriteFile
    {          // (LogMode & lmFileUpd)?FILE_APPEND_DATA:FILE_WRITE_DATA       //           //  LogMode |= lmFileUpd;
     Status = NCMN::NNTDLL::FileCreateSync(LogFilePath, FILE_APPEND_DATA, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ|FILE_SHARE_WRITE, (LogMode & lmFileUpd)?FILE_OPEN_IF:FILE_OVERWRITE_IF, FILE_NON_DIRECTORY_FILE, &hLogFile);
    }
     else Status = 0;
   if(!Status)
    {
     LARGE_INTEGER offset = {};
     IO_STATUS_BLOCK iosb = {};
     Status = NtWriteFile(hLogFile, NULL, NULL, NULL, &iosb, MPtr, MSize, &offset, NULL);
    } */
  }
 if(LogMode & lmCons)
  {
   NPTM::NAPI::write(Ctx->ConsHandle, MPtr, MSize);
  // IO_STATUS_BLOCK iosb = {};
  // if(!hConsOut)hConsOut = NtCurrentPeb()->ProcessParameters->StandardOutput;               // Not a real file handle on Windows 7(Real for an redirected IO?)!      // Win8+: \Device\ConDrv\Console     // https://stackoverflow.com/questions/47534039/windows-console-handle-for-con-device
  // if((UINT)hConsOut != 7)Status = NtWriteFile(hConsOut, NULL, NULL, NULL, &iosb, MPtr, MSize, NULL, NULL);        // Will not work until Windows 8
  }
#endif
 return 0;
}
//------------------------------------------------------------------------------------------------------------

};

// Defined in ase namespace
//template<int Flg=0> using LOGFAL = NLOG::LogMessage<Flg>;

