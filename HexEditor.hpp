//---------------------------------------------------------------------------

#ifndef HexEditorH
#define HexEditorH
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

#include <Windows.h>
//---------------------------------------------------------------------------
extern void   _cdecl LogProc(char* ProcName, char* Message, ...);

class CHexEditor
{
public:
static const COLORREF ClrDisabled = 0x80000000;
static const COLORREF ClrNoChange = 0xFF000000;

enum ESelType {stNone,stDisp,stText,stHex,stCustom}; // stCustom must be 4

private:
class CByteSource
{
public:
 virtual ~CByteSource(){};
 virtual UINT64 GetSize(void) = 0;
 virtual UINT64 GetPosition(void) = 0;
 virtual PBYTE  GetBuffer(void) = 0;
 virtual bool SetPosition(UINT64 NewPos) = 0;
 virtual bool SetSize(UINT64 NewSize) = 0;
 virtual UINT ReadBlock(PBYTE Buffer, UINT64 Offset, UINT Length) = 0;
 virtual UINT WriteBlock(PBYTE Buffer, UINT64 Offset, UINT Length) = 0;
};
//-------------------------------------
class CMemSrc: public CByteSource
{
 PBYTE  Data;
 UINT64 Size;
public:
 virtual ~CMemSrc(){HeapFree(GetProcessHeap(),0,this->Data);}
 virtual UINT64 GetSize(void){return this->Size;}
 virtual UINT64 GetPosition(void){return 0;}
 virtual PBYTE  GetBuffer(void){return this->Data;}
 virtual bool SetPosition(UINT64 NewPos){return true;}
 virtual bool SetSize(UINT64 NewSize)
  {
   if(NewSize == this->Size)return true;
   if(!NewSize){HeapFree(GetProcessHeap(),0,this->Data);this->Data=NULL;this->Size=0;return true;}
   if(!this->Data)this->Data = (PBYTE)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,NewSize+64);
	 else this->Data = (PBYTE)HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,this->Data,NewSize+64);
   this->Size = (this->Data)?(NewSize):(0);
   return true;
  }
 virtual UINT ReadBlock(PBYTE Buffer, UINT64 Offset, UINT Length)
  {
   if(Offset > this->Size)return 0;
   if((Offset+Length) > this->Size)Length = this->Size - Offset;  // Rest of data
   CopyMemory(Buffer,&this->Data[Offset],Length);
   return Length;
  }
 virtual UINT WriteBlock(PBYTE Buffer, UINT64 Offset, UINT Length)
  {
   if((Offset+Length) > this->Size)this->SetSize(Offset+Length);
   CopyMemory(&this->Data[Offset],Buffer,Length);
   return Length;
  }
};
//-------------------------------------
struct CSelArray
{
struct SSelRec
{
 UINT64 SelFirst;
 UINT64 SelLast;
 COLORREF Color;
};
 SSelRec* Array;
 UINT Number;
//---------
 CSelArray(void){FillMemory(this,sizeof(CSelArray),0x00);}
 ~CSelArray(void){this->Clear();}
//---------
 void Clear(void)
  {
   this->Number = 0;
   HeapFree(GetProcessHeap(),0,this->Array);
   this->Array  = NULL;
  }
//---------
 void Add(UINT64 First, UINT64 Last, COLORREF Color)
  {
   this->Number++;
   if(!this->Array)this->Array = (SSelRec*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,(this->Number * sizeof(SSelRec))+64);
	 else this->Array = (SSelRec*)HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,this->Array,(this->Number * sizeof(SSelRec))+64);
   this->Array[this->Number-1].SelFirst = First;
   this->Array[this->Number-1].SelLast  = Last;
   this->Array[this->Number-1].Color    = Color;
  }
//---------
 void Del(UINT Index)
  {
   if(Index >= this->Number)return;
   this->Number--;
   if(Index < this->Number)CopyMemory(&this->Array[Index],&this->Array[Index+1],(this->Number-Index)*sizeof(SSelRec));
   if(this->Number > 0)this->Array = (SSelRec*)HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,this->Array,(this->Number * sizeof(SSelRec))+64);
	 else {HeapFree(GetProcessHeap(),0,this->Array);this->Array=NULL;}
  }
