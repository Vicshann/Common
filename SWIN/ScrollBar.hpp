//==========================================================================================================================
//                                               SCROLL BAR CLASS
//==========================================================================================================================
/*
SBS_BOTTOMALIGN:  Aligns the bottom edge of the scroll bar with the bottom edge of the rectangle defined by the x, y, nWidth, and nHeight parameters of CreateWindowEx function. The scroll bar has the default height for system scroll bars. Use this style with the SBS_HORZ style.
SBS_HORZ:  Designates a horizontal scroll bar. If neither the SBS_BOTTOMALIGN nor SBS_TOPALIGN style is specified, the scroll bar has the height, width, and position specified by the x, y, nWidth, and nHeight parameters of CreateWindowEx.
SBS_LEFTALIGN:  Aligns the left edge of the scroll bar with the left edge of the rectangle defined by the x, y, nWidth, and nHeight parameters of CreateWindowEx. The scroll bar has the default width for system scroll bars. Use this style with the SBS_VERT style.
SBS_RIGHTALIGN:  Aligns the right edge of the scroll bar with the right edge of the rectangle defined by the x, y, nWidth, and nHeight parameters of CreateWindowEx. The scroll bar has the default width for system scroll bars. Use this style with the SBS_VERT style.
SBS_SIZEBOX:  The scroll bar is a size box. If you specify neither the SBS_SIZEBOXBOTTOMRIGHTALIGN nor the SBS_SIZEBOXTOPLEFTALIGN style, the size box has the height, width, and position specified by the x, y, nWidth, and nHeight parameters of CreateWindowEx. 
SBS_SIZEBOXBOTTOMRIGHTALIGN:  Aligns the lower right corner of the size box with the lower right corner of the rectangle specified by the x, y, nWidth, and nHeight parameters of CreateWindowEx. The size box has the default size for system size boxes. Use this style with the SBS_SIZEBOX or SBS_SIZEGRIP styles.
SBS_SIZEBOXTOPLEFTALIGN:  Aligns the upper left corner of the size box with the upper left corner of the rectangle specified by the x, y, nWidth, and nHeight parameters of CreateWindowEx. The size box has the default size for system size boxes. Use this style with the SBS_SIZEBOX or SBS_SIZEGRIP styles.
SBS_SIZEGRIP:  Same as SBS_SIZEBOX, but with a raised edge.  // The scroll bar is a size box with a raised edge. 
SBS_TOPALIGN:  Aligns the top edge of the scroll bar with the top edge of the rectangle defined by the x, y, nWidth, and nHeight parameters of CreateWindowEx. The scroll bar has the default height for system scroll bars. Use this style with the SBS_HORZ style.
SBS_VERT:  Designates a vertical scroll bar. If you specify neither the SBS_RIGHTALIGN nor the SBS_LEFTALIGN style, the scroll bar has the height, width, and position specified by the x, y, nWidth, and nHeight parameters of CreateWindowEx.
*/
class CSWScrBar: public CCldBase
{
 bool AUpd = true;
public:
 enum EScrType {stLine=1,stPage=2,stEdge=4,stTrack=8,stTrackDone=16};  // Can be masked

