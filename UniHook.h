
#pragma once

#ifndef UniHookH
#define UniHookH
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

#include <emmintrin.h>   // NOTE: All '#include' must be commented out and used only as a hint to compose BaseHdr.h individually for each project
#include "HDE.h"

// TODO: If hooking a function that is imported by this module, update ImportRecord to point to OrigProc
// https://software.intel.com/sites/landingpage/IntrinsicsGuide/#techs=SSE2
//---------------------------------------------------------------------------
#if defined(_AMD64_)
// Must be optimized to 'jmp rax' at the end  // NOTE: Arguments number is limited!!!!!
// LOGMSG("Calling: %p, %s, %s",Address,#NameAPI,LibPathName);  
#define APIWRAPPER(LibPathName,NameAPI) extern "C" _declspec(dllexport) void __cdecl NameAPI(PVOID ParA, PVOID ParB, PVOID ParC, PVOID ParD, PVOID ParE, PVOID ParF, PVOID ParG, PVOID ParH, PVOID ParIF, PVOID ParJ, PVOID ParK, PVOID ParL) \
{ \
 static void* Address; \
 if(!Address)Address = GetProcAddress(LoadLibrary(LibPathName),#NameAPI); \
 ((DWORD (__cdecl *)(...))(Address))(ParA, ParB, ParC, ParD, ParE, ParF, ParG, ParH, ParIF, ParJ, ParK, ParL); \
}

#else

#define APIWRAPPER(LibPathName,NameAPI) extern "C" _declspec(dllexport) _declspec(naked) void __cdecl NameAPI(void) \
{ \
 static void* Address; \
 if(!Address)Address = GetProcAddress(LoadLibrary(LibPathName),#NameAPI); \
 __asm mov EAX, [Address] \
 __asm jmp EAX \
}

#endif
//====================================================================================
#define ADDROFFSET(addr,offset) ((addr)+(offset))

#ifndef _REV_DW
#define _REV_DW(Value)    (((DWORD)(Value) << 24)|((DWORD)(Value) >> 24)|(((DWORD)(Value) << 8)&0x00FF0000)|(((DWORD)(Value) >> 8)&0x0000FF00))
#endif
//------------------------------------------------------------------------------------
#pragma pack(push,1)

_inline long  AddrToRelAddr(PVOID CmdAddr, DWORD CmdLen, PVOID TgtAddr){return -(((size_t)CmdAddr + CmdLen) - (size_t)TgtAddr);}
_inline PVOID RelAddrToAddr(PVOID CmdAddr, DWORD CmdLen, long TgtOffset){return (PVOID)((((PBYTE)CmdAddr) + CmdLen) + TgtOffset);}


//====================================================================================
//
//------------------------------------------------------------------------------------
#define VHOOK(proc) SVftHook<decltype(proc), &proc>              
template<typename T, void* HookProc> struct SVftHook      //  'T HProc' will crash the MSVC compiler!
{
 T* OrigProc;

 void SetHook(void** ProcAddr)
  {
   *(void**)&this->OrigProc = *ProcAddr; 
   *ProcAddr = HookProc; 
  }
};
//====================================================================================
//
//------------------------------------------------------------------------------------
#define PHOOK(proc) SProcHook<decltype(proc), &proc>              
template<typename T, void* HProc=0> struct SProcHook      //  'T HProc' will crash the MSVC compiler!    // Declare all members static that the hook struct can be declared temporary on stack (without 'static' cpecifier)?
{
#ifdef _AMD64_
 static const unsigned int TrLen = 12;   // 48 B8 11 22 33 00 FF FF FF 0F   movabs rax,FFFFFFF00332211  // jmp eax
#else
 static const unsigned int TrLen = 5;	  // E9 XX XX XX XX  Jmp REL32
#endif
 T*     OrigProc;   // Execute stolen code and continue
 T*     HookAddr;   // Hooked function address
 T*     HookProc;   // Hook function
 //PVOID* Import;   // Redirected import of this module (If need to destroy PE header, do it AFTER hooking)
 UINT   HookLen;
 BYTE   OriginCode[16];  // Original code to restore when hook removed    // Size of __m128i 
 BYTE   StolenCode[64];  // A modified stolen code

//------------------------------------------------------------------------------------
static bool IsAddrHooked(PVOID PAddr, PVOID Hook)        // Already hooked by this instance!  // It is allowed to hook by different instances
{
 PBYTE Addr = (PBYTE)PAddr;
#ifdef _AMD64_
 if((Addr[0] != 0x48)||(Addr[1] != 0xB8)||(Addr[10] != 0xFF)&&(Addr[11] != 0xE0))return false;  // Very unprobable to be an original code 
 if(Hook && (*(PVOID*)&Addr[2] != Hook))return false;
#else
 if(Addr[0] != 0xE9)return true;   // Not reliable, it may be an original code
 if(Hook && (RelAddrToAddr(Addr,5,*(PDWORD)&Addr[1]) != Hook))return false;
#endif
 return true;
}
//------------------------------------------------------------------------------------

public:
enum EHookFlg {hfNone,hfFillNop};       // Restore MemProt flag?
//------------------------------------------------------------------------------------
bool IsActive(void){return (bool)this->HookLen;}
//------------------------------------------------------------------------------------
bool SetHook(LPSTR ProcName, LPSTR LibName, UINT Flags=hfFillNop, PVOID HookFunc=NULL)
{
 if(this->IsActive())return false;       // Already set
 HMODULE  hLib  = GetModuleHandleA(LibName);
 if(!hLib)hLib  = LoadLibraryA(LibName);
 PBYTE ProcAddr = (PBYTE)GetProcAddress(hLib,ProcName);
 if(!ProcAddr)return false;
 return this->SetHook(ProcAddr,hfFillNop,HookFunc);  
}  
//------------------------------------------------------------------------------------
bool SetHook(PBYTE ProcAddr=NULL, UINT Flags=hfFillNop, PVOID HookFunc=NULL)   // Can be reused with same ProcAddr after 'Remove'
{
 if(this->IsActive())return false;
#ifdef _AMD64_
 HDE64 dhde;
#else
 HDE32 dhde;
#endif
 DWORD PrevProt  = 0;					   
 UINT  CodeLen   = 0;
 this->HookLen   = 0;
 if(!HookFunc)HookFunc = HProc;
 if(!ProcAddr)ProcAddr = (PBYTE)this->HookAddr;
 if(*ProcAddr == 0xFC)ProcAddr++;   // CLD  // Just in case
 VirtualProtect(this,sizeof(*this),PAGE_EXECUTE_READWRITE,&PrevProt);	 // On some platforms a data sections is not executable!		   
 for(PBYTE DisAddr=ProcAddr;this->HookLen < TrLen;DisAddr += dhde.len)   // MSVC compiler crash if 'this->HookLen += dhde.len' is here
  {
   this->HookLen += dhde.Disasm(DisAddr);
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
   PVOID  Addr = RelAddrToAddr(DisAddr,dhde.len,dhde.disp.disp32);           //   &DisAddr[(int)dhde.len + (int)dhde.disp.disp32];
   PDWORD Carr = (PDWORD)&this->StolenCode[CodeLen];
   Addr = *(PVOID*)Addr;
  // LOGMSG("jmp at %p to %p, disp32 = %08X, imm32 = %08X",DisAddr,Addr,dhde.disp.disp32,dhde.imm.imm32);
   Carr[0]  = 0xF82444C7;	// mov [RSP-8], DWORD
   Carr[2]  = 0xFC2444C7;	// mov [RSP-4], DWORD
   Carr[4]  = 0xF82464FF;  //  jmp [RSP-8] 
   Carr[1]  = ((PDWORD)&Addr)[0];
   Carr[3]  = ((PDWORD)&Addr)[1];
   CodeLen += 20;
   continue;
  } 
#endif

   memcpy(&this->StolenCode[CodeLen],DisAddr,dhde.len);   // Copy current instruction as is
   CodeLen += dhde.len;
  }
 BYTE Patch[16];   
 this->HookProc = (T*)HookFunc;
 this->HookAddr = (T*)ProcAddr;
 this->OrigProc = (T*)&StolenCode;
 VirtualProtect(this,sizeof(*this),PAGE_EXECUTE_READWRITE,&PrevProt);	// Module`s data section may be not executable!		   
 VirtualProtect(ProcAddr,TrLen,PAGE_EXECUTE_READWRITE,&PrevProt);  
 *(__m128i*)&Patch = *(__m128i*)ProcAddr;
 *(__m128i*)&this->OriginCode = *(__m128i*)&Patch;
 if(Flags & hfFillNop)memset(&Patch,0x90,this->HookLen);      // Optional, good for debugging
#ifdef _AMD64_
 this->StolenCode[CodeLen]   = 0xFF;
 this->StolenCode[CodeLen+1] = 0x25;
 this->StolenCode[CodeLen+2] = this->StolenCode[CodeLen+3] = this->StolenCode[CodeLen+4] = this->StolenCode[CodeLen+5] = 0;
 *((PBYTE*)&this->StolenCode[CodeLen+6]) = ProcAddr + this->HookLen;   // Aligned by HDE to whole command
 Patch[0]  = 0x48;				       // movabs rax,FFFFFFF00332211   // EAX is unused in x64 calling convention  // TODO: Usesome SSE instructions to copy this by one operation
 Patch[1]  = 0xB8;
 *(PVOID*)&Patch[2] = HookFunc;        // NOTE: This was modified to use a single assignment somewhere already!!!!!
 Patch[10] = 0xFF;                     // jmp EAX      // EAX is not preserved!  // Replace with QHalves method if EAX must be preserved  // No relative addresses here for easy overhooking
 Patch[11] = 0xE0;
#else
 this->StolenCode[CodeLen] = 0xE9;	   // JMP Rel32
 *((PDWORD)&this->StolenCode[CodeLen+1]) = AddrToRelAddr(&this->StolenCode[CodeLen],TrLen,&ProcAddr[this->HookLen]);
 Patch[0]  = 0xE9;
 *((PDWORD)&Patch[1]) = AddrToRelAddr(ProcAddr,TrLen,HookProc);
#endif
 _mm_storeu_si128((__m128i*)ProcAddr, *(__m128i*)&Patch);  // SSE2, single operation, should be safe       // TODO: memcpy with aligned SSE2 16 byte copy
 VirtualProtect(ProcAddr,TrLen,PrevProt,&PrevProt); 
 FlushInstructionCache(GetCurrentProcess(),ProcAddr,sizeof(this->OriginCode));   // Is it really needed here?
 return true;
}  
//------------------------------------------------------------------------------------
bool Remove(bool Any=true)     // NOTE: If someone else takes our jump code and after that we exit - CRASH!   // TODO: Rethink hook chaining
{
 if(!this->HookLen)return false;
 if(IsAddrHooked(this->HookAddr, (Any)?(NULL):(this->HookProc)))   // If some other instance is not restored it already // NOTE: Original code will be lost if first instance sees that someone else hooked it here
  {
   DWORD PrevProt = 0;					   
   VirtualProtect(this->HookAddr,this->HookLen,PAGE_EXECUTE_READWRITE,&PrevProt);
   _mm_storeu_si128((__m128i*)this->HookAddr, *(__m128i*)&this->OriginCode);      // SSE2, single operation            
   VirtualProtect(this->HookAddr,this->HookLen,PrevProt,&PrevProt);
   FlushInstructionCache(GetCurrentProcess(),this->HookAddr,sizeof(this->OriginCode));    // Is it really needed here?
  }
 this->HookLen = 0;
 return true;
}
//------------------------------------------------------------------------------------
};     


#pragma pack(pop)
//---------------------------------------------------------------------------




//---------------------------------------------------------------------------
#endif
