
#pragma once

struct NMOPS
{
private:
//===========================================================================
// NOTE: Non void* pointers expect alignment of same size
// NOTE: T should be void or 1,2,4,8 in size
// C++STD: A pointer to void will have the same representation and memory alignment as a pointer to char.
//------------------------------------------------------------------------------------------------------------
_finline static size_t AlignOfPtr(const void* Ptr)
{
 return 1 << ctz((size_t)Ptr);
}
//---------------------------------------------------------------------------
// Unaligned load for basic types
// Dst must be aligned to sizeof(T)
template<typename T, bool BE=false> constexpr _finline static void* LoadAsBytes(T* _RST Dst, void* _RST pSrc)
{
 // FWK_MEM_MISALIGN :
//  *Dst = (T*)Src;
//  Src += sizeof(T);
 size_t Val;
 uint8* _RST Src = (uint8*)pSrc;
 if constexpr(sizeof(T) > 0)Val = *Src;  // uint8
 if constexpr(sizeof(T) > 1)    // uint16
  {
   Src++; Val <<= 8;
   Val |= *Src;
  }
 if constexpr(sizeof(T) > 2)   // uint32
  {
   Src++; Val <<= 8;
   Val |= *Src;
   Src++; Val <<= 8;
   Val |= *Src;
  }
 if constexpr(sizeof(T) > 4)   // uint64
  {
   if constexpr(IsArchX64)
    {
     Src++; Val <<= 8;
     Val |= *Src;
     Src++; Val <<= 8;
     Val |= *Src;
     Src++; Val <<= 8;
     Val |= *Src;
     Src++; Val <<= 8;
     Val |= *Src;
    }
     else Src = (uint8*)LoadAsBytes(&((uint32*)Dst)[1], Src);   // Second half as uint32     (Src is 8b aligned)
  }
 if constexpr(sizeof(T) > 8)    // v128 (16 bytes) // Process another 8 bytes   (Src is 16b aligned)
  {
   Src = (uint8*)LoadAsBytes(&((uint64*)Dst)[1], Src);
  }
 if constexpr(sizeof(T) > 16)   // v256 (32 bytes) // Process another 16 bytes  (Src is 32b aligned)
  {
   Src = (uint8*)LoadAsBytes(&((uint64*)Dst)[2], Src);
   Src = (uint8*)LoadAsBytes(&((uint64*)Dst)[3], Src);
  }
 if constexpr(sizeof(T) > 32)   // v512 (64 bytes) // Process another 32 bytes  (Src is 64b aligned)
  {
   Src = (uint8*)LoadAsBytes(&((uint64*)Dst)[4], Src);
   Src = (uint8*)LoadAsBytes(&((uint64*)Dst)[5], Src);
   Src = (uint8*)LoadAsBytes(&((uint64*)Dst)[6], Src);
   Src = (uint8*)LoadAsBytes(&((uint64*)Dst)[7], Src);
  }
 if constexpr(!BE)Val = SwapBytes(Val);   // Extra operation but should make shifts easier (as with LE)
 if constexpr (sizeof(T) > sizeof(size_t))*(size_t*)Dst = Val;  // uint64 on x32 or vector types
   else *Dst = (T)Val;
 return Src;
}
//---------------------------------------------------------------------------
// Unaligned store for basic types
// Src must be aligned to sizeof(T)
// Should be half-faster than bytewise memcpy which had to be used if both Src and Dst is misaligned
template<typename T, bool BE=false> constexpr _finline static void* StoreAsBytes(T* _RST Src, void* _RST pDst)   // Shifts are used to allow in-register optimizations
{
 size_t Val;   // >>> Aligned load to pointer sized type (4/8)
 if constexpr (sizeof(T) > sizeof(size_t))Val = *(size_t*)Src;  // uint64 on x32 or vector types
   else Val = (size_t)*Src;
 uint8* _RST Dst = (uint8*)pDst;
 if constexpr(BE)Val = SwapBytes(Val);   // Extra operation but should make shifts easier (as with LE)
 if constexpr(sizeof(T) > 0)*Dst = (uint8)Val;  // uint8
 if constexpr(sizeof(T) > 1)    // uint16
  {
   Dst++; Val >>= 8;
   *Dst = (uint8)Val;
  }
 if constexpr(sizeof(T) > 2)   // uint32
  {
   Dst++; Val >>= 8;
   *Dst = (uint8)Val;
   Dst++; Val >>= 8;
   *Dst = (uint8)Val;
  }
 if constexpr(sizeof(T) > 4)   // uint64
  {
   if constexpr(IsArchX64)
    {
     Dst++; Val >>= 8;
     *Dst = (uint8)Val;
     Dst++; Val >>= 8;
     *Dst = (uint8)Val;
     Dst++; Val >>= 8;
     *Dst = (uint8)Val;
     Dst++; Val >>= 8;
     *Dst = (uint8)Val;
    }
     else Dst = (uint8*)StoreAsBytes(&((uint32*)Src)[1], Dst);   // Second half as uint32     (Src is 8b aligned)
  }
 if constexpr(sizeof(T) > 8)    // v128 (16 bytes) // Process another 8 bytes   (Src is 16b aligned)
  {
   Dst = (uint8*)StoreAsBytes(&((uint64*)Src)[1], Dst);
  }
 if constexpr(sizeof(T) > 16)   // v256 (32 bytes) // Process another 16 bytes  (Src is 32b aligned)
  {
   Dst = (uint8*)StoreAsBytes(&((uint64*)Src)[2], Dst);
   Dst = (uint8*)StoreAsBytes(&((uint64*)Src)[3], Dst);
  }
 if constexpr(sizeof(T) > 32)   // v512 (64 bytes) // Process another 32 bytes  (Src is 64b aligned)
  {
   Dst = (uint8*)StoreAsBytes(&((uint64*)Src)[4], Dst);
   Dst = (uint8*)StoreAsBytes(&((uint64*)Src)[5], Dst);
   Dst = (uint8*)StoreAsBytes(&((uint64*)Src)[6], Dst);
   Dst = (uint8*)StoreAsBytes(&((uint64*)Src)[7], Dst);
  }
 return Dst;
}
//---------------------------------------------------------------------------
// Unaligned store Zero (Unrolled and inlined)
// Can zero any type including structs and arrays
// Size and Dst may be unaligned
// NOTE: Probably inefficient for big structs
template<sint DLen> constexpr _finline static void* MemZeroConst(void* _RST pDst)   // Shifts are used to allow in-register optimizations
{
 uint8* _RST Dst = (uint8*)pDst;
 // TODO: Optional v128-v512 with NCFG::VectorizeMemOps
 // TODO: Loop on largest items
 if constexpr(DLen >= sizeof(uint64))       // 8 or more bytes left
  {
//   if(!((size_t)Dst & (sizeof(uint64)-1)){*(uint64*)Dst = 0; return MemZeroConst<DLen-sizeof(uint64)>(Dst + sizeof(uint64));}  // Aligned as 8
   if(!((size_t)Dst & (sizeof(uint64)-1)))   // Aligned as 8  // Looping on a largest - (u64) for now
    {
     constexpr int inum = DLen / sizeof(uint64);
     for(uint num=inum;num;num--,Dst+=sizeof(uint64))*(uint64*)Dst = 0;   // Stores pairs on x32 if not optimized
     return MemZeroConst<DLen-(sizeof(uint64)*inum)>(Dst);
    }
   if(!((size_t)Dst & (sizeof(uint32)-1))){*(uint32*)Dst = 0; return MemZeroConst<DLen-sizeof(uint32)>(Dst + sizeof(uint32));}  // Aligned as 4
   if(!((size_t)Dst & (sizeof(uint16)-1))){*(uint16*)Dst = 0; return MemZeroConst<DLen-sizeof(uint16)>(Dst + sizeof(uint16));}  // Aligned as 2
   *Dst = 0;
   return MemZeroConst<DLen-sizeof(uint8)>(Dst + sizeof(uint8));   // Unaligned
  }
 else if constexpr(DLen >= sizeof(uint32))  // 4 bytes left
  {
   if(!((size_t)Dst & (sizeof(uint32)-1))){*(uint32*)Dst = 0; return MemZeroConst<DLen-sizeof(uint32)>(Dst + sizeof(uint32));}  // Aligned as 4
   if(!((size_t)Dst & (sizeof(uint16)-1))){*(uint16*)Dst = 0; return MemZeroConst<DLen-sizeof(uint16)>(Dst + sizeof(uint16));}  // Aligned as 2
   *Dst = 0;
   return MemZeroConst<DLen-sizeof(uint8)>(Dst + sizeof(uint8));   // Unaligned
  }
 else if constexpr(DLen >= sizeof(uint16))  // 2 bytes left
  {
   if(!((size_t)Dst & (sizeof(uint16)-1))){*(uint16*)Dst = 0; return MemZeroConst<DLen-sizeof(uint16)>(Dst + sizeof(uint16));}  // Aligned as 2
   *Dst = 0;
   return MemZeroConst<DLen-sizeof(uint8)>(Dst + sizeof(uint8));   // Unaligned
  }
 else if constexpr(DLen >= sizeof(uint8))   // 1 byte left
  {
   *Dst = 0;
   return MemZeroConst<DLen-sizeof(uint8)>(Dst + sizeof(uint8));
  }
 else return Dst;    // No more to store
}

template<typename T> constexpr _finline static T* ZeroObject(T* _RST pDst){return (T*)MemZeroConst<sizeof(T)>(pDst);}
//---------------------------------------------------------------------------
template<typename T, uint ASize> constexpr static void InitFillPattern(uint32* FillArr, const T& Val)
{
 if constexpr((sizeof(T) < sizeof(v512)) || !NCFG::VectorizeMemOps)
  {
   if constexpr((sizeof(T) < sizeof(v256)) || !NCFG::VectorizeMemOps)
    {
     if constexpr((sizeof(T) < sizeof(v128)) || !NCFG::VectorizeMemOps)
      {
       if constexpr(sizeof(T) < sizeof(uint64))
        {
         if constexpr(sizeof(T) < sizeof(uint32))
          {
           if constexpr(sizeof(T) < sizeof(uint16))
            {
             uint32 xval = Val|((uint32)Val << 8)|((uint32)Val << 16)|((uint32)Val << 24);
             uint64 fval = xval | ((uint64)xval << 32);
             for(uint idx=0;idx < (ASize/sizeof(uint64));idx++)((uint64*)FillArr)[idx] = fval;
            }
             else {uint64 fval = Val | ((uint64)Val << 16) | ((uint64)Val << 32) | ((uint64)Val << 48); for(uint idx=0;idx < (ASize/sizeof(uint64));idx++)((uint64*)FillArr)[idx] = fval;}
          }
           else {uint64 fval = Val | ((uint64)Val << 32); for(uint idx=0;idx < (ASize/sizeof(uint64));idx++)((uint64*)FillArr)[idx] = fval;}
        }
        else {for(uint idx=0;idx < (ASize/sizeof(uint64));idx++)((uint64*)FillArr)[idx] = Val;}
      }
       else {for(uint idx=0;idx < (ASize/sizeof(v128));idx++)((v128*)FillArr)[idx] = Val;}
    }
     else {for(uint idx=0;idx < (ASize/sizeof(v256));idx++)((v256*)FillArr)[idx] = Val;}
  }
  else *(v512*)FillArr = Val;
}
//---------------------------------------------------------------------------
/*template<typename T> constexpr _finline static T* CopyObjects(T* _RST Dst, T* _RST Src, const uint Cnt=1)
{
 constexpr uint ta = alignof(T);
 if constexpr (ta >= sizeof(uint64))
  {
    // ???
  }
 return Dst;
} */
//---------------------------------------------------------------------------
template<typename T, bool rev=false> constexpr _finline static size_t CopyAs(void* _RST* _RST Dst, void* _RST* _RST Src, size_t Size)
{
 if constexpr (rev)
  {
   *((uint8**)Src) -= sizeof(T);
   *((uint8**)Dst) -= sizeof(T);
   *(*(T**)Dst) = *(*(T**)Src);
  }
  else
   {
    *(*(T**)Dst) = *(*(T**)Src);
    *((uint8**)Src) += sizeof(T);
    *((uint8**)Dst) += sizeof(T);
   }
 return Size - sizeof(T);
}
//---------------------------------------------------------------------------
template<typename T, bool rev=false> constexpr _finline static size_t StoreAs(T Val, void* _RST* _RST Dst, size_t Size)
{
 if constexpr (rev)
  {
   *((uint8**)Dst) -= sizeof(T);
   *(*(T**)Dst) = Val;
  }
  else
   {
    *(*(T**)Dst) = Val;
    *((uint8**)Dst) += sizeof(T);
   }
 return Size - sizeof(T);
}
//---------------------------------------------------------------------------
// Applicable only if Dst and Src have initially SAME alignment
// Worst case scenarios:
//   AlPtrA 1001 +1> 0010 +2> 0100 +4> 1000 +4> 1100 +4> 0000
//   AlPtrB 0101 +1> 0110 +2> 1000 +4> 1100 +4> 0000 +4> 0100 : Will never sync above 4
//
//   AlPtrA 1011 +1> 1100 +2> 1110
//   AlPtrB 0101 +1> 0110 +2> 1000 : Will never sync above 2
//
template<bool rev=false> constexpr static size_t MemCopySync(void* _RST* _RST Dst, void* _RST* _RST Src, size_t Size)
{
 size_t Mask = ((size_t)*Dst|(size_t)*Src);
 if(Mask & 0x01){Size=CopyAs<uint8,rev>(Dst, Src, Size); Mask = ((size_t)*Dst|(size_t)*Src);}  // u8
 if(Size >= sizeof(uint16))
  {
   //if(Mask & 0x01)return;  // Will never happen
   if(Mask & 0x03){Size=CopyAs<uint16,rev>(Dst, Src, Size); Mask = ((size_t)*Dst|(size_t)*Src);}   // u16
   if(Size >= sizeof(uint32))
    {
     if(Mask & 0x03)return Size;   // Will never sync above 2, may be split copy will be better
     if(Mask & 0x07){Size=CopyAs<uint32,rev>(Dst, Src, Size); Mask = ((size_t)*Dst|(size_t)*Src);}   // u32
     if(Size >= sizeof(uint64))   // For now, largest block is 8 (4x2 on x32)
      {
       if(Mask & 0x07)return Size;   // Will never sync above 4, may be split copy will be better
// --- VECTORIZED
       if constexpr(NCFG::VectorizeMemOps)  // VECTORIZED
        {
         if(Mask & 0x0F){Size=CopyAs<uint64,rev>(Dst, Src, Size); Mask = ((size_t)*Dst|(size_t)*Src);}   // u64
         if((Size >= sizeof(v128)) && !(Mask & 0x0F))  // May never sync above 8 but copy by 8 is fast enough (Only if there is no way for fast splitting for vector types)
          {
           if(Mask & 0x1F){Size=CopyAs<v128,rev>(Dst, Src, Size); Mask = ((size_t)*Dst|(size_t)*Src);}   // v128
           if((Size >= sizeof(v256)) && !(Mask & 0x1F))   // May never sync above 16 (No fast splitting for vector types?)
            {
             if(Mask & 0x3F){Size=CopyAs<v256,rev>(Dst, Src, Size); Mask = ((size_t)*Dst|(size_t)*Src);}   // v256
             if((Size >= sizeof(v512)) && !(Mask & 0x3F))  // May never sync above 32 (No fast splitting for vector types?) // Requires AVX512 on x86
              {
               while(Size >= sizeof(v512))Size=CopyAs<v512,rev>(Dst, Src, Size);    // Emulate if no 64-byte vector instructions present
              }
             while(Size >= sizeof(v256))Size=CopyAs<v256,rev>(Dst, Src, Size);   // Tail as v256
            }
           while(Size >= sizeof(v128))Size=CopyAs<v128,rev>(Dst, Src, Size);   // Tail as v128
          }
         while(Size >= sizeof(uint64))Size=CopyAs<uint64,rev>(Dst, Src, Size);   // Tail as u64
        }
// --- VECTORIZED
        else {while(Size >= sizeof(uint64))Size=CopyAs<uint64,rev>(Dst, Src, Size);}   // u64 (LOOP)
      }
     while(Size >= sizeof(uint32))Size=CopyAs<uint32,rev>(Dst, Src, Size);   // Tail as u32
    }
   while(Size >= sizeof(uint16))Size=CopyAs<uint16,rev>(Dst, Src, Size);   // Tail as u16
  }
 while(Size >= sizeof(uint8))Size=CopyAs<uint8,rev>(Dst, Src, Size);    // Tail as u8
 return Size; // No data left
}
//---------------------------------------------------------------------------
// Uses minimal alignment (like u32 dst when copying from v128 src)
template<uint AMin, bool rev=false> constexpr static size_t MemCopyMSync(void* _RST Dst, void* _RST Src, size_t Size)
{
 if constexpr (AMin > sizeof(uint8))
  {
   if constexpr (AMin > sizeof(uint16))
    {
     if constexpr (AMin > sizeof(uint32))
      {
       if constexpr (AMin > sizeof(uint64))
        {
// --- VECTORIZED
         if constexpr(NCFG::VectorizeMemOps)  // VECTORIZED
          {
           if constexpr (AMin > sizeof(v128))
            {
             if constexpr (AMin > sizeof(v256))
              {
               while(Size >= sizeof(v512))Size=CopyAs<v512,rev>(&Dst, &Src, Size);
              }
               else {while(Size >= sizeof(v256))Size=CopyAs<v256,rev>(&Dst, &Src, Size);}
            }
             else {while(Size >= sizeof(v128))Size=CopyAs<v128,rev>(&Dst, &Src, Size);}
          }
// --- VECTORIZED
           else {while(Size >= sizeof(uint64))Size=CopyAs<uint64,rev>(&Dst, &Src, Size);}    // u64 (LOOP)
        }
         else {while(Size >= sizeof(uint64))Size=CopyAs<uint64,rev>(&Dst, &Src, Size);}
      }
       else {while(Size >= sizeof(uint32))Size=CopyAs<uint32,rev>(&Dst, &Src, Size);}
    }
     else {while(Size >= sizeof(uint16))Size=CopyAs<uint16,rev>(&Dst, &Src, Size);}
  }
   else {while(Size >= sizeof(uint8))Size=CopyAs<uint8,rev>(&Dst, &Src, Size);}
 return Size;
}
//---------------------------------------------------------------------------
// Sizeof(D) is expected to be less than sizeof(S)
// No vectorization for shifts?
// NOTE: Will not work with vector types
template<typename D, typename S, bool rev> constexpr static size_t SplitCopy(void* _RST* _RST pDst, void* _RST* _RST pSrc, size_t Size)
{
 D* Dst = *(D**)pDst;
 S* Src = *(S**)pSrc;
 if constexpr (sizeof(S) > sizeof(D))  // SRC > DST  // Load as split SRC into smaller types of DST
 {
 while(Size >= sizeof(S))    // sizeof(S) > sizeof(D)
  {
   if constexpr (rev)
    {
     Src--;
     S val = *Src;
     if constexpr(NCFG::IsBigEnd)val = SwapBytes(val);  // Untested!
     for(uint ctr=sizeof(S)/sizeof(D);ctr--;){Dst--; *Dst = D(val >> (ctr*(sizeof(D)*8)));}    // Add attr to force it unrolled?
    }
   else
    {
     S val = *Src;
     Src++;
     if constexpr(NCFG::IsBigEnd)val = SwapBytes(val);  // Untested!
     for(uint ctr=sizeof(S)/sizeof(D);ctr;ctr--,val >>= (sizeof(D)*8)){*Dst = D(val); Dst++;}    // Add attr to force it unrolled?
    }
   Size -= sizeof(S);
  }
 }
 else  // DST > SRC // Store as  SRC into bigger types of DST
 {
 while(Size >= sizeof(D))
  {
   if constexpr (rev)
    {
     D val = 0;
     for(uint ctr=sizeof(D)/sizeof(S);ctr;ctr--){Src--; val |= ((D)*Src << ((ctr-1)*8));}    // Add attr to force it unrolled?
     if constexpr(NCFG::IsBigEnd)val = SwapBytes(val);   // Untested!
     Dst--;
     *Dst = val;
    }
   else
    {
     D val = 0;
     for(uint ctr=sizeof(D)/sizeof(S);ctr;ctr--){val |= ((D)*Src << ((ctr-1)*8)); Src--;}    // Add attr to force it unrolled?
     if constexpr(NCFG::IsBigEnd)val = SwapBytes(val);   // Untested!
     *Dst = val;
     Dst++;
    }
   Size -= sizeof(D);
  }
 }
 *pDst = Dst;   // More efficient than using REFs ?
 *pSrc = Src;
 return Size;
}
//---------------------------------------------------------------------------
// Store from SRC to DST by splitting SRC value (Dst blk size is expected to be less than T) (Min size is SplAlign)
// Parses second alignment
// IFs should be faster than SWITCH because not accessing memory?
template<typename T, bool rev=false> constexpr static size_t SplitCopy(void* _RST* _RST Dst, void* _RST* _RST Src, size_t Size, const size_t SplAlign)
{
 if constexpr (sizeof(T) <= sizeof(uint8))   // Should not happen assuming that SplAlign will be expected to be 0
  {
   while(Size)Size=CopyAs<uint8,rev>(Dst, Src, Size);   // By bytes, nothing to split
  }
 else if constexpr (sizeof(T) == sizeof(uint16))return SplitCopy<uint8,uint16,rev>(Dst, Src, Size);   // Split u16 into u8, SplAlign expected to be 1
 else if constexpr (sizeof(T) == sizeof(uint32))   // Split u32
  {
   if(SplAlign == sizeof(uint16))return SplitCopy<uint16,uint32,rev>(Dst, Src, Size);      // Split u32 into u16
   else return SplitCopy<uint8,uint32,rev>(Dst, Src, Size);            // Split u32 into u8
  }
 else if constexpr (sizeof(T) == sizeof(uint64))   // Split u64
  {
   if constexpr(IsArchX64)
    {
     if(SplAlign == sizeof(uint32))return SplitCopy<uint32,uint64,rev>(Dst, Src, Size);      // Split u64 into u32
     else if(SplAlign == sizeof(uint16))return SplitCopy<uint16,uint64,rev>(Dst, Src, Size);      // Split u64 into u16
     else return SplitCopy<uint8,uint64,rev>(Dst, Src, Size);            // Split u64 into u8
    }
   else return SplitCopy<uint32,rev>(Dst, Src, Size, SplAlign);   // Process as array of uint32   // Emulate on x32 as u32
  }
 else if constexpr (sizeof(T) >= sizeof(v128))     // No use splitting vector types (No shifts?)
  {
   if(SplAlign == sizeof(uint64)){while(Size >= sizeof(uint64))Size=CopyAs<uint64,rev>(Dst, Src, Size);}
   else if(SplAlign == sizeof(uint32)){while(Size >= sizeof(uint32))Size=CopyAs<uint32,rev>(Dst, Src, Size);}
   else if(SplAlign == sizeof(uint16)){while(Size >= sizeof(uint16))Size=CopyAs<uint16,rev>(Dst, Src, Size);}
   else {while(Size)Size=CopyAs<uint8,rev>(Dst, Src, Size);}
  }
 return Size;
}
//---------------------------------------------------------------------------
public:
template<bool rev=false> constexpr static void* MemCopy(void* _RST Dst, void* _RST Src, size_t Size)
{
 if(!Size)return Dst;
 size_t AlSrc = AlignOfPtr(Src);
 size_t AlDst = AlignOfPtr(Dst);
 if(AlSrc == AlDst)
  {
   Size  = MemCopySync<rev>(&Dst, &Src, Size);
   if(!Size)return Dst;
   AlSrc = AlignOfPtr(Src);    // Should never be equal unless there are some bugs in MemCopySync
   AlDst = AlignOfPtr(Dst);
  }
 size_t AlMin, AlMax;
 if(AlSrc < AlDst){AlMin = AlSrc; AlMax = AlDst;}
   else {AlMin = AlDst; AlMax = AlSrc;}
 if(AlMax & sizeof(uint16))Size=SplitCopy<uint16,rev>(&Dst, &Src, Size, AlMin);       // To u16
 else if(AlMax & sizeof(uint32))Size=SplitCopy<uint32,rev>(&Dst, &Src, Size, AlMin);  // To u32
 else
  {
   if constexpr(NCFG::VectorizeMemOps)
    {
     if(AlMax & sizeof(uint64))Size=SplitCopy<uint64,rev>(&Dst, &Src, Size, AlMin);   // To u64
     else if(AlMax & sizeof(v128))Size=SplitCopy<v128,rev>(&Dst, &Src, Size, AlMin);  // To u128
     else if(AlMax & sizeof(v256))Size=SplitCopy<v256,rev>(&Dst, &Src, Size, AlMin);  // To u256
     else Size=SplitCopy<v512,rev>(&Dst, &Src, Size, AlMin);    // To u512 (default)
    }
     else Size=SplitCopy<uint64,rev>(&Dst, &Src, Size, AlMin);   // To u64 (default)
  }
 while(Size)Size=CopyAs<uint8,rev>(&Dst, &Src, Size);   // Copy remaining as bytes
 return Dst;
}
//---------------------------------------------------------------------------
constexpr _finline static void* MemMove(void* _RST Dst, void* _RST Src, size_t Size)
{
 if(((uint8*)Dst <= (uint8*)Src) || ((uint8*)Dst >= ((uint8*)Src + Size)))return MemCopy<false>(Dst, Src, Size);  // Is forward copy faster?
  else return MemCopy<true>(((uint8*)Dst + Size), ((uint8*)Src + Size), Size);
}
//---------------------------------------------------------------------------
_finline static void* MemZero(void* _RST Dst, size_t Size)
{
 if((size_t)Dst & sizeof(uint8))Size=StoreAs<uint8>(0, &Dst, Size);          // Align to u16
 if(Size >= sizeof(uint16))
  {
   if((size_t)Dst & sizeof(uint16))Size=StoreAs<uint16>(0, &Dst, Size);      // Align to u32
   if(Size >= sizeof(uint32))
    {
     if((size_t)Dst & sizeof(uint32))Size=StoreAs<uint32>(0, &Dst, Size);    // Align to u64
     if(Size >= sizeof(uint64))
      {
// --- VECTORIZED
       if constexpr(NCFG::VectorizeMemOps)  // VECTORIZED
        {
         if((size_t)Dst & sizeof(uint64))Size=StoreAs<uint64>(0, &Dst, Size);    // Align to v128
         if(Size >= sizeof(v128))
          {
           if((size_t)Dst & sizeof(v128))Size=StoreAs<v128>(v128{0}, &Dst, Size);      // Align to v256
           if(Size >= sizeof(v256))
            {
             if((size_t)Dst & sizeof(v256))Size=StoreAs<v256>(v256{0}, &Dst, Size);    // Align to v512
             if(Size >= sizeof(v512))   // Requires AVX512 on x86
              {
               while(Size >= sizeof(v512))Size=StoreAs<v512>(v512{0}, &Dst, Size);    // TODO: Emulate (constexpr) if no 64-byte vector instructions present
              }
             if(Size >= sizeof(v256))Size=StoreAs<v256>(v256{0}, &Dst, Size);  // Tail as v256
            }
           if(Size >= sizeof(v128))Size=StoreAs<v128>(v128{0}, &Dst, Size);    // Tail as v128
          }
         if(Size >= sizeof(uint64))Size=StoreAs<uint64>(0, &Dst, Size);  // Tail as u64
        }
// --- VECTORIZED
       else {while(Size >= sizeof(uint64))Size=StoreAs<uint64>(0, &Dst, Size);}   // u64 (LOOP)
      }
     if(Size >= sizeof(uint32))Size=StoreAs<uint32>(0, &Dst, Size);  // Tail as u32
    }
   if(Size >= sizeof(uint16))Size=StoreAs<uint16>(0, &Dst, Size);  // Tail as u16
  }
 if(Size >= sizeof(uint8))Size=StoreAs<uint8>(0, &Dst, Size);  // Tail as u8
 return Dst;
}
//---------------------------------------------------------------------------
constexpr static void* MemRotLeft(void* _RST Dst, size_t Size, size_t Bytes)
{
 return nullptr;
}
//---------------------------------------------------------------------------
constexpr static void* MemRotRight(void* _RST Dst, size_t Size, size_t Bytes)
{
 return nullptr;
}
//---------------------------------------------------------------------------
// Fill value size is same size as T (uint8 for void* and char)      // TODO: Recursive?
template<typename T=uint8> constexpr static void* MemFill(void* _RST Dst, size_t Size, const T Val)  // TODO: Obj Type version to make alignment detection constexpr (MemFillObj)
{
 if(!Val)return MemZero(Dst, Size);
 alignas(NCFG::VectorizeMemOps?sizeof(v512):sizeof(uint64)) uint32 ValArr[NCFG::VectorizeMemOps?(sizeof(v512)/sizeof(uint32)):(sizeof(uint64)/sizeof(uint32))];   // TODO: Expand from Val
 InitFillPattern<T,sizeof(ValArr)>(ValArr, Val);

// if constexpr(NCFG::IsBigEnd)val = SwapBytes(val);    // TODO: Fix Swap to work with arrays by ref
 if((size_t)Dst & sizeof(uint8)){Size=StoreAs<uint8>(*(uint8*)&ValArr, &Dst, Size); if constexpr(sizeof(T) > sizeof(uint8))MemRotLeft(&ValArr, sizeof(ValArr), sizeof(uint8));}          // Align to u16
 if(Size >= sizeof(uint16))
  {
   if((size_t)Dst & sizeof(uint16)){Size=StoreAs<uint16>(*(uint16*)&ValArr, &Dst, Size); if constexpr(sizeof(T) > sizeof(uint16))MemRotLeft(&ValArr, sizeof(ValArr), sizeof(uint16));}      // Align to u32
   if(Size >= sizeof(uint32))
    {
     if((size_t)Dst & sizeof(uint32)){Size=StoreAs<uint32>(*(uint32*)&ValArr, &Dst, Size); if constexpr(sizeof(T) > sizeof(uint32))MemRotLeft(&ValArr, sizeof(ValArr), sizeof(uint32));}    // Align to u64
     if(Size >= sizeof(uint64))
      {
// --- VECTORIZED
       if constexpr(NCFG::VectorizeMemOps)  // VECTORIZED
        {
         if((size_t)Dst & sizeof(uint64)){Size=StoreAs<uint64>(*(uint64*)&ValArr, &Dst, Size); if constexpr(sizeof(T) > sizeof(uint64))MemRotLeft(&ValArr, sizeof(ValArr), sizeof(uint64));}    // Align to v128
         if(Size >= sizeof(v128))
          {
           if((size_t)Dst & sizeof(v128)){Size=StoreAs<v128>(*(v128*)&ValArr, &Dst, Size); if constexpr(sizeof(T) > sizeof(v128))MemRotLeft(&ValArr, sizeof(ValArr), sizeof(v128));}     // Align to v256
           if(Size >= sizeof(v256))
            {
             if((size_t)Dst & sizeof(v256)){Size=StoreAs<v256>(*(v256*)&ValArr, &Dst, Size); if constexpr(sizeof(T) > sizeof(v256))MemRotLeft(&ValArr, sizeof(ValArr), sizeof(v256));}    // Align to v512
             if(Size >= sizeof(v512))   // Requires AVX512 on x86
              {
               while(Size >= sizeof(v512))Size=StoreAs<v512>(*(v512*)&ValArr, &Dst, Size);    // TODO: Emulate (constexpr) if no 64-byte vector instructions present
              }
             if(Size >= sizeof(v256)){Size=StoreAs<v256>(*(v256*)&ValArr, &Dst, Size); if constexpr(sizeof(T) > sizeof(v256))MemCopyMSync<sizeof(v256)>(&ValArr, (uint8*)&ValArr + sizeof(v256), sizeof(v256));}  // Tail as v256
            }
           if(Size >= sizeof(v128)){Size=StoreAs<v128>(*(v128*)&ValArr, &Dst, Size); if constexpr(sizeof(T) > sizeof(v128))MemCopyMSync<sizeof(v128)>(&ValArr, (uint8*)&ValArr + sizeof(v128), sizeof(v128));}    // Tail as v128
          }
         if(Size >= sizeof(uint64)){Size=StoreAs<uint64>(*(uint64*)&ValArr, &Dst, Size); if constexpr(sizeof(T) > sizeof(uint64))MemCopyMSync<sizeof(uint64)>(&ValArr, (uint8*)&ValArr + sizeof(uint64), sizeof(uint64));} // Tail as u64
        }
// --- VECTORIZED
       else {while(Size >= sizeof(uint64))Size=StoreAs<uint64>(*(uint64*)&ValArr, &Dst, Size);}   // u64 (LOOP)
      }
     if(Size >= sizeof(uint32)){Size=StoreAs<uint32>(*(uint32*)&ValArr, &Dst, Size); if constexpr(sizeof(T) > sizeof(uint32))MemCopyMSync<sizeof(uint32)>(&ValArr, (uint8*)&ValArr + sizeof(uint32), sizeof(uint32));} // Tail as u32
    }
   if(Size >= sizeof(uint16)){Size=StoreAs<uint16>(*(uint16*)&ValArr, &Dst, Size); if constexpr(sizeof(T) > sizeof(uint16))MemCopyMSync<sizeof(uint16)>(&ValArr, (uint8*)&ValArr + sizeof(uint16), sizeof(uint16));}  // Tail as u16
  }
 if(Size >= sizeof(uint8))Size=StoreAs<uint8>(*(uint8*)&ValArr, &Dst, Size);  // Tail as u8
 return Dst;
}
//---------------------------------------------------------------------------
// Returns number of matched bytes
constexpr static size_t MemCmp(void* _RST Dst, void* _RST Src, size_t Size)
{
 return 0;
}
//---------------------------------------------------------------------------
/*static void TestMemCpy(void)
{
 alignas(sizeof(uint8))   uint8 TestStr1[]  = {"(1) Hello mem test World (1)!"};
 alignas(sizeof(uint16))  uint8 TestStr2[]  = {"(2) Hello mem test World (2)!"};
 alignas(sizeof(uint32))  uint8 TestStr4[]  = {"(4) Hello mem test World (4)!"};
 alignas(sizeof(uint64))  uint8 TestStr8[]  = {"(8) Hello mem test World (8)!"};
 alignas(sizeof(v128))    uint8 TestStr16[] = {"(16) Hello mem test World (16)!"};
 alignas(sizeof(v256))    uint8 TestStr32[] = {"(32) Hello mem test World (32)!"};
 alignas(sizeof(v512))    uint8 TestStr64[] = {"(64) Hello mem test World (64)!"};

 alignas(256) volatile uint8 TstBuf[1024] = {0};
 MemCopy((void*)&TstBuf[0xAF],&TestStr1,0x1A);
 MemCopy((void*)&TstBuf[0x76],(void*)&TstBuf[0xAF],0x1A);

 MemCopy((void*)&TstBuf,&TestStr2,sizeof(TestStr2));
 MemCopy((void*)&TstBuf,&TestStr4,sizeof(TestStr4));
 MemCopy((void*)&TstBuf,&TestStr8,sizeof(TestStr8));
 MemCopy((void*)&TstBuf,&TestStr16,sizeof(TestStr16));
 MemCopy((void*)&TstBuf,&TestStr32,sizeof(TestStr32));
 MemCopy((void*)&TstBuf,&TestStr64,sizeof(TestStr64));
}*/
//---------------------------------------------------------------------------

}; // NMOPS

