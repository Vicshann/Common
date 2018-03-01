//---------------------------------------------------------------------------

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

#pragma hdrstop

#include "MemBitmap.h"

//---------------------------------------------------------------------------
int CMemBitmap::LoadFromFile(LPSTR FileName)
{
 DWORD Result = 0;
 BITMAPFILEHEADER BmpFHdr;

 this->FreePixels();
 HANDLE hBmpFile = CreateFile(FileName,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,FILE_FLAG_SEQUENTIAL_SCAN,NULL);
 if(hBmpFile == INVALID_HANDLE_VALUE)return -1;
 ReadFile(hBmpFile,&BmpFHdr,sizeof(BITMAPFILEHEADER),&Result,NULL);
 if((Result != sizeof(BITMAPFILEHEADER))||(BmpFHdr.bfType != FLAG_BM)){CloseHandle(hBmpFile);return -2;}  // Not a BMP file!

 ULONG FileSize = GetFileSize(hBmpFile,NULL);
 ULONG DataSize = FileSize - sizeof(BITMAPFILEHEADER);
 BITMAPINFO* BmpInfo = (BITMAPINFO*)HeapAlloc(this->hDefHeap, HEAP_ZERO_MEMORY, DataSize+16);
 if(!BmpInfo){CloseHandle(hBmpFile);return -3;}
 ReadFile(hBmpFile, BmpInfo, DataSize, &Result, NULL);   // Read bitmap parameters
 CloseHandle(hBmpFile);
 if(Result != DataSize){HeapFree(this->hDefHeap, NULL, BmpInfo);return -3;}

 HDC hMemDC = CreateCompatibleDC(NULL);
 BITMAPINFOHEADER MemBmp;
 memset(&MemBmp,0,sizeof(MemBmp));
 MemBmp.biSize   = sizeof(MemBmp);
 MemBmp.biWidth  = BmpInfo->bmiHeader.biWidth;
 MemBmp.biHeight = BmpInfo->bmiHeader.biHeight;
 MemBmp.biPlanes = 1;
 MemBmp.biBitCount = 32;
 MemBmp.biCompression = BI_RGB;
 PBYTE   pBmpBits  = &((PBYTE)BmpInfo)[BmpFHdr.bfOffBits - sizeof(BITMAPFILEHEADER)];
 HDC     hScreen   = GetDC(NULL);
 HBITMAP hDcBmp    = CreateDIBitmap(hScreen, &MemBmp, CBM_INIT, pBmpBits, BmpInfo, DIB_RGB_COLORS);
 ReleaseDC(NULL,hScreen);
 if(!hDcBmp){DeleteDC(hMemDC);HeapFree(this->hDefHeap, NULL, BmpInfo);return -4;}
 HGDIOBJ prv = SelectObject(hMemDC, hDcBmp);

 memset(&this->InfBmp,0,sizeof(BITMAPINFO));
 this->InfBmp.bmiHeader.biSize   = sizeof(BITMAPINFOHEADER);
 this->InfBmp.bmiHeader.biWidth  = BmpInfo->bmiHeader.biWidth;
 this->InfBmp.bmiHeader.biHeight = -BmpInfo->bmiHeader.biHeight; // Negative for Top-Bottom image
 this->InfBmp.bmiHeader.biPlanes = 1;
 this->InfBmp.bmiHeader.biBitCount = 32;
 this->InfBmp.bmiHeader.biCompression = BI_RGB;

 this->BmpSize = (BmpInfo->bmiHeader.biWidth*BmpInfo->bmiHeader.biHeight)*sizeof(DWORD);
 this->BmpRows = (PDWORD)HeapAlloc(this->hDefHeap, HEAP_ZERO_MEMORY, BmpSize);
 memset(this->BmpRows,0,BmpSize);
 int lines = GetDIBits(hMemDC,hDcBmp,0,BmpInfo->bmiHeader.biHeight,this->BmpRows,&this->InfBmp,DIB_RGB_COLORS);
 DeleteDC(hMemDC);
 DeleteObject(hDcBmp);
 HeapFree(this->hDefHeap, NULL, BmpInfo);
 if(lines <= 0){HeapFree(this->hDefHeap, NULL, this->BmpRows);this->BmpRows=NULL;return -5;}
 this->iHeight = (this->InfBmp.bmiHeader.biHeight < 0)?(-this->InfBmp.bmiHeader.biHeight):(this->InfBmp.bmiHeader.biHeight);
 this->iWidth  = this->InfBmp.bmiHeader.biWidth;
 return 0;
}
//---------------------------------------------------------------------------
int CMemBitmap::SaveToFile(LPSTR FileName)  
{
 HANDLE hBmpFile = CreateFile(FileName,GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,CREATE_ALWAYS,FILE_FLAG_SEQUENTIAL_SCAN,NULL);
 if(hBmpFile == INVALID_HANDLE_VALUE)return -1;

 DWORD Result;
 BITMAPFILEHEADER bfhdr;
 BITMAPINFO OutBmpInf;
 UINT RowSize = _ALIGN_FRWRD((this->GetWidth()*3),4);
 memcpy(&OutBmpInf,&this->InfBmp.bmiHeader,sizeof(BITMAPINFOHEADER));
 OutBmpInf.bmiHeader.biBitCount    = 24;
 OutBmpInf.bmiHeader.biSizeImage  = RowSize * this->GetHeight();  

 bfhdr.bfType      = FLAG_BM;
 bfhdr.bfSize      = OutBmpInf.bmiHeader.biSizeImage + sizeof(BITMAPINFO) + sizeof(BITMAPFILEHEADER);
 bfhdr.bfOffBits   = sizeof(BITMAPINFO) + sizeof(BITMAPFILEHEADER);
 bfhdr.bfReserved1 = 0;
 bfhdr.bfReserved2 = 0;

 WriteFile(hBmpFile,&bfhdr,sizeof(BITMAPFILEHEADER),&Result,NULL);
 WriteFile(hBmpFile,&OutBmpInf,sizeof(BITMAPINFO),&Result,NULL);
 PBYTE RowData = (PBYTE)HeapAlloc(this->hDefHeap, HEAP_ZERO_MEMORY, RowSize+16);
 for(int row = 0,bmph=this->GetHeight();row < bmph;row++)
  {
   COLORREF* BmpRow = this->GetRow(row);
   for(int col = 0,idx=0,bmpw=this->GetWidth();col < bmpw;col++,idx+=3)*((PDWORD)&RowData[idx]) = BmpRow[col];
   WriteFile(hBmpFile,RowData,RowSize,&Result,NULL);
  }
 CloseHandle(hBmpFile);
 HeapFree(this->hDefHeap, NULL, RowData);
 return 0;
}
//---------------------------------------------------------------------------
int CMemBitmap::CreateNew(UINT Width, UINT Height, bool TopBottom)
{
 this->FreePixels();

 memset(&this->InfBmp,0,sizeof(BITMAPINFO));
 this->InfBmp.bmiHeader.biSize   = sizeof(BITMAPINFOHEADER);
 this->InfBmp.bmiHeader.biWidth  = Width;
 this->InfBmp.bmiHeader.biHeight = ((TopBottom)?(-Height):(Height));   //(rinv)?(Height):(-Height);
 this->InfBmp.bmiHeader.biPlanes = 1;
 this->InfBmp.bmiHeader.biBitCount = 32;
 this->InfBmp.bmiHeader.biCompression = BI_RGB;
 this->iWidth  = Width;
 this->iHeight = Height;
 this->BmpSize = (Width*Height)*sizeof(DWORD);
 this->BmpRows = (PDWORD)HeapAlloc(this->hDefHeap, HEAP_ZERO_MEMORY, BmpSize);
 memset(this->BmpRows,0,BmpSize);
 return 0;
}
//---------------------------------------------------------------------------
int CMemBitmap::StretchToDC(HDC dc, int DstX, int DstY, UINT DstW, UINT DstH, int SrcX, int SrcY, UINT SrcW, UINT SrcH)
{
 BITMAPINFO ibmp;
 if(this->IsEmpty())return -1;
 if(SrcW > this->GetWidth() )SrcW = this->GetWidth();
 if(SrcH > this->GetHeight())SrcH = this->GetHeight();
 return StretchDIBits(dc,DstX,DstY,DstW,DstH,SrcX,SrcY,SrcW,SrcH,this->BmpRows,&this->InfBmp,DIB_RGB_COLORS,SRCCOPY);
}
//---------------------------------------------------------------------------
int CMemBitmap::DrawToDC(HDC dc, int DstX, int DstY, int SrcX, int SrcY, UINT SrcW, UINT SrcH)
{
 if(this->IsEmpty())return -1;
 if(SrcW > this->GetWidth() )SrcW = this->GetWidth();
 if(SrcH > this->GetHeight())SrcH = this->GetHeight();
 return SetDIBitsToDevice(dc,DstX,DstY,SrcW,SrcH,SrcX,SrcY,0,this->GetHeight(),this->BmpRows,&this->InfBmp,DIB_RGB_COLORS);
}
//---------------------------------------------------------------------------
int CMemBitmap::StretchToBitmap(CMemBitmap* Bmp, int DstX, int DstY, UINT DstW, UINT DstH, int SrcX, int SrcY, UINT SrcW, UINT SrcH)
{
 if(this->IsEmpty())return -1;
 int res = -2;
 HDC     hMemDC  = CreateCompatibleDC(NULL);
 HDC     hScreen = GetDC(NULL);
 HBITMAP hDcBmp  = CreateCompatibleBitmap(hScreen,DstW,DstH);
 ReleaseDC(NULL,hScreen);
 SelectObject(hMemDC, hDcBmp);
 SetStretchBltMode(hMemDC,HALFTONE);
 SetBrushOrgEx(hMemDC,0,0,NULL);
 if(this->StretchToDC(hMemDC, 0, 0, DstW, DstH) > 0)
  {
   BITMAPINFOHEADER MemBmp;
   memset(&MemBmp,0,sizeof(MemBmp));
   MemBmp.biSize   = sizeof(MemBmp);
   MemBmp.biWidth  = DstW;
   MemBmp.biHeight = -DstH;  // Top-Bottom
   MemBmp.biPlanes = 1;
   MemBmp.biBitCount = 32;
   MemBmp.biCompression = BI_RGB;
   COLORREF* Pixels = (COLORREF*)HeapAlloc(this->hDefHeap, HEAP_ZERO_MEMORY, (DstW*DstH*4)+16);
   res = GetDIBits(hMemDC,hDcBmp,0,DstH,Pixels,(BITMAPINFO*)&MemBmp,DIB_RGB_COLORS);
   UINT ImgWidth  = Bmp->GetWidth();
   UINT ImgHeight = Bmp->GetHeight();
   for(UINT srow=0,drow=DstY;(srow < DstH)&&(drow < ImgHeight);srow++,drow++)
	{
	 COLORREF* SrcRow = &Pixels[DstW*srow];
	 COLORREF* DstRow = Bmp->GetRow(drow);
	 for(UINT scol=0,dcol=DstX;(scol < DstW)&&(dcol < ImgWidth);scol++,dcol++)DstRow[dcol] = SrcRow[scol];
	}
   HeapFree(this->hDefHeap, NULL, Pixels);
  }
 if(hMemDC)DeleteDC(hMemDC);
 if(hDcBmp)DeleteObject(hDcBmp);
 return res;
}
//---------------------------------------------------------------------------
void CMemBitmap::FlipRectV(int Top, int Left, int Right, int Bottom)
{
 int ImgWidth  = this->GetWidth();
 int ImgHeight = this->GetHeight();

 if(Right  < 0)Right  = ImgWidth  - 1;
 if(Bottom < 0)Bottom = ImgHeight - 1;
 for(int rctr=Top,rmax=(Bottom-Top+1)/2;rctr <= rmax;rctr++)
  {
   PDWORD RowA = this->GetRow(rctr);
   PDWORD RowB = this->GetRow(Bottom-rctr);
   for(int cctr=Left;cctr <= Right;cctr++)
	{
	 DWORD tmp  = RowA[cctr];
	 RowA[cctr] = RowB[cctr];
	 RowB[cctr] = tmp;
	}
  }
}
//---------------------------------------------------------------------------
void CMemBitmap::FilterPixRect(int Top, int Left, int Right, int Bottom, bool Grayscale, int Brightness, int Contrast, int Sharpen, int Blur)
{
 int ImgWidth  = this->GetWidth();
 int ImgHeight = this->GetHeight();
 //if(Left > ImgWidth)Left = ImgWidth;
 //if(Bottom > ImgHeight)Bottom = ImgHeight;
 int Length    = Right - Left + 1;
 for(int row=Top;row <= Bottom;row++)FilterRow(row, Left, Length, Grayscale, Brightness, Contrast, Sharpen, Blur);
}
//---------------------------------------------------------------------------
void CMemBitmap::FillPixRect(COLORREF Color, int Top, int Left, int Right, int Bottom)
{
 int ImgWidth  = this->GetWidth();
 int ImgHeight = this->GetHeight();
 //if(Left > ImgWidth)Left = ImgWidth;
 //if(Bottom > ImgHeight)Bottom = ImgHeight;
 if(Color == -1)
  {
 for(int row=Top;row <= Bottom;row++)  // Gather all available Pixels in the range and find a nearest one
  {
   COLORREF* PixRow = this->GetRow(row);
   for(int col=Left;col <= Right;col++)
	{
	 PixRow[col] = (PixRow[col] & 0xFF000000)|(~PixRow[col] & 0x00FFFFFF);  // Invert color
	}
  }
   return;
  }

 for(int row=Top;row <= Bottom;row++)  // Gather all available Pixels in the range and find a nearest one
  {
   COLORREF* PixRow = this->GetRow(row);
   for(int col=Left;col <= Right;col++)
	{
	 PixRow[col] = Color;
	}
  }
}
//---------------------------------------------------------------------------
void CMemBitmap::DrawPixRect(COLORREF Color, int Top, int Left, int Right, int Bottom)
{
 int ImgWidth  = this->GetWidth();
 int ImgHeight = this->GetHeight();
 //if(Left > ImgWidth)Left = ImgWidth;
 //if(Bottom > ImgHeight)Bottom = ImgHeight;
 if(Color == -1)
  {
 for(int row=Top;row <= Bottom;row++)  // Gather all available Pixels in the range and find a nearest one
  {
   COLORREF* PixRow = this->GetRow(row);
   if((row==Top)||(row==Bottom))
	{
	 for(int col=Left;col <= Right;col++)PixRow[col] = (PixRow[col] & 0xFF000000)|(~PixRow[col] & 0x00FFFFFF); // Invert color
	}
	 else
	  {
	   PixRow[Left]  = (PixRow[Left]  & 0xFF000000)|(~PixRow[Left]  & 0x00FFFFFF); // Invert color
	   PixRow[Right] = (PixRow[Right] & 0xFF000000)|(~PixRow[Right] & 0x00FFFFFF); // Invert color
	  }
  }
   return;
  }

 for(int row=Top;row <= Bottom;row++)  // Gather all available Pixels in the range and find a nearest one
  {
   COLORREF* PixRow = this->GetRow(row);
   if((row==Top)||(row==Bottom))
	{
	 for(int col=Left;col <= Right;col++)PixRow[col] = Color;
	}
	 else
	  {
	   PixRow[Left]  = Color;
	   PixRow[Right] = Color;
	  }
  }
}
//---------------------------------------------------------------------------
void CMemBitmap::DrawPixLine(COLORREF Color, int FromX, int FromY, int ToX, int ToY)
{
 const int deltaX = abs(ToX - FromX);
 const int deltaY = abs(ToY - FromY);
 const int signX  = FromX < ToX ? 1 : -1;
 const int signY  = FromY < ToY ? 1 : -1;
 int error = deltaX - deltaY;
 if(Color == -1)
  {
 this->GetRow(ToY)[ToX] = (this->GetRow(ToY)[ToX] & 0xFF000000) | ~(this->GetRow(ToY)[ToX] & 0x00FFFFFF);  //setPixel(x2, y2);
 while((FromX != ToX) || (FromY != ToY))
 {
  COLORREF* pixel = &this->GetRow(FromY)[FromX]; //setPixel(FromX, FromY);
  *pixel = (*pixel & 0xFF000000) | ~(*pixel & 0x00FFFFFF);
  const int error2 = error * 2;
  if(error2 > -deltaY){error -= deltaY;FromX += signX;}
  if(error2 < deltaX){error += deltaX; FromY += signY;}
 }
   return;
  }
 this->GetRow(ToY)[ToX] = Color;  //setPixel(x2, y2);
 while((FromX != ToX) || (FromY != ToY))
 {
  this->GetRow(FromY)[FromX] = Color; //setPixel(FromX, FromY);
  const int error2 = error * 2;
  if(error2 > -deltaY){error -= deltaY; FromX += signX;}
  if(error2 <  deltaX){error += deltaX; FromY += signY;}
 }
}
//---------------------------------------------------------------------------
void CMemBitmap::Rotate(float angle)  // Fast and rude (14*14 squares)
{
/* PDWORD NewBmpRows = (PDWORD)HeapAlloc(this->hDefHeap, 0, BmpSize);
 for(UINT ctr=0,BMax=(this->BmpSize/sizeof(COLORREF));ctr<BMax;ctr++)NewBmpRows[ctr] = 0x00FFFFFF;

 double ImgWidth  = this->GetWidth();
 double ImgHeight = this->GetHeight();
 SSinCos sico = FPUSinCosDeg32(angle);

 double Point1x = (-ImgHeight * sico.fSin);
 double Point1y = (ImgHeight  * sico.fCos);
 double Point2x = (ImgWidth   * sico.fCos - ImgHeight * sico.fSin);
 double Point2y = (ImgHeight  * sico.fCos + ImgWidth  * sico.fSin);
 double Point3x = (ImgWidth   * sico.fCos);
 double Point3y = (ImgWidth   * sico.fSin);

 double minx = Min((double)0,Min(Point1x,Min(Point2x,Point3x)));
 double miny = Min((double)0,Min(Point1y,Min(Point2y,Point3y)));
 double maxx = Max(Point1x,Max(Point2x,Point3x));
 double maxy = Max(Point1y,Max(Point2y,Point3y));

 for(double x=0;x < ImgWidth;x++)
  {
   for(double y=0;y < ImgHeight;y++)
	{
	 double SrcBitmapx = ((x+minx)*sico.fCos+(y+miny)*sico.fSin);
	 double SrcBitmapy = ((y+miny)*sico.fCos-(x+minx)*sico.fSin);
	 if((SrcBitmapx < 0) || (SrcBitmapx >= ImgWidth) || (SrcBitmapy < 0) || (SrcBitmapy >= ImgHeight))continue; // Out of canvas
	 NewBmpRows[FPURound32((y*ImgWidth)+x)] = this->GetRow(FPURound32(SrcBitmapy))[FPURound32(SrcBitmapx)];   //DestBitmap->Canvas->Pixels[x][y]=SrcBitmap->Canvas->Pixels[SrcBitmapx][SrcBitmapy];
	}
  }
 HeapFree(this->hDefHeap, NULL, this->BmpRows);
 this->BmpRows = NewBmpRows; */
}
//---------------------------------------------------------------------------
/*
 R0  (PDWORD*)[]
 --
 --  G0 (PDWORD)[]
 --  --
 --  --  B0  (DWORD)[]
 --  --  --
 --  --  B255
 --  --
 --  G255
 --
 R255
*/
COLORREF CMemBitmap::FindMostFreqColor(COLORREF* UnusedClr)
{
 PDWORD** RedPtrs = (PDWORD**)HeapAlloc(this->hDefHeap, HEAP_ZERO_MEMORY, sizeof(PVOID)*256);    // Array for RED indexes
 UINT     ImgSize = (this->GetWidth() * this->GetHeight());
 UINT     MostR   = 0;
 UINT     MostG   = 0;
 UINT     MostB   = 0;
 UINT     LastCtr = 0;
 for(UINT ctr=0;ctr<ImgSize;ctr++)  // Gather colors
  {
   DWORD Color = this->BmpRows[ctr];
   BYTE  CurR  = Color;
   BYTE  CurG  = Color >> 8;
   BYTE  CurB  = Color >> 16;
   if(!RedPtrs[CurR])RedPtrs[CurR] = (PDWORD*)HeapAlloc(this->hDefHeap, HEAP_ZERO_MEMORY, sizeof(PVOID)*256);  // Array for GREEN indexes
   if(!RedPtrs[CurR][CurG])RedPtrs[CurR][CurG] = (PDWORD)HeapAlloc(this->hDefHeap, HEAP_ZERO_MEMORY, sizeof(PVOID)*256);  // Array for BLUE indexes
   RedPtrs[CurR][CurG][CurB]++;
  }
 for(UINT CurR=0;CurR<256;CurR++) // Find most frequent color
  {
   if(!RedPtrs[CurR])continue;
   for(UINT CurG=0;CurG<256;CurG++)
	{
	 if(!RedPtrs[CurR][CurG])continue;
	 for(UINT CurB=0;CurB<256;CurB++)
	  {
	   UINT CurCtr = RedPtrs[CurR][CurG][CurB];
	   if(CurCtr <= LastCtr)continue;
	   LastCtr = CurCtr;
	   MostR   = CurR;
	   MostG   = CurG;
	   MostB   = CurB;
	  }
	}
  }

 if(UnusedClr) // Find some unused color
  {
   BYTE  UnR = 0;
   BYTE  UnG = 0;
   BYTE  UnB = 0;
   for(int CtrR=0;CtrR<256;CtrR++){if(!RedPtrs[CtrR]){UnR=CtrR;goto Exit;}} // One of RED codes is completely unused
   for(int CtrR=0;CtrR<256;CtrR++)
	{
	 if(!RedPtrs[CtrR])continue;
	 for(int CtrG=0;CtrG<256;CtrG++){if(!RedPtrs[CtrR][CtrG]){UnR=CtrR;UnG=CtrG;goto Exit;}} // One of GREEN codes is completely unused
	}
   for(int CtrR=0;CtrR<256;CtrR++)
	{
	 if(!RedPtrs[CtrR])continue;
	 for(int CtrG=0;CtrG<256;CtrG++)
	  {
	   if(!RedPtrs[CtrR][CtrG])continue;
	   for(int CtrB=0;CtrB<256;CtrB++){if(!RedPtrs[CtrR][CtrG][CtrB]){UnR=CtrR;UnG=CtrG;UnB=CtrB;goto Exit;}} // One of BLUE codes is completely unused
	  }
	}
Exit:
   *UnusedClr = RGB(UnR,UnG,UnB);
  }
 return RGB(MostR,MostG,MostB);   //((CurB << 16)|(CurG << 8)|CurR);
}
//---------------------------------------------------------------------------
void CMemBitmap::FilterRow(UINT RowIndex, UINT Offset, UINT Length, bool Grayscale, int Brightness, int Contrast, int Sharpen, int Blur)
{
/* UINT ImgWidth  = this->GetWidth();
 UINT ImgHeight = this->GetHeight();
 if((Offset+Length) > ImgWidth)Length = (ImgWidth - Offset);
 UPixel* CurRow = (UPixel*)this->GetRow(RowIndex);
 for(int ctr=Offset,LenMax=Offset+Length;ctr < LenMax;ctr++)
  {
   UPixel* CurPix = &CurRow[ctr];
   int B = CurPix->Blue;
   int G = CurPix->Green;
   int R = CurPix->Red;

   if(Blur != 0)     // Default 1
	{
	 UPixel PixPrev = ((ctr-1)<0)?(0):(CurRow[ctr-1]);
	 UPixel PixNext = ((ctr+1)<LenMax)?(CurRow[ctr+1]):0;
	 B = FPURound32((float)(((1)*PixPrev.Blue)  + (Blur*B) + ((1)*PixNext.Blue))  / (1+Blur+1));
	 G = FPURound32((float)(((1)*PixPrev.Green) + (Blur*G) + ((1)*PixNext.Green)) / (1+Blur+1));
	 R = FPURound32((float)(((1)*PixPrev.Red)   + (Blur*R) + ((1)*PixNext.Red))   / (1+Blur+1));
	}
	
   if(Sharpen != 0)  // Default 5   // Something wrong with the Matrix, but this is works!
	{
	 UPixel PixPrev = ((ctr-1)<0)?(0):(CurRow[ctr-1]);
	 UPixel PixNext = ((ctr+1)<LenMax)?(CurRow[ctr+1]):0;
	 B = FPURound32((float)(((-1)*PixPrev.Blue)  + (Sharpen*B) + ((-1)*PixNext.Blue))  / ((-1)+Sharpen+(-1)));
	 G = FPURound32((float)(((-1)*PixPrev.Green) + (Sharpen*G) + ((-1)*PixNext.Green)) / ((-1)+Sharpen+(-1)));
	 R = FPURound32((float)(((-1)*PixPrev.Red)   + (Sharpen*R) + ((-1)*PixNext.Red))   / ((-1)+Sharpen+(-1)));
    }

   // ***** Best Order is Brightness before Contrast ??? *****

   // Update Brightness
   if(Brightness != 0)      // (-255) - 0 - (+255)
    {
     R = R+Brightness;
     G = G+Brightness;
     B = B+Brightness;
    }

    // Update Contrast
   if(Contrast != 0)
	{
	 float CValue;
     if(Contrast > 0)CValue = (1 + (((float)Contrast) / 10)); // 0 - (+255)
       else CValue = (1 - (FPUSqrt32(-Contrast) / 16));       // 0 - (-255)
	 R = FPURound32(128 + ((R - 128) * CValue));
	 G = FPURound32(128 + ((G - 128) * CValue));
	 B = FPURound32(128 + ((B - 128) * CValue));
    }

   // Grayscale after all other filters
   if(Grayscale)R=G=B = FPURound32((R * 0.56)+(G * 0.33)+(B * 0.11));

   CurPix->Red   = BYTELIMIT(R);
   CurPix->Green = BYTELIMIT(G);
   CurPix->Blue  = BYTELIMIT(B);
  } */
}
//===========================================================================

