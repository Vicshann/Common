//==========================================================================================================================
//                                                  EDIT CLASS
//==========================================================================================================================
// https://docs.microsoft.com/en-us/windows/win32/controls/edit-controls-text-operations
/*
 Styles:
   ES_PASSWORD
   ES_READONLY
*/
class CSWEdit: public CCldBase
{          
public:
CSWEdit(void){}
//------------------------------------------------------------------------------------------------------------
int Create(SWDim& Wdim, LPCWSTR Text=L"", DWORD Style=0, DWORD ExStyle=0)
{        
 if(!this->SuperClassSysCtrl(L"EDIT", L"CSWEdit"))return -1;
 return (this->CreateWnd((LPCWSTR)this->WndClass, Text, Style|WS_CHILD, ExStyle, Wdim.PosX, Wdim.PosY, Wdim.Width, Wdim.Height, NULL))?(0):(-2);    // WS_EX_CLIENTEDGE  WS_EX_WINDOWEDGE
}
//------------------------------------------------------------------------------------------------------------
virtual bool WindowProc(HWND& hWnd, UINT& Msg, WPARAM& wParam, LPARAM& lParam, LRESULT& lResult)
{  
 CCldBase::WindowProc(hWnd, Msg, wParam, lParam, lResult);   
// LRESULT Result = 0;
// bool    NoOrig = false;
// LOGMSG("EditMsg: %08X)",Msg);
 switch(Msg)
  {
   case WM_CHAR:    // EN_CHANGE ???   // Seems there is no sane way to break on any modifications(Text Paste from clipboard)
//    if(this->OnMouseBtnUp)(this->GetOwnerWnd()->*OnMouseBtnUp)(this, Msg, wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
  //  Sleep(1);
    break;
  } 
// if(NoOrig)return Result;        
 return true;
}
//------------------------------------------------------------------------------------------------------------
// The return value will never be less than 1
UINT LineCount(void)
{
 return SendMessageW(this->GetHandle(),EM_GETLINECOUNT,0,0);
}
//------------------------------------------------------------------------------------------------------------
UINT GetTextLimit(void)  // 30000
{
 return SendMessageW(this->GetHandle(),EM_GETLIMITTEXT,0,0);
}
//------------------------------------------------------------------------------------------------------------
template<typename T> int AddLine(T Str)
{
 int index = this->GetTextLen();
 SendMessageW(this->GetHandle(), EM_SETSEL, (WPARAM)index, (LPARAM)index); // set selection - end of text
 if(sizeof(*Str) == sizeof(wchar_t))
  {
   UINT  Len = lstrlenW((PWSTR)Str) * sizeof(wchar_t);
   bool NeedBuf = Len && (Str[Len-1] != '\n');
   if(NeedBuf)
    {
     BYTE* buf = (BYTE*)malloc(Len+6);
     memcpy(buf, Str, Len);
     memcpy(&buf[Len], L"\r\n", 3*sizeof(wchar_t));
     int res = SendMessageW(this->GetHandle(), EM_REPLACESEL, 0, (LPARAM)buf); // append!;
     free(buf);
     return res;
    }
     else return SendMessageW(this->GetHandle(), EM_REPLACESEL, 0, (LPARAM)Str);
  }
  else if(sizeof(*Str) == sizeof(char))
   {
    UINT  Len = lstrlenA((LPSTR)Str) * sizeof(char);
    bool NeedBuf = Len && (Str[Len-1] != '\n');
    if(NeedBuf)
     {
      BYTE* buf = (BYTE*)malloc(Len+6);
      memcpy(buf, Str, Len);
      memcpy(&buf[Len], "\r\n", 3*sizeof(char));
      int res = SendMessageA(this->GetHandle(), EM_REPLACESEL, 0, (LPARAM)buf); // append!;
      free(buf);
      return res;
     }
      else return SendMessageA(this->GetHandle(), EM_REPLACESEL, 0, (LPARAM)Str);
   }
 return -9;
}
//------------------------------------------------------------------------------------------------------------
template<typename T> UINT GetLine(UINT Index, T Buffer, UINT Size)              // Size is in chars
{
 UINT cnt = this->LineCount();
 if(Index >= cnt)return -1;
 *(UINT32*)Buffer = Size;
 if(sizeof(*Buffer) == sizeof(wchar_t))return SendMessageW(this->GetHandle(),WM_GETTEXT,Index,(LPARAM)Buffer);    
  else if(sizeof(*Buffer) == sizeof(char))return SendMessageA(this->GetHandle(),WM_GETTEXT,Index,(LPARAM)Buffer);
 return -1;
}

};