 void (_fastcall CWndBase::*OnScroll)(CWndBase* Sender, int Type, int PosMod) = nullptr;

CSWScrBar(void){ }
//------------------------------------------------------------------------------------------------------------
int Create(SWDim& Wdim, LPCWSTR Text=L"", DWORD Style=0, DWORD ExStyle=0, bool AutoUpd=true)
{
 this->AUpd = AutoUpd;
 if(!this->SuperClassSysCtrl(L"SCROLLBAR", L"CSWScrBar"))return -1;
 return (this->CreateWnd((LPCWSTR)this->WndClass, Text, Style|WS_CHILD, 0, Wdim.PosX, Wdim.PosY, Wdim.Width, Wdim.Height, NULL))?(0):(-2);
}
//------------------------------------------------------------------------------------------------------------
virtual bool WindowProc(HWND& hWnd, UINT& Msg, WPARAM& wParam, LPARAM& lParam, LRESULT& lResult)
{  
 CCldBase::WindowProc(hWnd, Msg, wParam, lParam, lResult);   
// LRESULT Result = 0;
// bool    NoOrig = false;
 switch(Msg)
  {  
   case WM_VSCROLL:
   case WM_HSCROLL:
     {
      SCROLLINFO sci;
      sci.cbSize = sizeof(SCROLLINFO);
      sci.fMask  = SIF_ALL;
      GetScrollInfo(this->hWindow, SB_CTL, &sci);
      switch(wParam & 0xFFFF)
       {  
//     case SB_ENDSCROLL:    // ?????????

        case SB_THUMBPOSITION:
         if(this->AUpd)this->SetPos(sci.nTrackPos);
         if(this->OnScroll)(this->GetOwnerWnd()->*OnScroll)(this, stTrack|stTrackDone, sci.nTrackPos);
         break;
        case SB_THUMBTRACK:
         if(this->AUpd)this->SetPos(sci.nTrackPos);
         if(this->OnScroll)(this->GetOwnerWnd()->*OnScroll)(this, stTrack, sci.nTrackPos);
         break;
        case SB_TOP:
//        case SB_LEFT:
         if(this->AUpd)this->SetPos(sci.nMin);
         if(this->OnScroll)(this->GetOwnerWnd()->*OnScroll)(this, stEdge, sci.nMin);
         break;
        case SB_RIGHT:
//        case SB_BOTTOM:
         if(this->AUpd)this->SetPos(sci.nMax);
         if(this->OnScroll)(this->GetOwnerWnd()->*OnScroll)(this, stEdge, sci.nMax);
         break;
        case SB_LINEUP:
//        case SB_LINELEFT:
         if(this->AUpd)this->SetPos(sci.nPos-1);
         if(this->OnScroll)(this->GetOwnerWnd()->*OnScroll)(this, stLine, -1);
         break; 
        case SB_LINEDOWN:
//        case SB_LINERIGHT:
         if(this->AUpd)this->SetPos(sci.nPos+1);
         if(this->OnScroll)(this->GetOwnerWnd()->*OnScroll)(this, stLine, 1);
         break;
        case SB_PAGEUP:
//        case SB_PAGELEFT:
         if(this->AUpd)this->SetPos(sci.nPos-sci.nPage);
         if(this->OnScroll)(this->GetOwnerWnd()->*OnScroll)(this, stPage, -sci.nPage);
         break;
        case SB_PAGEDOWN:
//        case SB_PAGERIGHT:
         if(this->AUpd)this->SetPos(sci.nPos+sci.nPage);
         if(this->OnScroll)(this->GetOwnerWnd()->*OnScroll)(this, stPage, sci.nPage);
         break;
       }
     }
    break;
  } 
// if(NoOrig)return Result;        
 return true;
}
//------------------------------------------------------------------------------------------------------------
int SetRange(int iMin, int iMax, UINT uPage)
{
 SCROLLINFO sci;
 sci.cbSize = sizeof(SCROLLINFO);
 sci.fMask  = SIF_RANGE;
 sci.nPage  = uPage;
 sci.nMin   = iMin;
 sci.nMax   = iMax;
 if(uPage)sci.fMask |= SIF_PAGE;
 return SetScrollInfo(this->hWindow, SB_CTL, &sci, TRUE); 
}
//------------------------------------------------------------------------------------------------------------
int GetRange(int* pMin, int* pMax, UINT* pPage)
{
 SCROLLINFO sci;
 sci.cbSize = sizeof(SCROLLINFO);
 sci.fMask  = SIF_POS;
 if(pPage)sci.fMask |= SIF_PAGE;
 if(!GetScrollInfo(this->hWindow, SB_CTL, &sci))return -1;
 if(pPage)*pPage = sci.nPage;
 if(pMin)*pMin = sci.nMin;
 if(pMax)*pMax = sci.nMax;
 return 0;
}
//------------------------------------------------------------------------------------------------------------
int SetPos(int iPos)     // Is setting beyond limits sets to min/max or does nothing?
{
 SCROLLINFO sci;
 sci.cbSize = sizeof(SCROLLINFO);
 sci.fMask  = SIF_POS;
 sci.nPos   = iPos;
 return SetScrollInfo(this->hWindow, SB_CTL, &sci, TRUE); 
}
//------------------------------------------------------------------------------------------------------------
int GetPos(void)
{
 SCROLLINFO sci;
 sci.cbSize = sizeof(SCROLLINFO);
 sci.fMask  = SIF_POS;
 if(!GetScrollInfo(this->hWindow, SB_CTL, &sci))return -1;
 return sci.nPos;
}
//------------------------------------------------------------------------------------------------------------

};