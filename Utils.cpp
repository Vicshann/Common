
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

#include "Utils.h"
#include "FormatPE.h"
//====================================================================================

HANDLE hLogFile = INVALID_HANDLE_VALUE;
HANDLE hConsOut = INVALID_HANDLE_VALUE;
HANDLE hConsErr = INVALID_HANDLE_VALUE;

BYTE   LogFilePath[MAX_PATH];	
int    LogMode = lmNone;	
void _cdecl DummyLogProc(LPSTR, UINT){}
void (_cdecl *pLogProc)(LPSTR, UINT) = DummyLogProc;			   
			   			   
//---------------------------------------------------------------------------
#ifndef __BORLANDC__

#pragma function(memcmp, memset, memcpy, memmove)
//---------------------------------------------------------------------------
void*  __cdecl memmove(void* _Dst, const void* _Src, size_t _Size)
{
  if((char*)_Dst <= (char*)_Src)return memcpy(_Dst,_Src,_Size);
  size_t ALen = _Size/sizeof(size_t);
  size_t BLen = _Size%sizeof(size_t);
  for(size_t ctr=_Size-1;BLen > 0;ctr--,BLen--)((char*)_Dst)[ctr] = ((char*)_Src)[ctr]; 
  for(size_t ctr=ALen-1;ALen > 0;ctr--,ALen--) ((size_t*)_Dst)[ctr] = ((size_t*)_Src)[ctr];  
 return _Dst;
} 
//---------------------------------------------------------------------------
void*  __cdecl memcpy(void* _Dst, const void* _Src, size_t _Size)
{
 size_t ALen = _Size/sizeof(size_t);
 size_t BLen = _Size%sizeof(size_t);
 for(size_t ctr=0;ctr < ALen;ctr++)((size_t*)_Dst)[ctr] = ((size_t*)_Src)[ctr]; 
 for(size_t ctr=(ALen*sizeof(size_t));ctr < _Size;ctr++)((char*)_Dst)[ctr] = ((char*)_Src)[ctr];  
 return _Dst;
} 
//---------------------------------------------------------------------------
void*  __cdecl memset(void* _Dst, int _Val, size_t _Size)
{
 for(size_t ctr=0;ctr<_Size;ctr++)((PBYTE)_Dst)[ctr] = _Val;
 return _Dst;
} 
//---------------------------------------------------------------------------
int    __cdecl memcmp(const void* _Buf1, const void* _Buf2, size_t _Size) // '(*((ULONG**)&_Buf1))++;'	// TODO: Using Ptr increment will be faster than indexing?
{
 unsigned char* BufA = (unsigned char*)_Buf1;
 unsigned char* BufB = (unsigned char*)_Buf2; 
 for(;_Size >= sizeof(size_t); _Size-=sizeof(size_t), BufA+=sizeof(size_t), BufB+=sizeof(size_t)){if(*((size_t*)BufA) != *((size_t*)BufB))return (*((size_t*)BufA) - *((size_t*)BufB));}  // Enters here only if Size >= sizeof(ULONG)
 for(;_Size > 0; _Size--, BufA++, BufB++)
  {
   if(*((unsigned char*)BufA) != *((unsigned char*)BufB)){return (*((unsigned char*)BufA) - *((unsigned char*)BufB));}
  }			   // Enters here only if Size > 0
 return 0; 
} 
//---------------------------------------------------------------------------
int __cdecl stricmp(char const* String1,char const* String2)
{                 
 return lstrcmpiA(String1,String2);
}
//---------------------------------------------------------------------------
char* __cdecl strncpy(char* _Destination, char const* _Source, size_t _Count)
{
 return lstrcpynA(_Destination, _Source, _Count);
}
//---------------------------------------------------------------------------
extern "C" void* __cdecl malloc(size_t _Size)
{      
 return HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,_Size);
}
//---------------------------------------------------------------------------
extern "C" void* __cdecl calloc(size_t _Count, size_t _Size)
{
 return HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,_Size*_Count);
}
//---------------------------------------------------------------------------
extern "C" void __cdecl free(void* _Block)
{
 HeapFree(GetProcessHeap(),0,_Block);
}
//---------------------------------------------------------------------------
extern "C" __time64_t __cdecl _mktime64(_Inout_ struct tm* _Tm)
{
 return 0;   // No support for now    // We have to use some ancient third party code  // TODO: Integrate everything useful in a lightweight header-only framework
}
//---------------------------------------------------------------------------
extern "C" void __cdecl tzset(void)   // It is not even for for Windows :)
{

}
//---------------------------------------------------------------------------
void* __cdecl operator new(size_t n)      
{
 return HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,n);
}
//---------------------------------------------------------------------------
void* __cdecl operator new[](size_t n) 
{                
 return HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,n);
}
//---------------------------------------------------------------------------
void __cdecl operator delete(void* p)
{
 HeapFree(GetProcessHeap(),0,p);
}
//---------------------------------------------------------------------------
void __cdecl operator delete(void* p, size_t n)
{
 HeapFree(GetProcessHeap(),0,p);
}
//---------------------------------------------------------------------------
void __cdecl operator delete[](void* p)
{
 HeapFree(GetProcessHeap(),0,p);
}
//---------------------------------------------------------------------------

#ifndef NOMINIRTL 

extern "C" int _fltused = 0;

#ifndef _M_X64
extern "C" __declspec(naked) float _cdecl _CIsqrt(float &Value)
{
 __asm FSQRT
 __asm RET
}
//---------------------------------------------------------------------------
extern "C" __declspec(naked) long _cdecl _ftol2(float &Value)
{
 __asm PUSH EAX
 __asm PUSH EAX
 __asm FISTP QWORD PTR [ESP]
 __asm POP  EAX
 __asm POP  EDX
 __asm RET
}
//---------------------------------------------------------------------------
/*extern "C" __declspec(naked) unsigned __int64 _cdecl _aullshr(unsigned __int64 Value, unsigned int Shift)  // MS calls this function as _fastcall here!!!!
{
 __asm {
    CMP CL,64
    JNB SHORT RETZERO
    CMP CL,32
    JNB SHORT MORE32
    SHRD EAX,EDX,CL                       
    SHR EDX,CL
    RETN
MORE32:
    MOV EAX,EDX                             
    XOR EDX,EDX                           
    AND CL,0x1F
    SHR EAX,CL
    RETN
RETZERO:
    XOR EAX,EAX
    XOR EDX,EDX                            
    RETN
}
 /*
 if(Shift >= 64)return 0;
 if(Shift >= 32)return ((unsigned int*)&Value)[0] >> (Shift & 0x1F);
 unsigned int Lo = ((unsigned int*)&Value)[0];
 unsigned int Hi = ((unsigned int*)&Value)[1];
 Lo >>= Shift;
 Lo |= (Hi << (32-Shift));
 Hi >>= Shift;
 ((unsigned int*)&Value)[0] = Lo;
 ((unsigned int*)&Value)[1] = Hi;
 return Value;*/
/*}
//---------------------------------------------------------------------------
extern "C" __declspec(naked) unsigned __int64 _cdecl _allmul(unsigned __int64 multiplicand, unsigned __int64 multiplier)
{
 __asm {
    MOV EAX,DWORD PTR [ESP+0x08]
    MOV ECX,DWORD PTR [ESP+0x10]
    OR  ECX,EAX
    MOV ECX,DWORD PTR [ESP+0x0C]
    JNZ SHORT Part2
    MOV EAX,DWORD PTR [ESP+0x04]             
    MUL ECX
    RETN 
Part2:
    PUSH EBX
    MUL ECX
    MOV EBX,EAX
    MOV EAX,DWORD PTR [ESP+0x08]
    MUL DWORD PTR [ESP+0x14]
    ADD EBX,EAX
    MOV EAX,DWORD PTR [ESP+0x08]
    MUL ECX
    ADD EDX,EBX
    POP EBX                                
	RETN
 }
}
//---------------------------------------------------------------------------
*/
//---------------------------------------------------------------------------
extern "C" __declspec(naked) unsigned __int64 _cdecl _aullshr(unsigned __int64 Value, unsigned int Shift)  // MS calls this function as _fastcall !!!!   EDX:EAX, ECX    // ret
{
 __asm
 {
  push ECX
  push EDX
  push EAX
  call ShrULL
  retn
 }
 //return ShrULL(Value, Shift);
}
//---------------------------------------------------------------------------
extern "C" __declspec(naked) unsigned __int64 _cdecl _allshl(unsigned __int64 Value, unsigned int Shift)   // MS calls this function as _fastcall !!!!   EDX:EAX, ECX    // ret
{
 __asm
 {
  push ECX
  push EDX
  push EAX
  call ShlULL
  retn
 }
 //return ShlULL(Value, Shift);
}
//---------------------------------------------------------------------------
extern "C" __declspec(naked) unsigned __int64 _cdecl _allmul(unsigned __int64 multiplicand, unsigned __int64 multiplier)  // MS calls this function as _stdcall !!!! // ret 16 
{
 __asm jmp BinLongUMul
 //return BinLongUMul(multiplicand, multiplier);   // No sign? !!!!!!
}
//---------------------------------------------------------------------------
/*ULONGLONG __stdcall BinLongUDivStub(ULONGLONG Dividend, ULONGLONG Divisor)
{
 return BinLongUDiv(Dividend, Divisor);
}
*/
extern "C" __declspec(naked) unsigned __int64 _cdecl _aulldiv(unsigned __int64 dividend, unsigned __int64 divisor)  // MS calls this function as _stdcall !!!! // ret 16
{
__asm
{
    PUSH EBX
    PUSH ESI
    MOV EAX,DWORD PTR [ESP+0x18]
    OR EAX,EAX
    JNZ Lbl_77CA4682
    MOV ECX,DWORD PTR [ESP+0x14]
    MOV EAX,DWORD PTR [ESP+0x10]
    XOR EDX,EDX                           
    DIV ECX
    MOV EBX,EAX
    MOV EAX,DWORD PTR [ESP+0x0C]
    DIV ECX
    MOV EDX,EBX
    JMP Lbl_77CA46C3
Lbl_77CA4682:
    MOV ECX,EAX
    MOV EBX,DWORD PTR [ESP+0x14]
    MOV EDX,DWORD PTR [ESP+0x10]
    MOV EAX,DWORD PTR [ESP+0x0C]
Lbl_77CA4690:
    SHR ECX,1
    RCR EBX,1
    SHR EDX,1
    RCR EAX,1
    OR ECX,ECX
    JNZ Lbl_77CA4690
    DIV EBX
    MOV ESI,EAX
    MUL DWORD PTR [ESP+0x18]
    MOV ECX,EAX
    MOV EAX,DWORD PTR[ESP+0x14]
    MUL ESI
    ADD EDX,ECX
    JB  Lbl_77CA46BE
    CMP EDX,DWORD PTR [ESP+0x10]
    JA  Lbl_77CA46BE
    JB  Lbl_77CA46BF
    CMP EAX,DWORD PTR [ESP+0x0C]
    JBE Lbl_77CA46BF
Lbl_77CA46BE:
    DEC ESI
Lbl_77CA46BF:
    XOR EDX,EDX                          
    MOV EAX,ESI
Lbl_77CA46C3:
    POP ESI                                
    POP EBX  
    RETN 16                          
}
}


// __asm jmp BinLongUDivStub
 //return BinLongUDiv(dividend, divisor);

