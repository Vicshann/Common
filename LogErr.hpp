
#pragma once
/*
  Copyright (c) 2020 Victor Sheinmann, Vicshann@gmail.com

  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), 
  to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
  and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. 
*/

//---------------------------------------------------------------------------
struct SLog  // Log: Cached(In mem untill it full), File(Can me immediate or cached), Mem(Shared buffer), Raw(Not formatted, dumps all args), Callback(To some user callback)
{
static const int StackBufSize   = (1024 * 3);
static const int LogBufNameLen  = 128;
static const int LogFileNameLen = 256;

struct SArg   // All typical types to log are here  // Assignment must be as cheap as possible  // TODO: Move to FmtBuf
{
enum EArgType {atNone,atInt,atUInt,atInt64,atUInt64,atFlt,atDbl,atPtr,atAStr,atWStr,atData,atExtraData=0x80};
 union 
  {
   int IVal;
   unsigned int UVal;
   __int64 IVal64;
   unsigned __int64 UVal64;
   double DVal;   
   float FVal;
   void* Data;    // A String or a raw data block
   char* AStr; 
   wchar_t* WStr;
  };
 unsigned long Info;  // In bytes
//-------------------------------------
inline SArg(void* Data, unsigned int Size)
{
 this->SetData(Data, Size, atData);
}
//-------------------------------------
inline SArg(char* Str, unsigned int Len)
{
 this->SetData(Str, Len*sizeof(char), atAStr);
}
//-------------------------------------
template<int N> inline SArg(const char (&Str) [N])   
{
 if constexpr (sizeof(Str) <= sizeof(double))     // (N*sizeof(char))
  {
   this->Info |= atAStr|(sizeof(Str) << 8);
   this->DVal  = *(double*)Data;      // Must be readable, leaks extra bytes! 
  } 
   else 
    {
     this->Info |= atAStr|atExtraData|(sizeof(Str) << 8);
     this->AStr  = (char*)reinterpret_cast<const char*>(&Str);
    }
}
//-------------------------------------
inline SArg(char* Str)
{
 if(Str)
  {
   unsigned int Len = 0;
   while(Str[Len])Len++;
   this->SetData(Str, Len*sizeof(char), atAStr);      //this->Info |= (Len*sizeof(char)) << 8;
  }
   else this->Info = atAStr;
}
//-------------------------------------
inline SArg(wchar_t* Str, unsigned int Len)
{
 this->SetData(Str, Len*sizeof(wchar_t), atWStr); 
}
//-------------------------------------
template<int N> inline SArg(const wchar_t (&Str) [N])   
{ 
 if constexpr (sizeof(Str) <= sizeof(double))     // (N*sizeof(wchar_t))
  {   
   this->Info |= atWStr|(sizeof(Str) << 8);
   this->DVal  = *(double*)reinterpret_cast<const wchar_t*>(&Str);      // Must be readable, leaks extra bytes!
  } 
   else 
    {
     this->Info |= atWStr|atExtraData|(sizeof(Str) << 8);
     this->WStr  = (wchar_t*)reinterpret_cast<const wchar_t*>(&Str);
    }
}
//-------------------------------------
inline SArg(wchar_t* Str)
{
 if(Str)
  {
   unsigned int Len = 0;
   while(Str[Len])Len++;
   this->SetData(Str, Len*sizeof(wchar_t), atAStr);     //  this->Info |= (Len*sizeof(wchar_t)) << 8;
  }
   else this->Info = atAStr; 
}
//-------------------------------------
inline SArg(int Val)
{
 this->Info = atInt|(sizeof(Val) << 8);
 this->IVal = Val;
}
//-------------------------------------
inline SArg(unsigned int Val)
{
 this->Info = atUInt|(sizeof(Val) << 8);
 this->UVal = Val;
}
//-------------------------------------
inline SArg(__int64 Val)
{
 this->Info   = atInt64|(sizeof(Val) << 8);
 this->IVal64 = Val;
}
//-------------------------------------
inline SArg(unsigned __int64 Val)
{
 this->Info   = atUInt64|(sizeof(Val) << 8);
 this->UVal64 = Val;
}
//-------------------------------------
inline SArg(float Val)
{
 this->Info = atFlt|(sizeof(Val) << 8);
 this->FVal = Val;
}
//-------------------------------------
inline SArg(double Val)
{
 this->Info = atDbl|(sizeof(Val) << 8);
 this->DVal = Val;
}
//-------------------------------------
inline SArg(void* Val)
{
 this->Info = atPtr|(sizeof(Val) << 8);
 this->Data = Val;  
} 
//-------------------------------------
inline unsigned int GetDataSize(void){return this->Info >> 8;}
inline bool IsExtraData(void){return this->Info & atExtraData;}
//-------------------------------------
inline void SetData(void* Data, unsigned int Size, EArgType Type)
{
 if(Data && Size)
  {
   if(Size <= sizeof(double))
    {
     this->Info = Type|(Size << 8);
     this->DVal = *(double*)Data;      // Must be readable, leaks extra bytes!     
    } 
     else 
      {
       this->Info = Type|atExtraData|(Size << 8);
       this->Data = Data;
      }
  }
   else this->Info = Type;
}
//-------------------------------------

};

enum ELogFlags {
// Log levels
         llInfo,     // Any imformation, like some statistics
         llDebug,    // A debugging message
         llError,    // Report and continue
         llFailure,  // Report and terminate self

