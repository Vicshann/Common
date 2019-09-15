//---------------------------------------------------------------------------

#ifndef BigBmpClassH
#define BigBmpClassH

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

#include "Windows.h"
#include "OctreeQuant.h"

#include "Math.h"
//#include "Windows.h"

//---------------------------------------------------------------------------
#define BBSAFEPROC        // Choose: DEADLOCK or AccessViolation

#define FLAG_BM    0x4D42
#define FLAG_BS    0x5342
#define FLAG_BG    0x4142
#define FLAG_BAGR  0x52474142
#define MEGABYTE   1048576
#define RBUFSAVEMB 48              // Max mem for buffering rows when saving a image
#define IMGMINSIZE 320             // Min size of image in BAG
#define ROWBUFDIV  2               // Divider of viewport height for row buffering
#define BI_DRC     0x000000FE      // Direct Row Compression
#define BI_IRC     0x000000FF      // Indexed Row Compression
#define MAXPALSIZE 0xFFFF
#define FLAG_RGB08 0xAA
#define FLAG_RGB16 0xACAC          // Experimental found
#define FLAG_RGB24 0xAACCAA        // Experimental found
#define FLAG_RGB32 0xAAAAAAAA      // Experimental found
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))
#define DELTA(a,b) (((a) > (b)) ? ((a) - (b)) : ((b) - (a)))
#define BYTELIMIT(B) ((B) < 0)?(0):(((B) > 255)?(255):(B));
#define LODWORD(Value) ((DWORD)(Value))
#define HIDWORD(Value) ((DWORD)((Value)>>32))
#define _ALIGN_FRWRD(Value,Alignment) ((((Value)/(Alignment))+((bool)((Value)%(Alignment))))*(Alignment))
#define _ALIGN_BKWRD(Value,Alignment) (((Value)/(Alignment))*(Alignment))
#define _PERCTONUM(perc,pmax) (((perc)*(pmax))/100)
#define _NUMTOPERC(num,pmax)  (((num)*100)/(pmax))

#ifdef BBSAFEPROC
#define __GUARDNEEDED(proc) (proc)
#else
#define __GUARDNEEDED(proc) ;
#endif

typedef unsigned __int64 QWORD;

//---------------------------------------------------------------------------
__declspec(naked)int _stdcall FPURound32(float Value)
{  // Used Predefined Rounding Mode
 __asm FLD   DWORD PTR [ESP+4]
 __asm FISTP DWORD PTR [ESP+4]
 __asm MOV   EAX,[ESP+4]
 __asm RET 4
}
//---------------------------------------------------------------------------
__declspec(naked)float _stdcall FPUSqrt32(float Value)
{  // Float results returned in FPU stack
 __asm FLD   DWORD PTR [ESP+4]
 __asm FSQRT
 __asm RET 4
}
//---------------------------------------------------------------------------
#include <pshpack2.h>
struct BMSFHEADER
{
 WORD  FlagBS;
 DWORD RleFlag;
 DWORD IndexOffset;      // Offset of row index table
 DWORD PixelsOffset;     // Offset of graphic data
};
#include <poppack.h>
//---------------------------------------------------------------------------
struct BMPARCHIVEHDR      // All Bitmaps are Top-Down
{
 DWORD FlagBGAR;          // Format flag
 DWORD RlePFlag;
 DWORD BaseWidth;         // Original image width
 DWORD BaseHeight;        // Original image height
 DWORD LevelsCount;       // Number of image mips by POW2
 DWORD PaletteSize;       // Colors number in palette: 0 - used actual colors (1 Color = 4 bytes) ; Palette > 65536 colors is too big to be used !!!
 DWORD IndexesSize;
 DWORD BytesPerPixel;     // if > 2 - used actual colors ;(PaletteSize > 0) and 1 - 256colors max,2 - 65536 colors max
 // Colrs palette
 // Blocs of row indexes for a each image
 // Data blocks of each image
};
//---------------------------------------------------------------------------
struct BIGBMPINFO
{
 RECT  ImageRect;
 RECT  DcRect;
 DWORD ExRop;
 DWORD ExTranspClr;
 DWORD ExTranspMode;
 int   ExPosX;
 int   ExPosY;
 int   ExWidth;
 int   ExHeight;
 int   Width;
 int   Height;
 int   BitsPP;
 int   Images;
 int   IType;
 int   CurImage;
 int   Contrast;
 int   Brightness;
 int   ResQuality;
 bool  Resizing;
 bool  Grayscale;
 bool  Sharpen;
 bool  Blur;
};
//---------------------------------------------------------------------------
struct RESIZEAVERBUF
{
 DWORD TotalRed;
 DWORD TotalGreen;
 DWORD TotalBlue;
 DWORD PixCount;
};
//---------------------------------------------------------------------------
typedef bool _stdcall (*PROGRESSCALLBACK)(int Position, int Maximum);

