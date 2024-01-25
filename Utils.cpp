
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

#include "ThirdParty\NTDLL\ntdll.h"
#include "Utils.h"
#include "Common.hpp"
#include <Wbemidl.h>
//====================================================================================
                                            // sprintf: hex, bits

#ifndef NOLOG
HANDLE hLogFile = NULL;
HANDLE hConsOut = NULL;

wchar_t   LogFilePath[MAX_PATH];	
int    LogMode = lmNone;	
int    MaxLogLevel = 9;  // Log everything  // 0 will log nothing but still form log messages
void _cdecl DummyLogProc(LPSTR, UINT, UINT){}
void (_cdecl *pLogProc)(LPSTR Msh, UINT MsgLen, UINT Level) = DummyLogProc;			   
#endif			   			   
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
void*  __cdecl memset(void* _Dst, int _Val, size_t _Size)      // TODO: Aligned, SSE by MACRO
{
 size_t ALen = _Size/sizeof(size_t);
 size_t BLen = _Size%sizeof(size_t);
 size_t DVal =_Val & 0xFF;               // Bad and incorrect For x32: '(size_t)_Val * 0x0101010101010101;'         // Multiply by 0x0101010101010101 to copy the lowest byte into all other bytes
 if(DVal)
  {
   DVal = _Val | (_Val << 8) | (_Val << 16) | (_Val << 24);
#ifdef _AMD64_
   DVal |= DVal << 32;
#endif
  }
 for(size_t ctr=0;ctr < ALen;ctr++)((size_t*)_Dst)[ctr] = DVal; 
 for(size_t ctr=(ALen*sizeof(size_t));ctr < _Size;ctr++)((char*)_Dst)[ctr] = DVal;  
 return _Dst;
} 
//---------------------------------------------------------------------------
int    __cdecl memcmp(const void* _Buf1, const void* _Buf2, size_t _Size) // '(*((ULONG**)&_Buf1))++;'
{
 unsigned char* BufA = (unsigned char*)_Buf1;
 unsigned char* BufB = (unsigned char*)_Buf2; 
 for(;_Size >= sizeof(size_t); _Size-=sizeof(size_t), BufA+=sizeof(size_t), BufB+=sizeof(size_t))  // Enters here only if Size >= sizeof(ULONG)
  {
   if(*((size_t*)BufA) != *((size_t*)BufB))break;  // Have to break and continue as bytes because return value must be INT  // return (*((intptr_t*)BufA) - *((intptr_t*)BufB));  //  // TODO: Move everything to multiplatform FRAMEWORK
  }  
 for(;_Size > 0; _Size--, BufA++, BufB++)  // Enters here only if Size > 0
  {
   if(*((unsigned char*)BufA) != *((unsigned char*)BufB)){return ((int)*BufA - (int)*BufB);}   
  }			   
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
extern "C" void* __cdecl realloc(void* _Block, size_t _Size)
{
 return HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,_Block,_Size);
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
#ifndef _NODESTR
 HeapFree(GetProcessHeap(),0,p);
#endif
} 
//---------------------------------------------------------------------------
void __cdecl operator delete(void* p, size_t n)
{
#ifndef _NODESTR
 HeapFree(GetProcessHeap(),0,p);
#endif
}
//---------------------------------------------------------------------------
void __cdecl operator delete[](void* p)
{
#ifndef _NODESTR
 HeapFree(GetProcessHeap(),0,p);
#endif
}
//---------------------------------------------------------------------------
void __cdecl operator delete[](void* p, size_t n)
{
#ifndef _NODESTR
 HeapFree(GetProcessHeap(),0,p);
#endif
}   
//---------------------------------------------------------------------------


#ifndef NOMINIRTL 

#ifndef NOFLT 
extern "C" int _fltused = 0;
#endif

extern "C" int __cdecl _purecall(void) { return 0; }

extern "C" int __cdecl atexit( void (__cdecl *func )( void ) )   // MINIRTL ?
{
 if(func)func();
 return 0;
}

/*extern "C" const DWORD_PTR __security_cookie = 0xE64EBB40;

extern "C" void __fastcall __security_check_cookie(DWORD_PTR cookie)
{

} */
//---------------------------------------------------------------------------
#ifndef _M_X64
extern "C" __declspec(naked) float _cdecl _CIsqrt(float &Value)
{
 __asm FSQRT
 __asm RET
}
//---------------------------------------------------------------------------
extern "C" __declspec(naked) long _cdecl _ftol2(float &Value)  // What is FPU rounding mode? Should be TRUNCATE
{
 __asm PUSH EAX
 __asm PUSH EAX
 __asm FISTP QWORD PTR [ESP]    // SSE3: FISTTP converts the value in ST into a signed integer using truncation (chop) as rounding mode,
 __asm POP  EAX
 __asm POP  EDX
 __asm RET
}
//---------------------------------------------------------------------------
static const unsigned long long _Int32ToUInt32[] = {0ULL, 0x41F0000000000000ULL};

extern "C" __declspec(naked) double _cdecl _ultod3(unsigned long long val)  // needed for SSE code for 32-bit arch   // SSE2
{
 __asm
 {
   xorps   xmm0, xmm0
   cvtsi2sd xmm0, ecx
   shr     ecx, 31
   addsd   xmm0, ds:_Int32ToUInt32[ecx*8]
   test    edx, edx
   jz      short _ultod3_uint32
   xorps   xmm1, xmm1
   cvtsi2sd xmm1, edx
   shr     edx, 31
   addsd   xmm1, ds:_Int32ToUInt32[edx*8]
   mulsd   xmm1, ds:_Int32ToUInt32[8]     //  _DP2to32
   addsd   xmm0, xmm1
 _ultod3_uint32:                       
   retn
 }
}
//---------------------------------------------------------------------------
extern "C" __declspec(naked) double _cdecl _ltod3(long long val)  // needed for SSE code for 32-bit arch   // SSE2
{
 __asm
 {
   xorps   xmm1, xmm1
   cvtsi2sd xmm1, edx
   xorps   xmm0, xmm0
   cvtsi2sd xmm0, ecx
   shr     ecx, 31
   mulsd   xmm1, ds:_Int32ToUInt32[8]          //_DP2to32
   addsd   xmm0, ds:_Int32ToUInt32[ecx*8]
   addsd   xmm0, xmm1
   retn
 }
}
//---------------------------------------------------------------------------
//  Only works for inputs in the range: [0, 2^52)  // SSE2
unsigned long long double_to_uint64(__m128d x)    // Generated a bad optimized code!    // NOTE: Incorrect for a negative x. Good only for ftoa_simple
{  
// x = _mm_sub_pd(x, _mm_set_sd(0.5));    // Remove rounding effect
 x = _mm_add_pd(x, _mm_set1_pd(0x0010000000000000-0.5));     // No rounding now!
 __m128i res = _mm_xor_si128(_mm_castpd_si128(x), _mm_castpd_si128(_mm_set1_pd(0x0010000000000000)));
 return *(unsigned long long*)&res;  
}

