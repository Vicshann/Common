//---------------------------------------------------------------------------

#ifndef ImgMaskFinderH
#define ImgMaskFinderH
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

#include <windows.h>
#include <math.h>
#include "MemBitmap.h"
//---------------------------------------------------------------------------

#pragma pack( push, 1 )
struct SSinCos // Align 4
{
 float fSin;
 float fCos;
};
//---------------------------------------------------------------------------
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

 COLORREF GetColor(){return this->Color & 0x00FFFFFF;}
 SetColor(COLORREF clr){this->Color = (this->Color & 0xFF000000)|(clr & 0x00FFFFFF);}
};
#pragma pack( pop )
//---------------------------------------------------------------------------
template<class N, class M> __inline M NumToPerc(N Num, M MaxVal){return (((Num)*100)/(MaxVal));}
template<class P, class M> __inline M PercToNum(P Per, M MaxVal){return (((Per)*(MaxVal))/100);}
template<class T> __inline T    Min(T ValueA, T ValueB){return (ValueA > ValueB)?(ValueB):(ValueA);}
template<class T> __inline T    Max(T ValueA, T ValueB){return (ValueA > ValueB)?(ValueA):(ValueB);}


void    _stdcall fix__fpreset(void);
float   _stdcall FPUSqrt32(float Value);
long    _stdcall FPURound32(float Value);
SSinCos _stdcall FPUSinCosDeg32(float Angle);

