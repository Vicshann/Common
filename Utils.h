//---------------------------------------------------------------------------
#pragma once
/*
  Copyright (c) 2019 Victor Sheinmann, Vicshann@gmail.com

  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), 
  to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
  and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. 
*/

#ifndef UtilsH
#define UtilsH

//#define _WIN32
//#undef  _NO_COM

#define WIN32_LEAN_AND_MEAN		 // Exclude rarely-used stuff from Windows headers
//#define _WIN32_WINNT  0x0501     // For CoInitializeEx support

#include <Windows.h>
#include <tlhelp32.h>   // Required for tool help declarations

// Global disable
#pragma warning(disable:4244)    // conversion from '__int64' to 'int', possible loss of data
#pragma warning(disable:4267)    // 'argument': conversion from 'size_t' to 'DWORD', possible loss of data
#pragma warning(disable:4302)    // 'type cast': truncation from 'HANDLE' to 'UINT'
#pragma warning(disable:4311)    // 'type cast': pointer truncation from 'HANDLE' to 'UINT'
#pragma warning(disable:4200)    // nonstandard extension used: zero-sized array in struct/union
#pragma warning(disable:4996)    // 'gcvt': The POSIX name for this item is deprecated
#pragma warning(disable:4146)    // unary minus operator applied to unsigned type, result still unsigned  (DecStrToNum)

#pragma warning(push)
#pragma warning(disable:4244)     // Type cast (WinAPI compatibility)

//#define NOLOG
//---------------------------------------------------------------------------
#define PATHDLMR 0x2F    //  '/'
#define PATHDLML 0x5C    //  '\'

#ifdef NOLOG
#define LOGMSG(msg,...)
#define LOGTXT(txt,len)
#define DBGMSG LOGMSG
#define DBGTXT LOGTXT
#else   
#define LOGMSG(msg,...) LogProc(lfLineBreak|lfLogName|lfLogTime|lfLogThID,_PRNM_,msg,__VA_ARGS__)      // TODO: LogSafe or LogFast
#define LOGTXT(txt,len) LogProc(lfRawTextMsg,(char*)((size_t)len),txt); 
#ifdef _DEBUG
#define DBGMSG LOGMSG
#define DBGTXT LOGTXT
#else
#define DBGMSG(msg,...)
#define DBGTXT(txt,len)
#endif     
#endif  
#define OUTMSG(msg,...) LogProc(lfLineBreak,0,msg,__VA_ARGS__)  

#define FOREGROUND_YELLOW (FOREGROUND_RED|FOREGROUND_GREEN)


enum ELogModes {lmNone=0,lmFile=0x01,lmCons=0x02,lmProc=0x04,lmFileUpd=0x08};
enum ELogFlags {lfNone=0,lfLineBreak=0x01,lfLogName=0x02,lfLogTime=0x04,lfLogThID=0x08,lfLogMsgIdx=0x10,lfRawTextMsg=0x20};
void   _cdecl LogProc(int Flags, char* ProcName, char* Message, ...);
extern void (_cdecl *pLogProc)(LPSTR, UINT);
extern int  LogMode;
extern wchar_t LogFilePath[MAX_PATH];

#ifdef __BORLANDC__
#define _PRNM_ __FUNC__
#else
#define _PRNM_ __FUNCTION__
#endif

//---------------------------------------------------------------------------------
//                     COMPILE TIME STRING ENCRYPTION   [ lstrlen(ENCS("Hello World!")); ]
// Optimization must be turned ON to remove a HUGE portions of code (Macro expansion)
//---------------------------------------------------------------------------------
#define RESTRMAX 32
#define INFOXOR 0x5257424B
#define DOENCKEY(str,idx) (str[idx])
#define EKEY2 DOENCKEY(__TIME__,6)
#define EKEY1 DOENCKEY(__TIME__,7)
#define EKEY  ((EKEY1) ^ (EKEY2))

#define ESB1(str,key,idx) ((BYTE)((idx < sizeof(str))?((((str)[(idx)] ^ (BYTE)(key)) ^ (idx))):(0)))
#define ESB4(str,key,idx) ((DWORD)((ESB1(str,key,(idx*4))<<8)|(ESB1(str,key,((idx*4)+1))<<24)|(ESB1(str,key,((idx*4)+2)))|(ESB1(str,key,((idx*4)+3))<<16)))
#define SINF(str,key)     ((DWORD)((sizeof(str)|((key)<<24)) ^ INFOXOR))   
#define ESB(str,idx)      ((DWORD)ESB4(str,EKEY,idx)) 

// Try to construct this macro dynamically
//#define ENCS(str)  SDecryptRtStr<8>(SINF(str,EKEY), ESB(str,0), ESB(str,1), ESB(str,2), ESB(str,3), ESB(str,4), ESB(str,5), ESB(str,6), ESB(str,7))  // 32  
#define ENCSN(nam,str)  SDecryptRtStr<8> nam (SINF(str,EKEY), ESB(str,0), ESB(str,1), ESB(str,2), ESB(str,3), ESB(str,4), ESB(str,5), ESB(str,6), ESB(str,7))  // 32  
#define ENCS(str) ENCSN( ,str)
//#define ENCS(str) (str)

template<int i> class SDecryptRtStr  
{    // No another vars here - some optimization bug.
 WORD Size;    // Including terminating Zero
 union
  {
   DWORD dval[i];
   char  cval[i*sizeof(DWORD)];
  };
public:
 __declspec(noinline) SDecryptRtStr(DWORD info, ...)
 {
  info ^= INFOXOR;
  DWORD Key  = (info >> 24);
  DWORD bLen = (info & 0xFF);
  DWORD VXor = (Key << 24)|(Key << 16)|(Key << 8)|Key;  
  if(bLen > (i*sizeof(DWORD)))bLen = (i*sizeof(DWORD));
  UINT  wLen = (bLen/sizeof(DWORD))+(bool)(bLen%sizeof(DWORD));  // Always has a Zero DWORD at end
  ULONG_PTR* args = reinterpret_cast<ULONG_PTR*>(&info);
  args++;   
  for(UINT ctr=0;ctr < wLen;ctr++)
   {
    DWORD data = args[ctr];  // reinterpret_cast<PUINT>(args)[ctr];   // On x64 it is 8 bytes
    DWORD inx  = (ctr*4);
    DWORD xval = (((inx+1) << 24)|((inx+3) << 16)|((inx+0) << 8)|(inx+2));
    data = (data ^ VXor) ^ xval;
    data = ((data << 16)&0x00FF0000)|((data >> 8)&0x000000FF)|((data << 8)&0xFF000000)|((data >> 16)&0x0000FF00);
    this->dval[ctr] = data;  
   } 
  cval[bLen-1] = 0;
  this->Size = bLen;
 } 
 //------------------------ 
 ~SDecryptRtStr()
 {
  UINT wLen = (this->Size/sizeof(DWORD))+(bool)(this->Size%sizeof(DWORD));
  for(UINT ctr=0;ctr<wLen;ctr++)this->dval[ctr] = 0;
 }      
 operator const char*() const {return reinterpret_cast<const char*>(&this->cval);}
 operator LPSTR() const {return (LPSTR)&this->cval;}
 operator const int()   const {return this->Size;}
};
//---------------------------------------------------------------------------------




#if !defined NTSTATUS
typedef LONG NTSTATUS;
#endif

#if defined _M_X64
#define MEMORY_BASIC_INFO MEMORY_BASIC_INFORMATION64
#else
#define MEMORY_BASIC_INFO MEMORY_BASIC_INFORMATION32
#endif

#define CALL_VDESTR(Class) __asm { __asm lea  ECX, Class  \
                                   __asm mov  EAX, [ECX]  \
                                   __asm call [EAX]       }
     

#define _L(quote) _L2(quote)
#define _L2(quote) L##quote

#define _S( x ) #x
//====================================================================================
// RandomInRange function is missing?