//---------
 SSelRec* Get(UINT Index)
  {
   if(Index >= this->Number)return NULL;
   return &this->Array[Index];
  }
 UINT Count(void){return this->Number;}
};
//-------------------------------------
enum ECursMove {cmTop,cmCenter,cmBottom};
 CByteSource* src;
 UINT64 SelFirst;
 UINT64 SelLast;
 UINT64 DataLen;
 UINT64 DataPos;
 UINT64 DataOffs;  // View / 0 is default
 UINT64 CursPos;
 COLORREF CurClr;
 COLORREF TxtClr;
 COLORREF BgrClr;
 COLORREF SelClr;
 ESelType SelType;
 ESelType CurType;
 HBITMAP hMemBmp;
 HDC     hMemDC;
 HWND    TgtWnd;
 HFONT   Font;
 RECT WrkRc;
 int MPosX;
 int MPosY;
 int AdrChrs;
 int HexOff;
 int TxtOff;
 int WTop;
 int WLeft;
 int WWidth;
 int WHeight;
 int MaxChars;
 int MaxLines;    // Max lines to display, not an actual data
 int AddrSize;    // Address size of a displayed block (4 or 8 bytes)
 int CharWidth;
 int CharHeight;
 int ColDelSize;   // Size of column delimiter in chars
 int BytesInRow;
 int VisibleBytes;
 bool RebuildHls;
 bool NeedRedraw;
 bool CursSecHalf;
 bool CursVisible;
 BITMAPINFO SelBmp;
 COLORREF*  SelPixs;
 COLORREF*  SelHigh;
 CSelArray  SelArr;