#endif
#endif
#else    // __BORLANDC__
//---------------------------------------------------------------------------
__declspec(naked) void _stdcall fix__fpreset(void)  // Fix conflict of BDS and TTS in FPU
{
 __asm             // Must be naked - compiler is VERY stupid
 {
  push  0x027F     // Normal FPU status
  fldcw [ESP]
  pop   EAX
  retn
 }
}
#endif
//---------------------------------------------------------------------------
bool _stdcall IsLogHandle(HANDLE Hnd)
{
 return ((Hnd == hLogFile)||(Hnd == hConsOut)||(Hnd == hConsErr));
}
//---------------------------------------------------------------------------
// OPt: InterprocSync, Reopen/Update log // Depth // Fastcall, Max 3 constant args
//
void  _cdecl LogProc(char* ProcName, char* Message, ...)
{
 struct SLogger
  {
   static void _cdecl DoLogFile(HANDLE none1, PVOID data, DWORD datalen, PDWORD none2, PVOID none3){pLogProc((LPSTR)data,datalen);}  
  };
static CRITICAL_SECTION csec;
 if(!csec.DebugInfo)InitializeCriticalSection(&csec);

 if(!LogMode/* || !Message || !*Message*/)return;
 int msglen;
 LPSTR MsgPtr;
 DWORD Result;
 va_list args;
 BYTE Buffer[1025+256];     //  > 4k creates __chkstk   // 1024 is MAX for wsprintf
 BYTE OutBuffer[1025+256];

 va_start(args,Message);
 if(!ProcName || ((size_t)ProcName > 65536))
  {
 Buffer[0] = 0;
 if(ProcName && Message && Message[0])
  {
#ifdef LOGTHID
#ifdef LOGTICK
   wsprintf((LPSTR)&Buffer,"%08X %06X %s -> ",GetTickCount(),GetCurrentThreadId(),ProcName);
#else
   wsprintf((LPSTR)&Buffer,"%04X %s -> "GetCurrentThreadId(),ProcName);
#endif
#else
#ifdef LOGTICK
   wsprintf((LPSTR)&Buffer,"%08X %s -> ",GetTickCount(),ProcName);
#else
   lstrcat((LPSTR)&Buffer,ProcName);
   lstrcat((LPSTR)&Buffer," -> ");
#endif  
#endif
  }
 if(Message)
  {
   lstrcat((LPSTR)&Buffer,Message);
   lstrcat((LPSTR)&Buffer,"\r\n");
   msglen = wvsprintf((LPSTR)&OutBuffer,(LPSTR)&Buffer,args);   // TODO: Local implementation with Float types support
   MsgPtr = (LPSTR)&OutBuffer;
  }
   else {msglen = 0; MsgPtr = NULL;}
  }
   else
	{
	 if(Message)msglen = va_arg(args,size_t);  //(size_t)ProcName;
	 MsgPtr = Message;
	}
 EnterCriticalSection(&csec);
 if(LogMode & lmCons){if(hConsOut == INVALID_HANDLE_VALUE){hConsOut = GetStdHandle(STD_OUTPUT_HANDLE); SetConsoleTextAttribute(hConsOut,FOREGROUND_YELLOW);}}
 if(LogMode & lmSErr){if(hConsErr == INVALID_HANDLE_VALUE){hConsErr = GetStdHandle(STD_ERROR_HANDLE);}}
 if(LogMode & lmFile)
  {
   if(hLogFile == INVALID_HANDLE_VALUE)
    {
     hLogFile = CreateFile((LPSTR)&LogFilePath,GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,((LogMode & lmFileUpd)?(OPEN_ALWAYS):(CREATE_ALWAYS)),FILE_ATTRIBUTE_NORMAL|FILE_FLAG_SEQUENTIAL_SCAN,NULL); 
     if(LogMode & lmFileUpd)SetFilePointer(hLogFile,0,NULL,FILE_END);
    }
  } 
 if(msglen > 0)
  {
   if(LogMode & lmProc)pLogProc(MsgPtr,msglen);
   if(LogMode & lmCons)WriteConsole(hConsOut,MsgPtr,msglen,&Result,NULL);
   if(LogMode & lmSErr)WriteConsole(hConsErr,MsgPtr,msglen,&Result,NULL);
   if(LogMode & lmFile)WriteFile(hLogFile,MsgPtr,msglen,&Result,NULL);
  }
  else if(!Message)     // HEX dump
   {
    DWORD Size = va_arg(args,size_t);
    PBYTE Buff = va_arg(args,PBYTE);
    UINT  CPos = 0;
    while(Size > 0)
     {
      UINT len = 0;
	  for(;(CPos < 512)&&(Size > 0);CPos++,Size--)
       {
        WORD chr      = HexToChar(Buff[CPos]);
        Buffer[len++] = chr;
        Buffer[len++] = chr >> 8;
       }
      if(LogMode & lmProc)pLogProc((LPSTR)&Buffer,len);
      if(LogMode & lmCons)WriteConsole(hConsOut,&Buffer,len,&Result,NULL);
      if(LogMode & lmSErr)WriteConsole(hConsErr,&Buffer,len,&Result,NULL);
      if(LogMode & lmFile)WriteFile(hLogFile,&Buffer,len,&Result,NULL);
	 }
    if(LogMode & lmProc)pLogProc("\r\n",2);
    if(LogMode & lmCons)WriteConsole(hConsOut,"\r\n",2,&Result,NULL);
    if(LogMode & lmSErr)WriteConsole(hConsErr,"\r\n",2,&Result,NULL);
    if(LogMode & lmFile)WriteFile(hLogFile,"\r\n",2,&Result,NULL);
   }
 LeaveCriticalSection(&csec);
 va_end(args);
}
//---------------------------------------------------------------------------
void _stdcall SetINIValueInt(LPSTR SectionName, LPSTR ValueName, int Value, LPSTR FileName)
{
 BYTE Buffer[128];
 wsprintf((LPSTR)&Buffer,"%i",Value);
 WritePrivateProfileString(SectionName,ValueName,(LPSTR)&Buffer,FileName);
}
//---------------------------------------------------------------------------
int _stdcall RefreshINIValueInt(LPSTR SectionName, LPSTR ValueName, int Default, LPSTR FileName)
{
#ifdef DEMOVER
 int  Result = Default;
#else
 BYTE Buffer[128];
 int  Result = GetPrivateProfileInt(SectionName,ValueName,Default,FileName);
 wsprintf((LPSTR)&Buffer,"%i",Result);
 WritePrivateProfileString(SectionName,ValueName,(LPSTR)&Buffer,FileName);
#endif
 return Result;
}
//---------------------------------------------------------------------------
int _stdcall RefreshINIValueStr(LPSTR SectionName, LPSTR ValueName, LPSTR Default, LPSTR RetString, DWORD Size, LPSTR FileName)
{
#ifdef DEMOVER
 lstrcpy(RetString,Default);
 int Result = Size;
#else
 int  Result = GetPrivateProfileString(SectionName, ValueName, Default, RetString, Size, FileName);
 WritePrivateProfileString(SectionName,ValueName,RetString,FileName);
#endif
 return Result;

}
//---------------------------------------------------------------------------
int _stdcall RefreshINIValueStr(LPSTR SectionName, LPSTR ValueName, LPSTR Default, PWSTR RetString, DWORD Size, LPSTR FileName)
{
 BYTE Buffer[999];
#ifdef DEMOVER
 lstrcpy((LPSTR)&Buffer,Default);
 int res = Size;
#else
 int res = RefreshINIValueStr(SectionName, ValueName, Default, (LPSTR)&Buffer, sizeof(Buffer), FileName);
#endif
 return MultiByteToWideChar(CP_ACP,0,(LPSTR)&Buffer,res,RetString,Size);
}
//---------------------------------------------------------------------------
bool _stdcall IsAddrInModule(PVOID Addr, PVOID ModBase, UINT ModSize)
{
 if((PBYTE)Addr < (PBYTE)ModBase)return false;
 if((PBYTE)Addr >= &((PBYTE)ModBase)[ModSize])return false;
 return true;
}
//---------------------------------------------------------------------------
UINT _stdcall TrimFilePath(LPSTR FullPath)
{
 int ctr = lstrlen(FullPath)-1;
 for(;ctr > 0;ctr--){if((FullPath[ctr] == PATHDLMR)||(FullPath[ctr] == PATHDLML)){FullPath[ctr+1] = 0;return ctr+1;}}  
 return 0;
}
//---------------------------------------------------------------------------
void _stdcall CreateDirectoryPath(LPSTR Path) // Must end with '\\', may contain a filename at the end
{
 BYTE FullPath[MAX_PATH];

 lstrcpy((LPSTR)&FullPath, Path);     // TODO: Templated StrCpy
 for(int Count=0;FullPath[Count] != 0;Count++)
  {
   if((FullPath[Count]==PATHDLML)||(FullPath[Count]==PATHDLMR))
    {
     FullPath[Count] = 0;
	 CreateDirectory((LPSTR)&FullPath, NULL); // Faster Always create or Test it`s existance first ?
     FullPath[Count] = PATHDLML;
    } 
  }
}
//---------------------------------------------------------------------------
void _stdcall CreateDirectoryPathW(PWSTR Path) // Must end with '\\', may contain a filename at the end
{
 WCHAR FullPath[MAX_PATH];
 lstrcpyW((PWSTR)&FullPath, Path);
 for(int Count=0;FullPath[Count] != 0;Count++)
  {
   if((FullPath[Count]==PATHDLML)||(FullPath[Count]==PATHDLMR))
	{
	 FullPath[Count] = 0;
	 CreateDirectoryW((PWSTR)&FullPath, NULL); // Faster Always create or Test it`s existance first ?
	 FullPath[Count] = PATHDLML;
	}
  }
}
//---------------------------------------------------------------------------
UINT64 _stdcall QueryFileSize(LPSTR File)
{
 LARGE_INTEGER FileSize;
 HANDLE hFile = CreateFile(File,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL|FILE_FLAG_SEQUENTIAL_SCAN,NULL);
 if(hFile == INVALID_HANDLE_VALUE)return false;
 int res = GetFileSizeEx(hFile,&FileSize);
 CloseHandle(hFile);
 if(res < 0)return 0;
 return FileSize.QuadPart;
}
//---------------------------------------------------------------------------
SIZE_T _stdcall GetRealModuleSize(PVOID ModuleBase)
{
 MEMORY_BASIC_INFO meminfo;

 SIZE_T ModuleSize = 0;
 PBYTE  ModuleAddr = (PBYTE)ModuleBase;
 memset(&meminfo, 0, sizeof(MEMORY_BASIC_INFO));
 while(VirtualQuery((LPCVOID)ModuleAddr, (MEMORY_BASIC_INFORMATION*)&meminfo, sizeof(MEMORY_BASIC_INFO)))
  {
   ModuleAddr += meminfo.RegionSize;
   if(meminfo.AllocationBase == (ULONG_PTR)ModuleBase)ModuleSize += meminfo.RegionSize;
	 else break;
  }
 return ModuleSize;
}
//---------------------------------------------------------------------------
// Some pages of some system DLLs may be not mapped
// TODO: Get rid of this function or at least rename it!
ULONG_PTR _stdcall GetRealModuleSizeHardWay(ULONG_PTR ModuleBase)
{
 UINT Size = 1024*1024;
 ULONG_PTR Total = 0;
 while(Size > 4094)  // Until first unreadable region
  {
   if(IsBadReadPtr((void*)ModuleBase,Size))Size -= 4096;
    else
     {
      ModuleBase += Size;
      Total += Size; 
     }
  }
  return Total;
}
//---------------------------------------------------------------------------
PVOID _stdcall FindLocalModule(PVOID *ModuleBase, DWORD *ModuleSize)
{
 DWORD Type     = NULL;
 DWORD Size     = NULL;
 PVOID Base     = NULL;
 PVOID BaseAddr = (*ModuleBase);
 MEMORY_BASIC_INFORMATION MemInf;

 // Set initial region by allocation
 while(VirtualQuery(BaseAddr,&MemInf,sizeof(MEMORY_BASIC_INFORMATION)))
  {
   BaseAddr = (MemInf.AllocationBase)?(MemInf.AllocationBase):(MemInf.BaseAddress);     // Initial base
   Base     = &((BYTE*)BaseAddr)[MemInf.RegionSize];   // Next region
   Size     = MemInf.RegionSize;
   Type     = MemInf.Type;
   while(VirtualQuery(Base,&MemInf,sizeof(MEMORY_BASIC_INFORMATION)))
	{
	 if(MemInf.AllocationBase != BaseAddr)break;      // End of allocated region
     Base  = &((BYTE*)Base)[MemInf.RegionSize];   
     Size += MemInf.RegionSize;      
     Type  = MemInf.Type;
    }
   if(Type == MEM_IMAGE)
    {
     (*ModuleBase) = Base;  // Set next region base
     if(ModuleSize)(*ModuleSize) = Size;  // Set current module size
     return BaseAddr;    // Return module base
    }
   BaseAddr = &((BYTE*)BaseAddr)[Size]; 
  }
 return NULL;
}
//---------------------------------------------------------------------------
HMODULE _stdcall GetOwnerModule(PVOID Address)
{
 MEMORY_BASIC_INFORMATION MemInf;

 if(!VirtualQuery(Address,&MemInf,sizeof(MEMORY_BASIC_INFORMATION)))return NULL;
 if(MemInf.Type != MEM_IMAGE)return NULL;
 return (HMODULE)MemInf.AllocationBase;
}
//---------------------------------------------------------------------------
long _fastcall CharToHex(BYTE CharValue) 
{
 if((CharValue >= '0')&&(CharValue <= '9'))return (CharValue - '0');       // 0 - 9
 if((CharValue >= 'A')&&(CharValue <= 'F'))return (CharValue - ('A'-10));  // A - F
 if((CharValue >= 'a')&&(CharValue <= 'f'))return (CharValue - ('a'-10));  // a - f
 return -1;
}
//---------------------------------------------------------------------------
/*long _fastcall CharToHex(BYTE CharValue)   // Fast but not relocable
{
static const char HexTable[256] = {
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};
 return HexTable[CharValue];
} */
//---------------------------------------------------------------------------
BYTE _stdcall CharToLowCase(BYTE CharValue) 
{
 if((CharValue >= 0x41)&&(CharValue <= 0x5A))return ((CharValue-0x41)+0x61);
 return CharValue;
}
//---------------------------------------------------------------------------
// Outpud WORD can bi directly written to a string with right half-byte ordering
WORD _fastcall HexToChar(BYTE Value, bool UpCase)   
{
 WORD Result = 0;
 BYTE loffs = ((BYTE)!UpCase * 0x20)+('A'-0x0A);  // 0 or 32   // Case
 BYTE  Tmp = (Value & 0x0F);
 Result |= ((Tmp < 0x0A)?('0'+Tmp):(loffs+Tmp)) << 8;   // L
 Value >>= 4;
 Result |= ((Value < 0x0A)?('0'+Value):(loffs+Value));  // H
 return Result;
}
//---------------------------------------------------------------------------
/*WORD _fastcall HexToChar(BYTE Value, bool UpCase)  // Fast but not relocable
{
 static const char ChrTable[] = "0123456789abcdef0123456789ABCDEF";
 int offs = (int)UpCase << 4;  // 0 or 16
 return ((WORD)ChrTable[offs + (Value >> 4)] << 8) | (WORD)ChrTable[offs + (Value & 0x0F)];
} */
//---------------------------------------------------------------------------
DWORD _stdcall DecStrToDW(LPSTR String, UINT* Len)   // Fast, but not safe
{
 long  StrLength = 0;
 DWORD Result    = 0;
 DWORD DgtPow    = 1;
 BYTE  Symbol;

 for(int ctr=0;((BYTE)(String[ctr]-0x30)) <= 9;ctr++)StrLength++;     // Break on any non digit
 for(long ctr=1;ctr<=StrLength;ctr++)
  {
   Symbol  = (String[StrLength-ctr]-0x30);
   Result += (DgtPow*Symbol);
   DgtPow  = 1;
   for(long num = 0;num < ctr;num++)DgtPow = DgtPow*10;
  }  
 if(Len)*Len = StrLength;         
 return Result;
}
//---------------------------------------------------------------------------
DWORD _stdcall HexStrToDW(LPSTR String, UINT Bytes)   // Fast, but not safe  // TODO: Get rid of this function!
{
 UINT  StrLength = 0;
 DWORD Result    = 0;
 DWORD DgtPow    = 1;
 BYTE  Symbol;

 for(UINT ctr=0;CharToHex(String[ctr]) >= 0;ctr++)StrLength++;     // Break on any non hex digit
 if(Bytes)StrLength = ((StrLength > (Bytes*2))?(Bytes*2):(StrLength))&0xFE;
 for(UINT ctr=1;(ctr<=8)&&(ctr<=StrLength);ctr++)
  {
   Symbol  = (String[StrLength-ctr]-0x30);
   if(Symbol > 9)Symbol  -= 7;
   if(Symbol > 15)Symbol -= 32;
   Result += (DgtPow*Symbol);
   DgtPow  = 1;
   for(UINT num = 0;num < ctr;num++)DgtPow = DgtPow*16;
  }
 return Result;
}
//---------------------------------------------------------------------------
LPSTR _stdcall ConvertToDecDW(DWORD Value, LPSTR Number)
{
 char  DecNums[] = "0123456789";
 int   DgCnt = 0;
 DWORD TmpValue;
 DWORD Divid;

 Divid  = 1000000000;
 for(DgCnt = 0;DgCnt < 10;DgCnt++)
  {
   TmpValue = Value / Divid;
   Number[DgCnt] = DecNums[TmpValue];
   Value -= TmpValue * Divid;
   Divid  = Divid / 10;
  }
 Number[DgCnt] = 0;
 for(DgCnt = 0;(DgCnt < 9) && (Number[DgCnt] == '0');DgCnt++);
 return (LPSTR)(((DWORD)Number) + DgCnt);
}
//---------------------------------------------------------------------------
/*UINT _fastcall DecStrToNum(char* Str)   // Primitive - get rid of it! // Stops on first non numberic char without a problem
{
 UINT x = 0;
 bool neg = false;
 if (*Str == '-'){neg = true; ++Str;}
 for(char ch;((ch=*Str++ - '0') <= 9)&&(ch >= 0);)x = (x*10) + ch;
 if(neg)x = -x;
 return x;
} */
//---------------------------------------------------------------------------
UINT64 _fastcall DecStrToNum(char* Str)
{
 UINT64 x = 0;
 bool neg = false;
 if (*Str == '-'){neg = true; ++Str;}
 for(BYTE ch;(ch=*Str++ - '0') <= 9;)x = (x*10) + ch;
 if(neg)x = -x;
 return x;
}
//---------------------------------------------------------------------------
UINT64 _fastcall DecStrToNumFpu(char* Str)  // A little bit faster // NOTE: Need more tests in case it still can lose some precision somtimes
{
 long double x = 0;
 bool neg = false;
 if (*Str == '-'){neg = true; ++Str;}
 for(BYTE ch;(ch=*Str++ - '0') <= 9;)x = (x*10) + ch;
 if(neg)x = -x;
 return x;
}
//---------------------------------------------------------------------------
UINT64 _fastcall HexStrToNum(char* Str)
{
 UINT64 x = 0;
 for(long chv;(chv=CharToHex(*Str++)) >= 0;)x = (x<<4) + chv;  // (<<4) avoids call to __llmul which is big
 return x;
}
//---------------------------------------------------------------------------
UINT64 _fastcall HexStrToNumFpu(char* Str) // A little bit faster // NOTE: Need more tests in case it still can lose some precision somtimes
{
 long double x = 0;
 for(long chv;(chv=CharToHex(*Str++)) >= 0;)x = (x*16) + chv;
 return x;
}
//---------------------------------------------------------------------------
/*char* _fastcall DecNumToStrS(__int64 Val, char* buf, int* Len)     // Template now
{
 if(Val == 0){if(Len)*Len = 1; *buf = '0'; buf[1] = 0; return buf;}
 bool isNeg = (Val < 0);
 if(isNeg) Val = -Val;
 buf  = &buf[20];
 *buf = 0;
 char* end = buf;
 for(buf--;Val;buf--)
  {
   *buf  = (Val % 10) + '0';
   Val  /= 10;
  }
 if(isNeg)*buf = '-';
   else buf++;
 if(Len)*Len = end-buf;
 return buf;
} */
//---------------------------------------------------------------------------
char* _fastcall DecNumToStrU(UINT64 Val, char* buf, int* Len)
{
 if(Val == 0){if(Len)*Len = 1; *buf = 0; return buf;}  
 buf  = &buf[20];
 *buf = 0;
 char* end = buf;
 for(buf--;Val;buf--)
  {
   *buf  = (Val % 10) + '0';
   Val  /= 10;
  }
 buf++;
 if(Len)*Len = end-buf; 
 return buf;
}
//---------------------------------------------------------------------------
char* _fastcall HexNumToStr(UINT64 Value, int MaxDigits, char* Buffer, bool UpCase, int* Size)   // TODO: Get rid of ChrTable
{
 static const char ChrTable[] = "0123456789abcdef0123456789ABCDEF";
 if(MaxDigits <= 0)MaxDigits = (Value > 0xFFFFFFFF)?(16):(8);   // Smart
   else if(MaxDigits > 16)MaxDigits = 16;
 const char* ChrPtr = &ChrTable[(int)UpCase << 4];
 for(int DgCnt = MaxDigits-1;DgCnt >= 0;DgCnt--)
  {
   Buffer[DgCnt] = ChrPtr[Value & 0x0000000F];
   Value = Value >> 4;
  }
 Buffer[MaxDigits] = 0;
 if(Size)*Size = MaxDigits;
 return Buffer; 
}
//---------------------------------------------------------------------------
DWORD _stdcall WriteLocalProtectedMemory(PVOID Address, PVOID Data, DWORD DataSize, bool RestoreProt)
{
 DWORD             Result;
 DWORD             Offset;
 DWORD             PrevProt;
 DWORD             ProtSize;
 DWORD             BlockSize;
 PVOID             CurProtBase;
 MEMORY_BASIC_INFO MemInf;

 Offset      = 0;
 ProtSize    = 0;
 BlockSize   = DataSize;
 while(BlockSize)               // WARNING  BlockSize must be COUNTED TO ZERO !!!
  {  
   CurProtBase = &((BYTE*)Address)[Offset];
   if(!VirtualQuery(CurProtBase,(MEMORY_BASIC_INFORMATION*)&MemInf,sizeof(MEMORY_BASIC_INFO)))break;    // Rounded to begin of page
   if(MemInf.RegionSize > BlockSize)ProtSize = BlockSize;   // No Protection loop
	 else ProtSize = MemInf.RegionSize;
   if((MemInf.Protect==PAGE_READWRITE)||(MemInf.Protect==PAGE_EXECUTE_READWRITE)) // WRITECOPY  changed to READWRITE by writeprocessmemory - DO NOT ALLOW THIS !!!
	 {   
	  MoveMemory(CurProtBase,&((BYTE*)Data)[Offset],ProtSize);    // FastMoveMemory
	  Result = ProtSize;
	 }
	  else
	   {
		if(!VirtualProtect(CurProtBase,ProtSize,PAGE_EXECUTE_READWRITE,&PrevProt))break;   // Allow writing
		MoveMemory(CurProtBase,&((BYTE*)Data)[Offset],ProtSize);     // FastMoveMemory
		Result = ProtSize;
		if(RestoreProt){if(!VirtualProtect(CurProtBase,ProtSize,PrevProt,&PrevProt))break;}  // Restore protection
	   }
   if(Result  != ProtSize)break;
   Offset     += ProtSize;
   BlockSize  -= ProtSize;
  }
 return (DataSize-BlockSize); // Bytes written
}
//---------------------------------------------------------------------------
DWORD _stdcall WriteProtectedMemory(DWORD ProcessID, HANDLE hProcess, PVOID Address, PVOID Data, DWORD DataSize, bool RestoreProt)
{
 bool                     CurProc;
 bool                     CloseP;
 SIZE_T                   Result;
 DWORD                    Offset;
 DWORD                    PrevProt;
 DWORD                    ProtSize;
 DWORD                    BlockSize;
 PVOID                    CurProtBase;
 MEMORY_BASIC_INFORMATION MemInf;

 CloseP      = 0;
 Offset      = 0;
 CurProc     = 0;
 ProtSize    = 0;
 BlockSize   = DataSize;
 if(!hProcess){hProcess = OpenProcess(PROCESS_VM_READ|PROCESS_VM_WRITE|PROCESS_VM_OPERATION|PROCESS_QUERY_INFORMATION,false,ProcessID);CloseP=true;}
 if(hProcess == GetCurrentProcess())CurProc = true;
 while(BlockSize)               // WARNING  BlockSize must be COUNTED TO ZERO !!!
  {
   CurProtBase = &((BYTE*)Address)[Offset];
   if(!VirtualQueryEx(hProcess,CurProtBase,&MemInf,sizeof(MEMORY_BASIC_INFORMATION)))break;    // Rounded to begin of page
   if(MemInf.RegionSize > BlockSize)ProtSize = BlockSize;   // No Protection loop
	 else ProtSize = MemInf.RegionSize;
   if((MemInf.Protect==PAGE_READWRITE)||(MemInf.Protect==PAGE_EXECUTE_READWRITE)) // WRITECOPY  changed to READWRITE by writeprocessmemory - DO NOT ALLOW THIS !!!
	 {
	  if(CurProc){memmove(CurProtBase,&((BYTE*)Data)[Offset],ProtSize);Result = ProtSize;}
		else {if(!WriteProcessMemory(hProcess,CurProtBase,&((BYTE*)Data)[Offset],ProtSize,&Result))break;} // Size of Type VOID do not converted to DWORD - compatible with WIN32 And WIN64
	 }
	  else
	   {
		if(!VirtualProtectEx(hProcess,CurProtBase,ProtSize,PAGE_EXECUTE_READWRITE,&PrevProt))break;   // Allow writing
		if(CurProc){memmove(CurProtBase,&((BYTE*)Data)[Offset],ProtSize);Result = ProtSize;}
		  else {if(!WriteProcessMemory(hProcess,CurProtBase,&((BYTE*)Data)[Offset],ProtSize,&Result))break;} // Size of Type VOID do not converted to DWORD - compatible with WIN32 And WIN64
		if(RestoreProt){if(!VirtualProtectEx(hProcess,CurProtBase,ProtSize,PrevProt,&PrevProt))break;}  // Restore protection
	   }
   if(Result  != ProtSize)break;
   Offset     += ProtSize;
   BlockSize  -= ProtSize;
  }
 if(CloseP)CloseHandle(hProcess);
 return (DataSize-BlockSize); // Bytes written
}
//---------------------------------------------------------------------------
int _stdcall SizeOfSignatureData(LPSTR Signature, UINT SigLen) 
{
 int Size = 0;
 if(!SigLen)SigLen--;         // Overflow the counter - removes size limit
 if('R' == *Signature)        // Reversed signature!  // Revise!
  {
   Signature++; 
   SigLen--;
  }
 for(;*Signature && (SigLen >= 2);Signature++,SigLen--) // Scan by Half byte
  {
   if(*Signature == ' ')continue;   // Skip spaces
   if(*Signature == ':')break;      // Start of a comments
   if(*Signature == '*')            // *SkipNum*    
    {
     UINT Len = 0;
     Size += DecStrToDW(++Signature, &Len);      // <<<<< Deprecated function!
     Signature += Len;
     continue;
    }       
   long ValueH = CharToHex(Signature[0]);
   long ValueL = CharToHex(Signature[1]);
   if((ValueH < 0)&&(Signature[0] != '?'))break;
   if((ValueL < 0)&&(Signature[1] != '?'))break;
   Signature++;
   SigLen--;
   Size++;
  }
 return Size;
}
//---------------------------------------------------------------------------
// SigLen - number of chars in signature string (Can have a big block of signatures in one string and specify a separate one by offset and length)
// Please don`t pass a malformed signatures here :)
bool _stdcall IsMemSignatureMatch(PVOID Address, LPSTR Signature, UINT SigLen)
{
 BYTE  Value   = 0;
 int   Offset  = 0; 
 int   SigMult = 1;
 if(!SigLen)SigLen--;         // Overflow the counter - removes size limit
 if('R' == *Signature)        // Reversed signature!  // Revise!
  {
   Signature++; 
   SigLen--;
   Offset--;
   SigMult = -1;   
  } 
 for(;*Signature && !Value && (SigLen >= 2);Signature++,SigLen--) // Scan by Half byte
  {
   if(*Signature == ' ')continue;   // Skip spaces
   if(*Signature == ':')return true;      // Start of a comments
   if(*Signature == '*')            // *SkipNum*    
    {
     UINT Len = 0;
     int Counter = DecStrToDW(++Signature, &Len);      // <<<<< Deprecated function!
     Signature += Len;
     Offset += (Counter*SigMult);   // Skip N bytes
     continue;
    }       
   Value = ((PBYTE)Address)[Offset];
   long ValueH = CharToHex(Signature[0]);
   long ValueL = CharToHex(Signature[1]);
   if(ValueH < 0)ValueH = (Value >> 4);
   if(ValueL < 0)ValueL = (Value & 0x0F);
   Value  = (BYTE)(((ValueH << 4) | ValueL) ^ Value);
   Signature++;
   SigLen--;
   Offset += SigMult;
  }
 if(!Value && !*Signature)return true;
 return false;
}   
//--------------------------------------------------------------------------- 
 // Backward('R') sigs are useless and unsafe for a range search?
