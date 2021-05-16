
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

//#include <emmintrin.h>   // NOTE: All '#include' must be commented out and used only as a hint to compose BaseHdr.h individually for each project
//#include "FormatPE.h"
//#include "HDE.h"

// TODO: If hooking a function that is imported by this module, update ImportRecord to point to OrigProc
// https://software.intel.com/sites/landingpage/IntrinsicsGuide/#techs=SSE2
//---------------------------------------------------------------------------
#define ADDROFFSET(addr,offset) ((addr)+(offset))

#ifndef _REV_DW
#define _REV_DW(Value)    (((DWORD)(Value) << 24)|((DWORD)(Value) >> 24)|(((DWORD)(Value) << 8)&0x00FF0000)|(((DWORD)(Value) >> 8)&0x0000FF00))
#endif
//------------------------------------------------------------------------------------
#pragma pack(push,1)

struct NUNIHK
{
//====================================================================================
//
//------------------------------------------------------------------------------------
#define VHOOK(proc) NUNIHK::SVftHook<&proc>       // SVftHook<decltype(proc), &proc>         
template<auto HookProc> struct SVftHook      //  'T HProc' will crash the MSVC compiler!
{
 decltype(HookProc) OrigProc;  //T* OrigProc;

 bool IsSet(void){return (bool)this->OrigProc;}
 void SetHook(void** ProcAddr)
  {
   *(void**)&this->OrigProc = *ProcAddr; 
   *ProcAddr = HookProc; 
  }
};

#define VHOOKEX(proc,idx) NUNIHK::SVftHookEx<&proc, idx>  
template<auto HookProc, int VftIdx> struct SVftHookEx      //  'T HProc' will crash the MSVC compiler!
{
 decltype(HookProc) OrigProc;
 void** TargetVFT;

 void SetHook(void* TgtThis)   // Class addr
  {
   if(this->TargetVFT == *(void***)TgtThis)return;  // Already hooked
   this->TargetVFT = *(void***)TgtThis;
   *(void**)&this->OrigProc = this->TargetVFT[VftIdx]; 
   PVOID  BaseAddress = &this->TargetVFT[VftIdx];
   SIZE_T RegionSize  = sizeof(PVOID);
   ULONG  OldProtect  = 0;
   NtProtectVirtualMemory(NtCurrentProcess, &BaseAddress, &RegionSize, PAGE_EXECUTE_READWRITE, &OldProtect);   // VirtualProtect(&this->TargetVFT[VftIdx],sizeof(PVOID),PAGE_EXECUTE_READWRITE,&OldProt);  // TODO: Test if it is writable first
   *(decltype(HookProc)*)&this->TargetVFT[VftIdx] = HookProc;
   BaseAddress = &this->TargetVFT[VftIdx];
   RegionSize  = sizeof(PVOID);
   NtProtectVirtualMemory(NtCurrentProcess, &BaseAddress, &RegionSize, OldProtect, &OldProtect);    // VirtualProtect(&this->TargetVFT[VftIdx],sizeof(PVOID),OldProt,&OldProt);       
  }
//------------------------
 void Remove(void)
  {
   if(!this->TargetVFT)return;
   PVOID  BaseAddress = &this->TargetVFT[VftIdx];
   SIZE_T RegionSize  = sizeof(PVOID);
   ULONG  OldProtect  = 0;
   NtProtectVirtualMemory(NtCurrentProcess, &BaseAddress, &RegionSize, PAGE_EXECUTE_READWRITE, &OldProtect);    // VirtualProtect(&this->TargetVFT[VftIdx],sizeof(PVOID),PAGE_EXECUTE_READWRITE,&OldProtect);
   *(decltype(HookProc)*)&this->TargetVFT[VftIdx] = this->OrigProc;
   BaseAddress = &this->TargetVFT[VftIdx];
   RegionSize  = sizeof(PVOID);
   NtProtectVirtualMemory(NtCurrentProcess, &BaseAddress, &RegionSize, OldProtect, &OldProtect);    // VirtualProtect(&this->TargetVFT[VftIdx],sizeof(PVOID),OldProt,&OldProt);     
  }
};

//====================================================================================
//                                   SProcHook<decltype(&proc), &proc>            <typename T, T HProc=0> struct SProcHook   
//------------------------------------------------------------------------------------
#define PHOOK(proc) NUNIHK::SProcHook<decltype(&proc), &proc>             // Waiting for C++17`s auto as a template argument type   // TODO: Skip int3 or other types of software breakpoints

enum EHookFlg {hfNone,hfFillNop=1,hfFollowJmp=2,hfForceHook=4};       // Restore MemProt flag?


template<typename T, T HProc=0> struct SProcHook        //  'T HProc' will crash the MSVC compiler!    // Declare all members static that the hook struct can be declared temporary on stack (without 'static' cpecifier)?
{
#ifdef _AMD64_
 static const unsigned int TrLen = 12;    // 48 B8 11 22 33 00 FF FF FF 0F   movabs rax,FFFFFFF00332211  // jmp rax
#else
#ifdef NORELHOOK
 static const unsigned int TrLen = 6;	  // 68 XX XX XX XX  C3  // push Addr; retn
#else
 static const unsigned int TrLen = 5;	  // E9 XX XX XX XX   // Jmp REL32
#endif
#endif
 T     OrigProc;   // Execute stolen code and continue
 T     HookAddr;   // Hooked function address
 T     HookProc;   // Hook function
 //PVOID* Import;   // Redirected import of this module (If need to destroy PE header, do it AFTER hooking)
 UINT   HookLen;
 BYTE   OriginCode[16];  // Original code to restore when hook removed    // Size of __m128i 
 BYTE   StolenCode[64];  // A modified stolen code
//------------------------------------------------------------------------------------
static int CountNops(PVOID PAddr)
{
 int Cnt = 0;
 for(PBYTE Addr=(PBYTE)PAddr;(*Addr == 0x90)||(*Addr == 0xCC);Addr++)Cnt++;
 return Cnt;
}
//------------------------------------------------------------------------------------
static bool IsAddrHooked(PVOID PAddr, PVOID Hook)       // Already hooked by this instance!  // It is allowed to hook by a different instances       // TODO: Test it! 
{
 PBYTE Addr = (PBYTE)PAddr;
#ifdef _AMD64_
 if((Addr[0] != 0x48)||(Addr[1] != 0xB8)||(Addr[10] != 0xFF)&&(Addr[11] != 0xE0))return false;  // Very unprobable to be an original code 
 if(Hook && (*(PVOID*)&Addr[2] != Hook))return false;
#else
 if(Addr[0] != 0xE9)return true;   // Not reliable, it may be an original code
 if(Hook && (RelAddrToAddr(Addr,5,*(PDWORD)&Addr[1]) != Hook))return false;     // PDWORD?
#endif
 return true;
}
//------------------------------------------------------------------------------------
bool SetHookIntr(PBYTE ProcAddr=NULL, UINT Flags=EHookFlg::hfFillNop|EHookFlg::hfFollowJmp, void* HookFunc=NULL)   // Can be reused with same ProcAddr after 'Remove'  // Do not refer to 'T' from here or this function may be duplicated
{
 if(!ProcAddr || (this->IsActive() && !(Flags & EHookFlg::hfForceHook))){/*DBGMSG("Failed: %p",ProcAddr);*/ return false;}       // Logging here will make the message duplicated by templating and size will bloat!
// DBGMSG("Hooking: %p",ProcAddr);
#ifdef _AMD64_
 NHDE::HDE64 dhde;
#else
 NHDE::HDE32 dhde;
#endif					   
 UINT  CodeLen   = 0;
 this->HookLen   = 0;
// if(!HookFunc)HookFunc = HProc;       // No more bloating
 if(!ProcAddr)ProcAddr = *(PBYTE*)&this->HookAddr;
 if(*ProcAddr == 0xFC)ProcAddr++;   // CLD  // Just in case
 for(PBYTE DisAddr=ProcAddr;this->HookLen < TrLen;DisAddr += dhde.len)   // MSVC compiler crash if 'this->HookLen += dhde.len' is here
  {
   this->HookLen += dhde.Disasm(DisAddr);
   if((*DisAddr == 0xE9)&&(Flags & EHookFlg::hfFollowJmp))
    {
     dhde.len = 0;
     DisAddr  = (PBYTE)RelAddrToAddr(DisAddr,5,*(DWORD*)&DisAddr[1]);
     ProcAddr = DisAddr;
     this->HookLen = 0;
     CodeLen = 0;
     continue;
    }
   if((*DisAddr == 0xEB)&&(Flags & EHookFlg::hfFollowJmp))
    {
     dhde.len = 0;
     DisAddr  = (PBYTE)RelAddrToAddr(DisAddr,2,*(BYTE*)&DisAddr[1]);
     ProcAddr = DisAddr;
     this->HookLen = 0;
     CodeLen = 0;
     continue;
    }
#ifdef _AMD64_
   if(((*DisAddr & 0xFB)==0x48) && (DisAddr[1]==0x8B) && ((DisAddr[2] & 0x07)==0x05))       // 48=RAX-rdi; 4C=R8-R15  // mov REG, qword ptr [REL]
    {
     BYTE ExRegs = ((*DisAddr >> 2)&1);
     BYTE RegIdx = ((DisAddr[2] >> 3)&7);
     this->StolenCode[CodeLen++] = 0x48 | ExRegs;
     this->StolenCode[CodeLen++] = 0xB8 | RegIdx;     
     *(PVOID*)&this->StolenCode[CodeLen] = RelAddrToAddr(DisAddr,7,*(DWORD*)&DisAddr[3]);    // movabs REG, ADDR
     CodeLen += sizeof(PVOID);
     this->StolenCode[CodeLen++] = 0x48 | (ExRegs * 0x05);         // mov REG, [REG]   // 0x48/0x4D
     this->StolenCode[CodeLen++] = 0x8B;
     this->StolenCode[CodeLen++] = RegIdx * 9;
     if(RegIdx == 4)this->StolenCode[CodeLen++] = 0x24;
       else if(RegIdx == 5){this->StolenCode[(--CodeLen)++] = 0x6D; this->StolenCode[CodeLen++] = 0x00;}
     continue;
    }
 if(dhde.opcode == 0x74) // jz rel8     // TODO: Any flags!!!   // Now it is done for hooking KiUserExceptionDispatcher
  {								
   this->StolenCode[CodeLen++] = 0x75;   // JNZ
   this->StolenCode[CodeLen++] = 0x14;
   PVOID  Addr = RelAddrToAddr(DisAddr,dhde.len,dhde.imm.imm8);      // dhde.disp.disp8 ???
   PDWORD Carr = (PDWORD)&this->StolenCode[CodeLen];
  // LOGMSG("jmp at %p to %p, disp32 = %08X, imm32 = %08X",DisAddr,Addr,dhde.disp.disp32,dhde.imm.imm32);
   Carr[0]  = 0xF82444C7;	// mov [RSP-8], DWORD
   Carr[2]  = 0xFC2444C7;	// mov [RSP-4], DWORD
   Carr[4]  = 0xF82464FF;  //  jmp [RSP-8] 
   Carr[1]  = ((PDWORD)&Addr)[0];
   Carr[3]  = ((PDWORD)&Addr)[1];
   CodeLen += 20;
   continue;
  }
 if(dhde.opcode == 0xE8) // call rel32     
  {								
   PVOID  Addr = RelAddrToAddr(DisAddr,dhde.len,dhde.imm.imm32);   // Full QWORD addr is stored there    // dhde.disp.disp32 ???
   PDWORD Carr = (PDWORD)&this->StolenCode[CodeLen];
  // LOGMSG("Call at %p to %p, disp32 = %08X, imm32 = %08X",DisAddr,Addr,dhde.disp.disp32,dhde.imm.imm32);
   Carr[0]  = 0xF82444C7;	// mov [RSP-8], DWORD
   Carr[2]  = 0xFC2444C7;	// mov [RSP-4], DWORD
   Carr[4]  = 0xF82454FF;	// call [RSP-8]
   Carr[1]  = ((PDWORD)&Addr)[0];
   Carr[3]  = ((PDWORD)&Addr)[1];
   CodeLen += 20;
   continue;
  }
 if((dhde.opcode == 0xFF)&&(dhde.modrm == 0x25))    // jmp qword ptr [REL32] 
  {
   int Extra = CountNops(&DisAddr[dhde.len]);      // In Windows 10(2004) Shell32::SHGetFolderPathW is redirected to windows.storage.dll which is delay-loaded. Following jmp [rel] will set hook on delay-load stub instead of a target function
   if(((int)TrLen - ((int)this->HookLen + Extra)) <= 0)     // Fits as a last only    // <<<<<<<<< !!!!!!!!!! 
    {
     PVOID  Addr = RelAddrToAddr(DisAddr,dhde.len,dhde.disp.disp32);           //   &DisAddr[(int)dhde.len + (int)dhde.disp.disp32];
     PDWORD Carr = (PDWORD)&this->StolenCode[CodeLen];
     Addr = *(PVOID*)Addr;
  // LOGMSG("jmp at %p to %p, disp32 = %08X, imm32 = %08X",DisAddr,Addr,dhde.disp.disp32,dhde.imm.imm32);
     Carr[0]  = 0xF82444C7;	// mov [RSP-8], DWORD
     Carr[2]  = 0xFC2444C7;	// mov [RSP-4], DWORD
     Carr[4]  = 0xF82464FF;   // jmp [RSP-8] 
     Carr[1]  = ((PDWORD)&Addr)[0];
     Carr[3]  = ((PDWORD)&Addr)[1];
     CodeLen += 20;
     continue;
    }
     else      // Follow the jump
      {
       PVOID  Addr = RelAddrToAddr(DisAddr,dhde.len,dhde.disp.disp32);
       ProcAddr = DisAddr = *(PBYTE*)Addr;
       dhde.len = 0;
       this->HookLen = 0;
       CodeLen  = 0;
       continue;
      }
  } 
#else
 if(dhde.opcode == 0xE8) // call rel32     
  {								
   PVOID Addr = RelAddrToAddr(DisAddr,dhde.len,dhde.imm.imm32);  
   this->StolenCode[CodeLen] = 0xE8;
   *(PDWORD)&this->StolenCode[CodeLen+1] = AddrToRelAddr(&this->StolenCode[CodeLen],5,(PBYTE)Addr);
   CodeLen += 5;
   continue;
  }
#endif

   memcpy(&this->StolenCode[CodeLen],DisAddr,dhde.len);   // Copy current instruction as is
   CodeLen += dhde.len;
  }
 BYTE Patch[16];   
 *(PVOID*)&this->HookProc = HookFunc;   // *(PVOID*)&this->HookProc = (PVOID)HookFunc;
 *(PVOID*)&this->HookAddr = (PVOID)ProcAddr;
 *(PVOID*)&this->OrigProc = (PVOID)&StolenCode;
 PVOID  BaseAddress = ProcAddr;
 SIZE_T RegionSize  = TrLen;
 ULONG  OldProtect  = 0;	   
 NtProtectVirtualMemory(NtCurrentProcess, &BaseAddress, &RegionSize, PAGE_EXECUTE_READWRITE, &OldProtect);   // VirtualProtect(ProcAddr,TrLen,PAGE_EXECUTE_READWRITE,&PrevProt);  
 *(__m128i*)&Patch = *(__m128i*)ProcAddr;
 *(__m128i*)&this->OriginCode = *(__m128i*)&Patch;
 if(Flags & EHookFlg::hfFillNop)memset(&Patch,0x90,this->HookLen);      // Optional, good for debugging
#ifdef _AMD64_
 this->StolenCode[CodeLen]   = 0xFF;
 this->StolenCode[CodeLen+1] = 0x25;
 this->StolenCode[CodeLen+2] = this->StolenCode[CodeLen+3] = this->StolenCode[CodeLen+4] = this->StolenCode[CodeLen+5] = 0;
 *((PBYTE*)&this->StolenCode[CodeLen+6]) = ProcAddr + this->HookLen;   // Aligned by HDE to whole command
 Patch[0]  = 0x48;				       // movabs rax,FFFFFFF00332211   // EAX is unused in x64 calling convention  // TODO: Usesome SSE instructions to copy this by one operation
 Patch[1]  = 0xB8;
 *(PVOID*)&Patch[2] = *(PVOID*)&HookFunc;        // NOTE: This was modified to use a single assignment somewhere already!!!!!
 Patch[10] = 0xFF;                     // jmp EAX      // EAX is not preserved!  // Replace with QHalves method if EAX must be preserved  // No relative addresses here for easy overhooking
 Patch[11] = 0xE0;
#else
 this->StolenCode[CodeLen] = 0xE9;	   // JMP Rel32
 *((PDWORD)&this->StolenCode[CodeLen+1]) = AddrToRelAddr(&this->StolenCode[CodeLen],TrLen,&ProcAddr[this->HookLen]);
#ifdef NORELHOOK
 Patch[0] = 0x68;	   // push XXXXXXXX
 *((PDWORD)&Patch[1]) = (DWORD)HookProc;
 Patch[5] = 0xC3;      // ret
#else
 Patch[0]  = 0xE9;
 *((PDWORD)&Patch[1]) = AddrToRelAddr(ProcAddr,TrLen,*(PBYTE*)&HookProc);
#endif
#endif
 _mm_storeu_si128((__m128i*)ProcAddr, *(__m128i*)&Patch);  // SSE2, single operation, should be safe       // TODO: memcpy with aligned SSE2 16 byte copy
 NtProtectVirtualMemory(NtCurrentProcess, &BaseAddress, &RegionSize, OldProtect, &OldProtect);   // VirtualProtect(ProcAddr,TrLen,PrevProt,&PrevProt); 

 BaseAddress = this;
 RegionSize  = sizeof(*this);
 NtProtectVirtualMemory(NtCurrentProcess, &BaseAddress, &RegionSize, PAGE_EXECUTE_READWRITE, &OldProtect);  // VirtualProtect(this,sizeof(*this),PAGE_EXECUTE_READWRITE,&PrevProt);	 // On some platforms a data sections is not executable!
// FlushInstructionCache(GetCurrentProcess(),ProcAddr,sizeof(this->OriginCode));   // Is it really needed here?   (Just returns 1)
 return true;
}  
//------------------------------------------------------------------------------------

public:
//------------------------------------------------------------------------------------
bool IsActive(void){return (bool)this->HookLen;}
//------------------------------------------------------------------------------------
bool SetHook(LPSTR ProcName, LPSTR LibName, UINT Flags=EHookFlg::hfFillNop|EHookFlg::hfFollowJmp, T HookFunc=NULL)
{
 if(this->IsActive() && !(Flags & EHookFlg::hfForceHook))return false;       // Already set
 HMODULE  hLib  = (HMODULE)NNTDLL::GetModuleBaseLdr(LibName);            // GetModuleHandleA(LibName);
 if(!hLib && LibName)hLib  = LoadLibraryA(LibName);             // Only with a ForceLoad flag?
 PBYTE ProcAddr = (PBYTE)NPEFMT::GetProcAddr(hLib, ProcName);    // 'C:\Windows\AppPatch\AcLayers.dll' sometimes intercept GetProcAddress and substitutes its result
 if(!ProcAddr){DBGMSG("Failed: %s:%s",LibName?LibName:"",ProcName); return false;}
// DBGMSG("Module=%p, Proc=%p",hLib,ProcAddr);
 return this->SetHook(ProcAddr,Flags,HookFunc);  
}  
//------------------------------------------------------------------------------------
// If a JMP can`t be reused then it should be followed. Conditions fo this is different for x64 and x32
// Following a jump means to risk reaching end of a function
// First jump is a special case of redirection
// hfFollowJmp flag have no effect!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
bool SetHook(PVOID ProcAddr=NULL, UINT Flags=EHookFlg::hfFillNop|EHookFlg::hfFollowJmp, T HookFunc=NULL)  // Separated to prevent code bloating because of templating
{
 if(!HookFunc)HookFunc = HProc;
 return SetHookIntr((PBYTE)ProcAddr, Flags, HookFunc); 
}
//------------------------------------------------------------------------------------
bool Remove(bool Any=true)     // NOTE: If someone else takes our jump code and after that we exit - CRASH!   // TODO: Rethink hook chaining
{
 if(!this->HookLen)return false;
 if(IsAddrHooked(this->HookAddr, (Any)?(NULL):(this->HookProc)))   // If some other instance is not restored it already // NOTE: Original code will be lost if first instance sees that someone else hooked it here
  {
   PVOID  BaseAddress = this->HookAddr;
   SIZE_T RegionSize  = this->HookLen;
   ULONG  OldProtect  = 0;
   NtProtectVirtualMemory(NtCurrentProcess, &BaseAddress, &RegionSize, PAGE_EXECUTE_READWRITE, &OldProtect);  	// VirtualProtect(this->HookAddr,this->HookLen,PAGE_EXECUTE_READWRITE,&PrevProt);				      
   _mm_storeu_si128((__m128i*)this->HookAddr, *(__m128i*)&this->OriginCode);      // SSE2, single operation            
   NtProtectVirtualMemory(NtCurrentProcess, &BaseAddress, &RegionSize, OldProtect, &OldProtect);   // VirtualProtect(this->HookAddr,this->HookLen,PrevProt,&PrevProt);
//   FlushInstructionCache(GetCurrentProcess(),this->HookAddr,sizeof(this->OriginCode));    // Is it really needed here? (Just returns 1)
  }
 this->HookLen = 0;
 return true;
}
//------------------------------------------------------------------------------------
};     


#pragma pack(pop)
//------------------------------------------------------------------------------------
struct SHookRtlDispatchException
{
typedef bool (_cdecl *THookProc)(volatile PVOID ArgA, volatile PVOID ArgB, volatile PVOID ArgC, volatile PVOID ArgD, volatile PVOID RetVal);
static const int MaxQueue = 32;
#ifdef _AMD64_
static const int JmpLen = 14;      // jmp [Rel32]; ProcAddr; 
#else
static const int JmpLen = 5;       // jmp Rel32
#endif
 PBYTE ProcBegPtr;
 PBYTE ProcEndPtr;
 ULONG EndCodeLen;
 ULONG BegCodeLen;
 BYTE  OrigCodeBeg[16];
 BYTE  OrigCodeEnd[16];
 BYTE  CodeBeg[128];            // Must be executable!
 BYTE  CodeEnd[128];
//------------------------------------------------------------------------------------
/*SHookRtlDispatchException(void)       // Use it on already zeroed memory!!!!! // Avoid global initializers! No .CRT section needed!
{
 memset(this,0,sizeof(SHookRtlDispatchException));
}
//------------------------------------------------------------------------------------
~SHookRtlDispatchException()
{
 this->Remove();
} */
//------------------------------------------------------------------------------------
bool IsActive(void){return this->ProcBegPtr && this->ProcEndPtr;}
//------------------------------------------------------------------------------------
PVOID _stdcall FindRtlDispatchException(void)
{
 static PBYTE PBase = NULL;
 if(!PBase)
  {
   char PrName[] = {~'K',~'i',~'U',~'s',~'e',~'r',~'E',~'x',~'c',~'e',~'p',~'t',~'i',~'o',~'n',~'D',~'i',~'s',~'p',~'a',~'t',~'c',~'h',~'e',~'r',0};
   for(int ctr=0;PrName[ctr];ctr++)PrName[ctr] = ~PrName[ctr];
   PBase = (PBYTE)NPEFMT::GetProcAddr(NPEFMT::GetNtDllBaseFast(), PrName);   // "KiUserExceptionDispatcher"       
   if(!PBase)return NULL;
  }
 PBase += 8;     
 for(UINT ctr=0;;ctr++,PBase++)
  {
   if(ctr >= 56)return NULL;
#ifdef _AMD64_
   if(*(PDWORD)PBase == 0xE8D48B48){PBase+=4;          // mov RDX, RSP; call Rel32
#else
   if(*PBase == 0xE8){PBase++;                         // call Rel32
#endif
     PVOID Addr = RelAddrToAddr(PBase-1,5,*(PDWORD)PBase);
     DBGMSG("Addr: %p",Addr);
     return Addr;
    }
  }  
 return NULL;
}
//------------------------------------------------------------------------------------
/*
0F 8X  // Conditional Long Jump
7X     // Conditional Short Jump

E9     // Long Jump
EB     // Short Jump
*/
bool _stdcall SetHook(THookProc ProcBefore, THookProc ProcAfter)
{
#ifdef _AMD64_
 NHDE::HDE64 dhde;
#else
 NHDE::HDE32 dhde;
#endif
 PBYTE AddrQueue[MaxQueue];
 this->ProcBegPtr = (PBYTE)FindRtlDispatchException();
 if(!this->ProcBegPtr){DBGMSG("Failed to find RtlDispatchException!"); return false;}
 this->ProcEndPtr = this->ProcBegPtr;
 this->EndCodeLen = 0;
 this->BegCodeLen = 0;
 for(PBYTE DisAddr=this->ProcBegPtr;this->BegCodeLen < JmpLen;DisAddr += dhde.len)this->BegCodeLen += dhde.Disasm(DisAddr);   // Calc len of code at beginning
 for(UINT Depth=0;;)  // Trace to exit point(There is only one)    // MSVC compiler crash if 'this->HookLen += dhde.len' is here
  {
//   DBGMSG("Disasm at: %p",ProcEndPtr);
   UINT CmdLen = dhde.Disasm(ProcEndPtr);
   if((ProcEndPtr[0] == 0x0F)&&((ProcEndPtr[1] & 0xF0) == 0x80))  // Long jump
    {
     ProcEndPtr = (PBYTE)RelAddrToAddr(ProcEndPtr,6,*(long*)&ProcEndPtr[2]);
     DBGMSG("Jump 1 to: %p",ProcEndPtr);
     Depth = 0;
     continue;
    }
   else if(ProcEndPtr[0] == 0xE9)  // Long jump
    {
     ProcEndPtr = (PBYTE)RelAddrToAddr(ProcEndPtr,5,*(long*)&ProcEndPtr[1]);
     DBGMSG("Jump 2 to: %p",ProcEndPtr);
     Depth = 0;
     continue;
    }
   else if((ProcEndPtr[0] == 0xEB)||((ProcEndPtr[0] & 0xF0) == 0x70))  // Short jump
    {
     ProcEndPtr = (PBYTE)RelAddrToAddr(ProcEndPtr,2,*(char*)&ProcEndPtr[1]);
     DBGMSG("Jump 3 to: %p",ProcEndPtr);
     Depth = 0;
     continue;
    }
   else if((ProcEndPtr[0] == 0xC3)||(ProcEndPtr[0] == 0xC2))   // retn or ret 8
    {
     DBGMSG("RET at %p",ProcEndPtr);
     EndCodeLen = (ProcEndPtr[0] == 0xC3)?(1):(3);
     for(int DIdx=Depth-1;(DIdx >= 0) && (EndCodeLen < JmpLen);DIdx--)    // Step back to have space for a JUMP
      {
       PBYTE Ptr = AddrQueue[DIdx];
       EndCodeLen += (ProcEndPtr - Ptr);
       ProcEndPtr = Ptr;
      }
     DBGMSG("End Addr: %p",ProcEndPtr);
     break;
    }   
   if(Depth >= (MaxQueue-1))
    {
     memcpy(&AddrQueue[0],&AddrQueue[1],(MaxQueue-1)*sizeof(PVOID));
     AddrQueue[Depth] = ProcEndPtr;
    }
     else AddrQueue[Depth++] = ProcEndPtr;
   ProcEndPtr += CmdLen;  // dhde.len;
  }
 BYTE PatchBeg[16];
 BYTE PatchEnd[16];
#ifdef _AMD64_
 BYTE CTempl[] = {0x41,0x57,0x41,0x56,0x41,0x55,0x41,0x54,0x41,0x53,0x41,0x52,0x57,0x56,0x55,0x53,0x50,0x41,0x51,0x41,0x50,0x52,0x51,0x48,0xB8,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xD0,0x85,0xC0,0x59,0x5A,0x41,0x58,0x41,0x59,0x58,0x5B,0x5D,0x5E,0x5F,0x41,0x5A,0x41,0x5B,0x41,0x5C,0x41,0x5D,0x41,0x5E,0x41,0x5F,0x75,0x01,0xC3}; 
#else
 BYTE CTempl[] = {0x57,0x56,0x55,0x53,0x50,0x52,0x51,0xFF,0x74,0x24,0x24,0xFF,0x74,0x24,0x24,0xE8,0x00,0x00,0x00,0x00,0x85,0xC0,0x58,0x58,0x59,0x5A,0x58,0x5B,0x5D,0x5E,0x5F,0x75,0x03,0xC2,0x08,0x00};
#endif
 memcpy(&PatchBeg, ProcBegPtr, sizeof(PatchBeg));
 memcpy(&PatchEnd, ProcEndPtr, sizeof(PatchEnd));
 memcpy(&OrigCodeBeg, ProcBegPtr, sizeof(OrigCodeBeg));
 memcpy(&OrigCodeEnd, ProcEndPtr, sizeof(OrigCodeEnd));
 memcpy(&CodeBeg,&CTempl,sizeof(CTempl));
 memcpy(&CodeEnd,&CTempl,sizeof(CTempl));
 memcpy(&CodeBeg[sizeof(CTempl)],this->ProcBegPtr,this->BegCodeLen);
 memcpy(&CodeEnd[sizeof(CTempl)],this->ProcEndPtr,this->EndCodeLen);  // Final
 PBYTE BPtr = &CodeBeg[sizeof(CTempl)+this->BegCodeLen];
#ifdef _AMD64_
 *(PVOID*)&CodeBeg[25] = ProcBefore;
 *(PVOID*)&CodeEnd[25] = ProcAfter;
 *(PWORD)&BPtr[0]      = 0x25FF;     // Jmp [Rel32]
 *(PDWORD)&BPtr[2]     = 0;
 *(PVOID*)&BPtr[6]     = &this->ProcBegPtr[this->BegCodeLen];  // Retr from a stolen code
 *(PWORD)&PatchBeg[0]  = 0x25FF;     // Uses CodeBeg address
 *(PDWORD)&PatchBeg[2] = 0;
 *(PVOID*)&PatchBeg[6] = &CodeBeg;
 *(PWORD)&PatchEnd[0]  = 0x25FF;     // Uses CodeBeg address
 *(PDWORD)&PatchEnd[2] = 0;
 *(PVOID*)&PatchEnd[6] = &CodeEnd;
#else
 *(long*)&CodeBeg[16] = AddrToRelAddr<PBYTE>(&CodeBeg[15],5,(PBYTE)ProcBefore);
 *(long*)&CodeEnd[16] = AddrToRelAddr<PBYTE>(&CodeEnd[15],5,(PBYTE)ProcAfter);
 BPtr[0] = 0xE9;
 *(long*)&BPtr[1]     = AddrToRelAddr<PBYTE>(&BPtr[0],5,&this->ProcBegPtr[this->BegCodeLen]);  // Retr from a stolen code 
 PatchBeg[0]  = 0xE9;     // Uses CodeBeg address
 *(long*)&PatchBeg[1] = AddrToRelAddr<PBYTE>(&this->ProcBegPtr[0],5,&CodeBeg[0]);
 PatchEnd[0]  = 0xE9;     // Uses CodeBeg address
 *(long*)&PatchEnd[1] = AddrToRelAddr<PBYTE>(&this->ProcEndPtr[0],5,&CodeEnd[0]);
#endif
 PVOID  BaseAddress = this->ProcBegPtr;
 SIZE_T RegionSize  = sizeof(PatchBeg);
 ULONG  OldProtect  = 0;
 NtProtectVirtualMemory(NtCurrentProcess, &BaseAddress, &RegionSize, PAGE_EXECUTE_READWRITE, &OldProtect);  // VirtualProtect(this->ProcBegPtr,sizeof(PatchBeg),PAGE_EXECUTE_READWRITE,&PrevProt);
 _mm_storeu_si128((__m128i*)this->ProcBegPtr, *(__m128i*)&PatchBeg);  // SSE2, single operation, should be safe
 NtProtectVirtualMemory(NtCurrentProcess, &BaseAddress, &RegionSize, OldProtect, &OldProtect);   // VirtualProtect(this->ProcBegPtr,sizeof(PatchBeg),PrevProt,&PrevProt);

 BaseAddress = this->ProcEndPtr;
 RegionSize  = sizeof(PatchEnd);
 NtProtectVirtualMemory(NtCurrentProcess, &BaseAddress, &RegionSize, PAGE_EXECUTE_READWRITE, &OldProtect);  // VirtualProtect(this->ProcEndPtr,sizeof(PatchEnd),PAGE_EXECUTE_READWRITE,&PrevProt);
 _mm_storeu_si128((__m128i*)this->ProcEndPtr, *(__m128i*)&PatchEnd);  // SSE2, single operation, should be safe
 NtProtectVirtualMemory(NtCurrentProcess, &BaseAddress, &RegionSize, OldProtect, &OldProtect);   // VirtualProtect(this->ProcEndPtr,sizeof(PatchEnd),PrevProt,&PrevProt);
 return true;
}
//------------------------------------------------------------------------------------
bool Remove(void)
{
 if(!this->IsActive())return false;
 PVOID  BaseAddress = this->ProcBegPtr;
 SIZE_T RegionSize  = sizeof(OrigCodeBeg);
 ULONG  OldProtect  = 0;
 NtProtectVirtualMemory(NtCurrentProcess, &BaseAddress, &RegionSize, PAGE_EXECUTE_READWRITE, &OldProtect);  // VirtualProtect(this->ProcBegPtr,sizeof(OrigCodeBeg),PAGE_EXECUTE_READWRITE,&PrevProt);
 _mm_storeu_si128((__m128i*)this->ProcBegPtr, *(__m128i*)&OrigCodeBeg);  // SSE2, single operation, should be safe
 NtProtectVirtualMemory(NtCurrentProcess, &BaseAddress, &RegionSize, OldProtect, &OldProtect);   // VirtualProtect(this->ProcBegPtr,sizeof(OrigCodeBeg),PrevProt,&PrevProt);

 BaseAddress = this->ProcEndPtr;
 RegionSize  = sizeof(OrigCodeEnd);
 NtProtectVirtualMemory(NtCurrentProcess, &BaseAddress, &RegionSize, PAGE_EXECUTE_READWRITE, &OldProtect);  // VirtualProtect(this->ProcEndPtr,sizeof(OrigCodeEnd),PAGE_EXECUTE_READWRITE,&PrevProt);
 _mm_storeu_si128((__m128i*)this->ProcEndPtr, *(__m128i*)&OrigCodeEnd);  // SSE2, single operation, should be safe
 NtProtectVirtualMemory(NtCurrentProcess, &BaseAddress, &RegionSize, OldProtect, &OldProtect);   // VirtualProtect(this->ProcEndPtr,sizeof(OrigCodeEnd),PrevProt,&PrevProt);
 return true;
}
//------------------------------------------------------------------------------------

};
//------------------------------------------------------------------------------------
struct SHookLdrpInitialize      
{
 typedef void (_stdcall *THookProc)(volatile PCONTEXT Ctx, volatile PVOID NtDllBase);  // WinXP: Stack; Win7x32: Stack
#ifdef _AMD64_
 static const int JmpLen = 6;      // jmp [Rel32]; + Addr nearby
#else
static const int JmpLen = 5;      // jmp Rel32
#endif
 bool  ArgsOnStack;
 PBYTE PatchBegPtr;
 PBYTE ProcBegPtr;
 ULONG BegCodeLen;
 ULONG PatchSize;
 BYTE  OrigCodeBeg[16];
 BYTE  CodeBeg[128];            // Must be executable!

