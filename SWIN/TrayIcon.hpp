//==========================================================================================================================
//                                               TRAY ICON
//--------------------------------------------------------------------------------------------------------------------------
// PopUp menu list for any specific key
// Icon List and frame delay for animation
// Fast SetIcon for manual animation
// ShowMsg for balloon messages
// 
// https://stackoverflow.com/questions/41649303/difference-between-notifyicon-version-and-notifyicon-version-4-used-in-notifyico
//
class CSWTrayIcon: public CVisBase
{
friend CWndBase;

#pragma pack(push,1)
struct SNOTIFYICONDATAW32   // Size = 0x03BC // Handles are x32 too!    //  Shell_NotifyIconW uses this struct internally
{
    DWORD cbSize;
    DWORD hWnd;    // HWND
    UINT uID;
    UINT uFlags;
    UINT uCallbackMessage;
    DWORD hIcon;     // HICON
    WCHAR szTip[128];
    DWORD dwState;
    DWORD dwStateMask;
    WCHAR  szInfo[256];
    union {
        UINT  uTimeout;
        UINT  uVersion;  // used with NIM_SETVERSION, values 0, 3 and 4
    } DUMMYUNIONNAME;
    WCHAR  szInfoTitle[64];
    DWORD dwInfoFlags;
    GUID guidItem;
    DWORD hBalloonIcon;   // HICON
};