extern "C" __declspec(naked) unsigned long long _cdecl _dtoul3(double val) // needed for SSE code for 32-bit arch   // double passed in xmm0 and not retrievable by C++ code!   //      // Truncating
{
 __asm jmp double_to_uint64
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
//---------------------------------------------------------------------------
extern "C" __declspec(naked)  int _cdecl _aulldvrm(unsigned __int64 dividend, unsigned __int64 divisor)   
{
__asm
{
   PUSH    ESI
   MOV     EAX, [ESP+0x14]
   OR      EAX, EAX
   JNZ     SHORT LOC_4B2EC641
   MOV     ECX, [ESP+0x10]
   MOV     EAX, [ESP+0x0C]
   XOR     EDX, EDX
   DIV     ECX
   MOV     EBX, EAX
   MOV     EAX, [ESP+8]
   DIV     ECX
   MOV     ESI, EAX
   MOV     EAX, EBX
   MUL     DWORD PTR [ESP+0x10]
   MOV     ECX, EAX
   MOV     EAX, ESI
   MUL     DWORD PTR [ESP+0x10]
   ADD     EDX, ECX
   JMP     SHORT LOC_4B2EC688
LOC_4B2EC641:                          
   MOV     ECX, EAX
   MOV     EBX, [ESP+0x10]
   MOV     EDX, [ESP+0x0C]
   MOV     EAX, [ESP+8]
LOC_4B2EC64F:                          
   SHR     ECX, 1
   RCR     EBX, 1
   SHR     EDX, 1
   RCR     EAX, 1
   OR      ECX, ECX
   JNZ     SHORT LOC_4B2EC64F
   DIV     EBX
   MOV     ESI, EAX
   MUL     DWORD PTR [ESP+0x14]
   MOV     ECX, EAX
   MOV     EAX, [ESP+0x10]
   MUL     ESI
   ADD     EDX, ECX
   JB      SHORT LOC_4B2EC67D
   CMP     EDX, [ESP+0x0C]
   JA      SHORT LOC_4B2EC67D
   JB      SHORT LOC_4B2EC686
   CMP     EAX, [ESP+8]
   JBE     SHORT LOC_4B2EC686
LOC_4B2EC67D:                                                                 
   DEC     ESI
   SUB     EAX, [ESP+0x10]
   SBB     EDX, [ESP+0x14]
LOC_4B2EC686:                                                                
   XOR     EBX, EBX
LOC_4B2EC688:                         
   SUB     EAX, [ESP+8]
   SBB     EDX, [ESP+0x0C]
   NEG     EDX
   NEG     EAX
   SBB     EDX, 0
   MOV     ECX, EDX      // Unusual return format (2 results, 128 bit)
   MOV     EDX, EBX
   MOV     EBX, ECX
   MOV     ECX, EAX
   MOV     EAX, ESI
   POP     ESI
   RETN    16
}
}
//---------------------------------------------------------------------------
extern "C" __declspec(naked)  int _cdecl _aullrem(unsigned __int64 dividend, unsigned __int64 divisor)   
{
__asm
{
   PUSH    EBX
   MOV     EAX, [ESP+0x14]
   OR      EAX, EAX
   JNZ     SHORT LOC_4B2EC6D1
   MOV     ECX, [ESP+0x10]
   MOV     EAX, [ESP+0x0C]
   XOR     EDX, EDX
   DIV     ECX
   MOV     EAX, [ESP+8]
   DIV     ECX
   MOV     EAX, EDX
   XOR     EDX, EDX
   JMP     SHORT LOC_4B2EC721
LOC_4B2EC6D1:                           
   MOV     ECX, EAX
   MOV     EBX, [ESP+0x10]
   MOV     EDX, [ESP+0x0C]
   MOV     EAX, [ESP+8]
LOC_4B2EC6DF:                           
   SHR     ECX, 1
   RCR     EBX, 1
   SHR     EDX, 1
   RCR     EAX, 1
   OR      ECX, ECX
   JNZ     SHORT LOC_4B2EC6DF
   DIV     EBX
   MOV     ECX, EAX
   MUL     DWORD PTR [ESP+0x14]
   XCHG    EAX, ECX
   MUL     DWORD PTR [ESP+0x10]
   ADD     EDX, ECX
   JB      SHORT LOC_4B2EC70A
   CMP     EDX, [ESP+0x0C]
   JA      SHORT LOC_4B2EC70A
   JB      SHORT LOC_4B2EC712
   CMP     EAX, [ESP+8]
   JBE     SHORT LOC_4B2EC712
LOC_4B2EC70A:                         
   SUB     EAX, [ESP+0x10]
   SBB     EDX, [ESP+0x14]
LOC_4B2EC712:                          
   SUB     EAX, [ESP+8]
   SBB     EDX, [ESP+0x0C]
   NEG     EDX
   NEG     EAX
   SBB     EDX, 0
LOC_4B2EC721:                        
   POP     EBX
   RETN    16
}
}
//---------------------------------------------------------------------------



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
__declspec(noinline) PVOID _fastcall FixExportRedir(PVOID ThisLibBase, PVOID ExpProcAddr, PWSTR LibPath)
{
 LPSTR ProcName = NPEFMT::TGetProcedureInfoByAddr<NPEFMT::PECURRENT>((PBYTE)ThisLibBase, ExpProcAddr); 
 HMODULE hOrig = LoadLibraryW(LibPath);    // Not GetModuleHandle since both have the same name 
 DBGMSG("ThisLibBase=%p, OrigLibBase=%p, ExpProcAddr=%p, ExpProcName=%s = %ls",ThisLibBase, hOrig, ExpProcAddr, ProcName,    LibPath);
 return GetProcAddress(hOrig, ProcName); 
}
//---------------------------------------------------------------------------

bool _stdcall IsLogHandle(HANDLE Hnd)
{
#ifdef NOLOG
 return false;
#else
 return ((Hnd == hLogFile)||(Hnd == hConsOut));
#endif
}
//---------------------------------------------------------------------------
// !!! -0.177166 is incorrectly printed with %f!!!
// OPt: InterprocSync, Reopen/Update log // Depth // Fastcall, Max 3 constant args
// float pushed as double, char pushed as a signed size_t
// NOTE: Incorrectly receives UINT64 on x32 !!!!!!!
//
void  _cdecl LogProc(int Flags, char* ProcName, char* Message, ...)
{
#ifndef NOLOG
 va_list args;
 static volatile ULONG  MsgIdx  = 0;
 static volatile UINT64 PrvTime = 0;
 NTSTATUS Status;
 UINT  MSize = 0;
 int  LogLvl = Flags & 0xFF;
 char* MPtr  = NULL;
 char TmpBuf[1024*4];   // avoid chkstk

 if(!Message || !LogMode)return;   // NULL text string
 va_start(args,Message);
 ULONG MIndex = _InterlockedIncrement((long*)&MsgIdx);
 if(!(Flags & lfRawTextMsg))   // Format a normal message
  {   
   MPtr = TmpBuf;
   UINT Len = 0;
   if(Flags & lfLogTime)
    {
     NNTDLL::GetAbstractTimeStamp(&PrvTime);           // NNTDLL::GetTicks();
     ConvertToHexStr(PrvTime, 16, TmpBuf, true, &Len); // Ticks time
     MSize += Len;
     TmpBuf[MSize++] = 0x20;
    }
   if(Flags & lfLogMsgIdx)
    {
     ConvertToHexStr(MIndex, 8, &TmpBuf[MSize], true, &Len);  // Message Index (For message order detection)
     MSize += Len;
     TmpBuf[MSize++] = 0x20;
    }
   if(Flags & lfLogThID)
    {
     ConvertToHexStr(NtCurrentThreadId(), 6, &TmpBuf[MSize], true, &Len);   // Thread ID
     MSize += Len;
     TmpBuf[MSize++] = 0x20;
    }
   if(Flags & lfLogLevel)   
    {
     if(LogLvl==llLogVFail)*(UINT32*)&TmpBuf[MSize] = 'LIAF';  //FAIL      // Mind the byte order!
     else if(LogLvl==llLogVError)*(UINT32*)&TmpBuf[MSize] = 'RRRE';  //ERRR      // Mind the byte order!
     else if(LogLvl==llLogVWarning)*(UINT32*)&TmpBuf[MSize] = 'NRAW';  //WARN 
     else if(LogLvl==llLogVNote)*(UINT32*)&TmpBuf[MSize] = 'ETON';  //NOTE 
     else if(LogLvl==llLogVInfo)*(UINT32*)&TmpBuf[MSize] = 'OFNI';  //INFO  
     else if(LogLvl==llLogVDebug)*(UINT32*)&TmpBuf[MSize] = 'GBED';  //DEBG  
     else if(LogLvl==llLogVTrace)*(UINT32*)&TmpBuf[MSize] = 'ECRT';  //TRCE  
     else *(UINT32*)&TmpBuf[MSize] = 'ENON';  // NONE
     MSize += 4;
     TmpBuf[MSize++] = 0x20;
    }
   if(ProcName && (Flags & lfLogName))    
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
   MSize += FormatToBuffer(Message, &TmpBuf[MSize], (sizeof(TmpBuf)-3)-MSize, args);
   if(Flags & lfLineBreak)
    {
     TmpBuf[MSize++] = '\r';
     TmpBuf[MSize++] = '\n';
    }
   TmpBuf[MSize]   = 0;  // User`s LogProc may want this
  }
   else  // Log raw text (No line break supported)
    {
     MSize = ProcName?((UINT)ProcName):(NSTR::StrLen(Message));  // ProcName is message size 
     MPtr  = Message;
    }
 va_end(args);

 if(LogLvl && (LogLvl >= MaxLogLevel))return;   // Skip logging  // Log any unleveled messages only
 if(LogMode & lmProc)pLogProc(MPtr, MSize, LogLvl);
 if(LogMode & lmFile)
  {                                                                                                                 
   if(!hLogFile)  // NOTE: Without FILE_APPEND_DATA offsets must be specified to NtWriteFile     // https://nblumhardt.com/2016/08/atomic-shared-log-file-writes/
    {          // (LogMode & lmFileUpd)?FILE_APPEND_DATA:FILE_WRITE_DATA       //           //  LogMode |= lmFileUpd;
     Status = NCMN::NNTDLL::FileCreateSync(LogFilePath, FILE_APPEND_DATA, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ|FILE_SHARE_WRITE, (LogMode & lmFileUpd)?FILE_OPEN_IF:FILE_OVERWRITE_IF, FILE_NON_DIRECTORY_FILE, &hLogFile);
    }
     else Status = 0;
   if(!Status)
    {
     LARGE_INTEGER offset = {};
     IO_STATUS_BLOCK iosb = {};
     Status = NtWriteFile(hLogFile, NULL, NULL, NULL, &iosb, MPtr, MSize, &offset, NULL);
    }
  }
 if(LogMode & lmCons)
  {           
   IO_STATUS_BLOCK iosb = {};
   if(!hConsOut)hConsOut = NtCurrentPeb()->ProcessParameters->StandardOutput;               // Not a real file handle on Windows 7(Real for an redirected IO?)!      // Win8+: \Device\ConDrv\Console     // https://stackoverflow.com/questions/47534039/windows-console-handle-for-con-device
   if((UINT)hConsOut != 7)Status = NtWriteFile(hConsOut, NULL, NULL, NULL, &iosb, MPtr, MSize, NULL, NULL);        // Will not work until Windows 8
  }
#endif
}
//---------------------------------------------------------------------------
enum EFlags {
 FLAGS_ZEROPAD   = (1U <<  0U),
 FLAGS_LEFT      = (1U <<  1U),
 FLAGS_PLUS      = (1U <<  2U),
 FLAGS_SPACE     = (1U <<  3U),
 FLAGS_HASH      = (1U <<  4U),
 FLAGS_UPPERCASE = (1U <<  5U),
 FLAGS_CHAR      = (1U <<  6U),
 FLAGS_SHORT     = (1U <<  7U),
 FLAGS_LONG      = (1U <<  8U),
 FLAGS_LONG_LONG = (1U <<  9U),
 FLAGS_PRECISION = (1U << 10U),
 FLAGS_ADAPT_EXP = (1U << 11U)
};

inline bool _is_digit(char ch)
{
 return (ch >= '0') && (ch <= '9');
}
//---------------------------------------------------------------------------
// internal ASCII string to unsigned int conversion
inline unsigned int _atoi(char** Str)
{
 unsigned int x = 0;
 for(unsigned char ch;(ch=*(*Str) - '0') <= 9;(*Str)++)x = (x*10) + ch;        // Can use a different base?
 return x;
}
//---------------------------------------------------------------------------
template<typename T> inline unsigned int _strnlen_s(T str, size_t maxsize)
{
 if(!str)return 0;
 T s = str;
 for (; *s && maxsize--; ++s);
 return (unsigned int)(s - str);
}
//---------------------------------------------------------------------------
template<typename T> size_t _ntoa(char* buffer, size_t idx, size_t maxlen, T value, bool negative, T base, unsigned int prec, unsigned int width, unsigned int flags)
{
 char buf[32];
 size_t len = 0U;
 if (!value)flags &= ~FLAGS_HASH;   // no hash for 0 values
 // write if precision != 0 and value is != 0
 if (!(flags & FLAGS_PRECISION) || value) {
    do {
      const char digit = (char)(value % base);    // Not cheap!!!
      buf[len++] = digit < 10 ? '0' + digit : (flags & FLAGS_UPPERCASE ? 'A' : 'a') + digit - 10;
      value /= base;
    } while (value && (len < sizeof(buf)));
  }

  // pad leading zeros
  if (!(flags & FLAGS_LEFT)) {
    if (width && (flags & FLAGS_ZEROPAD) && (negative || (flags & (FLAGS_PLUS | FLAGS_SPACE))))width--;    
    while ((len < prec) && (len < sizeof(buf)))buf[len++] = '0';
    while ((flags & FLAGS_ZEROPAD) && (len < width) && (len < sizeof(buf)))buf[len++] = '0';  
  }

  // handle hash
  if (flags & FLAGS_HASH) {
    if (!(flags & FLAGS_PRECISION) && len && ((len == prec) || (len == width)) && (--len && (base == 16U)))len--;   
    if ((base == 16U) && !(flags & FLAGS_UPPERCASE) && (len < sizeof(buf)))buf[len++] = 'x';
    else if ((base == 16U) && (flags & FLAGS_UPPERCASE) && (len < sizeof(buf)))buf[len++] = 'X';
    else if ((base == 2U) && (len < sizeof(buf)))buf[len++] = 'b';   
    if(len < sizeof(buf))buf[len++] = '0';
  }

  if (len < sizeof(buf)) {
    if (negative)buf[len++] = '-';   
    else if (flags & FLAGS_PLUS)buf[len++] = '+';  // ignore the space if the '+' exists
    else if (flags & FLAGS_SPACE)buf[len++] = ' ';
  }

  const size_t start_idx = idx;
  // pad spaces up to given width
  if (!(flags & FLAGS_LEFT) && !(flags & FLAGS_ZEROPAD)) {
    for (size_t i = len; i < width; i++)buffer[idx++] = ' ';   // out(' ', buffer, idx++, maxlen);
  }

  // reverse string
  if((idx+len) > maxlen)return idx;     // ???
  while (len)buffer[idx++] = buf[--len];

  // append pad spaces up to given width
 if (flags & FLAGS_LEFT){while (idx - start_idx < width)buffer[idx++] = ' ';}    // out(' ', buffer, idx++, maxlen);    //    if(((width - (idx - start_idx)) + idx) > maxlen)   // Too costlly to check  
 return idx;
}
//---------------------------------------------------------------------------
// TODO: Put this and all num-to-str/str-to-num functions into Format.hpp (mamespace NFMT)
// TODO: Use a template to generate a type validation string from all passed arguments
// TODO: Repeated chars
// TODO: Add char/str repeat option, useful for logging trees
// TODO: Indexed arg reuse
//
int _stdcall FormatToBuffer(char* format, char* buffer, UINT maxlen, va_list va)
{
 size_t idx = 0U;
 while(*format && (idx < (size_t)maxlen))   // format specifier?  %[flags][width][.precision][length]
  {
   // Check if the next 4 bytes contain %(0x25) or end of string. Using the 'hasless' trick: https://graphics.stanford.edu/~seander/bithacks.html#HasLessInWord
//   v = *(stbsp__uint32 *)f;
//   c = (~v) & 0x80808080;
//   if (((v ^ 0x25252525) - 0x01010101) & c)goto schk1;
//   if ((v - 0x01010101) & c)goto schk2;

   if(*format != '%'){buffer[idx++]=*(format++); continue;}  
   format++; 

   // evaluate flags 
   unsigned int n;
   unsigned int flags = 0U;    
    do { switch(*format) {
        case '0': flags |= FLAGS_ZEROPAD; format++; n = 1U; break;
        case '-': flags |= FLAGS_LEFT;    format++; n = 1U; break;
        case '+': flags |= FLAGS_PLUS;    format++; n = 1U; break;
        case ' ': flags |= FLAGS_SPACE;   format++; n = 1U; break;
        case '#': flags |= FLAGS_HASH;    format++; n = 1U; break;
        default :                                   n = 0U; break;
      } } while (n);
    
    // evaluate width field
    unsigned int width = 0U;
    if (_is_digit(*format))width = _atoi(&format);
    else if (*format == '*') {
      const int w = va_arg(va, int);
      if (w < 0) {flags |= FLAGS_LEFT; width = (unsigned int)-w;}    // reverse padding        
      else  width = (unsigned int)w;
      format++;
    }

    // evaluate precision field
    unsigned int precision = 0U;
    if (*format == '.') {
      flags |= FLAGS_PRECISION;
      format++;
      if (_is_digit(*format))precision = _atoi(&format);      
      else if (*format == '*') {
        const int prec = (int)va_arg(va, int);
        precision = prec > 0 ? (unsigned int)prec : 0U;
        format++;
      }
    }
    
    // evaluate length field
    switch (*format) {     
      case 'l' :
        if(*(++format) == 'l'){flags |= FLAGS_LONG_LONG; format++;}
          else flags |= FLAGS_LONG;      
        break;
      case 'h':
        if(*(++format) == 'h'){flags |= FLAGS_CHAR; format++;}
          else flags |= FLAGS_SHORT; 
        break;
/*      case 't' :
        flags |= (sizeof(ptrdiff_t) == sizeof(long) ? FLAGS_LONG : FLAGS_LONG_LONG);
        format++;
        break;
      case 'j' :
        flags |= (sizeof(intmax_t) == sizeof(long) ? FLAGS_LONG : FLAGS_LONG_LONG);
        format++;
        break;  */
      case 'z' :
        flags |= (sizeof(size_t) == sizeof(long) ? FLAGS_LONG : FLAGS_LONG_LONG);
        format++;
        break;
      default :
        break;
    }
 
    switch (*format) {       // evaluate specifier
//      case 'd' :
      case 'i' :
      case 'u' :
      case 'x' :
      case 'X' :
      case 'o' :
      case 'b' : {
        // set the base
        unsigned int base;
        if (*format == 'x' || *format == 'X')base = 16U;        
        else if (*format == 'o')base = 8U;
        else if (*format == 'b')base = 2U;
        else {base = 10U; flags &= ~FLAGS_HASH;}   // no hash for dec format
                
        if (*format == 'X')flags |= FLAGS_UPPERCASE;     // uppercase       
        if (*format != 'i')flags &= ~(FLAGS_PLUS | FLAGS_SPACE);    // no plus or space flag for u, x, X, o, b   //  && (*format != 'd')
        // ignore '0' flag when precision is given
        if (flags & FLAGS_PRECISION)flags &= ~FLAGS_ZEROPAD;
        // convert the integer
        if (*format == 'i') {   // signed         //  || (*format == 'd')         
          if (flags & FLAGS_LONG_LONG) {
            const long long value = va_arg(va, long long);
            idx = _ntoa<unsigned long long>(buffer, idx, maxlen, (unsigned long long)(value > 0 ? value : 0 - value), value < 0, base, precision, width, flags);
          }
          else if (flags & FLAGS_LONG) {
            const long value = va_arg(va, long);
            idx = _ntoa<unsigned long>(buffer, idx, maxlen, (unsigned long)(value > 0 ? value : 0 - value), value < 0, base, precision, width, flags);
          }
          else {
            const int value = (flags & FLAGS_CHAR) ? (char)va_arg(va, int) : (flags & FLAGS_SHORT) ? (short int)va_arg(va, int) : va_arg(va, int);
            idx = _ntoa<unsigned long>(buffer, idx, maxlen, (unsigned int)(value > 0 ? value : 0 - value), value < 0, base, precision, width, flags);
          }
        }
        else {   // unsigned          
          if (flags & FLAGS_LONG_LONG)idx = _ntoa<unsigned long long>(buffer, idx, maxlen, va_arg(va, unsigned long long), false, base, precision, width, flags);
          else if (flags & FLAGS_LONG)idx = _ntoa<unsigned long>(buffer, idx, maxlen, va_arg(va, unsigned long), false, base, precision, width, flags);
          else {
            const unsigned int value = (flags & FLAGS_CHAR) ? (unsigned char)va_arg(va, unsigned int) : (flags & FLAGS_SHORT) ? (unsigned short int)va_arg(va, unsigned int) : va_arg(va, unsigned int);
            idx = _ntoa<unsigned long>(buffer, idx, maxlen, value, false, base, precision, width, flags);
          }
        }
        format++;
        break;
      }
      case 'f' :
      case 'F' : {
        size_t prec = (flags & FLAGS_PRECISION)?precision:14;   // Is 14 optimal?
        if(*format == 'F') {flags |= FLAGS_UPPERCASE; prec |= 0x1000;}  // ffUpperCase
        if(flags & FLAGS_HASH)prec |= 0x8000;       // ffZeroPad                      // ffCommaSep  // Or may be ffZeroPad 0x8000? 
        size_t len = 0;
        char buf[52];
        char* ptr = ftoa_simple(va_arg(va, double), prec, buf, sizeof(buf), &len);   // After this we have some space at beginning of the buffer
        bool negative = (*ptr == '-');
        if (!(flags & FLAGS_LEFT) && (flags & FLAGS_ZEROPAD)) {
          if (width && (negative || (flags & (FLAGS_PLUS | FLAGS_SPACE)))) width--;   
          while ((len < width) && (len < sizeof(buf))){*(--ptr) = '0'; len++;}   // Width adds zeroes BEFORE
        }
        if ((len < sizeof(buf)) && !negative) {  
          if (flags & FLAGS_PLUS){*(--ptr) = '+'; len++;} // ignore the space if the '+' exists    
          else if (flags & FLAGS_SPACE){*(--ptr) = ' '; len++;} 
        } 
        const size_t start_idx = idx;
        if (!(flags & FLAGS_LEFT) && !(flags & FLAGS_ZEROPAD)) {    // pad spaces up to given width         // TODO: CopyPadded inline function
          for (size_t i = len; i < width; i++)buffer[idx++] = ' '; }  // out(' ', buffer, idx++, maxlen);  
        while(len--)buffer[idx++] = *(ptr++);       //out(buf[--len], buffer, idx++, maxlen); // reverse string 
        if (flags & FLAGS_LEFT) {     // append pad spaces up to given width
          while (idx - start_idx < width)buffer[idx++] = ' '; }  // out(' ', buffer, idx++, maxlen);             
        format++; }
        break;
#if defined(PRINTF_SUPPORT_EXPONENTIAL)
      case 'e':
      case 'E':
      case 'g':
      case 'G':
        if ((*format == 'g')||(*format == 'G')) flags |= FLAGS_ADAPT_EXP;
        if ((*format == 'E')||(*format == 'G')) flags |= FLAGS_UPPERCASE;
        idx = _etoa(out, buffer, idx, maxlen, va_arg(va, double), precision, width, flags);
        format++;
        break;
#endif  // PRINTF_SUPPORT_EXPONENTIAL
      case 'c' : {
        unsigned int l = 1U;       
        if (!(flags & FLAGS_LEFT)){while (l++ < width)buffer[idx++] = ' ';}  // pre padding         //  TODO: Buffer limit               
        buffer[idx++] = (char)va_arg(va, int);  // char output       // out((char)va_arg(va, int), buffer, idx++, maxlen);       
        if (flags & FLAGS_LEFT){while (l++ < width)buffer[idx++] = ' ';}    // post padding          
        format++;
        break;
      }

      case 's' : {
        char* cp;
        wchar_t* wp;
        unsigned int l;
        if(flags & FLAGS_LONG)
         {
          wp = va_arg(va, wchar_t*);
          l  = _strnlen_s(wp, precision ? precision : (size_t)-1);    // precision or a full size 
         }
        else
         {
          cp = va_arg(va, char*);
          l  = _strnlen_s(cp, precision ? precision : (size_t)-1);    // precision or a full size 
         }
        unsigned int f = l;    // Full len  of the string       
        if(flags & FLAGS_PRECISION) f = l = (l < precision ? l : precision);    // Not greater than precision or 0
        if(!(flags & FLAGS_LEFT)){while (l++ < width)buffer[idx++] = ' ';}      // pre padding       
        if(flags & FLAGS_LONG)idx += NUTF::Utf16To8(buffer, wp, f, idx, 0);      
         else {for (;f;f--)buffer[idx++] = *(cp++);}   // string output         //  out(*(p++), buffer, idx++, maxlen);  // while ((*p != 0) && (!(flags & FLAGS_PRECISION) || precision--))buffer[idx++] = *(p++);       
        if(flags & FLAGS_LEFT){while (l++ < width)buffer[idx++] = ' ';}         // post padding
        format++;
        break;
      }

      case 'd' :                     // LOGMSG("\r\n%#*.32D",Size,Src);
      case 'D' : {   // Width is data block size, precision is line size(single line if not specified)  // '%*D' counted dump
        if(*format == 'D')flags |= FLAGS_UPPERCASE;
        unsigned char* DPtr  = va_arg(va, unsigned char*);
        unsigned int   RLen  = precision?precision:width;   // If no precision is specified then write everything in one line 
        unsigned int DelMult = (flags & FLAGS_SPACE)?3:2;
        for(unsigned int offs=0,roffs=0,rpos=0;idx < maxlen;offs++,roffs++)
         {
          bool HaveMore = offs < width;
          if(!HaveMore || (roffs >= RLen))
           {
            if(flags & FLAGS_HASH)   // Include char dump
             {
              unsigned int IndCnt = (flags & FLAGS_SPACE)?1:2;
              IndCnt += (RLen - roffs) * DelMult;  // Indent missing HEX space     //if(!SingleLine)
              if((idx+IndCnt) > maxlen)goto Exit;    // No more space
              memset(&buffer[idx], ' ', IndCnt);
              idx += IndCnt;            
              for(unsigned int ctr=0;(ctr < roffs)&&(idx < maxlen);ctr++)   // Create Text string
               {
                unsigned char Val = DPtr[rpos+ctr];
                if(Val < 0x20)Val = '.';
                buffer[idx++] = Val;
               }
             }
            if(!HaveMore || ((idx+4) > maxlen))break;
            buffer[idx++] = '\r';      // Add only if there is another line follows
            buffer[idx++] = '\n';
            rpos  = offs;
            roffs = 0;
           }
          unsigned short Val = HexToChar(DPtr[offs]);
          buffer[idx++] = Val;
          buffer[idx++] = Val >> 8;
          if(flags & FLAGS_SPACE)buffer[idx++] = ' ';
         } 
        format++; 
        break; }

      case 'p' : {
//        width = sizeof(void*) * 2U;
        flags |= FLAGS_ZEROPAD | FLAGS_UPPERCASE;
        const bool is_ll = (sizeof(void*) == sizeof(long long)) || (flags & FLAGS_LONG);
        if (is_ll)idx = _ntoa<unsigned long long>(buffer, idx, maxlen, (unsigned long long)va_arg(va, unsigned long long), false, 16U, precision, width=sizeof(long long)*2, flags);
          else idx = _ntoa<unsigned long>(buffer, idx, maxlen, (unsigned long)va_arg(va, unsigned long), false, 16U, precision, width=sizeof(long)*2, flags);
        format++;
        break;
      }

      case 'n': {       // Nothing printed. The number of characters written so far is stored in the pointed location.
        int* p = va_arg(va, int*);
        *p = idx; }     
        break;

      default:
        buffer[idx++] = *(format++);       //out(*format, buffer, idx++, maxlen); format++;       
        break;
    }
  }

Exit:
 buffer[idx] = 0;  // out((char)0, buffer, idx < maxlen ? idx : maxlen - 1U, maxlen);
 return (int)idx;
}
//---------------------------------------------------------------------------
int _cdecl PrintFmt(char* buffer, UINT maxlen, char* format, ...)
{
 va_list args;
 va_start(args,format);
 int MSize = FormatToBuffer(format, buffer, maxlen, args);
 va_end(args);
 return MSize; 
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
/*void _stdcall CreateDirectoryPath(LPSTR Path) // Must end with '\\', may contain a filename at the end
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
} */
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
PVOID _stdcall GetRealModuleBase(PVOID AddrInModule)
{
 MEMORY_BASIC_INFORMATION meminfo;
 SIZE_T RetLen     = 0; 
 memset(&meminfo, 0, sizeof(MEMORY_BASIC_INFO));
 if(NtQueryVirtualMemory(NtCurrentProcess,AddrInModule,MemoryBasicInformation,&meminfo,sizeof(MEMORY_BASIC_INFORMATION),&RetLen))return nullptr;   // Use MEMORY_IMAGE_INFORMATION instead?
 return meminfo.AllocationBase;
}
//---------------------------------------------------------------------------
SIZE_T _stdcall GetRealModuleSize(PVOID ModuleBase)
{
 MEMORY_BASIC_INFORMATION meminfo;
 SIZE_T RetLen     = 0; 
 SIZE_T ModuleSize = 0;
 PBYTE  ModuleAddr = (PBYTE)ModuleBase;
 memset(&meminfo, 0, sizeof(MEMORY_BASIC_INFO));
 while(!NtQueryVirtualMemory(NtCurrentProcess,ModuleAddr,MemoryBasicInformation,&meminfo,sizeof(MEMORY_BASIC_INFORMATION),&RetLen))   // Use MEMORY_IMAGE_INFORMATION instead?
  {
   ModuleAddr += meminfo.RegionSize;
   if(meminfo.AllocationBase == ModuleBase)ModuleSize += meminfo.RegionSize;
	 else break;
  }
 return ModuleSize;
}
//---------------------------------------------------------------------------
SIZE_T _stdcall CopyValidModuleMem(PVOID ModuleBase, PVOID DstAddr, SIZE_T DstSize)
{
 MEMORY_BASIC_INFORMATION meminfo;
 SIZE_T RetLen     = 0; 
 SIZE_T Offset     = 0;
 PBYTE  ModuleAddr = (PBYTE)ModuleBase;
 memset(&meminfo, 0, sizeof(MEMORY_BASIC_INFO));
 while(!NtQueryVirtualMemory(NtCurrentProcess,ModuleAddr,MemoryBasicInformation,&meminfo,sizeof(MEMORY_BASIC_INFORMATION),&RetLen))   // Use MEMORY_IMAGE_INFORMATION instead?
  {
   if(meminfo.AllocationBase != ModuleBase)break;
   if((Offset +  meminfo.RegionSize) > DstSize)break; 
   if(meminfo.State == MEM_COMMIT)memcpy((PBYTE)DstAddr + Offset, (PBYTE)ModuleBase + Offset, meminfo.RegionSize);
   ModuleAddr += meminfo.RegionSize;
   Offset     += meminfo.RegionSize;
  }
 return Offset;
}
//---------------------------------------------------------------------------
// Some pages of some system DLLs may not be mapped
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
// Outpud WORD can be directly written to a string with right half-byte ordering  (little endian)
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
/*LPSTR _stdcall ConvertToDecDW(DWORD Value, LPSTR Number)     // TODO: Remove
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
} */
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
/*UINT64 _fastcall DecStrToNumFpu(char* Str)  // A little bit faster // NOTE: Need more tests in case it still can lose some precision somtimes
{
 long double x = 0;
 bool neg = false;
 if (*Str == '-'){neg = true; ++Str;}
 for(BYTE ch;(ch=*Str++ - '0') <= 9;)x = (x*10) + ch;
 if(neg)x = -x;
 return x;
}*/
//---------------------------------------------------------------------------
/*UINT64 _fastcall DecStrToNum(char* Str)       // It is a template now
{
 UINT64 x = 0;
 bool neg = false;
 if (*Str == '-'){neg = true; ++Str;}
 for(BYTE ch;(ch=*Str++ - '0') <= 9;)x = (x*10) + ch;
 if(neg)x = -x;
 return x;
}*/
//---------------------------------------------------------------------------
/*UINT64 _fastcall HexStrToNum(char* Str)   // It is a template now
{
 UINT64 x = 0;
 for(long chv;(chv=CharToHex(*Str++)) >= 0;)x = (x<<4) + chv;  // (<<4) avoids call to __llmul which is big
 return x;
}*/
//---------------------------------------------------------------------------
/*UINT64 _fastcall HexStrToNumFpu(char* Str) // A little bit faster // NOTE: Need more tests in case it still can lose some precision somtimes
{
 long double x = 0;
 for(long chv;(chv=CharToHex(*Str++)) >= 0;)x = (x*16) + chv;
 return x;
} */
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
// NOTE: VMP somehow meke any VirtualProtect fail to read regions of protected DLLs with CurrentProcess handle // Solution: A patcher class with OpenProcess
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
   if(!VirtualQuery(CurProtBase,(MEMORY_BASIC_INFORMATION*)&MemInf,sizeof(MEMORY_BASIC_INFO))){DBGMSG("VirtualQuery fail=%u", GetLastError()); break; }   // Rounded to begin of page
   if(MemInf.RegionSize > BlockSize)ProtSize = BlockSize;   // No Protection loop
	 else ProtSize = MemInf.RegionSize;
   if((MemInf.Protect==PAGE_READWRITE)||(MemInf.Protect==PAGE_EXECUTE_READWRITE)) // WRITECOPY  changed to READWRITE by writeprocessmemory - DO NOT ALLOW THIS !!!
	 {   
	  MoveMemory(CurProtBase,&((BYTE*)Data)[Offset],ProtSize);    // FastMoveMemory
	  Result = ProtSize;
	 }
	  else
	   {
		if(!VirtualProtect(CurProtBase,ProtSize,PAGE_EXECUTE_READWRITE,&PrevProt)){DBGMSG("VirtualProtect 1 fail=%u, %p, %08X", GetLastError(), CurProtBase, ProtSize); break;}   // Allow writing
		MoveMemory(CurProtBase,&((BYTE*)Data)[Offset],ProtSize);     // FastMoveMemory
		Result = ProtSize;
		if(RestoreProt){if(!VirtualProtect(CurProtBase,ProtSize,PrevProt,&PrevProt)){DBGMSG("VirtualProtect 2 fail=%u", GetLastError()); break;} }  // Restore protection
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
int _stdcall WriteVmProtectedMemory(HANDLE* hProcess, PVOID TgtAddr, PVOID Data, SIZE_T Size, bool RstProt)
{
 NTSTATUS stat = 0;
 if(!*hProcess)
  {
   DWORD FVals = PROCESS_VM_OPERATION|PROCESS_QUERY_INFORMATION|PROCESS_VM_READ|PROCESS_VM_WRITE;            
   CLIENT_ID CliID;
   OBJECT_ATTRIBUTES ObjAttr;
   CliID.UniqueThread  = 0;
   CliID.UniqueProcess = (HANDLE)SIZE_T(NtCurrentProcessId());
   ObjAttr.Length = sizeof(ObjAttr);
   ObjAttr.RootDirectory = NULL;  
   ObjAttr.Attributes = 0;           // bInheritHandle ? 2 : 0;
   ObjAttr.ObjectName = NULL;
   ObjAttr.SecurityDescriptor = ObjAttr.SecurityQualityOfService = NULL;
   stat = NtOpenProcess(hProcess, FVals, &ObjAttr, &CliID);
   DBGMSG("NtOpenProcess: %08X", stat);
   if(stat)return -1;
  }       
 ULONG OldProtect;
 PVOID  PAddr = TgtAddr;
 SIZE_T PSize = Size;
 stat = NtProtectVirtualMemory(*hProcess, &PAddr, &PSize, PAGE_EXECUTE_READWRITE, &OldProtect);   //create GUARD page                  
 DBGMSG("ProtStat 1: %08X", stat);
 if(stat)return -2;
 stat = NtWriteVirtualMemory(*hProcess, TgtAddr, Data, Size, &Size); 
 DBGMSG("WrMem: %08X, TAddr=%p", stat,TgtAddr,Size);
 if(RstProt){stat = NtProtectVirtualMemory(*hProcess, &PAddr, &PSize, OldProtect, &OldProtect); DBGMSG("ProtStat 2: %08X", stat);} 
 return Size;          
};
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
   if(*Signature == ':')return true;      // Start of comments
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
PBYTE _stdcall FindMemPatternInRange(PBYTE AddrLo, PBYTE AddrHi, PBYTE Patern, UINT PatSize, UINT Step, UINT MatchIdx)
{                                      
 if(!Step)Step = 1;                // Support backwards?
 if(!MatchIdx)MatchIdx = 1;
 AddrHi -= PatSize;      // Prevent a buffer overflow
 for(;AddrLo <= AddrHi;AddrLo+=Step)
  {
   if(!memcmp(AddrLo, Patern, PatSize) && !--MatchIdx){DBGMSG("Address is %p",AddrLo); return AddrLo;} 
  }
 DBGMSG("Not found!");
 return NULL;
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
UINT __stdcall SaveMemoryToFile(LPSTR FileName, DWORD ProcessID, SIZE_T Address, SIZE_T BlockSize, BYTE ErrorByte)
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
void _stdcall DumpHexData(PVOID Data, UINT Size, UINT RowLen)      // Max row len is 256 bytes
{
 BYTE Buffer[512+4];
 if(RowLen > 256)RowLen = 256;
 PBYTE XData = (PBYTE)Data;
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
 XData = (PBYTE)Data;
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
void _stdcall DumpHexDataFmt(PVOID Data, UINT Size, UINT RowLen)  // Max row len is 128 bytes
{
 PBYTE XData = (PBYTE)Data;
 BYTE Buffer[256+128+4];
 if(RowLen > 128)RowLen = 128;
 for(UINT RSize=0;Size;XData+=RSize,Size-=RSize)
  {
   RSize = (RowLen > Size)?(Size):(RowLen);
   PBYTE DPtr  = (PBYTE)&Buffer;
   for(UINT ctr=0;ctr < RSize;ctr++)    // Create HEX string
    {
     WORD Val  = HexToChar(XData[ctr]);
     *(DPtr++) = Val;
     *(DPtr++) = Val >> 8;
    }
   for(UINT ctr=RSize;ctr < RowLen;ctr++,DPtr+=2)*((PWORD)DPtr) = 0x2020;   // Fill left space // Use memset?
   *(DPtr++) = 0x20;
   *(DPtr++) = 0x20;
   for(UINT ctr=0;ctr < RSize;ctr++)   // Create Text string
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
bool _stdcall FindFileByMask(LPSTR FileMask)
{
 WIN32_FIND_DATA fdat;
 HANDLE hSearch = FindFirstFile(FileMask,&fdat);
 if(hSearch == INVALID_HANDLE_VALUE)return false;
 TrimFilePath(FileMask);
 lstrcatA(FileMask, fdat.cFileName);
 FindClose(hSearch);
 return true;
}
//------------------------------------------------------------------------------------
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
#ifndef NOGDI
 HDC hDCScreen = GetDC(NULL);
 int Refresh   = GetDeviceCaps(hDCScreen, VREFRESH);
 ReleaseDC(NULL, hDCScreen);
 return Refresh;
#else
 return 60;
#endif
}
//---------------------------------------------------------------------------
/*long  _stdcall GetProcessPath(LPSTR ProcNameOrID, LPSTR PathBuffer, long BufferLength)  // NOTE: 'lstrcmpi' will work only if the process created normally(not work if a process created in another session by a hack)
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
	   hModulesSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pent32.th32ProcessID);     // AccessDenied on recent Win10 if a target process is a service
	   if(Module32First(hModulesSnap, &ment32))
		{
         do
          {
           if(lstrcmpi(pent32.szExeFile, ment32.szModule)==0)
            {
             int len = 0;
             for(;ment32.szExePath[len];len++)PathBuffer[len] = ment32.szExePath[len]; 
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
}*/
//---------------------------------------------------------------------------
long _stdcall NormalizeDrivePath(PWSTR PathBuffer, long BufferLength)
{
 wchar_t DrvPath[MAX_PATH];
 PROCESS_DEVICEMAP_INFORMATION_EX MDrives = {0};    // size is 36 for both x32/64
 PWSTR PathSrcPtr = PathBuffer;
 if((PathSrcPtr[0] == '\\') && (PathSrcPtr[7] == '\\') && (PathSrcPtr[1] == 'D') && (PathSrcPtr[6] == 'e'))
  {
   PathSrcPtr  += 8;
   BufferLength -= 8;
  }
 if((PathSrcPtr[0] == 'M') && (PathSrcPtr[1] == 'u') && (PathSrcPtr[2] == 'p') && (PathSrcPtr[3] == '\\'))   // NtQueryInformationProcess:ProcessImageFileName returns this for network drives
  {
   PathSrcPtr  += 4;
   BufferLength -= 4;
  }
 NTSTATUS stat = NtQueryInformationProcess(NtCurrentProcess, ProcessDeviceMap, &MDrives, sizeof(PROCESS_DEVICEMAP_INFORMATION), 0);           //DWORD DrvMsk = GetLogicalDrives();	
 if(stat < 0)
  {
   stat = NtQueryInformationProcess(NtCurrentProcess, ProcessDeviceMap, &MDrives, sizeof(PROCESS_DEVICEMAP_INFORMATION_EX), 0);
   if(stat < 0)return -1;
  }
 DWORD DrvMsk = MDrives.Query.DriveMap;
 for(wchar_t DrvIdx = 0; DrvMsk; DrvMsk >>= 1, DrvIdx++)
  {
   if(!(DrvMsk & 1))continue;
   wchar_t Drive[] = {wchar_t('A' + DrvIdx), ':', 0};             
   UINT res = QueryDosDeviceW(Drive, DrvPath, sizeof(DrvPath)/2); // \Device\LanmanRedirector\;Z:00000000000162a3\Host\sh   // \Device\LanmanRedirector\;Y:000000000002da76\Desktop-xxxxx\Tools    // \Device\ImDisk1
   if(!res)continue;
   BYTE DriveType = MDrives.Query.DriveType[DrvIdx];
   DrvPath[res] = 0;
   while(!DrvPath[res-1])res--;   // res includes some nulls!
   int RCnt = 2;
   PWSTR PPtr = DrvPath;
   if(DriveType == DRIVE_REMOTE)RCnt += 2;
   for(int idx=0;RCnt && *PPtr;PPtr++,res--)
    {
     if(*PPtr == '\\')RCnt--;
    }
   if(!memcmp(PathSrcPtr, PPtr, res*sizeof(wchar_t)))
    {
     PathBuffer[0] = Drive[0];
     PathBuffer[1] = Drive[1];
     long FLen = BufferLength - res;
     memmove(&PathBuffer[2], &PathSrcPtr[res], FLen*sizeof(wchar_t));
     FLen += 2;
     PathBuffer[FLen] = 0;
     return FLen; 
    }
  }
 return 0;
}
//---------------------------------------------------------------------------
long  _stdcall GetProcessPathById(DWORD ProcID, PWSTR PathBuffer, long BufferLength, bool Norm)   // Vista+ ???
{
 SYSTEM_PROCESS_ID_INFORMATION SysInfo;
 SysInfo.ProcessId = PVOID((SIZE_T)ProcID);
 SysInfo.ImageName.Length = 0;
 SysInfo.ImageName.MaximumLength = BufferLength * sizeof(wchar_t);
 SysInfo.ImageName.Buffer = PathBuffer;
 ULONG RetLen = 0;
 NTSTATUS Status = NtQuerySystemInformation(SystemProcessIdInformation, &SysInfo, sizeof(SysInfo), &RetLen);
 if(Status < 0)return Status;
 if(!SysInfo.ImageName.Buffer)return -1;
 long NSize = 0;
 PWSTR PathStr = SysInfo.ImageName.Buffer;
 UINT  PathLen = SysInfo.ImageName.Length; 
 if(Norm)NSize = NormalizeDrivePath(PathStr, PathLen/sizeof(WCHAR));
  else NSize = PathLen / sizeof(WCHAR);
 if(NSize > 0)
  {
   memmove(PathBuffer, PathStr, NSize*sizeof(WCHAR));  
   PathBuffer[NSize] = 0;
  }
 return NSize;
}
//---------------------------------------------------------------------------
// SeDebugPrivilege is required to open processes in another session(i.e. services)
long  _stdcall GetProcessPath(PWSTR ProcNameOrID, PWSTR PathBuffer, long BufferLength, bool Norm)  // NOTE: 'lstrcmpi' will work only if the process created normally(not work if a process created in another session by a hack)
{
 if((ULONG_PTR)ProcNameOrID <= 0xFFFF)
  {
   long res = GetProcessPathById((DWORD)ProcNameOrID, PathBuffer, BufferLength, Norm); 
   if(res >= 0)return res;
  }
 ULONG PInfBlkSize = 0x10000;
 SIZE_T RegionSize  = 0;
 PVOID  PInfBlkAddr = NULL;
 for(;;)
 { 
  RegionSize = PInfBlkSize;
  NTSTATUS Status = NtAllocateVirtualMemory(NtCurrentProcess, &PInfBlkAddr, 0, &RegionSize, MEM_COMMIT, PAGE_READWRITE);
  if(Status < 0)return -1;
  PInfBlkSize = RegionSize;
  Status = NtQuerySystemInformation(SystemProcessInformation, PInfBlkAddr, RegionSize, &PInfBlkSize);
  if(Status != STATUS_INFO_LENGTH_MISMATCH)break; // Full fit
  NtFreeVirtualMemory(NtCurrentProcess, &PInfBlkAddr, &RegionSize, MEM_RELEASE);  
  PInfBlkAddr = NULL;
  PInfBlkSize = (PInfBlkSize + 0x1FFF) & ~0x1FFF;
 }
 if(!PInfBlkSize)return -2;

 SYSTEM_PROCESS_INFORMATION* Pinf = (SYSTEM_PROCESS_INFORMATION*)PInfBlkAddr;
 OBJECT_ATTRIBUTES ObjAttr;
 long NSize = -3;  // Failed to open
 ObjAttr.Length = sizeof(ObjAttr);
 ObjAttr.RootDirectory = NULL;  
 ObjAttr.Attributes = 0;           // bInheritHandle ? 2 : 0;
 ObjAttr.ObjectName = NULL;
 ObjAttr.SecurityDescriptor = ObjAttr.SecurityQualityOfService = NULL;
 for(;;)
  {
   if(((((ULONG_PTR)ProcNameOrID > 0xFFFF) && !lstrcmpiW(ProcNameOrID, Pinf->ImageName.Buffer)) || ((DWORD)Pinf->UniqueProcessId == (DWORD)ProcNameOrID)))  // TODO: WSTR case insensitive compare
    {
     CLIENT_ID CliID;
     HANDLE hProcess = NULL;   
     CliID.UniqueThread  = 0;
     CliID.UniqueProcess = Pinf->UniqueProcessId;

     NtFreeVirtualMemory(NtCurrentProcess, &PInfBlkAddr, &RegionSize, MEM_RELEASE);   // The mem required anymore
     long res = GetProcessPathById((DWORD)CliID.UniqueProcess, PathBuffer, BufferLength, Norm); 
     if(res >= 0)return res;
 
     NTSTATUS stat = NtOpenProcess(&hProcess, PROCESS_QUERY_LIMITED_INFORMATION, &ObjAttr, &CliID);   // May require SE_DEBUG_PRIVILEGE
     if(stat == STATUS_ACCESS_DENIED)stat = NtOpenProcess(&hProcess, PROCESS_QUERY_LIMITED_INFORMATION, &ObjAttr, &CliID);   // Vista+   // Fails on 'Microsoft Windows [Version 10.0.18363.1198]'
     if(!stat)
      {
       ULONG RetLen = 0;
       stat = NtQueryInformationProcess(hProcess,ProcessImageFileName,PathBuffer,BufferLength * sizeof(WCHAR),&RetLen);   // Is it limited to MAX_PATH?
       if(!stat)
        {
         PWSTR PathStr = ((UNICODE_STRING*)PathBuffer)->Buffer;
         UINT  PathLen = ((UNICODE_STRING*)PathBuffer)->Length; 
         if(Norm)NSize = NormalizeDrivePath(PathStr, PathLen/sizeof(WCHAR));
          else NSize = PathLen / sizeof(WCHAR);
         if(NSize > 0)
          {
           memmove(PathBuffer, PathStr, NSize*sizeof(WCHAR));  
           PathBuffer[NSize] = 0;
          }
        }
         else {LOGMSG("Failed to query process: %08X(%u) with code %08X", (DWORD)CliID.UniqueProcess,(DWORD)CliID.UniqueProcess, stat);}
      }
       else {LOGMSG("Failed to open process: %08X(%u) with code %08X", (DWORD)CliID.UniqueProcess,(DWORD)CliID.UniqueProcess, stat);}
     break;
    }
   if(!Pinf->NextEntryOffset)break;
   Pinf = (SYSTEM_PROCESS_INFORMATION*)&((PBYTE)Pinf)[Pinf->NextEntryOffset];
  }
 NtFreeVirtualMemory(NtCurrentProcess, &PInfBlkAddr, &RegionSize, MEM_RELEASE); 
 return NSize;   // Not found!
}
//---------------------------------------------------------------------------
long  _stdcall GetProcessPathNoAdmin(PWSTR ProcNameOrID, PWSTR PathBuffer, long BufferLength)
{
 static const GUID xCLSID_WbemLocator = {0x4590f811, 0x1d3a, 0x11d0, 0x89,0x1f,0x00,0xaa,0x00,0x4b,0x2e,0x24};
 static const GUID xIID_IWbemLocator  = {0xdc12a687, 0x737f, 0x11cf, 0x88,0x4d,0x00,0xaa,0x00,0x4b,0x2e,0x24};

 HRESULT hr = 0;
 IWbemLocator         *WbemLocator  = NULL;
 IWbemServices        *WbemServices = NULL;
 IEnumWbemClassObject *EnumWbem  = NULL;
   
 hr = CoInitializeEx(0, COINIT_MULTITHREADED);
 hr = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
 hr = CoCreateInstance(xCLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, xIID_IWbemLocator, (LPVOID *) &WbemLocator);

 hr = WbemLocator->ConnectServer(L"ROOT\\CIMV2", NULL, NULL, NULL, 0, NULL, NULL, &WbemServices);
 hr = WbemServices->ExecQuery(L"WQL", L"SELECT ExecutablePath, ProcessID FROM Win32_Process WHERE ProcessID = 1160", WBEM_FLAG_FORWARD_ONLY, NULL, &EnumWbem);
  if (EnumWbem != NULL)
   {
            IWbemClassObject *result = NULL;
           ULONG returnedCount = 0;
           while((hr = EnumWbem->Next(WBEM_INFINITE, 1, &result, &returnedCount)) == S_OK) {
               VARIANT ProcessId;
               VARIANT CommandLine;
               VARIANT ExecutablePath;

               // access the properties
               hr = result->Get(L"ProcessId", 0, &ProcessId, 0, 0);
               hr = result->Get(L"CommandLine", 0, &CommandLine, 0, 0);
               hr = result->Get(L"ExecutablePath", 0, &ExecutablePath, 0, 0);

                 result->Release();
              }
   }
       EnumWbem->Release();
       WbemServices->Release();
       WbemLocator->Release();

       CoUninitialize();
       //getchar();

 return 0;
}
//---------------------------------------------------------------------------
/*long _stdcall GetProcessPathByHandle(HANDLE hProcess, LPSTR PathBuffer, long BufferLength)   // Requires ntdef header
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
           return WideCharToMultiByte(CP_ACP,0,(PWSTR)&UImagePath,-1,PathBuffer,BufferLength,NULL,NULL);
 return GetProcessPathByID(GetProcessId(hProcess), PathBuffer, BufferLength);
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
	 if(SystAffMask >> ctr)return SetProcessAffinityMask(hProcess, UINT(((UINT)1) << ctr));
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
UINT _stdcall GetRandomValue(UINT MinVal, UINT MaxVal)   // Bad distribution in sequential usage!!!!!!!
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
     if(!NPEFMT::IsValidPEHeader(ment32.hModule))continue;
     LPSTR MName = NPEFMT::GetExpModuleName(ment32.hModule, false);
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
HANDLE WINAPI CreateFileX(PVOID lpFileName,DWORD dwDesiredAccess,DWORD dwShareMode,LPSECURITY_ATTRIBUTES lpSecurityAttributes,DWORD dwCreationDisposition,DWORD dwFlagsAndAttributes,HANDLE hTemplateFile)
{
 if(!((PBYTE)lpFileName)[1])return CreateFileW((PWSTR)lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
 return CreateFileA((LPSTR)lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
}
//---------------------------------------------------------------------------
int _stdcall SaveMemToFile(PVOID FileName, PVOID Addr, SIZE_T Size) 
{
 DWORD  Result;
 HANDLE hFile;
 if(!((PBYTE)FileName)[1])hFile = CreateFileW((PWSTR)FileName,GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL|FILE_FLAG_SEQUENTIAL_SCAN,NULL);
   else hFile = CreateFileA((LPSTR)FileName,GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL|FILE_FLAG_SEQUENTIAL_SCAN,NULL);
 if(hFile == INVALID_HANDLE_VALUE){LOGMSG("Failed to save file: %s", FileName);return -1;}
 WriteFile(hFile,Addr,Size,&Result,NULL);
 CloseHandle(hFile);
 if((DWORD)Size != Result)return -2;
 return 0;
}
//---------------------------------------------------------------------------
void _stdcall ReverseBytes(PBYTE Array, UINT Size)
{
 PBYTE BkPtr = &Array[Size-1];
 for(;Size > 1;BkPtr--,Array++,Size-=2)
  {
   BYTE Val = *BkPtr;
   *BkPtr = *Array;
   *Array = Val; 
  }
}
//---------------------------------------------------------------------------
int _stdcall BinaryPackToBlobStr(LPSTR ApLibPath, LPSTR SrcBinPath, LPSTR OutBinPath, BYTE Key)
{
 UINT (_stdcall *aP_pack)(void *source,void *destination,UINT length, void *workmem,PVOID callback, void *cbparam);
 UINT (_stdcall *aP_workmem_size)(UINT inputsize);
 UINT (_stdcall *aP_max_packed_size)(UINT inputsize);
 HMODULE hApLib = LoadLibraryA(ApLibPath);
 if(!hApLib){LOGMSG("Failed to load ApLib: %s",ApLibPath); return -1;}
 *(PVOID*)&aP_pack = GetProcAddress(hApLib,"_aP_pack");
 *(PVOID*)&aP_workmem_size = GetProcAddress(hApLib,"_aP_workmem_size");
 *(PVOID*)&aP_max_packed_size = GetProcAddress(hApLib,"_aP_max_packed_size");

 CArr<BYTE> SrcFile;
 CArr<BYTE> DstFile;
 CArr<BYTE> WrkMem;
 CArr<BYTE> Packed;
 SrcFile.FromFile(SrcBinPath);
 if(SrcFile.Length() <= 0){LOGMSG("Failed to load the Binary: %s", SrcBinPath);}
 WrkMem.SetLength(aP_workmem_size(SrcFile.Length()));
 Packed.SetLength(aP_max_packed_size(SrcFile.Length())+sizeof(DWORD));
 UINT MapModSize = NPEFMT::GetPEImageSize(SrcFile.c_data());
 UINT OutLen = aP_pack(SrcFile.c_data(), Packed.c_data(), SrcFile.Length(), WrkMem.c_data(), NULL, NULL);
 if(OutLen == (UINT)-1){LOGMSG("Failed to pack the Binary!"); return -2;}     // APLIB_ERROR
 *(PDWORD)(&Packed.c_data()[OutLen]) = (MapModSize > OutLen)?(MapModSize):(OutLen);
 OutLen += sizeof(DWORD);  // Store original size at end of block
 if(BinDataToCArray(DstFile, Packed.c_data(), OutLen, "Blob", Key) <= 0){LOGMSG("Failed to convert the Binary!"); return -3;}
 DstFile.ToFile(OutBinPath);
 LOGMSG("Saved the Binary blob: %s",OutBinPath);
 Packed.SetLength(OutLen);
 BYTE BPath[MAX_PATH];
 lstrcpyA((LPSTR)&BPath,OutBinPath);
 lstrcpyA(GetFileExt((LPSTR)&BPath),"bin");
 Packed.ToFile((LPSTR)&BPath);
 return 0;
}
//------------------------------------------------------------------------------------------------------------
/* Types:    https://en.wikipedia.org/wiki/X.690
   0x01 - BOOLEAN
   0x02 - INTEGER
   0x03 - BIT STRING         // May be used to store another ASN1 data
   0x04 - OCTET STRING       // May be used to store another ASN1 data
   0x05 - NULL
   0x06 - OBJECT IDENTIFIER
   0x07 -
   0x08 - 
   0x09 - REAL
   0x0A - ENUMERATED
   0x16 - IA5String
   0x30 - SEQUENCE (sub tree)   
*/
// Returns offset to a next Element
long _stdcall NextItemASN1(PBYTE DataPtr, PBYTE* Body, PBYTE Type, UINT* Size)
{
 if(!*DataPtr)return 0;    // No more items
 *Body = DataPtr;
 *Type = *(DataPtr++);
 UINT Len = *(DataPtr++);
 if(Len & 0x80)
  {
   int ctr = Len & 0x7F;
   for(Len = 0;ctr > 0;ctr--)Len = (Len << 8) | *(DataPtr++);
  }
 UINT Res = (DataPtr - *Body) + Len;
 *Size = Len;
 *Body = DataPtr;
 return Res;
}
//------------------------------------------------------------------------------------------------------------
// NOTE: ASN.1 integers are all signed so you may get an unexpected extra first byte 0 (big endian) to represent a positive number
long _stdcall GetTypeFromASN1(PBYTE* DstPtr, PBYTE BufASN1, long LenASN1, UINT ValType, UINT ValIdx)  
{
 while(LenASN1 > 0)
  {
   BYTE  Type = 0;
   UINT  Size = 0;
   PBYTE Body = nullptr;
   long BlkLen = NextItemASN1(BufASN1, &Body, &Type, &Size);
//   LOGMSG("Type: %02X, %u, %08X",Type, Size, BlkLen);
   if((BlkLen <= 0) || (Type == 0x05))return 0;   // 05 is NULL (Always terminates a group or may be placed there just for fun?)
   bool BitStr = (Type == 0x03);
   if(BitStr && (*Body == 0))  // Only id full bytes
    {
     Type = 0x04;
     Body++;
    }
     else BitStr = false;
   if((Type == 0x30)||(Type == 0x31)||(Type == 0x04)||BitStr)     // 04:OCTET STRING  is used to store another ASN1 data
    {
     long len = GetTypeFromASN1(DstPtr, Body, Size, ValType, ValIdx);
     if(len != 0)return len;    // Break if found
    }
     else if((Type == ValType)&&(ValIdx-- == 0))
      {
       *DstPtr = Body;
       return Size;    
      }
   BufASN1 += BlkLen;
   LenASN1 -= BlkLen;
  }
 return 0;   // Not found!
}
//------------------------------------------------------------------------------------------------------------
void _stdcall DumpBufferASN1(PBYTE BufASN1, long LenASN1, int Depth)
{
 while(LenASN1 > 0)
  {
   BYTE  Type = 0;
   UINT  Size = 0;
   PBYTE Body = NULL;
   long BlkLen = NextItemASN1(BufASN1, &Body, &Type, &Size);
   LOGMSG("Depth=%u, Type=%02X, Size=%u, BlkLen=%08X",Depth, Type, Size, BlkLen);
   if((BlkLen <= 0) || (Type == 0x05))return;   // 05 is NULL (Always terminates a group or may be placed there just for fun?)
   bool BitStr = (Type == 0x03);
   if(BitStr && (*Body == 0))  // Only id full bytes
    {
     Type = 0x04;
     Body++;
    }
     else BitStr = false;
   if((Type == 0x30)||(Type == 0x31)||(Type == 0x04))     // 04:OCTET STRING  is used to store another ASN1 data
    {
     DumpBufferASN1(Body, Size, Depth+1);
    }
   BufASN1 += BlkLen;
   LenASN1 -= BlkLen;
  }
}
//------------------------------------------------------------------------------------------------------------
int _stdcall FormatDateForHttp(SYSTEMTIME* st, LPSTR DateStr)
{
 LCID lcidEnUs = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT);
 int olen = GetDateFormatA(lcidEnUs, 0, st, "ddd, dd MMM yyyy", DateStr, MAX_PATH);     // Wed, 27 Sep 2017 14:55:07 GMT
 DateStr[--olen] = 0x20;
 olen += GetTimeFormatA(lcidEnUs, 0, st, "HH:mm:ss", &DateStr[++olen], MAX_PATH); 
 lstrcatA(DateStr, " GMT");
 return olen+4;
}
//------------------------------------------------------------------------------------------------------------
bool _stdcall IsWow64(void)
{
 static PVOID Proc = NULL;
 if(!Proc)Proc = GetProcAddress(GetModuleHandle("Kernel32.dll"),"IsWow64Process");
 if(!Proc)return false;
 BOOL Result = 0;
 return ((BOOL (_stdcall *)(HANDLE,PBOOL))Proc) (GetCurrentProcess(), &Result) && Result;
}
//------------------------------------------------------------------------------------------------------------
int __stdcall SetProcessPrivilegeState(bool bEnable, LPSTR PrName, HANDLE hProcess)
{
 TOKEN_PRIVILEGES tp;
 HANDLE hToken = NULL;	
 bool   bOk = FALSE;
 if(!OpenProcessToken(hProcess, TOKEN_ADJUST_PRIVILEGES, &hToken))return -1; 	// 	|TOKEN_QUERY  ??? 		
 if(!LookupPrivilegeValueA(NULL, PrName, &tp.Privileges[0].Luid)){CloseHandle(hToken); return -2;}
 tp.PrivilegeCount = 1;
 tp.Privileges[0].Attributes = bEnable ? SE_PRIVILEGE_ENABLED : 0;
 if(!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, NULL)){CloseHandle(hToken); return -3;}
 bool res = (GetLastError() == ERROR_SUCCESS);		
 CloseHandle(hToken);	
 if(!res)return 1;    // ???
 return 0;
}
//------------------------------------------------------------------------------------------------------------
/*UINT _stdcall BuildNetDriveMap(CDynArray<SNetDrvMap>* Map)
{
 UINT DCnt = 0;
 wchar_t DrvPath[MAX_PATH];
 DWORD DrvMsk = GetLogicalDrives();	
 for(wchar_t DrvIdx = 0; DrvMsk; DrvMsk >>= 1, DrvIdx++)
  {
   wchar_t Drive[] = {wchar_t('A' + DrvIdx), ':', '\\', 0};             
   UINT uDriveType = GetDriveTypeW(Drive);    		
   if(uDriveType != DRIVE_REMOTE)continue;
   Drive[2] = 0;
   UINT res = QueryDosDeviceW(Drive, DrvPath, sizeof(DrvPath)/2); // \Device\LanmanRedirector\;Z:00000000000162a3\Host\sh
   if(!res)continue;
   DrvPath[res] = 0;
   while(!DrvPath[res-1])res--;   // res includes some nulls
   wchar_t* ssp = wcschr(DrvPath, ':');
   if(!ssp)continue;
   wchar_t* ssa = wcschr(ssp, '\\');
   if(!ssa)continue;
   wchar_t* ssb = wcschr(&ssa[1], '\\');
   if(!ssb)ssb = &DrvPath[res];     // End of a name

   SNetDrvMap rec;
   rec.NameLen = (ssb - ssa) + 1;
   rec.PathLen = (&DrvPath[res] - ssa) + 1;
   rec.Letter = ssp[-1];
   rec.RemPath[0] = '\\';
   wcscpy(&rec.RemPath[1], ssa);
   Map->PushBack(rec);
   DCnt++;
  }
 return DCnt;
} */
//------------------------------------------------------------------------------------------------------------
/*
PVOID _stdcall GetProcessImageBase(HANDLE hProcess)   // Requires ntdef header
{
 static PVOID GetProcInfo = NULL;
 H_PEB         ProcBlock;
 PROCBASICINFO ProcInfo;

 if(!GetProcInfo)GetProcInfo = GetProcAddress(GetModuleHandle("ntdll.dll"),"NtQueryInformationProcess");
 memset(&ProcInfo,0,sizeof(ProcInfo));
 memset(&ProcBlock,0,sizeof(ProcBlock));
 if( GetProcInfo &&
     !((DWORD (_stdcall *)(HANDLE,DWORD,PVOID,DWORD,PDWORD))GetProcInfo) (hProcess, 0, &ProcInfo, sizeof(ProcInfo), NULL) &&
	 ReadProcessMemory(hProcess, ProcInfo.PebBaseAddress,&ProcBlock,sizeof(ProcBlock),NULL))return ProcBlock.ImageBaseAddress;
 return NULL;
}

*/
//------------------------------------------------------------------------------------------------------------
ULONG _stdcall SetProcessUntrusted(HANDLE hProcess)    // Causes __alloca_probe_16
{
 UINT8 TmpSid[MAX_SID_SIZE];
 TOKEN_MANDATORY_LABEL tml = { { (PSID)&TmpSid, SE_GROUP_INTEGRITY } };
 ULONG cb = MAX_SID_SIZE;
 HANDLE hToken;
 if(!CreateWellKnownSid(WinUntrustedLabelSid, 0, tml.Label.Sid, &cb) || !OpenProcessToken(hProcess, TOKEN_ADJUST_DEFAULT, &hToken))return GetLastError();
 ULONG dwError = NOERROR;
 if(!SetTokenInformation(hToken, TokenIntegrityLevel, &tml, sizeof(tml)))dwError = GetLastError();
 CloseHandle(hToken);
 return dwError; 
}
//------------------------------------------------------------------------------------------------------------
NTSTATUS _stdcall CreateUntrustedFolder(PHANDLE phObject, PWSTR ObjectName)
{
 UINT8 UntrustedSid[MAX_SID_SIZE];
 ULONG cb = sizeof(UntrustedSid);
 if(CreateWellKnownSid(WinUntrustedLabelSid, 0, (PSID)&UntrustedSid, &cb))
  {
   UINT8 Sacl[MAX_SID_SIZE + sizeof(ACL) + sizeof(ACE_HEADER) + sizeof(ACCESS_MASK)];
   ULONG cb = sizeof(Sacl);
   InitializeAcl((PACL)&Sacl, cb, ACL_REVISION);
   if(AddMandatoryAce((PACL)&Sacl, ACL_REVISION, 0, 0, (PSID)&UntrustedSid))
    {
     SECURITY_DESCRIPTOR sd;
     UNICODE_STRING ObjectNameUS;
     UINT Length = 11;
     wchar_t Path[512] = {'\\','?','?','\\','G','l','o','b','a','l','\\'};
     for(int idx=0;*ObjectName;ObjectName++,Length++)Path[Length] = *ObjectName;
     //RtlInitUnicodeString(&ObjectNameUS, ObjectName);
     ObjectNameUS.Buffer = Path;
     ObjectNameUS.Length = Length * sizeof(wchar_t);
     ObjectNameUS.MaximumLength = ObjectNameUS.Length + sizeof(wchar_t);

     InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
     SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE);
     SetSecurityDescriptorSacl(&sd, TRUE, (PACL)&Sacl, FALSE);
     IO_STATUS_BLOCK iosb = {};
     OBJECT_ATTRIBUTES oattr = { sizeof(OBJECT_ATTRIBUTES), 0, &ObjectNameUS, OBJ_CASE_INSENSITIVE|OBJ_OPENIF, &sd };
     HANDLE ObjHndl = NULL;
     NTSTATUS Result = NtCreateFile(&ObjHndl, GENERIC_READ|SYNCHRONIZE, &oattr, &iosb, NULL, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_WRITE|FILE_SHARE_READ, FILE_CREATE, FILE_DIRECTORY_FILE|FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0); // NtCreateDirectoryObject(&ObjHndl, DIRECTORY_ALL_ACCESS, &oa);
     if(!Result)
      {
       if(phObject)*phObject = ObjHndl;
         else NtClose(ObjHndl);
      }
     return Result;
    }
 } 
 return STATUS_UNSUCCESSFUL;
}
//------------------------------------------------------------------------------------------------------------
NTSTATUS _stdcall CreateUntrustedNtObjDir(PHANDLE phObject, PWSTR ObjectName)
{
 UINT8 UntrustedSid[MAX_SID_SIZE];
 ULONG cb = sizeof(UntrustedSid);
 if(CreateWellKnownSid(WinUntrustedLabelSid, 0, (PSID)&UntrustedSid, &cb))
  {
   UINT8 Sacl[MAX_SID_SIZE + sizeof(ACL) + sizeof(ACE_HEADER) + sizeof(ACCESS_MASK)];
   ULONG cb = sizeof(Sacl);
   InitializeAcl((PACL)&Sacl, cb, ACL_REVISION);
   if(AddMandatoryAce((PACL)&Sacl, ACL_REVISION, 0, 0, (PSID)&UntrustedSid))
    {
     SECURITY_DESCRIPTOR sd;
     UNICODE_STRING ObjectNameUS;
     UINT Length = 1;   // 4
     wchar_t Path[512] = {'\\'}; // {'\\','?','?','\\'};
     for(int idx=0;*ObjectName;ObjectName++,Length++)Path[Length] = *ObjectName;
     //RtlInitUnicodeString(&ObjectNameUS, ObjectName);
     ObjectNameUS.Buffer = Path;
     ObjectNameUS.Length = Length * sizeof(wchar_t);
     ObjectNameUS.MaximumLength = ObjectNameUS.Length + sizeof(wchar_t);

     InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
     SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE);
     SetSecurityDescriptorSacl(&sd, TRUE, (PACL)&Sacl, FALSE);
     OBJECT_ATTRIBUTES oattr = { sizeof(OBJECT_ATTRIBUTES), 0, &ObjectNameUS, OBJ_CASE_INSENSITIVE|OBJ_OPENIF, &sd };
     HANDLE ObjHndl = NULL;
     NTSTATUS Result = NtCreateDirectoryObject(&ObjHndl, DIRECTORY_ALL_ACCESS, &oattr);
     if(!Result)
      {
       if(phObject)*phObject = ObjHndl;
         else NtClose(ObjHndl);   // NOTE: The directory will be removed after all handles to it are closed
      }
     return Result;
    }
 }  
 return STATUS_UNSUCCESSFUL;
}
//------------------------------------------------------------------------------------------------------------