PBYTE _stdcall FindMemSignatureInRange(PBYTE AddrLo, PBYTE AddrHi, LPSTR Signature, UINT Step, UINT MatchIdx, UINT SigLen)
{                                      
 if(!Step)Step = 1;                // Support backwards?
 if(!MatchIdx)MatchIdx = 1;
 AddrHi -= SizeOfSignatureData(Signature, SigLen);      // Prevent a buffer overflow
 for(;AddrLo <= AddrHi;AddrLo+=Step)
  {
   if(IsMemSignatureMatch(AddrLo, Signature, SigLen) && !--MatchIdx){DBGMSG("Address is %p for: %s",AddrLo,Signature); return AddrLo;} 
  }
 DBGMSG("Not found for: %s",Signature);
 return NULL;
}
//--------------------------------------------------------------------------- 
// Skips any not mapped memory pages
PBYTE _stdcall FindMemSignatureInRangeSafe(PBYTE AddrLo, PBYTE AddrHi, LPSTR Signature, UINT Step, UINT MatchIdx, UINT SigLen)
{                                      
 if(!Step)Step = 1;                // Support backwards?
 if(!MatchIdx)MatchIdx = 1;
 ULONG_PTR LstPage = ((ULONG_PTR)AddrLo & (ULONG_PTR)~0xFFF);   // Page size assumed 4K (0x1000)
 AddrHi -= SizeOfSignatureData(Signature, SigLen);      // Prevent a buffer overflow
 for(;AddrLo <= AddrHi;AddrLo+=Step)
  {
   ULONG_PTR CurPage = ((ULONG_PTR)AddrLo & (ULONG_PTR)~0xFFF);
   if(CurPage != LstPage)  // In a next page, test it
    {
     LstPage = CurPage;
     if(IsBadReadPtr(AddrLo,1)){AddrLo += 0x1000; continue;}   // To a next page/ Step value will be preserved(Unaligned will start not from beginning of a page)      
    }
   if(IsMemSignatureMatch(AddrLo, Signature, SigLen) && !--MatchIdx){DBGMSG("Address is %p for: %s",AddrLo,Signature); return AddrLo;} 
  }
 DBGMSG("Not found for: %s",Signature);
 return NULL;
}
//---------------------------------------------------------------------------
PVOID _stdcall FindMemSignatureLocal(PVOID Address, LPSTR Signature)
{
 DWORD BlockSize;
 DWORD SigLen;
 PBYTE BytesBuf;
 MEMORY_BASIC_INFORMATION MemInf;

 if((SigLen = lstrlen(Signature)) < 2)return NULL;
 SigLen = (SigLen/2);
 while(VirtualQuery(Address,&MemInf, sizeof(MEMORY_BASIC_INFORMATION)))
  {
   BytesBuf  = (PBYTE)Address;
   Address   = &((BYTE*)Address)[MemInf.RegionSize]; 
   BlockSize = MemInf.RegionSize;
   if((!(MemInf.State & MEM_COMMIT))||(MemInf.State & PAGE_NOACCESS)||(MemInf.State & PAGE_GUARD))continue;  	
   while (BlockSize >= SigLen)
   	{				  	 	 	    
	 if(IsMemSignatureMatch(BytesBuf,Signature,SigLen))return (PVOID)BytesBuf;		
	 BytesBuf++;
	 BlockSize--;
	}
  }
 return NULL;
}
//---------------------------------------------------------------------------
DWORD _stdcall ApplyPatchLocal(PVOID PatchAddr, PVOID PatchData, DWORD PatchSize, LPSTR Signature)
{
 // Uncomment to take a REVERSE signatures
 //BYTE Buffer[256];
 //wsprintf((LPSTR)&Buffer,"R %08X: %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",(DWORD)PatchAddr,((PBYTE)PatchAddr)[-1],((PBYTE)PatchAddr)[-2],((PBYTE)PatchAddr)[-3],((PBYTE)PatchAddr)[-4],((PBYTE)PatchAddr)[-5],((PBYTE)PatchAddr)[-6],((PBYTE)PatchAddr)[-7],((PBYTE)PatchAddr)[-8],((PBYTE)PatchAddr)[-9],((PBYTE)PatchAddr)[-10]);
 //OutputDebugString((LPSTR)&Buffer);

 if(IsBadReadPtr(PatchAddr,PatchSize))return 1;
 if(IsBadReadPtr(PatchData,PatchSize))return 2;
 if(Signature && !IsMemSignatureMatch(PatchAddr,Signature,(lstrlen(Signature)/2)))return 3;  //Signature not match
 if(!WriteLocalProtectedMemory(PatchAddr, PatchData, PatchSize, TRUE))return 4;   // Patching failed
 return 0;
}
//---------------------------------------------------------------------------
DWORD _stdcall SetFunctionPatch(HMODULE Module, DWORD Offset, PVOID FuncAddr, BYTE CmdCode, DWORD AddNOP, LPSTR Signature)
{
 PVOID PatchAddr;
 DWORD PatchLen;
 DWORD Address;
 DWORD Index = 0;
 BYTE  PatchData[128];

 memset(&PatchData,0x90,sizeof(PatchData)); // Fill with NOPs
 PatchAddr = &((BYTE*)Module)[Offset];
 switch(CmdCode)
  {
   case 0xE8:    // Call
   case 0xE9:    // Jmp
//     Address      = AddrToRelAddr(PatchAddr, 5, FuncAddr);   // AddrToRelAddr is in another header!  
     PatchLen     = 5;
     PatchData[Index] = CmdCode;
     Index++;
     break;
   case 0xB8:    // mov EAX,NNNN
   case 0x68:    // push DWORD
   case 0xA1:    // mov EAX,[NNNN]  
   case 0xA3:    // mov [NNNN],EAX
     Address      = (DWORD)FuncAddr;     
     PatchLen     = 5;
     PatchData[Index] = CmdCode;
     Index++;
     break;	 
   case 0x6A:    // push BYTE
     Address      = (DWORD)FuncAddr;     
     PatchLen     = 2;
     PatchData[Index] = CmdCode;
     Index++;
     break;	
   case 0x90:    // nop
     Address      = 0x90909090;     
	 PatchLen     = 1;
     PatchData[Index] = CmdCode;
     Index++;
     break;	
   case 0xEB:    // jmp rel BYTE
     PatchLen     = 1;
     PatchData[Index] = CmdCode;
	 //if(FuncAddr){}  // convert addr to rel
     Index++;
     break;	
  }
         
 ((PDWORD)&PatchData[Index])[0] = Address;  
 return ApplyPatchLocal(PatchAddr, &PatchData, (PatchLen+AddNOP), Signature);
}
//---------------------------------------------------------------------------
/*int _stdcall ByteArrayToHexStr(PBYTE Buffer, LPSTR DstStr, UINT HexByteCnt)
{
 UINT len = 0;
 for(UINT ctr=0;(ctr < HexByteCnt);len+=2,ctr++)
  {
   WORD chr      = HexToChar(Buffer[ctr]);
   DstStr[len]   = ((PBYTE)&chr)[0];
   DstStr[len+1] = ((PBYTE)&chr)[1];
  }
 return len;
}
//---------------------------------------------------------------------------
int _stdcall ByteArrayToHexStrSwap(PBYTE Buffer, LPSTR DstStr, UINT HexByteCnt)
{
 UINT len = 0;
 for(UINT ctr=0;(ctr < HexByteCnt);len+=2,ctr++)
  {
   WORD chr      = HexToChar(Buffer[ctr]);
   DstStr[len]   = ((PBYTE)&chr)[1];
   DstStr[len+1] = ((PBYTE)&chr)[0];
  }
 return len;
}*/
//---------------------------------------------------------------------------
int _stdcall ByteArrayToHexStr(PBYTE Buffer, LPSTR DstStr, UINT ByteCnt, bool UpCase)
{
 UINT len = 0;
 for(UINT ctr=0;(ctr < ByteCnt);ctr++)
  {
   WORD chr      = HexToChar(Buffer[ctr],UpCase);
   DstStr[len++] = chr;   
   DstStr[len++] = chr >> 8; 
  }
 return len;
}
//---------------------------------------------------------------------------
/*int _stdcall ByteArrayToHexStrSwap(PBYTE Buffer, LPSTR DstStr, UINT ByteCnt)
{
 UINT len = 0;
 for(UINT ctr=0;(ctr < ByteCnt);ctr++)
  {
   WORD chr      = HexToChar(Buffer[ctr]);
   DstStr[len++] = chr; 
   DstStr[len++] = chr >> 8;    
  }
 return len;
} */
//---------------------------------------------------------------------------
int _stdcall HexStrToByteArray(PBYTE Buffer, LPSTR SrcStr, UINT HexByteCnt)
{
 UINT len = 0;
 UINT ctr = 0;
 for(;(SrcStr[len]&&SrcStr[len+1])&&(!HexByteCnt||(ctr < HexByteCnt));len++)
  {
   if(SrcStr[len] <= 0x20)continue;   // Skip spaces and line delimitters
   int ByteHi  = CharToHex(SrcStr[len]);
   int ByteLo  = CharToHex(SrcStr[len+1]);
   if((ByteHi  < 0)||(ByteLo < 0))return ctr;  // Not a HEX char
   Buffer[ctr] = (ByteHi << 4)|ByteLo;
   ctr++;
   len++;
  }
 return ctr;
}
//------------------------------------
bool _stdcall IsValidAsciiString(PBYTE Ptr, UINT MinLen, UINT MaxLen)
{
 for(UINT ctr=0;ctr < MaxLen;ctr++)
  {
   BYTE val = *Ptr;
   if(!val)return (ctr >= MinLen);	   // End of String
   if((val < 0x20) && !((val == '\t')||(val == '\r')||(val == '\n')))return false;
   if(val >= 128)return false;
  }
 return true;	// Seems OK
}
//--------------------------------------------------------------------------- 
bool _stdcall IsStringContains(LPSTR String, LPSTR Target, UINT StrLen, UINT TgtLen) // NOTE: lstrcmp makes SORTED comparision of strings(by alphabet), not by LENGTH!
{
 if(!StrLen)StrLen = lstrlen(String);
 if(!TgtLen)TgtLen = lstrlen(Target);
 for(UINT offs=0;(StrLen - offs) >= TgtLen;offs++)
  {
   if(memcmp(&String[offs],Target,TgtLen) == 0)return true;  
  }
 return false;
}
//--------------------------------------------------------------------------- 
//Return the time as seconds elapsed since midnight, January 1, 1970, or -1 in the case of an error.
//
__int64  _stdcall GetTime64(bool Local)   // C++Builder fails 64bit consts!!!
{
 __int64 MAXTIME64  = 0x793406fffi64;        // number of seconds from 00:00:00, 01/01/1970 UTC to 23:59:59. 12/31/3000 UTC
 UINT64  EPOCH_BIAS = 116444736000000000i64; // Number of 100 nanosecond units from 1/1/1601 to 1/1/1970

 UINT64 ft, fu;
 if(Local)
  {
   GetSystemTimeAsFileTime((FILETIME*)&fu);
   FileTimeToLocalFileTime((FILETIME*)&fu,(FILETIME*)&ft);
  }
   else GetSystemTimeAsFileTime((FILETIME*)&ft);
 __int64 tim = (__int64)((ft - EPOCH_BIAS) / 10000000i64);
 if(tim > MAXTIME64)tim = (__int64)(-1);
 return tim;
}
//--------------------------------------------------------------------------- 
#define SECS_TO_FT_MULT 10000000
#define TIME_T_BASE ((UINT64)11644473600)