 struct SNIDataW    // Size = 0x05CC
  {
   DWORD Magic;
   DWORD Message;
   SNOTIFYICONDATAW32 Data;
   BYTE  Reserved[0x300];
  };
#pragma pack(pop)

static inline constexpr DWORD TrayIconMsgBase = WM_USER + 0x200;
static inline int GMsgTaskbarCreated = -1;
static inline UINT TrayVer   = 0;
static const  UINT RangeUID  = 100;   // Additional frames must be in this range

bool  Hidden    = false;
bool  Invalid   = false;
HWND  hTaskBar  = nullptr;
UINT  HIcoCtr   = 0;
DWORD* HIcoLst  = nullptr;  

static bool HaveTrayIcons(void){return GMsgTaskbarCreated >= 0;}    // The window may have some tray icons
//------------------------------------------------------------------------------------------------------------
static bool NotifyRelatedTrayIcon(CWndBase* Parent, UINT& Msg, WPARAM& wParam, LPARAM& lParam, LRESULT& lResult)   // In response to TrayIconMsgBase
{
 CSWTrayIcon* ti = nullptr;
 if(TrayVer > NOTIFYICON_VERSION)
  {
   ti = (CSWTrayIcon*)Parent->GetObj(HIWORD(lParam) / RangeUID);
   if(!ti)return false;
   Msg = LOWORD(lParam);
   lParam = GET_Y_LPARAM(wParam);
   wParam = GET_X_LPARAM(wParam);
  }
   else
    {
     ti = (CSWTrayIcon*)Parent->GetObj((UINT)wParam / RangeUID);
     if(!ti)return false;
     Msg = lParam;
     POINT cp = {};
     GetCursorPos(&cp);   // Cursor is positioned at the center of the tray icon
     lParam = cp.y;
     wParam = cp.x;
    }
 HWND Wnd = nullptr;
 return ti->WindowProc(Wnd, Msg, wParam, lParam, lResult); 
}
//------------------------------------------------------------------------------------------------------------
static UINT NotifyAllTrayIcons(CWndBase* Parent, UINT& Msg, WPARAM& wParam, LPARAM& lParam, LRESULT& lResult) 
{
 UINT Tot = 0;
 for(UINT ctr=0;ctr < Parent->GetObjCnt();ctr++)
  { 
   if(Parent->GetObjLst()[ctr]->IsDerivedFromAny(flDerTrayIcon))
    {
     HWND Wnd = nullptr;
     CSWTrayIcon* ti = (CSWTrayIcon*)Parent->GetObjLst()[ctr];
     Tot += ti->WindowProc(Wnd, Msg, wParam, lParam, lResult);   // An empty child slot found
    }
  }  
 return Tot;
}
//------------------------------------------------------------------------------------------------------------
// Find IconID for which there is no preloaded icon yet
UINT FindFreeIconFrameSlot(void)
{
 for(UINT Idx=0;Idx < this->HIcoCtr;Idx++)
  { 
   if(!this->HIcoLst[Idx])return Idx;
  }
 return this->HIcoCtr;     // Need a new slot
}
//------------------------------------------------------------------------------------------------------------
void StoreIconFrameInfo(UINT SlotIdx, HICON hIco)    // IconId will be SlotIdx+1
{ 
 if(this->HIcoLst)
  {
   if(SlotIdx == this->HIcoCtr)this->HIcoLst = (DWORD*)HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, this->HIcoLst, (++this->HIcoCtr * sizeof(DWORD)));   // Enlarge to +1
  }
   else { this->HIcoLst = (DWORD*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DWORD)); this->HIcoCtr = 1; }   // First child 
 this->HIcoLst[SlotIdx] = (DWORD)hIco;  
}
//------------------------------------------------------------------------------------------------------------
// Original Shell_NotifyIcon searches for "Shell_TrayWnd" window on every call and does bunch of checks on HICON after WinXP
//
int CtrlNotifyIcon(DWORD Msg, SNIDataW* pData)         // return Shell_NotifyIconW(Msg, pData);
{
 COPYDATASTRUCT cd = {};
 if(!this->hTaskBar)this->hTaskBar = FindWindowW(L"Shell_TrayWnd", 0);
 if(!this->hTaskBar)return -1;
  
 pData->Magic   = 0x34753423;
 pData->Message = Msg;
 if(Msg == NIM_SETFOCUS)   // Will it work on Win10?
  {
   DWORD dwProcessId = 0;
   GetWindowThreadProcessId(this->hTaskBar, &dwProcessId);
   AllowSetForegroundWindow(dwProcessId);
  }
 DWORD_PTR dwResult = 0;
 cd.dwData = 1;
 cd.cbData = sizeof(SNIDataW);   // The size should not be less than 'sizeof(SNOTIFYICONDATAW32) + 8 + 0x200 + 8 = 0x05CC'
 cd.lpData = pData;
 if(SendMessageTimeoutW(this->hTaskBar, WM_COPYDATA, (WPARAM)pData->Data.hWnd, (LPARAM)&cd, SMTO_BLOCK|SMTO_ABORTIFHUNG|SMTO_NOTIMEOUTIFNOTHUNG, 7000, &dwResult))return dwResult;
 return -2;
}        
//------------------------------------------------------------------------------------------------------------
public:     // NOTE: Compiler may merge your handlers if their bodies are same, ignoring fact of their address being taken
 void (_fastcall CObjBase::*OnMouseBtnDn)(CObjBase* Sender, WORD EvtKey, WORD KeyState, int x, int y) = nullptr;
 void (_fastcall CObjBase::*OnMouseBtnUp)(CObjBase* Sender, WORD EvtKey, WORD KeyState, int x, int y) = nullptr;
 void (_fastcall CObjBase::*OnMouseBtnClk)(CObjBase* Sender, WORD EvtKey, int x, int y) = nullptr;
 void (_fastcall CObjBase::*OnMouseMove)(CObjBase* Sender, int x, int y) = nullptr;
 void (_fastcall CObjBase::*OnContextMenu)(CObjBase* Sender, int x, int y) = nullptr;
 void (_fastcall CObjBase::*OnUserLAction)(CObjBase* Sender, int x, int y) = nullptr;
 void (_fastcall CObjBase::*OnIconLost)(CObjBase* Sender) = nullptr;

 void (_fastcall CObjBase::*OnBalloonShow)(CObjBase* Sender) = nullptr;
 void (_fastcall CObjBase::*OnBalloonUsrClk)(CObjBase* Sender) = nullptr;
 void (_fastcall CObjBase::*OnBalloonUsrClose)(CObjBase* Sender) = nullptr;

CSWTrayIcon(void) 
{ 
 this->Flags |= flDerTrayIcon; 
 this->SetTypeID(this); 
 if(!pChangeWindowMessageFilterEx)*(PVOID*)&pChangeWindowMessageFilterEx = GetProcAddress(GetModuleHandle("User32.dll"), "ChangeWindowMessageFilterEx");
}