int _stdcall FormatToBuffer(char* Format, char* DstBuf, UINT BufSize, va_list arglist);
bool _stdcall DeleteFolder(LPSTR FolderPath);
bool _stdcall DeleteFolderW(PWSTR FolderPath);
bool _stdcall IsAddrInModule(PVOID Addr, PVOID ModBase, UINT ModSize);
BOOL _stdcall IsKeyCombinationPressed(DWORD Combination);
bool  _stdcall IsLogHandle(HANDLE Hnd);
void  _stdcall DumpHexData(PVOID Data, UINT Size, UINT RowLen=32);
void  _stdcall DumpHexDataFmt(PVOID Data, UINT Size, UINT RowLen=32);
int   _stdcall RefreshINIValueInt(LPSTR SectionName, LPSTR ValueName, int Default, LPSTR FileName);
int   _stdcall RefreshINIValueStr(LPSTR SectionName, LPSTR ValueName, LPSTR Default, LPSTR RetString, DWORD Size, LPSTR FileName);
int   _stdcall RefreshINIValueStr(LPSTR SectionName, LPSTR ValueName, LPSTR Default, PWSTR RetString, DWORD Size, LPSTR FileName);
void  _stdcall SetINIValueInt(LPSTR SectionName, LPSTR ValueName, int Value, LPSTR FileName);
//int   _stdcall ByteArrayToHexStrSwap(PBYTE Buffer, LPSTR DstStr, UINT HexByteCnt);
int   _stdcall ByteArrayToHexStr(PBYTE Buffer, LPSTR DstStr, UINT ByteCnt, bool UpCase=true);
//int _stdcall HexStrToByteArray(PBYTE Buffer, LPSTR SrcStr, UINT HexByteCnt=0);
//UINT  _stdcall TrimFilePath(LPSTR FullPath);
//void _stdcall CreateDirectoryPath(LPSTR Path);
//void _stdcall CreateDirectoryPathW(PWSTR Path);
SIZE_T _stdcall GetRealModuleSize(PVOID ModuleBase);
SIZE_T _stdcall CopyValidModuleMem(PVOID ModuleBase, PVOID DstAddr, SIZE_T DstSize);
__int64   _stdcall GetTime64(bool Local=false);
bool  _stdcall IsValidAsciiString(PBYTE Ptr, UINT MinLen, UINT MaxLen);
bool  _stdcall IsStringContains(LPSTR String, LPSTR Target, UINT StrLen=0, UINT TgtLen=0);
long  _fastcall CharToHex(BYTE CharValue); 
WORD  _fastcall HexToChar(BYTE Value, bool UpCase=true);
//char* _fastcall DecNumToStrU(UINT64 Val, char* buf, int* Len);
BYTE  _stdcall CharToLowCase(BYTE CharValue);
//UINT64 _fastcall DecStrToNum(char* Str);  //UINT _fastcall DecStrToNum(char* Str);
//UINT64 _fastcall HexStrToNum(char* Str);
DWORD _stdcall DecStrToDW(LPSTR String, UINT* Len=NULL);
DWORD _stdcall HexStrToDW(LPSTR String, UINT Bytes);
DWORD _stdcall WriteLocalProtectedMemory(PVOID Address, PVOID Data, DWORD DataSize, bool RestoreProt);
int   _stdcall SizeOfSignatureData(LPSTR Signature, UINT SigLen=0);
bool  _stdcall IsMemSignatureMatch(PVOID Address, LPSTR Signature, UINT SigLen=0);
PBYTE _stdcall FindMemPatternInRange(PBYTE AddrLo, PBYTE AddrHi, PBYTE Patern, UINT PatSize, UINT Step, UINT MatchIdx);
PBYTE _stdcall FindMemSignatureInRange(PBYTE AddrLo, PBYTE AddrHi, LPSTR Signature, UINT Step=1, UINT MatchIdx=1, UINT SigLen=0);
PBYTE _stdcall FindMemSignatureInRangeSafe(PBYTE AddrLo, PBYTE AddrHi, LPSTR Signature, UINT Step=1, UINT MatchIdx=1, UINT SigLen=0);
//LPSTR _stdcall GetFileName(LPSTR FullPath);
//bool  _stdcall IsPathLink(LPSTR Name);
ULONGLONG __stdcall BinLongUMul(ULONGLONG Multiplicand, ULONGLONG Multiplier);
ULONGLONG __stdcall ShrULL(ULONGLONG Value, BYTE Shift);
ULONGLONG __stdcall ShlULL(ULONGLONG Value, BYTE Shift);
HGLOBAL _stdcall GetResource(HMODULE Module, LPSTR ResName, LPSTR ResType, PUINT ResSize);
UINT __stdcall SaveMemoryToFile(LPSTR FileName, DWORD ProcessID, DWORD Address, DWORD BlockSize, BYTE ErrorByte);
int _stdcall RedirectExports(HMODULE ModFrom, HMODULE ModTo);
int _stdcall ConvertToUTF8(PWSTR Src, LPSTR Dest, UINT DestLen);
long _stdcall NormalizeDrivePath(PWSTR PathBuffer, long BufferLength);
//long  _stdcall GetProcessPath(LPSTR ProcNameOrID, LPSTR PathBuffer, long BufferLngth);
long  _stdcall GetProcessPath(PWSTR ProcNameOrID, PWSTR PathBuffer, long BufferLength, bool Norm=true);
long  _stdcall GetProcessPathById(DWORD ProcID, PWSTR PathBuffer, long BufferLength, bool Norm);
long  _stdcall GetProcessPathNoAdmin(PWSTR ProcNameOrID, PWSTR PathBuffer, long BufferLength);
UINT _stdcall GetRandomValue(UINT MinVal, UINT MaxVal);
HMODULE _stdcall FindModuleByExpName(LPSTR ModuleName);
//bool _stdcall AssignFilePath(LPSTR DstPath, LPSTR BasePath, LPSTR FilePath);     // Template
HANDLE WINAPI CreateFileX(PVOID lpFileName,DWORD dwDesiredAccess,DWORD dwShareMode,LPSECURITY_ATTRIBUTES lpSecurityAttributes,DWORD dwCreationDisposition,DWORD dwFlagsAndAttributes,HANDLE hTemplateFile);
void _stdcall ReverseBytes(PBYTE Array, UINT Size);
int _stdcall BinaryPackToBlobStr(LPSTR ApLibPath, LPSTR SrcBinPath, LPSTR OutBinPath, BYTE Key);
UINT _stdcall NextItemASN1(PBYTE DataPtr, PBYTE* Body, PBYTE Type, UINT* Size);
int _stdcall FormatDateForHttp(SYSTEMTIME* st, LPSTR DateStr);
bool _stdcall IsWow64(void);
BOOL _stdcall ForceProcessSingleCore(HANDLE hProcess);
int __stdcall SetProcessPrivilegeState(bool bEnable, LPSTR PrName, HANDLE hProcess=GetCurrentProcess());
char* ftoa_simple(double num, size_t afterpoint, char *buf, size_t len, size_t* size);
int _stdcall FormatToBuffer(char* format, char* buffer, UINT maxlen, va_list va);
//---------------------------------------------------------------------------
inline int _cdecl PrintFToBuf(char* format, char* buffer, UINT maxlen, ...)
{
 va_list args;
 va_start(args,maxlen);
 int res = FormatToBuffer(format, buffer, maxlen, args);
 va_end(args);
 return res;
}
//---------------------------------------------------------------------------
inline void* operator new(size_t Size, void* Obj)
{
 return Obj;
}
//---------------------------------------------------------------------------
struct SAppFile
{
static void FreeAppMem(PBYTE Addon)
{
 HeapFree(GetProcessHeap(),0,Addon); 
}
//---------------------------------------------------------------------------
static long LoadAppFile(PVOID FileName, PBYTE* AppMem, ULONG* AppLen)
{
 DWORD Result = 0;
 HANDLE hFile;
 if(!((PBYTE)FileName)[1])hFile = CreateFileW((PWSTR)FileName,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL|FILE_FLAG_SEQUENTIAL_SCAN,NULL);
   else hFile = CreateFileA((LPSTR)FileName,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL|FILE_FLAG_SEQUENTIAL_SCAN,NULL);
 if(hFile == INVALID_HANDLE_VALUE){LOGMSG("Failed to open: %s", FileName);return -1;}
 *AppLen = GetFileSize(hFile,NULL);
 *AppMem = (PBYTE)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,(*AppLen+128)); 
 if(*AppLen)ReadFile(hFile,*AppMem,*AppLen,&Result,NULL);
 if(Result != *AppLen){LOGMSG("Failed to read: %s", FileName); FreeAppMem(*AppMem);return -2;}
 CloseHandle(hFile);
 return 0;
}
//---------------------------------------------------------------------------
static long SaveAppFile(PVOID FileName, PBYTE AppMem, ULONG AppLen)
{
 DWORD  Result;
 HANDLE hFile;
 if(!((PBYTE)FileName)[1])hFile = CreateFileW((PWSTR)FileName,GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL|FILE_FLAG_SEQUENTIAL_SCAN,NULL);
   else hFile = CreateFileA((LPSTR)FileName,GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL|FILE_FLAG_SEQUENTIAL_SCAN,NULL);
 if(hFile == INVALID_HANDLE_VALUE){LOGMSG("Failed to save file: %s", FileName);return -1;}
 WriteFile(hFile,AppMem,AppLen,&Result,NULL);
 CloseHandle(hFile);
 if(AppLen != Result)return -2;
 return 0;
}
//---------------------------------------------------------------------------

};

// Thread Environment Block (TEB)
/*#if defined(_M_X64) // x64
PTEB tebPtr = reinterpret_cast<PTEB>(__readgsqword(reinterpret_cast<DWORD_PTR>(&static_cast<NT_TIB*>(nullptr)->Self)));
#else // x86
PTEB tebPtr = reinterpret_cast<PTEB>(__readfsdword(reinterpret_cast<DWORD_PTR>(&static_cast<NT_TIB*>(nullptr)->Self)));
#endif

// Process Environment Block (PEB)
PPEB pebPtr = tebPtr->ProcessEnvironmentBlock;*/
//static inline PEB* NtCurrentPeb (VOID){return NtCurrentTeb()->ProcessEnvironmentBlock;}
//====================================================================================
#define ADDROFFSET(addr,offset) ((addr)+(offset))

#ifndef _REV_DW
#define _REV_DW(Value)    (((DWORD)(Value) << 24)|((DWORD)(Value) >> 24)|(((DWORD)(Value) << 8)&0x00FF0000)|(((DWORD)(Value) >> 8)&0x0000FF00))
#endif
//---------------------------------------------------------------------------
// Compiler always generates Stack Frame Pointers for member functions - used 'pop EBP' to restore it
// How to force the compiler do not make stack frame pointer ?
// How to force the compiler do not push ECX ?
// For __thiscall
#ifndef JUMPVFTMEMBER
#define JUMPVFTMEMBER(Index) __asm pop  ECX           \
                             __asm pop  EBP           \
                             __asm mov  EAX, [ECX]    \
                             __asm add  EAX, Index*4  \
                             __asm jmp  [EAX] 