float   _stdcall ColorDistance(COLORREF ColorA, COLORREF ColorB);
float   _stdcall PixelDistance(int PosXa, int PosYa, int PosXb, int PosYb);
void    _stdcall RGBtoHSV(BYTE r, BYTE g, BYTE b, int *hue, int *sat, int *val);
int     _stdcall FindPixelMatch(CMemBitmap* Bitmap, UINT SPosX, UINT SPosY, UINT PosAppr, UINT PixIndex, UINT NClrAppr, SSinCos* VecAngle, COLORREF* NearClr, UINT* FPosX, UINT* FPosY);
//---------------------------------------------------------------------------
class CPixTracer
{
struct SPixTrace
{
 COLORREF BestColor;
 UINT     TotalPixels;
 UINT     TotalWeight;
 UINT     StartX;
 UINT     StartY;
 UPixel*  Pixels[];
};

public:
 COLORREF    BgrColor;
 COLORREF    PixColor;
 UINT        MaxDetPixels;  // Maximum pixels in TgtColor detectable pixel sequence (An one big letter in a watermark is enough to detect a Target Color)
 UINT        MinDetPixels;  // Minimum pixels in TgtColor detectable pixel sequence
 UINT        MinDelPixels;  // Minimal group of pixels allowed to remove
 UINT        MaxDelPixels;  // Maximal group of pixels allowed to remove
 float       MaxClrDist;    // Param
 bool        FixByBrignt;   // Prefer a brighter pixels for fixing a holes

private:
 CMemBitmap  Bitmap;
 SPixTrace** TraceList;
 UPixel**    PixTraceBuf;
 UINT        PixTraceNum;
 UINT        TotalPixels;
 UINT        TotalWeight;
 UINT        RecuDepth;
 UINT        GMaxPixDist;
 UINT        GMinHue;
 UINT        GMinSat;
//----------------------------------
void FreeTraceList(void)
{
 if(!this->TraceList)return;
 for(UINT ctr=0;ctr < this->PixTraceNum;ctr++)HeapFree(GetProcessHeap(), NULL, this->TraceList[ctr]);
 HeapFree(GetProcessHeap(), NULL, this->TraceList);
 this->TraceList   = NULL;
 this->PixTraceNum = 0;
}
//----------------------------------
void AddTraceEntry(COLORREF BestColor, UINT PosX, UINT PosY)
{
 this->PixTraceNum++;
 if(this->TraceList)this->TraceList = (SPixTrace**)HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, this->TraceList, this->PixTraceNum*sizeof(SPixTrace*));
   else this->TraceList = (SPixTrace**)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, this->PixTraceNum*sizeof(SPixTrace*));
 SPixTrace* Trace   = (SPixTrace*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(SPixTrace)+(this->TotalPixels*sizeof(UPixel*)));
 Trace->StartX      = PosX;
 Trace->StartY      = PosY;
 Trace->BestColor   = BestColor;
 Trace->TotalPixels = this->TotalPixels;
 Trace->TotalWeight = this->TotalWeight;
 memcpy(&Trace->Pixels,this->PixTraceBuf,this->TotalPixels*sizeof(UPixel*));
 this->TraceList[this->PixTraceNum-1] = Trace;
}
//----------------------------------
void LocatePixel(UPixel* Pixel, PUINT PosX, PUINT PosY)
{
 UINT Index = (Pixel - (UPixel*)this->Bitmap.GetPixels());
 *PosY = Index / this->Bitmap.GetWidth();
 *PosX = Index % this->Bitmap.GetWidth();
}
//----------------------------------
bool IsPixelInRange(int PosX, int PosY){return !((PosX < 0)||(PosY < 0)||(PosX >= this->Bitmap.GetWidth())||(PosY >= this->Bitmap.GetHeight()));}
//----------------------------------
UINT CalcPixRect(SPixTrace* Trace, int* Top, int* Left, int* Right, int* Bottom)
{
 int ImgWidth   = this->Bitmap.GetWidth();
 UPixel* Pixels = (UPixel*)this->Bitmap.GetPixels();
 int pt = this->Bitmap.GetHeight();
 int pl = ImgWidth;
 int pr = 0;
 int pb = 0;
 for(UINT ctr=0;ctr < Trace->TotalPixels;ctr++)
  {
   UINT Index = Trace->Pixels[ctr] - Pixels;
   int  PosY  = Index / ImgWidth;
   int  PosX  = Index % ImgWidth;
   if(PosX < pl)pl = PosX;
   if(PosX > pr)pr = PosX;
   if(PosY < pt)pt = PosY;
   if(PosY > pb)pb = PosY;
  }
 if(Top)*Top = pt;
 if(Left)*Left = pl;
 if(Right)*Right = pr;
 if(Bottom)*Bottom = pb;
 return (pr - pl)*(pb - pt);
}
//----------------------------------
UPixel* FindPixel(SPixTrace* Trace, int PosX, int PosY, bool* OutOf=NULL)
{
 bool ORange = !this->IsPixelInRange(PosX,PosY);
 if(OutOf)*OutOf = ORange;
 if(ORange)return NULL; // The pixel is out of range
 UPixel* TgtAddr = (UPixel*)&this->Bitmap.GetRow(PosY)[PosX];        //    this->Bitmap.GetPixels()[(this->Bitmap.GetWidth() * PosY) + PosX];
 for(UINT ctr=0;ctr < Trace->TotalPixels;ctr++)
  {
   if(Trace->Pixels[ctr] == TgtAddr)return TgtAddr;  // The pixel is found
  }
 return NULL;
}
//----------------------------------
UPixel* GetPixel(int PosX, int PosY)
{
 if(!this->IsPixelInRange(PosX,PosY))return NULL; // The pixel is out of range
 return (UPixel*)&this->Bitmap.GetRow(PosY)[PosX];
}
//----------------------------------
void ClearWeights(void)
{
 UINT  TotalPix = this->Bitmap.GetWidth() * this->Bitmap.GetHeight();
 UPixel* Pixels = (UPixel*)this->Bitmap.GetPixels();
 for(UINT ctr=0;ctr < TotalPix;ctr++)Pixels[ctr].Weight = 0;          //this->Bitmap.GetPixels()[ctr] &= 0x00FFFFFF;
}
//----------------------------------
bool IsMatchColor(COLORREF ColorA, COLORREF ColorB)
{
 if(this->MaxClrDist == 0){if(ColorA != ColorB)return false;}
   else if(ColorDistance(ColorA, ColorB) > this->MaxClrDist)return false;
 return true;
}
//----------------------------------
bool GatherColorPixel(int PosX, int PosY, UINT Dist)
{
 if(Dist > this->GMaxPixDist)return false;
 if(this->RecuDepth > 160000)return false; // Stack overflow at depth 183294
 if(!this->IsPixelInRange(PosX,PosY))return false;  // The pixel is out of range
 register UPixel* Pixel = (UPixel*)&this->Bitmap.GetRow(PosY)[PosX];
 if(Pixel->Weight)return false;

 int hue, sat, val;
 bool res = false;
 RGBtoHSV(Pixel->Red, Pixel->Green, Pixel->Blue, &hue, &sat, &val);
 //if((hue >= this->GMinHue)||(sat >= this->GMinSat)){Dist = 0; res = true; this->PixTraceBuf[this->TotalPixels++] = Pixel;}
   if(sat >= 45){Dist = 0; res = true; this->PixTraceBuf[this->TotalPixels++] = Pixel;}
 Pixel->Weight = 1;
 this->RecuDepth++;
 res |= GatherColorPixel(PosX-1, PosY, Dist+1);
 res |= GatherColorPixel(PosX+1, PosY, Dist+1);
 res |= GatherColorPixel(PosX, PosY-1, Dist+1);
 res |= GatherColorPixel(PosX, PosY+1, Dist+1);
 this->RecuDepth--;
 return res;
}
//----------------------------------
// Pixel format: WEIGHT:B:G:R
// Weight is > 0 for any traced pixel
// NOTE: This function is called more than once for near pixels
// Only 3 args on the Stack + Locals
// NOTE: Can overflow the Stack
//
bool TracePixel(int PosX, int PosY, UPixel* Adjacent=NULL)
{
 if(this->RecuDepth > 160000)return false; // Stack overflow at depth 183294
 if(!this->IsPixelInRange(PosX,PosY))return false;  // The pixel is out of range
 register UPixel*  Pixel = (UPixel*)&this->Bitmap.GetRow(PosY)[PosX];
 register COLORREF Color = Pixel->GetColor();
 if(Color == this->BgrColor)return false;  // Already traced(Weight) or no need to trace(BgrColor)
 if(!this->IsMatchColor(this->PixColor,Color))return false;
 if(Pixel->Weight){if(Adjacent && IsMatchColor(this->PixColor,Adjacent->GetColor())){Pixel->Weight++;return true;} else return false;}  // Add more weight to an adjacent pixels

 this->RecuDepth++;
 Pixel->Weight  = 1;
 Pixel->Weight += this->TracePixel(PosX,   PosY-1, Pixel);  // Top
 Pixel->Weight += this->TracePixel(PosX-1, PosY,   Pixel);  // Left
 Pixel->Weight += this->TracePixel(PosX,   PosY+1, Pixel);  // Bottom
 Pixel->Weight += this->TracePixel(PosX+1, PosY,   Pixel);  // Right

 this->TracePixel(PosX-1, PosY-1);   // Left-Top
 this->TracePixel(PosX-1, PosY+1);   // Left-Bottom
 this->TracePixel(PosX+1, PosY-1);   // Right-Top
 this->TracePixel(PosX+1, PosY+1);   // Right-Bottom

 this->PixTraceBuf[this->TotalPixels] = Pixel;
 this->TotalWeight += Pixel->Weight;
 this->TotalPixels++;
 this->RecuDepth--;
 return true;
}
//----------------------------------
bool ConnectPixel(SPixTrace* Trace, UPixel* Pixel, int APosX, int APosY, int BPosX, int BPosY)
{
 bool ROutA, ROutB;
 UPixel* PixA = this->FindPixel(Trace, APosX, APosY, &ROutA);
 UPixel* PixB = this->FindPixel(Trace, BPosX, BPosY, &ROutB);
 if(PixA || PixB || ROutA || ROutB)return false; // not an edge pixel
 PixA = (UPixel*)&this->Bitmap.GetRow(APosY)[APosX];
 PixB = (UPixel*)&this->Bitmap.GetRow(BPosY)[BPosX];
 if((PixA->GetColor() == this->BgrColor)||(PixB->GetColor() == this->BgrColor))return false;  // Breaks

 if((PixA->Red > PixB->Red)&&(PixA->Blue > PixB->Blue)&&(PixA->Green > PixB->Green))Pixel->SetColor(((this->FixByBrignt)?(PixA->GetColor()):(PixB->GetColor()))); // Use HSV:V instead to get a brightest pixel?
   else Pixel->SetColor(((this->FixByBrignt)?(PixB->GetColor()):(PixA->GetColor())));
 return true;
}
//----------------------------------
UINT CleanPixels(SPixTrace* Trace)
{
 for(UINT ctr=0;ctr < Trace->TotalPixels;ctr++)
  {
   UINT PosX, PosY;
   UPixel* Pixel = Trace->Pixels[ctr];
   this->LocatePixel(Pixel, &PosX, &PosY);
   if(ConnectPixel(Trace, Pixel, PosX-1, PosY+1, PosX+1, PosY-1))continue;
   if(ConnectPixel(Trace, Pixel, PosX-1, PosY-1, PosX+1, PosY+1))continue;
   if(ConnectPixel(Trace, Pixel, PosX-1, PosY,   PosX+1, PosY  ))continue;
   if(ConnectPixel(Trace, Pixel, PosX,   PosY+1, PosX,   PosY-1))continue;

   Pixel->SetColor(this->BgrColor);//0x00FF0000 this->BgrColor); // !!!!!!!!!!!  Use color of a nearest pixel, prefer not a background ?
  }
 return Trace->TotalPixels;
}
//----------------------------------

