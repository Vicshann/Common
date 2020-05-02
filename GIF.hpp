
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

#include "RGBQuantOT.hpp"
//---------------------------------------------------------------------------
// https://github.com/deanm/omggif
// https://github.com/charlietangora/gif-h   // K-D tree, change detection
//
// NOTE: CRGBOTQuantizer is too slow for realtime screen recording
//
class CGIF
{
#pragma pack(push,1)
struct SGifHdr   //  Little endian platforms only   // GCC: struct __attribute__((packed, scalar_storage_order("big-endian"))) mystruct  // No custom INT class can help with bitfields
{
 char   TagVer[6];   // GIF89a
 UINT16 CanvasWidth;
 UINT16 CanvasHeight;
 struct      // Color tables can contain 2, 4, 8, 16, 32, 64, 128, 256 entries of 3 bytes (RGB)
  {
   UINT8  ClrTblSize : 3;  // The number of bits used for each color table entry minus one; if the color table has 256 entries, each entry would require an 8-bit index, so this field would contain 7 (111 in binary). 
   UINT8  SortFlag   : 1;  // Unused, set to 0
   UINT8  ColorRes   : 3;  // A number from 0 to 7, indicating the significant bits per sample of each color in the table, minus one.
   UINT8  ClrTblFlag : 1;  // Glabal palette is present
  } Flags;
 UINT8  BgrClrIdx;     // Unused if GClrTblFlag is 0   // Background for the canvas
 UINT8  AspectRatio;   // Unused, set to 0
 UINT8  Palette[0];    // Min 2 colors
};
//---------------------------------------------------------------------------
struct SGifImgDesc    //  Little endian platforms only
{
 UINT16 Left;
 UINT16 Top;
 UINT16 Width;
 UINT16 Height;
 struct 
  {
   UINT8 ClrTblSize    : 3;
   UINT8 Reserved      : 2;
   UINT8 SortFlag      : 1;
   UINT8 InterlaceFlag : 1;
   UINT8 ClrTblFlag    : 1;
  } Flags;
 UINT8  Palette[0];   
};
//---------------------------------------------------------------------------
/* From the spec:
     0 -   No disposal specified. The decoder is
           not required to take any action.
     1 -   Do not dispose. The graphic is to be left
           in place.
     2 -   Restore to background color. The area used by the
           graphic must be restored to the background color.
     3 -   Restore to previous. The decoder is required to
           restore the area overwritten by the graphic with
           what was there prior to rendering the graphic.
  4-7 -    To be defined.
 NOTE: Dispose background doesn't really work, apparently most browsers ignore the background palette index and clear to transparency.
 */
struct SGifGrCtrl
{
 struct 
  {
   UINT8 TranspClrFlg   : 1;
   UINT8 UserInputFlg   : 1;
   UINT8 DisposalMethod : 3;
   UINT8 Reserved       : 3;
  } Flags;
 UINT16 DelayTime;
 UINT8  TranspClrIdx;
};
//---------------------------------------------------------------------------
public:
struct SGifToBmp   // Size is 4 bytes
{
 struct 
  {
   UINT8 DisposalMethod : 3;
   UINT8 TranspClrFlg   : 1;   // TranspClrIdx is valid
   UINT8 LocalClrTbl    : 1;   // The Palette was local
   UINT8 DelayTimeFlg   : 1;   // DelayTime is valid
   UINT8 Reserved       : 2;
  } Flags;
 UINT8  TranspClrIdx;
 UINT16 DelayTime;
};
private:
#pragma pack(pop)
//===========================================================================
template<typename T> class CSLList     // TODO: Move it to some generic hpp
{
 template<typename T> struct SValBlk
  {
   SValBlk<T>* Prev;
   SValBlk<T>* Next;
   unsigned int Size;  // Size of value + Extra Data
   T Value;
  };
 SValBlk<T>* FirstBlk;
 SValBlk<T>* LastBlk;

 static SValBlk<T>* ValPtrToItem(T* ValPtr){return (SValBlk<T>*)((char*)ValPtr - (char*)&((SValBlk<T>*)0)->Value);}
//---------------------------------------------------------------------------
void Remove(SValBlk<T>* Itm)
{
 if(Itm->Prev)Itm->Prev->Next = Itm->Next;
 if(Itm->Next)Itm->Next->Prev = Itm->Prev; 
 if(Itm == this->FirstBlk)this->FirstBlk = Itm->Next;     
 if(Itm == this->LastBlk)this->LastBlk = Itm->Prev;     
 delete[] (char*)Itm;              // free(Itm);     // Unoptimized for a known-sized block
}
//---------------------------------------------------------------------------

public:
//---------------------------------------------------------------------------
CSLList(void)
{
 this->FirstBlk = this->LastBlk = nullptr;
}
//---------------------------------------------------------------------------
~CSLList()
{
 this->Clear();
}
//---------------------------------------------------------------------------
void Clear(void)
{
 for(SValBlk<T>* Blk=this->FirstBlk;Blk;)
  {
   unsigned char* Tmp = (unsigned char*)Blk;
   Blk = Blk->Next;
   delete[] Tmp;      // free(Tmp);    // Unoptimized for a known-sized block
  }
 this->FirstBlk = this->LastBlk = nullptr;
}
//---------------------------------------------------------------------------
void Remove(T* Itm){this->Remove(ValPtrToItem(Itm));}
unsigned int SizeOf(T*){return ValPtrToItem(Curr)->Size;}
//---------------------------------------------------------------------------
T* Next(T* Curr){SValBlk<T>* Ptr = ValPtrToItem(Curr)->Next; return (Ptr)?(&Ptr->Value):nullptr;}
T* Prev(T* Curr){SValBlk<T>* Ptr = ValPtrToItem(Curr)->Prev; return (Ptr)?(&Ptr->Value):nullptr;}
//---------------------------------------------------------------------------
T* First(void){return (this->FirstBlk)?(&this->FirstBlk->Value):nullptr;}
T* Last(void){return (this->LastBlk)?(&this->LastBlk->Value):nullptr;};
//---------------------------------------------------------------------------
T* Add(unsigned int ExtraSize=0)
{
 SValBlk<T>* NewBlk = (SValBlk<T>*) new char[sizeof(SValBlk<T>) + ExtraSize];
 NewBlk->Size = ExtraSize + sizeof(T);
 if(this->LastBlk)
  {
   NewBlk->Next = nullptr;
   NewBlk->Prev = this->LastBlk;
   this->LastBlk->Next = NewBlk;
   this->LastBlk = NewBlk; 
  }
  else 
   {
    this->LastBlk = this->FirstBlk = NewBlk;
    NewBlk->Next = NewBlk->Prev = nullptr;   
   }
 return &NewBlk->Value;
}
//---------------------------------------------------------------------------

};
//===========================================================================
struct SLZWDictionary    // Size is 20K
{
 uint16_t minCodeSize;
 uint16_t codeLength;
 uint16_t clearCode;
 uint16_t eoiCode;
 uint16_t currentIndex;
 uint16_t maxCode;
 int16_t  prefix[4096];    // 8K
 int16_t  length[4096];    // 8K
 uint8_t  byteValue[4096]; // 4K

//---------------------------------------------------------------------------
void Reset(void) // Resets a dictionary
{
 const size_t dictionarySize = (1 << this->minCodeSize);
 this->codeLength   = this->minCodeSize + 1;
 this->clearCode    = dictionarySize;
 this->eoiCode      = this->clearCode + 1;
 this->currentIndex = this->clearCode + 2;
 this->maxCode      = (1 << this->codeLength) - 1;
}
//---------------------------------------------------------------------------
void Init(size_t MinCodeSize)
{
 this->minCodeSize = MinCodeSize;
 size_t dictionarySize  = (1 << MinCodeSize);
 for(size_t i = 0; i < dictionarySize; i++) 
  {
   this->prefix[i]    = -1;
   this->byteValue[i] = static_cast<uint8_t>(i);
   this->length[i]    = 1;
  }
 this->Reset();
}
//---------------------------------------------------------------------------
size_t Add(int prefix, uint8_t byteValue)
{
 auto& ci = this->currentIndex;
 if(ci < 4096) 
  {
   if((ci == this->maxCode) && (this->codeLength < 12)) 
    {
     ++this->codeLength;
     this->maxCode = (1 << this->codeLength) - 1;
    }
   this->prefix[ci]    = prefix;
   this->byteValue[ci] = byteValue;
   this->length[ci]    = ((prefix < 0) ? 0 : this->length[prefix]) + 1;        
   return ci++; // Return the index where the entry was inserted
  }
 return ci;
}
//---------------------------------------------------------------------------
inline UINT8 GetFirstByte(size_t index)  // Get the first byte associated with a dictionary entry
{
 while(this->prefix[index] != -1)index = this->prefix[index];
 return this->byteValue[index];
}
//---------------------------------------------------------------------------

};
//===========================================================================
struct SPalette
{
 UINT  ClrCount;
 CRGBOTQuantizer::RGBQUAD Palette[256];