//---------------------------------------------
static WORD HexToCharUpC(BYTE Value)
{
 WORD Result = 0;
 for(int ctr=1;ctr >= 0;ctr--)
  {
   BYTE Tmp = (Value & 0x0F);
   if(Tmp < 0x0A)((PBYTE)&Result)[ctr] |= 0x30+Tmp;
	 else ((PBYTE)&Result)[ctr] |= 0x37+Tmp;  // Uppercase
   Value  >>= 4;
  }
 return Result;
}
//---------------------------------------------
static bool TrimRange(UINT64& First, UINT64& Last, UINT64 TrimToLen)  // Returns 'true' if range becomes invalid
{
 if(TrimToLen)
  {
   if(Last >= TrimToLen)Last = TrimToLen;
   if(First <= Last)return false;
  }
 First = Last = 0;
 return true;
}
//---------------------------------------------
void DrawData(void)    // TODO: Draw to memory and blend with selection only when changed
{
 if(!this->DataLen)return;
 BYTE Line[1056]; // Addr=8(16)+2+2, Data*4 = Max 256 BytesInRow
 BYTE Data[256];
 int   TxTop = 0;
 int   ASize = this->AddrSize - 1;
 int   BLeft = this->VisibleBytes;
 UINT64 DLen = this->DataLen;
 UINT64 DPtr = this->DataPos;
 FillMemory(&Line,sizeof(Line),0x20);
 for(int ctr=0;(ctr < this->MaxLines)&&(BLeft > 0);ctr++,DPtr+=this->BytesInRow,BLeft-=this->BytesInRow,TxTop+=this->CharHeight)
  {
   int    LPos = 0;
   int    BRow = (BLeft > this->BytesInRow)?(this->BytesInRow):(BLeft);
   UINT64 DPos = DPtr + this->DataOffs;
   this->src->ReadBlock((PBYTE)&Data, DPtr, this->BytesInRow);
   for(int actr=0;actr <= ASize;actr++,LPos+=2)
	{
	 WORD chr = HexToCharUpC(((PBYTE)&DPos)[actr]);
	 int CPos = (ASize-actr)*2;
	 Line[CPos+0] = ((PBYTE)&chr)[0];
	 Line[CPos+1] = ((PBYTE)&chr)[1];
	}
   LPos += this->ColDelSize;
   for(int dctr=0;dctr < BRow;dctr++,LPos+=3)
	{
	 WORD chr = HexToCharUpC(Data[dctr]);
	 Line[LPos+0] = ((PBYTE)&chr)[0];
	 Line[LPos+1] = ((PBYTE)&chr)[1];
	}
   for(;LPos < this->TxtOff;LPos++)Line[LPos] = 0x20;
   for(int dctr=0;dctr < BRow;dctr++,LPos++)
	{
	 BYTE Val = Data[dctr];
	 if(Val < 0x20)Val = '.';
	 Line[LPos] = Val;
	}
   TextOut(this->hMemDC,0,TxTop,(LPSTR)&Line,LPos);
  }
}
//---------------------------------------------
void DrawCursor(HDC TgtDc)
{
 RECT RHex;
 RECT RTxt;
 int  HexX, TxtX, HexY, TxtY;

 if(!BytePosToChars(this->CursPos, &HexX, &TxtX, &HexY))return;
 TxtY  = HexY;
 HexX += (bool)this->CursSecHalf;
 bool DoHex = true;
 bool DoTxt = true;
 if(HexX < this->MaxChars)CharToPixel(HexX, HexY, true);
   else DoHex = false;
 if(TxtX < this->MaxChars)CharToPixel(TxtX, TxtY, true);
   else DoTxt = false;
 if(DoHex)
  {
   RHex.top    = HexY;
   RHex.left   = HexX;
   RHex.right  = HexX + this->CharWidth;
   RHex.bottom = HexY + this->CharHeight;
   InvertRect(TgtDc,&RHex);
  }
 if(DoTxt)
  {
   RTxt.top    = TxtY;
   RTxt.left   = TxtX;
   RTxt.right  = TxtX + this->CharWidth;
   RTxt.bottom = TxtY + this->CharHeight;
   InvertRect(TgtDc,&RTxt);
  }
}
//---------------------------------------------
void HighlightChar(int X, int Y, COLORREF Color, COLORREF* Pixels, int PixStep=1) // Note: No bounds check here!
{
 if(X >= this->MaxChars)return;
 int PixPosX = (this->CharWidth  * X);
 int PixPosY = (this->CharHeight * Y);
 Color ^= this->BgrClr;  // Keep color on background
 for(int CtrY=0;CtrY < this->CharHeight;CtrY++)
  {
   COLORREF* Row = &Pixels[(PixPosY+CtrY)*this->WWidth];
   for(int CtrX=0;CtrX < this->CharWidth;CtrX+=PixStep)
	{
	 COLORREF* Cur = &Row[PixPosX+CtrX];
	 *Cur = (*Cur ^ Color) & 0x00FFFFFF;
	}
  }
}
//---------------------------------------------
void HighlightRange(ESelType Type, UINT64 First, UINT64 Last, COLORREF Color, COLORREF* Pixels)  // Highlites only visible part of Range
{
 if(Last < First){UINT64 tmp = First; First = Last; Last = tmp;}  // Exchange
 UINT64 Vis = (this->DataPos+this->VisibleBytes);
 if((Last < this->DataPos)||(First >= Vis))return;  // Not visible

 int OffTo   = ((Last >= Vis)?(this->VisibleBytes):(Last-this->DataPos));
 int OffFrom = ((First <= this->DataPos)?(0):(First - this->DataPos));
 int Range   = OffTo - OffFrom + 1;
 int FLine   = OffFrom / this->BytesInRow;
 int LLine   = OffTo   / this->BytesInRow;
 if(LLine >= this->MaxLines)LLine = this->MaxLines-1;

 if(Type == stDisp)   // Select as DISPLAY text
  {
   for(int ypos=FLine;ypos <= LLine;ypos++)
	{
	 for(int xpos=0;xpos < MaxChars;xpos++)this->HighlightChar(xpos, ypos, Color, Pixels);
	}
   return;
  }

 int HexFill = 1;// + (Type != stHex) - (Type == stCustom);
 int TxtFill = 1;// + (Type != stText)- (Type == stCustom);
 for(int ypos=FLine,LnOffs=(OffFrom % this->BytesInRow);ypos <= LLine;ypos++)
  {
   bool DoSel = true;
   for(int xpos=LnOffs;DoSel;) 
	{
	 int xoffs = xpos * 3;
	 this->HighlightChar(xpos+TxtOff, ypos, Color, Pixels, TxtFill);
	 xpos++;
	 Range--;
	 DoSel = (xpos < this->BytesInRow)&&(Range > 0);
	 this->HighlightChar(xoffs+HexOff, ypos, Color, Pixels, HexFill);
	 this->HighlightChar(xoffs+HexOff+1, ypos, Color, Pixels, HexFill);
	 if(DoSel)this->HighlightChar(xoffs+HexOff+2, ypos, Color, Pixels, HexFill);
	}
   LnOffs = 0;
  }
}
//---------------------------------------------
void DrawHighlights(bool HiRedraw)
{
 if(HiRedraw)
  {
   for(int ctr=0,total=(this->WWidth*this->WHeight);ctr < total;ctr++)this->SelHigh[ctr] = this->BgrClr;
   CSelArray::SSelRec* sel = NULL;
   for(int Index=0;sel = SelArr.Get(Index);Index++)if((int)sel->Color >= 0)this->HighlightRange(stCustom, sel->SelFirst, sel->SelLast, sel->Color, this->SelHigh);
  }
 for(int ctr=0,total=(this->WWidth*this->WHeight);ctr < total;ctr++)this->SelPixs[ctr] = this->BgrClr;   // Clear selection
 if(this->SelType)this->HighlightRange(this->SelType, this->SelFirst, this->SelLast, this->SelClr, this->SelPixs);   // Redraw selection
 for(int ctr=0,total=(this->WWidth*this->WHeight);ctr < total;ctr++)
  {
   if(this->SelHigh[ctr] == this->BgrClr)continue;  // Nothing to copy
   if(this->SelPixs[ctr] == this->BgrClr)this->SelPixs[ctr] = this->SelHigh[ctr];  // Direct copy
	 else this->SelPixs[ctr] = (this->SelPixs[ctr] ^ this->SelHigh[ctr]) & 0x00FFFFFF;
  }
 SetDIBits(this->hMemDC,this->hMemBmp,0,this->WHeight,this->SelPixs,&this->SelBmp,DIB_RGB_COLORS);
}
//---------------------------------------------
bool PixelToChar(int& X, int& Y, bool Margin)   // In owner window coordinates
{
 if(Margin){X -= this->WLeft; Y -= this->WTop;}
 if((X < 0)||(Y < 0))return false;
 X  = (X / this->CharWidth);
 Y  = (Y / this->CharHeight);
 if(X > this->MaxChars)X = this->MaxChars - 1;
 if(Y > this->MaxLines)Y = this->MaxLines - 1;
 return true;
}
//---------------------------------------------
bool CharToPixel(int& X, int& Y, bool Margin)
{
 if(X > this->MaxChars)X = this->MaxChars - 1;
 if(Y > this->MaxLines)Y = this->MaxLines - 1;
 X *= this->CharWidth;
 Y *= this->CharHeight;
 if(Margin){X += this->WLeft; Y += this->WTop;}
 return true;
}
//---------------------------------------------
UINT64 CharToBytePos(int X, int Y, ESelType* Type=NULL)
{
 if(X < this->AdrChrs)           // Address zone
  {
   if(Type)*Type = stDisp;
   X = (this->VisibleBytes % this->BytesInRow);
   if(X < 1)X = this->BytesInRow;
   X--;
  }
   else
	{
 if(Type)*Type = stNone;
 if(X >= (this->TxtOff+this->BytesInRow))return -1; // Out of field
 if(X >= this->TxtOff)
  {
   if(Type)*Type = stText;
   X -= this->TxtOff;  // Text Zone
  }
   else
	{
	 if(X >= (this->TxtOff - this->ColDelSize))return -1; // On delimiter
	 if(X < this->HexOff)return -1; // On delimiter
	 if(Type)*Type = stHex;
	 X -= this->HexOff;
	 X  = X / 3;
	}
   }
 int ByteOffs = (Y * this->BytesInRow) + X;
 UINT64 res = this->DataPos + ByteOffs;
 if(res >= this->DataLen){if(Type)*Type = stNone; return -2;}
 return res;
}
//---------------------------------------------
bool BytePosToChars(UINT64 Pos, int* HexX, int* TxtX, int* PosY)
{
 if((Pos < this->DataPos)||(Pos >= (this->DataPos+this->VisibleBytes)))return false; // Not in visible range
 int Offset = Pos - this->DataPos;
 int Line   = Offset / this->BytesInRow;
 int LOff   = Offset % this->BytesInRow;
 if(PosY)*PosY = Line;
 if(TxtX)*TxtX = this->TxtOff + LOff;
 if(HexX)*HexX = (this->HexOff)+(LOff*3);   // First char of the pair
 return true;
}
//---------------------------------------------
void ReleaseTarget(void)
{
 if(this->hMemBmp)DeleteObject(this->hMemBmp);
 if(this->hMemDC )DeleteDC(this->hMemDC);
 if(this->SelPixs){HeapFree(GetProcessHeap(),0,this->SelPixs);this->SelPixs=NULL;}
 if(this->SelHigh){HeapFree(GetProcessHeap(),0,this->SelHigh);this->SelHigh=NULL;}
}
//---------------------------------------------