//---------------------------------------------------------------------------
#endif
#ifndef JUMPEXTMEMBER                                            // for 'thiscall'
#define JUMPEXTMEMBER(Base,Offset) __asm pop  ECX                \
                                   __asm pop  EBP                \
                                   __asm mov  EAX, Base          \
                                   __asm lea  EAX, [EAX+Offset]  \
                                   __asm jmp  EAX 
//---------------------------------------------------------------------------
#endif
#ifndef JUMPEXTMEMBERDC                                           // for 'COM'
#define JUMPEXTMEMBERDC(Address) __asm pop  EBP                \
                                 __asm mov  EAX, Address       \
                                 __asm jmp  EAX 
//---------------------------------------------------------------------------
#endif
#ifndef JUMPEXTMEMBERD                                           // for 'COM'
#define JUMPEXTMEMBERD(Address) __asm pop  ECX                \
                                __asm pop  EBP                \
                                __asm mov  EAX, Address       \
                                __asm jmp  EAX 
//---------------------------------------------------------------------------
#endif

struct SAsm   // Data section must be executable to use this
{
#ifdef _AMD64_
 enum ECodes {cAddSP=0,cSubSP=6};
static PVOID GetCode(UINT Offs)
{
 BYTE Code[] = {
/*0000*/   0x58, 0x48,0x01,0xCC, 0xFF,0xE0,     // pop RAX;  add RSP, RCX; jmp RAX
/*0006*/   0x58, 0x48,0x29,0xCC, 0xFF,0xE0      // pop RAX;  sub RSP, RCX; jmp RAX
};   
 return &Code[Offs];
}
#else
enum ECodes {cAddSP=0,cSubSP=6};
static PVOID GetCode(UINT Offs)
{
 return 0;
}
#endif 
//---------------------------------------------------------------------------
static void AddSP(UINT Val)
{
 ((void (_fastcall *)(UINT))GetCode(cAddSP))(Val);
}
//---------------------------------------------------------------------------
static void SubSP(UINT Val)
{
 ((void (_fastcall *)(UINT))GetCode(cSubSP))(Val);
}
//---------------------------------------------------------------------------

};

/*

0 -> 0
1, 2, 3, 4 -> 4
5, 6, 7, 8 -> 8
9, 10, 11, 12 -> 12

template <size_t alignTo, typename T>
INLINE T HiAlignPtr(T ptr)
{
    return reinterpret_cast<T>((reinterpret_cast<size_t>(ptr) + alignTo - 1) & ~(alignTo - 1));
}

// =================================================================================================

0, 1, 2, 3 -> 0
4, 5, 6, 7 -> 4
8, 9, 10, 11 -> 8

template <size_t alignTo, typename T>
INLINE T LoAlignPtr(T ptr)
{
    return reinterpret_cast<T>(reinterpret_cast<size_t>(ptr) & ~(alignTo - 1));
}

// =================================================================================================


Returns whether pointer is N-byte aligned.

template <size_t N, typename T>
INLINE bool IsAligned(T ptr)
{
    return HiAlignPtr<N>(ptr) == ptr;
}

// =================================================================================================



*/
__declspec(noinline) PVOID _fastcall FixExportRedir(PVOID ThisLibBase, PVOID ExpProcAddr, PWSTR LibPath);
#if defined(_AMD64_)
// NOTE: Arguments number is limited!!!!!
// NOTE: Not going to be tail optimized('jmp rax')!
#define APIWRAPPER(LibPathName,NameAPI) extern "C" _declspec(dllexport) void __cdecl NameAPI(PVOID ParA, PVOID ParB, PVOID ParC, PVOID ParD, PVOID ParE, PVOID ParF, PVOID ParG, PVOID ParH, PVOID ParI, PVOID ParJ, PVOID ParK, PVOID ParL) \
{ \
 static void* Address = NULL; \
 if(!Address)Address = FixExportRedir(ThisLibBase, & NameAPI, LibPathName); \
 ((void (__cdecl *)(...))(Address))(ParA, ParB, ParC, ParD, ParE, ParF, ParG, ParH, ParI, ParJ, ParK, ParL); \
}
#else
// Only 'cdecl' can be exported without name mangling  
#define APIWRAPPER(LibPathName,NameAPI) extern "C" _declspec(dllexport, naked) void __fastcall NameAPI(PVOID ParA, PVOID ParB, ...) \
{ \
 static void* Address = NULL; \
 if(!Address)Address = FixExportRedir(ThisLibBase, & NameAPI, LibPathName); \
 __asm jmp [Address]  \
}
#endif
//====================================================================================

static inline __int64  _fastcall SysTimeToTime64(UINT64 SysTime)   // C++Builder fails 64bit consts!!!
{
 __int64 MAXTIME64  = 0x793406fffi64;        // number of seconds from 00:00:00, 01/01/1970 UTC to 23:59:59. 12/31/3000 UTC
 UINT64  EPOCH_BIAS = 116444736000000000i64; // Number of 100 nanosecond units from 1/1/1601 to 1/1/1970
 __int64 tim = (__int64)((SysTime - EPOCH_BIAS) / 10000000i64);
 if(tim > MAXTIME64)tim = (__int64)(-1);
 return tim;
}
//--------------------------------------------------------------------------- 

template<typename T> constexpr size_t countof(T& a){return (sizeof(T) / sizeof(*a));}         // Not for array classes or pointers!

template<class R, class T> __inline R GetAddress(T Src){return reinterpret_cast<R>(reinterpret_cast<void* &>(Src));}
//template<class T> __inline void ZeroStruct(T &Stru){FastZeroMemory(&Stru,sizeof(Stru));}  
//template<class T> __inline void ZeroPointer(T Stru){FastZeroMemory(Stru,sizeof(*Stru));}
template<class T> __inline T*   AddressOf(T Src){return reinterpret_cast<T*>(reinterpret_cast<void* &>(Src));}
template<class T> __inline int  SetAddressInVFT(PVOID VFT, int Index, T Proc, LPSTR Signature=NULL){PVOID Addr=reinterpret_cast<PVOID>(reinterpret_cast<void* &>(Proc));return MemoryPatchLocal(&((PVOID*)VFT)[Index], &Addr, sizeof(PVOID), Signature);}
template<class T> __inline bool SetPtrAddress(PVOID Src, T &Dst, LPSTR Signature=NULL)
{
 if(IsMemSignatureMatch(Src, Signature)){Dst = reinterpret_cast<T>(Src);return false;}
   else return true;
}

__inline void __cdecl SetAddress(PVOID SrcAddr,...){((PVOID*)(((PVOID*)&SrcAddr)[1]))[0] = SrcAddr;}   // Hack :)  

	
template<typename T> struct SSPPtr   // With this we can have any static members without a separate initialization   // static SSPPtr<decltype(::NtQueryInformationThread)> NtQueryInformationThread;     
{
 inline T*& Val(){static T* v; return v;}    // A static value holder
 inline bool IsNull(void){return (Val() == 0);}     
 inline void operator = (void* p) {Val() = (T*)p;}     
 inline operator T* () {return Val();}
};


inline int _fastcall FastRand(UINT64* Seed=nullptr) 
{ 
 static UINT64 state = 0;
 if(Seed)state = *Seed; 
 UINT64 z = (state += UINT64(0x9E3779B97F4A7C15));
 z = (z ^ (z >> 30)) * UINT64(0xBF58476D1CE4E5B9);
 z = (z ^ (z >> 27)) * UINT64(0x94D049BB133111EB);
 return z ^ (z >> 31);
} 

inline UINT _fastcall RndGen32(UINT dwMin, UINT dwMax)
{
 if(dwMax == dwMin)return dwMin;
 UINT dwResult = FastRand();
 if((dwMax - dwMin) == -1UL)return dwResult;
 dwResult = dwMin + (dwResult % (dwMax - dwMin + 1));
 return dwResult;
}

/*
All anonymous, unnamed namespaces in global scope (that is, unnamed namespaces that are not nested) of
 the same translation unit share the same namespace. This way you can make static declarations without using the 'static' keyword.
Each identifier that is enclosed within an unnamed namespace is unique within the translation unit in which the unnamed namespace is defined.
*/

 __inline bool IsFilePathDelim(WCHAR val){return ((val == PATHDLML)||(val == PATHDLMR));}
// Use a period (.) as a directory component in a path to represent the current directory.
// Use two consecutive periods (..) as a directory component in a path to represent the parent of the current directory.
template<typename T> __inline bool _stdcall IsDirSpecifier(T Name){return (((Name[0] == '.')&&(Name[1] == 0))||((Name[0] == '.')&&(Name[1] == '.')&&(Name[2] == 0)));}
//template<typename T> bool IsPathLink(T* Path){return (((Path[0] == '.')&&(Path[1] == 0))||((Path[0] == '.')&&(Path[1] == '.')&&(Path[2] == 0)));}

