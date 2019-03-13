
#pragma once

//======================================================================================================================
typedef size_t (*TMemResizer)(size_t Required, size_t Allocated, size_t Prealloc, size_t MaxSize);    // Returns size of memory to allocate. If returns 0 then a buffer is expected to be released completely

static inline size_t _fastcall MemResizerCompact(size_t Required, size_t Allocated, size_t Prealloc, size_t MaxSize)   // NOTE: Expected to be inlined. If not then put it in a class and pass it as a type to templates
{
 return Required + (Allocated?Allocated:Prealloc); 
}

// NOTE: Static polymorphism is used here
//======================================================================================================================
// Abstract Buffer interface: Memory, Files, Mapped Files, Shared Memory
//
class IBuffer
{
public:
virtual ~IBuffer() {};    // If 'pure' virtual setsructoris used then it have to be defined outside the class  // this->Release();
virtual size_t Size(void) = 0;
virtual size_t Resize(size_t NewSize) = 0;                                // File/Memory resize
virtual size_t Shrink(void) = 0;
virtual size_t Read(void* Ptr, size_t Size, size_t Offset) = 0;           // For file stream remember Last position and do not update FilePosition if Offset is same
virtual size_t Write(void* Ptr, size_t Size, size_t Offset) = 0;
virtual size_t Move(size_t OffsTo, size_t OffsFrom, size_t Size) = 0;     // Move inside the buffer (Cut at OffsFrom and insert at OffsTo)
virtual size_t Copy(size_t OffsTo, size_t OffsFrom, size_t Size) = 0;     // Copy inside the buffer (Copy at OffsFrom and overwrite at OffsTo)
virtual void*  Data(size_t Offset, size_t* Size) = 0;                     // May be hard to implement for FileBuffer but required for arrays
virtual void   Clear(void) = 0;
virtual void   Release(void) = 0;       // Release the resource (Free memory, Close File)
};
//======================================================================================================================
// Abstract Stream interface
// All size_t here in bytes
//
class IStream: virtual public IBuffer
{
 size_t Position;     // Should be 'size_t Position' and will be merged with same member in any IBuffer derived class for File Buffer because of 'virtual' inheritance

protected:
 using IBuffer::Move;        // Against Stream concept
 using IBuffer::Copy;        // Against Stream concept
 using IBuffer::Read;        // Hide Offset Read
 using IBuffer::Write;       // Hide Offset Write

public:
enum EStrmMov : unsigned {spBegin=0,spEnd=1,spCurr=2,spBack=4};  // TODO: Use constexpr operators and strongly typed enums?

IStream(void):Position(0) {}
//............................................................................
inline size_t GetPos(void) {return this->Position;}    
//............................................................................
inline size_t SetPos(size_t NewPos, unsigned Mov)      // Returns updated position value
{
 bool Back = Mov & spBack;
 unsigned Val = Mov & 0x03;
 size_t CurrPos = 0;
 if(Back)
  {
   if(spCurr == Val)CurrPos = (NewPos < this->Position)?(this->Position - NewPos):(0);    // Backward from current
   else if(size_t Len = this->Size(); spEnd == Val)CurrPos = (NewPos < Len)?(Len - NewPos):(0);   // Backward from end (Size)
  }
   else
    {
     size_t Len = this->Size();
     if(spBegin == Val)CurrPos = NewPos;
     else if(spCurr == Val)CurrPos = NewPos + this->Position;
     else if(spEnd == Val)CurrPos = NewPos + Len;
     if(CurrPos > Len)this->Resize(CurrPos);
    }
 this->Position = CurrPos;
 return CurrPos;
}    
//............................................................................
// For file stream remember Last position and do not update FilePosition if Offset is same
inline size_t Read(void* Ptr, size_t Size)  
{
 size_t rdlen = this->Read(Ptr, Size, this->Position);
 this->Position += rdlen;      // Read function shouldn`t do the same when IBuffer derivant have 'size_t Position'
 return rdlen;
}
//............................................................................
inline size_t Write(void* Ptr, size_t Size) 
{
 size_t wrlen = this->Write(Ptr, Size, this->Position);
 this->Position += wrlen;     // Write function shouldn`t do the same when IBuffer derivant have 'size_t Position'
 return wrlen;
}
//............................................................................
};
//======================================================================================================================
// Not have to be a contiguous block of memory
// Can implement locking or not
// Uses static buffer first which is optimal for temporary on stack creation
// This can be even a file read/write class
// You can add any specific functions, like OpenFile(Name), they will be inherited by containers 
// Works only on bytes because these functions can be very big to allow be templated for many types
// All byte-based functions are made 'final' that no object-sized derived class is able to override them and corrupt logic of IStream
//
template<int iDynPreAll=0, int iStatLen=0, typename TAlloc=SAllocHL<>, TMemResizer MRP=MemResizerCompact> class CBufDynStat: virtual public IBuffer
{
 size_t Used;
 size_t Allocated;
 union {
  alignas(ALIGNSIMD) void* BufPtr;
  alignas(ALIGNSIMD) UINT8 Buffer[iStatLen?iStatLen:1];  // Stupid C++: "base classes cannot contain zero-sized arrays". Who cares? (Especially in UNION)  
 };

public:       
//............................................................................
CBufDynStat(void)
{
 this->Used   = this->Allocated = 0;
 this->BufPtr = nullptr;
}
//............................................................................
virtual ~CBufDynStat()
{
 this->Release();     // Do it in IBuffer?
}
//............................................................................
virtual size_t Size(void) final
{
 return 4;
}
//............................................................................
virtual size_t Resize(size_t NewSize) final
{
 return 5;
}
//............................................................................
virtual size_t Shrink(void) final    // Shrinks Allocated to Used size
{
 return 6;
}
//............................................................................
virtual size_t Read(void* Ptr, size_t Size, size_t Offset) final    // Reads at current Position
{
 UINT8* DstPtr = (UINT8*)Ptr;
 while(Size)
  {
   size_t ObLen = 0;
   void* SrcPtr = this->Data(Offset, &ObLen);      // TODO: Optimize Data()
   if(!SrcPtr)break;     // No more data on Position
   size_t RdLen = (ObLen > Size)?Size:ObLen;      // MIN
   CopyMem(DstPtr, SrcPtr, RdLen);
   Size   -= RdLen;
   DstPtr += RdLen;
   Offset += RdLen;
  }
 return (DstPtr - (UINT8*)Ptr);
}
//............................................................................
virtual size_t Write(void* Ptr, size_t Size, size_t Offset) final   // Writes at current Position
{
 return 7;
}
//............................................................................
virtual size_t Move(size_t OffsTo, size_t OffsFrom, size_t Size) final
{
 return 8;
}
//............................................................................
virtual size_t Copy(size_t OffsTo, size_t OffsFrom, size_t Size) final
{
 return 9;
}
//............................................................................
// Size in:  For file readers, required size
// Size out: For file readers, read result size
//
virtual void* Data(size_t Offset, size_t* Size) final   
{
 return nullptr;
}
//............................................................................
virtual void Clear(void) {this->Resize(0);}          // Releases some memory (or not), accordingly to MemResizer
//............................................................................
virtual void Release(void) 
{

}
//............................................................................

};
//====================================================================================================================== 
// Allocated on stack
//
template<int iStatLen=0> class CBufStatic: virtual public IBuffer
{
 static const int FullSize = AlignFrwrdPow2(iStatLen, ALIGNSIMD);
 alignas(ALIGNSIMD) UINT8 Buffer[FullSize];
 size_t Used;      // It is better that in case of overflow it will corrupt itself first?

public:       
//............................................................................
CBufStatic(void):Used(0) {}
//............................................................................
virtual ~CBufStatic() {}
//............................................................................
virtual size_t Size(void) final {return this->Used;}
//............................................................................
virtual size_t Resize(size_t NewSize) final 
{
 if(NewSize >= FullSize)this->Used = FullSize;
  else this->Used = NewSize;
 return this->Used;
}
//............................................................................
virtual size_t Shrink(void) final {return this->Used;}
//............................................................................
virtual size_t Read(void* Ptr, size_t Size, size_t Offset) final    // Reads at current Position
{
 if(Offset >= this->Used)return 0;        // OPT: Disable in RELEASE build?
 if((Offset+Size) > this->Used)Size = this->Used - Offset; 
 CopyMem(Ptr, &this->Buffer[Offset], Size); 
 return Size;
}
//............................................................................
virtual size_t Write(void* Ptr, size_t Size, size_t Offset) final   // Writes at current Position
{
 if(Offset >= this->Used)return 0;        // OPT: Disable in RELEASE build?
 if((Offset+Size) > this->Used)
  {
   this->Used = FullSize;  // Can grow a little
   Size = this->Used - Offset; 
  }
 if(Ptr)CopyMem(&this->Buffer[Offset], Ptr, Size);
 return Size;
}
//............................................................................
//     0123456789: 2->6,3 = 0156782349
//                          0123456789
virtual size_t Move(size_t OffsTo, size_t OffsFrom, size_t Size) final       // UNTESTED!!!!!!!!!!!!!!
{
 if((OffsTo >= this->Used)||(OffsFrom >= this->Used))return 0;  // OPT: Disable in RELEASE build?
 size_t SizeTo   = Size;
 size_t SizeFrom = Size;
 //
 // TODO:
 //
 return Size;
}
//............................................................................
virtual size_t Copy(size_t OffsTo, size_t OffsFrom, size_t Size) final       // UNTESTED!!!!!!!!!!!!!!
{
 if((OffsTo >= this->Used)||(OffsFrom >= this->Used))return 0;  // OPT: Disable in RELEASE build?
 if((OffsFrom+Size) > this->Used)Size = this->Used - OffsFrom; 
 if((OffsTo+Size) > this->Used)                                // Safe but slow??????????????????
  {
   this->Used = FullSize;  // Can grow a little
   Size = this->Used - OffsTo; 
  }
 MoveMem(&this->Buffer[OffsTo], &this->Buffer[OffsFrom], Size);
 return Size;
}
//............................................................................
virtual void* Data(size_t Offset, size_t* Size) final   
{
 if(Offset >= this->Used)return nullptr;        // OPT: Disable in RELEASE build?
 *Size = this->Used - Offset;
 return &this->Buffer[Offset];
}
//............................................................................
virtual void Clear(void) {this->Resize(0);}          
//............................................................................
virtual void Release(void) { }
//............................................................................

};
//====================================================================================================================== 