 bool IsActive(void){return this->PatchBegPtr && this->ProcBegPtr;}
//------------------------------------------------------------------------------------
PVOID _stdcall FindLdrpInitialize(void)
{
 static PBYTE PBase = NULL;
 if(!PBase)
  {
   char PrName[] = {~'L',~'d',~'r',~'I',~'n',~'i',~'t',~'i',~'a',~'l',~'i',~'z',~'e',~'T',~'h',~'u',~'n',~'k',0};
   for(int ctr=0;PrName[ctr];ctr++)PrName[ctr] = ~PrName[ctr];
   PBase = (PBYTE)NPEFMT::GetProcAddr(NPEFMT::GetNtDllBaseFast(), PrName);   // "LdrInitializeThunk"    
   if(!PBase)return NULL;
  }
#ifdef _AMD64_
 NHDE::HDE64 dhde;
#else
 NHDE::HDE32 dhde;
#endif
 PBYTE LastInstr = PBase;
 for(UINT offs=0;offs < 32;LastInstr=PBase, PBase += dhde.len, offs += dhde.len)
  {
   int len = dhde.Disasm(PBase);    
   if(len != 5)continue;   // Not call(E8) or jump(E9)
   if((*PBase != 0xE8) && (*PBase != 0xE9))continue;
   PVOID Addr = RelAddrToAddr(PBase,5,*(PDWORD)&PBase[1]);
#ifndef _AMD64_
   ArgsOnStack = (*LastInstr != 0x8B);   // i.e. Fastcall on Win10 WOW64    // 8F bor mov reg32, reg32
#else
   ArgsOnStack = true;    // Usual fastcall
#endif
   DBGMSG("Addr: %p, OnStk=%u",Addr, (int)ArgsOnStack);
   return Addr;
  }
 return NULL;
}
//------------------------------------------------------------------------------------
bool _stdcall SetHook(THookProc ProcBefore)
{
#ifdef _AMD64_
 NHDE::HDE64 dhde;
#else
 NHDE::HDE32 dhde;
#endif
 this->ProcBegPtr = (PBYTE)FindLdrpInitialize();
 if(!this->ProcBegPtr){DBGMSG("Failed to find LdrpInitialize!"); return false;}
 this->BegCodeLen = 0;
#ifdef _AMD64_
 this->PatchBegPtr = &this->ProcBegPtr[-1];
 ULONG ABytesNum = 0;
 for(BYTE Val=*this->PatchBegPtr;(ABytesNum < 8)&&(Val == *this->PatchBegPtr);this->PatchBegPtr--)ABytesNum++;    // 0xCC or 0x90
 if(ABytesNum < 8){DBGMSG("Not enough space for hook address!"); return false;}
 this->PatchBegPtr++;
 this->PatchSize   = ABytesNum;
#else
 this->PatchBegPtr = this->ProcBegPtr;
 this->PatchSize   = 0;
#endif
 for(PBYTE DisAddr=this->ProcBegPtr;this->BegCodeLen < JmpLen;DisAddr += dhde.len)   // MSVC compiler crash if 'this->HookLen += dhde.len' is here
  {
   this->BegCodeLen += dhde.Disasm(DisAddr);
  }
 this->PatchSize += this->BegCodeLen;
 BYTE PatchBuf[16];
 _mm_storeu_si128((__m128i*)&OrigCodeBeg, *(__m128i*)this->PatchBegPtr);  // SSE2, single operation, should be safe
 _mm_storeu_si128((__m128i*)&PatchBuf, *(__m128i*)&OrigCodeBeg); 
#ifdef _AMD64_
 *(PVOID*)&PatchBuf    = &CodeBeg;   // Trampoline code
 *(PWORD)&PatchBuf[8]  = 0x25FF;     // Jmp [Rel32]
 *(long*)&PatchBuf[10] = AddrToRelAddr<PBYTE>(this->ProcBegPtr, 6, this->PatchBegPtr);

 *(PDWORD)&CodeBeg[ 0]  = 0xB8485251;   // push rcx;  push rdx; movabs
 *(PVOID*)&CodeBeg[ 4]  = ProcBefore;
 *(PDWORD)&CodeBeg[12]  = 0x20EC8348;   // sub rsp, 32  // reserve for 4 args
 *(UINT64*)&CodeBeg[16] = 0x595A20C48348D0FF;    // call rax; add rsp, 32; pop rdx; pop rcx   // Caller removes args from stack
 memcpy(&CodeBeg[24], this->ProcBegPtr, this->BegCodeLen);
 UINT Offs = 24 + this->BegCodeLen;
 *(PWORD)&CodeBeg[Offs] = 0xB848;    // movabs RAX
 *(PVOID*)&CodeBeg[Offs+2]  = &this->ProcBegPtr[this->BegCodeLen];  // Continue to original code
 *(PWORD)&CodeBeg[Offs+2+8] = 0xE0FF;    // jmp RAX
#else
 PatchBuf[0] = 0xE9;
 *(long*)&PatchBuf[1] = AddrToRelAddr<PBYTE>(this->ProcBegPtr, 5, &CodeBeg[0]);
 UINT DPOffs = 0;
 if(ArgsOnStack)
  {
   *(PDWORD)&CodeBeg[0] = 0x082474FF;   // push [ESP+8]
   *(PDWORD)&CodeBeg[4] = 0x082474FF;   // push [ESP+8]
   CodeBeg[8] = 0xE8;            // call rel32
   *(long*)&CodeBeg[9]  = AddrToRelAddr<PBYTE>(&CodeBeg[8], 5, (PBYTE)ProcBefore);
   DPOffs = 13;
  }
   else
    {
     *(PDWORD)&CodeBeg[0]  = 0x51525251;    // push ecx;  push edx;  push edx;  push ecx
     CodeBeg[4] = 0xE8;    // call rel32
     *(long*)&CodeBeg[5]   = AddrToRelAddr<PBYTE>(&CodeBeg[4], 5, (PBYTE)ProcBefore);
     *(PWORD)&CodeBeg[9]   = 0x595A;   // pop edx;  pop ecx
     DPOffs = 11;
    }
 memcpy(&CodeBeg[DPOffs], this->ProcBegPtr, this->BegCodeLen);
 DPOffs += this->BegCodeLen;
 CodeBeg[DPOffs] = 0x68;    // push
 *(PVOID*)&CodeBeg[DPOffs+1] = &this->ProcBegPtr[this->BegCodeLen];     // Continue to original code
 CodeBeg[DPOffs+5] = 0xC3;  // ret
#endif
 PVOID  BaseAddress = this->PatchBegPtr;
 SIZE_T RegionSize  = this->PatchSize;
 ULONG  OldProtect  = 0;
 NtProtectVirtualMemory(NtCurrentProcess, &BaseAddress, &RegionSize, PAGE_EXECUTE_READWRITE, &OldProtect);  
 _mm_storeu_si128((__m128i*)this->PatchBegPtr, *(__m128i*)&PatchBuf);  // SSE2, single operation, should be safe
 NtProtectVirtualMemory(NtCurrentProcess, &BaseAddress, &RegionSize, OldProtect, &OldProtect);  
 return true;
}
//------------------------------------------------------------------------------------
bool Remove(void)
{
 if(!this->IsActive())return false;
 PVOID  BaseAddress = this->PatchBegPtr;
 SIZE_T RegionSize  = this->PatchSize;
 ULONG  OldProtect  = 0;
 NtProtectVirtualMemory(NtCurrentProcess, &BaseAddress, &RegionSize, PAGE_EXECUTE_READWRITE, &OldProtect);  
 _mm_storeu_si128((__m128i*)this->PatchBegPtr, *(__m128i*)&OrigCodeBeg);  // SSE2, single operation, should be safe
 NtProtectVirtualMemory(NtCurrentProcess, &BaseAddress, &RegionSize, OldProtect, &OldProtect);  
 return true;
}
//------------------------------------------------------------------------------------
};
//------------------------------------------------------------------------------------
};