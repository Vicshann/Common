//==========================================================================================================================
//                                                 BUTTON CLASS
//==========================================================================================================================
// https://docs.microsoft.com/en-us/windows/win32/controls/button-messages          // See redirection from parent
class CSWButton: public CCldBase     // Todo: Separaate Styles
{
public:
// bool (_fastcall CWndBase::*OnMouseBtnUp)(CWndBase* Sender, LRESULT* Result, WORD WMsg, WORD KeyEx, int x, int y);    // If a callback returns true then no need to pass this message next to original WindowProc
 void (_fastcall CWndBase::*OnMouseBtnUp)(CWndBase* Sender, WORD WMsg, WORD KeyEx, int x, int y) = nullptr;    // If a callback returns true then no need to pass this message next to original WindowProc

CSWButton(void){ }
//------------------------------------------------------------------------------------------------------------
int Create(SWDim& Wdim, LPCWSTR Text=L"", DWORD Style=0, DWORD ExStyle=0)  // Separate from constructor to allow some additional configuration
{ 
// DWORD Style = WS_CHILD|BS_CENTER|BS_DEFPUSHBUTTON; 
 if(!this->SuperClassSysCtrl(L"BUTTON", L"CSWButton"))return -1;
 return (this->CreateWnd((LPCWSTR)this->WndClass, Text, Style|WS_CHILD, ExStyle, Wdim.PosX, Wdim.PosY, Wdim.Width, Wdim.Height, 0))?(0):(-2);
}
//------------------------------------------------------------------------------------------------------------
virtual bool WindowProc(HWND& hWnd, UINT& Msg, WPARAM& wParam, LPARAM& lParam, LRESULT& lResult)
{  
 CCldBase::WindowProc(hWnd, Msg, wParam, lParam, lResult);   
// LRESULT Result = 0;
// bool    NoOrig = false;
 switch(Msg)
  {
   case WM_LBUTTONUP:
   case WM_RBUTTONUP:
   case WM_MBUTTONUP:
    if(this->OnMouseBtnUp)(this->GetOwnerWnd()->*OnMouseBtnUp)(this, Msg, wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
    break;
  } 
// if(NoOrig)return Result;        
 return true;
}
//------------------------------------------------------------------------------------------------------------
int GetChecked(void)
{
 int res = SendMessageW(this->GetHandle(),BM_GETCHECK,0,0);
 if(res == BST_CHECKED)return 1;
 if(res == BST_UNCHECKED)return 0;
 return -1;
}
//------------------------------------------------------------------------------------------------------------
int GetState(void)
{
 return SendMessageW(this->GetHandle(),BM_GETSTATE,0,0);
}
//------------------------------------------------------------------------------------------------------------
void SetChecked(int Chk)
{
 int Val = BST_INDETERMINATE;
 if(Chk > 0)Val = BST_CHECKED;
  else if(Chk == 0)Val = BST_UNCHECKED;
 SendMessageW(this->GetHandle(),BM_SETCHECK,Val,0);
}
//------------------------------------------------------------------------------------------------------------

};