//====================================================================================
template <typename T> bool _fastcall IsNumberInRange(T Number, T TresholdA, T TresholdB)
{
 T vMin, vMax; 
 if(TresholdA < TresholdB){vMin=TresholdA;vMax=TresholdB;}  // Is it really necessary?
   else {vMin=TresholdB;vMax=TresholdA;}
 return ((Number >= vMin)&&(Number <= vMax));
}
//---------------------------------------------------------------------------
/*template <typename T> static CJSonItem* EnsureParam(T val, LPSTR Name, CJSonItem* Owner)  // If parameter don`t esist - creates it and sets to 'Val' else just returns pointer to it
{
 CJSonItem* res = Owner->Get(Name);
 if(!res)res = Owner->Add(CJSonItem((T)val,Name));	 //	... and  'const char*' becomes 'bool' if overloaded constr exists only for 'char*' - WHY???!!!
 return res;
}
//---------------------------------------------------------------------------
template <typename T> static CJSonItem* SetParamValue(T val, LPSTR Name, CJSonItem* Owner)
{
 CJSonItem* res = Owner->Get(Name);
 if(!res)res = Owner->Add(CJSonItem((T)val,Name));
 (*res) = (T)val;
 return res;
}*/
//---------------------------------------------------------------------------
template <typename T> static T crc_calc_dir(T base, T crc, char *str, UINT len)
{
 for(;len;str++,len--)
  {
   crc ^= *str;
   for(int ctr=0;ctr<8;ctr++)crc = ((crc & 1)?((crc >> 1) ^ base):(crc >> 1));  
  }
 return crc; 
}
//--------------------------------------------------------------------------- 
template<typename T> void _stdcall HookEntryVFT(UINT Index, PVOID VftPtr, PVOID* OrigAddr, T HookProc)
{
 PVOID  Proc = GetAddress<PVOID>(HookProc);  //  (&HookIDirect3D9::HookCreateDevice);
 if(((PVOID*)VftPtr)[Index] != Proc)
  {
   *OrigAddr = ((PVOID*)VftPtr)[Index]; 
   WriteLocalProtectedMemory(&((PVOID*)VftPtr)[Index], &Proc, sizeof(PVOID), TRUE);
  }
}
//--------------------------------------------------------------------------- 
template<typename T> bool _stdcall IsPathLink(T Name)
{
 if((Name[0] == '.')&&(Name[1] == 0))return true;
 if((Name[0] == '.')&&(Name[1] == '.')&&(Name[2] == 0))return true;
 return false;
}
//---------------------------------------------------------------------------
// 'buf' is for storage only, DO NOT expect result to be at beginning of it
template<typename T, typename S> S DecNumToStrS(T Val, S buf, UINT* Len=0)     
{
 if(Val == 0){if(Len)*Len = 1; *buf = '0'; buf[1] = 0; return buf;}
 bool isNeg = (Val < 0);
 if(isNeg) Val = -Val;       // warning C4146: unary minus operator applied to unsigned type, result still unsigned
 buf  = &buf[20];
 *buf = 0;
 S end = buf;
 for(buf--;Val;buf--)
  {
   *buf  = (Val % 10) + '0';
   Val  /= 10;
  }
 if(isNeg)*buf = '-';
   else buf++;
 if(Len)*Len = end-buf;     // A counted string
  else buf[end-buf] = 0;    // A null terminated string
 return buf;
} 
//---------------------------------------------------------------------------
// No Streams support!
template<typename T, typename O> O _fastcall DecNumToStrU(T Val, O buf, int* Len)     // A/W char string and Signed/Unsigned output by constexpr
{
 if(Val == 0){if(Len)*Len = 1; *buf = '0'; buf[1] = 0; return buf;}
 buf  = &buf[20];
 *buf = 0;
 O end = buf;
 for(buf--;Val;buf--)
  {
   *buf  = (Val % 10) + '0';  // NOTE: Ensure that this is optimized to a single DIV operation with remainder preservation
   Val  /= 10;
  }
 buf++;
 if(Len)*Len = end-buf; 
  else buf[end-buf] = 0;
 return buf;     // Optionally move?
}
//--------------------------------------------------------------------------- 
template<typename O, typename T> O _fastcall DecStrToNum(T Str, long* Size=nullptr)
{
 O x = 0;
 T Old = Str;
 bool neg = false;
 if (*Str == '-'){neg = true; ++Str;}
 for(unsigned char ch;(ch=*Str++ - '0') <= 9;)x = (x*10) + ch;        // Can use a different base?
 if(Size)*Size = (char*)Str - (char*)Old - 1;               // Constexpr?
 if(neg)x = -x;
 return x;
}
//---------------------------------------------------------------------------
template<typename O, typename T> O _fastcall HexStrToNum(T Str, long* Size=nullptr)   // Stops on a first invlid hex char    // Negative values?
{
 O x = 0;
 T Old = Str;
 for(long chv;(chv=CharToHex(*Str++)) >= 0;)x = (x<<4) + chv;  // (<<4) avoids call to __llmul which is big
 if(Size)*Size = (char*)Str - (char*)Old - 1;               // Constexpr?
 return x;
}
//---------------------------------------------------------------------------
template<typename T> static T ReverseBytes(T Val)
{
 T Res;
 for(int ctr=0;ctr < sizeof(T);ctr++)((unsigned char*)&Res)[ctr] = ((unsigned char*)&Val)[sizeof(T)-ctr-1];
 return Res;
}
//------------------------------------------------------------------------------------------------------------
template<typename T> void _fastcall ReverseElements(T* Array, UINT Count)
{
 T* ArrEnd = &Array[Count-1];   // Last Element
 Count = Count / 2;
 for(UINT ctr=0;ctr < Count;ctr++)
  {
   T val = Array[ctr];
   Array[ctr] = ArrEnd[-ctr];
   ArrEnd[-ctr] = val;
  }
}
//---------------------------------------------------------------------------
template<typename T> int _stdcall GetSubStrOffs(T* Str, T* SubStr, int LenSubStr=0, bool CaseSens=false)   // if constexpr (...)   // Zero-Terminated strings only! - Fix this
{
 if(!LenSubStr)LenSubStr = lstrlen(SubStr);
 DWORD Flags = (!CaseSens)?(NORM_IGNORECASE):(0);
 for(int Offs=0;*Str;Offs++,Str++)
  {
   if(CompareString(LOCALE_INVARIANT,Flags,Str,LenSubStr,SubStr,LenSubStr) == CSTR_EQUAL)return Offs;
  }
 return -1;
}
//------------------------------------------------------------------------------------------------------------
template<typename T> UINT TrimFilePath(T Path)
{
 int SLast = -1;
 for(int ctr=0;Path[ctr];ctr++)
  {
   if((Path[ctr] == 0x2F)||(Path[ctr] == 0x5C))SLast = ctr;
  }
 SLast++;
 if(SLast > 0)Path[SLast] = 0;
 return SLast;
}
//---------------------------------------------------------------------------
template<typename T> T GetFileName(T FullPath, UINT Length=-1)    // TODO: Just scan forward, no StrLen and backward scan  // Set constexpr 'IF' in case a T is a str obj an its size is known?
{
 int LastDel = -1; 
 for(int ctr=0,val=FullPath[ctr];val && Length;ctr++,Length--,val=FullPath[ctr]){if(IsFilePathDelim(val))LastDel=ctr;}
 return &FullPath[LastDel+1];
}
//---------------------------------------------------------------------------
template<typename T> T GetFileExt(T FullPath, UINT Length=-1) 
{
 int LastDel = -1;
 int ctr = 0;
 for(BYTE val=FullPath[ctr];val && Length;ctr++,Length--,val=FullPath[ctr]){if(val=='.')LastDel=ctr;}
 if(LastDel < 0)LastDel = ctr-1;
 return &FullPath[LastDel+1];
}
//---------------------------------------------------------------------------
template<typename T> bool IsFileExists(T FilePath)
{
 return !(((sizeof(*FilePath) > 1)?(GetFileAttributesW((PWSTR)FilePath)):(GetFileAttributesA((LPSTR)FilePath))) == INVALID_FILE_ATTRIBUTES);
}
//---------------------------------------------------------------------------
template<typename T> int NormalizeFileName(T FileName, char rep='_')  // Use counter because T may be an object with [] operator
{
 int Total = 0;
 for(int ctr=0;BYTE val=FileName[ctr];ctr++)
  {
   if((val == '|')||(val == '<')||(val == '>')||(val == '"')||(val == '?')||(val == '*')||(val == ':')/*||(val == '\\')||(val == '/')*/){FileName[ctr] = rep; Total++;}
  }
 return Total;
} 
//---------------------------------------------------------------------------
template<typename T> T IncrementFileName(T FileName)
{
 if(!IsFileExists(FileName))return FileName;
 BYTE Ext[32];
 T ExtPtr = GetFileExt(FileName);
 NSTR::StrCopy((T)&Ext, &ExtPtr[-1]);
 for(UINT ctr=0;;ctr++) 
  {
   int Len  = 0;
   T NumPtr = DecNumToStrU(ctr, ExtPtr, &Len);
   NSTR::StrCopy(ExtPtr, NumPtr, Len);  // Mobe inplace
   NSTR::StrCopy(&ExtPtr[Len], (T)&Ext);
   if(!IsFileExists(FileName))break;
  }
 return FileName;
}
//---------------------------------------------------------------------------
/*template<typename T> T GetCmdLineBegin(T CmdLine)
{
 char SFch = '"';
 while(*CmdLine && (*CmdLine <= 0x20))CmdLine++;
 if(*CmdLine == SFch)CmdLine++;
   else SFch = 0x20;
 while(*CmdLine && (*CmdLine != SFch))CmdLine++;
 if(*CmdLine)CmdLine++;
 while(*CmdLine && (*CmdLine <= 0x20))CmdLine++;
 return CmdLine; 
}*/
//---------------------------------------------------------------------------
template<typename T> T GetCmdLineParam(T CmdLine, T Param, unsigned short Scope='\"\"', PUINT ParLen=NULL)  //, unsigned int ParLen)  // Return size of Param string? // May overflow without 'ParLen'
{
 char SFchB = Scope >> 8;
 char SFchE = Scope;
 while(*CmdLine && (*CmdLine <= 0x20))CmdLine++;  // Skip any spaces before
 if(*CmdLine == SFchB)CmdLine++;  // Skip opening quote
   else SFchE = 0x20;             // No quotes, scan until a first space
 T ParBeg = CmdLine;
 if(Param)
  {
   UINT MaxLen = (ParLen)?(*ParLen):(-1);  // -1 is MaxUINT
   while(*CmdLine && (*CmdLine != SFchE) && MaxLen--)*(Param++) = *(CmdLine++); 
   *Param = 0;
  }
  else {while(*CmdLine && (*CmdLine != SFchE))CmdLine++;}
 if(ParLen)*ParLen = CmdLine - ParBeg;  // In Chars
 if(*CmdLine)CmdLine++;  // Skip last Quote or Space
// while(*CmdLine && (*CmdLine <= 0x20))CmdLine++; // Skip any spaces after 
 return CmdLine; 
}
//---------------------------------------------------------------------------
template<typename D, typename S> bool AssignFilePath(D DstPath, S BasePath, S FilePath)  // TODO: Should return length
{
 if(!FilePath || !FilePath[0])return false;
 if(FilePath[1] != ':') 
  {
   NSTR::StrCopy(DstPath, BasePath);    // lstrcpy(DstPath, BasePath);
   S Ptr = (IsFilePathDelim(FilePath[0]))?(&FilePath[1]):(&FilePath[0]);
   NSTR::StrCnat(DstPath, Ptr);  //  lstrcat(DstPath, Ptr);
  }
   else NSTR::StrCopy(DstPath, FilePath);  // lstrcpy(DstPath, FilePath);
 return true;
}
//---------------------------------------------------------------------------
template<typename S> void _stdcall CreateDirectoryPath(S Path) // Must end with '\\', may contain a filename at the end
{
 WCHAR FullPath[MAX_PATH];
 S PathPtr = (S)&FullPath;
 NSTR::StrCopy(PathPtr, Path);
 for(int Count=0;PathPtr[Count] != 0;Count++)
  {
   if((PathPtr[Count]==PATHDLML)||(PathPtr[Count]==PATHDLMR))
	{
	 PathPtr[Count] = 0;
	 if(!((PBYTE)Path)[1])CreateDirectoryW((PWSTR)&FullPath, NULL); // Faster Always create or Test it`s existance first ?
       else CreateDirectoryA((LPSTR)&FullPath, NULL);
	 PathPtr[Count] = PATHDLML;
	}
  }
} 
//---------------------------------------------------------------------------
// File.* supported        // NOTE: GetFileAttributes can succeed on a deleted file
template<typename S> int IsFileExist(S Path)
{
 if(!Path)return -1;
 HANDLE hFile = INVALID_HANDLE_VALUE;
 if constexpr (sizeof(Path[0]) > 1)
  {
   WIN32_FIND_DATAW fdat;
   hFile = FindFirstFileW(Path, &fdat);
  }
   else
    {
     WIN32_FIND_DATAA fdat;
     hFile = FindFirstFileA(Path, &fdat);
    }
 if(hFile == INVALID_HANDLE_VALUE)
  {
   if(GetLastError() == ERROR_FILE_NOT_FOUND)return 0;
   return -2;
  }
 FindClose(hFile);
 return 1;
}
//---------------------------------------------------------------------------
template<typename S> bool INISetValueInt(S SectionName, S ValueName, int Value, S FileName)
{
 if(!ValueName)return false;   // WritePrivateProfileString will remove section if ValueName is NULL
 if constexpr (sizeof(FileName[0]) > 1)
  {
   wchar_t Buffer[128];
   wsprintfW(Buffer,L"%i",Value);
   return WritePrivateProfileStringW(SectionName,ValueName,Buffer,FileName);
  }
   else
    {
     char Buffer[128];
     wsprintfA(Buffer,"%i",Value);
     return WritePrivateProfileStringA(SectionName,ValueName,Buffer,FileName);
    }
}
//---------------------------------------------------------------------------
template<typename S> int INIRefreshValueInt(S SectionName, S ValueName, int Default, S FileName)
{
 int Result = 0;
 if(!ValueName)return 0;   // WritePrivateProfileString will remove section if ValueName is NULL
 if constexpr (sizeof(FileName[0]) > 1)
  {
   wchar_t Buffer[128];
   Result = GetPrivateProfileIntW(SectionName,ValueName,Default,FileName);
   wsprintfW(Buffer,L"%i",Result);
   WritePrivateProfileStringW(SectionName,ValueName,Buffer,FileName);
  }
   else
    {
     char Buffer[128];
     Result = GetPrivateProfileIntA(SectionName,ValueName,Default,FileName);
     wsprintfA(Buffer,"%i",Result);
     WritePrivateProfileStringA(SectionName,ValueName,Buffer,FileName);
    }
 return Result;
}
//---------------------------------------------------------------------------
template<typename S> int INIRefreshValueStr(S SectionName, S ValueName, S Default, S RetString, UINT Size, S FileName)
{
 int Result = 0;
 if(!ValueName)return 0;   // WritePrivateProfileString will remove section if ValueName is NULL
 if constexpr (sizeof(FileName[0]) > 1)
  {
   Result = GetPrivateProfileStringW(SectionName, ValueName, Default, RetString, Size, FileName);
   WritePrivateProfileStringW(SectionName,ValueName,RetString,FileName);  
  }
   else
    {
     Result = GetPrivateProfileStringA(SectionName, ValueName, Default, RetString, Size, FileName);
     WritePrivateProfileStringA(SectionName,ValueName,RetString,FileName);
    }
 return Result;
}
//---------------------------------------------------------------------------