         lfUpdFile,  // Do not overwrite an existing log file
};

enum CloseFlags {cfFile=1,cfConsole=2,cfBuffer=4};
typedef int (__fastcall *TLOGPROC)(class CLogger* Log, unsigned int Flags, SArg* Array);   // MSFASTCALL uses only ECX and EDX
//===========================================================================
TLOGPROC pLogProc;
void* ConHandle;       // Buffer ptr or a file/console handle
void* FileHandle;
void* LogBuffer;       // class 
unsigned int Flags;
//===========================================================================
static inline SLog* GetSingletonLog(void)
{
 static SLog DefaultLog;
 return &DefaultLog;
}
//---------------------------------------------------------------------------
static inline SLog*& GetDefaultLog(void)    // Assignable
{
 static SLog* DefLogPtr = GetSingletonLog();
 return DefLogPtr;
}
//---------------------------------------------------------------------------
static inline unsigned int GetCountFromFlags(unsigned int Flags){return (unsigned short)Flags;}
//---------------------------------------------------------------------------
static inline unsigned int GetExtraDataSize(unsigned int Cnt, SArg* Array)
{
 unsigned int Size = 0;
 for(unsigned int ctr=0;ctr < Cnt;ctr++)Size += Array[ctr].GetDataSize();
 return Size;
}
//---------------------------------------------------------------------------
static inline unsigned int WriteExtraData(unsigned int Cnt, SArg* Array, unsigned char* DstBuf, unsigned int BufSize)
{
 unsigned int Size = 0;
 for(unsigned int ctr=0;ctr < Cnt;ctr++)
  {
   SArg* Arg = &Array[ctr];
   if(!Arg->IsExtraData())continue;
   unsigned int Len = Arg->GetDataSize();
   if(!Len)continue;
   if(Len > BufSize)break;  // No more space in the buffer!
   memcpy(DstBuf, Array[ctr].Data, Len);
   BufSize -= Len;
   DstBuf  += Len;
   Size    += Len;
  }
 return Size;
}
//---------------------------------------------------------------------------
// An unnamed buffer is not a shared memory
static inline int CreateLogBuffer(unsigned int Size, char* Name=nullptr)
{
 DestroyLogBuffer();

 return 0;
}
//---------------------------------------------------------------------------
static inline int DestroyLogBuffer(void)
{

 return 0;
}
//===========================================================================
int Initialize(void)
{

}
//---------------------------------------------------------------------------
int Close(void)
{

};
//---------------------------------------------------------------------------
// On Windows Use AllocConsole if currently no console created for calling process
int OpenConsole(void)
{

}
//---------------------------------------------------------------------------
int OpenFile(void)
{

}
//---------------------------------------------------------------------------

//
//
//---------------------------------------------------------------------------
class CLogger
{

public:
template<typename... Args> static int LogProc(unsigned int Flags, const Args&... arg)         // char* CallerName, char* MsgFmt,
{
 SArg ArgArr[] = {arg...};
 return pLogProc->(this, Flags & sizeof...(arg) ,ArgArr); 
}
//---------------------------------------------------------------------------

};
//===========================================================================




static inline char* GetLogBufName(void)
{
 char BufName[LogBufNameLen] = {0};
 return BufName;
}
//---------------------------------------------------------------------------
static inline void*& GetLogBufPtr(void)
{
 static void* BufPtr = nullptr;
 return BufPtr;
}
//---------------------------------------------------------------------------
static inline unsigned int& GetLogBufLen(void)
{
 static unsigned int BufSize = 0;
 return BufSize;
}
//---------------------------------------------------------------------------
static inline TLOGPROC& CurrentLogProc(void)
{
 static TLOGPROC pLogProc = 0;//LogProcDummy;
 return pLogProc;
}
//---------------------------------------------------------------------------
template<typename... Args> static int LogProc(unsigned int Flags, const Args&... arg)         // char* CallerName, char* MsgFmt,
{
 SArg ArgArr[] = {arg...};
 return 0;//CurrentLogProc()(this, Flags & sizeof...(arg) ,ArgArr);   //   LogProcInternal(Flags & sizeof...(arg), CallerName, MsgFmt, ArgArr);
}
//---------------------------------------------------------------------------
static int __fastcall LogProcDummy(CLogger* Log, unsigned int Flags, SArg* Array){return 0;}
//---------------------------------------------------------------------------
// Async logging is always raw
// Logs to a current buffer which may be a shared memory
static int __fastcall LogProcBufRaw(CLogger* Log, unsigned int Flags, SArg* Array)   // To a local memory buffer
{

 return Flags & 0xFFFF;
}
//---------------------------------------------------------------------------
static int __fastcall LogProcFileRaw(CLogger* Log, unsigned int Flags, SArg* Array)    // Used a limited stack buffer
{

 return Flags & 0xFFFF;
}
//---------------------------------------------------------------------------








};
//---------------------------------------------------------------------------