class CBIGBITMAP
{
 private:
   HANDLE           DefHeap;
   HANDLE           hFileBmp;
   HANDLE           hMappedBmp;
   BITMAPINFO       *BitmapInfo;
   BITMAPINFO       *BmpInfoOrig;
   BITMAPINFO       *BmpInfoProcs;
   BITMAPINFO       *BmpViewInfoHdr;
   RESIZEAVERBUF    *ResAverBufPtr;
   HDC              DstWndDC;
   int              DstWidth;
   int              DstHeight;
   int              SrcWidth;
   int              SrcHeight;
   int              BmpWidth;      // For Easy access
   int              BmpHeight;     // Height in BITMAPINFO may be negative for top_down bitmaps
   int              BaseBmpWidth;
   int              ImagesNumber;
   int              BAGImageIndex;
   int              FirstRowLoaded;
   int              TotalRowsLoaded;
   int              PrvRowIndex;       // Decompress row
   int              PrvPixCount;       // Decompress row
   int              PrvPixOffset;      // Decompress row
   int              RowsToBuffer;
   int              DExPosX;
   int              DExPosY;
   int	            DExWidth;
   int	            DExHeight;
   int              FContrast;
   int              FBrightness;
   bool             FBlur;
   bool             FSharpen;
   bool             FGrayscale;
   bool             BmpTopDown;
   bool             ResizePixels;
   bool             ProcessPixels;
   bool             BmpFExtended;
   DWORD            DExRasterOp;
   DWORD            DExTranspClr;
   DWORD            DExTranspMode;
   DWORD            TrpModeTwoClr;
   DWORD            ImageType;
   DWORD            RleFlagCode;
   DWORD            FlagCode;        // Precalculate For DeCompressRowRLE
   DWORD            SizeMask;        // Precalculate For DeCompressRowRLE
   DWORD            UsedRowSize;
   DWORD            BytesInPixel;
   DWORD            AlignedRowSize;
   DWORD            RowsBufSize;
   DWORD            MapPageSize;
   DWORD            ResizeQuality;
   DWORD            RowsOffset;         // Offet of bitmap rows data in file
   DWORD            ADstRowSize;
   DWORD            DstRowSize;
   DWORD            DstBytesPP;
   DWORD            DRowWidth;
   PVOID            IndexBuffer;
   PVOID            RowIndexBuff;
   PVOID            DecRowBuffer;
   PVOID            MemBmpBuffer;
   PVOID            ReadedRowsBuf;
   PVOID            MappedSection;
   RECT             ViewRect;
   RECT             TargetRect;
   BMPARCHIVEHDR    BmpAHdr;
   CRITICAL_SECTION CSecRowsMap;
   PROGRESSCALLBACK CallBackProc;

