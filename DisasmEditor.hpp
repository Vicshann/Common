
#pragma once

#ifndef DisEditorH
#define DisEditorH
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
class CDisAsmEditor
{
public:
static const COLORREF ClrDisabled = 0x80000000;
static const COLORREF ClrNoChange = 0xFF000000;

enum ESelType {stNone,stDisp,stText,stHex,stCustom}; // stCustom must be 4

class CByteSource
{
public:
 virtual ~CByteSource(){};
 virtual PBYTE  GetData(void) = 0;
 virtual UINT64 GetSize(void) = 0;
 virtual bool   SetSize(UINT64 NewSize) = 0;
 virtual UINT   ReadBlock(PBYTE Buffer, UINT64 Offset, UINT Length) = 0;
 virtual UINT   WriteBlock(PBYTE Buffer, UINT64 Offset, UINT Length) = 0;
};
//-------------------------------------
class CMemSrc: public CByteSource
{
protected:
 PBYTE  Data;
 UINT64 Size;
public:
 virtual ~CMemSrc(){HeapFree(GetProcessHeap(),0,this->Data);}
 virtual PBYTE  GetData(void){return this->Data;}
 virtual UINT64 GetSize(void){return this->Size;}
 virtual bool   SetSize(UINT64 NewSize)
  {
   if(NewSize == this->Size)return true;
   if(!NewSize){HeapFree(GetProcessHeap(),0,this->Data);this->Data=NULL;this->Size=0;return true;}
   if(!this->Data)this->Data = (PBYTE)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,NewSize+64);
	 else this->Data = (PBYTE)HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,this->Data,NewSize+64);
   this->Size = (this->Data)?(NewSize):(0);
   return true;
  }
 virtual UINT   ReadBlock(PBYTE Buffer, UINT64 Offset, UINT Length)
  {
   if(Offset > this->Size)return 0;
   if((Offset+Length) > this->Size)Length = this->Size - Offset;  // Rest of data
   memcpy(Buffer,&this->Data[Offset],Length);    // CopyMemory
   return Length;
  }
 virtual UINT   WriteBlock(PBYTE Buffer, UINT64 Offset, UINT Length)
  {
   if((Offset+Length) > this->Size)this->SetSize(Offset+Length);
   memcpy(&this->Data[Offset],Buffer,Length);    // CopyMemory
   return Length;
  }
};
//-------------------------------------
class CPtrSrc: public virtual CMemSrc
{
public:
 virtual ~CPtrSrc(){}
 virtual bool   SetSize(UINT64 NewSize){this->Size = NewSize; return true;}
 void SetData(PBYTE Dat){this->Data = Dat;}
};
//-------------------------------------

private:
struct SSelRec
{
 WORD   OffsFirst;  // Chars
 WORD   OffsLast;
 UINT64 SelFirst;   // Addr, meaning a line
 UINT64 SelLast;
};

struct SAddrRec      // Each for a line
{
 UINT64 Addr;
 WORD   PixOffs;
 WORD   ChrOffs;
 BYTE   Size;
 BYTE   Length;
 BYTE   Text[32];
};

enum ECursMove {cmTop,cmCenter,cmBottom};
 CByteSource* DataSrc;
 SAddrRec* AddrLines;
 UINT64 SelFirst;
 UINT64 SelLast;
 UINT64 DataPos;   // When set, must be valid for DataSrc::Size
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
// int BytesInRow;   // Dynamic
// int VisibleBytes;    // ????????????????????????????
// bool RebuildHls;
 bool NeedRedraw;
// bool CursSecHalf;
// bool CursVisible;
 BITMAPINFO SelBmp;
 COLORREF*  SelPixs;