public:
static long CharToHex(BYTE CharValue)
{
 if((CharValue >= 0x30)&&(CharValue <= 0x39))return (CharValue - 0x30);		 // 0 - 9
 if((CharValue >= 0x41)&&(CharValue <= 0x46))return (CharValue - (0x41-10)); // A - F
 if((CharValue >= 0x61)&&(CharValue <= 0x66))return (CharValue - (0x61-10)); // a - f
 return -1;
}
//---------------------------------------------
static WORD HexToChar(BYTE Value)
{
 WORD Result = 0;
 for(int ctr=1;ctr >= 0;ctr--)
  {
   BYTE Tmp = (Value & 0x0F);
   if(Tmp < 0x0A)((PBYTE)&Result)[ctr] |= 0x30+Tmp;
	 else ((PBYTE)&Result)[ctr] |= 0x37+Tmp;  // Uppercase
   Value  >>= 4;
  }
 return Result;
}
//---------------------------------------------
static int HexStrToByteArray(PBYTE Buffer, LPSTR SrcStr, UINT HexByteCnt)
{
 UINT len = 0;
 UINT ctr = 0;
 for(;(SrcStr[len]&&SrcStr[len+1])&&(!HexByteCnt||(ctr < HexByteCnt));len++)
  {
   if(SrcStr[len] == 0x20)continue;   // Skip spaces
   int ByteHi  = CharToHex(SrcStr[len]);
   int ByteLo  = CharToHex(SrcStr[len+1]);
   if((ByteHi  < 0)||(ByteLo < 0))return len;  // Not a HEX char
   Buffer[ctr] = (ByteHi << 4)|ByteLo;
   ctr++;
   len++;
  }
 return ctr;
}
//------------------------------------
static int ByteArrayToHexStr(PBYTE Buffer, LPSTR DstStr, UINT ByteCnt)
{
 UINT len = 0;
 for(UINT ctr=0;(ctr < ByteCnt);len+=2,ctr++)
  {
   WORD chr      = HexToChar(Buffer[ctr]);
   DstStr[len]   = ((PBYTE)&chr)[0];
   DstStr[len+1] = ((PBYTE)&chr)[1];
  }
 return len;
}
//---------------------------------------------------------------------------
static bool IsKeyCombinationPressed(UINT Combination)
{
 BYTE KeyCode;
 WORD KeyState;

 for(DWORD ctr=4;ctr > 0;ctr--)
  {
   if((KeyCode = (Combination & 0x000000FF)))
	{
     KeyState = GetAsyncKeyState(KeyCode);	// 1 - key is DOWN; 0 - key is UP
	 if(!(KeyState & 0x8000))return false;  // If one of keys in combination is up - no combination pressed
	}
   Combination = (Combination >> 8);
  }
 return true;
}
//---------------------------------------------------------------------------