// Return address always points to Number[16-MaxDigits];
//
template<typename T, typename S> S ConvertToHexStr(T Value, int MaxDigits, S NumBuf, bool UpCase, UINT* Len=0) 
{
 const int cmax = sizeof(T)*2;      // Number of byte halves (Digits)
 char  HexNums[] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F','0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};     // Must be optimized to PlatLen assignments
 UINT Case = UpCase?0:16;
 if(Value)
  {
   if(MaxDigits <= 0)              // Auto set max digits
    {
     MaxDigits = 0;
     T tmp = Value;    // Counter needed to limit a signed value        
     for(int ctr=cmax;tmp && ctr;ctr--,MaxDigits++,tmp >>= 4);    // for(T tmp = Value;tmp;tmp>>=4,MaxDigits++);
     if(MaxDigits & 1)MaxDigits++;    // Full bytes
    }         
   S DstPtr = &NumBuf[MaxDigits-1];
   for(int Ctr = 0;DstPtr >= NumBuf;DstPtr--)   // Start from last digit
    {
     if(Ctr < cmax)
      {
       *DstPtr = HexNums[(Value & 0x0000000F)+Case];   // From end of buffer
       Value = Value >> 4;
       Ctr++;
      }
       else *DstPtr = '0';
    }
  }
   else       // Fast 0
    {
     if(MaxDigits <= 0)MaxDigits = 2;
     for(int ctr=0;ctr < MaxDigits;ctr++)NumBuf[ctr] = '0';
    }
 if(Len)*Len = MaxDigits;
   else NumBuf[MaxDigits] = 0;
 return NumBuf; 
}
//---------------------------------------------------------------------------
template<typename T> int HexStrToByteArray(PBYTE Buffer, T SrcStr, UINT HexByteCnt=-1)
{
 UINT ctr = 0;
 for(UINT len = 0;(SrcStr[len]&&SrcStr[len+1])&&(ctr < HexByteCnt);len++)   // If it would be possible to make an unmodified defaults to disappear from compilation...
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
//---------------------------------------------------------------------------

//template<typename T> T _fastcall CharLowSimple(T val) {return (((val >= 'A') && (val <= 'Z')) ? (val + 0x20) : (val));}

/*template<typename T> bool _fastcall IsStrEqSimpleIC(T StrA, T StrB, int MaxLen=-1)
{
 for(int ctr = 0; (MaxLen < 0) || (ctr < MaxLen); ctr++)
  {
   if(CharLowSimple(StrA[ctr]) != CharLowSimple(StrB[ctr]))return false;
   if(!StrA[ctr])break;
  }
 return true;
} */
//---------------------------------------------------------------------------
inline BYTE EncryptByteWithCtr(BYTE Val, BYTE Key, UINT ctr){return ((Val ^ Key)+(BYTE)(ctr * (UINT)Key));}
inline BYTE DecryptByteWithCtr(BYTE Val, BYTE Key, UINT ctr){return ((Val - (BYTE)(ctr * (UINT)Key)) ^ Key);}
inline void EncryptStrSimple(LPSTR SrcStr, LPSTR DstStr, BYTE Key, UINT Size=0)
{
 if(!SrcStr || !Key)return; 
 if(!DstStr)DstStr = SrcStr;
 UINT ctr = 0; 
 if(!Size)Size = -1;
 for(;SrcStr[ctr] && (ctr < Size);ctr++)DstStr[ctr] = EncryptByteWithCtr(SrcStr[ctr], Key, ctr); 
 DstStr[ctr]=EncryptByteWithCtr(SrcStr[ctr], Key, ctr);   // Encrypt Terminating 0
}
//---------------------------------------------------------------------------
inline void DecryptStrSimple(LPSTR SrcStr, LPSTR DstStr, BYTE Key, UINT Size=0)
{
 if(!SrcStr || !Key)return; 
 if(!DstStr)DstStr = SrcStr;
 UINT ctr = 0; 
 if(!Size)Size = -1;
 for(;ctr < Size;ctr++)
  {
   DstStr[ctr] = DecryptByteWithCtr(SrcStr[ctr], Key, ctr); 
   if(!DstStr[ctr])break;    // A terminating 0 decrypted
  }
}
//---------------------------------------------------------------------------
template<typename S> UINT BinDataToCArray(S& OutStrm, PBYTE BinPtr, UINT SizeInBytes, LPSTR Name, BYTE XorKey=0, UINT ESize=sizeof(void*))
{
 const char* TypeArr[] = {"char","short","long","__int64"}; // 1,2,4,8 -> 0,1,2,3
 BYTE Line[512];
 int  idx=0;
 while((idx < 4)&&!(ESize & (1 << idx)))idx++;
 wsprintfA((LPSTR)&Line,"unsigned long BSize%s = %u;\r\nunsigned %s %s[] = {\r\n",Name,SizeInBytes,TypeArr[idx],Name);    // ByteSize required if a element size is more than a byte
 OutStrm += (LPSTR)&Line;
 UINT ElemSize = ESize*sizeof(WORD);  
 while(SizeInBytes > 0)
  {       
   UINT Offs = 0;
   Line[Offs++] = '\t';    // Signle tab for a new line
   while(((Offs+ElemSize) < (sizeof(Line)-3))&&(SizeInBytes > 0))    // 3 = 0x,
    { 
     Line[Offs++] = '0';
     Line[Offs++] = 'x';
     PWORD BPtr = (PWORD)&Line[Offs];
     Offs += ElemSize;
     PWORD EPtr = (PWORD)&Line[Offs];      
     for(UINT ctr=0;ctr < ESize;ctr++)
      {
       EPtr--; 
       if(SizeInBytes){*EPtr = HexToChar(EncryptByteWithCtr(*BinPtr,XorKey,SizeInBytes)); BinPtr++; SizeInBytes--;} 
        else *EPtr = 0x3030;  // '00'
      }
     if(SizeInBytes)Line[Offs++] = ',';
    }
   Line[Offs++] = '\r';
   Line[Offs++] = '\n';
   Line[Offs++] = 0;
   OutStrm += (char*)&Line;
  }
 OutStrm += "};\r\n";
 return OutStrm.Length();
}
//---------------------------------------------------------------------------
template<class T> struct NakedType { typedef T type; };
template<class T> struct NakedType<T*> : NakedType<T> {};
template<class T> struct NakedType<T&> : NakedType<T> {};
template<class T> struct NakedType<T&&> : NakedType<T> {};
template<class T> struct NakedType<T const> : NakedType<T> {};
template<class T> struct NakedType<T volatile> : NakedType<T> {};
template<class T> struct NakedType<T const volatile> : NakedType<T> {};
//template<class T> struct NakedType<T[]> : NakedType<T> {};
//template<class T, int n> struct NakedType<T[n]> : NakedType<T> {};

template<typename S> int RegDeleteKeyRecursive(HKEY hKeyRoot, S lpSubKey)
{
 HKEY hKey;
 LONG lResult;
 UINT SubKeyLen;
 static const int ChrLen = sizeof(*lpSubKey);
 static const bool IsWS  = (ChrLen > 1);
 NakedType<decltype(*lpSubKey)>::type szName[MAX_PATH];    // decltype(remove_reference_t<*lpSubKey>::type)    wchar_t

 if constexpr (IsWS)lResult = RegDeleteKeyW(hKeyRoot, lpSubKey);  // First, see if we can delete the key without having to recurse.
   else lResult = RegDeleteKeyA(hKeyRoot, lpSubKey); 
 if(lResult == ERROR_SUCCESS)return 0;

 if constexpr (IsWS)lResult = RegOpenKeyExW(hKeyRoot, lpSubKey, 0, KEY_READ, &hKey);
   else lResult = RegOpenKeyExA(hKeyRoot, lpSubKey, 0, KEY_READ, &hKey);
 if(lResult != ERROR_SUCCESS) 
  {
   if(lResult == ERROR_FILE_NOT_FOUND)return 0;   // Key not found
    else return -1;    // Error opening the key
  }

 if constexpr (IsWS)SubKeyLen = lstrlenW(lpSubKey);
   else SubKeyLen = lstrlenA(lpSubKey);
 if((lpSubKey[SubKeyLen-1] == '\\')||(lpSubKey[SubKeyLen-1] == '/'))SubKeyLen--;    // In chars

 for(;;)
  {
   DWORD dwSize = MAX_PATH;    // in chars
   if constexpr (IsWS)lResult = RegEnumKeyExW(hKey, 0, szName, &dwSize, NULL, NULL, NULL, NULL); // Enumerate the keys
     else lResult = RegEnumKeyExA(hKey, 0, szName, &dwSize, NULL, NULL, NULL, NULL);
   if(lResult != ERROR_SUCCESS)break;
   memmove(&szName[SubKeyLen+1], &szName[0], (dwSize+1)*ChrLen);
   memcpy(&szName[0], lpSubKey, SubKeyLen*ChrLen);
   szName[SubKeyLen] = '\\';
   if(RegDeleteKeyRecursive(hKeyRoot, szName) < 0)break;  // Can`t continue!
  }

 RegCloseKey(hKey);
 if constexpr (IsWS)lResult = RegDeleteKeyW(hKeyRoot, lpSubKey);   // Try again to delete the key.
   else lResult = RegDeleteKeyA(hKeyRoot, lpSubKey);
 if(lResult == ERROR_SUCCESS)return 0;
 return -2;
}
//------------------------------------------------------------------------------------
// 0: AABBCCDDEEFFGGHH <7,>7 (S-1)  AABBCCDD <3,>3 (S-1)  AABB <1,>1 (S-1)  AA
// 1: HHBBCCDDEEFFGGAA <5,>5 (S-3)  DDBBCCAA <1,>1 (S-3)  BBAA
// 2: HHGGCCDDEEFFBBAA <3,>3 (S-5)  DDCCBBAA
// 3: HHGGFFDDEECCBBAA <1,>1 (S-7)
// 4: HHGGFFEEDDCCBBAA
template<typename T> constexpr __forceinline static T RevByteOrder(T Value)   // Can be used at compile time
{
 if constexpr (sizeof(T) > 1)
  {
   T Result = ((Value & 0xFF) << ((sizeof(T)-1)*8)) | ((Value >> ((sizeof(T)-1)*8)) & 0xFF);  // Exchange edge 1
   if constexpr (sizeof(T) > 2) 
    {
     Result |= ((Value & 0xFF00) << ((sizeof(T)-3)*8)) | ((Value >> ((sizeof(T)-3)*8)) & 0xFF00); // Exchange edge 2
     if constexpr (sizeof(T) > 4)
      {
       Result |= ((Value & 0xFF0000) << ((sizeof(T)-5)*8)) | ((Value >> ((sizeof(T)-5)*8)) & 0xFF0000); // Exchange edge 3
       Result |= ((Value & 0xFF000000) << ((sizeof(T)-7)*8)) | ((Value >> ((sizeof(T)-7)*8)) & 0xFF000000); // Exchange edge 4
      }
    }
   return Result;
  }
 return Value;
}
//------------------------------------------------------------------------------------
// MSVC:
//unsigned short _byteswap_ushort(unsigned short value);
//unsigned long _byteswap_ulong(unsigned long value);
//unsigned __int64 _byteswap_uint64(unsigned __int64 value);
// GCC:
//int32_t __builtin_bswap32 (int32_t x)
//int64_t __builtin_bswap64 (int64_t x)
//
template<typename T> constexpr __forceinline static T SwapBytes(T Value)  // Unsafe with optimizations?  // https://mklimenko.github.io/english/2018/08/22/robust-endian-swap/
{
 union {T val; UINT8 raw[sizeof(T)];} src, dst;
 src.val = Value;
 for(int idx=0,ridx=sizeof(T)-1;idx < sizeof(T)lidx++,ridx--)dst.raw[idx] = src.raw[ridx]; // Lets hope it will be optimized to bswap 
 return dst.val;
// uint8_t* SrcBytes = (uint8_t*)&Value;
// uint8_t  DstBytes[sizeof(T)];
// for(int idx=0;idx < sizeof(T);idx++)DstBytes[idx] = SrcBytes[(sizeof(T)-1)-idx];
// return *(T*)&DstBytes;
}
//------------------------------------------------------------------------------------


//#if defined _M_X64
//#pragma pack(push,8)	// NOTE: Why 'pack(push,8)' is not working here?
//#else
//#pragma pack(push,4)
//#endif

/*
typedef struct _LSA_UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PWSTR  Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

struct FILE_BASIC_INFORMATION 
{
  LARGE_INTEGER CreationTime;
  LARGE_INTEGER LastAccessTime;
  LARGE_INTEGER LastWriteTime;
  LARGE_INTEGER ChangeTime;
  ULONG         FileAttributes;
#if defined _M_X64
  ULONG Bogus;
#endif
};

struct FILE_STANDARD_INFORMATION 
{
  LARGE_INTEGER AllocationSize;
  LARGE_INTEGER EndOfFile;
  ULONG         NumberOfLinks;
  BOOLEAN       DeletePending;
  BOOLEAN       Directory;
};

// ...
struct FILE_ALL_INFORMATION 
{
  FILE_BASIC_INFORMATION     BasicInformation;
  FILE_STANDARD_INFORMATION  StandardInformation;
//  FILE_INTERNAL_INFORMATION  InternalInformation;
//  FILE_EA_INFORMATION        EaInformation;
//  FILE_ACCESS_INFORMATION    AccessInformation;
//  FILE_POSITION_INFORMATION  PositionInformation;
//  FILE_MODE_INFORMATION      ModeInformation;
//  FILE_ALIGNMENT_INFORMATION AlignmentInformation;
//  FILE_NAME_INFORMATION      NameInformation;
};

#define FILE_SUPERSEDE                  0x00000000
#define FILE_OPEN                       0x00000001
#define FILE_CREATE                     0x00000002
#define FILE_OPEN_IF                    0x00000003
#define FILE_OVERWRITE                  0x00000004
#define FILE_OVERWRITE_IF               0x00000005
#define FILE_MAXIMUM_DISPOSITION        0x00000005

typedef struct _OBJECT_ATTRIBUTES {
    ULONG Length;
    HANDLE RootDirectory;
    PUNICODE_STRING ObjectName;
    ULONG Attributes;
    PVOID SecurityDescriptor;        // Points to type SECURITY_DESCRIPTOR
    PVOID SecurityQualityOfService;  // Points to type SECURITY_QUALITY_OF_SERVICE
} OBJECT_ATTRIBUTES;
typedef OBJECT_ATTRIBUTES *POBJECT_ATTRIBUTES;

typedef struct _IO_STATUS_BLOCK {
    union {
        ULONG Status;
        PVOID Pointer;
    };

    ULONG_PTR Information;
} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;

typedef
VOID
(NTAPI *PIO_APC_ROUTINE) (
    IN PVOID ApcContext,
    IN PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG Reserved
    );
     */
//#pragma pack(pop)
//---------------------------------------------------------------------------
inline bool IsValidHandle(HANDLE hndl){return (((ULONG_PTR)hndl + 1) > 1);}   // NULL or INVALID_HANDLE_VALUE
                        
template<typename T> class SAlloc
{
 PVOID  Mem;
 SIZE_T Len;
 HANDLE hHeap;
public:
 SAlloc(UINT Size=0){this->hHeap = GetProcessHeap(); this->Mem = NULL; this->Len = 0; if(Size)this->Allocate(Size);}
 ~SAlloc(){this->Free();} 
 operator   const T()    {return (T)this->Mem;}
 T operator->() const { return (T)this->Mem; }
 T Get(void){return (T)this->Mem;}
 T Allocate(UINT Size){this->Mem = (this->Mem)?(HeapReAlloc(this->hHeap,HEAP_ZERO_MEMORY,this->Mem,Size)):(HeapAlloc(this->hHeap,HEAP_ZERO_MEMORY,Size)); this->Len = Size; return (T)this->Mem;}
 SIZE_T Size(void){return this->Len;}
 void   Free(void){if(this->Mem)HeapFree(GetProcessHeap(),0,this->Mem);}
};
//---------------------------------------------------------------------------
class CHndl
{
 HANDLE hHand;
public:
 CHndl(void){this->hHand = INVALID_HANDLE_VALUE;}
 CHndl(HANDLE hVal){this->hHand = hVal;}
 ~CHndl(){this->Close();}
 void Close(void){if(this->IsValid()){CloseHandle(this->hHand); this->hHand = INVALID_HANDLE_VALUE;}}
 bool IsValid(void){return (((ULONG_PTR)this->hHand + 1) > 1);}  // Not 0 or -1(INVALID_HANDLE_VALUE)
 void Set(HANDLE Hnd){this->hHand = Hnd;}
 HANDLE Get(void){return this->hHand;}
 HANDLE Invalidate(void){HANDLE val = this->hHand; this->hHand = INVALID_HANDLE_VALUE; return val;}
 operator   const HANDLE() {return this->hHand;}
 operator   const bool() {return this->IsValid();}
}; 
//---------------------------------------------------------------------------
template<typename T> class CArr          // bool GrowOnly
{
 T* AData;

public:
 CArr(void){this->AData = NULL;}
 CArr(UINT Cnt){this->AData = NULL; this->Resize(Cnt);}
 ~CArr(){this->Resize(0);}
 operator  T*() {return this->AData;}    // operator   const T*() {return this->AData;}
 T* Data(void){return this->AData;}
 T* c_data(void){return this->AData;}   // For name compatibility in a templates
 UINT Count(void){return (this->Size() / sizeof(T));}
 UINT Size(void){return ((this->AData)?(((size_t*)this->AData)[-1]):(0));}
 UINT Length(void){return this->Size();}
//----------------------------------------------------------
 bool Add(T* Elems, UINT Cnt)
  {
   return this->Append(Elems, Cnt * sizeof(T));
  }
//----------------------------------------------------------
 bool Assign(void* Bytes, UINT Len)     // In Bytes
  {
   if(!this->Resize(Len))return false;
   if(Bytes)memcpy(this->AData, Bytes, Len);
   return true;
  }
//----------------------------------------------------------
 bool Append(void* Bytes, UINT Len)     // In Bytes
  {
   UINT OldSize = this->Size();
   if(!this->Resize(OldSize+Len))return false;
   if(Bytes)memcpy(&((PBYTE)this->AData)[OldSize], Bytes, Len);
   return true;
  }
//----------------------------------------------------------
 CArr<T>& operator += (const char* str){this->Append((void*)str, lstrlenA(str)); return *this;}
//----------------------
 CArr<T>& operator += (const wchar_t* str){this->Append((void*)str, lstrlenW(str)); return *this;}
//----------------------------------------------------------
 bool Resize(UINT Cnt)   // In Elements
 {
  return this->SetLength(Cnt*sizeof(T));
 }
 bool SetLength(UINT Len)    // In bytes!
  {
  HANDLE hHeap = GetProcessHeap();
  size_t* Ptr = (size_t*)this->AData;
  if(Len && Ptr)Ptr = (size_t*)HeapReAlloc(hHeap,HEAP_ZERO_MEMORY,&Ptr[-1],Len+sizeof(size_t));
	else if(!Ptr)Ptr = (size_t*)HeapAlloc(hHeap,HEAP_ZERO_MEMORY,Len+sizeof(size_t));
	  else if(!Len && Ptr){HeapFree(hHeap,0,&Ptr[-1]); this->AData=NULL; return false;}
  if(!Ptr)return false;
  *Ptr = Len;
  this->AData = (T*)(++Ptr);
  return true;

  }
//----------------------------------------------------------
 UINT FromFile(PVOID FileName)
  {
   HANDLE hFile;
   if(!((PBYTE)FileName)[1])hFile = CreateFileW((PWSTR)FileName,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL|FILE_FLAG_SEQUENTIAL_SCAN,NULL);
     else hFile = CreateFileA((LPSTR)FileName,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL|FILE_FLAG_SEQUENTIAL_SCAN,NULL);
   if(hFile == INVALID_HANDLE_VALUE)return 0;
   DWORD Result   = 0;
   DWORD FileSize = GetFileSize(hFile,NULL);
   UINT  LdCnt    = (FileSize / sizeof(T));
   if(FileSize && this->Resize(LdCnt))ReadFile(hFile,this->AData,(LdCnt*sizeof(T)),&Result,NULL);
   CloseHandle(hFile);
   return (Result / sizeof(T));
  }
//----------------------------------------------------------
 UINT ToFile(PVOID FileName)       // TODO: Bool Append    // From - To
  {
   HANDLE hFile;
   UINT SavLen = this->Size();
   if(!SavLen)return 0;   
   if(!((PBYTE)FileName)[1])hFile = CreateFileW((PWSTR)FileName,GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL|FILE_FLAG_SEQUENTIAL_SCAN,NULL);
     else hFile = CreateFileA((LPSTR)FileName,GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL|FILE_FLAG_SEQUENTIAL_SCAN,NULL);
   if(hFile == INVALID_HANDLE_VALUE)return 0;
   DWORD Result = 0;
   WriteFile(hFile,this->AData,SavLen,&Result,NULL);
   CloseHandle(hFile);
   return (Result / sizeof(T));
  }
//----------------------------------------------------------

}; 
//---------------------------------------------------------------------------
template<typename T, int PreAll=0> class CGrowArray    
{
 T* Data;

public:
 CGrowArray(void){this->Data=NULL; if(PreAll)this->Resize(PreAll);}   // Constructor of CDynBuffer will be called
 ~CGrowArray(){this->Resize(0);}      // Destructor of CDynBuffer will be called
 UINT Count(void){return (this->Data)?(((PDWORD)this->Data)[-2]):(0);}
 T* c_data(void){return this->Data;}
 T* Resize(UINT Elems) // Only enlarges the array // Used to avoid excess reallocations of a small blocks
 {
  if(!Elems)
   {
    if(this->Data){HeapFree(GetProcessHeap(),0,&((PDWORD)this->Data)[-2]); this->Data=NULL;} 
    return NULL;
   }
  PDWORD Val = &((PDWORD)this->Data)[-2];
  UINT Size = (Elems*sizeof(T))+(sizeof(DWORD)*3);
  if(!this->Data)Val = (PDWORD)HeapAlloc(GetProcessHeap(),0,Size);   // Make HEAP_ZERO_MEMORY optional?
   else
    {
     if(Elems <= Val[1]){Val[0] = Elems;return this->Data;}  // Only update the Count
     Val = (PDWORD)HeapReAlloc(GetProcessHeap(),0,Val,Size);
    }
  Val[0] = Val[1] = Elems;
  this->Data = (T*)&Val[2];
  return this->Data;
 }
 T* Assign(T* Data, UINT Cnt=1)
 {
  this->Resize(Cnt);
  if(Data)memcpy(this->c_data(), Data, sizeof(T)*Cnt);
   else memset(this->c_data(), 0, sizeof(T)*Cnt);
  return this->c_data();
 }
 T* Append(T* Data, UINT Cnt=1)
 {
  UINT OldCnt = this->Count();
  this->Resize(OldCnt + Cnt);
  if(Data)memcpy(&this->c_data()[OldCnt], Data, sizeof(T)*Cnt);
    else memset(&this->c_data()[OldCnt], 0, sizeof(T)*Cnt);
  return &this->c_data()[OldCnt];
 }
 T&         operator [] (int index){return this->Data[index];}
//-------------
};
//---------------------------------------------------------------------------
template<int ChunkSize=0> class CChunkStream
{
 struct SChunk
  {
   UINT Size;
   UINT Free;
   SChunk* Next;
   BYTE Data[0];
  }*FirstChk;
 UINT Size;

public:
 CChunkStream(void){this->Data = NULL; this->Size = 0;}   // Constructor of CDynBuffer will be called
 ~CChunkStream(){this->Free();}      // Destructor of CDynBuffer will be called
 UINT GetSize(void){return this->Size;}
//-------------
 void* Append(void* Data, UINT Size, UINT Extra=0)
  {
   UINT DatSize = Size+Extra;
   UINT ChkSize = (DatSize > ChunkSize)?(DatSize):(ChunkSize);
   if(!this->FirstChk)
	{
	 this->FirstChk = (SChunk*)HeapAlloc(GetProcessHeap(),0,ChkSize+sizeof(SChunk));
	 this->FirstChk->Size = ChkSize;
	 this->FirstChk->Free = ChkSize;
	 this->FirstChk->Next = NULL;
	}
   SChunk* CurChk = NULL;
   SChunk* LstChk = this->FirstChk;
   for(SChunk* Chk = this->FirstChk;Chk;LstChk=Chk,Chk=Chk->Next)
	{
	 if(Chk->Free >= DatSize){CurChk = Chk; break;}
	}
   if(!CurChk)
	{
	 LstChk->Next = (SChunk*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,ChkSize+sizeof(SChunk));
	 LstChk->Next->Size = ChkSize;
	 LstChk->Next->Free = ChkSize;
	 LstChk->Next->Next = NULL;
	 CurChk = LstChk->Next;
	}
   UINT Offs = CurChk->Size - CurChk->Free;
   CurChk->Free -= DatSize;
   this->Size   += DatSize;
   PVOID Dst = &CurChk->Data[Offs];
   memcpy(&((PBYTE)Dst)[Extra],Data,Size);
   return Dst;
  }
//-------------
 void Free(void)
  {
   SChunk* NxtChk = NULL;
   for(SChunk* Chk = this->FirstChk;Chk;Chk=NxtChk)
	{
	 NxtChk = Chk->Next;
	 HeapFree(GetProcessHeap(),0,Chk);
	}
   this->FirstChk = NULL;
   this->Size = 0;
  }
//-------------
};
//---------------------------------------------------------------------------
template<typename T, int PreallocCnt=64> class CObjStack
{
 CRITICAL_SECTION csec;
 T* Buffer;
 UINT Count;
 UINT Total;
public:
//-------------------------------
CObjStack(void)
{
 InitializeCriticalSection(&this->csec);
 this->Count  = 0;
 this->Total  = PreallocCnt;
 this->Buffer = (T*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,this->Total*sizeof(T)); 
}
//-------------------------------
~CObjStack()
{
 EnterCriticalSection(&this->csec);
 if(this->Buffer)HeapFree(GetProcessHeap(),0,this->Buffer); 
 this->Buffer = nullptr;
 LeaveCriticalSection(&this->csec);

 DeleteCriticalSection(&this->csec);
}
//-------------------------------
bool PopObject(T* Obj)
{
 bool res = false;
 EnterCriticalSection(&this->csec);
 if(this->Count)
  {
   res = true;
   this->Count--;
   memcpy(Obj, &this->Buffer[this->Count], sizeof(T));   
  }
 LeaveCriticalSection(&this->csec);
 return res;
}
//-------------------------------
bool PushObject(T* Obj)
{
 bool res = false;
 EnterCriticalSection(&this->csec);
 if(this->Count < this->Total)
  {
   memcpy(&this->Buffer[this->Count], Obj, sizeof(T));
   this->Count++;
   memset(Obj,0,sizeof(T));   // Prevent any destructors
  }
 LeaveCriticalSection(&this->csec);
 return res;
}
//-------------------------------

}; 
//------------------------------------------------------------------------------------------------------------
class CPerfProbe
{
 ULARGE_INTEGER CntBefore; 
 ULARGE_INTEGER DeltaMin; 
 ULARGE_INTEGER DeltaMax; 

public:
void Begin(void)
{
 this->CntBefore.QuadPart = GetTickCount64(); //__rdtsc();
 this->DeltaMin.QuadPart = -1;
 this->DeltaMax.QuadPart = 0;
}
void Update(void)
{
 ULARGE_INTEGER Delta; 
 ULARGE_INTEGER CntCurr;

 CntCurr.QuadPart = GetTickCount64(); //__rdtsc();
 Delta.QuadPart = CntCurr.QuadPart - this->CntBefore.QuadPart; 
 if(Delta.QuadPart > this->DeltaMax.QuadPart)this->DeltaMax.QuadPart = Delta.QuadPart;
 if(Delta.QuadPart < this->DeltaMin.QuadPart)this->DeltaMin.QuadPart = Delta.QuadPart;
 this->CntBefore.QuadPart = CntCurr.QuadPart;
// LOGMSG("Delta: %u", (UINT)Delta.QuadPart); 
}

void End(char* Name)
{
 this->Update();
 LOGMSG("'%s': DeltaMin(%u), DeltaMax(%u)", Name?Name:"", (UINT)this->DeltaMin.QuadPart, (UINT)this->DeltaMax.QuadPart);	
}

};


//---------------------------------------------------------------------------
/*using namespace std;

class CXmlTableWriter
{
 bool ValsInTags;
 std::ofstream fs;
 xml::writer xw;
 xml::element* tbl;
 xml::element* rec;
 xml::element* srec;

public: 
CXmlTableWriter(LPSTR FilePath, bool UseTags): xw(fs),fs(FilePath,std::ofstream::binary|std::ofstream::trunc)  //,tbl(Name,xw)
{
 ValsInTags = UseTags;
 tbl = rec = srec = NULL;
}
//----------------------------
~CXmlTableWriter()
{
 if(tbl)delete(tbl);
 if(rec)delete(rec);
}
//----------------------------
void BeginTable(UINT Id, LPSTR Name, LPSTR Type, LPSTR ReleaseModule, LPSTR ReleaseSide)
{
 BYTE buff[64];
 tbl = new xml::element(Name,xw);  // Name is "table"
 tbl->attr("id", DecNumToStrU(Id, buff, NULL)).attr("type", Type).attr("release-module", ReleaseModule).attr("release-side", ReleaseSide).attr("version", "0.9");
}
//----------------------------
void EndTable(void)
{
 delete(tbl);
 tbl = NULL;
}
//----------------------------
void BeginRecord(LPSTR Name)
{
 rec = new xml::element(Name,xw); //,(LPSTR)&Tabuls);
}
//----------------------------
void EndRecord(void)
{
 delete(rec);
 rec = NULL;
}
//----------------------------
void BeginSubRecord(LPSTR Name)
{
 srec = new xml::element(Name,xw); //,(LPSTR)&Tabuls);
}
//----------------------------
void EndSubRecord(void)
{
 delete(srec);
 srec = NULL;
}
//----------------------------
void AddValue(LPSTR Name, LPSTR Value)
{
 if(!Value)Value = "";
 if(this->ValsInTags)
  {
   xml::element elem(Name,xw);
   elem.contents(Value);
  }
   else rec->attr(Name, Value);
}
//----------------------------
}; */
//---------------------------------------------------------------------------



#ifndef CompTimeH
#define ctENCSA(Str) (Str)
#define ctENCSW(Str) (Str)
#endif

#ifdef _DEBUG
#define DSIG(sig,cmnt)  sig##": "##cmnt
#else
#define DSIG(sig,cmnt)  sig
#endif

#define AddrBySig(sig,midx,base,size) AddrBySigInRange<__COUNTER__,midx>(sig, base, size) 
template<int Ctr, int MIdx, typename T> constexpr __forceinline PBYTE _stdcall AddrBySigInRange(T& Str, PBYTE Base, UINT Size)    // Passed an encryptes sig str but decrypted it only if the ptr is NULL  // Only as 'T&' it is passed as expected to the decryptor
{
 static PBYTE Addr = NULL; 
 if(!Addr)Addr = FindMemSignatureInRange(Base, &Base[Size], (LPSTR)&ctENCSA(Str)[0], 1, MIdx, 0);    // Include CompileTime.h or define an empty ctENCSA macro 
 return Addr;
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#pragma warning(pop)

#endif
