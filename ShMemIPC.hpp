
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

struct NShMem     // TODO: Crossplatform - part of the FRAMEWORK 
{
static inline char ObjDirName[] = {"TempNamedObjects"};    // TODO: Generate it
//===========================================================================
// 'Global\HelloWorld' => '\BaseNamedObjects\HelloWorld'
// 'Local\HelloWorld'  => '\Sessions\1\BaseNamedObjects\HelloWorld'
//
//----------------------------------------------------------------------------
static void InitObjAttrForBaseNamedObj(OBJECT_ATTRIBUTES* Attr, UNICODE_STRING* UStr, wchar_t* Buf, LPSTR Name, LPSTR ObjDirPath)
{
 UINT Length = 0;
 if((*Name != '\\')&&(*Name != '/'))
  {
   char Path[] = {'\\','B','a','s','e','N','a','m','e','d','O','b','j','e','c','t','s','\\',0};    // If static then the sring will be in memory as is
   if(!ObjDirPath)ObjDirPath = Path;     // wchar_t Path[] = {'\\','T','e','m','p','N','a','m','e','d','O','b','j','e','c','t','s','\\'}; //  {'\\','B','a','s','e','N','a','m','e','d','O','b','j','e','c','t','s','\\'};  
   if((*ObjDirPath != '\\')&&(*ObjDirPath != '/'))Buf[Length++] = '\\';
   for(;*ObjDirPath;Length++,ObjDirPath++)Buf[Length] = *ObjDirPath;
   if((Buf[Length-1] != '\\')&&(Buf[Length-1] != '/'))Buf[Length++] = '\\';
  }
 for(;*Name;Name++,Length++)Buf[Length] = *Name;
 UStr->Buffer = Buf;
 UStr->Length = Length * sizeof(wchar_t);
 UStr->MaximumLength = UStr->Length + sizeof(wchar_t);
 Attr->Length = sizeof(OBJECT_ATTRIBUTES);
 Attr->RootDirectory = NULL;
 Attr->ObjectName = UStr;
 Attr->Attributes = 0;
 Attr->SecurityQualityOfService = NULL;
 Attr->SecurityDescriptor = NULL; 
// DBGMSG("%u - '%ls'",Length,Buf);
}
//----------------------------------------------------------------------------
static NTSTATUS CreateNtObjDirectory(LPSTR ObjDirName, PHANDLE phDirObj)   // Create objects directory with NULL security
{
 wchar_t Path[512] = {'\\'}; 
 UNICODE_STRING ObjectNameUS;
 SECURITY_DESCRIPTOR  sd = {SECURITY_DESCRIPTOR_REVISION, 0, 4};    // NULL security descriptor: InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION); SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE);
 OBJECT_ATTRIBUTES oattr = { sizeof(OBJECT_ATTRIBUTES), 0, &ObjectNameUS, OBJ_CASE_INSENSITIVE|OBJ_OPENIF, &sd };
 UINT Length = 1;   
 for(int idx=0;*ObjDirName;ObjDirName++)Path[Length++] = *ObjDirName;
 ObjectNameUS.Buffer = Path;
 ObjectNameUS.Length = Length * sizeof(wchar_t);
 ObjectNameUS.MaximumLength = ObjectNameUS.Length + sizeof(wchar_t);
 return NtCreateDirectoryObject(phDirObj, DIRECTORY_ALL_ACCESS, &oattr);
}
//----------------------------------------------------------------------------
static NTSTATUS MutexCreateA(PHANDLE pHandle, LPSECURITY_ATTRIBUTES lpMutexAttributes, BOOL bInitialOwner, LPSTR lpName, LPSTR lpObjDir)
{
 OBJECT_ATTRIBUTES oattr = {};       
 UNICODE_STRING ObjPathUS;
 wchar_t Path[256]; 
 InitObjAttrForBaseNamedObj(&oattr, &ObjPathUS, Path, lpName, lpObjDir);
 if(lpMutexAttributes)
  {
   oattr.Attributes = lpMutexAttributes->bInheritHandle ? 2 : 0;  
   oattr.SecurityDescriptor = lpMutexAttributes->lpSecurityDescriptor;
  }
 oattr.Attributes |= OBJ_OPENIF;   // Open if already exist
 return NtCreateMutant(pHandle, MUTEX_ALL_ACCESS, &oattr, bInitialOwner);
}
//----------------------------------------------------------------------------
static NTSTATUS EventCreateA(PHANDLE pHandle, LPSECURITY_ATTRIBUTES lpEventAttributes, BOOL bManualReset, BOOL bInitialState, LPSTR lpName, LPSTR lpObjDir)
{
 OBJECT_ATTRIBUTES oattr = {};       
 UNICODE_STRING ObjPathUS;
 wchar_t Path[256]; 
 InitObjAttrForBaseNamedObj(&oattr, &ObjPathUS, Path, lpName, lpObjDir);
 if(lpEventAttributes)
  {
   oattr.Attributes = lpEventAttributes->bInheritHandle ? 2 : 0;  
   oattr.SecurityDescriptor = lpEventAttributes->lpSecurityDescriptor;
  }
 oattr.Attributes |= OBJ_OPENIF;   // Open if already exist
 return NtCreateEvent(pHandle, EVENT_ALL_ACCESS, &oattr, bManualReset?NotificationEvent:SynchronizationEvent, bInitialState);
}
//----------------------------------------------------------------------------                                        
static NTSTATUS CreateMemSection(PHANDLE SectionHandle, ACCESS_MASK DesiredAccess, PLARGE_INTEGER MaximumSize, ULONG SectionPageProtection, ULONG AllocationAttributes, LPSTR SecName, LPSTR lpObjDir)
{
 OBJECT_ATTRIBUTES oattr = {};       
 UNICODE_STRING ObjPathUS;
 wchar_t Path[256]; 
 InitObjAttrForBaseNamedObj(&oattr, &ObjPathUS, Path, SecName, lpObjDir); 
 oattr.Attributes |= OBJ_OPENIF;   // Open if already exist
 return NtCreateSection(SectionHandle, DesiredAccess, &oattr, MaximumSize, SectionPageProtection, AllocationAttributes, NULL);
}
//---------------------------------------------------------------------------- 
static NTSTATUS OpenMemSection(PHANDLE SectionHandle, ACCESS_MASK DesiredAccess, LPSTR SecName, LPSTR lpObjDir)
{
 OBJECT_ATTRIBUTES oattr = {};       
 UNICODE_STRING ObjPathUS;
 wchar_t Path[256]; 
 InitObjAttrForBaseNamedObj(&oattr, &ObjPathUS, Path, SecName, lpObjDir); 
 return NtOpenSection(SectionHandle, DesiredAccess, &oattr);
}
//---------------------------------------------------------------------------- 
static inline long WaitForSingle(HANDLE hHandle, DWORD dwMilliseconds)
{
#ifdef _NTDLL_H
 LARGE_INTEGER Timeout;
 LARGE_INTEGER* Tio;
 if(INFINITE != dwMilliseconds){Timeout.QuadPart = -10000i64 * dwMilliseconds; Tio = &Timeout;}
   else Tio = NULL;
 NTSTATUS status = NtWaitForSingleObject(hHandle, FALSE, Tio);
 if((long)status < 0)return WAIT_FAILED;     
 return status;  // If Alerted is FALSE
#else
 return WaitForSingleObject(hHandle, dwMilliseconds);
#endif
}
//----------------------------------------------------------------------------
static inline ULONG GetTicksCount(void)
{
#ifdef _NTDLL_H
 PBYTE pKiUserSharedData = reinterpret_cast<PBYTE>(0x7FFE0000);
 return (((*(PDWORD)pKiUserSharedData)?((UINT64)*(PDWORD)pKiUserSharedData):(*(UINT64*)&pKiUserSharedData[0x320])) * *(PDWORD)&pKiUserSharedData[4]) >> 24;  // 'TickCountLow * TickCountMultiplier' or 'TickCount * TickCountMultiplier'
#else
 return GetTickCount();
#endif
}
//----------------------------------------------------------------------------
static inline ULONG CurrentThreadID(void)
{
#ifdef _NTDLL_H
 return NtCurrentThreadId();
#else
 return GetCurrentThreadId();
#endif
}
//----------------------------------------------------------------------------
static inline ULONG CurrentProcessID(void)
{
#ifdef _NTDLL_H
 return NtCurrentProcessId();
#else
 return GetCurrentProcessId();
#endif
}
//----------------------------------------------------------------------------
static inline long CloseOHandle(HANDLE Hndl)
{
 if(((SIZE_T)Hndl + 1) > 1)   // Checked for NULL(0) and INVALID_HANDLE_VALUE(-1)
#ifdef _NTDLL_H
  return !NtClose(Hndl);  
#else
  return CloseHandle(Hndl);  
#endif
 return 0;
}
//----------------------------------------------------------------------------
template<typename A> static ULONG SizeString(A Str)
{
 unsigned long idx = 0;
 while(Str[idx])idx++;
 return idx;
}
//----------------------------------------------------------------------------
template<typename A, typename B> static ULONG CopyString(A DstStr, B SrcStr, unsigned long MaxSize=-1)
{
 unsigned long idx = 0;
 for(;MaxSize && SrcStr[idx];idx++,MaxSize--)DstStr[idx] = SrcStr[idx];
 DstStr[idx] = 0;
 return idx; 
}
//----------------------------------------------------------------------------
template<typename A, typename B> static ULONG AddString(A DstStr, B SrcStr, unsigned long MaxSize=-1)
{
 unsigned long idx = 0;
 ULONG DstLen = SizeString(DstStr);
 DstStr += DstLen;
 for(;MaxSize && SrcStr[idx];idx++,MaxSize--)DstStr[idx] = SrcStr[idx];
 DstStr[idx] = 0;
 return DstLen + idx;
}
//----------------------------------------------------------------------------
template<typename T, typename O> static O _fastcall UIntToString(T Val, O buf, int* Len)     // TODO: Optimize
{
 if(Val == 0){if(Len)*Len = 1; *buf = '0'; buf[1] = 0; return buf;}
 buf  = &buf[20];
 *buf = 0;
 O end = buf;
 for(buf--;Val;buf--)
  {
   *buf  = (Val % 10) + '0';
   Val  /= 10;
  }
 buf++;
 if(Len)*Len = end-buf; 
  else buf[end-buf] = 0;
 return buf;     // Optionally move?
}
//----------------------------------------------------------------------------
template<typename T, typename S> static S UIntToHexString(T Value, int MaxDigits, S NumBuf, bool UpCase, int* Len=0)    // TODO: Optimize
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


//============================================================================
//  Slow Critcal Section :(
//-------------------------------------
// Declare all variables volatile, so that the compiler won't try to optimize something important.
// SRWLock spin count is 1024 if there is more than one processor core (NtCurrentTeb()->ProcessEnvironmentBlock->NumberOfProcessors)
//
//#define _USESYSCRITSEC
template<unsigned long DefSpinCtr=1024, unsigned long DefTimeout=-1> class CCritSectEx           // -1 is INFINITE      // TODO: Shareable critical section
{
#ifndef _USESYSCRITSEC
 volatile HANDLE hSemaphore;
 volatile long	 OwnerThID;
 volatile long   WaitThCtr;
 volatile long   RecurCtr;
 static inline int CoresCnt = 0; 
//----------------------------------------------------------------------------
#else
  CRITICAL_SECTION csec;
#endif

public:
#ifndef _USESYSCRITSEC
CCritSectEx(void)
{
 memset(this, 0,sizeof(*this)); 
 if(!CoresCnt)
  {
#ifdef _NTDLL_H
   this->CoresCnt = NtCurrentTeb()->ProcessEnvironmentBlock->NumberOfProcessors;
#else
   SYSTEM_INFO stSI;
   GetSystemInfo(&stSI);
   this->CoresCnt = stSI.dwNumberOfProcessors;
#endif
  }
}
~CCritSectEx(){NShMem::CloseOHandle(this->hSemaphore);}
#else
CCritSectEx(void){InitializeCriticalSection(&this->csec);} 
~CCritSectEx(){DeleteCriticalSection(&this->csec);}  
#endif
//----------------------------------------------------------------------------
bool Lock(ULONG SpinCtr=DefSpinCtr, ULONG WaitTimeout=DefTimeout)
{
#ifndef _USESYSCRITSEC
 if(this->TryLock(SpinCtr))return true;
// if(!WaitTimeout)return false;      // No waiting time specified!  
 if(!this->hSemaphore) // && !_InterlockedCompareExchangePointer(&this->hSemaphore, (void*)-1, NULL))  	// Ensure that we have the kernel event created
  {
#ifdef _NTDLL_H
   HANDLE hSemaphore = NULL;
   NTSTATUS status = NtCreateSemaphore(&hSemaphore,NULL,NULL,0,0x7FFFFFFF);    // 40000000 	STATUS_OBJECT_NAME_EXISTS
#else
   HANDLE hSemaphore = CreateSemaphoreW(NULL, 0, 0x7FFFFFFF, NULL);    
#endif
   if(_InterlockedCompareExchangePointer(&this->hSemaphore, hSemaphore, NULL))NShMem::CloseOHandle(hSemaphore); //Close it if someone is already created it
//   if(this->TryLock(SpinCtr))return true;
  }
 bool bWaiter = false;
 ULONG ThisThreadId = CurrentThreadID();
 for(ULONG InitialTicks = GetTicksCount();;)   // TODO: Crossplatform timing
  {
   if(!bWaiter)_InterlockedIncrement(&this->WaitThCtr);
   if(!this->OwnerThID && !_InterlockedCompareExchange(&this->OwnerThID, ThisThreadId, 0)){_InterlockedDecrement(&this->WaitThCtr); ++this->RecurCtr; return true;}
   ULONG WaitElapsed;
   if((ULONG)-1 != WaitTimeout)
    {
     WaitElapsed = GetTicksCount() - InitialTicks; // how much time elapsed
     if(WaitTimeout <= WaitElapsed){_InterlockedDecrement(&this->WaitThCtr); return false;}   // Failed to acquire - TIMEOUT
     WaitElapsed = WaitTimeout - WaitElapsed;
    }
	else WaitElapsed = (ULONG)-1;
   switch(NShMem::WaitForSingle(this->hSemaphore, WaitTimeout))
    {
     case WAIT_OBJECT_0:
     case WAIT_ABANDONED:  // An previous owner thread just died 
       bWaiter = false;
       break;
     case WAIT_TIMEOUT:
//       DBGMSG("WAIT_TIMEOUT %u\n", CurrentThreadID());
       bWaiter = true;
       break;
    }
  }  
 #else
 EnterCriticalSection(&this->csec);
 #endif
 return true;
}
//----------------------------------------------------------------------------
bool Unlock(void)
{
#ifndef _USESYSCRITSEC
 DWORD ThisThreadId = CurrentThreadID();
 if(ThisThreadId != this->OwnerThID)return false;   // Inconsistent Unlock!
 if(--this->RecurCtr > 0)return false;   // Still owned by this thread
 //_WriteBarrier(); // changes done to the shared resource are committed.
 _InterlockedAnd(&this->OwnerThID, 0);  // this->OwnerThID = 0;     // Hangs everything without Interlocked write here
 //_ReadWriteBarrier(); // The CS is released.
 if(this->WaitThCtr > 0) // AFTER it is released we check if there're waiters.
  {
   _InterlockedDecrement(&this->WaitThCtr);
#ifdef _NTDLL_H
   NtReleaseSemaphore(this->hSemaphore, 1, NULL);
#else
   ReleaseSemaphore(this->hSemaphore, 1, NULL); // Notify waiters that we are finished // Increase count  // The state of a semaphore object is signaled when its count is greater than zero, and nonsignaled when its count is equal to zero. 
#endif
  }  
#else
 LeaveCriticalSection(&this->csec);
#endif
 return true;
}
//----------------------------------------------------------------------------
bool TryLock(ULONG SpinCtr=DefSpinCtr)    // Light lock
{
#ifndef _USESYSCRITSEC
 ULONG ThisThreadId = CurrentThreadID();
 if(ThisThreadId == this->OwnerThID){++this->RecurCtr; return true;}    // Recursion of already acquired
 if(this->CoresCnt <= 1)return false;
 do
  {
   if(!this->OwnerThID && !_InterlockedCompareExchange(&this->OwnerThID, ThisThreadId, 0)){++this->RecurCtr; return true;}  // Takes ownersip if OwnerThID is 0   
   YieldProcessor();    // _mm_pause() 
  }
   while(SpinCtr--);
 return false; 
#else
 return TryEnterCriticalSection(&this->csec);
#endif
}
//----------------------------------------------------------------------------
void Release(void)   // Emergency release if owner thread has died
{

}
//----------------------------------------------------------------------------
};
//===========================================================================
//
//
//
//---------------------------------------------------------------------------
class CGrowBuf  // Size: 16/32   // A helper class to use with IPC procedure call     // TODO: A decent allocator
{
static const unsigned int PAGE_SIZE = 0x1000;
static const unsigned int GRAN_SIZE = 0x10000;
static SIZE_T AlignAllocSize(SIZE_T Size){return (Size + (GRAN_SIZE-1)) & ~(GRAN_SIZE-1);}

PBYTE GetLocalBuf(void){return (PBYTE)this + sizeof(CGrowBuf);}

protected:
 PBYTE  Buff;   // May be not NULL but point to a stack buffer in a derived class
 SIZE_T DSize;  // Size of a valid data
 SIZE_T FSize;  // Full size of allocated buffer
 SIZE_T LSize;  // Size of local buffer which may follow after a CGrowBuf instance in a derived class

public:
CGrowBuf(void)
{
 this->FSize = this->LSize = this->DSize = 0; 
 this->Buff  = this->GetLocalBuf();
}
~CGrowBuf(){this->Clear();}
//------------------------------------------------------------------------------------
UINT  GetLen(void){return this->DSize;}
PVOID GetPtr(void){return this->Buff;}
//------------------------------------------------------------------------------------
static void FreeMem(PVOID Addr, SIZE_T Size)  
{
 if(!Addr)return;
 DBGMSG("Releasing: %p, %p",Addr,Size);
 Size = AlignAllocSize(Size);  
 PBYTE APtr = (PBYTE)Addr;
 PBYTE EPtr = &APtr[Size];
 while(APtr < EPtr)
  {
#ifdef _NTDLL_H
   SIZE_T FSize = 0;
   if(NtFreeVirtualMemory(NtCurrentProcess,(PVOID*)&APtr,&FSize,MEM_RELEASE))break;   // Step by GRAN_SIZE  
   APtr += FSize;
#else
   VirtualFree(APtr,0,MEM_RELEASE);  
   APtr += GRAN_SIZE;    // Fails if blocks were larger until reached a base addr of a next block (Cannot know without VirtualWuery from which blocks this range consists) 
#endif
   DBGMSG("Released: %p",APtr);
  }
 DBGMSG("Left: %p",(EPtr-APtr));
}
//------------------------------------------------------------------------------------
// When you allocate less than 64K you end up with a range of wasted pages that can't be reserved or committed. So virtual address space goes to waste. 
static PVOID AllocMem(PVOID Addr, SIZE_T* Size)     // Fast but wastes some memory!
{
 *Size = AlignAllocSize(*Size);      // To avoid 64k leftover holes
 DBGMSG("Allocating: %p, %p",Addr, *Size);
#ifdef _NTDLL_H
 if(NtAllocateVirtualMemory(NtCurrentProcess, &Addr, 0, Size, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE))Addr = NULL;   // Commits only a page-aligned FSize
#else
 BaseAddress = VirtualAlloc(Addr,*Size,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);           // Untested!
#endif
 DBGMSG("Allocated: %p, %p",Addr, *Size);
 return Addr;
}
//------------------------------------------------------------------------------------
static PVOID ReAllocMem(PVOID Addr, SIZE_T OldSize, SIZE_T* Size)
{
 DBGMSG("Addr=%p, OldSize=%08X, NewSize=%08X",Addr,OldSize,*Size);
 if(*Size <= OldSize){*Size = OldSize; DBGMSG("Cannot shrink: %p",Addr); return Addr;}
 SIZE_T AOSize = AlignAllocSize(OldSize);
 SIZE_T NSize  = *Size - OldSize;
 PVOID NewAddr = AllocMem((PBYTE)Addr + AOSize, &NSize);    
 if(NewAddr){*Size = AOSize+NSize; DBGMSG("Expanded: %p to %p, %p",Addr,NewAddr,*Size); return Addr;}  // Added an adjacent block
 NewAddr = AllocMem(NULL, Size);
 DBGMSG("Relocated: %p",NewAddr);
 memcpy(NewAddr, Addr, OldSize);     // Lets crash if it is NULL :)
 FreeMem(Addr, OldSize);
 return NewAddr;
}
//------------------------------------------------------------------------------------
void Assign(PVOID Data, SIZE_T DataLen, bool DoCopy=false)
{
 this->Clear();
 this->DSize = DataLen;
 if(DoCopy)
  {
   if(DataLen > this->LSize)
    {
     this->FSize = DataLen;
     this->Buff  = (PBYTE)AllocMem(NULL, &this->FSize);
    }
   memcpy(this->Buff,Data,DataLen);
  }
   else    // Useful to allow any derived class to process some external buffer as its own (Usually this is done with some Allocator/Stream class)
    {
     this->Buff  = (PBYTE)Data; 
     this->FSize = 0;      // Not owns the buffer
    }
// DBGMSG("Assigned: %p, %08X, %u",this->Buff,DataLen,(int)DoCopy);
}
//------------------------------------------------------------------------------------
void GrowFor(SIZE_T Len)    
{  
 if(Len > this->LSize)
  {
   if(!this->FSize)
    {
     this->FSize  = Len;
     PBYTE NewPtr = (PBYTE)AllocMem(NULL, &this->FSize); 
     memcpy(NewPtr, this->Buff, this->DSize);    // Copy local data
     this->Buff   = NewPtr;    
    }
     else if(Len > this->FSize)    // No shrinking supported!  //  (CGrowBuf should be short lived) 
      {
       this->FSize = Len;
       this->Buff  = (PBYTE)ReAllocMem(this->Buff, this->DSize, &this->FSize);   
      }
  }
 this->DSize = Len;
}
//------------------------------------------------------------------------------------
void Clear(void)
{
 if(this->FSize > this->LSize)
  {
   FreeMem(this->Buff, this->FSize);
   this->FSize = 0;
  }  
// DBGMSG("Cleared: %p",this->Buff);  
 this->DSize = 0; 
 this->Buff  = this->GetLocalBuf(); // Even if there is none
}
//------------------------------------------------------------------------------------

};
//====================================================================================
template<typename T> class CGrowArrImpl: public CGrowBuf 
{
protected:
CGrowArrImpl(void) {}   // Prenents direct instantiation
void Resize(size_t Cnt){this->GrowFor(Cnt * sizeof(T));}

public:
 operator  T*() {return (T*)this->Buff;}     // Gives 'Array[idx]' access
 UINT Count(void){return (this->DSize / sizeof(T));}
 UINT Size(void){return this->DSize;}

T* Add(T* Data, size_t Cnt=1)
{
// DBGMSG("Adding: %p, %u",Data,Cnt);
 size_t OIdx = this->Count();
 this->Resize(OIdx+Cnt);
 if(Data)memcpy(&((T*)this->Buff)[OIdx], Data, Cnt*sizeof(T));
 return &((T*)this->Buff)[OIdx];
}
//------------------------------------------------------------------------------------
bool Remove(size_t Idx, size_t Cnt=1)
{
// DBGMSG("Removing: %u, %u",Idx,Cnt);
 size_t OIdx = this->Count();
 if(Idx >= OIdx)return false;
 size_t LIdx = Idx + Cnt;
 if(LIdx < OIdx)
  {
   memmove(&((T*)this->Buff)[Idx], &((T*)this->Buff)[LIdx], (OIdx-LIdx)*sizeof(T)); 
   this->Resize(OIdx-Cnt); 
  }
  else this->Resize(Idx);       // At the end of list
 return true;
}
//------------------------------------------------------------------------------------
bool Remove(T* Itm, size_t Cnt=1)
{
 return this->Remove(size_t(Itm - ((T*)this->Buff)), Cnt);
}
//------------------------------------------------------------------------------------

};
//====================================================================================
template<typename T, UINT Prealloc=32> class CGrowArr: public CGrowArrImpl<T>
{
 T Array[Prealloc];

public:
 CGrowArr(void){this->LSize = Prealloc * sizeof(T);}
};
//====================================================================================
template<UINT MaxSize=512> class CArgPack: public CGrowBuf     // Use this if there are more than one argument IN or OUT
{
 BYTE Data[MaxSize];

public:
CArgPack(void){this->LSize = MaxSize;}
//------------------------------------------------------------------------------------
template<typename T> PBYTE PushArgEx(T& Value, char* Name=NULL, UINT Hint=0){return this->PushBlkEx(sizeof(T), &Value, Name, Hint);}
PBYTE PushBlkEx(UINT ValLen, PVOID Value=NULL, char* Name=NULL, UINT Hint=0)
{
 UINT Offs = this->GetLen();
 this->PushBlk(ValLen, Value);     // Pointer is invalidated by ReAlloc
 if(Name){this->PushStr(Name); ValLen |= 0x80000000;}
 if(Hint){this->PushArg(Hint); ValLen |= 0x40000000;}
 this->PushArg(ValLen);
 return (PBYTE)this->GetPtr() + Offs;
}
//------------------------------------------------------------------------------------
PBYTE PopBlkEx(UINT* ValLen, char* Name=NULL, UINT* Hint=NULL)  // No PopArgEx for this
{
 UINT ValSize = 0;
 UINT HintVal = 0;
 if(!this->PopArg(ValSize))return NULL;
 if(ValSize & 0x40000000)this->PopArg(HintVal);
 if(ValSize & 0x80000000)this->PopStr(Name, (Hint)?(*Hint):(0));
 ValSize &= 0x0FFFFFFF;
 if(Hint)*Hint = HintVal;
 if(ValLen)*ValLen = ValSize; 
 return this->PopBlk(ValSize, NULL);
}
//------------------------------------------------------------------------------------
PBYTE GetBlkAt(UINT& Offset=-1)
{
 if(Offset == (UINT)-1)Offset = this->DSize;
 UINT ValSize = 0;
 if(sizeof(ValSize) > Offset)return NULL; 
 Offset -= sizeof(ValSize);                 
 ValSize = *(UINT*)&this->Buff[Offset];
 if(ValSize & 0x40000000)Offset -= sizeof(UINT);
 if(ValSize & 0x80000000)
  {
   UINT StrSize = 0;
   if(sizeof(StrSize) > Offset)return NULL; 
   Offset -= sizeof(StrSize);                 
   StrSize = *(UINT*)&this->Buff[Offset];
   if(StrSize > Offset)return NULL; 
   Offset -= StrSize;
  }
 ValSize &= 0x0FFFFFFF;
 if(ValSize > Offset)return NULL; 
 Offset -= ValSize;
 return &this->Buff[Offset];
}
//------------------------------------------------------------------------------------
template<typename T> PBYTE PushArg(T& Value){return this->PushBlk(sizeof(T), &Value);}
template<typename T> PBYTE PopArg(T& Value){return this->PopBlk(sizeof(T), &Value);}
template<typename T> T     PopArg(void){T val; this->PopBlk(sizeof(T), &val); return val;}
//------------------------------------------------------------------------------------
template<typename T> PBYTE PushStr(T Str)
{
 UINT Len = 0;  // In chars // Not Including Zero
 for(int ctr=0;Str[ctr];ctr++,Len++);
 this->PushBlk(Len*sizeof(*Str), Str);    // Push the string Str
 return this->PushArg(Len);    // CharCount of string
}
//------------------------------------------------------------------------------------
template<typename T> PBYTE PopStr(T Str, UINT MaxLen=0)  // MaxLen in chars includes Zero 
{
 UINT FullLen = 0;
 this->PopArg(FullLen);
 if(!MaxLen || (--MaxLen > FullLen))MaxLen = FullLen;
 if(MaxLen < FullLen)this->PopBlk((FullLen-MaxLen)*sizeof(*Str), NULL);  // Skip end of string
 if(Str)Str[MaxLen] = 0;
 return this->PopBlk(MaxLen*sizeof(*Str), Str);
}
//------------------------------------------------------------------------------------
PBYTE PushBlk(UINT ValLen, PVOID Value=NULL)       // TODO: Optimize allocations // Use an external memory pool
{
// DBGMSG("ValLen: %08X",ValLen);
 SIZE_T OldSize = this->DSize;
 this->GrowFor(this->DSize + ValLen);
 PBYTE DstPtr = &this->Buff[OldSize];
 if(Value)memcpy(DstPtr,Value,ValLen);
 return DstPtr;
}
//------------------------------------------------------------------------------------
PBYTE PopBlk(UINT ValLen, PVOID Value=NULL)
{
 if(ValLen > this->DSize){this->Clear(); return NULL;}  // No that much data in buffer
 this->DSize -= ValLen;                  // Just forgetting current block size and resizing it on next PUSH?
 PBYTE DstPtr = &this->Buff[this->DSize];
 if(Value)memcpy(Value,DstPtr,ValLen);
 return DstPtr;
}
//------------------------------------------------------------------------------------
};
//====================================================================================
//
//
//
//------------------------------------------------------------------------------------
template<typename Usr=long> class CSharedMem     // TODO: Kernel support (Use same NTAPI?)   // TODO: Time of message locking to unlock by timeout even if an owner process is crashed
{
static const int MaxNameSize = 64;

#pragma pack(push,1)
struct SMemDescr: public Usr  
{
 volatile UINT32 MemFlgs;    // Unused for now
 volatile UINT32 MemSize;    // Size of MemData
 volatile BYTE   NtfEName[MaxNameSize];
 volatile BYTE   SynMName[MaxNameSize];         // Name of Mutex
 volatile BYTE   Data[0];              // Should be aligned to 8
};
#pragma pack(pop)

 HANDLE hNotifyEvt;    // NOTE: Do not rely on Notification Event when used a multiple observers(Set timeout as low as possible and confirm changes by some other means)
 HANDLE hSyncMutex;
 HANDLE hMapFile;
 HANDLE hDirObj;
 SMemDescr* MemDesc;   // Shared memory buffer

//---------------------------------------------------------

//---------------------------------------------------------
#ifndef _NTDLL_H
static ULONG GetObjNamespaceStr(char* DstStr)
{
 char NameSpace[] = {'G','l','o','b','a','l','\\'};     // NOTE: Incompatible with custom named object directory
 return CopyString(DstStr, NameSpace, sizeof(NameSpace));
}
#endif
//---------------------------------------------------------
static void MakeObjName(ULONG_PTR Value, LPSTR CustomPart, LPSTR OutName)
{
 char TmpBuf[MaxNameSize];
 TmpBuf[0] = 0;
 if(CustomPart)
  {
   int idx = 0;
   if(Value)TmpBuf[idx++] = '_';
   CopyString(&TmpBuf[idx], CustomPart, sizeof(TmpBuf)-(8+8));  // Global\XXXXXXXX_CustomPart
  }
 ULONG_PTR NumPart = Value;
#ifndef _NTDLL_H
 ULONG Len = GetObjNamespaceStr(OutName); 
#else
 ULONG Len = 0;
#endif
 if(NumPart)
  {
   NumPart ^= (GetTicksCount() * (CurrentThreadID() * CurrentProcessID()));  // Randomized?
   int HLen = 0;
   char HexBuf[64];
   char* ptr = UIntToHexString(NumPart, sizeof(void*)*2, HexBuf, true, &HLen); 
   Len += CopyString(&OutName[Len], ptr, HLen); 
  }
 CopyString(&OutName[Len], TmpBuf); 
}
//---------------------------------------------------------

public:

CSharedMem(void)
{
 this->hMapFile   = NULL;
 this->hNotifyEvt = NULL;
 this->hSyncMutex = NULL;
 this->MemDesc    = NULL;
}
//---------------------------------------------------------
~CSharedMem(void)
{
 this->Disconnect();
}
//---------------------------------------------------------
bool   IsConnected(void){return (bool)this->hMapFile;}
HANDLE GetMapHandle(void){return this->hMapFile;}
//---------------------------------------------------------
static bool IsMappingExist(LPSTR MapName)
{
 char FullPath[256];
#ifndef _NTDLL_H
 ULONG Len = GetObjNamespaceStr(FullPath); 
#else
 ULONG Len = 0;
#endif
 Len += CopyString(&FullPath[Len], MapName); 
#ifdef _NTDLL_H
 HANDLE hMap = NULL;
 if(OpenMemSection(&hMap, FILE_MAP_ALL_ACCESS, FullPath, ObjDirName) < 0)return false;     // Doesn`t exist or access denied
#else
 HANDLE hMap = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, FullPath);
 if(!hMap)return false;     // Doesn`t exist or access denied
#endif
 CloseOHandle(hMap);
 return true;
}
//---------------------------------------------------------
static UINT64 GetMappingSize(LPSTR MapName)
{
 char FullPath[256];
 UINT64 Size = 0;
#ifndef _NTDLL_H
 ULONG Len = GetObjNamespaceStr(FullPath); 
#else
 ULONG Len = 0;
#endif
 Len += CopyString(&FullPath[Len], MapName); 
#ifdef _NTDLL_H
 HANDLE hMap = NULL;
 if(OpenMemSection(&hMap, FILE_MAP_ALL_ACCESS, FullPath, ObjDirName) < 0)return 0;     // Doesn`t exist or access denied
#else
 HANDLE hMap = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, FullPath);
 if(!hMap)return 0;     // Doesn`t exist or access denied
#endif
#ifdef _NTDLL_H
 SECTION_BASIC_INFORMATION SectionInfo; // = { 0 };
 NTSTATUS res = NtQuerySection(hMap, SectionBasicInformation, &SectionInfo, sizeof(SectionInfo), 0);
 if(!res)Size = SectionInfo.MaximumSize.QuadPart;
#else
// No documented way to get mapping size? 
#endif
 CloseOHandle(hMap);
 return Size;
}
//---------------------------------------------------------
int Connect(LPSTR MapName, SIZE_T SizeOfNew)    // Creates or opens a Shared Memory 
{
 BYTE TmpBuf[MaxNameSize];
 if(this->hMapFile){if(this->Disconnect() < 0){DBGMSG("Failed to disconnect!"); return -1;}}
 SizeOfNew = AlignFrwd(SizeOfNew,8);         
 MakeObjName(0, MapName, (LPSTR)&TmpBuf);
 NTSTATUS status = CreateNtObjDirectory(ObjDirName, &this->hDirObj);      // Create objects directory with NULL security
 if(status < 0){DBGMSG("CreateNtObjDirectory failed(%08X)", status); return -2;} 

/*SECURITY_ATTRIBUTES security;
ZeroMemory(&security, sizeof(security));
security.nLength = sizeof(security);
ConvertStringSecurityDescriptorToSecurityDescriptor(
         L"D:P(A;OICI;GA;;;SY)(A;OICI;GA;;;BA)(A;OICI;GWGR;;;IU)",
         1,
         &security.lpSecurityDescriptor,
         NULL);
  */
#ifdef _NTDLL_H
 LARGE_INTEGER MaxSize;
 MaxSize.QuadPart = SizeOfNew;  
 status = CreateMemSection(&this->hMapFile, SECTION_ALL_ACCESS, &MaxSize, PAGE_READWRITE, SEC_COMMIT, (LPSTR)&TmpBuf, ObjDirName);
 if(status < 0){DBGMSG("CreateMapping failed(%08X): Size=%08X", status, SizeOfNew); return -3;}   // HANDLE hSec = CreateFileMappingW(INVALID_HANDLE_VALUE,NULL,PAGE_EXECUTE_READWRITE|SEC_COMMIT,0,ModSize,NULL);
 bool CreatedNew = !(STATUS_OBJECT_NAME_EXISTS == status);
 PVOID BaseAddr  = NULL;
 status = NtMapViewOfSection(this->hMapFile,NtCurrentProcess,&BaseAddr,0,SizeOfNew,NULL,&SizeOfNew,ViewShare,0,PAGE_READWRITE);
 if(status){DBGMSG("Failed to map memory: %08X", status); CloseOHandle(this->hMapFile); this->hMapFile = NULL; return -4;}
 this->MemDesc   = (SMemDescr*)BaseAddr;
#else
 this->hMapFile   = CreateFileMappingA(INVALID_HANDLE_VALUE,NULL,PAGE_READWRITE,0,SizeOfNew,(LPSTR)&TmpBuf);
//LocalFree(securityDescriptor.lpSecurityDescriptor);
 if(!this->hMapFile){DBGMSG("CreateMapping failed(%u): Size=%08X", GetLastError(), SizeOfNew); return -5;}
 bool CreatedNew  = !(GetLastError() == ERROR_ALREADY_EXISTS);
 this->MemDesc    = (SMemDescr*)MapViewOfFile(this->hMapFile,FILE_MAP_ALL_ACCESS,0,0,SizeOfNew);  // Real memory pages will be allocated on first access?
 if(!this->MemDesc){DBGMSG("Failed to map memory: %u", GetLastError()); CloseOHandle(this->hMapFile); this->hMapFile = NULL; return -6;}
#endif
 if(CreatedNew)
  {
   DBGMSG("Created: Initializing IPC header at %p",this->MemDesc);
//   memset(this->MemDesc,0,sizeof(SMemDescr)+sizeof(PVOID));
   this->MemDesc->MemSize = SizeOfNew;
   MakeObjName((ULONG_PTR)this->hMapFile, "S", (LPSTR)&this->MemDesc->SynMName);    // Create a Mutex name
   MakeObjName((ULONG_PTR)this->hMapFile, "N", (LPSTR)&this->MemDesc->NtfEName);  
  }
#ifdef _NTDLL_H
 NShMem::MutexCreateA(&this->hSyncMutex, NULL, FALSE, (LPSTR)&this->MemDesc->SynMName, ObjDirName);  
 NShMem::EventCreateA(&this->hNotifyEvt, NULL, TRUE, FALSE, (LPSTR)&this->MemDesc->NtfEName, ObjDirName);
#else  
 this->hSyncMutex = CreateMutexA(NULL, FALSE, (LPSTR)&this->MemDesc->SynMName);     // Take the names from shared memory 
 this->hNotifyEvt = CreateEventA(NULL, TRUE, FALSE, (LPSTR)&this->MemDesc->NtfEName); 
#endif
 if(!this->hSyncMutex || !this->hNotifyEvt){DBGMSG("Failed to create sync objects: hSyncMutex=%p(%s), hNotifyEvt=%p(%s)", this->hSyncMutex, &this->MemDesc->SynMName, this->hNotifyEvt, &this->MemDesc->NtfEName); this->Disconnect(); return -7;}
 DBGMSG("CreatedNew=%u, SizeOfNew=%08X, MemDesc=%p, MMapName='%s', SynMName='%s', NtfEName='%s'", CreatedNew, SizeOfNew, this->MemDesc, &TmpBuf, &this->MemDesc->SynMName, &this->MemDesc->NtfEName);
 return !CreatedNew;  // Created a new shared buffer
}
//---------------------------------------------------------
int Disconnect(void)
{
 if(!this->IsConnected())return 0;  // Not connected
 DBGMSG("Disconnecting...");
 this->LockBuffer(9000);       // Try to Lock but allow Disconnect in case someone else is hang up
 HANDLE hMap = this->hMapFile;
 this->hMapFile = NULL;
 if(!this->MemDesc)return -2;
 if(NTSTATUS stat = NtUnmapViewOfSection(NtCurrentProcess, this->MemDesc)){DBGMSG("Failed to unmap memory: %08X", stat); return -3;}
 DBGMSG("MemDesc=%p",this->MemDesc);
 this->MemDesc   = NULL;
 CloseOHandle(hMap); 
 if(this->hNotifyEvt)CloseOHandle(this->hNotifyEvt);
 this->hNotifyEvt = NULL;
 this->UnlockBuffer();
 if(this->hSyncMutex)CloseOHandle(this->hSyncMutex);
 this->hSyncMutex = NULL; 
 if(this->hDirObj)CloseOHandle(this->hDirObj);
 return 0;
}
//---------------------------------------------------------
bool LockBuffer(UINT WaitDelay=5000)
{
// DBGMSG("<<<<<<<<<<<<<<<<: %u",WaitDelay);
// DWORD val = GetTicksCount();
 bool  res = (NShMem::WaitForSingle(this->hSyncMutex,WaitDelay) != WAIT_TIMEOUT);
// DBGMSG("<<<<<<<<<<<<<<<<: %u = %u",res,GetTicksCount()-val);
 if(res && !this->IsConnected()){this->UnlockBuffer(); return false;}
 return res;
}
//---------------------------------------------------------
bool UnlockBuffer(void)
{
#ifdef _NTDLL_H
 UINT res = NtReleaseMutant(this->hSyncMutex, 0) >= 0;
#else
 UINT res = ReleaseMutex(this->hSyncMutex);
#endif 
// DBGMSG(">>>>>>>>>>>>>>>>: %u",res);
 return res;
}
//---------------------------------------------------------
PBYTE BufferPtr(void)
{
 if(!this->IsConnected())return NULL;
 return (PBYTE)&this->MemDesc->Data;
}
//---------------------------------------------------------
UINT BufferSize(void)
{
 if(!this->IsConnected())return 0;
 return (this->MemDesc->MemSize - sizeof(SMemDescr));
}
//---------------------------------------------------------
Usr* UserData(void){return this->MemDesc;}
//---------------------------------------------------------
bool NotifyChange(void)
{
#ifdef _NTDLL_H
 return NtSetEvent(this->hNotifyEvt, 0) >= 0;
#else
 return SetEvent(this->hNotifyEvt);
#endif
}
//---------------------------------------------------------
bool ResetChange(void)
{
#ifdef _NTDLL_H
 return NtClearEvent(this->hNotifyEvt) >= 0;
#else
 return ResetEvent(this->hNotifyEvt);  //  At this point all waiting threads are got the event. They all will call this ResetEvent (That`s not a problem?)  
#endif
}
//---------------------------------------------------------
// You must check for a new messages before calling this function because some other thread may already reset the Notify Event after a change
//
bool WaitForChange(UINT WaitDelay=1000)
{
// DBGMSG("++++++++++++++++: %u",WaitDelay);
// DWORD val = GetTickCount();
 bool res = (NShMem::WaitForSingle(this->hNotifyEvt,WaitDelay) != WAIT_TIMEOUT);
// DBGMSG("++++++++++++++++: %u = %u",res,GetTickCount()-val);
 return res;
}
//---------------------------------------------------------

};
//===========================================================================
// Multi Producer, Multi Consumer
//
// Limitations:
//  A data block can`t be split so it will be written at beginning of buffer destroying blocks(and leaving that memory unused until an another block may reclaim it) at its end (where it initially supposed to be written) to keep the data stream circular.
// 
//===========================================================================
class CSharedIPC 
{
static const int WaitChangeDelMs = 100;
#pragma pack(push,1)
public:
struct SMsgBlk     // Size is 32 + Data + ValidMrk2    // Always aligned to 16 bytes  
{
 volatile UINT32 DataSize;    
 volatile UINT32 ViewCntr;   
 volatile UINT32 PrevOffs; 
 volatile UINT32 NextOffs;   // From beginning of buffer  // There may be gaps between messages after a wrap overwrites some of them
 volatile UINT32 TargetID;
 volatile UINT32 SenderID;
 volatile UINT32 MsgSeqID;   // Incremented for each message
 volatile UINT32 ValidMrk;   // A Opening marker. An Closing marker is after the data  
 volatile BYTE   Data[0];    // Better to be aligned to 8 bytes

