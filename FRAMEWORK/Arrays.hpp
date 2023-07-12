
#pragma once

template<typename T, int ATerm=-1> class CArr    // TODO: Allocator based?
{
 using Self = CArr<T,ATerm>;
 static constexpr const uint SizeIdx  = -1;
 static constexpr const uint ASizeIdx = -2;
 static constexpr const uint BeginIdx = (16/sizeof(uint));   // 2 on x64, 4 on x32
 T* AData;

//----------------------------------------------------------
// Ptr is Base + 16
static bool DeAllocate(vptr Ptr)      // TODO: Use allocator
{
 if(!Ptr)return true;
 uint* DPtr = (uint*)Ptr;
 if(NPTM::NAPI::munmap(&DPtr[-BeginIdx], DPtr[ASizeIdx]) < 0){ DBGERR("Failed to deallocate!"); return false; }
 return true;
}
//----------------------------------------------------------
static vptr Allocate(uint Len)
{
 uint  flen = AlignP2Frwd(Len + sizeof(vptr), NPTM::MEMPAGESIZE);
 void* fdat = (void*)NPTM::NAPI::mmap(nullptr, flen, PX::PROT_READ|PX::PROT_WRITE, PX::MAP_PRIVATE|PX::MAP_ANONYMOUS, -1, 0);
 if(NPTM::GetMMapErrFromPtr(fdat)){ DBGERR("Error: Failed to allocate!"); return nullptr; }
 uint* Ptr  = &((uint*)fdat)[BeginIdx];
 Ptr[SizeIdx]  = Len;
 Ptr[ASizeIdx] = flen;
 return Ptr;
}
//----------------------------------------------------------
static vptr ReAllocate(uint Len, vptr OldPtr)  // TODO: Use allocator
{
 if(!OldPtr)return Allocate(Len);
 uint flen = AlignP2Frwd(Len + sizeof(vptr), NPTM::MEMPAGESIZE);
 if(flen <= ((uint*)OldPtr)[ASizeIdx])   // Fits in last allocation
  {
   ((uint*)OldPtr)[SizeIdx] = Len;
   return OldPtr;
  }
 vptr NewPtr = Allocate(Len);
 if(NewPtr)memcpy(NewPtr, OldPtr, ((uint*)OldPtr)[SizeIdx]);  // Copy old data
 DeAllocate(OldPtr);
 return NewPtr;
}
//----------------------------------------------------------

public:
 CArr(void){this->AData = nullptr;}
 CArr(uint Cnt){this->AData = nullptr; this->Resize(Cnt);}
 ~CArr(){this->SetLength(0);}
 operator  T*() {return this->AData;}    // operator   const T*() {return this->AData;}
 T* Data(void){return this->AData;}
 T* c_data(void){return this->AData;}    // For name compatibility in a templates
 uint Count(void){return (this->Size() / sizeof(T));}
 uint Size(void) const {return ((this->AData)?(((uint*)this->AData)[-1]):(0));}
 uint Length(void){return this->Size();}

//----------------------------------------------------------
void Clear(void){this->SetLength(0);}
//----------------------------------------------------------
void TakeFrom(Self& arr)
{
 this->SetLength(0);
 this->AData = arr.AData;
 arr.AData = nullptr;
}
//----------------------------------------------------------
bool Assign(void* Items, uint Cnt)     // Cnt is in Items
{
 uint NewLen = Cnt * sizeof(T);
 if(!this->SetLength(NewLen))return false;
 if(Items)memcpy(this->AData, Items, NewLen);
 return true;
}
//----------------------------------------------------------
bool Append(void* Items, uint Cnt)     // Cnt is in Items
{
 uint OldSize = this->Size();
 uint NewLen  = Cnt * sizeof(T);
 if(!this->SetLength(OldSize+NewLen))return false;
 if(Items)memcpy(&((uint8*)this->AData)[OldSize], Items, NewLen);
 LOGMSG("DST=%p, SRC=%p, LEN=%08X",&((uint8*)this->AData)[OldSize], Items, NewLen);
 return true;
}
//----------------------------------------------------------
 //CArr<T>& operator += (const char* str){this->Append((void*)str, lstrlenA(str)); return *this;}
//----------------------
 //CArr<T>& operator += (const wchar_t* str){this->Append((void*)str, lstrlenW(str)); return *this;}
//----------------------
inline Self& operator += (const Self& arr){this->Append(arr.AData, arr.Size()); return *this;}
template<typename A, uint N> inline Self& operator += (const A(&str)[N])
{
 static_assert(sizeof(A) == sizeof(T));  // At least size must match!
 uint len; // = N;  //(N && (sizeof(*str) == 1) && !str[N-1])?(N-1):(N);
 if constexpr (ATerm >= 0)
  {
   for(len=0;(str[len] != (A)ATerm)&&(len < N);)len++;
  }
   else len = N;
 this->Append((vptr)&str, len);
 return *this;
}
//----------------------------------------------------------
inline bool Resize(uint Cnt){return this->SetLength(Cnt*sizeof(T));}  // In Elements
bool SetLength(uint Len)    // In bytes!
{
 uint* Ptr = (uint*)this->AData;
 if(Len && Ptr)Ptr = (uint*)ReAllocate(Len, Ptr);
	else if(!Ptr && Len)Ptr = (uint*)Allocate(Len);
	  else if(!Len && Ptr)
    {
     DeAllocate(Ptr);
     this->AData=nullptr;
     return false;
    }
      else return true;
 if(!Ptr)return false;
 this->AData = (T*)Ptr;
 return true;
}
//----------------------------------------------------------
sint FromFile(achar* FileName)
{
 int df = NPTM::NAPI::open(FileName,PX::O_RDONLY,0);
 if(df < 0){ DBGERR("Error: Failed to open the file %i: %s!",df,FileName); return -1; }
 sint flen = NPTM::NAPI::lseek(df, 0, PX::SEEK_END);    // TODO: Use fstat
 NPTM::NAPI::lseek(df, 0, PX::SEEK_SET);     // Set the file position back
 if(flen < 0){NPTM::NAPI::close(df); return -2;}
 if(this->SetLength(flen) < 0){NPTM::NAPI::close(df); return -3;}
 sint rlen = NPTM::NAPI::read(df, this->AData, this->Size());
 NPTM::NAPI::close(df);
 if(rlen < 0)return -4;
 if(rlen != flen){ DBGERR("Error: Data size mismatch!"); return -5; }
 return (rlen / sizeof(T));
}
//----------------------------------------------------------
sint ToFile(achar* FileName, uint Length=0, uint Offset=0)       // TODO: Bool Append    // From - To
{
 uint DataSize = this->Size();
 if(Offset >= DataSize)return 0;  // Beyond the data
 if(!Length)Length = DataSize;
 if((Offset+Length) > DataSize)Length = (DataSize - Offset);
 if(!Length)return 0;
 int df = NPTM::NAPI::open(FileName,PX::O_CREAT|PX::O_RDWR|PX::O_TRUNC, 0666);   // O_TRUNC  O_EXCL   // 0666
 if(df < 0){ DBGERR("Error: Failed to create the output data file(%i): %s!",df,FileName); return -1; }
 sint wlen = NPTM::NAPI::write(df, &((uint8*)this->AData)[Offset], Length);
 NPTM::NAPI::close(df);
 if(wlen < 0)return -2;
 if((size_t)wlen != this->Size()){ DBGERR("Error: Data size mismatch!"); return -3; }
 return (wlen / sizeof(T));
}
//----------------------------------------------------------

};
//---------------------------------------------------------------------------