// NOTE: Compiler expects those to be demangled AND with external linkage! (No inlining for them, at all!)
//       Use '-ffunction-sections' + '-Wl,-gc-sections' to remove unused external-linkage functions
// NOTE: Will suppress force_inline
// TODO: Get rid of div and mult
// TODO: Align the addresses and use xmm for copy operations
extern "C"
{

void* memcpy(void* Dst, const void* Src, size_t Size)  // static
{
 return NMOPS::MemCopy(Dst, (void*)Src, Size);
}
//---------------------------------------------------------------------------
void* memmove(void* Dst, const void* Src, size_t Size)   // Fixed but inefficient
{
 return NMOPS::MemMove(Dst, (void*)Src, Size);
}
//---------------------------------------------------------------------------
// NOTE: memset will not be inlined and Val is never expanded at compile time when zero to MemZero
// Is there a way to inline and optimize it?
void* memset(void* Dst, const unsigned int Val, size_t Size)      // TODO: Aligned, SSE by MACRO   _EXTERNC
{
 return NMOPS::MemFill<uint8>(Dst,Size,(uint8)Val);
}
//---------------------------------------------------------------------------
// TODO: Need a function which returns number of matched bytes, not just diff of a last unmatched byte
int memcmp(const void* Buf1, const void* Buf2, size_t Size)  // No alignment handling!
{
 const unsigned char* BufA = (const unsigned char*)Buf1;
 const unsigned char* BufB = (const unsigned char*)Buf2;
 for(;Size >= sizeof(size_t); Size-=sizeof(size_t), BufA+=sizeof(size_t), BufB+=sizeof(size_t))  // Enters here only if Size >= sizeof(ULONG)
  {
   if(*((const size_t*)BufA) != *((const size_t*)BufB))break;  // Have to break and continue as bytes because return value must be INT  // return (*((intptr_t*)BufA) - *((intptr_t*)BufB));  //  // TODO: Move everything to multiplatform FRAMEWORK
  }
 for(;Size > 0; Size--, BufA++, BufB++)  // Enters here only if Size > 0
  {
   if(*((const unsigned char*)BufA) != *((const unsigned char*)BufB)){return ((int)*BufA - (int)*BufB);}
  }
 return 0;
}
//---
}
//===========================================================================