CHexEditor(void)
{
 FillMemory(this,sizeof(CHexEditor),0x00);  // OK
 this->src = new CMemSrc;
 this->BytesInRow  = 16;
 this->ColDelSize  = 2;
 this->SelClr      = 0x00E0E0F0;
 this->TxtClr      = 0x00000000;
 this->BgrClr      = 0x00FFFFFF;
 this->CurClr      = 0x00505050;
 this->CursVisible = true;
 this->CharWidth = this->CharHeight = 1; // No 'Division by Zero'
}
//---------------------------------------------
~CHexEditor()
{
 this->ReleaseTarget();
 delete(this->src);
}
//---------------------------------------------
void Clear(void)
{
 this->SelFirst = this->SelLast = 0;
 this->SelType  = stNone;
 this->SelArr.Clear();
 this->DataResize(0);
}
//---------------------------------------------
UINT SelCount(void){return SelArr.Count();}
bool AddSel(UINT64 First, UINT64 Last, COLORREF Color)
{
 if(Last < First){UINT64 tmp = First; First = Last; Last = tmp;}  // Exchange
 //if((First >= this->DataLen)||(Last >= this->DataLen))return false;   // Not useful: Prevents from preparing a static selections table
 SelArr.Add(First,Last,Color);
 this->RebuildHls = true;
 return true;
}
//---------------------------------------------
void DelSel(UINT Index)
{
 SelArr.Del(Index);
 this->RebuildHls = true;
}
//---------------------------------------------
bool GetSel(UINT Index, UINT64* First, UINT64* Last, COLORREF* Color)
{
 CSelArray::SSelRec* sel = SelArr.Get(Index);
 if(!sel)return false;
 if(Color)*Color = sel->Color;
 if(First)*First = sel->SelFirst;
 if(Last)*Last   = sel->SelLast;
 return true;
}
//---------------------------------------------
bool SetSel(UINT Index, UINT64 First, UINT64 Last, COLORREF Color)
{
 CSelArray::SSelRec* sel = SelArr.Get(Index);
 if(!sel)return false;
 if((int)Color != ClrNoChange)sel->Color = Color;
 sel->SelFirst = First;
 sel->SelLast  = Last;
 this->RebuildHls = true;
 return true;
}
//---------------------------------------------
// bool SetFile(PWSTR Name);
bool SetBuffer(PBYTE Data, UINT Size)
{
 this->src->SetPosition(0);
 this->src->WriteBlock(Data,0,Size);
 this->DataPos = 0;
 this->DataResize(this->src->GetSize());
 return true;
}
//---------------------------------------------
bool GetSelection(UINT64* First, UINT64* Last)
{
 if(this->SelType == stNone)return false;
 if(First)*First = this->SelFirst;
 if(Last)*Last   = this->SelLast;
 return true;
}
//---------------------------------------------
void ClearSelection(void)
{
 this->SelFirst = this->SelLast = 0;
 this->SelType  = stNone;
 this->Redraw(true);
}
//---------------------------------------------
bool StartSelect(int X, int Y)   // Mouse
{
 if(!this->PixelToChar(X, Y, true))return false;
 UINT64 BPos = CharToBytePos(X, Y, &this->SelType);
 if(this->SelType == stNone)this->SelFirst = this->SelLast = 0;
   else this->SelFirst = this->SelLast = BPos;
 this->Redraw(true);
 return true;
}
//---------------------------------------------
bool StopSelect(int X, int Y)    // Mouse 
{
 ESelType SType;
 if(!this->PixelToChar(X, Y, true))return false;
 UINT64 BPos = CharToBytePos(X, Y, &SType);
 if(SType == stNone)return false;
 this->SelLast = BPos;
 this->Redraw(true);
 return true;
}
//---------------------------------------------
bool TrackSelect(int X, int Y)  // Mouse 
{
 ESelType SType;
 if(!this->PixelToChar(X, Y, true))return false;
 UINT64 BPos = CharToBytePos(X, Y, &SType);
 if(SType != this->SelType)return false;
 this->SelLast = BPos;
 this->Redraw(true);
 return true;
}
//---------------------------------------------
bool SetTarget(HWND hWnd, HFONT Font, RECT* Margins=NULL, COLORREF TxtColor=-1, COLORREF BgrColor=-1, COLORREF SelColor=-1)  // Call this after each resize
{
 this->ReleaseTarget();
 RECT WndRect;
 HDC  hTgtDC  = GetDC(hWnd);
 this->Font   = Font;
 this->TgtWnd = hWnd;
 if((int)TxtColor >= 0)this->TxtClr = TxtColor;
 if((int)BgrColor >= 0)this->BgrClr = BgrColor;
 if((int)SelColor >= 0)this->SelClr = SelColor;
 this->hMemDC = CreateCompatibleDC(hTgtDC);
 SelectObject(this->hMemDC,this->Font);
 GetWindowRect(hWnd, &WndRect);
 this->WWidth  = WndRect.right  - WndRect.left;
 this->WHeight = WndRect.bottom - WndRect.top;
 if(Margins)
  {
   this->WWidth  -= Margins->left + Margins->right;
   this->WHeight -= Margins->top  + Margins->bottom;
   this->WTop  = Margins->top;
   this->WLeft = Margins->left;
  }
   else this->WTop = this->WLeft = 0;

 this->WrkRc.top    = this->WTop;
 this->WrkRc.left   = this->WLeft;
 this->WrkRc.right  = this->WLeft + this->WWidth;
 this->WrkRc.bottom = this->WTop  + this->WHeight;

 this->hMemBmp = CreateCompatibleBitmap(hTgtDC,this->WWidth,this->WHeight);
 SelectObject(this->hMemDC,this->hMemBmp);
 SetBkMode(this->hMemDC,TRANSPARENT);   // OPAQUE
 SetTextColor(this->hMemDC,(COLORREF)this->TxtClr);
 SetBkColor(this->hMemDC,(COLORREF)this->BgrClr);    //0x00bbggrr 0x00F0CAA6 - SkyBlue
 ReleaseDC(hWnd,hTgtDC);

 BYTE TstStr[] = {"A"};      //"1-9 A-Z a-z %$ () , ` | _};
 DrawText(this->hMemDC,(LPSTR)&TstStr,sizeof(TstStr)-1,&WndRect,DT_CALCRECT|DT_NOCLIP|DT_WORDBREAK|DT_CENTER);
 this->CharHeight = WndRect.bottom - WndRect.top;
 this->CharWidth  = WndRect.right  - WndRect.left;
 this->MaxLines   = this->WHeight / this->CharHeight;
 this->MaxChars   = this->WWidth  / this->CharWidth;

 this->SelBmp.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
 this->SelBmp.bmiHeader.biWidth       = this->WWidth;
 this->SelBmp.bmiHeader.biHeight      = -this->WHeight;
 this->SelBmp.bmiHeader.biPlanes      = 1;
 this->SelBmp.bmiHeader.biBitCount    = 32;
 this->SelBmp.bmiHeader.biCompression = BI_RGB;
 this->SelPixs = (COLORREF*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,(this->WWidth*this->WHeight*sizeof(COLORREF))+64);
 this->SelHigh = (COLORREF*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,(this->WWidth*this->WHeight*sizeof(COLORREF))+64);
 this->RebuildHls = true;
 this->SetPosition(this->DataPos);
 return true;
}
//---------------------------------------------
void Redraw(bool Force=false)
{
 if(Force || this->RebuildHls || this->NeedRedraw)
  {
   this->DrawHighlights(this->RebuildHls); // Only if needed
   this->DrawData();       // Only if needed
   this->RebuildHls = false;
   this->NeedRedraw = false;
  }
 HDC hTgtDC = GetDC(this->TgtWnd);
 BitBlt(hTgtDC,this->WLeft,this->WTop,this->WWidth,this->WHeight,this->hMemDC,0,0,SRCCOPY);
 if(this->CursVisible)this->DrawCursor(hTgtDC);
 ReleaseDC(this->TgtWnd,hTgtDC);
 ValidateRect(this->TgtWnd,&this->WrkRc);  // Prevent it from repainting by someone else
}
//---------------------------------------------
bool SetPosition(UINT64 Pos, bool KeepLine=false)
{
 this->RebuildHls = true;
 this->NeedRedraw = true;
 if(KeepLine)Pos -= (Pos % this->BytesInRow);
 if(Pos != this->DataPos)
  {
   if(Pos >= this->DataLen)return false;
   if(src->SetPosition(Pos))this->DataPos = Pos;
	 else return false;
  }
 UINT64 MaxDisp = this->BytesInRow * this->MaxLines;
 UINT64 DatLeft = this->DataLen - this->DataPos;
 this->VisibleBytes = (DatLeft >= MaxDisp)?(MaxDisp):(DatLeft);
 UINT64 DPos = this->DataPos + this->DataOffs;
 this->AddrSize = (((DPos+this->VisibleBytes) > 0xFFFFFFFF)?(8):(4));
 this->AdrChrs  = (this->AddrSize * 2);   // 1 byte == 2 chars to display
 this->HexOff   = this->AdrChrs + this->ColDelSize;
 this->TxtOff   = this->HexOff  + (this->BytesInRow * 3) + this->ColDelSize - 1;
 this->Redraw(true);
 return true;
}
//---------------------------------------------
bool SetCursorPos(UINT64 Pos, bool SecHalf=false)
{
 if(Pos >= this->DataLen)return false;
 this->CursPos     = Pos;
 this->CursSecHalf = SecHalf;
 this->Redraw();
 return true;
}
//---------------------------------------------
bool SetCursorAt(int X, int Y)    // Pixel coords
{
 if(!PixelToChar(X, Y, true))return false;
 ESelType SType;
 bool SecHalf = false;
 UINT64 BPos = CharToBytePos(X, Y, &SType);
 if((SType == stNone)||(BPos == (UINT64)-1))return false;
 if(SType == stHex)
  {
   X -= this->HexOff;
   X  = X % 3;
   if(X > 1)return false;  // On delimiter
   SecHalf = (bool)X;
  }
 this->CurType = SType;
 return this->SetCursorPos(BPos, SecHalf);
}
//---------------------------------------------
void ShowCursor(bool Show)
{
 this->CursVisible = Show;
 this->Redraw();
}
//---------------------------------------------
bool StepLines(int Lines)
{
 int Offs = Lines * this->BytesInRow;
 if(!SetPosition(this->DataPos + Offs))return false;
 this->Redraw(true);
 return true;
}
//---------------------------------------------
// Without 'KeepLine' cursor will be allways first in line
//
bool MoveToCursor(ECursMove Type, bool KeepLine=true)
{
 UINT64 NewPos = this->CursPos;
 if(this->DataLen >= this->VisibleBytes)
  {
   switch(Type)
	{
	 case cmCenter:
	  {
	   UINT Offs = (this->MaxLines/2)*this->BytesInRow;
	   if(NewPos > Offs)NewPos -= Offs;
		 else NewPos = 0;
	  }
	  break;
	 case cmBottom:
	  {
	   UINT Offs = (this->MaxLines-1)*this->BytesInRow;
	   if(NewPos > Offs)NewPos -= Offs;
		 else NewPos = 0;
	  }
      break;
	}
  }
   else NewPos = 0;      // All data fits
 if(KeepLine)NewPos -= (NewPos % this->BytesInRow);
 return this->SetPosition(NewPos);
}
//---------------------------------------------
bool InputValue(WCHAR Val, bool MoveCursor=true)  // HalfByte or FullChar
{
 if(this->CurType == stNone)return false;
 if(this->CurType == stHex)
  {
   BYTE Value;
   long Char = CharToHex(Val);
   if(Char < 0)return false;
   if(!this->src->ReadBlock(&Value, this->CursPos, 1))return false;
   if(this->CursSecHalf)
	{
	 Value = (Value & 0xF0) | Char;
	 if(!this->src->WriteBlock(&Value, this->CursPos, 1))return false;
	 if(MoveCursor && ((this->CursPos+1) < this->DataLen))this->CursPos++;
       else MoveCursor = false;
	 this->CursSecHalf = false;
	}
	 else
	  {
	   Value = (Value & 0x0F) | (Char << 4);
	   if(!this->src->WriteBlock(&Value, this->CursPos, 1))return false;
       this->CursSecHalf = true;
	  }
  }
   else
	{
	 if(!this->src->WriteBlock((PBYTE)&Val, this->CursPos, 1))return false;
	 if(MoveCursor && ((this->CursPos+1) < this->DataLen))this->CursPos++;
	   else MoveCursor = false;
	}
 if(MoveCursor && !this->IsCursorVisible())return this->MoveToCursor(cmBottom,true);
 this->Redraw(true);
 return true;
}
//---------------------------------------------
bool IsCursorVisible(void)
{
 return ((this->CursPos >= this->DataPos)&&(this->CursPos < (this->DataPos + this->VisibleBytes)));
}
//---------------------------------------------
UINT64 GetCursorPos(bool* SecHalf)
{
 if(SecHalf)*SecHalf = this->CursSecHalf;
 return this->CursPos;
}
//---------------------------------------------
void StoreMousePos(int X, int Y)
{
 this->MPosX = X;
 this->MPosY = Y;
}
//---------------------------------------------
bool IsCanStartSelect(int& X, int& Y)
{
 int DeltaX = X - this->MPosX;
 int DeltaY = Y - this->MPosY;
 int Dist   = this->CharHeight / 2;
 if(DeltaX < 0)DeltaX = -DeltaX;
 if(DeltaY < 0)DeltaY = -DeltaY;
 if((DeltaX > Dist)||(DeltaY > Dist))
  {
   X = this->MPosX;
   Y = this->MPosY;
   return true;
  }
 return false;
}
//---------------------------------------------
UINT64 GetDataPos(void){return this->DataPos;}
UINT64 GetDataSize(void){return this->DataLen;}
PBYTE  GetDataPtr(void){return this->src->GetBuffer();}
//---------------------------------------------
UINT GetData(PBYTE Buffer, UINT64 Offset, UINT Length)
{
 if(Offset > this->DataLen)return 0;
 return this->src->ReadBlock(Buffer,Offset,Length);
}
//---------------------------------------------
UINT SetData(PBYTE Buffer, UINT64 Offset, UINT Length)
{
 //if(Offset > this->DataLen)return 0;
 UINT64 CFullLen = Offset + Length;
 if(CFullLen > this->DataLen)this->DataResize(CFullLen);
 UINT res = this->src->WriteBlock(Buffer,Offset,Length);
 if(res)this->Redraw(true);
 return res;
}
//---------------------------------------------
bool DataResize(UINT64 NewSize) // Resizes Buffer or File(Dangerous!)
{
 UINT64 OldSize = this->DataLen;
 CSelArray::SSelRec* sel = NULL;
 if(!this->src->SetSize(NewSize))return false;
 this->DataLen  = NewSize;
 if(NewSize < this->CursPos)this->CursPos = NewSize-1;
 if(TrimRange(this->SelFirst, this->SelLast, NewSize))this->SelType = stNone;
 for(int Index=0;sel = SelArr.Get(Index);Index++)if(TrimRange(sel->SelFirst, sel->SelLast, NewSize))sel->Color = ClrDisabled;
 if(NewSize > 0)
  {
   if(NewSize < this->DataPos)NewSize = NewSize - 1;
	 else NewSize = this->DataPos;
  }
 return this->SetPosition(NewSize, true);
}
//---------------------------------------------
bool CopyRangeToClpbrd(UINT64 Offset, UINT Length, ESelType Mode)   // TODO: Text/Full modes
{
 if(this->DataLen <= Offset)return false;
 PVOID TmpDataBuf  = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,Length+64);
 Length = this->GetData((PBYTE)TmpDataBuf,Offset,Length);
 if(!Length){HeapFree(GetProcessHeap(),0,TmpDataBuf); return false;}

 HANDLE hBufferTxt  = NULL;
 PVOID  DataPointer = NULL;
 switch(Mode)
  {
   case stHex:
	{
	 hBufferTxt  = GlobalAlloc(GHND,(Length*2)+1);
     DataPointer = GlobalLock(hBufferTxt);
	 int len = ByteArrayToHexStr((PBYTE)TmpDataBuf, (LPSTR)DataPointer, Length);
	 ((LPSTR)DataPointer)[len] = 0;
	}
	break;
   case stText:
	{
	 hBufferTxt  = GlobalAlloc(GHND,Length+1);
	 DataPointer = GlobalLock(hBufferTxt);
	 ((LPSTR)DataPointer)[Length] = 0;
	 for(UINT ctr=0;ctr < Length;ctr++)
	  {
	   BYTE val = ((PBYTE)TmpDataBuf)[ctr];
	   if(val < 0x20)val = 0x20;
	   ((LPSTR)DataPointer)[ctr] = val;
	  }
	}
	break;
   case stDisp:
	{
     // TODO
	}
	break;
   default: {HeapFree(GetProcessHeap(),0,TmpDataBuf); return false;}
  }
 HeapFree(GetProcessHeap(),0,TmpDataBuf);
 GlobalUnlock(hBufferTxt);

 if(!OpenClipboard(NULL)){GlobalFree(hBufferTxt); return false;}
 EmptyClipboard();  // ???
 SetClipboardData(CF_TEXT,hBufferTxt);
 CloseClipboard();
 return true;
}
//---------------------------------------------
bool CopyToClpbrd(int Mode=0, int HiIdx=0)
{
 if(Mode <= 0)Mode = this->SelType;
 if(Mode == stNone)return false;
 UINT64 Offset;
 UINT   Length;
 if(Mode & stCustom)
  {
   UINT64   Last;
   COLORREF Color;
   if(!GetSel(HiIdx, &Offset, &Last, &Color) || (Color == ClrDisabled))return false;
   Length = Last - Offset + 1;
   Mode  &= ~stCustom;
  }
   else
	{
	 Offset = this->SelFirst;
	 Length = this->SelLast - this->SelFirst + 1;
	}
 return this->CopyRangeToClpbrd(Offset, Length, (ESelType)Mode);
}

};
//---------------------------------------------------------------------------
#endif