 UINT CalcSize(UINT* SizeIdx)
 {
  UINT TPalSize   = 2;
  UINT PalSizeIdx = 0;
   while(PalSizeIdx < 8)  //  2, 4, 8, 16, 32, 64, 128, 256
    {
     if(this->ClrCount <= TPalSize)break;
     TPalSize <<= 1;
     PalSizeIdx++;   
    }
  *SizeIdx = PalSizeIdx; 
  return TPalSize;  // One of predefined sizes
 }
};
//---------------------------------------------------------------------------
public:
struct SDataBlk
{
 UINT  DataSize;
 UINT8 Data[0];
};
//---------------------------------------------------------------------------
class CImgBlk
{
 friend CGIF;

 UINT16   Width; 
 UINT16   Height; 
 UINT16   Top; 
 UINT16   Left;
 UINT8    DisposalMthd;    // If animated
 int      AlphaPresPerc;   // -1 If no alpha present
 int      TranspColor;     // -1 no transparency   // NRGB
 int      DelayTime;       // -1 if No animation required (Even if there are multiple images)
 int      TranspClrIdx;    // Only if animation is used
 bool     LocalClrTbl;
 UINT8    BytesPerPixel;
 UINT     PixelsSize;      // Size of 'Pixels' in bytes
 SPalette Palette;
 UINT8    Pixels[0];

public:
 void* GetPixels(void){return &this->Pixels;}
 void* GetRow(UINT16 RowIdx){return (RowIdx < this->Height)?(&this->Pixels[(this->Width * RowIdx) * this->BytesPerPixel]):(nullptr);}
 void  SetPixel(UINT32 ValRGBA, UINT16 PosX, UINT16 PosY){if((PosY < this->Height) && (PosX < this->Width))this->Pixels[(PosY*this->Width)+PosX] = ValRGBA;}
};

private:
 CSLList<CImgBlk> ImgList;
 CSLList<SDataBlk> BlkList;
 UINT     GClrTblRefs;
 int      CnvWidth;
 int      CnvHeight;
 int      LoopValue;
 int      BgrClrIdx;    // If global palette is present
 bool     HaveGifData;  // If a GIF loaded or already generated
 CRGBOTQuantizer::RGBQUAD BgrColor;   // For vancas creation
 SPalette GPalette; 

//---------------------------------------------------------------------------
static UINT LZWGifEncode(UINT8 min_code_size, UINT8* buf, UINT8* index_stream, UINT index_stream_length)
{
 UINT p = 0;
 buf[p++] = min_code_size;

 int cur_subblock = p++;  // Pointing at the length field.

 int clear_code = 1 << min_code_size;
 int code_mask  = clear_code - 1;
 int eoi_code   = clear_code + 1;
 int next_code  = eoi_code   + 1;

 int cur_code_size = min_code_size + 1;  // Number of bits per code.
 int cur_shift = 0; 
 int cur = 0;  // We have at most 12-bit codes, so we should have to hold a max of 19 bits here (and then we would write out).

auto emit_bytes_to_buffer = [&](int bit_block_size) 
{
 while(cur_shift >= bit_block_size) 
  {
   buf[p++] = cur & 0xff;
   cur >>= 8; cur_shift -= 8;
   if(p == (cur_subblock + 256))  // Finished a subblock.    //===
    { 
     buf[cur_subblock] = 255;
     cur_subblock = p++;
    }
  }
};
//--------------------------------------
auto emit_code = [&](int c) 
 {
  cur |= c << cur_shift;
  cur_shift += cur_code_size;
  emit_bytes_to_buffer(8);
 };
//--------------------------------------

 // Output code for the current contents of the index buffer.
 int ib_code = index_stream[0] & code_mask;  // Load first input index.
 short* code_table = new short [0x200000];   // 2Mb: Trading off memory for speed   // Key'd on our 20-bit "tuple".

 emit_code(clear_code);  // Spec says first code should be a clear code.
 memset(code_table, -1, 0x200000);   // -1 for unused entries

 for(UINT i = 1; i < index_stream_length; ++i)   // First index already loaded, process the rest of the stream.
  {
   int k = index_stream[i] & code_mask;
   int cur_key  = ib_code << 8 | k;     // (prev, k) unique tuple.
   int cur_code = code_table[cur_key];  // buffer + k.

    // Check if we have to create a new code table entry.
   if(cur_code == -1)  // We don't have buffer + k.
    {
     // Emit index buffer (without k).
     // This is an inline version of emit_code, because this is the core
     // writing routine of the compressor (and V8 cannot inline emit_code
     // because it is a closure here in a different context).  Additionally
     // we can call emit_byte_to_buffer less often, because we can have
     // 30-bits (from our 31-bit signed SMI), and we know our codes will only
     // be 12-bits, so can safely have 18-bits there without overflow.
     // emit_code(ib_code);
     cur |= ib_code << cur_shift;
     cur_shift += cur_code_size;
     while(cur_shift >= 8) 
      {
       buf[p++] = cur & 0xff;
       cur >>= 8; cur_shift -= 8;
       if(p == (cur_subblock + 256))   // Finished a subblock.
        {
         buf[cur_subblock] = 255;
         cur_subblock = p++;
        }
      }

     if(next_code == 4096)   // Table full, need a clear.
      {
       emit_code(clear_code);
       next_code = eoi_code + 1;
       cur_code_size = min_code_size + 1;
       memset(code_table, -1, 0x200000);  //  code_table = { };
      } else {  // Table not full, insert a new entry.
       // Increase our variable bit code sizes if necessary.  This is a bit
       // tricky as it is based on "timing" between the encoding and
       // decoder.  From the encoders perspective this should happen after
       // we've already emitted the index buffer and are about to create the
       // first table entry that would overflow our current code bit size.
       if (next_code >= (1 << cur_code_size)) ++cur_code_size;
       code_table[cur_key] = next_code++;  // Insert into code table.
      }

     ib_code = k;  // Index buffer to single input k.
    } 
     else ib_code = cur_code;  // Index buffer to sequence in code table.      
  }

 emit_code(ib_code);  // There will still be something in the index buffer.
 emit_code(eoi_code);  // End Of Information.

 // Flush / finalize the sub-blocks stream to the buffer.
 emit_bytes_to_buffer(1);

  // Finish the sub-blocks, writing out any unfinished lengths and
  // terminating with a sub-block of length 0.  If we have already started
  // but not yet used a sub-block it can just become the terminator.
  if((cur_subblock + 1) == p)buf[cur_subblock] = 0; // Started but unused.    
   else {  // Started and used, write length and additional terminator block.
    buf[cur_subblock] = p - cur_subblock - 1;
    buf[p++] = 0;
  }

 delete[] code_table;
 return p;
}
//---------------------------------------------------------------------------
SGifHdr* AddGifHeader(SPalette* Palette)  
{
 UINT TPalSize   = 0;
 UINT PalSizeIdx = 0;
 if(Palette)
  {
   if(Palette->ClrCount < 2)  
    {
     Palette->ClrCount = 2;
     Palette->Palette[0].Value = 0;  // Black
     Palette->Palette[1].Value = -1; // White
    }
   TPalSize = Palette->CalcSize(&PalSizeIdx);
  }

 UINT Size = sizeof(SGifHdr) + (TPalSize*3);
 SDataBlk* Blk = this->BlkList.Add(Size);
 Blk->DataSize = Size;
 memset(&Blk->Data, 0, Size);
 SGifHdr* GHdr = (SGifHdr*)&Blk->Data;

 memcpy(&GHdr->TagVer,"GIF89a",6);
 GHdr->CanvasWidth  = this->CnvWidth;
 GHdr->CanvasHeight = this->CnvHeight;
// GHdr->AspectRatio  = 0;   // memset
 if(Palette)
  {
   GHdr->BgrClrIdx        = (this->BgrClrIdx >= 0)?this->BgrClrIdx:0;
   GHdr->Flags.SortFlag   = 0;
   GHdr->Flags.ClrTblFlag = 1;
   GHdr->Flags.ColorRes   = 7;    // All 8 bits (Always?)
   GHdr->Flags.ClrTblSize = PalSizeIdx;
   AssignPalette(GHdr->Palette, this->GPalette.Palette, this->GPalette.ClrCount);
  }
 return GHdr;
}
//---------------------------------------------------------------------------
UINT8* AddGifLooping(void)
{
 if(this->LoopValue < 0)return nullptr;
 UINT8 Block[] = {0x21, 0xFF, 0x0B, 0x4E, 0x45, 0x54, 0x53, 0x43, 0x41, 0x50, 0x45, 0x32, 0x2E, 0x30, 0x03, 0x01, 0x00, 0x00, 0x00};
 SDataBlk* Blk = this->BlkList.Add(sizeof(Block));
 Blk->DataSize = sizeof(Block);
 memcpy(&Blk->Data, &Block, sizeof(Block));
 Blk->Data[16] = this->LoopValue;  // LoByte
 Blk->Data[17] = this->LoopValue >> 8;  // HiByte
 return Blk->Data;
}
//---------------------------------------------------------------------------
SGifGrCtrl* AddGifImageAnimExt(CImgBlk* Img)
{
 if(Img->DelayTime < 0)return nullptr;
 SDataBlk* Blk = this->BlkList.Add(sizeof(SGifGrCtrl)+2);
 Blk->DataSize = sizeof(SGifGrCtrl)+4;
 Blk->Data[0]  = 0x21;
 Blk->Data[1]  = 0xF9;
 Blk->Data[2]  = sizeof(SGifGrCtrl);
 Blk->Data[3+sizeof(SGifGrCtrl)] = 0;
 SGifGrCtrl* Ctrl = (SGifGrCtrl*)&Blk->Data[3];
 Ctrl->DelayTime  = Img->DelayTime;
 Ctrl->Flags.DisposalMethod = Img->DisposalMthd;
 Ctrl->Flags.UserInputFlg = 0;
 Ctrl->Flags.Reserved = 0;
 Ctrl->Flags.TranspClrFlg = (Img->TranspClrIdx >= 0);    // Must be already translated to palette
 if(Ctrl->Flags.TranspClrFlg)Ctrl->TranspClrIdx = Img->TranspClrIdx;
 return Ctrl;
}
//---------------------------------------------------------------------------
SGifImgDesc* AddGifImageHeader(CImgBlk* Img)
{
 UINT TPalSize   = 0;
 UINT PalSizeIdx = 0;
 if(Img->LocalClrTbl)
  {
   if(Img->Palette.ClrCount < 2)  
    {
     Img->Palette.ClrCount = 2;
     Img->Palette.Palette[0].Value = 0;  // Black
     Img->Palette.Palette[1].Value = -1; // White
    }
   TPalSize = Img->Palette.CalcSize(&PalSizeIdx);
  }
 UINT Size = sizeof(SGifImgDesc) + (TPalSize*3) + 1;
 SDataBlk* Blk = this->BlkList.Add(Size);
 Blk->DataSize = Size;
 memset(&Blk->Data, 0, Size);
 Blk->Data[0] = 0x2C;
 SGifImgDesc* IHdr = (SGifImgDesc*)&Blk->Data[1];
 IHdr->Top    = Img->Top;
 IHdr->Left   = Img->Left;
 IHdr->Width  = Img->Width;
 IHdr->Height = Img->Height;
 if(Img->LocalClrTbl)
  {
   IHdr->Flags.ClrTblFlag = 1;
   IHdr->Flags.ClrTblSize = PalSizeIdx;
   AssignPalette(IHdr->Palette, Img->Palette.Palette, Img->Palette.ClrCount);
  }
 return IHdr; 
}
//---------------------------------------------------------------------------
UINT8* AddGifImageData(CImgBlk* Img, UINT8 MinCodeSize)
{
 UINT SrcSize = Img->PixelsSize / sizeof(UINT32);  // One-byte after indexing
 UINT8* buf = new UINT8 [SrcSize + (SrcSize / 2)];    // Worst case size
 UINT   len = LZWGifEncode(MinCodeSize, buf, Img->Pixels, SrcSize);   // TODO: Use a stream class? Or add each block separately to BlkList?
 SDataBlk* Blk = this->BlkList.Add(len);
 Blk->DataSize = len;
 memcpy(&Blk->Data, buf, Blk->DataSize);
 delete[] buf;       
 return 0;
}
//---------------------------------------------------------------------------
void ConvertImagePixelsToIndexes(CImgBlk* Img, CRGBOTQuantizer* Quant, bool DoDithering)
{
struct SPixQErr
{
 INT32 R;
 INT32 G;
 INT32 B;
 INT32 I;  // Index (UINT8) but INT32 to keep alignment

 static inline int GifIMax(int l, int r) { return l>r?l:r; }

 inline void Update(INT32 ErrR, INT32 ErrG, INT32 ErrB, INT32 Val)
  {
   this->R += GifIMax( -this->R, (ErrR * Val) >> 4 );   // '>> 4' is '/ 16'
   this->G += GifIMax( -this->G, (ErrG * Val) >> 4 ); 
   this->B += GifIMax( -this->B, (ErrB * Val) >> 4 ); 
  }
};

 if(Img->BytesPerPixel <= 1)return; 
 UINT8*  DstPtr = (UINT8*)&Img->Pixels;
 UINT32* SrcPtr = (UINT32*)&Img->Pixels;
 UINT  TotalPixels = Img->PixelsSize / sizeof(UINT32);  
 SPalette* Palette = (Img->LocalClrTbl)?(&Img->Palette):(&this->GPalette);
 if(DoDithering)                // Floyd–Steinberg dithering
  {  
   // QuantPixels initially holds color*256 for all pixels
   // The extra 8 bits of precision allow for sub-single-color error values to be propagated
   UINT Width  = Img->Width;
   UINT Height = Img->Height; 
   SPixQErr* QuantPixels = (SPixQErr*)malloc(TotalPixels * sizeof(SPixQErr));    
   SPixQErr* QPixelsEnd  = &QuantPixels[TotalPixels];
   for(UINT idx=0;idx < TotalPixels;idx++)   // ARGB  : BB GG RR AA 
    {
     UINT32   Pixel = SrcPtr[idx];
     SPixQErr* pixr = &QuantPixels[idx];
     pixr->R = (Pixel >> 8) & 0x0000FF00;
     pixr->G = Pixel & 0x0000FF00;
     pixr->B = (Pixel << 8) & 0x0000FF00;
    }

   for(UINT32 RowIdx=0; RowIdx < Height; RowIdx++)
    {
     SPixQErr* CurPixRow = &QuantPixels[RowIdx * Width];
     SPixQErr* NxtPixRow = &CurPixRow[Width];
     for(UINT32 ColIdx=0; ColIdx < Width; ColIdx++)
      {
       SPixQErr* nextPix = &CurPixRow[ColIdx]; 

       // Compute the colors we want (rounding to nearest)
       INT32 rr = (nextPix->R + 127) >> 8; // '>> 8' is '/ 256'
       INT32 gg = (nextPix->G + 127) >> 8;  
       INT32 bb = (nextPix->B + 127) >> 8; 
                                                   
       // Search the palete
       if(rr > 255)rr = 255;   // CRGBOTQuantizer requires this limits
       if(gg > 255)gg = 255;
       if(bb > 255)bb = 255;
       INT32 bestInd = Quant->FindNearestColor(rr, gg, bb); 
       CRGBOTQuantizer::RGBQUAD* PalEntry = &Palette->Palette[bestInd];

       // Write the result to the temp buffer
       INT32 r_err = nextPix->R - (INT32(PalEntry->Red  ) << 8); // '<< 8' is '* 256'
       INT32 g_err = nextPix->G - (INT32(PalEntry->Green) << 8); 
       INT32 b_err = nextPix->B - (INT32(PalEntry->Blue ) << 8); 

       nextPix->R = PalEntry->Red;
       nextPix->G = PalEntry->Green;
       nextPix->B = PalEntry->Blue;
       nextPix->I = bestInd;

       // Propagate the error to the four adjacent locations that we haven't touched yet
       SPixQErr* pix7 = &CurPixRow[ColIdx + 1];                 // Only Right and Down to avoid extra checks? // May be Star propagation will be better?
       SPixQErr* pix3 = &NxtPixRow[ColIdx - 1];   
       SPixQErr* pix5 = &NxtPixRow[ColIdx];     
       SPixQErr* pix1 = &NxtPixRow[ColIdx + 1];
        
       if(pix7 < QPixelsEnd)pix7->Update(r_err, g_err, b_err, 7);
       if(pix3 < QPixelsEnd)pix3->Update(r_err, g_err, b_err, 3);
       if(pix5 < QPixelsEnd)pix5->Update(r_err, g_err, b_err, 5);
       if(pix1 < QPixelsEnd)pix1->Update(r_err, g_err, b_err, 1);  
      }
    }

   // Copy the palettized result to the output buffer
   for(UINT idx=0;idx < TotalPixels;idx++)DstPtr[idx] = QuantPixels[idx].I;   // ARGB  : BB GG RR AA 
   free(QuantPixels);
  }
   else
    {
     for(UINT ctr=0;ctr < TotalPixels;ctr++,DstPtr++,SrcPtr++)
      {
       UINT32 Pixel = *SrcPtr;
       *DstPtr = Quant->FindNearestColor(Pixel >> 16, Pixel >> 8, Pixel);
      } 
    }
 Img->BytesPerPixel = 1;
}
//---------------------------------------------------------------------------
static void UpdatePaletteFromImage(CImgBlk* Image, CRGBOTQuantizer* Quant)  // Cleans up transparency
{
 bool NeedAlphaCnv = ((Image->AlphaPresPerc >= 0) && (Image->TranspColor >= 0));
 if(Image->TranspColor >= 0)Quant->AppendColor(Image->TranspColor >> 16, Image->TranspColor >> 8, Image->TranspColor);
 if(NeedAlphaCnv)
  {
   for(UINT RowIdx=0;RowIdx < Image->Height;RowIdx++)
    {
     UINT32* CurRow = (UINT32*)&Image->Pixels[(Image->Width * Image->BytesPerPixel) * RowIdx];
     for(UINT ColIdx=0;ColIdx < Image->Width;ColIdx++)
      {
       UINT32 Pixel = CurRow[ColIdx];
       UINT   Alpha = Pixel >> 24;    // 00-Completely transparent, 255-Completely opaque
       int    APerc = ((Alpha*100)/255);  // NumToPerc
       if(APerc >= Image->AlphaPresPerc)Quant->AppendColor(Pixel >> 16, Pixel >> 8, Pixel);    // Alpha transparency to GIF`s 1-bit transparency
         else CurRow[ColIdx] = Image->TranspColor;  // Transparent
      }
    }
  }
 else
  {
   for(UINT RowIdx=0;RowIdx < Image->Height;RowIdx++)
    {
     UINT32* CurRow = (UINT32*)&Image->Pixels[(Image->Width * Image->BytesPerPixel) * RowIdx];
     for(UINT ColIdx=0;ColIdx < Image->Width;ColIdx++)
      {
       UINT32 Pixel = CurRow[ColIdx];
       Quant->AppendColor(Pixel >> 16, Pixel >> 8, Pixel); 
      }
    }
  }
}
//---------------------------------------------------------------------------
static void AssignPalette(UINT8* GifPalette, CRGBOTQuantizer::RGBQUAD* SrcPal, UINT ClrCount)
{
 UINT Offs = 0;
 for(UINT ctr=0;ctr < ClrCount;ctr++)
  {
   GifPalette[Offs++] = SrcPal[ctr].Red;
   GifPalette[Offs++] = SrcPal[ctr].Green;
   GifPalette[Offs++] = SrcPal[ctr].Blue;
  }
}
//---------------------------------------------------------------------------
static void AssignPalette(CRGBOTQuantizer::RGBQUAD* DstPal, UINT8* GifPalette, UINT ClrCount)
{
 UINT Offs = 0;
 for(UINT ctr=0;ctr < ClrCount;ctr++)
  {
   DstPal[ctr].Red    = GifPalette[Offs++];
   DstPal[ctr].Green  = GifPalette[Offs++]; 
   DstPal[ctr].Blue   = GifPalette[Offs++]; 
   DstPal[ctr].Unused = 0;
  }
}
//---------------------------------------------------------------------------
static int CalcSectionSize(PBYTE Section)
{
 int Size = 0; // Subblock offset
 int Len;
 do
  {
   Len   = Section[Size];
   Size += Len + 1;
  }
   while(Len);
 return Size;
}
//---------------------------------------------------------------------------
static inline unsigned int GetBits(UINT BitCnt, UINT8* BytePtr, UINT& Offset, UINT& BytesLeft, UINT& BitsRead)
{
 unsigned int Value = 0;
 BytePtr = &BytePtr[Offset];
 for(int ctr=0;ctr < (int)BitCnt;ctr++,BitsRead++)
  {
   if(BitsRead > 7)
    {
     BitsRead = 0; 
     BytePtr++; 
     Offset++;
     if(!--BytesLeft)
      {
       BytesLeft = *BytePtr;
       BytePtr++;
       Offset++;
       if(!BytesLeft)return -1;  // End of blocks sequence reached
      }
    }
   unsigned int bit = bool(*BytePtr & (1 << BitsRead));
   Value |= (bit << ctr);
  }
 return Value;
}
//---------------------------------------------------------------------------
// Compute output index of y-th input line, in frame of height h.
static int GetInterlacedLineIndex(int h, int y)
{
 int p = (h - 1) / 8 + 1;      // number of lines in current pass 
 if (y < p)return y * 8; // pass 1  
    
 y -= p;
 p  = (h - 5) / 8 + 1;
 if (y < p)return y * 8 + 4; // pass 2
       
 y -= p;
 p  = (h - 3) / 4 + 1;
 if (y < p)return y * 4 + 2; // pass 3 
        
 y -= p;    
 return y * 2 + 1;  // pass 4 
}
//---------------------------------------------------------------------------
static int UpdateImageRows(CImgBlk* Image, SLZWDictionary* Decomp, size_t& PixelsOffset, size_t Index, bool Interlaced)
{
 UINT16 length = Decomp->length[Index];
 UINT8  buffer[4096];   // Max data block   
 size_t bufferIndex = length - 1;
 while(Decomp->prefix[Index] != -1)  // extract the color indicies
  {
   buffer[bufferIndex] = Decomp->byteValue[Index];
   Index = Decomp->prefix[Index];
   --bufferIndex;
  }
 buffer[bufferIndex] = Decomp->byteValue[Index];     // Now we have 'length' of color indicies in the 'buffer'
 if((PixelsOffset + length) > Image->PixelsSize)return -1;   // Overflow!
 if(Interlaced)
  {
   while(length)
    {
     UINT CurrRowIdx  = PixelsOffset / Image->Width;
     UINT CurrRowOffs = PixelsOffset - (CurrRowIdx * Image->Width);         // Recalculate for each row because data stream may span across multiple interlace Blocks (4 in total)
     int RealRowIdx   = GetInterlacedLineIndex(Image->Height, CurrRowIdx);
     UINT Size = ((length > Image->Width)?(Image->Width):length);
     memcpy(&Image->Pixels[(RealRowIdx * Image->Width)+CurrRowOffs], &buffer, Size);
     PixelsOffset += Size;
     length -= Size;
    }
  }
   else
    {
     memcpy(&Image->Pixels[PixelsOffset], &buffer, length);
     PixelsOffset += length;
    }
 return 0;
}
//---------------------------------------------------------------------------


public:
//---------------------------------------------------------------------------
CGIF(void)
{
 this->CnvWidth = this->CnvHeight = -1;
 this->GClrTblRefs = 0;
 this->BgrClrIdx = -1;  
 this->LoopValue = -1;
 this->BgrColor.Value = -1;  // White
 this->HaveGifData = false;
}
//---------------------------------------------------------------------------
~CGIF()
{

}
//---------------------------------------------------------------------------
void Clear(void)
{
 this->ImgList.Clear();
 this->BlkList.Clear();
}
//---------------------------------------------------------------------------
CImgBlk*  ImgFirst(void){return this->ImgList.First();}
CImgBlk*  ImgNext(CImgBlk* Img){return this->ImgList.Next(Img);}

SDataBlk* BlkFirst(void){return this->BlkList.First();}
SDataBlk* BlkNext(SDataBlk* Blk){return this->BlkList.Next(Blk);}
//---------------------------------------------------------------------------
inline static UINT16 FrmDelayMs(UINT ms){return (ms / 10);}
inline static UINT16 FrmDelayFps(float fps){return (100.0 / fps);}
//---------------------------------------------------------------------------
// BgrColor is translated to nearest in a global color table if it is shared by images or written there directly. 
// Keepeng a global table with at least Black, White and BgrColor if it is not Black or White
//
int CreateCanvas(UINT16 Width, UINT16 Height, int BgrClrRGB=0x00FFFFFF) 
{
 this->CnvWidth  = Width;
 this->CnvHeight = Height;
 this->BgrColor.Red   = BgrClrRGB >> 16;
 this->BgrColor.Blue  = BgrClrRGB;
 this->BgrColor.Green = BgrClrRGB >> 8;
 return 0;
}
//---------------------------------------------------------------------------
int RemoveImage(UINT ImgIdx)
{
 UINT Idx = 0;
 for(CImgBlk* Blk=this->ImgList.First();Blk;Idx++,Blk=this->ImgList.Next(Blk))
  {
   if(ImgIdx != Idx)continue;
   if(!Blk->LocalClrTbl)this->GClrTblRefs--;
   this->ImgList.Remove(Blk);
   return ImgIdx;
  }
 return -1;
}
//---------------------------------------------------------------------------
// AlphaPreservationTreshold: -1 - No Transparency; 100 - Keep all pixels with any Alpha value; 0 - Remove all pixels with Alpha less than 255
//
CImgBlk* AddImage(UINT16 Width, UINT16 Height, UINT16 Top, UINT16 Left, int TranspClrRGB=-1, int AlphaPreservationTreshold=-1, int DelayTime=-1, int DisposalMthd=0, bool LocalClrTbl=false)
{
 if(this->CnvWidth < 0)this->CreateCanvas(Width, Height);
 UINT PixelsSize = (Width * Height) * sizeof(UINT32);     // Pixels is in ARGB format
 CImgBlk* NewPtr = this->ImgList.Add(PixelsSize);   
 NewPtr->Width   = Width;
 NewPtr->Height  = Height;
 NewPtr->Top     = Top;
 NewPtr->Left    = Left;

 NewPtr->DelayTime     = DelayTime;
 NewPtr->TranspColor   = TranspClrRGB;
 NewPtr->DisposalMthd  = DisposalMthd;
 NewPtr->AlphaPresPerc = AlphaPreservationTreshold;
 NewPtr->LocalClrTbl   = LocalClrTbl;
 NewPtr->PixelsSize    = PixelsSize;
 NewPtr->TranspClrIdx  = -1;
 NewPtr->BytesPerPixel = 4;   // ARGB
 NewPtr->Palette.ClrCount = 0;      // Assigned later
 if(!LocalClrTbl)this->GClrTblRefs++;
 return NewPtr;
}
//---------------------------------------------------------------------------
CImgBlk* AddBitmap(BITMAPFILEHEADER* BmpFile, UINT16 Top=0, UINT16 Left=0, int TranspClrRGB=-1, int AlphaPreservationTreshold=-1, int DelayTime=-1, int DisposalMthd=0, bool LocalClrTbl=false)
{   
 BITMAPINFO* BmpInf = (BITMAPINFO*)&((UINT8*)BmpFile)[sizeof(BITMAPFILEHEADER)];
 if(BmpInf->bmiHeader.biCompression != BI_RGB)return nullptr;
 int Height = BmpInf->bmiHeader.biHeight;
 if(Height < 0)Height = -Height;
 if(BmpInf->bmiHeader.biWidth > this->CnvWidth)this->CnvWidth = BmpInf->bmiHeader.biWidth;
 if(Height > this->CnvHeight)this->CnvHeight = Height; 
 CImgBlk* Img = this->AddImage(BmpInf->bmiHeader.biWidth, Height, Top, Left, TranspClrRGB, AlphaPreservationTreshold, DelayTime, DisposalMthd, LocalClrTbl);
 int BmpRowSize = (BmpInf->bmiHeader.biWidth * (BmpInf->bmiHeader.biBitCount / 8) + 3) & ~3;   // DWORD aligned
 UINT32* ImgRow = (UINT32*)&Img->Pixels;
 UINT8* BmpPixels = &((UINT8*)BmpFile)[BmpFile->bfOffBits];
 int    BmpWidth  = BmpInf->bmiHeader.biWidth;
 if(BmpInf->bmiHeader.biHeight >= 0){BmpPixels = &BmpPixels[(Height-1)*BmpRowSize]; BmpRowSize = -BmpRowSize;}   // Backward rows
 if(BmpInf->bmiHeader.biBitCount == 8)
  {   
   for(UINT RowIdx=0;RowIdx < Img->Height;RowIdx++)
    { 
     UINT8* BmpRow = (UINT8*)BmpPixels;
     for(UINT ColIdx=0;ColIdx < Img->Width;ColIdx++)
      {
       RGBQUAD* Pixel = &BmpInf->bmiColors[BmpRow[ColIdx]];
       ImgRow[ColIdx] = (Pixel->rgbRed << 16)|(Pixel->rgbGreen << 8)|Pixel->rgbBlue;
      }
     ImgRow += Img->Width;
     BmpPixels += BmpRowSize;
    }
  }
 else if(BmpInf->bmiHeader.biBitCount == 16)
  {
   for(UINT RowIdx=0;RowIdx < Img->Height;RowIdx++)
    {  
     UINT16* BmpRow = (UINT16*)BmpPixels;
     for(UINT ColIdx=0;ColIdx < Img->Width;ColIdx++)
      {
       UINT16 Pixel = BmpRow[ColIdx];
       UINT Red   = (Pixel >> 10) & 0x1F;
       UINT Green = (Pixel >> 5) & 0x1F;
       UINT Blue  = Pixel & 0x1F;
       ImgRow[ColIdx] = (Red << 16)|(Green << 8)|Blue;
      }
     ImgRow += Img->Width;
     BmpPixels += BmpRowSize;
    }
  }
 else if(BmpInf->bmiHeader.biBitCount == 24)
  {
   for(UINT RowIdx=0;RowIdx < Img->Height;RowIdx++)
    {  
     UINT8* BmpRow = (UINT8*)BmpPixels;
     for(UINT ColIdx=0;ColIdx < Img->Width;ColIdx++,BmpRow+=3)
      {      
       ImgRow[ColIdx] = (BmpRow[2] << 16)|(BmpRow[1] << 8)|BmpRow[0];
      }
     ImgRow += Img->Width;
     BmpPixels += BmpRowSize;
    }
  }
 else if(BmpInf->bmiHeader.biBitCount == 32)
  {
   for(UINT RowIdx=0;RowIdx < Img->Height;RowIdx++)
    {  
     UINT32* BmpRow = (UINT32*)BmpPixels;
     for(UINT ColIdx=0;ColIdx < Img->Width;ColIdx++)
      {
       ImgRow[ColIdx] = BmpRow[ColIdx];
      }
     ImgRow += Img->Width;
     BmpPixels += BmpRowSize;
    }
  }
 return Img;
}
//--------------------------------------------------------------------------- 
int MakeGIF(int LoopVal=-1, bool DoDithering=false, bool AlwaysUpdateGlobalPalette=false, int QuantizingLevel=5)
{
 CRGBOTQuantizer gquant;
 this->BlkList.Clear();
 SPalette* Palette = NULL;
 if(!this->HaveGifData)   // Calculate palettes
  {
   this->LoopValue = LoopVal;
   if(this->GClrTblRefs)gquant.SetOctreeDepth(QuantizingLevel);   // Global palette
     else AlwaysUpdateGlobalPalette = false;      // No images use the global palette
   for(CImgBlk* Img=this->ImgList.First();Img;Img=this->ImgList.Next(Img))     // Build Palettes
    {
     if(Img->LocalClrTbl)
      {
       CRGBOTQuantizer lquant;
       lquant.SetOctreeDepth(QuantizingLevel); 
       UpdatePaletteFromImage(Img, &lquant);
       Img->Palette.ClrCount = lquant.GetPalette(Img->Palette.Palette, 256);
       if(Img->TranspColor >= 0)Img->TranspClrIdx = lquant.FindNearestColor(Img->TranspColor >> 16, Img->TranspColor >> 8, Img->TranspColor);
       this->ConvertImagePixelsToIndexes(Img, &lquant, DoDithering);
      }
     if(!Img->LocalClrTbl || AlwaysUpdateGlobalPalette)UpdatePaletteFromImage(Img, &gquant);    // Update global palette
    }
   if(this->GClrTblRefs)   // After the palette is updated from images
    {
     memset(&this->GPalette.Palette, 0, sizeof(this->GPalette)); 
     this->GPalette.ClrCount = gquant.GetPalette(this->GPalette.Palette, 256);
     Palette = &this->GPalette;
     this->BgrClrIdx = gquant.FindNearestColor(this->BgrColor.Red, this->BgrColor.Green, this->BgrColor.Blue);
    }
   for(CImgBlk* Img=this->ImgList.First();Img;Img=this->ImgList.Next(Img))     // Convert pixels for global palette
    {
     if(!Img->LocalClrTbl)this->ConvertImagePixelsToIndexes(Img, &gquant, DoDithering);
    }     
   this->HaveGifData = true;
  }
   else  // A GIF is already loaded
    {
     if(this->GPalette.ClrCount)Palette = &this->GPalette;    // Do not add palette if it wasn`t there
    }
 SGifHdr* GHdr = this->AddGifHeader(Palette); 
 this->AddGifLooping();
 for(CImgBlk* Img=this->ImgList.First();Img;Img=this->ImgList.Next(Img))
  {
   if(!Img->LocalClrTbl && (Img->TranspClrIdx < 0))Img->TranspClrIdx = gquant.FindNearestColor(Img->TranspColor >> 16, Img->TranspColor >> 8, Img->TranspColor);
   this->AddGifImageAnimExt(Img);
   SGifImgDesc* IHdr = this->AddGifImageHeader(Img);
   this->AddGifImageData(Img, ((Img->LocalClrTbl)?(IHdr->Flags.ClrTblSize):(GHdr->Flags.ClrTblSize))+1);
  }
 return 0;
}
//---------------------------------------------------------------------------
int MakeBMP(bool SetAlpha=true)     // Makes series of BMPs from a current GIF   // TODO: RLE
{
 this->BlkList.Clear();
 for(CImgBlk* Img=this->ImgList.First();Img;Img=this->ImgList.Next(Img))
  {
   UINT BmpRowSize = ((Img->Width * Img->BytesPerPixel) + 3) & ~3;   // Must be DWORD aligned
   SPalette* Pal = (Img->LocalClrTbl)?(&Img->Palette):(&this->GPalette);   // Palette shouls be local and with 0 entries if BytesPerPixel is greater than 1
   UINT FullSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFO) + (Pal->ClrCount * 4) + (BmpRowSize * Img->Height);   // Palette entry 257 is SGifToBmp   
   SDataBlk* Blk = this->BlkList.Add(FullSize);
   Blk->DataSize = FullSize;
   BITMAPFILEHEADER* bfhdr = (BITMAPFILEHEADER*)&Blk->Data;
   BITMAPINFO*   OutBmpInf = (BITMAPINFO*)&Blk->Data[sizeof(BITMAPFILEHEADER)];

   OutBmpInf->bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
   OutBmpInf->bmiHeader.biWidth       = Img->Width;
   OutBmpInf->bmiHeader.biHeight      = -Img->Height; // Negative for Top-Bottom image
   OutBmpInf->bmiHeader.biPlanes      = 1;
   OutBmpInf->bmiHeader.biBitCount    = 8 * Img->BytesPerPixel;
   OutBmpInf->bmiHeader.biCompression = BI_RGB;  
   OutBmpInf->bmiHeader.biSizeImage   = (Img->PixelsSize / sizeof(UINT32)) * Img->BytesPerPixel;        

   OutBmpInf->bmiHeader.biXPelsPerMeter = 0;
   OutBmpInf->bmiHeader.biYPelsPerMeter = 0;
   OutBmpInf->bmiHeader.biClrUsed       = Pal->ClrCount;
   OutBmpInf->bmiHeader.biClrImportant  = 0;

   bfhdr->bfType      = 0x4D42;  // 'BM'
   bfhdr->bfSize      = OutBmpInf->bmiHeader.biSizeImage + sizeof(BITMAPINFO) + sizeof(BITMAPFILEHEADER);
   bfhdr->bfOffBits   = sizeof(BITMAPINFO) + sizeof(BITMAPFILEHEADER) + (Pal->ClrCount * 4);
   bfhdr->bfReserved1 = Img->Top;   
   bfhdr->bfReserved2 = Img->Left;

   memcpy(&OutBmpInf->bmiColors, &Pal->Palette, Pal->ClrCount * 4); 
   SGifToBmp* BDesc = (SGifToBmp*)&OutBmpInf->bmiColors[Pal->ClrCount];    // Palette Entry 257
   BDesc->TranspClrIdx = Img->TranspClrIdx;
   BDesc->DelayTime    = Img->DelayTime;
   BDesc->Flags.DisposalMethod = Img->DisposalMthd;
   BDesc->Flags.DelayTimeFlg   = (Img->DelayTime >= 0);
   BDesc->Flags.TranspClrFlg   = (Img->TranspClrIdx >= 0);
   BDesc->Flags.LocalClrTbl    = Img->LocalClrTbl;

   UINT8* BmpRowPtr = &Blk->Data[bfhdr->bfOffBits];
   UINT8* ImgRowPtr = &Img->Pixels[0];
   for(int ctr=Img->Height,Width=Img->Width;ctr > 0;ctr--)        
    {
     *(UINT32*)&BmpRowPtr[BmpRowSize-4] = 0;  // Don`t leave it undefined
     memcpy(BmpRowPtr, ImgRowPtr, Width);
     BmpRowPtr += BmpRowSize;
     ImgRowPtr += Width;
    }
  }
 return 0;
}
//---------------------------------------------------------------------------
int LoadGIF(PVOID Data, UINT Size)  // Read into sequence of images, decompresses pixel data
{
 this->ImgList.Clear();
 if(Size < sizeof(SGifHdr))return -1;
 SGifHdr* GHdr = (SGifHdr*)Data;
 UINT  HdrSize = sizeof(SGifHdr);
 this->CnvWidth    = GHdr->CanvasWidth;
 this->CnvHeight   = GHdr->CanvasHeight; 
 this->LoopValue   = -1;
 this->GClrTblRefs = 0;
 this->GPalette.ClrCount = (GHdr->Flags.ClrTblFlag)?( 1 << ((int)GHdr->Flags.ClrTblSize + 1) ):0;
 if(this->GPalette.ClrCount)
  {  
   AssignPalette(this->GPalette.Palette, (UINT8*)Data + HdrSize, this->GPalette.ClrCount);
   HdrSize += this->GPalette.ClrCount * 3;
  }
 UINT8* BlkPtr = (UINT8*)Data + HdrSize;
 UINT8* EndPtr = (UINT8*)Data + Size;

 int DelayTime    = -1;
 int DisposalMthd = 0;
 int TranspClrIdx = -1;  // Index at first, next an actual color
 int Result = 0;
 bool Interlaced  = false;
 size_t PixelsOffset = 0;
 CImgBlk* LastImg = nullptr;
 SLZWDictionary* Decomp = new SLZWDictionary;
 for(UINT Len=0;BlkPtr < EndPtr;BlkPtr+=Len)
  {
   if(*BlkPtr == 0x3B)break;  // End of file   
   if(*BlkPtr == 0x21)  // Parse an extension
    {
     Len = CalcSectionSize(&BlkPtr[2]) + 2;
     if(BlkPtr[1] == 0xF9)  // Graphics Control Extension (Placed before 'Image Descriptor')
      {
       SGifGrCtrl* CtHdr = (SGifGrCtrl*)&BlkPtr[3];
       DelayTime = CtHdr->DelayTime;
       DisposalMthd = CtHdr->Flags.DisposalMethod;
       if(CtHdr->Flags.TranspClrFlg)TranspClrIdx = CtHdr->TranspClrIdx;
      }
     else if((BlkPtr[1] == 0xFF) && (BlkPtr[2] == 11) && (BlkPtr[14] == 3))   // Application Extension Label. Expect only 'NETSCAPE2.0'
      {
       this->LoopValue = *(UINT16*)&BlkPtr[16];
      }
     continue;
    }
   if(*BlkPtr == 0x2C)     // Image Descriptor
    {
     SGifImgDesc* Desc = (SGifImgDesc*)&BlkPtr[1];
     UINT PalColCnt    = (Desc->Flags.ClrTblFlag)?( 1 << ((int)Desc->Flags.ClrTblSize + 1) ):0;
     UINT PaletteSize  = PalColCnt * 3;
     int  TranspColor  = -1;
     Len = sizeof(SGifImgDesc) + PaletteSize + 1;
     if(TranspClrIdx >= 0)
      {
       UINT8* Ptr  = (Desc->Flags.ClrTblFlag)?(&Desc->Palette[TranspClrIdx*3]):(&GHdr->Palette[TranspColor*3]);
       TranspColor = (Ptr[0] << 16)|(Ptr[1] << 8)|Ptr[2];
      }
     LastImg = this->AddImage(Desc->Width, Desc->Height, Desc->Top, Desc->Left, TranspClrIdx, 100, DelayTime, DisposalMthd, PalColCnt);
     LastImg->BytesPerPixel = 1;  // Leave it palettized
     LastImg->TranspClrIdx  = TranspClrIdx;
     LastImg->TranspColor   = TranspColor;
     LastImg->LocalClrTbl   = Desc->Flags.ClrTblFlag;
     if(PalColCnt)
      {
       LastImg->Palette.ClrCount = PalColCnt;
       AssignPalette(LastImg->Palette.Palette, &BlkPtr[1+sizeof(SGifImgDesc)], PalColCnt);   
      } 
       else this->GClrTblRefs++;
     Interlaced   = Desc->Flags.InterlaceFlag;
     DelayTime    = -1;
     TranspClrIdx = -1; 
     DisposalMthd = 0;
     PixelsOffset = 0;
     continue;
    }
   if(*BlkPtr < 0x20)
    {    
     Len = 0;
     Decomp->Init(BlkPtr[Len++]);   // MinCodeSize
     UINT BytesLeft  = BlkPtr[Len++];    // Block size
     if(!BytesLeft){Result = -2; break;}  // No data!
     UINT   BitsRead = 0;
     int16_t index;
     int16_t old;
     index = GetBits(Decomp->codeLength, BlkPtr, Len, BytesLeft, BitsRead);     // Always read one more bit than the code length
     if(index != Decomp->clearCode){Result = -3; break;}
     index = GetBits(Decomp->codeLength, BlkPtr, Len, BytesLeft, BitsRead);    
     UpdateImageRows(LastImg, Decomp, PixelsOffset, index, Interlaced);   // paint(state, state.index);
     old = index;
     for(;;)
      {
       index = GetBits(Decomp->codeLength, BlkPtr, Len, BytesLeft, BitsRead); 
//       if(index < 0)break;     // Should not happen (eoiCode must be reached)
       if(index < Decomp->currentIndex)  // Does <index> exist in the dictionary?
        { 
         if(index == Decomp->eoiCode){Len++; break;}   // Increase Len for last 00 which is expected but not checked
          else if(index == Decomp->clearCode) 
           {
            Decomp->Reset();
            index = GetBits(Decomp->codeLength, BlkPtr, Len, BytesLeft, BitsRead); 
//            if(index < 0)break;     // Should not happen (eoiCode must be reached)
            UpdateImageRows(LastImg, Decomp, PixelsOffset, index, Interlaced);   // paint(state, state.index);
            old = index;           
           }
          else   // code that already exists
           {
            UpdateImageRows(LastImg, Decomp, PixelsOffset, index, Interlaced);  //  paint(state, state.index);
            Decomp->Add(old, Decomp->GetFirstByte(index));
           }
        }
         else if(index == Decomp->currentIndex)     // <index> does not exist in the dictionary
          {   
           UINT8 b = Decomp->GetFirstByte(old);       // B <- first byte of string at <old>
           size_t idx = Decomp->Add(old, b); 
           UpdateImageRows(LastImg, Decomp, PixelsOffset, idx, Interlaced);  // paint(state, idx);
          }      
       old = index;  
      }
     Len += BytesLeft;
     continue;
    }
   Result = -4;   // Unknown code or a broken file
   break;
  }

 this->HaveGifData = true;
 delete Decomp;
 return Result;
}
//---------------------------------------------------------------------------

};
//---------------------------------------------------------------------------
/*
 Check:  NEUQUANT Neural-Net quantization algorithm by Anthony Dekker, 1994. See "Kohonen neural networks for optimal colour quantization" in "Network: Computation in Neural Systems" Vol. 5 (1994) pp 351-367. for a discussion of the algorithm.
*/