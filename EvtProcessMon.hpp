
#pragma once

/*
  Copyright (c) 2018 Victor Sheinmann, Vicshann@gmail.com

  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), 
  to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
  and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. 
*/

//#define WIN32_LEAN_AND_MEAN		 // Exclude rarely-used stuff from Windows headers
//#define _WIN32_WINNT  0x0501     // For CoInitializeEx support

#include <windows.h>
#include <wmistr.h>
#include <evntrace.h>
#include <Evntcons.h>

//------------------------------------------------------------------------------------------------------------
// NOTE: Processes that make use of ETW require debugging privileges.
// NOTE: WMI Win32_Process is implemented in same way (see KrnlProv.dll)
class CETWProcMon
{
 static const unsigned int ETW_KRNLPR_IMAGE   = 0x00000040;
 static const unsigned int ETW_KRNLPR_THREAD  = 0x00000020;
 static const unsigned int ETW_KRNLPR_PROCESS = 0x00000010;

 static const inline BYTE KrnlProcGUID[] = {0xD6,0x2C,0xFB,0x22,0x7B,0x0E,0x2B,0x42,0xA0,0xC7,0x2F,0xAD,0x1F,0xD0,0xE7,0x16};
//                                                                                                                                            id ver chn  lev opc tsk Keyword
 static const inline EVENT_DESCRIPTOR EvtImageUnLoad  = {6, 0,  0x10, 4,  0,  6,  0x8000000000000040};   // _ImageUnload    EVENT_DESCRIPTOR <6, 0,  10h, 4,  ?,  6,  8000000000000040h>   // Guessed
 static const inline EVENT_DESCRIPTOR EvtImageLoad    = {5, 0,  0x10, 4,  0,  5,  0x8000000000000040};   // _ImageLoad      EVENT_DESCRIPTOR <5, 0,  10h, 4,  0,  5,  8000000000000040h>
 static const inline EVENT_DESCRIPTOR EvtThreadStop   = {4, 1,  0x10, 4,  2,  4,  0x8000000000000020};   // _ThreadStop     EVENT_DESCRIPTOR <4, 1,  10h, 4,  2,  4,  8000000000000020h>
 static const inline EVENT_DESCRIPTOR EvtThreadStart  = {3, 1,  0x10, 4,  1,  3,  0x8000000000000020};   // _ThreadStart    EVENT_DESCRIPTOR <3, 1,  10h, 4,  1,  3,  8000000000000020h>
 static const inline EVENT_DESCRIPTOR EvtProcessStop  = {2, 1,  0x10, 4,  2,  2,  0x8000000000000010};   // _ProcessStop    EVENT_DESCRIPTOR <2, 1,  10h, 4,  2,  2,  8000000000000010h>
 static const inline EVENT_DESCRIPTOR EvtProcessStart = {1, 1,  0x10, 4,  2,  1,  0x8000000000000010};   // _ProcessStart   EVENT_DESCRIPTOR <1, 1,  10h, 4,  ?,  1,  8000000000000010h>   // Guessed  // Ver expected <= 2

#pragma pack(push,1)
struct SKrnlPrImage
{
 SIZE_T  UnkA;       // 0
 SIZE_T  UnkB;       // 1
 DWORD   UnkC;       // 2
 int     UnkD;
 int     UnkE;
 int     UnkF;
 wchar_t Name[1];    // 3
};
struct SKrnlPrThreadBeg
{
 DWORD  UnkA;    // 6
 DWORD  UnkB;    // 7
 SIZE_T UnkC;    // 0
 SIZE_T UnkD;    // 1
 SIZE_T UnkE;    // 2
 SIZE_T UnkF;    // 3
 SIZE_T UnkG;    // 4
 SIZE_T UnkH;    // 5
};
struct SKrnlPrThreadEnd
{
 DWORD  UnkA;    // 6
 DWORD  UnkB;    // 7
};
struct SKrnlPrProcess
{


};
#pragma pack(pop)

