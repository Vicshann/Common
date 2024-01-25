//==========================================================================================================================
//                                                 COMBO BOX CLASS
//==========================================================================================================================
/*
https://learn.microsoft.com/en-us/windows/win32/controls/cbn-selchange
 
CBS_SIMPLE: 	Displays the list at all times, and shows the selected item in an edit control.
CBS_DROPDOWN: 	Displays the list when the icon is clicked, and shows the selected item in an edit control.
CBS_DROPDOWNLIST: 	Displays the list when the icon is clicked, and shows the selected item in a static control.
*/
class CSWComboBox: public CCldBase
{
public:
// void (_fastcall CWndBase::*OnMouseBtnUp)(CWndBase* Sender, WORD WMsg, WORD KeyEx, int x, int y) = nullptr;    // If a callback returns true then no need to pass this message next to original WindowProc

CSWComboBox(void){ }
//------------------------------------------------------------------------------------------------------------
int Create(SWDim& Wdim, LPCWSTR Text=L"", DWORD Style=0, DWORD ExStyle=0)  // Separate from constructor to allow some additional configuration
{                     
 if(!this->SuperClassSysCtrl(L"ComboBox", L"CSWComboBox"))return -1;
 return (this->CreateWnd((LPCWSTR)this->WndClass, Text, Style|WS_CHILD, ExStyle, Wdim.PosX, Wdim.PosY, Wdim.Width, Wdim.Height, 0))?(0):(-2);
}           
//------------------------------------------------------------------------------------------------------------
virtual bool WindowProc(HWND& hWnd, UINT& Msg, WPARAM& wParam, LPARAM& lParam, LRESULT& lResult)    // No Effect!!!!!!!!!
{ 
 switch(Msg)
  {
   case WM_KEYDOWN:      // CBN_SELCHANGE to parent
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
}
//------------------------------------------------------------------------------------------------------------
UINT GetCount(void)
{
 return (UINT)SendMessageW(this->GetHandle(),CB_GETCOUNT,0,0);
}
//------------------------------------------------------------------------------------------------------------
int Reset(void)
{
 return (int)SendMessageW(this->GetHandle(),CB_RESETCONTENT,0,0);
}
//------------------------------------------------------------------------------------------------------------
int SelectItem(int Idx)
{
 return (int)SendMessageW(this->GetHandle(),CB_SETCURSEL,(WPARAM)Idx,0);
}
//------------------------------------------------------------------------------------------------------------
int GetSelItem(void)
{
 return (int)SendMessageW(this->GetHandle(),CB_GETCURSEL,0,0);
}
//------------------------------------------------------------------------------------------------------------
int Preallocate(UINT ItmCount, size_t MemSize)
{
 return (int)SendMessageW(this->GetHandle(),CB_RESETCONTENT,(WPARAM)ItmCount,(LPARAM)MemSize);
}
//------------------------------------------------------------------------------------------------------------
// The return value is the zero-based index to the string in the list box of the combo box
template<typename T> UINT AddStr(T Str)              // Size is in chars
{
 if(sizeof(*Str) == sizeof(wchar_t))return SendMessageW(this->GetHandle(),CB_ADDSTRING,0,(LPARAM)Str);    
  else if(sizeof(*Str) == sizeof(char))return SendMessageA(this->GetHandle(),CB_ADDSTRING,0,(LPARAM)Str);
 return -1;
}
//------------------------------------------------------------------------------------------------------------
// If Idx is -1 then adds to end
// does not cause a list with the CBS_SORT style to be sorted
// The return value is the index of the position at which the string was inserted
template<typename T> UINT InsertStr(int Idx, T Str)              // Size is in chars
{
 if(sizeof(*Str) == sizeof(wchar_t))return SendMessageW(this->GetHandle(),CB_INSERTSTRING,(WPARAM)Idx,(LPARAM)Str);    
  else if(sizeof(*Str) == sizeof(char))return SendMessageA(this->GetHandle(),CB_INSERTSTRING,(WPARAM)Idx,(LPARAM)Str);
 return -1;
}
//------------------------------------------------------------------------------------------------------------
int DeleteItem(int Idx)
{
 return (int)SendMessageW(this->GetHandle(),CB_DELETESTRING,(WPARAM)Idx,0);
}
//------------------------------------------------------------------------------------------------------------
size_t GetItemData(int Idx)
{
 return (size_t)SendMessageW(this->GetHandle(),CB_GETITEMDATA,(WPARAM)Idx,0);
}
//------------------------------------------------------------------------------------------------------------
int SetItemData(int Idx, size_t Data)
{
 return (int)SendMessageW(this->GetHandle(),CB_DELETESTRING,(WPARAM)Idx,(LPARAM)Data);
}

};