//------------------------------------------------------------------------------------------------------------
UINT GetBaseId(void){return this->IdxInPar * RangeUID;}
//------------------------------------------------------------------------------------------------------------
int Create(HICON Icon, LPCWSTR Hint=nullptr, bool Visible=false)  // Separate from constructor to allow some additional configuration
{     
 int res = this->CreateIcon(Icon, Hint, Visible);
 if((GMsgTaskbarCreated < 0) && (res >= 0))
  {
   GMsgTaskbarCreated = RegisterWindowMessageW(L"TaskbarCreated");
   if(pChangeWindowMessageFilterEx)
    {
     pChangeWindowMessageFilterEx(this->GetOwnerHandle(),TrayIconMsgBase, MSGFLT_ALLOW, nullptr);       // Win7 and above // TODO: Import dynamically into a separate global struct
     pChangeWindowMessageFilterEx(this->GetOwnerHandle(),GMsgTaskbarCreated, MSGFLT_ALLOW, nullptr);  
    }
  }            
 return res;
}  
//------------------------------------------------------------------------------------------------------------
// NOTE: Use GetMessagePos() or GetCursorPos() on WinXP to get valid coordinates
//
virtual bool WindowProc(HWND& hWnd, UINT& Msg, WPARAM& wParam, LPARAM& lParam, LRESULT& lResult)    // No Effect!!!!!!!!!
{ 
 switch(Msg)
  {
   case WM_MOUSEMOVE:
    if(this->OnMouseMove)(this->GetOwnerWnd()->*OnMouseMove)(this, wParam, lParam);
    break;
   case WM_LBUTTONDOWN:
    if(this->OnMouseBtnDn)(this->GetOwnerWnd()->*OnMouseBtnDn)(this, MK_LBUTTON, MK_LBUTTON, wParam, lParam);
    break;
   case WM_LBUTTONUP:
    if(this->OnMouseBtnUp)(this->GetOwnerWnd()->*OnMouseBtnUp)(this, MK_LBUTTON, 0, wParam, lParam);
    break;       
   case WM_LBUTTONDBLCLK:
    if(this->OnMouseBtnClk)(this->GetOwnerWnd()->*OnMouseBtnClk)(this, MK_LBUTTON, wParam, lParam);
    break;
   case WM_RBUTTONDOWN:
    if(this->OnMouseBtnDn)(this->GetOwnerWnd()->*OnMouseBtnDn)(this, MK_RBUTTON, MK_RBUTTON, wParam, lParam);
    break;
   case WM_RBUTTONUP:
    if(this->OnMouseBtnUp)(this->GetOwnerWnd()->*OnMouseBtnUp)(this, MK_RBUTTON, 0, wParam, lParam);
    break;
   case WM_RBUTTONDBLCLK:
    if(this->OnMouseBtnClk)(this->GetOwnerWnd()->*OnMouseBtnClk)(this, MK_RBUTTON, wParam, lParam);
    break;
   case WM_MBUTTONDOWN:
    if(this->OnMouseBtnDn)(this->GetOwnerWnd()->*OnMouseBtnDn)(this, MK_MBUTTON, MK_MBUTTON, wParam, lParam);
    break;
   case WM_MBUTTONUP:
    if(this->OnMouseBtnUp)(this->GetOwnerWnd()->*OnMouseBtnUp)(this, MK_MBUTTON, 0, wParam, lParam);
    break;
   case WM_MBUTTONDBLCLK:
    if(this->OnMouseBtnClk)(this->GetOwnerWnd()->*OnMouseBtnClk)(this, MK_MBUTTON, wParam, lParam);
    break;
//   case WM_MOUSEWHEEL:      // Not received
//    Sleep(1);
//   break;
   case WM_XBUTTONDOWN:    // Only VK_XBUTTON1 is reported
    if(this->OnMouseBtnDn)(this->GetOwnerWnd()->*OnMouseBtnDn)(this, MK_XBUTTON1, MK_XBUTTON1, wParam, lParam);
    break;
   case WM_XBUTTONUP:      // Only VK_XBUTTON1 is reported
    if(this->OnMouseBtnUp)(this->GetOwnerWnd()->*OnMouseBtnUp)(this, MK_XBUTTON1, 0, wParam, lParam);
    break;
   case WM_XBUTTONDBLCLK:  // Only VK_XBUTTON1 is reported
    if(this->OnMouseBtnClk)(this->GetOwnerWnd()->*OnMouseBtnClk)(this, MK_XBUTTON1, wParam, lParam);
    break;
//   case WM_MOUSEHWHEEL:    // Not received
//    Sleep(1);
//   break;
   case WM_CONTEXTMENU:      // RBTN 
    if(this->OnContextMenu)(this->GetOwnerWnd()->*OnContextMenu)(this, wParam, lParam);
    break;

   case NIN_SELECT:          // WM_USER  // LBTN  (Why?)
   case NIN_KEYSELECT:
    if(this->OnUserLAction)(this->GetOwnerWnd()->*OnUserLAction)(this, wParam, lParam);         
    break;

   case NIN_BALLOONSHOW:     
    if(this->OnBalloonShow)(this->GetOwnerWnd()->*OnBalloonShow)(this);
    break;
   case NIN_BALLOONHIDE:        // Never reported on Win10
    Sleep(0);
    break;
   case NIN_BALLOONTIMEOUT:     // Win10: Sent when user closes the balloon message ('x' button or settings)
    if(this->OnBalloonUsrClose)(this->GetOwnerWnd()->*OnBalloonUsrClose)(this);
    break;
   case NIN_BALLOONUSERCLICK:     
    if(this->OnBalloonUsrClk)(this->GetOwnerWnd()->*OnBalloonUsrClk)(this);
    break;

   case NIN_POPUPOPEN:     // Vista+. Sent only if no NIF_SHOWTIP is specified?   // ProcessHacker uses this
   case NIN_POPUPCLOSE:    // May be usefull for OnMouseExit implementation?
    break;

   default:         // NIN_KEYSELECT + NIN_SELECT - No way to activate tray icon by keyboard in Win10?
     if(Msg == CSWTrayIcon::GMsgTaskbarCreated)   // The icon is lost, we have to inform an user about that. No way to restore it without caching
      {
       this->hTaskBar = nullptr;
       this->Invalid  = true;
       if(this->OnIconLost)(this->GetOwnerWnd()->*OnIconLost)(this);
      }
       else return false;
  } 
 lResult = 0; // Confirm that we processed the message     
 return true;
}
//------------------------------------------------------------------------------------------------------------
// Creates a base icon. hIcon can be null if you want to preload multiple frames and then start assigning them
int CreateIcon(HICON hIcon, LPCWSTR Hint=nullptr, bool Visible=false)
{
 SNIDataW tid = {};
 tid.Data.cbSize = sizeof(SNOTIFYICONDATAW32);
 tid.Data.uVersion = NOTIFYICON_VERSION + 1;     // new format (NTDDI_VERSION >= NTDDI_VISTA)   // 4 will fail on WinXpSP2

 tid.Data.hWnd    = (DWORD)this->GetOwnerHandle();
 tid.Data.uID     = this->GetBaseId();
 tid.Data.uFlags  = NIF_MESSAGE;
 tid.Data.uCallbackMessage = TrayIconMsgBase;
 tid.Data.dwState = NIS_HIDDEN;       // Requires NIF_STATE
 tid.Data.dwStateMask = tid.Data.dwState; 
 tid.Data.hIcon   = (DWORD)hIcon;
 if(hIcon)tid.Data.uFlags |= NIF_ICON;
 if(!Visible)tid.Data.uFlags |= NIF_STATE;
 if(Hint)
  {
   tid.Data.uFlags |= NIF_TIP|NIF_SHOWTIP;   // Normally, when uVersion is set to NOTIFYICON_VERSION_4, the standard tooltip is suppressed and can be replaced by the application-drawn, pop-up UI
   lstrcpynW(tid.Data.szTip, Hint, sizeof(tid.Data.szTip) / sizeof(WCHAR));
  }
 if(!this->CtrlNotifyIcon(NIM_ADD, &tid))return -1;
 if(!this->CtrlNotifyIcon(NIM_SETVERSION, &tid))          // "NIM_SETVERSION must be called every time a notification area icon is added (NIM_ADD)"
  {
   tid.Data.uVersion--; 
   if(!this->CtrlNotifyIcon(NIM_SETVERSION, &tid))return -2; 
  }  
 TrayVer = tid.Data.uVersion;   
 this->Invalid = false;
 this->Hidden  = !Visible;   
 return 0;          // No ICON storing is required here, probably no animation will be needed
}
//------------------------------------------------------------------------------------------------------------
int UpdateIcon(HICON hIcon, LPCWSTR Hint=nullptr, bool ForceShow=false)
{
 if(this->Invalid)return -1;

 SNIDataW tid = {};
 tid.Data.cbSize  = sizeof(SNOTIFYICONDATAW32); 
 tid.Data.hWnd    = (DWORD)this->GetOwnerHandle();
 tid.Data.uID     = this->GetBaseId();    // We address our base icon
 tid.Data.uFlags  = NIF_SHOWTIP;
 if(hIcon)
  {
   tid.Data.uFlags |= NIF_ICON;
   tid.Data.hIcon   = (DWORD)hIcon;  
  }
 if(Hint)
  {
   tid.Data.uFlags |= NIF_TIP;
   lstrcpynW(tid.Data.szTip, Hint, sizeof(tid.Data.szTip) / sizeof(WCHAR));
  }
 if(ForceShow)
  {
   tid.Data.uFlags |= NIF_STATE;
   tid.Data.dwStateMask = NIS_HIDDEN;
  }
 if(!this->CtrlNotifyIcon(NIM_MODIFY, &tid))return -3;
 return 0;
} 
//------------------------------------------------------------------------------------------------------------
// Preloads a new ICON resource into Taskbar
int PreloadIconFrame(HICON hIcon)
{
 if(this->Invalid)return -1;
 if(!hIcon)return -2;

 SNIDataW tid = {};
 UINT SlotIdx = this->FindFreeIconFrameSlot();
 tid.Data.cbSize  = sizeof(SNOTIFYICONDATAW32);
 tid.Data.hWnd    = (DWORD)this->GetOwnerHandle();
 tid.Data.uID     = this->GetBaseId() + SlotIdx + 1;    // Must be unique or CtrlNotifyIcon will fail
 tid.Data.uFlags  = NIF_ICON|NIF_STATE;
 tid.Data.hIcon   = (DWORD)hIcon;      // hIcon is used as ICON ID
 tid.Data.dwState = NIS_HIDDEN;       
 tid.Data.dwStateMask = tid.Data.dwState;
 if(!this->CtrlNotifyIcon(NIM_ADD, &tid))return -4;
 this->StoreIconFrameInfo(SlotIdx, hIcon); 
 return SlotIdx;     // Return the frame index
}
//------------------------------------------------------------------------------------------------------------
// Replaces frame`s icon. Cannot add a new frame but can create it in an empty slot
int UpdateIconFrame(UINT Idx, HICON hIcon)
{
 if(this->Invalid)return -1;
 if(Idx >= this->HIcoCtr)return -2;
 if(!hIcon)return -3;

 SNIDataW tid = {};
 tid.Data.cbSize  = sizeof(SNOTIFYICONDATAW32);
 tid.Data.hWnd    = (DWORD)this->GetOwnerHandle();
 tid.Data.uID     = this->GetBaseId() + Idx + 1;    // Must be unique or CtrlNotifyIcon will fail
 tid.Data.uFlags  = NIF_ICON;
 tid.Data.hIcon   = (DWORD)hIcon;     
 if(!this->HIcoLst[Idx])
  {
   tid.Data.uFlags |= NIF_STATE;
   tid.Data.dwState = NIS_HIDDEN;       
   tid.Data.dwStateMask = tid.Data.dwState;
   if(!this->CtrlNotifyIcon(NIM_ADD, &tid))return -4;
  }
   else
    {
     if(!this->CtrlNotifyIcon(NIM_MODIFY, &tid))return -4;
    }
 this->HIcoLst[Idx] = tid.Data.hIcon;
 return 0; 
}
//------------------------------------------------------------------------------------------------------------
// Removes the icon but keeps its slot to be reused
int RemoveIconFrame(UINT Idx, HICON hIcon)
{
 if(this->Invalid)return -1;
 if(Idx >= this->HIcoCtr)return -2;
 if(!this->HIcoLst[Idx])return -3;   // Already removed
 if(!hIcon)return -4;

 SNIDataW tid = {};
 tid.Data.cbSize = sizeof(SNOTIFYICONDATAW32); 
 tid.Data.hWnd   = (DWORD)this->GetOwnerHandle();
 tid.Data.uID    = this->GetBaseId() + Idx + 1;
 if(!this->CtrlNotifyIcon(NIM_DELETE, &tid))return -5;
 this->HIcoLst[Idx] = 0;
 return 0; 
}
//------------------------------------------------------------------------------------------------------------
// Does nothing if base icon is hidden
int SelectIconFrame(UINT Idx, bool ForceShow=false)
{
 if(Idx >= this->HIcoCtr)return -1;
 if(!this->HIcoLst[Idx])return -2;   // Empty
 if(this->Hidden && !ForceShow)return 0;    // Don`t even try

 SNIDataW tid = {};
 tid.Data.cbSize  = sizeof(SNOTIFYICONDATAW32); 
 tid.Data.hWnd    = (DWORD)this->GetOwnerHandle();
 tid.Data.uID     = this->GetBaseId();    // We address our base icon
 tid.Data.uFlags  = NIF_ICON|NIF_STATE|NIF_SHOWTIP;
 tid.Data.hIcon   = this->HIcoLst[Idx];   // It is used as ID, doesn`t matter if this HICON is invalid already
 tid.Data.dwState = NIS_SHAREDICON;       // Use a preloaded ICON
 tid.Data.dwStateMask = NIS_SHAREDICON; 
 if(ForceShow)tid.Data.dwStateMask |= NIS_HIDDEN;
 if(!this->CtrlNotifyIcon(NIM_MODIFY, &tid))return -3;
 if(ForceShow)this->Hidden = false;
 return 0;
}
//------------------------------------------------------------------------------------------------------------
// Removes the Icon, including any preloaded frames
int RemoveIcon(void)
{
 SNIDataW tid = {};
 UINT Total = 0;
 tid.Data.cbSize = sizeof(SNOTIFYICONDATAW32); 
 tid.Data.hWnd   = (DWORD)this->GetOwnerHandle();
 for(UINT Idx=0;Idx < this->HIcoCtr;Idx++)     // Delete preloaded frames
  {
   if(!this->HIcoLst[Idx])continue;
   tid.Data.uID = this->GetBaseId() + Idx + 1;  
   if(this->CtrlNotifyIcon(NIM_DELETE, &tid))Total++;
  }
 if(this->HIcoLst){HeapFree(GetProcessHeap(),0,this->HIcoLst); this->HIcoLst=nullptr;}
 this->HIcoCtr = 0;
 tid.Data.uID  = this->GetBaseId();
 if(!this->CtrlNotifyIcon(NIM_DELETE, &tid))return -1;    // Delete base icon
 return Total;
}
//------------------------------------------------------------------------------------------------------------
// To remove the message set Msg to an empty string
// Flags: 0 or NIIF_INFO, NIIF_WARNING, NIIF_ERROR, NIIF_USER (WXPSP2), NIIF_NOSOUND (WXP), NIIF_LARGE_ICON (VISTA) 
//
// Balloons may be disabled in user`s system. To enable:
// [HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Explorer\Advanced]
//   "EnableBalloonTips"=dword:00000001
//
int ShowBalloonMsg(LPCWSTR Msg, LPCWSTR Title, DWORD Flags, HICON hIcon=nullptr, UINT Timeout=6000, bool ForceShow=false)
{
 SNIDataW tid = {};
 tid.Data.cbSize = sizeof(SNOTIFYICONDATAW32); 
 tid.Data.hWnd   = (DWORD)this->GetOwnerHandle();
 tid.Data.uID    = this->GetBaseId();
 tid.Data.uFlags = NIF_INFO|NIF_SHOWTIP;      // NIF_SHOWTIP must be always specified for base icon or its tip will become invisible
 tid.Data.dwInfoFlags = Flags;
 if(ForceShow)
  {
   tid.Data.uFlags |= NIF_STATE;
   tid.Data.dwStateMask = NIS_HIDDEN;
  } 
 if(Msg)lstrcpynW(tid.Data.szInfo, Msg, sizeof(tid.Data.szInfo) / sizeof(WCHAR));
 if(Title)lstrcpynW(tid.Data.szInfoTitle, Title, sizeof(tid.Data.szInfoTitle) / sizeof(WCHAR));
 if(hIcon)tid.Data.hBalloonIcon = tid.Data.hIcon = (DWORD)hIcon;
 if(!this->CtrlNotifyIcon(NIM_MODIFY, &tid))return -1;
 return 0;
}
//------------------------------------------------------------------------------------------------------------
virtual bool Show(bool Show=true)
{ 
 SNIDataW tid = {};
 tid.Data.cbSize  = sizeof(SNOTIFYICONDATAW32); 
 tid.Data.hWnd    = (DWORD)this->GetOwnerHandle();
 tid.Data.uID     = this->GetBaseId();
 tid.Data.uFlags  = NIF_STATE|NIF_SHOWTIP;      // NIF_SHOWTIP must be always specified for base icon or its tip will become invisible
 tid.Data.dwState = (!Show)?NIS_HIDDEN:0;       // Requires NIF_STATE
 tid.Data.dwStateMask = NIS_HIDDEN; 
 if(!this->CtrlNotifyIcon(NIM_MODIFY, &tid))return false;
 this->Hidden = !Show;
 return true;
}     
//------------------------------------------------------------------------------------------------------------

};