//====================================================================================================================== 
// Should not override byte-based interface functions with counter-based ones!
// StupidC++: There is no way to just replace virtual functions from a base class with nonvirtual ones from a derived class by just marking them as private or something ('const' will help a little)
// Derived from IStream because it need to be accepted by any function that accepts IStream
// TODO: Copy/Move constructor; ToStream/FromStream methods   // NOTE: Add things ONLY when they are required for something
//              
template<typename T, typename TMBlk=CBufDynStat<> > class CStream: public virtual TMBlk, public virtual IStream
{
protected:      // Hide Byte-Sized functions
 using IStream::Move;        // Against Stream concept
 using IStream::Copy;        // Against Stream concept

public:
//............................................................................
inline T*     Data(size_t Index, size_t* Count) const    // 'const' is added to functions to redefine virtual functions of base class without renaming them or changing arguments
{
 *Count *= sizeof(T);   // Count to Size
 T* res  = (T*)((IStream*)this)->Data(Index * sizeof(T), Count);   // 'const' is stripped
 *Count /= sizeof(T);   // Size to Count
 return res;         
}  
//............................................................................
inline size_t GetPos(void) {return this->IStream::GetPos() / sizeof(T);}   
//............................................................................
inline size_t SetPos(size_t NewPos, int Mov=spBegin) {return this->IStream::SetPos(NewPos * sizeof(T), Mov) / sizeof(T);}       // I hope that unused results hint the compiler to not do these calculations at all 
//............................................................................
inline size_t Read(T* Ptr, size_t Count=1) {return this->IStream::Read(Ptr, Count * sizeof(T)) / sizeof(T);}       // This should read objects without calling constructors. That way it may be used for some kind og object exchange in a same process?
//............................................................................
inline size_t Write(T* Ptr, size_t Count=1) {return this->IStream::Write(Ptr, Count * sizeof(T)) / sizeof(T);}     // This probably should prevent objects from destruction to safely exchange obects through streams  // Use refCount?
//............................................................................
inline size_t Count(void) {return this->Size() / sizeof(T);}
//............................................................................

};
//====================================================================================================================== 