 static UINT32 FullSize(UINT32 DSize){
   return AlignFrwd(DSize + sizeof(SMsgBlk) + sizeof(UINT32), 16);}    // After Hdr+Data, at aligned end is UINT32(~ValidMrk)  // Align 16 (SSE compatible)
 UINT32 FullSize(void){return FullSize(this->DataSize);}   // AlignFrwd(this->DataSize + sizeof(SMsgBlk) + 8,8);}     // All data blocks aligned to 8 bytes    
 bool  IsBroadcast(void){return !this->TargetID;}
};

private:
struct SDescr   // No message wrapping is supported(), if it is not fits then RD and WR pointers are get updated to beginning of the buffer
{
 enum EFlags {flEmpty,flUsed=1};
 volatile UINT32 Flags;      // 0 if buffer is empty    // MessageCtr is useless and too costly to maintain
 volatile UINT32 NxtMsgID;   // Next MessageID to be used (Used by each instance to exclude a viewed messages from enumeration)  // What will happen after overflow?
 volatile UINT32 FirstBlk;   // Points to oldest available message
 volatile UINT32 LastBlk;    // Points last added message  
};
#pragma pack(pop)

 UINT32   SyncDelay;
 UINT32   InstanceID;
 SMsgBlk* NewMsg;   // Temporary ptr, protected by Lock
 CSharedMem<SDescr> MBuf;

//---------------------------------------------------------------------------
bool IsValidHdr(SMsgBlk* Blk)
{
 PBYTE Ptr = this->MBuf.BufferPtr();
 PBYTE End = &Ptr[this->MBuf.BufferSize()];
 PBYTE Msg = (PBYTE)Blk;                 
 if((Msg < Ptr)||(Msg >= End)){DBGMSG("MsgBegOutside"); return false;}    // Begin is not inside the shared buffer
 if((&Msg[sizeof(SMsgBlk)] < Ptr)||(&Msg[sizeof(SMsgBlk)] > End)){DBGMSG("HdrOutside"); return false;}    // Hdr is not inside the shared buffer
 ULONG Len = Blk->FullSize();
 if((&Msg[Len] < Ptr)||(&Msg[Len] > End)){DBGMSG("MsgEndOutside"); return false;}    // End is not inside the shared buffer
 if(!Blk->SenderID){DBGMSG("Unfinished"); return false;}   // Not finished yet       
 bool res = !(~(*(UINT32*)&Msg[Len-sizeof(UINT32)]) ^ Blk->ValidMrk);  
 if(!res){DBGMSG("OutOfSync: %08X, %08X",Msg-Ptr,Len); return false;}       // Validation markers do not match    // Happens if doing sequential add-remove of pairs Req/Rsp of different size
 return res;
}
//---------------------------------------------------------------------------
void SetValidHdr(SMsgBlk* Blk)
{
// DBGMSG("Blk: %p",Blk);
 ULONG Len = Blk->FullSize();
 Blk->ValidMrk = (Blk->DataSize ^ Blk->TargetID ^ Blk->MsgSeqID);       // Checksum of a fields that will stay unmodified
 *(UINT32*)&((PBYTE)Blk)[Len-sizeof(UINT32)] = ~Blk->ValidMrk;
}
//---------------------------------------------------------------------------
void InvalidateHdr(SMsgBlk* Blk)
{
// DBGMSG("Blk: %p",Blk);
 ULONG Len = Blk->FullSize();    
 *(UINT32*)&((PBYTE)Blk)[Len-sizeof(UINT32)] = Blk->ValidMrk = 0;
}
//---------------------------------------------------------------------------

public:
static const int MSG_BROADCAST = 0;                                 
enum MEFlags {mfNone,mfLocked=0x02,mfAnyTgt=0x04,mfOwnMsg=0x08,mfNoLock=0x10,mfHaveNxtMsg=0x20,mfHaveLstMsg=0x40,mfEnumActive=0x80}; 
//---------------------------------------------------------------------------
struct SEnumCtx
{
 UINT32 Flags;
 UINT32 TgtID;
 UINT32 SndID;
 UINT32 NxtOffs;
 UINT32 LstOffs;
 UINT32 NxtMsgID;
 UINT32 LstMsgID;    // Last Found message ID
 UINT32 MaxViewCtr;
 SEnumCtx(void){this->Reset();}
 void Reset(void){TgtID=SndID=NxtMsgID=LstMsgID=Flags=0; MaxViewCtr=NxtOffs=LstOffs=-1;}
};
//---------------------------------------------------------------------------
CSharedIPC(UINT SDel=5000)
{
 this->SyncDelay  = SDel;
 this->InstanceID = ((SIZE_T)this + GetTicksCount())|1;  // Must never be 0   // Reinit after Disconnect?
}
//---------------------------------------------------------------------------
~CSharedIPC(){}
//---------------------------------------------------------------------------
bool   IsConnected(void){return this->MBuf.IsConnected();}
void   SetSyncDelay(UINT Delay){this->SyncDelay = Delay;}
HANDLE GetMapHandle(void){return this->MBuf.GetMapHandle();}
//---------------------------------------------------------------------------
bool IsEmpty(void)
{
 if(!this->IsConnected())return true;
 return !(this->MBuf.UserData()->Flags & SDescr::flUsed);
}
//---------------------------------------------------------------------------
SMsgBlk* GetFirstBlk(void)     // Oldest
{
 if(this->IsEmpty())return NULL;
 return (SMsgBlk*)&this->MBuf.BufferPtr()[this->MBuf.UserData()->FirstBlk];   // if(!this->IsValidHdr(CurMsg))return NULL;    ????????????
}
//---------------------------------------------------------------------------
SMsgBlk* GetLastBlk(void)      // Most recent (Backward enumeration) 
{
 if(this->IsEmpty())return NULL;
 return (SMsgBlk*)&this->MBuf.BufferPtr()[this->MBuf.UserData()->LastBlk];
}
//---------------------------------------------------------------------------
SMsgBlk* GetPrevBlk(SMsgBlk* Blk)     // In memory order                
{
 if(this->IsEmpty() || (Blk->PrevOffs == (UINT32)-1))return NULL;
 return (SMsgBlk*)&this->MBuf.BufferPtr()[Blk->PrevOffs];
}
//---------------------------------------------------------------------------
SMsgBlk* GetNextBlk(SMsgBlk* Blk)     // In memory order        
{
 if(this->IsEmpty() || (Blk->NextOffs == (UINT32)-1))return NULL;
 return (SMsgBlk*)&this->MBuf.BufferPtr()[Blk->NextOffs];
}
//---------------------------------------------------------------------------
int Connect(LPSTR QueueName, UINT QueueSize)
{
 int res = this->MBuf.Connect(QueueName, QueueSize);
 DBGMSG("Status=%i, InstanceID=%08X, Size=%08X, Name='%s'",res,this->InstanceID,QueueSize,QueueName);
 return res;   
}
//---------------------------------------------------------------------------
int Disconnect(void)
{
 DBGMSG("InstanceID=%08X",this->InstanceID);    // Always log 'this' or InstanceID?  
 return this->MBuf.Disconnect();
}
//---------------------------------------------------------------------------
bool Clear(void)         // Don`t forget to keep NxtMsgID !
{
 if(!this->IsConnected() || !this->MBuf.LockBuffer(this->SyncDelay))return false;
 SDescr* Desc = this->MBuf.UserData();
 if(Desc)
  {
   Desc->Flags = SDescr::flEmpty;
   Desc->FirstBlk = Desc->LastBlk = 0;  
   DBGMSG("Done");
  }
 this->MBuf.UnlockBuffer();
 return true;
}
//---------------------------------------------------------------------------
static SMsgBlk* DataPtrToMsgHdr(PBYTE Data){return (SMsgBlk*)&Data[-sizeof(SMsgBlk)];}
UINT32 GetNexMsgSeqNum(void){return (this->IsConnected())?(this->MBuf.UserData()->NxtMsgID):(-1);}
//---------------------------------------------------------------------------
// TODO: Enumeration in different processes should not block each other? (But enumeration definently should block any manipulation(SRW lock))
PBYTE EnumFirst(SEnumCtx* Ctx, PUINT Size)    // For This InstanceID or broadcast  // Peek - do not remove messages; Any - Get messages for any InstanceID
{
// DBGMSG("InstanceID=%08X, Flags=%08X",this->InstanceID,Ctx->Flags);
 if(this->IsEmpty() || (Ctx->Flags & mfEnumActive))return NULL;   // No messages yet   // Checks connection first
 SDescr* Desc = this->MBuf.UserData();     
 if(!(Ctx->Flags & mfNoLock))
  {
   if(!this->MBuf.LockBuffer(this->SyncDelay)){DBGMSG("Failed to lock!"); return NULL;}
     else Ctx->Flags |= mfLocked;
  }
 if(!Ctx->TgtID && !(Ctx->Flags & mfAnyTgt))Ctx->TgtID = this->InstanceID;     // Target this instance by default    // TODO: Flag to receive only targeted and broadcast messages 
// DBGMSG("Flags=%08X, NxtOffs=%08X, NxtMsgID=%08X, LstOffs=%08X, LstMsgID=%08X",Ctx->Flags,Ctx->NxtOffs,Ctx->NxtMsgID,Ctx->LstOffs,Ctx->LstMsgID);
 if(Ctx->Flags & mfHaveLstMsg)  // Higher priority // LstMsgID and LstOffs must be valid   // A message with LstMsgID may be a response and may be removed by a receiver
  {
//   DBGMSG("Beg mfHaveLstMsg: NxtOffs=%08X, LstOffs=%08X, LstMsgID=%08X",Ctx->NxtOffs,Ctx->LstOffs,Ctx->LstMsgID);
   Ctx->NxtOffs = Desc->FirstBlk;        // Start from beginning of the buffer (Default)
   if(Ctx->LstOffs != (UINT32)-1)     // Offset is already specified
    {
     SMsgBlk* CurMsg = (SMsgBlk*)&this->MBuf.BufferPtr()[Ctx->LstOffs];
     if(!this->IsValidHdr(CurMsg) || (Ctx->LstMsgID != CurMsg->MsgSeqID))Ctx->LstOffs = -1;  // An invalid offset or a different massage at it
    }
   if(Ctx->LstOffs == (UINT32)-1)  // Try to continue with ID after LstMsgID
    {
     UINT32 LstID = Ctx->NxtMsgID;
     if(Ctx->LstMsgID > (LstID-1))  // MsgID has been overflowed  // TODO: TEST IT !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
      {
       bool OPoint = false;
       SMsgBlk* CurMsg = NULL;
       UINT32 Offs = Desc->LastBlk;                       
       while(Offs != (UINT32)-1)  // Find an overflow point [56789-OP-01234 <<]  // 9 is OP here    
        {
         CurMsg = (SMsgBlk*)&this->MBuf.BufferPtr()[Offs];
         if(!this->IsValidHdr(CurMsg)){DBGMSG("Corrupted1: Removing mfHaveLstMsg"); Ctx->Flags &= ~mfHaveLstMsg; return NULL;}    // Corrupted buffer! 
         if(LstID <= CurMsg->MsgSeqID){OPoint = true; break;}  // Overflow point  
         Offs  = CurMsg->PrevOffs;      // Enumerating backwards
         LstID = CurMsg->MsgSeqID;
        }
       if(OPoint && CurMsg)   // If false then starting from beginning(Entire buffer has changed)
        {
         for(;;) 
          {
           if(Ctx->LstMsgID >= CurMsg->MsgSeqID){Ctx->NxtOffs = CurMsg->NextOffs; break;} // Found with a lesser or equal ID (Continue from a next message)  
           Offs = CurMsg->PrevOffs;      // Enumerating backwards
           if(Offs == (UINT32)-1)break;   // No more messages - enumerate from beginning
           CurMsg = (SMsgBlk*)&this->MBuf.BufferPtr()[Offs];
           if(!this->IsValidHdr(CurMsg)){DBGMSG("Corrupted2: Removing mfHaveLstMsg"); Ctx->Flags &= ~mfHaveLstMsg; return NULL;}    // Corrupted buffer!                         
          }
        }
      }
       else   // Not overflowed or overflowed too far with Message IDs
        {
         for(UINT32 Offs = Desc->LastBlk;Offs != (UINT32)-1;)     // Find a message offset by ID     
          {
           SMsgBlk* CurMsg = (SMsgBlk*)&this->MBuf.BufferPtr()[Offs];
           if(!this->IsValidHdr(CurMsg)){DBGMSG("Corrupted3: Removing mfHaveLstMsg"); Ctx->Flags &= ~mfHaveLstMsg; return NULL;}    // Corrupted buffer!  
           if(Ctx->LstMsgID >= CurMsg->MsgSeqID){Ctx->NxtOffs = CurMsg->NextOffs; break;} // Found with a lesser or equal ID (Continue from a next message)  
           Offs = CurMsg->PrevOffs;      // Enumerating backwards
          }
        }
    }
     else
      {
       SMsgBlk* CurMsg = (SMsgBlk*)&this->MBuf.BufferPtr()[Ctx->LstOffs]; // LstOffs is already checked
       Ctx->NxtOffs = CurMsg->NextOffs;
      }
//   DBGMSG("End mfHaveLstMsg: NxtOffs=%08X, LstOffs=%08X, LstMsgID=%08X",Ctx->NxtOffs,Ctx->LstOffs,Ctx->LstMsgID);
  }
 else if(Ctx->Flags & mfHaveNxtMsg)   // Search for exact match (Fail enumeration if not found)
  {
   if(Ctx->NxtOffs != (UINT32)-1)     // Offset is already specified
    {
     SMsgBlk* CurMsg = (SMsgBlk*)&this->MBuf.BufferPtr()[Ctx->NxtOffs];
     if(!this->IsValidHdr(CurMsg) || (Ctx->NxtMsgID != CurMsg->MsgSeqID))Ctx->NxtOffs = -1;  // An invalid offset or a different massage at it
    }
   if(Ctx->NxtOffs == (UINT32)-1) 
    {
     for(UINT32 Offs = Desc->LastBlk;Offs != (UINT32)-1;)     // Find a message offset by ID     
      {
       SMsgBlk* CurMsg = (SMsgBlk*)&this->MBuf.BufferPtr()[Offs];
       if(!this->IsValidHdr(CurMsg)){DBGMSG("Corrupted: Removing mfHaveNxtMsg"); Ctx->Flags &= ~mfHaveNxtMsg; return NULL;}    // Corrupted buffer!  
       if(Ctx->NxtMsgID == CurMsg->MsgSeqID){Ctx->NxtOffs = Offs; break;}  // Found exact match! 
       Offs = CurMsg->PrevOffs;      // Enumerating backwards
      }
    }
  }
   else Ctx->NxtOffs = Desc->FirstBlk;        // Start from beginning of the buffer (Default)

 if(Ctx->NxtOffs != (UINT32)-1)    // NxtOffs is for a first message
  { 
   Ctx->Flags   |= mfEnumActive;
   Ctx->NxtMsgID = ((SMsgBlk*)&this->MBuf.BufferPtr()[Ctx->NxtOffs])->MsgSeqID;
//   DBGMSG("From: Ctx=%p, NxtOffs=%08X, NxtMsgID=%08X, LstMsgID=%08X",Ctx,Ctx->NxtOffs,Ctx->NxtMsgID,Ctx->LstMsgID);
   if(PBYTE Blk  = this->EnumNext(Ctx, Size))return Blk;
  }
// DBGMSG("Exiting!"); 
 if(!(Ctx->Flags & mfNoLock))this->MBuf.UnlockBuffer(); 
 return NULL;
}
//---------------------------------------------------------------------------
// No viwed messages removal supported. We cannot remove a targeted messages and leave a broadcast messages because this will fragment the buffer 
//
PBYTE EnumNext(SEnumCtx* Ctx, PUINT Size)
{
 if(!(Ctx->Flags & mfEnumActive) || this->IsEmpty())return NULL;    // Not possible
 SDescr* Desc = this->MBuf.UserData();
 while(Ctx->NxtOffs != (UINT32)-1)   // NOTE: The buffer will always end up being full of a viwed messages 
  {
   SMsgBlk* CurMsg = (SMsgBlk*)&this->MBuf.BufferPtr()[Ctx->NxtOffs];      // Without Lock this may become invalid because of some AddBlock
   if(!this->IsValidHdr(CurMsg) || (Ctx->NxtMsgID != CurMsg->MsgSeqID)){Ctx->NxtOffs = (UINT32)-1; break;}  // Check NxtMsgID in the loop in case that the buffer is not locked  // NxtOffs and NxtMsgID used to validate that NextMessage is same as was found previously 
   UINT32 CurrOffs = Ctx->NxtOffs;
   Ctx->NxtOffs    = CurMsg->NextOffs;    // Enumerate forward in the linked list of messages  // Save the offset that we can continue later     
   if(Ctx->NxtOffs != (UINT32)-1)
    {
     Ctx->NxtMsgID = ((SMsgBlk*)&this->MBuf.BufferPtr()[Ctx->NxtOffs])->MsgSeqID;
     Ctx->Flags   |= mfHaveNxtMsg;
    }
     else 
      {
       Ctx->NxtMsgID = -1;
       Ctx->Flags   &= ~mfHaveNxtMsg;
      }
//   DBGMSG("Next: Ctx=%p, NxtOffs=%08X, NxtMsgID=%08X, LstMsgID=%08X",Ctx,Ctx->NxtOffs,Ctx->NxtMsgID,Ctx->LstMsgID);
//   DBGMSG("Msg at %08X: Size=%08X(%08X), ViewCntr=%u, SenderID=%08X, TargetID=%08X, MsgSeqID=%08X",((PBYTE)CurMsg - this->MBuf.BufferPtr()),CurMsg->DataSize,SMsgBlk::FullSize(CurMsg->DataSize),CurMsg->ViewCntr,CurMsg->SenderID,CurMsg->TargetID,CurMsg->MsgSeqID);                                                               
   if((!Ctx->TgtID || CurMsg->IsBroadcast() || (CurMsg->TargetID == Ctx->TgtID)) && (CurMsg->ViewCntr <= Ctx->MaxViewCtr) && ((!Ctx->SndID || (CurMsg->SenderID == Ctx->SndID)) && ((Ctx->Flags & mfOwnMsg) || (CurMsg->SenderID != this->InstanceID))))  // NOTE: Headers are followed by a data and this approach is very cache unfriendly. But this way a maximum number of messages that the buffer can hold is not predefined and depends only on size of messages
    {
     if(Size)*Size = CurMsg->DataSize;
     Ctx->LstMsgID = CurMsg->MsgSeqID;
     Ctx->LstOffs  = CurrOffs;
     Ctx->Flags   |= mfHaveLstMsg;     // Do not read an already viwed messages when repeating enumeration with same context   // Allow this Flag to be forced here?  
     if(CurMsg->ViewCntr != (UINT32)-1)CurMsg->ViewCntr++;  // Do not overflow!
//     DBGMSG("Found at %08X: Size=%08X, ViewCntr=%u, SenderID=%08X, TargetID=%08X, MsgSeqID=%08X",((PBYTE)CurMsg - this->MBuf.BufferPtr()),CurMsg->DataSize,CurMsg->ViewCntr,CurMsg->SenderID,CurMsg->TargetID,CurMsg->MsgSeqID);   
     return (PBYTE)&CurMsg->Data;
    }      
  }
// DBGMSG("No more messages!");
 return NULL;      // No more messages    // If you try again to call NextBlock it will start from beginning
}
//---------------------------------------------------------------------------
bool EnumClose(SEnumCtx* Ctx)
{                    
// DBGMSG("Ctx=%p, Flags=%08X, Conn=%u, NxtOffs=%08X, NxtMsgID=%08X, LstMsgID=%08X",Ctx,Ctx->Flags,this->IsConnected(),Ctx->NxtOffs,Ctx->NxtMsgID,Ctx->LstMsgID);
 bool res = true;
 if(Ctx->Flags & mfLocked)
  {
   res = this->MBuf.UnlockBuffer();
   Ctx->Flags &= ~mfLocked;
  }
 if(Ctx->Flags & mfEnumActive)Ctx->Flags &= ~mfEnumActive;  
 if(!this->IsConnected()){DBGMSG("Not Active!"); return false;}
 // DBGMSG("Done!");   // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 return res;
}
//---------------------------------------------------------------------------
// Beware of oveflowing the buffer with this while in a middle of message enumeration(Without Locking)
// Example for blocks of same size:
// |A....| : |FL...| : |F-L..| : |F--L.| : |F---L|     // (F <= L): Here we can have a 'No Next' in last block
// |LF---| : |-LF--| : |--LF-| : |---LF| : |F---L|     // (F <= L): This rotation will continue until buffer destroyed
//
int AddBlock(UINT TgtID, PVOID* Data, UINT Size)          // TgtID = 0 - Broadcast message
{
 if(this->NewMsg || !this->IsConnected()){DBGMSG("Not ready yet!"); return -2;}
 if(Size > this->MBuf.BufferSize()){DBGMSG("Block too big: Size=%08X, BufferSize=%08X",Size,this->MBuf.BufferSize()); return -3;}      // Won`t fit even if will be the only message in buffer
 if(!this->MBuf.LockBuffer(this->SyncDelay)){DBGMSG("Failed to lock IPC buffer!"); return -4;}
 SDescr* Desc    = this->MBuf.UserData();
 SMsgBlk* LstMsg = NULL;
 SMsgBlk* FstMsg = NULL;
 UINT32 FullLen  = SMsgBlk::FullSize(Size);  
 UINT32 FMsg     = Desc->FirstBlk; 
 UINT32 LMsg     = Desc->LastBlk;
 if(!this->IsEmpty())
  {
   FstMsg = (SMsgBlk*)&this->MBuf.BufferPtr()[FMsg]; 
   LstMsg = (SMsgBlk*)&this->MBuf.BufferPtr()[LMsg];    // Last valid message written
   if(this->IsValidHdr(FstMsg) && this->IsValidHdr(LstMsg))
    {
     bool MStraight = (FMsg <= LMsg);
     LMsg += LstMsg->FullSize();   // Now LMsg will point to free space for a new message
     if((LMsg + FullLen) >= this->MBuf.BufferSize()){LMsg = 0; MStraight = false;}   // Will not fit in rest of buffer - start from beginning   // At 0 there is always a valid message or else the buffer is empty  
     if(!MStraight)    // |LF---| : |-LF--| : |--LF-| : |---LF|
      {
       UINT32 EndOffs = LMsg + FullLen;    // End offset of a new message   
       if((Desc->LastBlk >= LMsg)&&(EndOffs > Desc->LastBlk)){DBGMSG("DelPrev: EndOffs=%08X, Desc:LastBlk=%08X",EndOffs, Desc->LastBlk); LstMsg = NULL;}  // Will be overwritten
       for(SMsgBlk* FirstMsg = (SMsgBlk*)&this->MBuf.BufferPtr()[FMsg];EndOffs > FMsg;FirstMsg = (SMsgBlk*)&this->MBuf.BufferPtr()[FMsg])     // Move forward FirstBlk
        {
//         DBGMSG("Overwriting: FirstBlk=%08X, LastBlk=%08X, FullLen=%08X, Size=%08X, ViewCntr=%u, NextOffs=%08X, SeqID=%08X, SenderID=%08X, TargetID=%08X",FMsg, LMsg, FullLen, FirstMsg->DataSize, FirstMsg->ViewCntr, FirstMsg->NextOffs,  FirstMsg->MsgSeqID, FirstMsg->SenderID, FirstMsg->TargetID);      
         if(!this->IsValidHdr(FirstMsg)||(FirstMsg->NextOffs <= FMsg)||(FirstMsg->NextOffs == (UINT32)-1))      // Rest of messages is probably corrupted as well / Loop / no next message
          {
           DBGMSG("ResetBuf: FMsg=%08X, LMsg=%08X",FMsg, LMsg);
           FMsg = 0;         // Only messages at beginning of buffer is considered valid now  // New message will taaake as much memory as needed at rest of the buffer(We already checked that it will fit there)
           break;
          }
         FMsg = FirstMsg->NextOffs; 
        }
      }
    }
     else   // Someone corrupted one of msg headers?
      {
       DBGMSG("Corrupted: FMsg=%08X, LMsg=%08X",FMsg, LMsg); 
       Desc->Flags = SDescr::flEmpty;
       FMsg = LMsg = 0;     // Make it empty
       LstMsg = NULL;
      }
   ((SMsgBlk*)&this->MBuf.BufferPtr()[FMsg])->PrevOffs = -1;   // Update NoPrev for a first message  
   if(FMsg == LMsg)LstMsg = NULL;      // Only one message will be
  }
// DBGMSG("NewMsg: Offset=%08X, FullLen=%08X, MsgSeqID=%08X",LMsg, FullLen, Desc->NxtMsgID); 
 this->NewMsg = (SMsgBlk*)&this->MBuf.BufferPtr()[LMsg];   // LastBlk must point to a last added message
 this->NewMsg->MsgSeqID = Desc->NxtMsgID;       
 this->NewMsg->DataSize = Size;  // Viewed = 0 (High 4 bits)
 this->NewMsg->NextOffs = -1;    // New message is a last message
 this->NewMsg->ViewCntr = 0;
 this->NewMsg->SenderID = 0;     // Set to 0 to not allow it be enumerated until CloseBlock is called
 this->NewMsg->TargetID = TgtID;
 if(LstMsg)this->NewMsg->PrevOffs = ((PBYTE)LstMsg - this->MBuf.BufferPtr()); 
   else this->NewMsg->PrevOffs = -1;
// DBGMSG("Adding: FirstBlk=%08X, LastBlk=%08X, Size=%08X, Sender=%08X, Target=%08X, MsgID=%08X, PrevOffs=%08X",FMsg,LMsg,FullLen,this->InstanceID,TgtID,Desc->NxtMsgID,this->NewMsg->PrevOffs);    // TODO: For Full debug log level
 this->SetValidHdr(this->NewMsg);    // Must be done before modification of FirstBlk and LastBlk (Not Synced msg enumeration)
 Desc->FirstBlk = FMsg;      
 Desc->LastBlk  = LMsg;      
 if(LstMsg)LstMsg->NextOffs = LMsg;   // LstMsg is pointed by current LastBlk, Setting it before moving LastBlk forward will show in unlocked enumeration that the last message have a NEXT value  
 if(Data)*Data  = (PBYTE)&this->NewMsg->Data;
 return 0;
}
//---------------------------------------------------------------------------
bool CloseBlock(UINT32* MsgSeqNum)     // Call after AddBlock or Enumeration
{
// DBGMSG("InstanceID=%08X",this->InstanceID);
 if(!this->NewMsg || !this->IsConnected())return false;
 SDescr* Desc   = this->MBuf.UserData(); 
 if(MsgSeqNum)*MsgSeqNum = Desc->NxtMsgID;   // This message`s ID
 this->NewMsg->SenderID = this->InstanceID;  // Now it can be enumerated
 this->NewMsg   = NULL;
 Desc->Flags   |= SDescr::flUsed;      
 Desc->NxtMsgID++;   //_InterlockedIncrement(&Desc->NxtMsgID);        // Must be modified last - this will trigger any waiting on HashChange
 this->MBuf.NotifyChange();
 return this->MBuf.UnlockBuffer();
}
//---------------------------------------------------------------------------
// Should be called only for a locked buffer
ULONG RemoveLastBlocks(ULONG BlkCnt, bool KeepBroadcast=false)   // TODO: Never remove a BROADCAST messages(Optional?)!
{
 if(this->IsEmpty())return 0;
 ULONG RemTotal = 0; 
 SDescr* Desc = this->MBuf.UserData();
 UINT32  Offs = Desc->LastBlk;
 while(RemTotal < BlkCnt)        
  {
   SMsgBlk* CurMsg = (SMsgBlk*)&this->MBuf.BufferPtr()[Offs];
   if(!this->IsValidHdr(CurMsg))break;   // For 'Desc->LastBlk' 0 is valid 
//   DBGMSG("Removing Msg at %08X, NextOffs=%08X, Size=%08X, MsgID=%08X",Offs,CurMsg->NextOffs,CurMsg->FullSize(), CurMsg->MsgSeqID); 
   Offs = CurMsg->PrevOffs;      // Enumerating backwards
   RemTotal++;
   this->InvalidateHdr(CurMsg);
   if(Offs == (UINT32)-1)  // No more messages in the buffer
    {
     Desc->Flags    = SDescr::flEmpty;    // No messages left
     Desc->FirstBlk = Desc->LastBlk = 0;
     DBGMSG("Emptied!"); 
     return RemTotal;
    }
  }
 if(!RemTotal)return 0;
 Desc->LastBlk = Offs;   // A Last messages removed 
 ((SMsgBlk*)&this->MBuf.BufferPtr()[Offs])->NextOffs = -1;  // No Next message
 return RemTotal;
}
//---------------------------------------------------------------------------
static DWORD TicksDelta(DWORD Initial)
{
 DWORD Curr = GetTicksCount();
 return (Curr > Initial)?(Curr-Initial):(Initial-Curr);
}
//---------------------------------------------------------------------------
bool IsChanged(UINT32* LstChngHash)
{
 if(!this->IsConnected())return false;
 SDescr* Desc   = this->MBuf.UserData();       // Shared access is not important here
 UINT32 NewHash = Desc->NxtMsgID;              // Test only one field
 bool res = (NewHash != *LstChngHash); 
//   if(res){ DBGMSG("NewHash=%u, LstChngHash=%u",NewHash,*LstChngHash); }   // <<<<<<<<<<<<<<<<<<<<<<<<<<<<
 *LstChngHash = NewHash;
 return res;
}
//---------------------------------------------------------------------------
bool WaitForChange(UINT32* LstChngHash=NULL, UINT WaitDelay=5000)    // After this you can start enumerating for messages     // TODO: Use WaitOnAddress if available!
{
 if(!this->IsConnected())return false; 
 for(DWORD InitTick = GetTicksCount();TicksDelta(InitTick) <= WaitDelay;)
  {
   if(LstChngHash && this->IsChanged(LstChngHash))return true;     // Someone changed the queue
   if(this->MBuf.WaitForChange(WaitChangeDelMs))return true;       // A notification frome someone
  }
 return false; 
}
//---------------------------------------------------------------------------
bool ResetChange(void)
{
 return this->MBuf.ResetChange();    // Do not reset LstChngHash as it may be already changed 
}
//---------------------------------------------------------------------------
bool NotifyChange(void)
{
 return this->MBuf.NotifyChange();
}
//---------------------------------------------------------------------------

};
//===========================================================================
//
//
//
//---------------------------------------------------------------------------
class CMessageIPC
{ 
protected:
 LONG InstID;
 UINT DSeqNum;
 UINT WaitDelay;   
 CSharedIPC ipc;
 CCritSectEx<> csec;   // Prevents other threads from messing up 'ExchangeMsg'
//------------------------------------------------------------------------------------
static inline ULONG MakeIdStr(UINT InstanceID, char* StrOut)
{
 ULONG Index = 0;
 int   HLen  = 0;
 char HexBuf[64];
 StrOut[Index++] = 'I';
 char* ptr = NShMem::UIntToHexString(InstanceID, sizeof(UINT)*2, HexBuf, true, &HLen);
 Index += CopyString(&StrOut[Index], ptr, HLen); 
 StrOut[Index++] = 'S';
 ptr = UIntToString(sizeof(PVOID),HexBuf,&HLen);
 Index += CopyString(&StrOut[Index], ptr, HLen);    
 return Index;
}
//------------------------------------------------------------------------------------

public:
struct SMsgHdr      // Size 16
{
 UINT16 MsgType;    // EMsgType
 UINT16 MsgID;
 UINT32 DataID;     // In responce set it to 'Sequence' of request message
 UINT32 Sequence;   // A Seq number to detect a message loss
 UINT32 DataSize;   // 0 for mtEndStrm
 BYTE   Data[0];

