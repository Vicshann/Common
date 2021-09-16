
#pragma once
/*
  Copyright (c) 2021 Victor Sheinmann, Vicshann@gmail.com

  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), 
  to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
  and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. 
*/

//---------------------------------------------------------------------------

class NProtoBuf
{
enum EPBType {pbtVarInt, pbtX64V, pbtData, pbtGrBeg, pbtGrEnd, pbtX32V};   // NOTE: pbtGrBeg and pbtGrEnd are deprecated

//---------------------------------------------------------------------------
static UINT64 GetFieldNumb(UINT64 Val){return Val >> 3;}
static UINT64 GetFieldType(UINT64 Val){return Val & 7;}
//---------------------------------------------------------------------------
static UINT64 ReadVarIntRaw(PBYTE& Stream, SIZE_T Left)
{
 UINT64 Res = 0;
 UINT Shift = 0;
 for(;Left;Left--,Shift+=7)
  {
   BYTE Val = *(Stream++); 
   Res |= ((UINT64)(Val & 0x7F)) << Shift;
   if(!(Val & 0x80))return Res; // Finished
  }
 Stream++;  // Make it overflow to indicate incompleteness
 return Res;
}
//---------------------------------------------------------------------------
static bool DumpHexDataBlock(int Depth, PBYTE Data, SIZE_T Size, char*& DstBuf, SIZE_T& DstSize)  // Max row len is 128 bytes
{
 BYTE Buffer[256+128+4];
 UINT RowLen = 32;
 for(UINT RSize=0;Size;Data+=RSize,Size-=RSize)
  {
   RSize = (RowLen > Size)?(Size):(RowLen);
   PBYTE DPtr  = (PBYTE)&Buffer;
   for(UINT ctr=0;ctr < RSize;ctr++)    // Create HEX string
    {
     WORD Val  = HexToChar(Data[ctr]);
     *(DPtr++) = Val;
     *(DPtr++) = Val >> 8;
    }
   for(UINT ctr=RSize;ctr < RowLen;ctr++,DPtr+=2)*((PWORD)DPtr) = 0x2020;   // Fill left space // Use memset?
   *(DPtr++) = 0x20;
   *(DPtr++) = 0x20;
   for(UINT ctr=0;ctr < RSize;ctr++)   // Create Text string
    {
     BYTE Val  = Data[ctr];
     if(Val < 0x20)Val = '.';
     *(DPtr++) = Val;
    }
   *(DPtr++) = '\r';
   *(DPtr++) = '\n';
   SIZE_T LineLen = (DPtr - (PBYTE)&Buffer);
   FillChars(' ', Depth, DstBuf, DstSize);
   if(!DstSize)return false;
   if(LineLen > DstSize)LineLen = DstSize;
   memcpy(DstBuf, &Buffer, LineLen);
   DstBuf  += LineLen;
   DstSize -= LineLen;
   *DstBuf  = 0;
   if(!DstSize)return false;
  }
 return true;
}
//---------------------------------------------------------------------------
static SIZE_T FillChars(char val, UINT Num, char*& DstBuf, SIZE_T& DstSize)
{
 if(Num > DstSize)Num = DstSize;
 for(UINT ctr=0;ctr < Num;ctr++,DstBuf++)*DstBuf = val;
 DstSize -= Num;
 *DstBuf = 0;
 return Num;
}
//---------------------------------------------------------------------------
static UINT PrintFieldHdr(int Depth, UINT64 Field, char* Name, char*& DstBuf, SIZE_T& DstSize)
{
 SIZE_T OldDstSize = DstSize;
 FillChars(' ', Depth, DstBuf, DstSize);
 if(DstSize < 16)return 0;
 int Len = PrintFmt(DstBuf, DstSize, "%-4llu <%s> = ", Field, Name);
 DstBuf  += Len;
 *DstBuf = 0;
 if((SIZE_T)Len >= DstSize)return 0;
 DstSize -= Len;
 return OldDstSize - DstSize;
}
//---------------------------------------------------------------------------
static bool PrintFieldInt(int Depth, UINT64 Field, UINT64 Value, char*& DstBuf, SIZE_T& DstSize)
{
 if(!PrintFieldHdr(Depth, Field, "varint", DstBuf, DstSize))return false;
 if(DstSize < 16)return 0;
 int Len = PrintFmt(DstBuf, DstSize, "%lli\r\n", Value);
 DstBuf  += Len;
 *DstBuf  = 0;
 if((SIZE_T)Len >= DstSize)return false;
 DstSize -= Len;
 return true;
}
//---------------------------------------------------------------------------
static bool PrintField32(int Depth, UINT64 Field, UINT32 Value, char*& DstBuf, SIZE_T& DstSize)
{
 if(!PrintFieldHdr(Depth, Field, "32bit", DstBuf, DstSize))return false;
 if(DstSize < 64)return 0;
 int VInt = *(int*)&Value;
 float VFlt = *(float*)&Value;
 int Len = PrintFmt(DstBuf, DstSize, "%08X / %u / %i / %f\r\n", Value, Value, VInt, VFlt);
 DstBuf  += Len;
 *DstBuf  = 0;
 if((SIZE_T)Len >= DstSize)return false;
 DstSize -= Len;
 return true;
}
//---------------------------------------------------------------------------
static bool PrintField64(int Depth, UINT64 Field, UINT64 Value, char*& DstBuf, SIZE_T& DstSize)
{
 if(!PrintFieldHdr(Depth, Field, "64bit", DstBuf, DstSize))return false;
 if(DstSize < 64)return 0;
 int VInt = *(int*)&Value;
 float VFlt = *(float*)&Value;
 int Len = PrintFmt(DstBuf, DstSize, "%08X / %llu / %lli / %llf\r\n", Value, Value, VInt, VFlt);
 DstBuf  += Len;
 *DstBuf  = 0;
 if((SIZE_T)Len >= DstSize)return false;
 DstSize -= Len;
 return true;
}
//---------------------------------------------------------------------------
static bool PrintFieldRaw(int Depth, UINT64 Field, PBYTE Data, SIZE_T Size, char*& DstBuf, SIZE_T& DstSize)
{
 int Len = PrintFmt(DstBuf, DstSize, "bytes(%lli):\r\n", Size);
 DstBuf  += Len;
 *DstBuf  = 0;
 if((SIZE_T)Len >= DstSize)return false;
 DstSize -= Len;
 if(Size && DstSize)return DumpHexDataBlock(Depth+8, Data, Size, DstBuf, DstSize); 
 return true;
}
//------------------------------------------------------------------------------------------------------------
// https://developers.google.com/protocol-buffers/docs/encoding
//
static int DumpProtoBufMsgIntrn(PBYTE Data, SIZE_T Size, char*& DstBuf, SIZE_T& DstSize, int Depth)  // Recursive
{
 if(!IsValidMessage(Data, Size))return -1;     // Invalid ProtoBuf message
 if(DstBuf && DstSize && (Depth > 0))
  {
   if(DstSize <= 30)return -10;   // Not enough Dst space       // 20 chars is max int64 number
   int Len = PrintFmt(DstBuf, DstSize, "message(%lli):\r\n", Size);
   DstBuf  += Len;
   DstSize -= Len;
   *DstBuf = 0;
  }
 PBYTE SrcDEnd = &Data[Size];
 while(Data < SrcDEnd)
  {
   UINT64 Tag = ReadVarIntRaw(Data, SrcDEnd-Data);
   if(Data >= SrcDEnd)return -3;  // Source data is incomplete for the tag
   UINT64 FieldNum = GetFieldNumb(Tag);
   UINT64 FieldTyp = GetFieldType(Tag);
   SIZE_T SrcDLeft = SrcDEnd-Data;
//   DBGMSG("Data=%p, FieldNum=%u, FieldTyp=%u",Data,FieldNum,FieldTyp);
   switch(FieldTyp)
    {
     case pbtVarInt:
      {
       UINT64 Val = ReadVarIntRaw(Data, SrcDLeft);
       if(Data > SrcDEnd)return -4; 
       if(DstBuf && DstSize){if(!PrintFieldInt(Depth, FieldNum, Val, DstBuf, DstSize))return -11;}
      }
      break;
     case pbtX64V:
      if(SrcDLeft < 8)return -5;
      if(DstBuf && DstSize){if(!PrintField64(Depth, FieldNum, *(UINT64*)Data, DstBuf, DstSize))return -12;}
      Data += 8;
      break;
     case pbtData:
      {
       UINT64 Len = ReadVarIntRaw(Data, SrcDLeft);
       if(Data > SrcDEnd)return -6; 
       if(DstBuf && DstSize){if(!PrintFieldHdr(Depth, FieldNum, "chunk", DstBuf, DstSize))return -13;}
       if(&Data[Len] > SrcDEnd)return -2;  // Incoomplete, will not pass validation anyway
       int res = DumpProtoBufMsgIntrn(Data, Len, DstBuf, DstSize, Depth+2);
       if(res < 0)   // Not dumped as a message
        {
         if(res != -1)Len = SrcDEnd-Data;  // Dump as much of incomplete data as we have
         if(DstBuf && DstSize){if(!PrintFieldRaw(Depth, FieldNum, Data, Len, DstBuf, DstSize))return -14;}
        }
       Data += Len;
      }
      break;
     case pbtX32V:
      if(SrcDLeft < 4)return -7; 
      if(DstBuf && DstSize){if(!PrintField32(Depth, FieldNum, *(UINT32*)Data, DstBuf, DstSize))return -15;}
      Data += 4;
      break;
     default: return -1;  // Invalid
    }
   if(DstBuf && !DstSize)return -10; // Not enough Dst space
  }
 return 0;
}
//------------------------------------------------------------------------------------------------------------

public:
//---------------------------------------------------------------------------
static bool IsValidMessage(PBYTE Data, SIZE_T Size)   // Not recursive
{
 PBYTE SrcDEnd = &Data[Size];
 UINT64 PrvFieldNum = 0;
 while(Data < SrcDEnd)
  {
   UINT64 Tag = ReadVarIntRaw(Data, SrcDEnd-Data);
   if(Data >= SrcDEnd)return false;  // Incomplete   
   UINT64 FieldTyp = GetFieldType(Tag);
   SIZE_T DLeft = SrcDEnd-Data;
   switch(FieldTyp)
    {
     case pbtVarInt:       
      ReadVarIntRaw(Data, DLeft);
      if(Data > SrcDEnd)return false;  // Incomplete 
      break;
     case pbtX64V:
      if(DLeft < 8)return false;  // Incomplete
      Data += 8;
      break;
     case pbtData:
      {
       UINT64 Len = ReadVarIntRaw(Data, DLeft);
       if(Data > SrcDEnd)return false;  // Incomplete
       Data += Len; 
       if(Data > SrcDEnd)return false;  // Incomplete
      }
      break;
     case pbtX32V:
      if(DLeft < 4)return false;  // Incomplete
      Data += 4;
      break;
     default: return false;  // Invalid
    }
   UINT64 FieldNum = GetFieldNumb(Tag);
   if(!FieldNum)return false;  // Invalid // Fields always start from 1?
   if(PrvFieldNum && (FieldNum < PrvFieldNum))return false;   // Invalid: broken sequence   // Are fields always sequential  // NOTE: not 'FieldNum <= PrvFieldNum' because there are duplicate field numbers encountered sometimes
   PrvFieldNum = FieldNum;
  } 
 return true;
}
//---------------------------------------------------------------------------
static int DumpMessage(PBYTE Data, SIZE_T Size, char* DstBuf, SIZE_T DstSize)  
{
 char* OldDst = DstBuf;
 int res = DumpProtoBufMsgIntrn(Data, Size, DstBuf, DstSize, 0);
 if(res == 0)return DstBuf - OldDst;
 return res;
}
//---------------------------------------------------------------------------
// Path: '2:6:9:1'  // Fields count from 1
// If AbsIdx is true then absolute indexes are used instead of fields` indexes
static int GetFieldBodyOffset(PBYTE Data, SIZE_T Size, char* Path, SIZE_T* FldSize, bool AbsIdx)   // Recursive
{
 long SPLen = 0;
 UINT CurFIdx = DecStrToNum<UINT>(Path, &SPLen);
 UINT FldCtr = 0;
 PBYTE SrcDBeg = Data; 
 PBYTE SrcDEnd = &Data[Size];
 UINT64 PrvFieldNum = 0;
 Path += SPLen;
 while(*Path && ((*Path < '0')||(*Path > '9')))Path++;
 while(Data < SrcDEnd)
  {
   UINT64 Tag = ReadVarIntRaw(Data, SrcDEnd-Data);
   if(Data >= SrcDEnd)return -1;  // Incomplete   
   UINT64 FieldNum = GetFieldNumb(Tag);
   UINT64 FieldTyp = GetFieldType(Tag);
   SIZE_T DLeft = SrcDEnd-Data;   
   PBYTE  DPrv = Data; 
   if(!FieldNum)return -2;  // Invalid // Fields always start from 1?
   if(PrvFieldNum && (FieldNum <= PrvFieldNum))return -2;   // Invalid: broken sequence   // Are fields always sequential
   PrvFieldNum = FieldNum;
   if((AbsIdx && (FldCtr > CurFIdx))||(!AbsIdx && (FieldNum > CurFIdx)))return -3;   // Index not found
   switch(FieldTyp)
    {
     case pbtVarInt:       
      ReadVarIntRaw(Data, DLeft);
      if(Data > SrcDEnd)return -1;  // Incomplete 
      if((AbsIdx && (FldCtr == CurFIdx))||(!AbsIdx && (FieldNum == CurFIdx))){if(FldSize)*FldSize = Data - DPrv; return Data - SrcDBeg;}
      break;
     case pbtX64V:
      if(DLeft < 8)return -1;  // Incomplete
      if((AbsIdx && (FldCtr == CurFIdx))||(!AbsIdx && (FieldNum == CurFIdx))){if(FldSize)*FldSize = 8; return Data - SrcDBeg;}
      Data += 8;
      break;
     case pbtData:
      {
       UINT64 Len = ReadVarIntRaw(Data, DLeft);
       if(Data > SrcDEnd)return -1;  // Incomplete      
       if((AbsIdx && (FldCtr == CurFIdx))||(!AbsIdx && (FieldNum == CurFIdx)))
        {
         if(!*Path){if(FldSize)*FldSize = Len; return Data - SrcDBeg;}         
         int offs = GetFieldBodyOffset(Data, DLeft, Path, FldSize, AbsIdx);
         if(offs >= 0)return (Data - SrcDBeg) + offs;    // Pass the offset 
        }
       Data += Len; 
       if(Data > SrcDEnd)return -1;  // Incomplete
      }
      break;
     case pbtX32V:
      if(DLeft < 4)return -1;  // Incomplete
      if((AbsIdx && (FldCtr == CurFIdx))||(!AbsIdx && (FieldNum == CurFIdx))){if(FldSize)*FldSize = 4; return Data - SrcDBeg;}
      Data += 4;
      break;
     default: return -2;  // Invalid
    }
   FldCtr++;
  } 
 return -3;
}
//---------------------------------------------------------------------------

};
//---------------------------------------------------------------------------