public:
//----------------------------------
CPixTracer(UINT MinDetCnt=150, UINT MaxDetCnt=250, UINT MinDelCnt=30, UINT MaxDelCnt=2500, float MaxDist=0)
{
 this->MaxClrDist   = MaxDist;
 this->MaxDetPixels = MaxDetCnt;
 this->MinDetPixels = MinDetCnt;
 this->MaxDelPixels = MaxDelCnt;
 this->MinDelPixels = MinDelCnt;
 this->PixTraceNum  = 0;
 this->TraceList    = NULL;
 this->PixTraceBuf  = NULL;
 this->FixByBrignt  = true;
}
//----------------------------------
~CPixTracer()
{
 if(this->PixTraceBuf)HeapFree(GetProcessHeap(), NULL, this->PixTraceBuf);
 this->FreeTraceList();
}
//----------------------------------
CMemBitmap* GetBitmap(void){return &this->Bitmap;}
//----------------------------------
UINT ColorBitmap(void)
{
 UINT ClnTotal = 0;
 for(UINT ctr=0;ctr < this->PixTraceNum;ctr++)
  {
   SPixTrace* Trace = this->TraceList[ctr];
   for(UINT idx=0;idx < Trace->TotalPixels;idx++)
	{
	 Trace->Pixels[idx]->SetColor((Trace->StartX * Trace->StartY)+64); // +(this->TotalWeight/2)
	}
   ClnTotal += Trace->TotalPixels;
  }
 return ClnTotal;
}
//----------------------------------
//  We serach for more pixels with less total weight
//
COLORREF FindTargetColor(bool GatherAllSeq=false)
{
 COLORREF BestColor = -1;
 int  BestCol     = -1;
 int  BestRow     = -1;
 UINT BestTotPix  = 0;
 UINT BestTotWgt  = -1;
 int  ImgWidth    = this->Bitmap.GetWidth();
 int  ImgHeight   = this->Bitmap.GetHeight();
 this->ClearWeights();
 this->FreeTraceList();
 for(int row=0;row < ImgHeight;row++)  // Gather all available Pixels in the range and find a nearest one
  {
   COLORREF* PixRow = this->Bitmap.GetRow(row);
   for(int col=0;col < ImgWidth;col++)
	{
	 this->PixColor    = PixRow[col];
	 this->TotalPixels = 0;
	 this->TotalWeight = 0;
	 this->RecuDepth   = 0;
	 if(this->TracePixel(col, row))
	  {
	   UINT ExcessWeight = this->TotalWeight - this->TotalPixels;
	   if((ExcessWeight <= BestTotWgt) && (this->TotalPixels >= this->MinDetPixels) && (this->TotalPixels <= this->MaxDetPixels))   // NOT (this->TotalPixels >= BestTotPix) !!!
		{
		 BestColor  = this->PixColor;
		 BestTotWgt = ExcessWeight;
		 BestTotPix = this->TotalPixels;  // For debugging?
		 BestCol    = col;  // For debugging
		 BestRow    = row;  // For debugging
		 //if(this->TotalPixels >= this->MinDelPixels)this->AddTraceEntry(this->PixColor, col, row);  // Save for possible later cleanup
		}
	   //if(this->TotalPixels >= this->MinDelPixels)this->AddTraceEntry(this->PixColor, col, row);  // Save for possible later cleanup   //else if(GatherAllSeq && (this->TotalPixels >= this->MinDelPixels))this->AddTraceEntry(this->PixColor, col, row);  // Use it here for debug coloring
	   if(GatherAllSeq)this->AddTraceEntry(this->PixColor, col, row);  // A third version :)
	  }
	}
  }
 this->PixColor = BestColor;
 return BestColor;
}
//----------------------------------
UINT GatherColorSegments(COLORREF Color)
{
 int  ImgWidth  = this->Bitmap.GetWidth();
 int  ImgHeight = this->Bitmap.GetHeight();
 this->ClearWeights();
 this->FreeTraceList();
 for(int row=0;row < ImgHeight;row++)  // Gather all available Pixels in the range and find a nearest one
  {
   COLORREF* PixRow = this->Bitmap.GetRow(row);
   for(int col=0;col < ImgWidth;col++)
	{
	 if(PixRow[col]   != Color)continue;  // Trace only the target color
	 this->PixColor    = Color;
	 this->TotalPixels = 0;
	 this->TotalWeight = 0;
	 this->RecuDepth   = 0;
	 if(this->TracePixel(col, row))
	  {
	   if((this->TotalPixels >= this->MinDelPixels)&&(this->TotalPixels <= this->MaxDelPixels))this->AddTraceEntry(this->PixColor, col, row);  // Save for possible later cleanup
	  }
	}
  }
 return this->PixTraceNum;
}
//----------------------------------
UINT CleanColorSegments(COLORREF Color)
{
 UINT ClnTotal = 0;
 this->ClearWeights();
 for(UINT ctr=0;ctr < this->PixTraceNum;ctr++)
  {
   if(this->TraceList[ctr]->BestColor != Color)continue;    // Clean only the target color
   ClnTotal += this->CleanPixels(this->TraceList[ctr]);
  }
 return ClnTotal;
}
//----------------------------------
UINT AutoCleanImage(UINT MinPixels, UINT MaxPixels, COLORREF Color=-1, UINT DMax=25)
{
 for(int ClrDist=0;ClrDist <= DMax;ClrDist++)
  {
   this->MaxClrDist  = ClrDist;
   COLORREF TgtColor = ((Color == -1)?(this->FindTargetColor()):(Color));
   if((TgtColor == -1) || !this->GatherColorSegments(TgtColor))continue; // No segments
   this->Bitmap.UndoPush();
   UINT PixNum = this->CleanColorSegments(TgtColor);
   if((PixNum > MinPixels)&&(PixNum < MaxPixels))
	{
	 this->Bitmap.UndoSkip();
	 return PixNum;
	}
   this->Bitmap.UndoPop();
  }
 return 0;
}
//----------------------------------
UINT AnalyzeAsBitMask(void)
{
 this->ClearWeights();
 this->MaxClrDist   = 64;
 this->MinDelPixels = 4;
 this->MinDetPixels = 4;
 this->MaxDetPixels = 9999;
 this->PixColor     = this->FindTargetColor(true);
 this->MaxClrDist   = 0;

 UINT     MinPix   = -1;
 UINT     MaxPix   = 0;
 UINT     PixTotal = 0;
 float    MaxDist  = 0;
 COLORREF MinColor = -1;
 COLORREF MaxColor = 0;
 for(UINT ctr=0;ctr < this->PixTraceNum;ctr++)
  {
   SPixTrace* Trace = this->TraceList[ctr];
   if(Trace->BestColor != this->PixColor)continue;    // Clean only the target color
   UINT Pixels = Trace->TotalPixels;
   for(UINT idx=0;idx < Pixels;idx++)
	{
	 COLORREF CurColor = Trace->Pixels[idx]->GetColor();
	 if(CurColor < MinColor)MinColor = CurColor;
	 if(CurColor > MaxColor)MaxColor = CurColor;
	}
   if(Pixels < MinPix)MinPix = Pixels;
   if(Pixels > MaxPix)MaxPix = Pixels;
   PixTotal += Pixels;
  }
 this->MaxClrDist   = ColorDistance(MinColor, MaxColor);
 this->MinDelPixels = MinPix - (MinPix/8);   // ((MinPix > 15)?(MinPix-5):(MinPix));
 this->MaxDetPixels = MaxPix + (MaxPix/8);   // 30;
 this->MinDetPixels = (this->MaxDetPixels / 2) + (MaxPix/8);  // +30
 if(this->MinDetPixels > this->MaxDetPixels)this->MaxDetPixels = this->MinDetPixels + 30;
 return PixTotal;
}
//----------------------------------
void ReinitImage(void)
{
 this->FreeTraceList();
 if(this->Bitmap.IsEmpty())return;
 this->BgrColor = this->Bitmap.FindMostFreqColor();
 UINT PixCount  = this->Bitmap.GetWidth() * this->Bitmap.GetHeight();
 if(this->PixTraceBuf)this->PixTraceBuf = (UPixel**)HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, this->PixTraceBuf, PixCount*sizeof(UPixel*));
   else this->PixTraceBuf = (UPixel**)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, PixCount*sizeof(UPixel*));
}
//----------------------------------
bool LoadImage(LPSTR FileName)
{
 if(this->Bitmap.LoadFromFile(FileName) != 0)return false;
 this->ReinitImage();
 return true;
}
//----------------------------------
bool SaveImage(LPSTR FileName)
{
 return (this->Bitmap.SaveToFile(FileName) == 0);
}
//----------------------------------
void ClearImage(void)
{
 this->Bitmap.FreePixels();
 this->FreeTraceList();
}
//----------------------------------
//           SPECIALS
//----------------------------------
UINT ClearColoredStamps(CMemBitmap* Stamp, int MinHue=5, int MinSat=5, int MaxPixDist=3, int MinToRemove=20)  //  MaxPixDist=16
{
 int ImgWidth  = this->Bitmap.GetWidth();
 int ImgHeight = this->Bitmap.GetHeight();
 this->ClearWeights();
 this->FreeTraceList();

 this->GMaxPixDist = MaxPixDist;
 this->GMinHue     = MinHue;
 this->GMinSat     = MinSat;
 for(int row=0;row < ImgHeight;row++)  // Gather all available Pixels in the range and find a nearest one
  {
   COLORREF* PixRow = this->Bitmap.GetRow(row);
   for(int col=0;col < ImgWidth;col++)
	{
	 int hue, sat, val;
	 UPixel*  Pixel = (UPixel*)&PixRow[col];
	 if(Pixel->Weight)continue;
	 RGBtoHSV(Pixel->Red, Pixel->Green, Pixel->Blue, &hue, &sat, &val);
	 //if((hue < MinHue)&&(sat < MinSat))continue;
       if(sat < 45)continue;

	 this->TotalPixels = 0;
	 this->TotalWeight = 0;
	 this->RecuDepth   = 0;
	 if(this->GatherColorPixel(col, row, 0))this->AddTraceEntry(0, col, row);   // End of last pixel group
	}
  }
 UINT TotalRemoved = 0;
 for(UINT ctr=0;ctr < this->PixTraceNum;ctr++)
  {
   int Top, Left, Right, Bottom;
   SPixTrace* Trace = this->TraceList[ctr];
   int PicCtr = this->CalcPixRect(Trace, &Top, &Left, &Right, &Bottom);
   if(PicCtr < MinToRemove)continue;
   if(Stamp)Stamp->StretchToBitmap(&this->Bitmap,Left,Top,Right-Left+1,Bottom-Top+1);
	 else this->Bitmap.FillPixRect(this->BgrColor, Top, Left, Right, Bottom);
   TotalRemoved += PicCtr;
  }
 this->ClearWeights();
 this->FreeTraceList();
 return TotalRemoved;
}
//----------------------------------


};
//---------------------------------------------------------------------------
#endif
