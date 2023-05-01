
#pragma once
/*
  Copyright (c) 2023 Victor Sheinmann, Vicshann@gmail.com

  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"),
  to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
  and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

//=================================================================================================================
// Compresses into a string of bytes, hiding zero bytes
// A repeated byte is a marker. Two same bytes is a waste, will become three. But no need for best marker byte search.
// Dups more than 67mb is unsupported! No checks!
// Encode dups starting from 3 bytes (will take 4 bytes least)
//
class CStrRLE
{
 unsigned char* DatPtr  = nullptr;
 unsigned int   DatSize = 0;
 unsigned int   DatOffs = 0;
 unsigned int   SameCtr = 0;
 unsigned int   DupsCtr = 0;
 unsigned int   DtSrc32 = -1;
 unsigned char  MinDups = 4;     // 3 will encode into 3 which is useless // TODO: Find with marker?
 unsigned char  Marker  = 0xD9;  // 0xDDD9;
 unsigned char  NSByte  = 0xDD;  // MrkBytes >> 8;
 unsigned char  LastVal = 0;

//-----------------------------------------------------------------------------------------------------------------
// 8-bit bytes to 7-bit bytes. Highest byte is always OK (Never zero)
static unsigned int EncodeCounter(unsigned int Ctr, unsigned int CtrSize)
{
 unsigned int Val = 0;
 for(unsigned int idx=0;idx < CtrSize;idx++)Val |= ((Ctr << idx) & (0x7F << (8*idx)));
 return Val;
}
//-----------------------------------------------------------------------------------------------------------------
static unsigned int DecodeCounter(unsigned int Ctr, unsigned int CtrSize)
{
 unsigned int Val = 0;
 for(unsigned int idx=0;idx < CtrSize;idx++)Val |= ((Ctr & (0x7F << (8*idx))) >> idx);
 return Val;
}
//-----------------------------------------------------------------------------------------------------------------
static unsigned int UpdateCrc32(unsigned char Val, unsigned int Crc, unsigned int Count=1)
{
 unsigned int constexpr msk = 0xEDB88320;
 for(unsigned int ctr=0;ctr < Count;ctr++)
  {
   Crc = Crc ^ Val;
   for(unsigned i=8;i;i--)Crc = (Crc >> 1) ^ (msk & -(Crc & 1));
  }
 return Crc;  // NOTE: Final must be inverted
}
//-----------------------------------------------------------------------------------------------------------------
// Not completely correct but consumes little of memory
// Crc of uncompressed data must be known before it is compressed because we may send it before the compressed data
static unsigned short FindMarker(unsigned char* Data, unsigned int Size, unsigned int* Crc)
{
 unsigned short MArr[256] = {};
 unsigned int   Mask[8] = {(unsigned int)-1,(unsigned int)-1,(unsigned int)-1,(unsigned int)-1,(unsigned int)-1,(unsigned int)-1,(unsigned int)-1,(unsigned int)-1};   // 256 bits
 for(unsigned int ctr=0;ctr < Size;ctr++)
  {
   unsigned char Idx = Data[ctr];
   if(Crc)*Crc = UpdateCrc32(Idx, *Crc);
   if(MArr[Idx] == 0xFFFF)continue;
   MArr[Idx]++;
   if(MArr[Idx] == 0xFFFF)     // Saturated
    {
     int MIdx = Idx >> 5;      // '/ 32'
     int BIdx = Idx & 0x1F;
     Mask[MIdx] &= ~(1 << BIdx);   // Set to 0
     unsigned int MSum = Mask[0]|Mask[1]|Mask[2]|Mask[3]|Mask[4]|Mask[5]|Mask[6]|Mask[7];
     if(!MSum)  // Reset saturation   // TODO: Decrement when all 256 are >1    // Decrement by min when first FF is encountered
      {
       Mask[0]=Mask[1]=Mask[2]=Mask[3]=Mask[4]=Mask[5]=Mask[6]=Mask[7]= (unsigned int)-1;
       for(unsigned int idx=0;idx < sizeof(MArr);idx++)MArr[idx] = 0;
      }
    }
  }
 unsigned int  MinVal = -1;
 unsigned char Marker = 0;
 unsigned char PrvMarker = 0;
 for(unsigned int ctr=1;ctr < 255;ctr++)   // Skip 0x00 and 0xFF, they are special
  {
   unsigned int Val = MArr[ctr];
   if(Val < MinVal)
    {
     PrvMarker = Marker;
     MinVal = Val;
     Marker = ctr;
    }
  }
 return (Marker << 8)|PrvMarker;   // HiByte is Null replace byte and usually it is more frequent
}
//-----------------------------------------------------------------------------------------------------------------
void EncodeDups(unsigned char* Dst, unsigned int& DstOffs, bool NeedMask)
{
 unsigned int CCtr;
 int Number = this->SameCtr - 1; // Counter of 0 is 1 byte
 if(Number < 0x20)CCtr = 1;
 else
  {
   if(Number < 0x1000)CCtr = 2;
   else if(Number < 0x80000)CCtr = 3;
   else CCtr = 4;   // 4 repeats as is to flag the counter
   Number = EncodeCounter(Number, CCtr) | (0x00808080 >> (8*(4-CCtr)));
  }
 unsigned int BShift = ((CCtr-1)*8);
 Number |= ((CCtr-1) << (BShift+5));    // ICCNNNNN...
 if(NeedMask)   // Max ctr is: 0x1F(1F), 0x1F7F(0FFF), 0x1F7F7F(07FFFF), 0x1F7F7F7F(03FFFFFF)    // High bit is always 1 for counters > 1 byte to avoid Nulls
  {
   Number |= (0x80 << BShift);  // Set Invert marker for Zero/Mrk byte in HighByte
   if(!this->LastVal)this->LastVal = ~this->LastVal;
  }
 Dst[DstOffs++] = this->Marker;
 Dst[DstOffs++] = this->LastVal;
 for(int ctr=CCtr-1;ctr >= 0;ctr--)Dst[DstOffs++] = Number >> (8 * ctr);     // Store byte count
}
//-----------------------------------------------------------------------------------------------------------------
void StoreCompressed(unsigned char* Dst, unsigned int& DstOffs)
{
 bool NeedMask = !this->LastVal || (this->LastVal == this->Marker);
 if((this->SameCtr >= this->MinDups) || NeedMask)    // May check LastVal by list if other bytes need to be inverted  // Encode 0x00 and marker bytes with any count
  {
   if(this->SameCtr)this->EncodeDups(Dst, DstOffs, NeedMask);
  }
  else for(unsigned int ctr=0;ctr < this->SameCtr;ctr++)Dst[DstOffs++] = this->LastVal;       // Save single bytes
}
//-----------------------------------------------------------------------------------------------------------------
unsigned char RevZeroVal(unsigned char Val)
{
 if(Val == 0x00)Val = this->NSByte;
   else if(Val == this->NSByte)Val = 0x00;
 return Val;
}
//-----------------------------------------------------------------------------------------------------------------

public:

//-----------------------------------------------------------------------------------------------------------------
unsigned int GetCrc(void){return ~this->DtSrc32;}
//-----------------------------------------------------------------------------------------------------------------
void SetDataForCompr(void* Data, unsigned int DataSize)
{
 this->DtSrc32 = -1;
 this->DatPtr  = (unsigned char*)Data;
 unsigned short Mrk = FindMarker(this->DatPtr, DataSize, &this->DtSrc32);
 this->Marker  = Mrk;
 this->NSByte  = Mrk >> 8;
 this->DatSize = DataSize;
 this->DatOffs = 0;
 this->LastVal = *this->DatPtr;
 this->SameCtr = 0;
 this->DupsCtr = 0;
}
//-----------------------------------------------------------------------------------------------------------------
void SetDataForDeCompr(void* Data, unsigned int DataSize)
{
 this->DatPtr  = (unsigned char*)Data;
 this->Marker  = this->DatPtr[0];
 this->NSByte  = this->DatPtr[1];
 this->DatSize = DataSize - 2;
 this->DatOffs = 0;
 this->LastVal = 0;
 this->SameCtr = 0;
 this->DupsCtr = 0;  // For decompression
 this->DtSrc32 = -1;
 this->DatPtr += 2;
}
//-----------------------------------------------------------------------------------------------------------------
// Encode bytes to store in zero-terminated strings
// Expect Dst data to be max DstSize+8
unsigned int CompressBlk(unsigned char* Dst, unsigned int DstSize)
{
 unsigned int DstOffs = 0;
 if(!this->DatOffs)
  {
   Dst[DstOffs++] = this->Marker;
   Dst[DstOffs++] = this->NSByte;
  }
 while(this->DatOffs < this->DatSize)
  {
   unsigned char Val = this->RevZeroVal(this->DatPtr[this->DatOffs++]);
   if(Val == LastVal){SameCtr++; continue;}
   this->StoreCompressed(Dst, DstOffs);
   LastVal = Val;
   SameCtr = 1;
   if(DstOffs >= DstSize)return DstOffs;
  }
 this->StoreCompressed(Dst, DstOffs);
 SameCtr = 0;
 return DstOffs;
}
//-----------------------------------------------------------------------------------------------------------------
static unsigned int CalcDecompBufSize(unsigned char* Src, unsigned int SrcSize)
{
 unsigned int  SrcOffs = 2;
 unsigned int  DstOffs = 0;
 unsigned char MrkByte = *Src;
 while(SrcOffs < SrcSize)
  {
   unsigned char Val = Src[SrcOffs++];
   if(Val == MrkByte)   // RLE marker
    {
     SrcOffs++;  // Value
     Val = Src[SrcOffs++];  // Counter and flags
     unsigned int CtrSize = (Val >> 5) & 3;
     unsigned int CtrVal  = Val & 0x1F;
     if((SrcOffs+CtrSize) > SrcSize)break;
     for(unsigned int ctr=0;ctr < CtrSize;ctr++)CtrVal = (CtrVal << 8)|Src[SrcOffs++];
     CtrVal = DecodeCounter(CtrVal, CtrSize+1);
   //  DBGMSG("Dups %08X at offs %08X",CtrVal,SrcOffs-(CtrSize+(3+1)));
     DstOffs += CtrVal+1;
    }
     else DstOffs++;
  }
 return DstOffs;
}
//-----------------------------------------------------------------------------------------------------------------
unsigned int DeCompressBlk(unsigned char* Dst, unsigned int DstSize)
{
 unsigned int DstOffs = 0;
 for(;;)
  {
   if(!this->DupsCtr)
    {
     if(this->DatOffs >= this->DatSize)break;
     unsigned char Val = this->RevZeroVal(this->DatPtr[this->DatOffs++]);
     if(Val == this->Marker)   // RLE marker
      {
       this->LastVal = this->DatPtr[this->DatOffs++];
       Val = this->DatPtr[this->DatOffs++];  // Counter and flags
       if((Val & 0x80)&&(this->LastVal != this->Marker))this->LastVal = ~this->LastVal;   // Restore Null   // Maker counters are also may be 0x00
       this->LastVal = this->RevZeroVal(this->LastVal);
       unsigned int CtrSize = (Val >> 5) & 3;
       unsigned int CtrVal  = Val & 0x1F;
       if((this->DatOffs+CtrSize) > this->DatSize)break;   // Thoise checks are costly: Always assyme that the data is correct?
       for(unsigned int ctr=0;ctr < CtrSize;ctr++)CtrVal = (CtrVal << 8)|this->DatPtr[this->DatOffs++];
       this->DupsCtr = DecodeCounter(CtrVal, CtrSize+1) + 1;
    //   DBGMSG("Dups %08X at offs %08X",CtrVal,this->DatOffs-(CtrSize+(3+1)));
      }
       else
        {
         Dst[DstOffs++] = Val;   // Store single byte
         this->DtSrc32 = UpdateCrc32(Val, this->DtSrc32);
         if(DstOffs >= DstSize)return DstOffs;
        }
    }
   while(this->DupsCtr)
    {
     this->DupsCtr--;
     Dst[DstOffs++] = this->LastVal;
     this->DtSrc32 = UpdateCrc32(this->LastVal, this->DtSrc32);
     if(DstOffs >= DstSize)return DstOffs;
    }
  }
 return DstOffs;
}
//-----------------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------
};
//-----------------------------------------------------------------------------------------------------------------
/* Example:

   {
    CStrRLE rle;
    CMiniStr src;
    CMiniStr dst;
    unsigned char tmp[300];

    src.FromFile("Data.bin");
    rle.SetDataForCompr(src.c_data(), src.Length());
    for(;;)
     {
      unsigned int DLen = rle.CompressBlk(tmp, 256);
      if(!DLen)break;
      dst.cAppend((char*)&tmp, DLen);
      dst.ToFile("Data.rle");
     }
    UINT RSize =  CStrRLE::CalcDecompBufSize(dst.c_data(), dst.Length());

    src.Clear();
    rle.SetDataForDeCompr(dst.c_data(), dst.Length());
    for(;;)
     {
      unsigned int DLen = rle.DeCompressBlk(tmp, 256);
      if(!DLen)break;
      src.cAppend((char*)&tmp, DLen);
      src.ToFile("Data.src");
     }
   }

*/
//-----------------------------------------------------------------------------------------------------------------