   int   _stdcall ChangeColorFormat(PVOID DstPixels, PVOID SrcPixels, DWORD DstBytePP, DWORD SrcBytePP, DWORD PixNumber, OCTREEQUANTIZER *Quantizer);
   void  _stdcall ResizePixelsRow(PVOID RowBuffer, RECT *SrcRect, int NewWidth, int NewHeight, int DstRowIndex, int Quality);
   void  _stdcall SetPixelRGB(PVOID DataPtr, DWORD BytesPP, BYTE Red, BYTE Green, BYTE Blue);
   void  _stdcall FilterRow(PVOID DstRowPtr, PVOID SrcRowPtr, int DstBytesPP, int SrcBytesPP, int RowWidth, int Brightness, int Contrast, bool Grayscale, bool Sharpen, bool Blur);
   DWORD _stdcall GetComprRowWidthF(HANDLE hDataFile, DWORD RowFileOffset, DWORD RleFlagCode, int PixelsInRow, int BytesInPixel);
   DWORD _stdcall CompressRow(PVOID DstBuffer,PVOID SrcBuffer,DWORD RowLength,DWORD PixelSize,DWORD FlagCode);
   DWORD _stdcall CompressPixels(PVOID DstBuffer,DWORD *DstOffset,DWORD ColorValue,DWORD PixelsNumber,DWORD PixelSize,DWORD FlagCode);
   DWORD _stdcall CreateModeAndCounter(DWORD ColorValue,DWORD PixelsNumber,DWORD PixelSize,DWORD FlagCode,DWORD *Remainder,DWORD *CtrModSize);
   DWORD _stdcall DeCompressRowRLE(PVOID SrcBuffer, int PixOffset, int PixCount);
   DWORD _stdcall ExtractRGB(PVOID DataPtr, DWORD BytesPP);
   DWORD _stdcall LoadBitmapRows(int StartRow, int RowsNumber);
   DWORD _stdcall UpdateBitmapBuffer(void);
   DWORD _stdcall GetBitmapPixel(int XPos, int YPos);
   PVOID _stdcall GetBitmapRow(int Index, int PixOffset, int PixCount);
   void    inline ProcGuardEnter(void);
   void    inline ProcGuardExit(void);

 public:
   void  _stdcall SetCallBack(PROGRESSCALLBACK CallBack);
   DWORD _stdcall SaveBitmap(LPSTR FileName, RECT *DstRect, RECT *SrcRect, DWORD BitPerPix, DWORD Format, DWORD ResQuality, bool Resize, bool Grayscale, bool Sharpen, bool Blur, int Contrast, int Brightness);
   DWORD _stdcall SetView(HDC TargetDC, RECT *DstRect, RECT *SrcRect);
   DWORD _stdcall SetProcessing(bool Resizing, bool Grayscale, bool Sharpen, bool Blur, int ResQuality, int Contrast, int Brightness);
   DWORD _stdcall SetDrawExParams(int PosX, int PosY, int DestWidth, int DestHeight, DWORD RasterOp, DWORD TranspClr, DWORD TranspMode);
   DWORD _stdcall GetBitmapInfo(BIGBMPINFO *BigInfo);
   DWORD _stdcall SelectImageIndex(int Index);
   DWORD _stdcall LoadBitmap(LPSTR FileName);
   DWORD _stdcall FreeBitmap(void);
   DWORD _stdcall ReDrawEx(void);
   DWORD _stdcall ReDraw(void);