 bool ActiveWrk; 
 HANDLE hTraceTh;
 UINT TraceEvtMsk;
 TRACEHANDLE TrcHndl;
 TRACEHANDLE SesHndl;
 struct
  {
   EVENT_TRACE_PROPERTIES Props;
   wchar_t NameBuf[MAX_PATH*2];    // 1040
  }PropBuf; 
// wchar_t sSessNam[MAX_PATH];  
//------------------------------------------------------------------------------------------------------------
static VOID WINAPI EventRecordCallback(EVENT_RECORD* pEventRecord)  // This is where you would get the details of ETW event
{
 if(!pEventRecord || !pEventRecord->UserContext)return;
 CETWProcMon* This = (CETWProcMon*)pEventRecord->UserContext;
 if(memcmp(&pEventRecord->EventHeader.ProviderId, &KrnlProcGUID, sizeof(GUID)))return;  // Not our provider of interest!

 if(This->ProcessCallback)This->ActiveWrk = This->ProcessCallback(pEventRecord, 0);
}
//------------------------------------------------------------------------------------------------------------
static ULONG WINAPI BufferCallback(EVENT_TRACE_LOGFILEW* pLogFile) 
{     
 return ((CETWProcMon*)pLogFile->Context)->ActiveWrk;
}
//------------------------------------------------------------------------------------------------------------
static DWORD WINAPI ProcessTraceThreadProc(LPVOID This)
{                               
 while(((CETWProcMon*)This)->ActiveWrk)
  {
   if(ProcessTrace(&((CETWProcMon*)This)->TrcHndl, 1, NULL, NULL))return 1;   // calling thread will be blocked until BufferCallback returns FALSE or all events are delivered or CloseTrace is called
  }
 return 0;
}
//------------------------------------------------------------------------------------------------------------


public:
PVOID ImageCallback;
PVOID ThreadCallback;
bool (_fastcall *ProcessCallback)(EVENT_RECORD* Evt, bool Dir);

CETWProcMon(void)
{
 memset(this, 0, sizeof(CETWProcMon));
 this->PropBuf.Props.Wnode.BufferSize  = sizeof(this->PropBuf);
 this->PropBuf.Props.Wnode.Flags       = 0x20000;
 this->PropBuf.Props.LogFileNameOffset = sizeof(EVENT_TRACE_PROPERTIES);  // 120, no alignment problem
 this->PropBuf.Props.LoggerNameOffset  = sizeof(EVENT_TRACE_PROPERTIES)+(MAX_PATH * sizeof(WCHAR));
}
~CETWProcMon(){this->StopTracing();}
//------------------------------------------------------------------------------------------------------------
int StopTracing(void)
{
 if(!this->ActiveWrk)return 1;    // Not active
 this->ActiveWrk = false;
 CloseTrace(this->TrcHndl);
// EnableTraceEx((LPCGUID)&KrnlProcGUID, NULL, this->SesHndl, EVENT_CONTROL_CODE_DISABLE_PROVIDER, 0, 0, 0, 0, NULL);    // Not in KrnlProv.dll
 StopTraceW(this->SesHndl, this->PropBuf.NameBuf, &this->PropBuf.Props);     //  this->PropBuf.NameBuf is initialized ??????????
 if(this->hTraceTh)
  {
   WaitForSingleObject(this->hTraceTh, INFINITE);
   CloseHandle(this->hTraceTh);
   this->hTraceTh = NULL;
  }
 return 0;
}
//------------------------------------------------------------------------------------------------------------
//   Microsoft-Windows-Kernel-File            {EDD08927-9CC4-4E65-B970-C2560FB5C289}   // From Win7 
//   Microsoft-Windows-Kernel-Process         {22FB2CD6-0E7B-422B-A0C7-2FAD1FD0E716}   // From Vista
//
int StartTracing(PWSTR SessName=NULL)
{ 
 if(this->ActiveWrk)return -1;    // Already active
 EVENT_TRACE_LOGFILEW loggerInfo = {};

 this->TraceEvtMsk = ETW_KRNLPR_PROCESS;           // All: 0x70
// if(this->ImageCallback)this->TraceEvtMsk |= ETW_KRNLPR_IMAGE;
// if(this->ThreadCallback)this->TraceEvtMsk |= ETW_KRNLPR_THREAD;
// if(this->ProcessCallback)this->TraceEvtMsk |= ETW_KRNLPR_PROCESS;
// if(!this->TraceEvtMsk)return -2;

 if(!SessName)SessName = L"EvtProcMon"; // NOTE: Session name is a global object and will not be deleted automatically on the process termination
// lstrcpynW(this->sSessNam,SessName,sizeof(this->sSessNam));
 if(QueryTraceW(NULL, SessName, &this->PropBuf.Props))
  { 
   this->PropBuf.Props.LogFileMode    = EVENT_TRACE_REAL_TIME_MODE;
   this->PropBuf.Props.BufferSize     = 64;
   this->PropBuf.Props.MinimumBuffers = 20;
   this->PropBuf.Props.MaximumBuffers = 200;
   this->PropBuf.Props.FlushTimer     = 1;       
   lstrcpynW(&this->PropBuf.NameBuf[MAX_PATH], SessName, MAX_PATH);                                                          
   if(StartTraceW(&this->SesHndl, SessName, &this->PropBuf.Props))return -3;
   EnableTraceEx((LPCGUID)&KrnlProcGUID, NULL, this->SesHndl, EVENT_CONTROL_CODE_ENABLE_PROVIDER, 255, this->TraceEvtMsk, 0, EVENT_ENABLE_PROPERTY_SID, NULL);  // Do EVENT_CONTROL_CODE_DISABLE_PROVIDER on termination?                   
  }
   else this->SesHndl = NULL;
 loggerInfo.Context = this;
 loggerInfo.LoggerName = SessName;
 loggerInfo.ProcessTraceMode = PROCESS_TRACE_MODE_EVENT_RECORD | PROCESS_TRACE_MODE_REAL_TIME | PROCESS_TRACE_MODE_RAW_TIMESTAMP;
// loggerInfo.BufferCallback   = BufferCallback;          
 loggerInfo.EventRecordCallback = EventRecordCallback;    // provide a callback whenever we get an event record
 
 this->TrcHndl = OpenTraceW(&loggerInfo);
 if(INVALID_PROCESSTRACE_HANDLE == this->TrcHndl)return -4;
 DWORD ThreadId  = 0;
 this->ActiveWrk = true;
 this->hTraceTh  = CreateThread(0, 0, ProcessTraceThreadProc, (LPVOID)this, CREATE_SUSPENDED, &ThreadId);
 if(!this->hTraceTh){this->StopTracing(); return -5;}
 SetThreadPriority(this->hTraceTh, THREAD_PRIORITY_TIME_CRITICAL);
 ResumeThread(this->hTraceTh);
 return 0;
}
//------------------------------------------------------------------------------------------------------------

};
//------------------------------------------------------------------------------------------------------------

