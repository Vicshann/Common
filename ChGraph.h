//---------------------------------------------------------------------------

#ifndef ChGraphH
#define ChGraphH

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


#include "MemBitmap.h"
//---------------------------------------------------------------------------
class CChGraph
{
 UINT     TrGrphCIdx;   // Current index in the Cycled buffer
 UINT     TrGrphRMax;   // Max resolution
 UINT     TrGrphSize;   // One STrGraph = 1s
 struct   STrGraph {DWORD TrRec; DWORD TrTrm;}*TrGrphData;

template<class N, class M> __inline M NumToPerc(N Num, M MaxVal){return (((Num)*100)/(MaxVal));}
template<class P, class M> __inline M PercToNum(P Per, M MaxVal){return (((Per)*(MaxVal))/100);}

public:

CChGraph(void)
{
 TrGrphCIdx = TrGrphRMax = 0;
 this->SetTraffBuff(60*30);  // 30min
}
//-------------------
void Reset(void)
{
 TrGrphCIdx = TrGrphRMax = 0;
 memset(TrGrphData,0,TrGrphSize);
}
//-------------------
void SetTraffBuff(UINT Secs)  // Don`t call this when Graph is drawed from other thread!
{
 TrGrphCIdx = 0;
 TrGrphSize = Secs;
 TrGrphData = (STrGraph*)calloc(TrGrphSize,sizeof(STrGraph));
}
//-------------------
void AddTraffData(UINT Secs, UINT TrRec, UINT TrTrm)    // Input are in kb/s
{
 if(TrRec > TrGrphRMax)TrGrphRMax = TrRec;  // Resize Graph
 if(TrTrm > TrGrphRMax)TrGrphRMax = TrTrm;
 for(;Secs;Secs--)
  {
   this->TrGrphData[TrGrphCIdx].TrRec = TrRec;
   this->TrGrphData[TrGrphCIdx].TrTrm = TrTrm;
   TrGrphCIdx++;
   if(TrGrphCIdx >= TrGrphSize)TrGrphCIdx = 0;  // Cycled buffer
  }
}
//-------------------
void GetTraffData(UINT Index, PUINT TrRecPerc, PUINT TrTrmPerc)
{
 UINT RIdx  = (TrGrphCIdx + Index);
 if(RIdx > TrGrphSize)RIdx = (RIdx - TrGrphSize);
 *TrRecPerc = NumToPerc(this->TrGrphData[RIdx].TrRec, this->TrGrphRMax);  // Recalc max to shrink the Graph?
 *TrTrmPerc = NumToPerc(this->TrGrphData[RIdx].TrTrm, this->TrGrphRMax);
}
//-------------------
void GetTraffMaxSample(int MinPos, int MaxPos, PUINT TrRecPerc, PUINT TrTrmPerc)
{
 int len  = MaxPos - MinPos;
 if(MinPos < 0)MinPos = 0;
 int RIdx = TrGrphCIdx + MinPos;
 if(RIdx >= TrGrphSize)RIdx = RIdx % TrGrphSize;

 *TrRecPerc = *TrTrmPerc = 0;
 for(;len > 0;len--,RIdx++)  // Backward reading
  {
   if(RIdx >= TrGrphSize)RIdx = 0;
   UINT TrRec = this->TrGrphData[RIdx].TrRec;
   UINT TrTrm = this->TrGrphData[RIdx].TrTrm;
   if(TrRec > *TrRecPerc)*TrRecPerc = TrRec;
   if(TrTrm > *TrTrmPerc)*TrTrmPerc = TrTrm;
  }
 if(this->TrGrphRMax)
  {
   *TrRecPerc = NumToPerc(*TrRecPerc, this->TrGrphRMax);  // Recalc max to shrink the Graph?
   *TrTrmPerc = NumToPerc(*TrTrmPerc, this->TrGrphRMax);
  }
}
//-------------------
void CreateTraffGraph(CMemBitmap* Bmp)
{
 int Width  = Bmp->GetWidth();
 int Height = Bmp->GetHeight();
 int LstSampPos = -1;

 Bmp->FillPixRect(0,0,0,Width-1,Height-1);
 UINT LastTrRec = -1;
 UINT LastTrTrm = -1;
 UINT TrRecPerc = 0;
 UINT TrTrmPerc = 0;
 UINT GraphMax  = 0;
 for(int ctr=0;ctr < TrGrphSize;ctr++)
  {
   UINT TrRec = this->TrGrphData[ctr].TrRec;
   UINT TrTrm = this->TrGrphData[ctr].TrTrm;
   if(TrRec > GraphMax)GraphMax = TrRec;  // Resize Graph
   if(TrTrm > GraphMax)GraphMax = TrTrm;
  }
 this->TrGrphRMax = GraphMax;    // Update Max value // SLOW!!!
 for(int PosX=0;PosX < Width;PosX++)
  {
   double perc = NumToPerc((double)PosX,(double)Width);
   UINT SamplePos = PercToNum(perc, (double)this->TrGrphSize);
   if(SamplePos != LstSampPos)
	{
	 this->GetTraffMaxSample(LstSampPos, SamplePos, &TrRecPerc, &TrTrmPerc);
	 TrRecPerc  = PercToNum(TrRecPerc, Height-2);   // Y
	 TrTrmPerc  = PercToNum(TrTrmPerc, Height-2);   // Y
	 if(TrRecPerc < 1)TrRecPerc = 1;
	 if(TrTrmPerc < 1)TrTrmPerc = 1;
	 LstSampPos = SamplePos;
	}
   for(int PosY=0;PosY < TrRecPerc;PosY++)Bmp->GetRow(PosY)[PosX] ^= 0x007F0000;  // FF  // RED
   for(int PosY=0;PosY < TrTrmPerc;PosY++)Bmp->GetRow(PosY)[PosX] ^= 0x00007F00;  // FF  // GREEN
  }
}
//-------------------

};
//---------------------------------------------------------------------------
#endif
