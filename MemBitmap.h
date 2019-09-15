//---------------------------------------------------------------------------

#ifndef MemBitmapH
#define MemBitmapH

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

//---------------------------------------------------------------------------
#include <windows.h>

#define FLAG_BM    0x4D42
#define BYTELIMIT(B) ((B) < 0)?(0):(((B) > 255)?(255):(B));
#define _ALIGN_FRWRD(Value,Alignment) ((((Value)/(Alignment))+((bool)((Value)%(Alignment))))*(Alignment))
#define _ALIGN_BKWRD(Value,Alignment) (((Value)/(Alignment))*(Alignment))

#pragma pack( push, 1 )
union UPixel
{
private:
 DWORD Color;
public:
 struct
  {
   BYTE Red;
   BYTE Green;
   BYTE Blue;
   BYTE Weight;
  };

 UPixel(DWORD val){this->Color = val;}
 operator   const DWORD()    {return this->Color;}
 COLORREF GetColor(void){return this->Color & 0x00FFFFFF;}
 void     SetColor(COLORREF clr){this->Color = (this->Color & 0xFF000000)|(clr & 0x00FFFFFF);}
};
#pragma pack( pop )

//----------------------------------------
class CMemBitmap  // Stores all bitmaps as 32bpp // Always Top-Bottom
{
static const int MaxUndo = 16;
private:
 HANDLE hDefHeap;
 PDWORD BmpRows;    // Bitmap is stored as 32bpp
 UINT   BmpSize;
 UINT   UndoCtr;
 int    iWidth;
 int    iHeight;
 PDWORD UndoList[MaxUndo];
 BITMAPINFO InfBmp;

public:
 bool IsEmpty(void){return !this->BmpRows;}
 UINT GetSize(void){return this->BmpSize;}
 int  GetWidth(void){return this->iWidth;}    // {return this->InfBmp.bmiHeader.biWidth;}
 int  GetHeight(void){return this->iHeight;}  // {return (((long)this->InfBmp.bmiHeader.biHeight < 0)?(-this->InfBmp.bmiHeader.biHeight):(this->InfBmp.bmiHeader.biHeight));}
 void FilterPixRect(int Top, int Left, int Right, int Bottom, bool Grayscale, int Brightness=0, int Contrast=0, int Sharpen=0, int Blur=0);
 void FilterRow(UINT RowIndex, UINT Offset, UINT Length, bool Grayscale, int Brightness=0, int Contrast=0, int Sharpen=0, int Blur=0);
 void FlipRectV(int Top, int Left, int Right, int Bottom);
 COLORREF* GetRow(UINT Index){return &this->BmpRows[Index*this->InfBmp.bmiHeader.biWidth];}
 COLORREF* GetPixels(void){return this->BmpRows;}

 int      SaveToFile(LPSTR FileName);
 int      LoadFromFile(LPSTR FileName);
 int      CreateNew(UINT Width, UINT Height, bool TopBottom=true);
 COLORREF FindMostFreqColor(COLORREF* UnusedClr=NULL);

 void PrepHalftoneStretch(HDC dc)
  {
   SetBrushOrgEx(dc,0,0,NULL);
   SetStretchBltMode(dc,HALFTONE);
  }

 int  DrawToDC(HDC dc, int DstX, int DstY, int SrcX=0, int SrcY=0, UINT SrcW=-1, UINT SrcH=-1);
 int  StretchToDC(HDC dc, int DstX, int DstY, UINT DstW, UINT DstH, int SrcX=0, int SrcY=0, UINT SrcW=-1, UINT SrcH=-1);
 int  StretchToBitmap(CMemBitmap* Bmp, int DstX, int DstY, UINT DstW, UINT DstH, int SrcX=0, int SrcY=0, UINT SrcW=-1, UINT SrcH=-1);
 void FillPixRect(COLORREF Color, int Top, int Left, int Right, int Bottom);
 void DrawPixRect(COLORREF Color, int Top, int Left, int Right, int Bottom);
 void DrawPixLine(COLORREF Color, int FromX, int FromY, int ToX, int ToY);
 void Rotate(float angle);

 CMemBitmap(void)
  {
   for(int ctr=0;ctr < MaxUndo;ctr++)UndoList[ctr] = NULL;
   this->hDefHeap = GetProcessHeap();
   this->BmpRows  = NULL;
   this->BmpSize  = 0;
   this->UndoCtr  = 0;
  }

 ~CMemBitmap()
  {
   this->FreePixels();
  }

 void FreePixels(void)
  {
   this->UndoClear();
   if(this->BmpRows)HeapFree(this->hDefHeap, NULL, this->BmpRows);
   this->BmpRows = NULL;
   this->BmpSize = 0;
   this->iHeight = 0;
   this->iWidth  = 0;
  }

 bool UndoPop(void)  // Not Cyclic
  {
   if(!this->UndoCtr)return false;
   this->UndoCtr--;
   memcpy(this->BmpRows,this->UndoList[this->UndoCtr],this->BmpSize);
   HeapFree(this->hDefHeap, NULL, this->UndoList[this->UndoCtr]);
   this->UndoList[this->UndoCtr] = NULL;
   return true;
  }

 bool UndoSkip(void)
  {
   if(!this->UndoCtr)return false;
   this->UndoCtr--;
   HeapFree(this->hDefHeap, NULL, this->UndoList[this->UndoCtr]);
   this->UndoList[this->UndoCtr] = NULL;
   return true;
  }

 bool UndoPush(void)
  {
   if(this->UndoCtr == MaxUndo)return false;
   this->UndoList[this->UndoCtr] = (PDWORD)HeapAlloc(this->hDefHeap, HEAP_ZERO_MEMORY, this->BmpSize);
   memcpy(this->UndoList[this->UndoCtr],this->BmpRows,this->BmpSize);
   this->UndoCtr++;
   return true;
  }

 void UndoClear(void)
  {
   for(UINT ctr=0;ctr < this->UndoCtr;ctr++)this->UndoPop();
  }
//--------------------
static bool IsBitmapBottomUp(LPSTR BmpFile)
{
 DWORD Result = 0;
 BITMAPINFO BmpInfo;
 BITMAPFILEHEADER BmpFHdr;

 HANDLE hBmpFile = CreateFile(BmpFile,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,FILE_FLAG_SEQUENTIAL_SCAN,NULL);
 if(hBmpFile == INVALID_HANDLE_VALUE)return false;
 ReadFile(hBmpFile,&BmpFHdr,sizeof(BITMAPFILEHEADER),&Result,NULL);
 if((Result != sizeof(BITMAPFILEHEADER))||(BmpFHdr.bfType != FLAG_BM)){CloseHandle(hBmpFile);return false;}  // Not a BMP file!
 ReadFile(hBmpFile, &BmpInfo, sizeof(BmpInfo), &Result, NULL);   // Read bitmap parameters
 CloseHandle(hBmpFile);
 if(Result != sizeof(BmpInfo))return false;
 return (BmpInfo.bmiHeader.biHeight >= 0);
}

};
//---------------------------------------------------------------------------
#endif
