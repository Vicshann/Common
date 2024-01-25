//==========================================================================================================================
//                                          DATE TIME PICKER CLASS
//==========================================================================================================================
class CSWDTPicker: public CCldBase
{
public:
// void (_fastcall CWndBase::*OnMouseBtnUp)(CWndBase* Sender, WORD WMsg, WORD KeyEx, int x, int y) = nullptr;    // If a callback returns true then no need to pass this message next to original WindowProc

CSWDTPicker(void){ }
//------------------------------------------------------------------------------------------------------------
int Create(SWDim& Wdim, LPCWSTR Text=L"", DWORD Style=0, DWORD ExStyle=0)  // Separate from constructor to allow some additional configuration
{                     
 if(!this->SuperClassSysCtrl(L"SysDateTimePick32", L"CSWDTPicker"))return -1;
 return (this->CreateWnd((LPCWSTR)this->WndClass, Text, Style|WS_CHILD, ExStyle, Wdim.PosX, Wdim.PosY, Wdim.Width, Wdim.Height, 0))?(0):(-2);
}           
//------------------------------------------------------------------------------------------------------------
/*virtual bool WindowProc(HWND& hWnd, UINT& Msg, WPARAM& wParam, LPARAM& lParam, LRESULT& lResult)    // No Effect!!!!!!!!!
{ 
 switch(Msg)
  {
   case WM_KEYDOWN:
 //  case WM_RBUTTONUP:
 //  case WM_MBUTTONUP:
//     Sleep(1);// if(this->OnMouseBtnUp)(this->GetOwnerWnd()->*OnMouseBtnUp)(this, Msg, wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
    break;
  } 
 
 CCldBase::WindowProc(hWnd, Msg, wParam, lParam, lResult);   
// LRESULT Result = 0;
// bool    NoOrig = false;
 switch(Msg)
  {
   case WM_KEYDOWN:
 //  case WM_RBUTTONUP:
 //  case WM_MBUTTONUP:
 //    Sleep(1);// if(this->OnMouseBtnUp)(this->GetOwnerWnd()->*OnMouseBtnUp)(this, Msg, wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
    break;
  } 
// if(NoOrig)return Result;        
 return true;
}  */
//------------------------------------------------------------------------------------------------------------
int GetSysTime(SYSTEMTIME* stime)
{
 int res = (int)SendMessageA(this->GetHandle(),DTM_GETSYSTEMTIME,0,(LPARAM)stime);
 if(GDT_VALID == res)return 1;
 if(GDT_NONE == res)return 0;
 return -1;
}
//------------------------------------------------------------------------------------------------------------

};