    	 _stdcall CBIGBITMAP(void);
     	 _stdcall ~CBIGBITMAP();
};
//---------------------------------------------------------------------------
// VERY NEED OPTIMIZATION !!!!!
int __stdcall TranspStrDIBits(HDC hDstDC,int XDest,int YDest,int DestWidth,int DestHeight,int XSrc,int YSrc,int SrcWidth,int SrcHeight,PVOID SrcPixels,BITMAPINFO *SrcBmpInfo,COLORREF TranspColor)
{
 int        Result;
 int        DstPosY;
 int        RowsCount;
 int        RowsOffset;
 DWORD      PixColor;
 DWORD      PixBufSize;
 PVOID      ImgPixPtr;
 PVOID      MskPixPtr;
 PVOID      ImgBuffer;
 PVOID      MskBuffer;
 HBITMAP    hDstBmp;
 BITMAPINFO DstBmpInfo;

 ZeroMemory(&DstBmpInfo,sizeof(BITMAPINFO));
 hDstBmp = GetCurrentObject(hDstDC,OBJ_BITMAP);
 DstBmpInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
 GetDIBits(hDstDC,hDstBmp,YDest,DestHeight,NULL,&DstBmpInfo,DIB_RGB_COLORS);  // Get size of buffer
 PixBufSize = DstBmpInfo.bmiHeader.biWidth;
 PixBufSize = _ALIGN_FRWRD(PixBufSize,sizeof(DWORD));
 PixBufSize = ((PixBufSize*4)*DestHeight);       // 4 - size of ONE pixel
 ImgBuffer  = VirtualAlloc(NULL,(PixBufSize*2),MEM_COMMIT,PAGE_READWRITE);  // For a image and mask
 MskBuffer  = &((BYTE*)ImgBuffer)[PixBufSize];   // Already aligned to DWORD

 DstBmpInfo.bmiHeader.biBitCount    = 32;
 DstBmpInfo.bmiHeader.biCompression = BI_RGB;

 DstPosY    = YDest;
 RowsCount  = DestHeight;
 RowsOffset = (DstBmpInfo.bmiHeader.biHeight-YDest-DestHeight);
 if(RowsOffset < 0){RowsCount += RowsOffset;DstPosY += RowsOffset;RowsOffset = 0;}
 GetDIBits(hDstDC,hDstBmp,RowsOffset,RowsCount,MskBuffer,&DstBmpInfo,DIB_RGB_COLORS);   // Get Background pixels
 StretchDIBits(hDstDC,XDest,YDest,DestWidth,DestHeight,XSrc,YSrc,SrcWidth,SrcHeight,SrcPixels,SrcBmpInfo,DIB_RGB_COLORS,SRCCOPY);  // Stretch source image to DC
 GetDIBits(hDstDC,hDstBmp,RowsOffset,RowsCount,ImgBuffer,&DstBmpInfo,DIB_RGB_COLORS);   // Get Stretched image pixels

 // Replace transparent color in image to pixels from the mask in dest rectangle
 for(int hctr=0,xpos=((XDest > 0)?(XDest):(0)),pmax=((XDest+DestWidth)>DstBmpInfo.bmiHeader.biWidth)?(DstBmpInfo.bmiHeader.biWidth):(XDest+DestWidth);hctr < RowsCount;hctr++)
  {
   ImgPixPtr = &((DWORD*)ImgBuffer)[hctr*DstBmpInfo.bmiHeader.biWidth];
   MskPixPtr = &((DWORD*)MskBuffer)[hctr*DstBmpInfo.bmiHeader.biWidth];
   for(int wctr=xpos;wctr < pmax;wctr++){if((((DWORD*)ImgPixPtr)[wctr] & 0x00FFFFFF) == TranspColor)((DWORD*)ImgPixPtr)[wctr] = ((DWORD*)MskPixPtr)[wctr];}
  }

 DstBmpInfo.bmiHeader.biHeight = DestHeight;
 Result = SetDIBitsToDevice(hDstDC,XDest,DstPosY,DestWidth,DestHeight,XDest,0,0,RowsCount,ImgBuffer,&DstBmpInfo,DIB_RGB_COLORS);
 VirtualFree(ImgBuffer,NULL,MEM_RELEASE);
 return Result;
}
//---------------------------------------------------------------------------
// OPTIMIZE: Need save an align mode and prev pos ???
int __stdcall DrawAngleRectText(HDC hDstDC, LPSTR String, LPRECT DstRect, UINT Format)
{
 int     SPosX;
 int     SPosY;
 int     EPosX;
 int     EPosY;
 int     PosVal;
 int     StrWidth;
 int     StrHeight;
 int     Result;
 UINT    PrevAlign;
 RECT    TxtRect;
 POINT   OldPos;
 LOGFONT FontInf;

 TxtRect.top    = 0;
 TxtRect.left   = 0;
 TxtRect.right  = DstRect->right-DstRect->left;
 TxtRect.bottom = DstRect->bottom-DstRect->top;
 DrawText(hDstDC,String,-1,&TxtRect,(DT_CALCRECT|Format));

 PrevAlign = GetTextAlign(hDstDC);
 MoveToEx(hDstDC,0,0,&OldPos);
 Result    = GetObject(GetCurrentObject(hDstDC,OBJ_FONT),sizeof(LOGFONT),&FontInf);
 if(FontInf.lfOrientation != 0)
  {
   StrWidth  = (TxtRect.right-TxtRect.left);
   StrHeight = (TxtRect.bottom-TxtRect.top);
   SPosX     = ((DstRect->right-DstRect->left)-StrWidth)/2;
   SPosY     = OldPos.y;
   if(FontInf.lfOrientation > 0)
	{
	 PosVal  = (FontInf.lfOrientation/10);
	 EPosX   = (SPosX+StrWidth);   // text are horizontally centered
	 EPosY   = SPosY;
	 if(PosVal > 90)
	  {
	   PosVal  = _NUMTOPERC(PosVal,90);
	   // Some unknow correction ???!!!
	   Result  = (PosVal > 50)?(50-(PosVal-50)):PosVal;
	   Result  = _NUMTOPERC(Result,50);
	   Result  = _PERCTONUM(Result,45);
	   PosVal += Result;
	   //
	   PosVal  = _PERCTONUM(PosVal,StrWidth);
	   EPosX  -= PosVal;

	   PosVal  = (FontInf.lfOrientation/10);
	   PosVal  = (90-(PosVal-90));
	   PosVal  = _NUMTOPERC(PosVal,90);

	   // Some unknow correction ???!!!
	   Result  = (PosVal > 50)?(50-(PosVal-50)):PosVal;
	   Result  = _NUMTOPERC(Result,50);
	   Result  = _PERCTONUM(Result,45);
	   PosVal += Result;

	   PosVal  = _PERCTONUM(PosVal,StrWidth);
	   EPosY  += PosVal;

	   SPosY  += ((EPosY+DstRect->top)-SPosY)+2;
	   if(EPosX < SPosX)
		 SPosX += (SPosX-EPosX)*10;
	   SetTextAlign(hDstDC,TA_UPDATECP|TA_BOTTOM|TA_LEFT);
	  }
	   else
        {
		 PosVal  = _NUMTOPERC(PosVal,90);
		 // Some unknow correction ???!!!
		 Result  = (PosVal > 50)?(50-(PosVal-50)):PosVal;
		 Result  = _NUMTOPERC(Result,50);
		 Result  = _PERCTONUM(Result,45);
		 PosVal += Result;
		 //
		 PosVal  = _PERCTONUM(PosVal,StrWidth);
		//// EPosX  -= PosVal;
		 EPosY  += PosVal;

		 SPosY  += ((EPosY+DstRect->top)-SPosY)+2;
		 SetTextAlign(hDstDC,TA_UPDATECP|TA_TOP|TA_LEFT);
		}
	}
	 else
	  {
	   PosVal = -(FontInf.lfOrientation/10);
	   EPosX  = (SPosX+StrWidth);   // text are horizontally centered
	   EPosY  = SPosY;
	   if(PosVal > 90)
        {
		 PosVal  = (90-(PosVal-90));
		 PosVal  = _NUMTOPERC(PosVal,90);
		 // Some unknow correction ???!!!
		 Result  = (PosVal > 50)?(50-(PosVal-50)):PosVal;
		 Result  = _NUMTOPERC(Result,50);
		 Result  = _PERCTONUM(Result,45);
		 PosVal += Result;
		 //
		 PosVal  = _PERCTONUM(PosVal,StrWidth);
		 EPosX  -= PosVal;

		 if(EPosX > SPosX)SPosX += (EPosX-SPosX);
		 SetTextAlign(hDstDC,TA_UPDATECP|TA_BOTTOM|TA_LEFT);
        }
		 else SetTextAlign(hDstDC,TA_UPDATECP|TA_TOP|TA_LEFT);
	  }
   MoveToEx(hDstDC,SPosX,SPosY,NULL);
  }
   else SetTextAlign(hDstDC,TA_NOUPDATECP|TA_TOP|TA_LEFT);

 Result = DrawText(hDstDC,String,-1,DstRect,Format);
 MoveToEx(hDstDC,OldPos.x,OldPos.y,NULL);
 SetTextAlign(hDstDC,PrevAlign);
 return Result;
}
//---------------------------------------------------------------------------
#endif
