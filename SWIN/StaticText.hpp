//==========================================================================================================================
//                                                STATIC TEXT CLASS
//==========================================================================================================================
/*
 Styles:
   ES_PASSWORD
   ES_READONLY
*/
class CSWStatic: public CCldBase
{            
public:
CSWStatic(void){ }
//------------------------------------------------------------------------------------------------------------
int Create(SWDim& Wdim, LPCWSTR Text=L"", DWORD Style=0, DWORD ExStyle=0)
{        
 if(!this->SuperClassSysCtrl(L"STATIC", L"CSWStatic"))return -1;
 return (this->CreateWnd((LPCWSTR)this->WndClass, Text, Style|WS_CHILD, ExStyle, Wdim.PosX, Wdim.PosY, Wdim.Width, Wdim.Height, NULL))?(0):(-2);    // WS_EX_CLIENTEDGE  WS_EX_WINDOWEDGE
}
//------------------------------------------------------------------------------------------------------------
/*virtual bool WindowProc(HWND& hWnd, UINT& Msg, WPARAM& wParam, LPARAM& lParam, LRESULT& lResult)
{  
 if(WM_CTLCOLORSTATIC == Msg)
  {
   Sleep(1);
  }
 CCldBase::WindowProc(hWnd, Msg, wParam, lParam, lResult);   
// LRESULT Result = 0;
// bool    NoOrig = false;
 switch(Msg)
  {
 //  case WM_CHAR:
//    if(this->OnMouseBtnUp)(this->GetOwnerWnd()->*OnMouseBtnUp)(this, Msg, wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
//    Sleep(1);
 //   break;
  } 
// if(NoOrig)return Result;        
 return true;
} */
//------------------------------------------------------------------------------------------------------------

};