//------------------------------------------------------------------------------
// TODO: Abstract container which do not knows if it an array or a linked list
//==============================================================================
template<typename T> class CObjStack
{
 UINT Count;
 T**  Array;   // Should be enough or else use a dyn alloc

//----------------------------------------
void ResizeFor(UINT Cnt)
{
 if(!Cnt)
  {
   if(this->Array)HeapFree(GetProcessHeap(),0,this->Array); 
   this->Array = NULL;
   this->Count = 0;
   return;
  } 
 if(this->Array)this->Array = (T**)HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,this->Array,Cnt*sizeof(PVOID));
   else this->Array = (T**)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,Cnt*sizeof(PVOID));
 this->Count = Cnt;
}
//----------------------------------------

public:
//----------------------------------------
CObjStack(void)
{
 this->Count = 0;
 this->Array = NULL;
}
//----------------------------------------
~CObjStack()
{
 this->Clear();
}
//----------------------------------------
void Clear(void){this->Pop(this->Count);}
//----------------------------------------
UINT GetCount(void){return this->Count;}
//----------------------------------------
T* Get(UINT Idx)
{
 if(Idx >= this->Count)return NULL;
 return this->Array[this->Count-Idx-1];
}
//----------------------------------------
void Set(UINT Idx, T* Val)
{
 if(Idx >= this->Count)return;
 this->Array[this->Count-Idx-1] = Val;
}
//----------------------------------------
T* Push(UINT Num)  // Creates and Pushes N of new objects. You can access them as 0-N
{
 this->ResizeFor(this->Count + Num);
 T** Ptr = &this->Array[this->Count-1];
 for(UINT ctr=0;ctr < Num;ctr++,Ptr--)*Ptr = new T();    // Initialized to Zero if no constructor
 return this->Array[this->Count-1];   // Useful when adding an one object
}
//----------------------------------------
T* Pop(UINT Num)  // Removes(deletes) N objects from list
{
 if(Num > this->Count)Num = this->Count;
 T** Ptr = &this->Array[this->Count-1];
 for(UINT ctr=0;ctr < Num;ctr++,Ptr--)delete(*Ptr);
 this->ResizeFor(this->Count - Num);
 if(!this->Array) return NULL;
 return this->Array[this->Count-1];
}
//----------------------------------------
T* PopBk(UINT Num)
{
 if(Num > this->Count)Num = this->Count;
 T** Ptr = this->Array;
 for(UINT ctr=0;ctr < Num;ctr++,Ptr++)delete(*Ptr);
 UINT Left = this->Count - Num;
 if(Left)memmove(&this->Array[0], &this->Array[Num], Left*sizeof(PVOID));
 this->ResizeFor(Left);
 if(!this->Array) return NULL;
 return this->Array[0];
}
//----------------------------------------

};
//==============================================================================
template<typename T> class CGrowArr
{
public:
 T* Data;

 CGrowArr(void){this->Data = NULL;}
 CGrowArr(UINT Cnt){this->Data = NULL; this->Resize(Cnt);}
 ~CGrowArr(){this->Resize(0);}
 operator  T*() {return this->Data;}
 UINT Count(void){return (this->Size() / sizeof(T));}
 UINT Size(void){return ((this->Data)?(((size_t*)this->Data)[-1]):(0));}
//------------------------------------------------------------------------------------
bool Resize(UINT Cnt)
{
 Cnt = (Cnt*sizeof(T))+sizeof(size_t);
 HANDLE hHeap = GetProcessHeap();
 size_t* Ptr = (size_t*)this->Data;
 if(Cnt && this->Data)Ptr = (size_t*)HeapReAlloc(hHeap,HEAP_ZERO_MEMORY,&Ptr[-1],Cnt);
   else if(!this->Data)Ptr = (size_t*)HeapAlloc(hHeap,HEAP_ZERO_MEMORY,Cnt);
     else if(!Cnt && this->Data){HeapFree(hHeap,0,&Ptr[-1]); this->Data=NULL; return false;}
 *Ptr = Cnt;
 this->Data = (T*)(++Ptr);
 return true;
}

}; 
//====================================================================================
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



//==============================================================================