 CSharedIPC::SMsgBlk* GetBlk(void){return (CSharedIPC::SMsgBlk*)&((PBYTE)this)[-sizeof(CSharedIPC::SMsgBlk)];}  // SMsgHdr is DATA of SMsgBlk
}; 

struct SMsgCtx
{
 bool   EnumMsg;
 UINT32 Change;
 SMsgHdr* Last;
 CSharedIPC::SEnumCtx BEnum;

 SMsgCtx(UINT Flags=0){this->Reset(Flags);}
 void Reset(UINT Flags=0){BEnum.Reset(); EnumMsg=false; Change=0; Last=NULL; BEnum.Flags=Flags;}
} MCtx;    // For global use
//------------------------------------------------------------------------------------

CMessageIPC(void)
{
 this->InstID    = -1;
 this->DSeqNum   = 0;
 this->WaitDelay = 5000;      // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
}
//------------------------------------------------------------------------------------
~CMessageIPC()
{
 this->Disconnect();
}
//------------------------------------------------------------------------------------
static bool IsExistForID(UINT InstanceID)
{
 char NameBuf[128];
 MakeIdStr(InstanceID, NameBuf);    
 return CSharedMem<>::IsMappingExist(NameBuf);
}
//------------------------------------------------------------------------------------
static UINT GetSizeForID(UINT InstanceID)
{
 char NameBuf[128];
 MakeIdStr(InstanceID, NameBuf);  
 return CSharedMem<>::GetMappingSize(NameBuf);
}
//------------------------------------------------------------------------------------
bool IsConnected(void)
{
 if(_InterlockedOr(&this->InstID, 0) == (LONG)-1)return false;
 return this->ipc.IsConnected();
}
//------------------------------------------------------------------------------------
void SetDelays(UINT WDelay, UINT SDelay){this->WaitDelay = WaitDelay; this->ipc.SetSyncDelay(SDelay);}
void Clear(void){this->ipc.Clear();}
UINT GetID(void){return this->InstID;}
//------------------------------------------------------------------------------------
int Connect(UINT InstanceID, UINT Size=0, bool DoClr=false)    // IPCSIZE
{
 char NameBuf[128];
 if(this->IsConnected())return 0;
 UINT XSize = GetSizeForID(InstanceID);
 if(XSize)Size = XSize;          // Always use size of an existing buffer
 MakeIdStr(InstanceID, NameBuf);      // System type must match 
 int res = this->ipc.Connect(NameBuf, Size); 
 if(DoClr)this->Clear();  // !!! Resets NxtMsgID !!!
 this->MCtx.Reset(); 
// if(res >= 0){this->PutMsg(mtInfo, miHello, 0, NULL, 0); this->InstID = InstanceID;}   // Notify clients 
 _InterlockedExchange(&this->InstID, InstanceID);
 return res;
}                                                               
//------------------------------------------------------------------------------------
int Disconnect(void)
{
 if(!this->IsConnected())return 0;
 _InterlockedExchange(&this->InstID, -1);  // this->InstID  = -1;
 this->csec.Lock();  // To prevent ExchangeMsg from waiting on a closed IPC
 this->DSeqNum = 0;
// this->PutMsg(mtInfo, miGoodby, 0, NULL, 0);
 int res = this->ipc.Disconnect();
 this->csec.Unlock();
 return res;
} 
//------------------------------------------------------------------------------------
void     EndMsg(bool TryRemoveLast=false){this->EndMsg(&this->MCtx,TryRemoveLast);}
SMsgHdr* GetMsg(void){return this->GetMsg(&this->MCtx);}
//------------------------------------------------------------------------------------
bool EndMsg(SMsgCtx* Ctx, bool TryDelLastReqRsp=false)    // Close message enumeration and unlocks the buffer  // NOTE: A targeted messages allowed to be removed
{
 if(!Ctx->EnumMsg)return false;
                           //   TryDelLastReqRsp = false;   // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
 if(TryDelLastReqRsp && Ctx->Last)  // Try to remove a last Request/Response pair to save the buffer space
  {
   if(CSharedIPC::SMsgBlk* LastBlk = this->ipc.GetLastBlk())     // Response
    {
     if(CSharedIPC::SMsgBlk*PrevBlk = this->ipc.GetPrevBlk(LastBlk))   // Request
      {
       SMsgHdr* LastMsg = (SMsgHdr*)&LastBlk->Data;
       SMsgHdr* PrevMsg = (SMsgHdr*)&PrevBlk->Data;
       if((LastMsg == Ctx->Last) && (LastBlk->MsgSeqID == Ctx->Last->GetBlk()->MsgSeqID))   // Last found for Response must be Last message in the buffer    // Address may be same but a message is different                                 
        {
//         DBGMSG("LastMsg(Rsp):(Size=%08X, MsgType=%u, MsgID=%u, DataID=%u, Sequence=%u), PrevMsg(Req):(Size=%08X, MsgType=%u, MsgID=%u, DataID=%u, Sequence=%u)",LastMsg->DataSize,LastMsg->MsgType,LastMsg->MsgID,LastMsg->DataID,LastMsg->Sequence,  PrevMsg->DataSize,PrevMsg->MsgType,PrevMsg->MsgID,PrevMsg->DataID,PrevMsg->Sequence); 
         if((LastMsg->DataID == PrevMsg->Sequence)&&(LastMsg->MsgType == (PrevMsg->MsgType << 1))&&(LastMsg->MsgID == PrevMsg->MsgID))  // Delete both Request and Response messages
          {
           this->ipc.RemoveLastBlocks(2);
          }
           else this->ipc.RemoveLastBlocks(1);    // Delete only Last(Response) message because some other massages between Request and Response
        }
      }
    }
  }
 Ctx->EnumMsg = false;      // Finish enumeration
 Ctx->Last=NULL; 
 bool res = this->ipc.EnumClose(&Ctx->BEnum); 
 this->csec.Unlock();
// DBGMSG("LeaveCriticalSection");
 return res;
}
//------------------------------------------------------------------------------------
SMsgHdr* GetMsg(SMsgCtx* Ctx)    // Will unlock the queue when there is no more messages   // Try to name it 'GetMessage' and you will get 'GetMessageA' thanks to macro definition 
{
 if(!this->IsConnected())return NULL; 
 if(!Ctx->EnumMsg)
  {
//   DBGMSG("TEST: WaitForChange %u",this->WaitDelay);
   if(!this->ipc.WaitForChange(&Ctx->Change, this->WaitDelay)){/*DBGMSG("TEST: Timeout: Ctx=%p",Ctx);*/ return NULL;}  // Timeout and no messages
//   DBGMSG("TEST: Change Detected");
//   DBGMSG("Before EnterCriticalSection");
   this->csec.Lock(); 
//   DBGMSG("After EnterCriticalSection");
//   DBGMSG("TEST: CtxTgtID=%08X, CtxSndID=%08X",Ctx->BEnum.TgtID,Ctx->BEnum.SndID);
   Ctx->Last    = (SMsgHdr*)this->ipc.EnumFirst(&Ctx->BEnum,NULL);
   Ctx->EnumMsg = true;    // Enumeration started
//   DBGMSG("TEST: FirstMsg=%p",Ctx->Last);
   this->ipc.ResetChange();
  }
   else Ctx->Last = (SMsgHdr*)this->ipc.EnumNext(&Ctx->BEnum,NULL);    // Continuing
// DBGMSG("Msg=%p, EnumMsg=%u, ENxtOffs=%08X, ENxtMsgID=%u, ELstMsgID=%u, EFlags=%08X",Ctx->Last,Ctx->EnumMsg,Ctx->BEnum.NxtOffs,Ctx->BEnum.NxtMsgID,Ctx->BEnum.LstMsgID,Ctx->BEnum.Flags);
 if(!Ctx->Last)this->EndMsg(Ctx);       // No more commands
 return Ctx->Last;
}
//------------------------------------------------------------------------------------
int BeginMsg(PVOID* MsgPtr, UINT16 MsgType, UINT16 MsgID, UINT32 DataID, UINT DataSize, UINT TgtID=CSharedIPC::MSG_BROADCAST)
{
 if(!this->IsConnected())return -1;
 this->csec.Lock();      
 SMsgHdr* hdr   = NULL;
 if(this->ipc.AddBlock(TgtID, (PVOID*)&hdr, DataSize+sizeof(SMsgHdr)) < 0){this->csec.Unlock(); return -2;}
 hdr->MsgType   = MsgType;
 hdr->MsgID     = MsgID;
 hdr->DataID    = DataID;
 hdr->Sequence  = this->DSeqNum++;   // A Seq number to detect a message loss
 hdr->DataSize  = DataSize;          // 0 for mtEndStrm
 if(MsgPtr)*MsgPtr = &hdr->Data;
 return 0;
}
//------------------------------------------------------------------------------------
int DoneMsg(UINT32* MsgSeqNum=NULL)
{
 if(!this->ipc.CloseBlock(MsgSeqNum)){this->csec.Unlock(); return -3;}
 this->csec.Unlock();
 return 0;
}
//------------------------------------------------------------------------------------
int PutMsg(UINT16 MsgType, UINT16 MsgID, UINT32 DataID, PVOID MsgData, UINT DataSize, UINT32* MsgSeqNum=NULL, UINT TgtID=CSharedIPC::MSG_BROADCAST)      // From clients to exact InstanceID (And optionally a stream) or broadcast
{
 PVOID MsgPtr = NULL;
 if(this->BeginMsg(&MsgPtr, MsgType, MsgID, DataID, DataSize, TgtID) < 0)return -5;
 if(MsgData && DataSize)memcpy(MsgPtr,MsgData,DataSize);
 return this->DoneMsg(MsgSeqNum);
}
//------------------------------------------------------------------------------------
// RspSize contains size of RspData buffer and later set to size of received data in that buffer
// Returns size of response
// While inside this function, a message enumeration in other threads must be stopped(This function doesn`t change last message index(mfPeek) but still uses it)
//
int ExchangeMsg(UINT16 MsgID, UINT16 MsgType, CGrowBuf* Req, CGrowBuf* Rsp, UINT TgtID=CSharedIPC::MSG_BROADCAST)    // TODO: Lock buffer and rmove Req/Rsp if no new messages added
{
// DBGMSG("Request: MsgID=%u, MsgType=%u, ReqSize=%08X",MsgID,MsgType,Req->GetLen());   // TODO: For Full debug log level
 if(!this->IsConnected()){DBGMSG("Not connected!"); return -1;}
 if(!this->IsOtherConnections()){DBGMSG("No other connections!"); this->Disconnect(); return -2;}
 this->csec.Lock();      
// DBGMSG("Entered!");
 SMsgCtx LMCtx;    
 LMCtx.BEnum.LstMsgID = this->ipc.GetNexMsgSeqNum() + 1;    // Limit enumeration to messages added after our request
 int res = this->PutMsg(MsgType, MsgID, 0, Req->GetPtr(), Req->GetLen(), &LMCtx.BEnum.LstMsgID, TgtID);   // NxtMsgID assigned ID of the Request message
 if(res < 0){DBGMSG("Failed: %i",res); this->csec.Unlock(); return res;}
 if(!Rsp){DBGMSG("Done, no response expected."); this->csec.Unlock(); return 0;}   // No response needed
 UINT16 ExpectMT = MsgType << 1;  // Special value! Should it be this way?
 DWORD  StTicks  = GetTicksCount(); 
 LMCtx.BEnum.Flags |= CSharedIPC::mfHaveLstMsg;  // Enumerate only messages AFTER LstMsgID
// DBGMSG("EnumMsg=%u, ENxtMsgID=%08X, ENxtOffs=%08X",LMCtx.EnumMsg,LMCtx.BEnum.NxtMsgID,LMCtx.BEnum.NxtOffs);
 for(SMsgHdr* Cmd = this->GetMsg(&LMCtx);Cmd || (CSharedIPC::TicksDelta(StTicks) < this->WaitDelay);Cmd = this->GetMsg(&LMCtx))   // DO NOT BREAK THIS LOOP by other means than 'meDone'!    // NOTE: Tick counter may overflow!!!
  {
   if(!Cmd)continue;
//   DBGMSG("MsgType=%04X, MsgID=%04X, DataID=%08X, Sequence=%08X, DataSize=%08X",Cmd->MsgType,Cmd->MsgID,Cmd->DataID,Cmd->Sequence,Cmd->DataSize);
   if((Cmd->MsgType != ExpectMT)||(Cmd->MsgID != MsgID)||(Cmd->DataID != (this->DSeqNum-1)))continue;
   Rsp->Assign(&Cmd->Data,Cmd->DataSize, true);
//   DBGMSG("Response: MsgID=%u, MsgType=%u, RspSize=%08X, RspWaitMs=%u",MsgID,MsgType,Rsp->GetLen(),GetTicksCount()-StTicks);     // TODO: For Full debug log level
   this->EndMsg(&LMCtx, true);      // Unlock buffer
   this->csec.Unlock();
   return Rsp->GetLen();
  }
 DBGMSG("No Response: MsgID=%u, MsgType=%u, ReqSize=%08X, RspWaitMs=%u",MsgID,MsgType,Req->GetLen(),GetTicksCount()-StTicks);
 this->csec.Unlock();
 return -5;    // No response
}
//------------------------------------------------------------------------------------
#ifdef _NTDLL_H
bool IsOtherConnections(void){return (this->GetConnectionsNumber() > 1);}
int  GetConnectionsNumber(void)  
{
 if(!this->IsConnected())return false;
 BYTE Buffer[sizeof(OBJECT_BASIC_INFORMATION)+512]; 
 ULONG RetLen = 0;      
 memset(&Buffer,0,sizeof(Buffer));
 NTSTATUS res = NtQueryObject(this->ipc.GetMapHandle(),ObjectBasicInformation,&Buffer,sizeof(OBJECT_BASIC_INFORMATION),&RetLen);  // It requires sizeof(OBJECT_BASIC_INFORMATION), not more or less. Why? What about a different Windows versions?  // Returns STATUS_INFO_LENGTH_MISMATCH if size is not sizeof(OBJECT_BASIC_INFORMATION)
 if(STATUS_SUCCESS != res)return true;   // Unknown, assume that it is
 return ((OBJECT_BASIC_INFORMATION*)&Buffer)->HandleCount;
}
//------------------------------------------------------------------------------------
#endif
};
//====================================================================================
};