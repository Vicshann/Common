//==========================================================================================================================
//                                               BASE WINDOW CLASS
//==========================================================================================================================
class CWndBase: public CVisBase     
{
protected:
 bool       bClosed = false;
 HWND       hWindow = nullptr;    // Handle to this window control
 ATOM       WndClass = 0;         // NULL for a system windows
 WNDPROC    OrigWndProc = DefWindowProcW;

//CWndBase* GetOwnerWnd(void){return reinterpret_cast<CWndBase*>(this->OwnerObj);}     // Simple helper cast: moved to ObjBase
//------------------------------------------------------------------------------------------------------------
static LRESULT CALLBACK WndProxyProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
 CWndBase* This = (CWndBase*)GetWindowLongPtrW(hWnd, WLP_USERDATA);    // Or use a window`s extra bytes instead?
 if(!This && ((Msg == WM_NCCREATE)||(Msg == WM_CREATE)))     // WM_NCCREATE then WM_CREATE or only one of them
  {
   This = (CWndBase*)((CREATESTRUCT*)lParam)->lpCreateParams;
   UINT  ClsExLen = (UINT)GetClassLongPtrW(hWnd, GCL_CBCLSEXTRA); 
   PVOID OProc    = (PVOID)GetClassLongPtrW(hWnd, ClsExLen-sizeof(PVOID));   
   if(!OProc)SetClassLongPtrW(hWnd,ClsExLen-sizeof(PVOID),(ULONG_PTR)This->OrigWndProc);   // Assign original WndProc to the class
     else This->OrigWndProc = (WNDPROC)OProc;             
   SetWindowLongPtrW(hWnd, WLP_USERDATA, (ULONG_PTR)This);    // What about MDI windows? 
   This->hWindow = hWnd;   // This assignment duplicated in CreateWnd after CreateWindowEx returns
  } 
 if(This)
  {
   LRESULT Res = 0;
   if(This->WindowProc(hWnd, Msg, wParam, lParam, Res))return Res;
  }
 return DefWindowProcW(hWnd, Msg, wParam, lParam);
}
//------------------------------------------------------------------------------------------------------------
// Used to superclass system controls classes
//
ATOM SuperClassSysCtrl(LPCWSTR OrigClassName, LPCWSTR ClassName, UINT ExStyles=0)  // hInstance is NULL to superclass a system controls
{
 WNDCLASSEXW wcls;  
 WNDCLASSEXW ewcls; 

 wcls.cbSize   = sizeof(WNDCLASSEXW);
 ewcls.cbSize  = sizeof(WNDCLASSEXW);
 if(!GetClassInfoExW(NULL,OrigClassName,&wcls))return 0;     // Wrong system class?
 if(!this->hOwnerMod)this->SetHInstance();   
 if(ATOM MyClass  = GetClassInfoExW(this->hOwnerMod,ClassName,&ewcls))   // Returns an ATOM cast to BOOL as said on MSDN     // CLASS is already registered
  {        
   this->WndClass = MyClass;                                 // UnregisterClassA on it will fail unless there is no windows of this class left
   return this->WndClass;         // ATOM of an existing class
  }
 this->OrigWndProc  = wcls.lpfnWndProc;          // WM_CREATE will assign 'THIS' pointer to a window and there we assign original lpfnWndProc to a CLASS     // Should be after GetClassInfoExA???? - test it later
 wcls.hInstance     = this->hOwnerMod;
 wcls.lpfnWndProc   = (WNDPROC)&CWndBase::WndProxyProc;
 wcls.lpszClassName = ClassName;
 wcls.style        |= ExStyles | CS_GLOBALCLASS;
 wcls.cbClsExtra   += sizeof(PVOID);    // To store original lpfnWndProc
 this->WndClass     = RegisterClassExW(&wcls);
 return this->WndClass;
}
//------------------------------------------------------------------------------------------------------------
HWND CreateWnd(LPCWSTR ClassName, LPCWSTR WindowName, DWORD Style, DWORD ExStyle, int PosX, int PosY, int Width, int Height, HWND hParentWnd=NULL)
{      
 if(!this->hOwnerMod)this->SetHInstance();
 if(this->OwnerObj)hParentWnd = this->GetOwnerWnd()->hWindow;   // Overrides
 if(this->hWindow = CreateWindowExW(ExStyle, ClassName, WindowName, Style, PosX, PosY, Width, Height, hParentWnd, 0, this->hOwnerMod, this))     // 'this' is passed as LPARAM but not used in WM_CREATE message because WNDPROC is not subclassed yet
  {
   if(this->OwnerObj)this->SetFont(this->GetOwnerWnd()->GetFont());    // Use Parent`s font  // 'true' - redraw immediately?
  }
 return this->hWindow;
}
//------------------------------------------------------------------------------------------------------------
void NotifyChildren(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam, UINT NotifyFlg=0)
{
 if(!this->ChildLst)return; 
 for(UINT ctr=0;ctr < this->ChildCnt;ctr++)
  { 
   if(!this->ChildLst[ctr]->IsDerivedFromAny(flDerWndBase))continue;  // Skip non windowed components
   CWndBase* Wnd = (CWndBase*)this->ChildLst[ctr];
   if(Wnd->hWindow == hWnd)continue;     // Do not loop itself from 'NotifyParents'
   LRESULT Res = 0;
   Wnd->WindowProc(hWnd, Msg, wParam, lParam, Res);
   if(NotifyFlg & wnDeepChld)Wnd->NotifyChildren(hWnd, Msg, wParam, lParam, NotifyFlg);  // Recursive
  }
}
//------------------------------------------------------------------------------------------------------------  enum EWNotify {wnChildren=1, wnDeepChld=2, wnDeepPrnt=4};
void NotifyParents(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam, UINT NotifyFlg=0)
{
 if(!this->OwnerObj->IsDerivedFromAny(flDerWndBase))return;
 CWndBase* ParWnd = (CWndBase*)this->OwnerObj;
 LRESULT Res = 0;
 ParWnd->WindowProc(hWnd, Msg, wParam, lParam, Res);
 if(NotifyFlg & wnChildren)ParWnd->NotifyChildren(hWnd, Msg, wParam, lParam, NotifyFlg);
 if(NotifyFlg & wnDeepPrnt)ParWnd->NotifyParents(hWnd, Msg, wParam, lParam, NotifyFlg);
}
//------------------------------------------------------------------------------------------------------------  enum EWNotify {wnChildren=1, wnDeepChld=2, wnDeepPrnt=4};

