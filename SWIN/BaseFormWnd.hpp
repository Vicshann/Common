//==========================================================================================================================
//                                               WINDOW FORM CLASS
//==========================================================================================================================
// TODO: Unicode flag (RegisterClassExW) // Affects child sys windows?
class CWndForm: public CWndBase    // TODO: Partial update of controls, do not redraw all
{
 HFONT Font = nullptr;   // Custom class windows 
//------------------------------------------------------------------------------------------------------------

public:
//------------------------------------------------------------------------------------------------------------
CWndForm(void)
{ 
 this->Flags |= flDerWndForm;   
}
//------------------------------------------------------------------------------------------------------------
int Create(LPCWSTR WndName, SWDim& Wdim, HWND hParentWnd, DWORD Style=0, DWORD ExStyle=0, HICON hWndIcon=LoadIconW(NULL, (LPCWSTR)IDI_APPLICATION), HBRUSH hBgrBrush=GetSysColorBrush(COLOR_BTNFACE))
{
 WNDCLASSEXW wcls; 
 WCHAR WCName[256];

 lstrcpyW(&WCName[1], L"CSWForm");
 WCName[0] = (hBgrBrush)?('S'):('N');    // ???
 if(!this->hOwnerMod)this->SetHInstance();
 if(!(this->WndClass = GetClassInfoExW(this->hOwnerMod,(LPCWSTR)&WCName,&wcls)))  // Returns an ATOM cast to BOOL as said on MSDN     // CLASS is already registered  // UnregisterClassA on it will fail unless there is no windows of this class left
  {
   wcls.cbSize        = sizeof(WNDCLASSEX);
   wcls.style         = CS_OWNDC|CS_HREDRAW|CS_VREDRAW|CS_GLOBALCLASS;
   wcls.cbClsExtra    = sizeof(PVOID);   // To store DefWindowProc
   wcls.cbWndExtra    = 0;
   wcls.hInstance     = this->hOwnerMod; 
   wcls.hIcon         = hWndIcon;                 
   wcls.hCursor       = LoadCursorW(0, (LPCWSTR)IDC_ARROW);
   wcls.hbrBackground = hBgrBrush;//GetSysColorBrush(COLOR_BTNFACE); // (HBRUSH)(COLOR_BACKGROUND+1);  
   wcls.lpszClassName = (LPCWSTR)&WCName;
   wcls.lpszMenuName  = 0;
   wcls.hIconSm       = 0;
   wcls.lpfnWndProc   = (WNDPROC)&CWndBase::WndProxyProc;   // Belongs to this window CLASS  // WM_CREATE is passed to class`s proc because only there it is specified before a window created
   this->WndClass     = RegisterClassExW(&wcls);
   if(!this->WndClass)return -1;
  }
 return (this->CreateWnd((LPCWSTR)this->WndClass, WndName, Style, ExStyle, Wdim.PosX, Wdim.PosY, Wdim.Width, Wdim.Height, hParentWnd))?(0):(-2);
}
//------------------------------------------------------------------------------------------------------------
virtual bool WindowProc(HWND& hWnd, UINT& Msg, WPARAM& wParam, LPARAM& lParam, LRESULT& lResult)
{    
 CWndBase::WindowProc(hWnd, Msg, wParam, lParam, lResult);  // Let base class to process the messages before we extend them   // Before or After processing
 switch(Msg)
  {
   case WM_SIZE:
    this->NotifyChildren(this->hWindow, Msg, wParam, lParam);
    break;
   case WM_GETFONT:
    lResult = (LRESULT)this->Font;
    break; 
   case WM_SETFONT:
    this->Font = (HFONT)wParam;
    if((bool)lParam){InvalidateRect(hWnd, NULL, TRUE); lResult = CallWindowProcW(this->OrigWndProc, hWnd, WM_PAINT, 0, 0);}
    break;
  }        
 return true;  
}
//------------------------------------------------------------------------------------------------------------
};