// COLORREF*  SelHigh;
 SSelRec    CurSel;

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
void UpdateDisasm(void)
{
 int   TxTop = 0;
 UINT64 DLen = this->DataSrc->GetSize();
 UINT64 DPtr = this->DataPos;
 for(int ctr=0;ctr < this->MaxLines;ctr++,TxTop+=this->CharHeight)
  {
   UINT64 DPos = DPtr + this->DataOffs;  
   this->AddrLines[ctr].Addr    = DPos;
   this->AddrLines[ctr].PixOffs = TxTop;
   this->AddrLines[ctr].ChrOffs = TxTop / this->CharHeight;
   UINT TxtLen = 0;
   int len = this->cbkDisasmAddr(this->UsrCtx, DPos, (char*)&this->AddrLines[ctr].Text, &TxtLen);
   if(len < 0)break;  
   this->AddrLines[ctr].Size   = len;
   this->AddrLines[ctr].Length = TxtLen;
   DPtr += len;
  }
}
//---------------------------------------------
void DrawData(void)    // TODO: Draw to memory and blend with selection only when changed
{
 BYTE Line[1056]; // Addr=8(16)+2+2, Data*4 = Max 256 BytesInRow
 BYTE Data[64];
// BYTE Text[256];
 int   TxTop = 0;
 int   ASize = this->AddrSize - 1;
// int   BLeft = this->VisibleBytes;
 UINT64 DLen = this->DataSrc->GetSize();
 UINT64 DPtr = this->DataPos;
 if(!DLen)return;
 memset(&Line, 0x20, sizeof(Line));  
 for(int ctr=0;ctr < this->MaxLines;ctr++,TxTop+=this->CharHeight)
  {
   int    LPos = 0;
   UINT64 DPos = DPtr + this->DataOffs;  
//   this->AddrLines[ctr].Addr    = DPos;
//   this->AddrLines[ctr].PixOffs = TxTop;
//   this->AddrLines[ctr].ChrOffs = TxTop / this->CharHeight;
   for(int actr=0;actr <= ASize;actr++,LPos+=2)     // Write Addr field
	{                                              
	 WORD chr = HexToCharUpC(((PBYTE)&DPos)[actr]);   // Endianess!!!!!!!!!!!!!
	 int CPos = (ASize-actr)*2;
	 Line[CPos+0] = ((PBYTE)&chr)[0];
	 Line[CPos+1] = ((PBYTE)&chr)[1];
	}
//   UINT TxtLen = 0;
//   int len = this->cbkDisasmAddr(this->UsrCtx, DPos, (char*)&Text, &TxtLen);
//   if(len < 0)break;
   int len = this->AddrLines[ctr].Size;
   this->DataSrc->ReadBlock((PBYTE)&Data, DPos, len);
   DPtr += len;
   LPos += this->ColDelSize;
   for(int dctr=0;dctr < len;dctr++,LPos+=3)       // Write data field
	{
	 WORD chr = HexToCharUpC(Data[dctr]);
	 Line[LPos+0] = ((PBYTE)&chr)[0];
	 Line[LPos+1] = ((PBYTE)&chr)[1];
	}                                                    
   for(;LPos < this->TxtOff;LPos++)Line[LPos] = 0x20;     // TODO: Data bytes TAB
   memcpy(&Line[LPos], &this->AddrLines[ctr].Text, this->AddrLines[ctr].Length);  ///////////// lstrcatA((char*)&Line[LPos], (char*)&Text);   // Write disasm field
   LPos += this->AddrLines[ctr].Length;               
   BOOL res = TextOutA(this->hMemDC,0,TxTop,(LPSTR)&Line,LPos);   // No UNICODE for now
   res++;
  }
}
//---------------------------------------------
SAddrRec* FindRecForAddr(UINT64 Addr, int* Index=nullptr)
{
 for(int ctr=0;ctr < this->MaxLines;ctr++)
  {
   if(this->AddrLines[ctr].Addr == Addr)
    {
     if(Index)*Index = ctr;
     return &this->AddrLines[ctr];
    }
  }
 return nullptr;
}
//---------------------------------------------
// When starting selection on address area - select by lines. When on bytes or instruction area - select as text(including multiline and comments)
// Cursor marks only address, selection of the line follows when stepping
// Cursor coordinates updated by DrawData
// Selections are done without any caret display
// Cursor is current PC pointer
//
void DrawCursor(HDC TgtDc)    // PC and sel cursor
{
 RECT RAddr;
 if(SAddrRec* Addr = this->FindRecForAddr(this->CursPos))
  {
   RAddr.top    = this->WTop + Addr->PixOffs;
   RAddr.left   = 0;    // No margin?????
   RAddr.right  = (this->AddrSize*2) * this->CharWidth;
   RAddr.bottom = RAddr.top + this->CharHeight;
   InvertRect(TgtDc, &RAddr);  
  }
 if(SAddrRec* Addr = this->FindRecForAddr(this->CurSel.SelLast))  // SEL cursor always marks a char at end of selection
  {
   if((this->CursPos == this->CurSel.SelLast) && (this->CurSel.SelLast == this->CurSel.SelFirst) && !this->CurSel.OffsLast)return;  // Do not draw SEL cursor at PC cursor automatically
   RAddr.top    = this->WTop + Addr->PixOffs;
   RAddr.left   = this->CurSel.OffsLast * this->CharWidth;    // No margin?????
   RAddr.right  = RAddr.left + this->CharWidth;
   RAddr.bottom = RAddr.top  + this->CharHeight;
   InvertRect(TgtDc, &RAddr);  
  }
}
//---------------------------------------------
void HighlightChar(int X, int Y, COLORREF Color, COLORREF* Pixels, int PixStep=1) // Note: No bounds check here!  //By pixels - slow! :: TODO: FillRect(xor)? 
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
void HighlightRange(SSelRec* Sel, COLORREF Color, COLORREF* Pixels)  // Highlites only visible part of Range
{
 if(!this->AddrLines)return;
 if(this->AddrLines[0].Addr > Sel->SelLast)return;
 if(this->AddrLines[this->MaxLines-1].Addr < Sel->SelFirst)return;
 if((Sel->SelFirst == Sel->SelLast) && (Sel->OffsFirst == Sel->OffsLast) && (Sel->OffsFirst || (Sel->SelFirst != this->CursPos)))return;  // No visible SEL range, only SEL cursor
 for(int ctr=0;ctr < this->MaxLines;ctr++)
  {
   SAddrRec* Addr = &this->AddrLines[ctr];
   int XOffs = 0;
   int XChrs = this->MaxChars;
   if(Addr->Addr < Sel->SelFirst)continue;
   if(Addr->Addr > Sel->SelLast)break;
   if(Addr->Addr == Sel->SelFirst)
    {
     XOffs  = Sel->OffsFirst;
     XChrs -= XOffs;
    }
   else if(Addr->Addr == Sel->SelLast)XChrs -= Sel->OffsLast;
   for(int xpos=XOffs;xpos < XChrs;xpos++)this->HighlightChar(xpos, Addr->ChrOffs, Color, Pixels);
  }
}
//---------------------------------------------
void DrawHighlights(void) //bool HiRedraw)
{
/* if(HiRedraw)
  {
   for(int ctr=0,total=(this->WWidth*this->WHeight);ctr < total;ctr++)this->SelHigh[ctr] = this->BgrClr;
   this->HighlightRange(stCustom, sel->SelFirst, sel->SelLast, sel->Color, this->SelHigh);
  }*/
 for(int ctr=0,total=(this->WWidth*this->WHeight);ctr < total;ctr++)this->SelPixs[ctr] = this->BgrClr;   // Clear selection
 this->HighlightRange(&this->CurSel, this->SelClr, this->SelPixs);   // Redraw selection
/* for(int ctr=0,total=(this->WWidth*this->WHeight);ctr < total;ctr++)
  {
   if(this->SelHigh[ctr] == this->BgrClr)continue;  // Nothing to copy
   if(this->SelPixs[ctr] == this->BgrClr)this->SelPixs[ctr] = this->SelHigh[ctr];  // Direct copy
	 else this->SelPixs[ctr] = (this->SelPixs[ctr] ^ this->SelHigh[ctr]) & 0x00FFFFFF;
  } */
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
/*UINT64 CharToBytePos(int X, int Y, ESelType* Type=NULL)
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
 if(res >= this->DataSrc->GetSize()){if(Type)*Type = stNone; return -2;}
 return res;
} */
//---------------------------------------------
/*bool BytePosToChars(UINT64 Pos, int* HexX, int* TxtX, int* PosY)
{
 if((Pos < this->DataPos)||(Pos >= (this->DataPos+this->VisibleBytes)))return false; // Not in visible range
 int Offset = Pos - this->DataPos;
 int Line   = Offset / this->BytesInRow;
 int LOff   = Offset % this->BytesInRow;
 if(PosY)*PosY = Line;
 if(TxtX)*TxtX = this->TxtOff + LOff;
 if(HexX)*HexX = (this->HexOff)+(LOff*3);   // First char of the pair
 return true;
} */
//---------------------------------------------
void ReleaseTarget(void)    // TODO: Reallocate buffers only when size changes
{
 if(this->hMemBmp)DeleteObject(this->hMemBmp);
 if(this->hMemDC )DeleteDC(this->hMemDC);
 if(this->SelPixs){HeapFree(GetProcessHeap(),0,this->SelPixs);this->SelPixs=NULL;}
// if(this->SelHigh){HeapFree(GetProcessHeap(),0,this->SelHigh);this->SelHigh=NULL;}
 if(this->AddrLines){HeapFree(GetProcessHeap(),0,this->AddrLines);this->AddrLines=NULL;}
}
//---------------------------------------------


public:
void* UsrCtx;
int (_cdecl *cbkDisasmAddr)(void* Ctx, UINT64 Addr, char* Str, UINT* Len);

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
CDisAsmEditor(void)
{
 memset(this,0,sizeof(CDisAsmEditor));  //    FillMemory(this,sizeof(CHexEditor),0x00);  // OK
 this->DataSrc     = nullptr;
// this->BytesInRow  = 16;
 this->ColDelSize  = 2;
 this->SelClr      = 0x00E0E0F0;
 this->TxtClr      = 0x00000000;
 this->BgrClr      = 0x00FFFFFF;
 this->CurClr      = 0x00505050;
// this->CursVisible = true;
 this->CharWidth = this->CharHeight = 1; // No 'Division by Zero'
}
//---------------------------------------------
~CDisAsmEditor()
{
 this->ReleaseTarget();
}
//---------------------------------------------
void Reset(void)
{
// this->SelFirst = this->SelLast = 0;
 this->SelType  = stNone;
// this->SelArr.Clear();   
 this->SetPosition(0);   // Is cursor reset?
}
//---------------------------------------------
bool GetSel(UINT Index, UINT64* First, UINT64* Last, COLORREF* Color)
{
/* CSelArray::SSelRec* sel = SelArr.Get(Index);
 if(!sel)return false;
 if(Color)*Color = sel->Color;
 if(First)*First = sel->SelFirst;
 if(Last)*Last   = sel->SelLast; */
 return true;
}
//---------------------------------------------
bool SetSel(UINT Index, UINT64 First, UINT64 Last, COLORREF Color)
{
/* CSelArray::SSelRec* sel = SelArr.Get(Index);
 if(!sel)return false;
 if((int)Color != ClrNoChange)sel->Color = Color;
 sel->SelFirst = First;
 sel->SelLast  = Last;
 this->RebuildHls = true;  */
 return true;
}
//---------------------------------------------
void SetDataSource(CByteSource* Src)
{
 this->DataPos = 0;
 this->DataSrc = Src;
 this->SetPosition(0);
}
//---------------------------------------------
UINT64 GetSelCurAddr(void)
{
 return this->CurSel.SelLast;
}
//---------------------------------------------
bool GetSelection(UINT64* First, UINT64* Last)
{
/* if(this->SelType == stNone)return false;
 if(First)*First = this->SelFirst;
 if(Last)*Last   = this->SelLast; */
 return true;
}
//---------------------------------------------
void ClearSelection(void)
{
/* this->SelFirst = this->SelLast = 0;
 this->SelType  = stNone;
 this->Redraw(true);   */
}
//---------------------------------------------
bool StartSelect(int X, int Y)   // Mouse
{
 if(!this->PixelToChar(X, Y, true))return false;
 if(Y >= this->MaxLines)return false;
 SAddrRec* Addr = &this->AddrLines[Y];
 this->CurSel.SelFirst  = this->CurSel.SelLast  = Addr->Addr;
 this->CurSel.OffsFirst = this->CurSel.OffsLast = X;
 this->Redraw(true);   
 return true;
}
//---------------------------------------------
bool StopSelect(int X, int Y)    // Mouse 
{
/* ESelType SType;
 if(!this->PixelToChar(X, Y, true))return false;
 UINT64 BPos = CharToBytePos(X, Y, &SType);
 if(SType == stNone)return false;
 this->SelLast = BPos;
 this->Redraw(true);  */
 return true;
}
//---------------------------------------------
bool TrackSelect(int X, int Y)  // Mouse 
{
/* ESelType SType;
 if(!this->PixelToChar(X, Y, true))return false;
 UINT64 BPos = CharToBytePos(X, Y, &SType);
 if(SType != this->SelType)return false;
 this->SelLast = BPos;
 this->Redraw(true); */
 return true;
}
//---------------------------------------------
void SelectAddr(UINT64 Addr)      // Simple selection to sync with PC
{
 this->CurSel.OffsFirst = this->CurSel.OffsLast = 0;
 this->CurSel.SelFirst  = this->CurSel.SelLast  = Addr;
 this->NeedRedraw = true;    // TODO: Separate DC for highlits
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
 GetClientRect(hWnd, &WndRect);
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
 DrawTextA(this->hMemDC,(LPSTR)&TstStr,sizeof(TstStr)-1,&WndRect,DT_CALCRECT|DT_NOCLIP|DT_WORDBREAK|DT_CENTER);   // Get font rect size   // May suddenly fail
 this->CharHeight = WndRect.bottom - WndRect.top;
 this->CharWidth  = WndRect.right  - WndRect.left;
 if(!this->CharHeight)this->CharHeight = 1;       // Corrupted on minimize of window
 if(!this->CharWidth)this->CharWidth = 1;
 this->MaxLines   = this->WHeight / this->CharHeight;
 this->MaxChars   = this->WWidth  / this->CharWidth;

 this->SelBmp.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
 this->SelBmp.bmiHeader.biWidth       = this->WWidth;
 this->SelBmp.bmiHeader.biHeight      = -this->WHeight;
 this->SelBmp.bmiHeader.biPlanes      = 1;
 this->SelBmp.bmiHeader.biBitCount    = 32;
 this->SelBmp.bmiHeader.biCompression = BI_RGB;
 this->SelPixs = (COLORREF*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,(this->WWidth*this->WHeight*sizeof(COLORREF))+64);
// this->SelHigh = (COLORREF*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,(this->WWidth*this->WHeight*sizeof(COLORREF))+64);
 this->AddrLines  = (SAddrRec*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,(this->MaxLines+6)*sizeof(SAddrRec));
// this->RebuildHls = true;
 this->SetPosition(this->DataPos);     // Recalc for a new window and Redraw at current position
 return true;
}
//---------------------------------------------
void Redraw(bool Force=false)
{
 if(Force || /*this->RebuildHls ||*/ this->NeedRedraw)
  {
   this->UpdateDisasm();
   this->DrawHighlights();//this->RebuildHls); // Only if needed
   this->DrawData();       // Only if needed
//   this->RebuildHls = false;
   this->NeedRedraw = false;
  }
 HDC hTgtDC = GetDC(this->TgtWnd);
 BitBlt(hTgtDC,this->WLeft,this->WTop,this->WWidth,this->WHeight,this->hMemDC,0,0,SRCCOPY);
 this->DrawCursor(hTgtDC);
 ReleaseDC(this->TgtWnd,hTgtDC);
 ValidateRect(this->TgtWnd,&this->WrkRc);  // Prevent it from repainting by someone else
}
//---------------------------------------------
bool SetPosition(UINT64 Pos)
{
 if(!this->TgtWnd)return false;
// this->RebuildHls = true;
 this->NeedRedraw = true;
 if(Pos != this->DataPos)
  {
   if(Pos >= this->DataSrc->GetSize())return false;
   this->DataPos = Pos;
  }
// UINT64 MaxDisp = this->BytesInRow * this->MaxLines;
 UINT64 DatLeft = this->DataSrc->GetSize() - this->DataPos;
// this->VisibleBytes = (DatLeft >= MaxDisp)?(MaxDisp):(DatLeft);
 UINT64 DPos = this->DataPos + this->DataOffs;
 this->AddrSize = 4;//(((DPos+this->VisibleBytes) > 0xFFFFFFFF)?(8):(4));
 this->AdrChrs  = (this->AddrSize * 2);   // 1 byte == 2 chars to display
 this->HexOff   = this->AdrChrs + this->ColDelSize;
 this->TxtOff   = this->HexOff  + (30) + this->ColDelSize - 1;     // ????????????
 this->Redraw(true);  
 return true;
}
//---------------------------------------------
bool SetCursorPos(UINT64 Pos)
{
 if(Pos >= this->DataSrc->GetSize())return false;
 this->CursPos     = Pos;
 this->Redraw();
 return true;
}
//---------------------------------------------
bool SetCursorAt(int X, int Y)    // Pixel coords
{
/* if(!PixelToChar(X, Y, true))return false;
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
 this->CurType = SType;  */
 return 0;//this->SetCursorPos(BPos, SecHalf);
}
//---------------------------------------------
/*void ShowCursor(bool Show)
{
 this->CursVisible = Show;
 this->Redraw();
} */
//---------------------------------------------
bool StepLines(int Lines)
{
/* int Offs = Lines * this->BytesInRow;
 if(!this->SetPosition(this->DataPos + Offs))return false; */
 this->Redraw(true);
 return true;
}
//---------------------------------------------
// Without 'KeepLine' cursor will be allways first in line
//
bool MoveToCursor(ECursMove Type, bool KeepLine=true)
{
 UINT64 NewPos = this->CursPos;
/* if(this->DataSrc->GetSize() >= this->VisibleBytes)
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
 if(KeepLine)NewPos -= (NewPos % this->BytesInRow);   */
 return this->SetPosition(NewPos);
}
//---------------------------------------------
/*bool InputValue(WCHAR Val, bool MoveCursor=true)  // HalfByte or FullChar
{
 if(this->CurType == stNone)return false;
 if(this->CurType == stHex)
  {
   BYTE Value;
   long Char = CharToHex(Val);
   if(Char < 0)return false;
   if(!this->DataSrc->ReadBlock(&Value, this->CursPos, 1))return false;
   if(this->CursSecHalf)
	{
	 Value = (Value & 0xF0) | Char;
	 if(!this->DataSrc->WriteBlock(&Value, this->CursPos, 1))return false;
	 if(MoveCursor && ((this->CursPos+1) < this->DataSrc->GetSize()))this->CursPos++;
       else MoveCursor = false;
	 this->CursSecHalf = false;
	}
	 else
	  {
	   Value = (Value & 0x0F) | (Char << 4);
	   if(!this->DataSrc->WriteBlock(&Value, this->CursPos, 1))return false;
       this->CursSecHalf = true;
	  }
  }
   else
	{
	 if(!this->DataSrc->WriteBlock((PBYTE)&Val, this->CursPos, 1))return false;
	 if(MoveCursor && ((this->CursPos+1) < this->DataSrc->GetSize()))this->CursPos++;
	   else MoveCursor = false;
	}
 if(MoveCursor && !this->IsCursorVisible())return this->MoveToCursor(cmBottom,true);
 this->Redraw(true);
 return true;
} */
//---------------------------------------------
bool IsCursorVisible(void)
{
 return (bool)this->FindRecForAddr(this->CursPos);
// return true;//((this->CursPos >= this->DataPos)&&(this->CursPos < (this->DataPos + this->VisibleBytes)));   // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
}
//---------------------------------------------
UINT64 GetCursorPos(void)
{
// if(SecHalf)*SecHalf = this->CursSecHalf;
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
bool CopyRangeToClpbrd(UINT64 Offset, UINT Length, ESelType Mode)   // TODO: Text/Full modes
{
 if(this->DataSrc->GetSize() <= Offset)return false;
 PVOID TmpDataBuf  = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,Length+64);   // TODO: Use data source only
 Length = this->DataSrc->ReadBlock((PBYTE)TmpDataBuf,Offset,Length);  //--------------------  this->GetData((PBYTE)TmpDataBuf,Offset,Length);
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
