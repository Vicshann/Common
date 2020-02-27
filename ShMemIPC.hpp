
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

struct ShMem      
{
//===========================================================================
template<typename Usr=long> class CSharedMem     // TODO: Kernel support (Use same NTAPI?)   // TODO: Time of message locking to unlock by timeout even if an owner process is crashed
{
static const int MaxNameSize = 64;

#pragma pack(push,1)
struct SMemDescr: public Usr  
{
 volatile ULONG MemFlgs;    // Unused for now
 volatile ULONG MemSize;    // Size of MemData
 volatile BYTE NtfEName[MaxNameSize];
 volatile BYTE SynMName[MaxNameSize];         // Name of Mutex
 volatile BYTE Data[0];              // Should be aligned to 8
};
#pragma pack(pop)

 HANDLE hNotifyEvt;    // NOTE: Do not rely on Notification Event when used a multiple observers(Set timeout as low as possible and confirm changes by some other means)
 HANDLE hSyncMutex;
 HANDLE hMapFile;
 SMemDescr* MemDesc;   // Shared memory buffer

//---------------------------------------------------------
static void MakeObjName(ULONG_PTR Value, LPSTR CustomPart, LPSTR OutName)
{
 BYTE TmpBuf[MaxNameSize];
 TmpBuf[0] = 0;
 if(CustomPart)
  {
   int idx = 0;
   if(Value)TmpBuf[idx++] = '_';
   lstrcpynA((LPSTR)&TmpBuf[idx],CustomPart,sizeof(TmpBuf)-(8+8));  // Global\XXXXXXXX_CustomPart
  }
 ULONG_PTR NumPart = Value;
 if(NumPart)
  {
   NumPart ^= (GetTickCount() * (GetCurrentThreadId() * GetCurrentProcessId()));  // Randomized?
   wsprintfA(OutName,"Global\\%p%s",NumPart,(LPSTR)&TmpBuf);
  }
   else wsprintfA(OutName,"Global\\%s",(LPSTR)&TmpBuf);
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
 BYTE FullPath[256];
 wsprintfA((LPSTR)&FullPath,"Global\\%s",MapName);
 HANDLE res = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, (LPSTR)&FullPath);
 if(!res)return false;     // Doesn`t exist or access denied
 CloseHandle(res);
 return true;
}
//---------------------------------------------------------
static UINT64 GetMappingSize(LPSTR MapName)
{
 BYTE FullPath[256];
 UINT64 Size = 0;
 wsprintfA((LPSTR)&FullPath,"Global\\%s",MapName);
 HANDLE hMap = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, (LPSTR)&FullPath);
 if(!hMap)return 0;     // Doesn`t exist or access denied
#ifdef _NTDLL_H
 SECTION_BASIC_INFORMATION SectionInfo; // = { 0 };
 NTSTATUS res = NtQuerySection(hMap, SectionBasicInformation, &SectionInfo, sizeof(SectionInfo), 0);
 if(!res)Size = SectionInfo.MaximumSize.QuadPart;
#endif
 CloseHandle(hMap);  
 return Size;
}
//---------------------------------------------------------
int Connect(LPSTR MapName, UINT SizeOfNew)    // Creates or opens a Shared Memory 
{
 BYTE TmpBuf[MaxNameSize];
 if(this->hMapFile){if(this->Disconnect() < 0){DBGMSG("Failed to disconnect!"); return -1;}}
 SizeOfNew = AlignFrwd(SizeOfNew,8);         
 MakeObjName(0, MapName, (LPSTR)&TmpBuf);

/*SECURITY_ATTRIBUTES security;
ZeroMemory(&security, sizeof(security));
security.nLength = sizeof(security);
ConvertStringSecurityDescriptorToSecurityDescriptor(
         L"D:P(A;OICI;GA;;;SY)(A;OICI;GA;;;BA)(A;OICI;GWGR;;;IU)",
         1,
         &security.lpSecurityDescriptor,
         NULL);
  */
 this->hMapFile   = CreateFileMappingA(INVALID_HANDLE_VALUE,NULL,PAGE_READWRITE,0,SizeOfNew,(LPSTR)&TmpBuf);
//LocalFree(securityDescriptor.lpSecurityDescriptor);
 if(!this->hMapFile){DBGMSG("Failed to create mapping: %u", GetLastError()); return -2;}
 bool CreatedNew  = !(GetLastError() == ERROR_ALREADY_EXISTS);
 this->MemDesc    = (SMemDescr*)MapViewOfFile(this->hMapFile,FILE_MAP_ALL_ACCESS,0,0,SizeOfNew);  // Real memory pages will be allocated on first access?
 if(!this->MemDesc){DBGMSG("Failed to map memory: %u", GetLastError()); CloseHandle(this->hMapFile); this->hMapFile = NULL; return -3;}
 if(CreatedNew)
  {
   DBGMSG("Created: Initializing the header");
//   memset(this->MemDesc,0,sizeof(SMemDescr)+sizeof(PVOID));
   this->MemDesc->MemSize = SizeOfNew;
   MakeObjName((ULONG_PTR)this->hMapFile, "S", (LPSTR)&this->MemDesc->SynMName);    // Create a Mutex name
   MakeObjName((ULONG_PTR)this->hMapFile, "N", (LPSTR)&this->MemDesc->NtfEName);  
  }
 this->hSyncMutex = CreateMutexA(NULL, FALSE, (LPSTR)&this->MemDesc->SynMName);     // Take the names from shared memory 
 this->hNotifyEvt = CreateEventA(NULL, TRUE, FALSE, (LPSTR)&this->MemDesc->NtfEName); 
 if(!this->hSyncMutex || !this->hNotifyEvt){DBGMSG("Failed to create sync objects: hSyncMutex=%p(%s), hNotifyEvt=%p(%s)", this->hSyncMutex, &this->MemDesc->SynMName, this->hNotifyEvt, &this->MemDesc->NtfEName); this->Disconnect(); return -4;}
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
 if(!UnmapViewOfFile(this->MemDesc)){DBGMSG("Failed to unmap memory!");return -3;}
 DBGMSG("MemDesc=%p",this->MemDesc);
 this->MemDesc   = NULL;
 CloseHandle(hMap); 
 if(this->hNotifyEvt)CloseHandle(this->hNotifyEvt);
 this->hNotifyEvt = NULL;
 this->UnlockBuffer();
 if(this->hSyncMutex)CloseHandle(this->hSyncMutex);
 this->hSyncMutex = NULL; 
 return 0;
}
//---------------------------------------------------------
bool LockBuffer(UINT WaitDelay=5000)
{
// DBGMSG("<<<<<<<<<<<<<<<<: %u",WaitDelay);
 DWORD val = GetTickCount();
 bool  res = (WaitForSingleObject(this->hSyncMutex,WaitDelay) != WAIT_TIMEOUT);
// DBGMSG("<<<<<<<<<<<<<<<<: %u = %u",res,GetTickCount()-val);
 if(res && !this->IsConnected()){this->UnlockBuffer(); return false;}
 return res;
}
//---------------------------------------------------------
bool UnlockBuffer(void)
{
 UINT res = ReleaseMutex(this->hSyncMutex);
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
 return SetEvent(this->hNotifyEvt);
}
//---------------------------------------------------------
bool ResetChange(void)
{
 return ResetEvent(this->hNotifyEvt);  //  At this point all waiting threads are got the event. They all will call this ResetEvent (That`s not a problem?)  
}
//---------------------------------------------------------
// You must check for a new messages before calling this function because some other thread may already reset the Notify Event after a change
//
bool WaitForChange(UINT WaitDelay=1000)
{
// DBGMSG("++++++++++++++++: %u",WaitDelay);
// DWORD val = GetTickCount();
 bool res = (WaitForSingleObject(this->hNotifyEvt,WaitDelay) != WAIT_TIMEOUT);
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
struct SMsgBlk    // Always aligned to 64 bit
{
 volatile ULONG DataSize;    
 volatile ULONG ViewCntr;   // Useful for debugging, nothing more
 volatile ULONG PrevOffs; 
 volatile ULONG NextOffs;   // From beginning of buffer  // There may be gaps between messages after a wrap overwrites some of them
 volatile ULONG TargetID;
 volatile ULONG SenderID;
 volatile ULONG MsgSeqID;   // Incremented for each message
 volatile ULONG ValidMrk;   // A Opening marker. An Closing marker is after the data  
 volatile BYTE  Data[0];    // Better to be aligned to 8 bytes

 static ULONG FullSize(ULONG DSize){return AlignFrwd(DSize + sizeof(SMsgBlk) + 8,8);}
 ULONG FullSize(void){return FullSize(this->DataSize);}   // AlignFrwd(this->DataSize + sizeof(SMsgBlk) + 8,8);}     // All data blocks aligned to 8 bytes    
 bool  IsBroadcast(void){return !this->TargetID;}
};

private:
struct SDescr   // No message wrapping is supported(), if it is not fits then RD and WR pointers are get updated to beginning of the buffer
{
 enum EFlags {flEmpty,flUsed=1};
 volatile ULONG Flags;      // 0 if buffer is empty    // MessageCtr is useless and too costly to maintain
 volatile ULONG NxtMsgID;   // Next MessageID to be used (Used by each instance to exclude a viewed messages from enumeration)  // What will happen after overflow?
 volatile ULONG FirstBlk;   // Points to oldest available message
 volatile ULONG LastBlk;    // Points last added message  
};
#pragma pack(pop)

 ULONG  SyncDelay;
 ULONG  InstanceID;
 SMsgBlk* NewMsg;   // Temporary ptr, protected by Lock
 CSharedMem<SDescr> MBuf;

//---------------------------------------------------------------------------
bool IsValidHdr(SMsgBlk* Blk)
{
 PBYTE Ptr = this->MBuf.BufferPtr();
 PBYTE End = &Ptr[this->MBuf.BufferSize()];
 PBYTE Msg = (PBYTE)Blk;                 
 if((Msg < Ptr)||(Msg >= End)){DBGMSG("Problem: %u",0); return false;}    // Begin is not inside the shared buffer
 if((&Msg[sizeof(SMsgBlk)] < Ptr)||(&Msg[sizeof(SMsgBlk)] > End)){DBGMSG("Problem: %u",1); return false;}    // Hdr is not inside the shared buffer
 ULONG Len = Blk->FullSize();
 if((&Msg[Len] < Ptr)||(&Msg[Len] > End)){DBGMSG("Problem: %u",2); return false;}    // End is not inside the shared buffer
 if(!Blk->SenderID){DBGMSG("Problem: %u",3); return false;}   // Not finished yet       
 bool res = !(~(*(ULONG*)&Msg[Len-sizeof(ULONG)]) ^ Blk->ValidMrk);  // Validation markers do not match
 if(!res){DBGMSG("Problem: %u",4); return false;}
 return res;
}
//---------------------------------------------------------------------------
void SetValidHdr(SMsgBlk* Blk)
{
 PBYTE Msg = (PBYTE)Blk;
 ULONG Len = Blk->FullSize();
 Blk->ValidMrk = (Blk->DataSize ^ Blk->TargetID ^ Blk->MsgSeqID);       // Checksum of a fields that will stay unmodified
 *(ULONG*)&Msg[Len-(sizeof(ULONG)*2)] = 0;              // ???
 *(ULONG*)&Msg[Len-sizeof(ULONG)] = ~Blk->ValidMrk;
}
//---------------------------------------------------------------------------
void InvalidateHdr(SMsgBlk* Blk)
{
 PBYTE Msg = (PBYTE)Blk;
 ULONG Len = Blk->FullSize();
 Blk->ValidMrk = 0;     
 *(ULONG*)&Msg[Len-sizeof(ULONG)] = 0;
}
//---------------------------------------------------------------------------

public:
static const int MSG_BROADCAST = 0;                                 
enum MEFlags {mfNone,mfLocked=0x02,mfAnyTgt=0x04,mfOwnMsg=0x08,mfNoLock=0x10,mfHaveNxtMsg=0x20,mfHaveLstMsg=0x40,mfEnumActive=0x80}; 
//---------------------------------------------------------------------------
struct SEnumCtx
{
 ULONG Flags;
 ULONG TgtID;
 ULONG SndID;
 ULONG NxtOffs;
 ULONG LstOffs;
 ULONG NxtMsgID;
 ULONG LstMsgID;    // Last Found message ID
 ULONG FirstMsgID;  // For buffer overflow detection
 SEnumCtx(void){TgtID=SndID=NxtMsgID=LstMsgID=Flags=0; NxtOffs=LstOffs=-1;}
};
//---------------------------------------------------------------------------
CSharedIPC(ULONG SDel=5000)
{
 this->SyncDelay  = SDel;
 this->InstanceID = ((ULONG)this + GetTickCount())|1;  // Must never be 0
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
 if(this->IsEmpty() || (Blk->PrevOffs == (ULONG)-1))return NULL;
 return (SMsgBlk*)&this->MBuf.BufferPtr()[Blk->PrevOffs];
}
//---------------------------------------------------------------------------
SMsgBlk* GetNextBlk(SMsgBlk* Blk)     // In memory order        
{
 if(this->IsEmpty() || (Blk->NextOffs == (ULONG)-1))return NULL;
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
ULONG GetNexMsgSeqNum(void){return (this->IsConnected())?(this->MBuf.UserData()->NxtMsgID):(-1);}
//---------------------------------------------------------------------------
// TODO: Enumeration in different processes should not block each other? (But enumeration definently should block any manipulation)
// TODO: Receive only with ViewCtr <= than specified
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
// DBGMSG("Flags=%08X, NxtOffs=%08X, NxtMsgID=%u, LstOffs=%08X, LstMsgID=%u",Ctx->Flags,Ctx->NxtOffs,Ctx->NxtMsgID,Ctx->LstOffs,Ctx->LstMsgID);
 if(Ctx->Flags & mfHaveLstMsg)  // Higher priority // LstMsgID and LstOffs must be valid   // A message with LstMsgID may be a response and may be removed by a receiver
  {
//   DBGMSG("Beg mfHaveLstMsg: NxtOffs=%08X, LstOffs=%08X, LstMsgID=%u",Ctx->NxtOffs,Ctx->LstOffs,Ctx->LstMsgID);
   Ctx->NxtOffs = Desc->FirstBlk;        // Start from beginning of the buffer (Default)
   if(Ctx->LstOffs != (ULONG)-1)     // Offset is already specified
    {
     SMsgBlk* CurMsg = (SMsgBlk*)&this->MBuf.BufferPtr()[Ctx->LstOffs];
     if(!this->IsValidHdr(CurMsg) || (Ctx->LstMsgID != CurMsg->MsgSeqID))Ctx->LstOffs = -1;  // An invalid offset or a different massage at it
    }
   if(Ctx->LstOffs == (ULONG)-1)  // Try to continue with ID after LstMsgID
    {
     ULONG LstID = Ctx->NxtMsgID;
     if(Ctx->LstMsgID > (LstID-1))  // MsgID has been overflowed  // TODO: TEST IT !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
      {
       bool OPoint = false;
       SMsgBlk* CurMsg = NULL;
       ULONG Offs  = Desc->LastBlk;                       
       while(Offs != (ULONG)-1)  // Find an overflow point [56789-OP-01234 <<]  // 9 is OP here    
        {
         CurMsg = (SMsgBlk*)&this->MBuf.BufferPtr()[Offs];
         if(!this->IsValidHdr(CurMsg)){DBGMSG("Corrupted: Removing mfHaveLstMsg"); Ctx->Flags &= ~mfHaveLstMsg; return NULL;}    // Corrupted buffer! 
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
           if(Offs == (ULONG)-1)break;   // No more messages - enumerate from beginning
           CurMsg = (SMsgBlk*)&this->MBuf.BufferPtr()[Offs];
           if(!this->IsValidHdr(CurMsg)){DBGMSG("Corrupted: Removing mfHaveLstMsg"); Ctx->Flags &= ~mfHaveLstMsg; return NULL;}    // Corrupted buffer!                         
          }
        }
      }
       else   // Not overflowed or overflowed too far with Message IDs
        {
         for(ULONG Offs = Desc->LastBlk;Offs != (ULONG)-1;)     // Find a message offset by ID     
          {
           SMsgBlk* CurMsg = (SMsgBlk*)&this->MBuf.BufferPtr()[Offs];
           if(!this->IsValidHdr(CurMsg)){DBGMSG("Corrupted: Removing mfHaveLstMsg"); Ctx->Flags &= ~mfHaveLstMsg; return NULL;}    // Corrupted buffer!  
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
//   DBGMSG("End mfHaveLstMsg: NxtOffs=%08X, LstOffs=%08X, LstMsgID=%u",Ctx->NxtOffs,Ctx->LstOffs,Ctx->LstMsgID);
  }
 else if(Ctx->Flags & mfHaveNxtMsg)   // Search for exact match (Fail enumeration if not found)
  {
   if(Ctx->NxtOffs != (ULONG)-1)     // Offset is already specified
    {
     SMsgBlk* CurMsg = (SMsgBlk*)&this->MBuf.BufferPtr()[Ctx->NxtOffs];
     if(!this->IsValidHdr(CurMsg) || (Ctx->NxtMsgID != CurMsg->MsgSeqID))Ctx->NxtOffs = -1;  // An invalid offset or a different massage at it
    }
   if(Ctx->NxtOffs == (ULONG)-1) 
    {
     for(ULONG Offs = Desc->LastBlk;Offs != (ULONG)-1;)     // Find a message offset by ID     
      {
       SMsgBlk* CurMsg = (SMsgBlk*)&this->MBuf.BufferPtr()[Offs];
       if(!this->IsValidHdr(CurMsg)){DBGMSG("Corrupted: Removing mfHaveNxtMsg"); Ctx->Flags &= ~mfHaveNxtMsg; return NULL;}    // Corrupted buffer!  
       if(Ctx->NxtMsgID == CurMsg->MsgSeqID){Ctx->NxtOffs = Offs; break;}  // Found exact match! 
       Offs = CurMsg->PrevOffs;      // Enumerating backwards
      }
    }
  }
   else Ctx->NxtOffs = Desc->FirstBlk;        // Start from beginning of the buffer (Default)

 if(Ctx->NxtOffs != (ULONG)-1)    // NxtOffs is for a first message
  { 
   Ctx->Flags   |= mfEnumActive;
   Ctx->NxtMsgID = ((SMsgBlk*)&this->MBuf.BufferPtr()[Ctx->NxtOffs])->MsgSeqID;
//   DBGMSG("From: Ctx=%p, NxtOffs=%08X, NxtMsgID=%u, LstMsgID=%u",Ctx,Ctx->NxtOffs,Ctx->NxtMsgID,Ctx->LstMsgID);
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
 while(Ctx->NxtOffs != (ULONG)-1)   // NOTE: The buffer will always end up being full of a viwed messages 
  {
   SMsgBlk* CurMsg = (SMsgBlk*)&this->MBuf.BufferPtr()[Ctx->NxtOffs];      // Without Lock this may become invalid because of some AddBlock
   if(!this->IsValidHdr(CurMsg) || (Ctx->NxtMsgID != CurMsg->MsgSeqID)){Ctx->NxtOffs = (ULONG)-1; break;}  // Check NxtMsgID in the loop in case that the buffer is not locked  // NxtOffs and NxtMsgID used to validate that NextMessage is same as was found previously 
   ULONG CurrOffs = Ctx->NxtOffs;
   Ctx->NxtOffs   = CurMsg->NextOffs;    // Enumerate forward in the linked list of messages  // Save the offset that we can continue later     
   if(Ctx->NxtOffs != (ULONG)-1)
    {
     Ctx->NxtMsgID = ((SMsgBlk*)&this->MBuf.BufferPtr()[Ctx->NxtOffs])->MsgSeqID;
     Ctx->Flags   |= mfHaveNxtMsg;
    }
     else 
      {
       Ctx->NxtMsgID = -1;
       Ctx->Flags   &= ~mfHaveNxtMsg;
      }
//   DBGMSG("Next: Ctx=%p, NxtOffs=%08X, NxtMsgID=%u, LstMsgID=%u",Ctx,Ctx->NxtOffs,Ctx->NxtMsgID,Ctx->LstMsgID);
//   DBGMSG("Msg at %08X: Size=%08X, ViewCntr=%u, SenderID=%08X, TargetID=%08X, MsgSeqID=%08X",((PBYTE)CurMsg - this->MBuf.BufferPtr()),CurMsg->DataSize,CurMsg->ViewCntr,CurMsg->SenderID,CurMsg->TargetID,CurMsg->MsgSeqID);                                                               
   if((!Ctx->TgtID || CurMsg->IsBroadcast() || (CurMsg->TargetID == Ctx->TgtID)) && ((!Ctx->SndID || (CurMsg->SenderID == Ctx->SndID)) && ((Ctx->Flags & mfOwnMsg) || (CurMsg->SenderID != this->InstanceID))))  // NOTE: Headers are followed by a data and this approach is very cache unfriendly. But this way a maximum number of messages that the buffer can hold is not predefined and depends only on size of messages
    {
     if(Size)*Size = CurMsg->DataSize;
     Ctx->LstMsgID = CurMsg->MsgSeqID;
     Ctx->LstOffs  = CurrOffs;
     Ctx->Flags   |= mfHaveLstMsg;     // Do not read an already viwed messages when repeating enumeration with same context   // Allow this Flag to be forced here?
#ifdef _DEBUG     
     if(CurMsg->ViewCntr != (ULONG)-1)CurMsg->ViewCntr++;  // Do not overflow!
#endif
     DBGMSG("Found at %08X: Size=%08X, ViewCntr=%u, SenderID=%08X, TargetID=%08X, MsgSeqID=%u",((PBYTE)CurMsg - this->MBuf.BufferPtr()),CurMsg->DataSize,CurMsg->ViewCntr,CurMsg->SenderID,CurMsg->TargetID,CurMsg->MsgSeqID);   
     return (PBYTE)&CurMsg->Data;
    }      
  }
// DBGMSG("No more messages!");
 return NULL;      // No more messages    // If you try again to call NextBlock it will start from beginning
}
//---------------------------------------------------------------------------
bool EnumClose(SEnumCtx* Ctx)
{                    
// DBGMSG("Ctx=%p, Flags=%08X, Conn=%u, NxtOffs=%08X, NxtMsgID=%u, LstMsgID=%u",Ctx,Ctx->Flags,this->IsConnected(),Ctx->NxtOffs,Ctx->NxtMsgID,Ctx->LstMsgID);
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
 ULONG FullLen   = SMsgBlk::FullSize(Size);  
 ULONG FMsg      = Desc->FirstBlk; 
 ULONG LMsg      = Desc->LastBlk;
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
       ULONG EndOffs = LMsg + FullLen;    // End offset of a new message   
       if((Desc->LastBlk >= LMsg)&&(EndOffs > Desc->LastBlk)){DBGMSG("DelPrev: EndOffs=%08X, Desc:LastBlk=%08X",EndOffs, Desc->LastBlk); LstMsg = NULL;}  // Will be overwritten
       for(SMsgBlk* FirstMsg = (SMsgBlk*)&this->MBuf.BufferPtr()[FMsg];EndOffs > FMsg;FirstMsg = (SMsgBlk*)&this->MBuf.BufferPtr()[FMsg])     // Move forward FirstBlk
        {
//         DBGMSG("Overwriting: FirstBlk=%08X, LastBlk=%08X, FullLen=%08X, Size=%08X, ViewCntr=%u, NextOffs=%08X, SeqID=%08X, SenderID=%08X, TargetID=%08X",FMsg, LMsg, FullLen, FirstMsg->DataSize, FirstMsg->ViewCntr, FirstMsg->NextOffs,  FirstMsg->MsgSeqID, FirstMsg->SenderID, FirstMsg->TargetID);      
         if(!this->IsValidHdr(FirstMsg)||(FirstMsg->NextOffs <= FMsg)||(FirstMsg->NextOffs == (ULONG)-1))      // Rest of messages is probably corrupted as well / Loop / no next message
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
 DBGMSG("NewMsg: Offset=%08X, FullLen=%08X, MsgSeqID=%u",LMsg, FullLen, Desc->NxtMsgID); 
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
bool CloseBlock(PULONG MsgSeqNum)     // Call after AddBlock or Enumeration
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
 ULONG Offs = Desc->LastBlk;
 while(RemTotal < BlkCnt)        
  {
   SMsgBlk* CurMsg = (SMsgBlk*)&this->MBuf.BufferPtr()[Offs];
   if(!this->IsValidHdr(CurMsg))break;   // For 'Desc->LastBlk' 0 is valid 
   DBGMSG("Removing Msg at %08X, NextOffs=%08X, Size=%08X, MsgID=%u",Offs,CurMsg->NextOffs,CurMsg->FullSize(), CurMsg->MsgSeqID); 
   Offs = CurMsg->PrevOffs;      // Enumerating backwards
   RemTotal++;
   this->InvalidateHdr(CurMsg);
   if(Offs == (ULONG)-1)  // No more messages in the buffer
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
 DWORD Curr = GetTickCount();
 return (Curr > Initial)?(Curr-Initial):(Initial-Curr);
}
//---------------------------------------------------------------------------
bool IsChanged(ULONG* LstChngHash)
{
 if(!this->IsConnected())return false;
 SDescr* Desc  = this->MBuf.UserData();       // Shared access is not important here
 ULONG NewHash = Desc->NxtMsgID;              // Test only one field
 bool res = (NewHash != *LstChngHash); 
//   if(res){ DBGMSG("NewHash=%u, LstChngHash=%u",NewHash,*LstChngHash); }   // <<<<<<<<<<<<<<<<<<<<<<<<<<<<
 *LstChngHash = NewHash;
 return res;
}
//---------------------------------------------------------------------------
bool WaitForChange(ULONG* LstChngHash=NULL, UINT WaitDelay=5000)    // After this you can start enumerating for messages     //  WaitOnAddress ????????????
{
 if(!this->IsConnected())return false; 
 for(DWORD InitTick = GetTickCount();TicksDelta(InitTick) <= WaitDelay;)
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

};
//===========================================================================
//
//
//
//---------------------------------------------------------------------------
class CArgBuf     // A helper class to use with IPC procedure call
{
protected:
 bool  Heap;
 UINT  Size;
 PBYTE Buff;

public:
CArgBuf(void){this->Heap=false; this->Clear();}
CArgBuf(PBYTE B, UINT S){this->Buff=B; this->Size=S; this->Heap=false;}    // Use an external buffer
~CArgBuf(){this->Clear();}
//------------------------------------------------------------------------------------
UINT  GetLen(void){return this->Size;}
PVOID GetPtr(void){return this->Buff;}
//------------------------------------------------------------------------------------
virtual void Assign(PVOID Data, UINT DataLen, bool DoCopy=false)
{
 this->Clear();
 this->Size = DataLen;
 if(DoCopy)
  {
   this->Buff = (PBYTE)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,DataLen); 
   this->Heap = true;
   memcpy(this->Buff,Data,DataLen);
  }
   else 
    {
     this->Buff = (PBYTE)Data; 
     this->Heap = false;
    }
}
//------------------------------------------------------------------------------------
virtual void Clear(void)
{
 if(this->Heap){HeapFree(GetProcessHeap(),0,this->Buff); this->Heap = false;}     
 this->Size = 0; 
 this->Buff = 0; 
}
//------------------------------------------------------------------------------------

};
//------------------------------------------------------------------------------------
template<int MaxSize=512> class CArgPack: public CArgBuf     // Use this if there are more than one argument IN or OUT
{
 BYTE Data[MaxSize];

public:
CArgPack(void)
{
 this->Buff = (PBYTE)&this->Data;
}
//------------------------------------------------------------------------------------
virtual void Clear(void)
{
 if(this->Heap){HeapFree(GetProcessHeap(),0,this->Buff); this->Heap = false;}      
 this->Size = 0; 
 this->Buff = (PBYTE)&this->Data;
}
//------------------------------------------------------------------------------------
virtual void Assign(PVOID Data, UINT DataLen, bool DoCopy=false)
{
 this->Clear();
 this->Size = DataLen;
 if(DoCopy)
  {
   if(DataLen > MaxSize)
    {
     this->Buff = (PBYTE)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,DataLen);
     this->Heap = true;
    }
     else this->Heap = false;
    memcpy(this->Buff,Data,DataLen);
  }
   else {this->Buff = (PBYTE)Data; this->Heap = false;}
}
//------------------------------------------------------------------------------------
template<typename T> PBYTE PushArgEx(T& Value, char* Name=NULL, UINT Hint=0){return this->PushBlkEx(sizeof(T), &Value, Name, Hint);}
PBYTE PushBlkEx(UINT ValLen, PVOID Value=NULL, char* Name=NULL, UINT Hint=0)
{
 PBYTE Blk = this->PushBlk(ValLen, Value);
 if(Name){this->PushStr(Name); ValLen |= 0x80000000;}
 if(Hint){this->PushArg(Hint); ValLen |= 0x40000000;}
 this->PushArg(ValLen);
 return Blk;
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
PBYTE PushBlk(UINT ValLen, PVOID Value=NULL)
{
 UINT FullLen = this->Size + ValLen;
 if(!this->Heap)
  {
   if(FullLen > MaxSize)  // Moving to Heap 
    {
     this->Buff = (PBYTE)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,FullLen);     
     this->Heap = true;
     memcpy(this->Buff, &this->Data,this->Size);    
    }
  }
   else this->Buff = (PBYTE)HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,this->Buff,FullLen);
 PBYTE DstPtr = &this->Buff[this->Size];
 if(Value)memcpy(DstPtr,Value,ValLen);
 this->Size += ValLen;
 return DstPtr;
}
//------------------------------------------------------------------------------------
PBYTE PopBlk(UINT ValLen, PVOID Value=NULL)
{
 if(ValLen > this->Size){this->Clear(); return NULL;}  // No that much data in buffer
 this->Size  -= ValLen;
 PBYTE DstPtr = &this->Buff[this->Size];
 if(Value)memcpy(Value,&this->Buff[this->Size],ValLen);
 return DstPtr;
}
//------------------------------------------------------------------------------------
};
//===========================================================================
//
//
//
//---------------------------------------------------------------------------
class CMessageIPC
{ 
protected:
 UINT InstID;
 UINT DSeqNum;
 UINT WaitDelay;   
 CSharedIPC ipc;
 CRITICAL_SECTION csec;  // Prevents other threads from messing up 'ExchangeMsg'
//------------------------------------------------------------------------------------
public:
struct SMsgHdr  
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
 bool  EnumMsg;
 ULONG Change;
 SMsgHdr* Last;
 CSharedIPC::SEnumCtx BEnum;

 SMsgCtx(UINT Flags=0){EnumMsg=false; Change=0; Last=NULL; BEnum.Flags=Flags;}
} MCtx;    // For global use
//------------------------------------------------------------------------------------

CMessageIPC(void)
{
 this->InstID    = -1;
 this->DSeqNum   = 0;
 this->WaitDelay = 5000;      // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
 InitializeCriticalSection(&this->csec);
}
//------------------------------------------------------------------------------------
~CMessageIPC()
{
 this->Disconnect();
 DeleteCriticalSection(&this->csec);
}
//------------------------------------------------------------------------------------
static bool IsExistForID(UINT InstanceID)
{
 BYTE NameBuf[128];
 wsprintfA((LPSTR)&NameBuf,"I%08XS%u",InstanceID,sizeof(PVOID));
 return CSharedMem<>::IsMappingExist((LPSTR)&NameBuf);
}
//------------------------------------------------------------------------------------
static UINT GetSizeForID(UINT InstanceID)
{
 BYTE NameBuf[128];
 wsprintfA((LPSTR)&NameBuf,"I%08XS%u",InstanceID,sizeof(PVOID));
 return CSharedMem<>::GetMappingSize((LPSTR)&NameBuf);
}
//------------------------------------------------------------------------------------
bool IsConnected(void){return this->ipc.IsConnected();}
void SetDelays(UINT WDelay, UINT SDelay){this->WaitDelay = WaitDelay; this->ipc.SetSyncDelay(SDelay);}
void Clear(void){this->ipc.Clear();}
UINT GetID(void){return this->InstID;}
//------------------------------------------------------------------------------------
int Connect(UINT InstanceID, UINT Size=0, bool DoClr=false)    // IPCSIZE
{
 BYTE NameBuf[128];
 if(this->IsConnected())return 0;
 UINT XSize = GetSizeForID(InstanceID);
 if(XSize)Size = XSize;          // Always use size of an existing buffer
 this->InstID = InstanceID; 
 wsprintfA((LPSTR)&NameBuf,"I%08XS%u",InstanceID,sizeof(PVOID));   // System type must match 
 int res = this->ipc.Connect((LPSTR)&NameBuf, Size); 
 if(DoClr)this->Clear();  // !!! Resets NxtMsgID !!!
// if(res >= 0){this->PutMsg(mtInfo, miHello, 0, NULL, 0); this->InstID = InstanceID;}   // Notify clients 
 return res;
}                                                               
//------------------------------------------------------------------------------------
int Disconnect(void)
{
 if(!this->IsConnected())return 0;
 this->InstID  = -1;
 this->DSeqNum = 0;
// this->PutMsg(mtInfo, miGoodby, 0, NULL, 0);
 return this->ipc.Disconnect();
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
 LeaveCriticalSection(&this->csec); 
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
   EnterCriticalSection(&this->csec);
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
 EnterCriticalSection(&this->csec);
 SMsgHdr* hdr   = NULL;
 if(this->ipc.AddBlock(TgtID, (PVOID*)&hdr, DataSize+sizeof(SMsgHdr)) < 0){LeaveCriticalSection(&this->csec); return -2;}
 hdr->MsgType   = MsgType;
 hdr->MsgID     = MsgID;
 hdr->DataID    = DataID;
 hdr->Sequence  = this->DSeqNum++;   // A Seq number to detect a message loss
 hdr->DataSize  = DataSize;          // 0 for mtEndStrm
 if(MsgPtr)*MsgPtr = &hdr->Data;
 return 0;
}
//------------------------------------------------------------------------------------
int DoneMsg(PULONG MsgSeqNum=NULL)
{
 if(!this->ipc.CloseBlock(MsgSeqNum)){LeaveCriticalSection(&this->csec); return -3;}
 LeaveCriticalSection(&this->csec);
 return 0;
}
//------------------------------------------------------------------------------------
int PutMsg(UINT16 MsgType, UINT16 MsgID, UINT32 DataID, PVOID MsgData, UINT DataSize, PULONG MsgSeqNum=NULL, UINT TgtID=CSharedIPC::MSG_BROADCAST)      // From clients to exact InstanceID (And optionally a stream) or broadcast
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
int ExchangeMsg(UINT16 MsgID, UINT16 MsgType, CArgBuf* Req, CArgBuf* Rsp, UINT TgtID=CSharedIPC::MSG_BROADCAST)    // TODO: Lock buffer and rmove Req/Rsp if no new messages added
{
// DBGMSG("Request: MsgID=%u, MsgType=%u, ReqSize=%08X",MsgID,MsgType,Req->GetLen());   // TODO: For Full debug log level
 if(!this->IsConnected()){DBGMSG("Not connected!"); return -1;}
 if(!this->IsOtherConnections()){DBGMSG("No other connections!"); this->Disconnect(); return -2;}
 EnterCriticalSection(&this->csec);
// DBGMSG("Entered!");
 SMsgCtx LMCtx;    
 LMCtx.BEnum.LstMsgID = this->ipc.GetNexMsgSeqNum() + 1;    // Limit enumeration to messages added after our request
 int res = this->PutMsg(MsgType, MsgID, 0, Req->GetPtr(), Req->GetLen(), &LMCtx.BEnum.LstMsgID, TgtID);   // NxtMsgID assigned ID of the Request message
 if(res < 0){DBGMSG("Failed: %i",res); LeaveCriticalSection(&this->csec); return res;}
 if(!Rsp){DBGMSG("Done, no response expected."); LeaveCriticalSection(&this->csec); return 0;}   // No response needed
 UINT16 ExpectMT = MsgType << 1;  // Special value! Should it be this way?
 DWORD  StTicks  = GetTickCount(); 
 LMCtx.BEnum.Flags |= CSharedIPC::mfHaveLstMsg;  // Enumerate only messages AFTER LstMsgID
// DBGMSG("EnumMsg=%u, ENxtMsgID=%u, ENxtOffs=%08X",LMCtx.EnumMsg,LMCtx.BEnum.NxtMsgID,LMCtx.BEnum.NxtOffs);
 for(SMsgHdr* Cmd = this->GetMsg(&LMCtx);Cmd || (CSharedIPC::TicksDelta(StTicks) < this->WaitDelay);Cmd = this->GetMsg(&LMCtx))   // DO NOT BREAK THIS LOOP by other means than 'meDone'!    // NOTE: Tick counter may overflow!!!
  {
   if(!Cmd)continue;
//   DBGMSG("MsgType=%04X, MsgID=%04X, DataID=%08X, Sequence=%08X, DataSize=%08X",Cmd->MsgType,Cmd->MsgID,Cmd->DataID,Cmd->Sequence,Cmd->DataSize);
   if((Cmd->MsgType != ExpectMT)||(Cmd->MsgID != MsgID)||(Cmd->DataID != (this->DSeqNum-1)))continue;
   Rsp->Assign(&Cmd->Data,Cmd->DataSize, true);
//   DBGMSG("Response: MsgID=%u, MsgType=%u, RspSize=%08X, RspWaitMs=%u",MsgID,MsgType,Rsp->GetLen(),GetTickCount()-StTicks);     // TODO: For Full debug log level
   this->EndMsg(&LMCtx, true);      // Unlock buffer
   LeaveCriticalSection(&this->csec);
   return Rsp->GetLen();
  }
 DBGMSG("No Response: MsgID=%u, MsgType=%u, ReqSize=%08X, RspWaitMs=%u",MsgID,MsgType,Req->GetLen(),GetTickCount()-StTicks);
 LeaveCriticalSection(&this->csec);
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