public:
 void (_fastcall CWndBase::*OnClose)(CWndBase* Sender) = nullptr; 
 void (_fastcall CWndBase::*OnPaint)(CWndBase* Sender) = nullptr; 
 void (_fastcall CWndBase::*OnResize)(CWndBase* Sender, UINT Type, WORD Width, WORD Height) = nullptr; 
 void (_fastcall CWndBase::*OnMouseBtnDn)(CWndBase* Sender, WORD EvtKey, WORD KeyState, int x, int y) = nullptr;
 void (_fastcall CWndBase::*OnKeyDn)(CWndBase* Sender, WORD KeyCode, WORD RepCnt, BYTE ScanCode, bool ExtKey, bool PrvState) = nullptr; 

//------------------------------------------------------------------------------------------------------------
CWndBase(void)
{ 
 this->Flags      |= flDerWndBase; 
}
//------------------------------------------------------------------------------------------------------------
// Notifies only known CWndBase controls, not system ones
//
void NotifyChildren(UINT Msg, WPARAM wParam, LPARAM lParam, UINT NotifyFlg=0){this->NotifyChildren(this->hWindow, Msg, wParam, lParam, NotifyFlg);}
void NotifyParents(UINT Msg, WPARAM wParam, LPARAM lParam, UINT NotifyFlg=0){this->NotifyParents(this->hWindow, Msg, wParam, lParam, NotifyFlg);}
//------------------------------------------------------------------------------------------------------------
bool TakeOwnership(HWND hWindow)     // Subclass it
{
 return true;
}
//------------------------------------------------------------------------------------------------------------
bool TakeOwnership(CWndBase* hWindow)
{
 return true;
}
//------------------------------------------------------------------------------------------------------------
/*
By default, the DefWindowProc function sends the WM_SIZE and WM_MOVE messages to the window. The WM_SIZE and WM_MOVE messages are not sent if an application handles 
the WM_WINDOWPOSCHANGED message without calling DefWindowProc. It is more efficient to perform any move or size change processing during the WM_WINDOWPOSCHANGED message 
without calling DefWindowProc. 

*/
bool CascadeChangeWindowRect(RECT* wrec)
{
 return 0;
}
//------------------------------------------------------------------------------------------------------------
UINT ChildWindowsCount(void)
{

 return 0;
}
//------------------------------------------------------------------------------------------------------------
bool IsClosed(void){return IsWindow(this->hWindow);}
HWND GetHandle(void){return this->hWindow;}
//HWND GetOwnerHandle(void){return (this->OwnerObj)?(((CWndBase*)this->OwnerObj)->hWindow):(NULL);}     // Disabled here: owner may be non-window object
//------------------------------------------------------------------------------------------------------------
virtual ~CWndBase()    // A thread cannot use DestroyWindow to destroy a window created by a different thread. 
{                      // 'DestroyWindow' may fail here because all child windows is already deleted by parent`s 'DestroyWindow' call
 if(this->hWindow)DestroyWindow(this->hWindow);    // If the specified window is a parent or owner window, DestroyWindow automatically destroys the associated child or owned windows when it destroys the parent or owner window.
 if(this->WndClass)UnregisterClassW((LPCWSTR)this->WndClass, this->hOwnerMod);   // Restore original WindowProc before this?   // Will work only if there is no windows left of that class
}  
//------------------------------------------------------------------------------------------------------------
virtual bool WindowProc(HWND& hWnd, UINT& Msg, WPARAM& wParam, LPARAM& lParam, LRESULT& lResult)      // Returns TRUE if the message processed
{
 if(CSWTrayIcon::HaveTrayIcons())
  {
   if((Msg == CSWTrayIcon::GMsgTaskbarCreated) || (Msg == WM_DPICHANGED))CSWTrayIcon::NotifyAllTrayIcons(this, Msg, wParam, lParam, lResult);     // TODO: WM_DPICHANGED should be processed in main window handler
  }

 switch(Msg)
 {
   case WM_CLOSE:
     if(this->OnClose)(this->*OnClose)(this);   // If called after CallWindowProcA then all children controls are already destroyed and it is impossible to read anything from them in OnClose handler  
     break;
//  case WM_SETCURSOR:
//    break;
   case WM_TIMER:
     lResult = 0;
     return CSWTimer::ProcessTimerCallback(this, wParam, lParam);

   case CSWTrayIcon::TrayIconMsgBase:   
     return CSWTrayIcon::NotifyRelatedTrayIcon(this, Msg, wParam, lParam, lResult);      // This message is only for our window
 }

 lResult = CallWindowProcW(this->OrigWndProc, hWnd, Msg, wParam, lParam);   // Process on original WindowProc, let Windows update what it needs to  // TODO: decide this for each of our supported message if we need to do this before or after a child controld WndProc call
 switch(Msg)
  {
//   case WM_CLOSE:
//     if(this->OnClose)(this->*OnClose)(this);     
//     break;

   case WM_VSCROLL:  // Hello! We need these in a child control handler, not here!
   case WM_HSCROLL:
    if(lParam)       // Pass it down to a child Scroll Bar    // TODO: Pass other scroll bar messages back to it
     {
      if(CWndBase* CldThis = (CWndBase*)GetWindowLongPtrW((HWND)lParam, WLP_USERDATA)) 
       {
        hWnd   = CldThis->GetHandle();
        lParam = NULL;        // Prevents this message from looping WindowProc
        CldThis->WindowProc(hWnd, Msg, wParam, lParam, lResult);
       }
     }
    break;

   case WM_SIZE:       // WM_SIZING later
    if(this->OnResize)(this->*OnResize)(this, (UINT)wParam, WORD(lParam & 0xFFFF), WORD(lParam >> 16));
    break;

   case WM_PAINT:         // Never received for main windows! Only for controls?
   case WM_ERASEBKGND:    // If you set the class background brush to NULL, however, the system sends a WM_ERASEBKGND message to your window procedure whenever the window background must be drawn, letting you draw a custom background.
    if(this->OnPaint)(this->*OnPaint)(this);
    break;

   case WM_XBUTTONDOWN:
    if(this->OnMouseBtnDn)(this->*OnMouseBtnDn)(this, ((wParam >> 16) & XBUTTON1)?(MK_XBUTTON1):(MK_XBUTTON2), wParam & 0xFFFF, int(lParam & 0xFFFF), int(lParam >> 16));
    break;
   case WM_MBUTTONDOWN:
    if(this->OnMouseBtnDn)(this->*OnMouseBtnDn)(this, MK_MBUTTON, wParam & 0xFFFF, int(lParam & 0xFFFF), int(lParam >> 16));
    break;
   case WM_RBUTTONDOWN:
    if(this->OnMouseBtnDn)(this->*OnMouseBtnDn)(this, MK_RBUTTON, wParam & 0xFFFF, int(lParam & 0xFFFF), int(lParam >> 16));
    break;
   case WM_LBUTTONDOWN:
    if(this->OnMouseBtnDn)(this->*OnMouseBtnDn)(this, MK_LBUTTON, wParam & 0xFFFF, int(lParam & 0xFFFF), int(lParam >> 16));
    break;

   case WM_KEYDOWN:
   case WM_SYSKEYDOWN:   // For F10
    if(this->OnKeyDn)(this->*OnKeyDn)(this, (WORD)wParam, lParam & 0xFFFF, (lParam >> 16) & 0xFF, lParam & (1 << 24), lParam & (1 << 30));
    break;
  }
 return true;
}      
//------------------------------------------------------------------------------------------------------------
// NOTE: Only system window classes support these by default (All of them?)
virtual HFONT GetFont(void){return (HFONT)SendMessageW(this->hWindow, WM_GETFONT, NULL, NULL);}
virtual void  SetFont(HFONT Font, bool Redraw=false){SendMessageW(this->hWindow, WM_SETFONT, (LPARAM)Font, Redraw);}
virtual bool Show(bool Show=true){return ShowWindow(this->hWindow, (Show)?(SW_SHOW):(SW_HIDE));}   
//------------------------------------------------------------------------------------------------------------
/*virtual HCURSOR SetCursor(HCURSOR cur)
{
 HCURSOR res = (HCURSOR)SetClassLongPtrW(this->hWindow,GCLP_HCURSOR,(LONG_PTR)cur);   // No effect and a bad idea to to it globally
 SendMessageW(this->hWindow, WM_SETCURSOR, 0, (LPARAM)cur);
 return res;
} */
//------------------------------------------------------------------------------------------------------------
UINT GetTextLen(void)
{       
 return (UINT)SendMessageW(this->GetHandle(),WM_GETTEXTLENGTH,0,0);
}
//------------------------------------------------------------------------------------------------------------
template<typename T> int SetText(T Str)
{
 if(sizeof(*Str) == sizeof(wchar_t))return SendMessageW(this->GetHandle(),WM_SETTEXT,0,(LPARAM)Str);
  else if(sizeof(*Str) == sizeof(char))return SendMessageA(this->GetHandle(),WM_SETTEXT,0,(LPARAM)Str);
 return -9;
}
//------------------------------------------------------------------------------------------------------------
template<typename T> int GetText(T Buffer, UINT Size)              // Size is in chars
{
 LRESULT len = this->GetTextLen();
 if(len <= 0){*Buffer=0; return 0;}
 if((UINT)len > Size)len = len;
 if(sizeof(*Buffer) == sizeof(wchar_t))return SendMessageW(this->GetHandle(),WM_GETTEXT,len+1,(LPARAM)Buffer);    // No need for specialization because it will still fail if you pass a wrong buffer type 
  else if(sizeof(*Buffer) == sizeof(char))return SendMessageA(this->GetHandle(),WM_GETTEXT,len+1,(LPARAM)Buffer);
 return -1;
}
//------------------------------------------------------------------------------------------------------------

};