UINT64 FileTimeToT64(FILETIME *pft)
{   
 LARGE_INTEGER li;
 li.LowPart  = pft->dwLowDateTime;
 li.HighPart = pft->dwHighDateTime;   
 return (li.QuadPart / SECS_TO_FT_MULT);
}
void T64ToFileTime(UINT64 pt, FILETIME *pft)
{   
 LARGE_INTEGER li;    
 li.QuadPart=pt*SECS_TO_FT_MULT;
 pft->dwLowDateTime=li.LowPart;   
 pft->dwHighDateTime=li.HighPart;   
}
UINT64 FindTimeTBase(void)
{  
 // Find 1st Jan 1970 as a FILETIME
 SYSTEMTIME st;  
 FILETIME ft;
 memset(&st,0,sizeof(st));
 st.wYear=1970;  
 st.wMonth=1;  
 st.wDay=1;
 SystemTimeToFileTime(&st, &ft);
 return FileTimeToT64(&ft);
}
UINT64 SystemTimeToT64(SYSTEMTIME *pst)
{
 FILETIME ft;
 //FILETIME ftl;
 SystemTimeToFileTime(pst, &ft);
 //LocalFileTimeToFileTime(&ftLocal, &ft);
 return (FileTimeToT64(&ft) - TIME_T_BASE);

//  (*pt) -= TIME_T_BASE;

// FILETIME ft;
// SystemTimeToFileTime(pst, &ft);
// return FileTimeToT64(&ft);
}
void T64ToSystemTime(UINT64 pt, SYSTEMTIME *pst)
{
 FILETIME ft;
 pt += FindTimeTBase();
 T64ToFileTime(pt,&ft);
 FileTimeToSystemTime(&ft,pst);
}
void T64ToLocalSysTime(UINT64 pt, SYSTEMTIME *pst)
{
 FILETIME ft;
 FILETIME lft;
 pt += FindTimeTBase();
 T64ToFileTime(pt,&ft);
 FileTimeToLocalFileTime(&ft, &lft);
 FileTimeToSystemTime(&lft,pst);
}
//---------------------------------------------------------------------------
// 2015-11-27 20:43:48
LPSTR _stdcall UnixDateTimeToStr(UINT64 DateTime, LPSTR Buffer)
{
 SYSTEMTIME systime;
 T64ToSystemTime(DateTime, &systime);
 wsprintf(Buffer,"%.4u-%.2u-%.2u %.2u:%.2u:%.2u",systime.wYear,systime.wMonth,systime.wDay,systime.wHour,systime.wMinute,systime.wSecond);
 return Buffer;
}
//---------------------------------------------------------------------------
UINT64 FileTimeToUnixTime(FILETIME &ft)
{
 ULARGE_INTEGER ull;
 ull.LowPart  = ft.dwLowDateTime;
 ull.HighPart = ft.dwHighDateTime;
 return ull.QuadPart / 10000000ULL - 11644473600ULL;
}
//---------------------------------------------------------------------------
void UnixTimeToFileTime(UINT64 t, LPFILETIME pft)
{
 LONGLONG ll = Int32x32To64(t, 10000000) + 116444736000000000;  // Note that LONGLONG is a 64-bit value
 pft->dwLowDateTime = (DWORD)ll;
 pft->dwHighDateTime = ll >> 32;
}
//---------------------------------------------------------------------------
void UnixTimeToSystemTime(UINT64 t, LPSYSTEMTIME pst)
{
 FILETIME ft;
 UnixTimeToFileTime(t, &ft);
 FileTimeToSystemTime(&ft, pst);
}
//---------------------------------------------------------------------------
DWORD hash_string_simple(char *str, UINT len)
{
 DWORD hash = 0;
 for(int ctr=len/sizeof(DWORD);ctr > 0;ctr--,str+=4){ hash ^= *((PDWORD)str); hash <<= 8; }
 for(int ctr=len%sizeof(DWORD);ctr > 0;ctr--,str++){ hash ^= *str; hash <<= 8; }
 hash |= (BYTE)len;
 return hash;
}
//---------------------------------------------------------------------------
DWORD hash_string(char *s, int* len)
{
 DWORD hash = 0;
 int l = 0;
 for(; *s; ++s,++l)
  {
   hash += *s;
   hash += (hash << 10);
   hash ^= (hash >> 6);
  }
 hash += (hash << 3);
 hash ^= (hash >> 11);
 hash += (hash << 15);
 if(len)*len = l;
 return hash;
}
//---------------------------------------------------------------------------
/*unsigned long djb_hashl(const char *clave)
{
 unsigned long c,i,h;
 for(i=h=0;clave[i];i++)
  {
   // c = toupper(clave[i]);
   h = ((h << 5) + h) ^ c;
  }
 return h;
}*/
//---------------------------------------------------------------------------
bool _stdcall TestIsWow64Process(void)
{
 BOOL res = false;
 //IsWow64Process(GetCurrentProcess(),&res);
 return res;
}
//------------------------------------------------------------------------------------
ULONGLONG __stdcall ShlULL(ULONGLONG Value, BYTE Shift)
{
 if(!Shift)return Value;
 if(Shift >= 64)return 0;
 if(Shift >= 32)
  {
   ((ULARGE_INTEGER*)&Value)->HighPart = ((ULARGE_INTEGER*)&Value)->LowPart << (0x1F & Shift);
   ((ULARGE_INTEGER*)&Value)->LowPart  = 0;
  }
   else
	{
	 ((ULARGE_INTEGER*)&Value)->HighPart  = (((ULARGE_INTEGER*)&Value)->LowPart >> (32 - Shift)) | (((ULARGE_INTEGER*)&Value)->HighPart << Shift);
	 ((ULARGE_INTEGER*)&Value)->LowPart <<= Shift;
	}
 return Value;
}
//---------------------------------------------------------------------------
ULONGLONG __stdcall ShrULL(ULONGLONG Value, BYTE Shift)     // TODO: Replace with something FASTER!
{
 if(!Shift)return Value;
 if(Shift >= 64)return 0;
 if(Shift >= 32)return ((ULARGE_INTEGER*)&Value)->HighPart >> (0x1F & Shift);
 ((ULARGE_INTEGER*)&Value)->LowPart    = (((ULARGE_INTEGER*)&Value)->HighPart << (32 - Shift)) | (((ULARGE_INTEGER*)&Value)->LowPart >> Shift);
 ((ULARGE_INTEGER*)&Value)->HighPart >>= Shift;
 return Value;
}
//---------------------------------------------------------------------------
/*ULONGLONG __stdcall BinLongUDiv(ULONGLONG Dividend, ULONGLONG Divisor, ULONGLONG *Rem)  // Somhow incorrect!!!
{
 if(!Divisor)return 0;   // Dividend;   // Or return 0 ?
 ULONGLONG Quotient, Remainder;
 Quotient = Remainder = 0;
 long idx = 0;

 // Get highest bit index of Dividend  (Use an Intrinsics?)
 ULARGE_INTEGER Value;
 Value.QuadPart = Dividend;
 if(!Value.HighPart){idx += 32;Value.HighPart = Value.LowPart;Value.LowPart = 0;}  // High 32 bits are zero
 ULONG PrvVal = Value.HighPart;
 if(!(PrvVal >> 16)){idx += 16;Value.HighPart <<= 16;}   // High 16 bits are zero
 if(!(PrvVal >>  8)){idx +=  8;Value.HighPart <<=  8;}   // High 8  bits are zero
 if(!(PrvVal >>  4)){idx +=  4;Value.HighPart <<=  4;}   // High 4  bits are zero
 if(!(PrvVal >>  2)){idx +=  2;Value.HighPart <<=  2;}   // High 2  bits are zero
 Dividend = Value.QuadPart;

 for(;idx < (sizeof(ULONGLONG)*8);idx++)  // Set initial idx to result of BSR (Bit Scan Reverse) for Dividend
  {
   Quotient  <<= 1;
   Remainder <<= 1;
   if(Dividend & ~(((ULONGLONG)(-1))/2))Remainder |= 1; // Dividend & 0x8000000000000000
   Dividend  <<= 1;
   if(Remainder >= Divisor)
	{
	 Remainder -= Divisor;
	 Quotient  |= 1;
	}
  }
 if(Rem)*Rem = Remainder;
 return Quotient;
}*/
//---------------------------------------------------------------------------
BOOL _stdcall IsKeyCombinationPressed(DWORD Combination)
{
 for(DWORD ctr=4;ctr > 0;ctr--)
  {
   if(BYTE KeyCode = (Combination & 0x000000FF))
	{
     WORD KeyState = GetAsyncKeyState(KeyCode);	// 1 - key is DOWN; 0 - key is UP
	 if(!(KeyState & 0x8000))return FALSE;  // If one of keys in combination is up - no combination pressed 
	}
   Combination = (Combination >> 8);
  }
 return TRUE;
}
//---------------------------------------------------------------------------
HGLOBAL _stdcall GetResource(HMODULE Module, LPSTR ResName, LPSTR ResType, PUINT ResSize)
{
 HRSRC   hDataRes = FindResource(Module,ResName,ResType);     // RT_RCDATA
 if(!hDataRes)return NULL;
 HGLOBAL rcGlobal = LoadResource(Module,hDataRes);
 if(rcGlobal && ResSize)*ResSize = SizeofResource(Module,hDataRes);
 return rcGlobal;
}
//---------------------------------------------------------------------------
ULONGLONG __stdcall BinLongUMul(ULONGLONG Multiplicand, ULONGLONG Multiplier)
{
 ULONGLONG Summ = 0;
 for(long idx=0;Multiplier && (idx < (sizeof(ULONGLONG)*8));idx++)
  {
   if(Multiplier & 1)Summ += Multiplicand;
   Multiplicand <<= 1;
   Multiplier   >>= 1;
  }
 return Summ;
}
//====================================================================================
UINT __stdcall SaveMemoryToFile(LPSTR FileName, DWORD ProcessID, DWORD Address, DWORD BlockSize, BYTE ErrorByte)
{
 DWORD       ReadBytes;
 DWORD       PrBReaded;
 DWORD       SavedBytes;
 DWORD       Result;
 HANDLE      hProcess;
 HANDLE      hDefHeap;
 HANDLE      hMemFile;
 LPVOID      ReadBuffer;
 SYSTEM_INFO SysInfo;

 DeleteFile(FileName);    // For Safe
 PrBReaded  = 0;
 SavedBytes = 0;
 hProcess   = OpenProcess(PROCESS_VM_READ,false,ProcessID);
 if(hProcess == NULL)return -1;
 hMemFile   = CreateFile(FileName,GENERIC_WRITE,NULL,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
 if(hMemFile == INVALID_HANDLE_VALUE){CloseHandle(hProcess);return -2;}
 GetSystemInfo(&SysInfo);
 hDefHeap   = GetProcessHeap();            // Get Default Process Heap
 ReadBuffer = HeapAlloc(hDefHeap, HEAP_ZERO_MEMORY, SysInfo.dwPageSize);
 ReadBytes  = SysInfo.dwPageSize-(Address % SysInfo.dwPageSize);  //Bytes Remains on page
 do
  {
   memset(ReadBuffer,ErrorByte,SysInfo.dwPageSize);    //Mark Data as EMPTY
   if(ReadBytes > BlockSize)ReadBytes = BlockSize;
   SIZE_T RRes;
   ReadProcessMemory(hProcess,LPCVOID(Address+SavedBytes),ReadBuffer,ReadBytes,&RRes);
   PrBReaded  += RRes;
   if(!WriteFile(hMemFile, ReadBuffer, ReadBytes, &Result, NULL)){PrBReaded = -3;goto RExit;}
   SavedBytes += ReadBytes;
   BlockSize  -= ReadBytes;
   ReadBytes   = SysInfo.dwPageSize;
  }
   while((int)BlockSize > 0);
RExit:
 CloseHandle(hMemFile);
 CloseHandle(hProcess);
 HeapFree(hDefHeap, NULL, ReadBuffer);
 if((int)PrBReaded <= 0)DeleteFile(FileName);
 return PrBReaded;
}
//---------------------------------------------------------------------------
/*template<typename T> int _stdcall TRedirectExportsIntrn(HMODULE ModFrom, HMODULE ModTo)
{
 DOS_HEADER     *DosHdr;
 WIN_HEADER<T>  *WinHdr;
 EXPORT_DIR     *Export;
 DATA_DIRECTORY *ExportDir;

 int Total = 0;
 DosHdr    = (DOS_HEADER*)ModFrom;
 WinHdr    = (WIN_HEADER<T>*)&((PBYTE)ModFrom)[DosHdr->OffsetHeaderPE];
 ExportDir = &WinHdr->OptionalHeader.DataDirectories.ExportTable;
 Export    = (EXPORT_DIR*)&((PBYTE)ModFrom)[ExportDir->DirectoryRVA]; 
 PBYTE BlkAddr = &((PBYTE)ModFrom)[Export->AddressTableRVA];
 DWORD BlkSize = Export->NamePointersNumber * sizeof(T);
 DWORD OldProt;
 VirtualProtect(BlkAddr,BlkSize,PAGE_EXECUTE_READWRITE,&OldProt);
 for(DWORD ctr=0;ctr < Export->NamePointersNumber;ctr++)
  {
   LPSTR CurProcName = (LPSTR)&((PBYTE)ModFrom)[(((PDWORD)&((PBYTE)ModFrom)[Export->NamePointersRVA])[ctr])];
   PVOID ToProc = GetProcAddress(ModTo,CurProcName);
   if(!ToProc){LOGMSG("Import failed: %p:'%s'",ModTo,CurProcName); continue;}
   WORD  Value = ((PWORD)&((PBYTE)ModFrom)[Export->OrdinalTableRVA])[ctr];
   T*    Entry = &((T*)&((PBYTE)ModFrom)[Export->AddressTableRVA])[Value];	   // x64: PDWORD?
   LOGMSG("Redirected: From=%p:%p, To=%p:%p, Name='%s'",ModFrom,*Entry,ModTo,ToProc,CurProcName);
   *Entry = (T)ToProc; 	
   Total++;   
  }
 VirtualProtect(BlkAddr,BlkSize,OldProt,&OldProt);
 return Total;
} 
//---------------------------------------------------------------------------
int _stdcall RedirectExports(HMODULE ModFrom, HMODULE ModTo) 
{
 if(!IsValidPEHeader(ModFrom))return -1;
 if(IsValidModuleX64(ModFrom))return TRedirectExportsIntrn<PETYPE64>(ModFrom, ModTo); 
 return TRedirectExportsIntrn<PETYPE32>(ModFrom, ModTo); 
}*/
//---------------------------------------------------------------------------
void _stdcall DumpHexData(PBYTE Data, UINT Size, UINT RowLen)      // Max row len is 256 bytes
{
 BYTE Buffer[512+4];
 if(RowLen > 256)RowLen = 256;
 PBYTE XData = Data;
 UINT  XSize = Size;
 for(UINT RSize=0;XSize;XData+=RSize,XSize-=RSize)   // First dump HEX
  {
   RSize = (RowLen > XSize)?(XSize):(RowLen);
   PBYTE DPtr  = (PBYTE)&Buffer;
   for(UINT ctr=0;ctr < RSize;ctr++)
    {
     WORD Val  = HexToChar(XData[ctr]);
     *(DPtr++) = Val;
     *(DPtr++) = Val >> 8; 
    }
   *(DPtr++) = '\r';
   *(DPtr++) = '\n';
   LOGTXT((LPSTR)&Buffer,(DPtr - (PBYTE)&Buffer));
  }
 XData = Data;
 XSize = Size;
 for(UINT RSize=0;XSize;XData+=RSize,XSize-=RSize)   // Second dump TEXT
  {
   RSize = (RowLen > XSize)?(XSize):(RowLen);
   PBYTE DPtr  = (PBYTE)&Buffer;
   for(UINT ctr=0;ctr < RSize;ctr++)
    {
     BYTE Val  = XData[ctr];
     if(Val < 0x20)Val = '.';
     *(DPtr++) = Val;
    }
   *(DPtr++) = '\r';
   *(DPtr++) = '\n';
   LOGTXT((LPSTR)&Buffer,(DPtr - (PBYTE)&Buffer));
  }
}
//---------------------------------------------------------------------------
void _stdcall DumpHexDataFmt(PBYTE Data, UINT Size, UINT RowLen)  // Max row len is 128 bytes
{
 BYTE Buffer[256+128+4];
 if(RowLen > 128)RowLen = 128;
 for(UINT RSize=0;Size;Data+=RSize,Size-=RSize)
  {
   RSize = (RowLen > Size)?(Size):(RowLen);
   PBYTE DPtr  = (PBYTE)&Buffer;
   for(UINT ctr=0;ctr < RSize;ctr++)    // Create HEX string
    {
     WORD Val  = HexToChar(Data[ctr]);
     *(DPtr++) = Val;
     *(DPtr++) = Val >> 8;
    }
   for(UINT ctr=RSize;ctr < RowLen;ctr++,DPtr+=2)*((PWORD)DPtr) = 0x2020;   // Fill left space // Use memset?
   *(DPtr++) = 0x20;
   *(DPtr++) = 0x20;
   for(UINT ctr=0;ctr < RSize;ctr++)   // Create Text string
    {
     BYTE Val  = Data[ctr];
     if(Val < 0x20)Val = '.';
     *(DPtr++) = Val;
    }
   *(DPtr++) = '\r';
   *(DPtr++) = '\n';
   LOGTXT((LPSTR)&Buffer,(DPtr - (PBYTE)&Buffer));
  }
}
//---------------------------------------------------------------------------
/* PRELIMINARY!!!
In the ANSI version of this function, the name is limited to MAX_PATH characters. To extend this limit to 32,767 wide characters, 
call the Unicode version of the function (GetFileAttributesExW), and prepend "\\?\" to the path. For more information, see Naming a File.
*/
bool _stdcall DeleteFolderW(PWSTR FolderPath)
{
 DWORD  PathLen;
 HANDLE hSearch;
 WIN32_FIND_DATAW fdat;
 WCHAR  PathBuffer[MAX_PATH];

 PathBuffer[0] = 0;
 lstrcatW(PathBuffer,FolderPath);
 PathLen = lstrlenW(PathBuffer);
 if(IsFilePathDelim(PathBuffer[PathLen-1]))PathLen--;
 PathBuffer[PathLen+0] = '\\';
 PathBuffer[PathLen+1] = '*';
 PathBuffer[PathLen+2] = 00;
 hSearch = FindFirstFileW(PathBuffer,&fdat);
 if(hSearch == INVALID_HANDLE_VALUE)return false;
 do
  {    
   if((fdat.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && IsPathLink(&fdat.cFileName[0]))continue;  // Not a real directory
   PathBuffer[PathLen+1] = 0;
   lstrcatW(PathBuffer,fdat.cFileName);
   if(fdat.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)DeleteFolderW(PathBuffer);
	 else DeleteFileW(PathBuffer);
  }
   while(FindNextFileW(hSearch,&fdat));
 FindClose(hSearch);
 return RemoveDirectoryW(FolderPath);
}
//---------------------------------------------------------------------------
bool _stdcall DeleteFolder(LPSTR FolderPath)
{
 DWORD  PathLen;
 HANDLE hSearch;
 WIN32_FIND_DATA fdat;
 CHAR   PathBuffer[MAX_PATH];

 PathBuffer[0] = 0;
 lstrcat(PathBuffer,FolderPath);
 PathLen = lstrlen(PathBuffer);
 if(IsFilePathDelim(PathBuffer[PathLen-1]))PathLen--;
 PathBuffer[PathLen+0] = '\\';
 PathBuffer[PathLen+1] = '*';
 PathBuffer[PathLen+2] = 00;
 hSearch = FindFirstFile(PathBuffer,&fdat);
 if(hSearch == INVALID_HANDLE_VALUE)return false;
 do
  {
   if((fdat.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && IsPathLink(&fdat.cFileName[0]))continue;  // Not a real directory
   PathBuffer[PathLen+1] = 0;
   lstrcat(PathBuffer,fdat.cFileName);
   if(fdat.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)DeleteFolder(PathBuffer);
	 else DeleteFile(PathBuffer);
  }
   while(FindNextFile(hSearch,&fdat));
 FindClose(hSearch);
 return RemoveDirectory(FolderPath);
}
//---------------------------------------------------------------------------
int _stdcall CopyFileByMask(LPSTR DstDir, LPSTR FileMask, bool Overwr)
{
 WIN32_FIND_DATA fdat;
 BYTE FilePath[MAX_PATH];
 BYTE NewPath[MAX_PATH];
 HANDLE hSearch = FindFirstFile(FileMask,&fdat);
 if(hSearch == INVALID_HANDLE_VALUE)return -1;
 CreateDirectoryPath(DstDir);
 lstrcpy((LPSTR)&FilePath,FileMask);
 int didx = TrimFilePath((LPSTR)&FilePath);
 int FNum = 0;
 do
  {
   if(IsDirSpecifier((LPSTR)&fdat.cFileName) && (fdat.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))continue;
   lstrcpy((LPSTR)&FilePath[didx],(LPSTR)&fdat.cFileName);
   lstrcpy((LPSTR)&NewPath,DstDir);
   lstrcat((LPSTR)&NewPath,(LPSTR)&fdat.cFileName);
   CopyFile((LPSTR)&FilePath,(LPSTR)&NewPath,Overwr);
  }
   while(FindNextFile(hSearch,&fdat));
 FindClose(hSearch);
 return FNum;
}
//---------------------------------------------------------------------------
void _stdcall FreeAppMem(PBYTE Addon)
{
 HeapFree(GetProcessHeap(),0,Addon); 
}
//---------------------------------------------------------------------------
long _stdcall LoadAppFile(LPSTR FileName, PBYTE* AppMem, ULONG* AppLen)
{
 DWORD Result = 0;
 HANDLE hFile = CreateFile(FileName,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL|FILE_FLAG_SEQUENTIAL_SCAN,NULL);
 if(hFile == INVALID_HANDLE_VALUE){LOGMSG("Failed to open: %s", FileName);return -1;}
 *AppLen = GetFileSize(hFile,NULL);
 *AppMem = (PBYTE)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,(*AppLen+128)); 
 if(*AppLen)ReadFile(hFile,*AppMem,*AppLen,&Result,NULL);
 if(Result != *AppLen){LOGMSG("Failed to read: %s", FileName); FreeAppMem(*AppMem);return -2;}
 CloseHandle(hFile);
 return *AppLen;
}
//---------------------------------------------------------------------------
long _stdcall SaveAppFile(LPSTR FileName, PBYTE AppMem, ULONG AppLen)
{
 DWORD  Result;
 HANDLE hFile = CreateFile(FileName,GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL|FILE_FLAG_SEQUENTIAL_SCAN,NULL);
 if(hFile == INVALID_HANDLE_VALUE){LOGMSG("Failed to save file: %s", FileName);return -1;}
 WriteFile(hFile,AppMem,AppLen,&Result,NULL);
 CloseHandle(hFile);
 return Result;
}
//---------------------------------------------------------------------------
// Suspicious... Taked from 'WinApiOverride32' source
WORD _stdcall CalcSimpleCRC16(PVOID Buffer, DWORD BufferSize)
{
 if(!Buffer || !BufferSize)return 0;
 DWORD Checksum = 0;
 for(DWORD ctr=0;ctr < (BufferSize/2);ctr++)Checksum += ((PWORD)Buffer)[ctr];  
 if(BufferSize % 2)Checksum += ((PBYTE)Buffer)[BufferSize-1];
 Checksum  = (Checksum >> 16) + (Checksum & 0xffff);
 Checksum += (Checksum >> 16);
 Checksum =~ Checksum;
 return Checksum;
}
//---------------------------------------------------------------------------
// Something from USB spec.
DWORD _stdcall CalcSimpleCRC32(PVOID Buffer, DWORD BufferSize)
{
 const DWORD CRC32POLYNOME = 0xEDB88320;
 if(!Buffer || !BufferSize)return 0;
 DWORD Checksum = 0xFFFFFFFF;
 for(UINT bctr=0;bctr < BufferSize;bctr++)
  {
   Checksum ^= ((PBYTE)Buffer)[bctr];
   for(UINT ctr=0;ctr < 8;ctr++)Checksum = ((Checksum >> 1) ^ (CRC32POLYNOME & ~((Checksum & 1) - 1))); // {if((Checksum = Checksum >> 1) & 1)Checksum ^= 0xA001A001;}
  }
 return Checksum;
}
//---------------------------------------------------------------------------
LPSTR _stdcall FormatCurDateTime(LPSTR DateTimeStr)
{
 static DWORD MsgNum = 0;
 SYSTEMTIME   CurDateTime;
   
 GetLocalTime(&CurDateTime);
 wsprintf(DateTimeStr,"%u_%u.%u.%u_%u:%u:%u:%u",MsgNum,CurDateTime.wDay,CurDateTime.wMonth,CurDateTime.wYear,CurDateTime.wHour,CurDateTime.wMinute,CurDateTime.wSecond,CurDateTime.wMilliseconds);
 MsgNum++;
 return DateTimeStr;
}
//---------------------------------------------------------------------------
void _stdcall ConMessageOut(LPSTR Message, DWORD TxtAttr)
{
 DWORD  Result;
 HANDLE hConOutput;

 hConOutput = GetStdHandle(STD_OUTPUT_HANDLE);
 if(TxtAttr)SetConsoleTextAttribute(hConOutput,TxtAttr);
 WriteConsole(hConOutput,Message,lstrlen(Message),&Result,NULL);
 WriteConsole(hConOutput,"\n\r",2,&Result,NULL);
}	
//---------------------------------------------------------------------------
BOOL _stdcall SetWinConsoleSizes(DWORD WndWidth, DWORD WndHeight, DWORD BufWidth, DWORD BufHeight)
{
 HANDLE hConOutput;  
 CONSOLE_SCREEN_BUFFER_INFO ConBufInfo;

 hConOutput = GetStdHandle(STD_OUTPUT_HANDLE);
 if(!GetConsoleScreenBufferInfo(hConOutput,&ConBufInfo))return false;
 // 
 // Need to do some VERY HARD calculations here!!!
 //
 
 ConBufInfo.dwSize.X = BufWidth;
 ConBufInfo.dwSize.Y = BufHeight;
  
 if(!SetConsoleWindowInfo(hConOutput,true,&ConBufInfo.srWindow))return false;
 if(!SetConsoleScreenBufferSize(hConOutput,ConBufInfo.dwSize))return false; 
 return true;
}
//---------------------------------------------------------------------------
// CharPosFromLeft and CharPosFromRight implement as strscanc and strscans for a substring
//---------------------------------------------------------------------------
BOOL _stdcall IsUnicodeString(PVOID String)
{
 if((((BYTE*)String)[0]!=0)&&(((BYTE*)String)[1]==0)&&(((BYTE*)String)[2]!=0)&&(((BYTE*)String)[3]==0))return true;
 return false;
}
//---------------------------------------------------------------------------
/*BOOL _stdcall SetDbgFlag(BOOL Flag)
{
 BOOL  OldFlag;
 PVOID Address;

 __asm { MOV EAX,DWORD PTR FS:[0x18] 
         MOV Address,EAX }   
 DBGMSGOUT("SetDbgFlag::Value of 'FS:[18]' = %08X.", FOREGROUND_GREEN,(DWORD)Address);           
 if(Address)
  {
   Address = (PVOID)((DWORD*)Address)[12];
   DBGMSGOUT("SetDbgFlag::Value of '(FS:[18])+30' = %08X.", FOREGROUND_GREEN,(DWORD)Address);           
   if(Address)
    {
     OldFlag = ((BYTE*)Address)[2];
     ((BYTE*)Address)[2] = Flag;
     DBGMSGOUT("SetDbgFlag::Old DBG flag value = %02X.", FOREGROUND_GREEN,OldFlag);           
    } 
  }
 return OldFlag;
}*/
//---------------------------------------------------------------------------
int _stdcall GetDesktopRefreshRate(void)
{
 HDC hDCScreen = GetDC(NULL);
 int Refresh   = GetDeviceCaps(hDCScreen, VREFRESH);
 ReleaseDC(NULL, hDCScreen);
 return Refresh;
}
//---------------------------------------------------------------------------
long  _stdcall GetProcessPath(LPSTR ProcNameOrID, LPSTR PathBuffer, long BufferLngth)  // NOTE: 'lstrcmpi' will work only if the process created normally(not work if a process created in another session by a hack)
{
 PROCESSENTRY32 pent32;
 MODULEENTRY32  ment32;
 HANDLE         hProcessSnap;
 HANDLE         hModulesSnap;

 *PathBuffer   = 0;
 hProcessSnap  = INVALID_HANDLE_VALUE;
 hModulesSnap  = INVALID_HANDLE_VALUE;
 ment32.dwSize = sizeof(MODULEENTRY32);
 pent32.dwSize = sizeof(PROCESSENTRY32);
 hProcessSnap  = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
 if(Process32First(hProcessSnap, &pent32))
  {
   do                                                                                   
	{
	 if(((((ULONG_PTR)ProcNameOrID > 0xFFFF) && !lstrcmpi(ProcNameOrID, pent32.szExeFile)) || (pent32.th32ProcessID == (DWORD)ProcNameOrID))) 
	  {
	   hModulesSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pent32.th32ProcessID);
	   if(Module32First(hModulesSnap, &ment32))
		{
         do
          {
           if(lstrcmpi(pent32.szExeFile, ment32.szModule)==0)
            {
             int len = 0;
             while(ment32.szExePath[len])PathBuffer[len] = ment32.szExePath[len++];  // ++ before or after assignment?
             CloseHandle(hModulesSnap);
             return len;  
            }
          }
		   while(Module32Next(hModulesSnap, &ment32));
		}
       CloseHandle(hModulesSnap);
       break;
	  }
	}
   while(Process32Next(hProcessSnap, &pent32));
  }
 CloseHandle(hProcessSnap);
 return 0;
}
//---------------------------------------------------------------------------
/*long _stdcall GetProcessPathByHandle(HANDLE hProcess, LPSTR PathBuffer, long BufferLngth)   // Requires ntdef header
{
 PVOID         GetProcInfo;
 PEBL          ProcBlock;
 PROCPARAMS    ProcPars;
 PROCBASICINFO ProcInfo;
 wchar_t       UImagePath[MAX_PATH];      // Better allocate buffer in heap

 memset(&ProcPars,0,sizeof(ProcPars));
 memset(&ProcInfo,0,sizeof(ProcInfo));
 memset(&ProcBlock,0,sizeof(ProcBlock));
 memset(&UImagePath,0,sizeof(UImagePath));
 if( (GetProcInfo = GetProcAddress(GetModuleHandle("ntdll.dll"),"NtQueryInformationProcess")) &&
     !((DWORD (_stdcall *)(HANDLE,DWORD,PVOID,DWORD,PDWORD))GetProcInfo) (hProcess, 0, &ProcInfo, sizeof(ProcInfo), NULL) &&
     ReadProcessMemory(hProcess, ProcInfo.PebBaseAddress,&ProcBlock,sizeof(ProcBlock),NULL) &&
     ReadProcessMemory(hProcess, ProcBlock.ProcessParameters,&ProcPars,sizeof(ProcPars),NULL) &&
     ReadProcessMemory(hProcess, ProcPars.ApplicationName.Buffer,&UImagePath,ProcPars.ApplicationName.Length,NULL))
           return WideCharToMultiByte(CP_ACP,0,(PWSTR)&UImagePath,-1,PathBuffer,BufferLngth,NULL,NULL);
 return GetProcessPathByID(GetProcessId(hProcess), PathBuffer, BufferLngth);
}*/
//---------------------------------------------------------------------------
bool _stdcall FindFile(LPSTR FilePath, LPSTR OutBuffer)
{
 int    Index = -1;
 DWORD  Attrs;
 HANDLE hSearch;
 WIN32_FIND_DATA fdat;

 if((hSearch = FindFirstFile(FilePath,&fdat)) == INVALID_HANDLE_VALUE)return false;
 if(!OutBuffer)OutBuffer = FilePath; 
   else lstrcpy(OutBuffer, FilePath);
 FindClose(hSearch);  
 for(int ctr=0;OutBuffer[ctr];ctr++){if((OutBuffer[ctr] == PATHDLML)||(OutBuffer[ctr] == PATHDLMR))Index=ctr;}
 if(Index >= 0)OutBuffer[Index+1] = 0;
 lstrcat(OutBuffer, (LPSTR)&fdat.cFileName); 
 Attrs = GetFileAttributes(OutBuffer);
 if((Attrs != INVALID_FILE_ATTRIBUTES)&&!(Attrs & FILE_ATTRIBUTE_DIRECTORY))return true;  
 return false;
}
//---------------------------------------------------------------------------
DWORD _stdcall GetFileNameByHandle(HANDLE hFile, LPSTR Name)
{
 PVOID  Proc;
 DWORD  Result;
 PWSTR  NamePtr;
 BYTE   Status[8];
 struct UNI_NAME
  {
   DWORD   Length;
   wchar_t Data[MAX_PATH];
  }String;

 if(Name)Name[0] = 0;
 if(!(Proc = GetProcAddress(GetModuleHandle("ntdll.dll"),"NtQueryInformationFile")))return -1;
 Result = ((DWORD (_stdcall *)(PVOID,PVOID,PVOID,DWORD,DWORD))Proc)(hFile,(PVOID)&Status,(PVOID)&String,sizeof(String),9); // FILE_NAME_INFORMATION  // Only path, no drive letter
 if(Result)return Result;
 Result  = (String.Length/sizeof(wchar_t));
 NamePtr = (PWSTR)&String.Data;
 NamePtr[Result] = 0;
 for(int ctr=Result-1;ctr >= 0;ctr--)if((String.Data[ctr]==PATHDLMR)||(String.Data[ctr]==PATHDLML)){NamePtr = (PWSTR)&String.Data[ctr+1];break;}
 WideCharToMultiByte(CP_ACP,0,NamePtr,-1,Name,MAX_PATH,NULL,NULL);
 return 0;
}
//---------------------------------------------------------------------------
BOOL _stdcall ForceProcessSingleCore(HANDLE hProcess)
{
 DWORD_PTR ProcAffMask = 0;
 DWORD_PTR SystAffMask = 0;

 if(!GetProcessAffinityMask(hProcess,&ProcAffMask,&SystAffMask))return false;
 if(SystAffMask & 0xFFFFFFFE)  // Zero bit is always set for first core
  {
   for(UINT ctr=31;ctr > 0;ctr--) // Leave first core for something else and find next
	{
	 if(SystAffMask >> ctr)return SetProcessAffinityMask(hProcess, (((DWORD)1) << ctr));
	}
  }
 return false;
}
//---------------------------------------------------------------------------
int _stdcall ConvertFromUtf8(LPSTR DstStr, LPSTR SrcStr, UINT DstSize)
{
 WCHAR Buffer[1024];
 Buffer[0] = 0;
 MultiByteToWideChar(CP_UTF8,0,SrcStr,-1,(PWSTR)&Buffer,(sizeof(Buffer)/2));
 int len = WideCharToMultiByte(CP_ACP,0,(PWSTR)&Buffer,-1,DstStr,DstSize,NULL,NULL); 
 return len;
}
//---------------------------------------------------------------------------
int _stdcall ConvertToUtf8(LPSTR DstStr, LPSTR SrcStr, UINT DstSize)
{
 WCHAR Buffer[1024];
 Buffer[0] = 0;
 MultiByteToWideChar(CP_ACP,0,SrcStr,-1,(PWSTR)&Buffer,(sizeof(Buffer)/2));
 int len = WideCharToMultiByte(CP_UTF8,0,(PWSTR)&Buffer,-1,DstStr,DstSize,NULL,NULL);
 return len;
}
//---------------------------------------------------------------------------
int _stdcall ConvertToUTF8(PWSTR Src, LPSTR Dest, UINT DestLen)
{
 return WideCharToMultiByte(CP_UTF8,0,Src,-1,Dest,DestLen,NULL,NULL);
}
//---------------------------------------------------------------------------
//                              0001020304050607 0809101112131415
//  All bits indexed from left (d7d6d5d4d3d2d1d0 d7d6d5d4d3d2d1d0)
//
bool __stdcall GetBit(PBYTE Buffer, UINT BitIndex)
{
 UINT tmpi  = BitIndex/8;
 Buffer    += tmpi;
 BitIndex  -= (tmpi*8);
 BYTE Shift = ~BitIndex & 0x07;
 return *Buffer & (1 << Shift);
}
//---------------------------------------------------------------------------
void __stdcall SetBit(PBYTE Buffer, UINT BitIndex, bool Bit)
{
 UINT tmpi = BitIndex/8;
 Buffer   += tmpi;
 BitIndex -= (tmpi*8);
 BYTE Shift = ~BitIndex & 0x07;
 if(Bit)*Buffer |= (1 << Shift);
   else *Buffer &= ~(1 << Shift);
}
//---------------------------------------------------------------------------
// Can be used with same or different bit streams
//
void __stdcall SwapBit(PBYTE BufferA, PBYTE BufferB, UINT BitIndexA, UINT BitIndexB)
{
 UINT tmpi  = BitIndexA/8;
 BufferA   += tmpi;
 BitIndexA -= (tmpi*8);
 BYTE MaskA = (1 << (~BitIndexA & 0x07));
 bool ValA  = (*BufferA & MaskA);  // Why this won`t work in IF expression?

 tmpi       = BitIndexB/8;
 BufferB   += tmpi;
 BitIndexB -= (tmpi*8);
 BYTE MaskB = (1 << (~BitIndexB & 0x07));
 bool ValB  = (*BufferB & MaskB);  // Why this won`t work in IF expression?

 if(ValA)*BufferB |= MaskB;
   else *BufferB &= ~MaskB;

 if(ValB)*BufferA |= MaskA;
   else *BufferA &= ~MaskA;
}
//---------------------------------------------------------------------------
// Buffer must be at least twice size of BitCnt in bytes
//
void __stdcall SwapBits(PBYTE Buffer, UINT BitIndex, UINT BitCnt)
{
 for(UINT ctr=0;ctr<BitCnt;ctr++)SwapBit(Buffer,Buffer, BitIndex+ctr, BitIndex+BitCnt+ctr);
}
//---------------------------------------------------------------------------
DWORD _stdcall MakeBitMask(UINT Offset, UINT Count)
{
 if(!Count)return 0;
 return (((DWORD)-1) >> ((sizeof(DWORD)*8)-Count)) << Offset;
}
//---------------------------------------------------------------------------
UINT _stdcall CountBits(DWORD Value, UINT MaxBits, bool Bit)
{
 UINT  Total = 0;
 DWORD Msk   = Bit;
 for(UINT ctr=0;ctr < MaxBits;ctr++)
  {
   Total += !((Value&1) ^ Msk);
   Value >>= 1;
  }
 return Total;
}
//---------------------------------------------------------------------------
int _stdcall BitPos(DWORD Value, UINT MaxBits, bool Bit)
{
 UINT  Total = 0;
 DWORD Msk   = Bit;
 for(UINT ctr=0;ctr < MaxBits;ctr++)
  {
   if((Value&1) == Msk)return ctr;
   Value >>= 1;
  }
 return -1;
}
//---------------------------------------------------------------------------
int _stdcall MaxOnePos(DWORD Value, UINT MaxBits)
{
 UINT MaxPos = -1;
 for(UINT ctr=0;(ctr < MaxBits)&&Value;ctr++)
  {
   Value >>= 1;
   MaxPos  = ctr;
  }
 return MaxPos;
}
//---------------------------------------------------------------------------
UINT _stdcall GetRandomValue(UINT MinVal, UINT MaxVal)
{
 LARGE_INTEGER RndSeed;
 if(QueryPerformanceCounter(&RndSeed));
  else RndSeed.QuadPart = GetTickCount();

 UINT   RndMul = 134775813 * RndSeed.QuadPart + 1;
 UINT64 Value  = MaxVal-MinVal+1;
 UINT64 RndVal = ((UINT)RndMul * Value) >> 32;
 return (MinVal + RndVal);
}
//---------------------------------------------------------------------------
HMODULE _stdcall FindModuleByExpName(LPSTR ModuleName)              // For modules with EXPORT directory
{
 MODULEENTRY32 ment32;
 ment32.dwSize = sizeof(MODULEENTRY32);
 HANDLE hModulesSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());
 if(Module32First(hModulesSnap, &ment32))
  {
   do
    {
     if(!IsValidPEHeader(ment32.hModule))continue;
     LPSTR MName = GetExpModuleName(ment32.hModule, false);
     if(!lstrcmpiA(MName,ModuleName))
      {
       CloseHandle(hModulesSnap);
       return ment32.hModule; 
      }
    }
	 while(Module32Next(hModulesSnap, &ment32));
  }
 CloseHandle(hModulesSnap);
 return NULL;
}
//---------------------------------------------------------------------------
