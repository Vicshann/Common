
#pragma once

#include <windows.h>
#include <Windowsx.h>
#include <Commctrl.h>

#include "CompileTime.hpp"     

//==========================================================================================================================
// NOTE: No multithreading yet!

// Windows only, GUI only, Header only library

/*
 BEHAVIOUR notes:
  When a window moved, it may affect dimentions of its siblings if some alignment are used and its own
  When a window resize, its children and sibling also may be resized



 TODO: 'Stick to edge' feature. Set child`s stick to left and no stick to right and when a window resized, child is moved to left with it 
       Set child`s stick to right and no stick to left and when a window resized, child is stayed at right position
       Set child`s stick to left and stick to right and when a window resized, child is resized with it horizontally 

WM_SETFONT

TODO: Event notify: local(The control only),Parent(Also a parent control), Form(Only an owning form(First control in hierarhy)), Global(All controls on the form(But not the form itself?))
------------------------------------------------------------------
http://www.mctrl.org/about.php

https://www.codeproject.com/Articles/1042516/%2FArticles%2F1042516%2FCustom-Controls-in-Win-API-Scrolling

About Window Classes: https://msdn.microsoft.com/ru-ru/library/windows/desktop/ms633574%28v=vs.85%29.aspx#system 

Buttons: https://msdn.microsoft.com/ru-ru/library/windows/desktop/bb775943(v=vs.85).aspx
Button Types: https://msdn.microsoft.com/ru-ru/library/windows/desktop/bb775947%28v=vs.85%29.aspx
Button Styles: https://msdn.microsoft.com/ru-ru/library/windows/desktop/bb775951%28v=vs.85%29.aspx


Combo Boxes: https://msdn.microsoft.com/ru-ru/library/windows/desktop/bb775792(v=vs.85).aspx
Combo Box Styles: https://msdn.microsoft.com/ru-ru/library/windows/desktop/bb775796%28v=vs.85%29.aspx

Edit Controls: https://msdn.microsoft.com/ru-ru/library/windows/desktop/bb775458(v=vs.85).aspx
Edit Control Styles: https://msdn.microsoft.com/ru-ru/library/windows/desktop/bb775464(v=vs.85).aspx

List Boxes: https://msdn.microsoft.com/ru-ru/library/windows/desktop/bb775146(v=vs.85).aspx
List Box Styles: https://msdn.microsoft.com/ru-ru/library/windows/desktop/bb775149(v=vs.85).aspx

Rich Edit Controls: https://msdn.microsoft.com/ru-ru/library/windows/desktop/bb787605(v=vs.85).aspx
                    https://msdn.microsoft.com/ru-ru/library/windows/desktop/bb787605(v=vs.85).aspx     
Rich Edit Control Styles: https://msdn.microsoft.com/ru-ru/library/windows/desktop/bb774367(v=vs.85).aspx
                          https://msdn.microsoft.com/ru-ru/library/windows/desktop/bb774367(v=vs.85).aspx

Scroll Bars: https://msdn.microsoft.com/ru-ru/library/windows/desktop/bb787529(v=vs.85).aspx
Static Control Styles: https://msdn.microsoft.com/ru-ru/library/windows/desktop/bb760773(v=vs.85).aspx

Using Toolbar Controls: https://msdn.microsoft.com/ru-ru/library/windows/desktop/bb760446%28v=vs.85%29.aspx
Toolbar Control and Button Styles: https://msdn.microsoft.com/ru-ru/library/windows/desktop/bb760439%28v=vs.85%29.aspx 
                                   https://msdn.microsoft.com/ru-ru/library/windows/desktop/bb775452%28v=vs.85%29.aspx
About Rebar Controls: https://msdn.microsoft.com/ru-ru/library/windows/desktop/bb774373%28v=vs.85%29.aspx
                      https://msdn.microsoft.com/ru-ru/library/windows/desktop/hh298391%28v=vs.85%29.aspx

INITCOMMONCONTROLSEX: https://msdn.microsoft.com/en-us/library/windows/desktop/bb775507%28v=vs.85%29.aspx
--------------------------------------------------------------
- the MulDiv trick allows you to use point size, rather than logical units, to specify your font size. 
            const long nFontSize = 10;

            HDC hdc = GetDC(hWnd);

            LOGFONT logFont = {0};
            logFont.lfHeight = -MulDiv(nFontSize, GetDeviceCaps(hdc, LOGPIXELSY), 72);
            logFont.lfWeight = FW_BOLD;
            _tcscpy_s(logFont.lfFaceName, fontName);

            s_hFont = CreateFontIndirect(&logFont);

            ReleaseDC(hWnd, hdc);
SendMessage(s_hWndButton, WM_SETFONT, (WPARAM)s_hFont, (LPARAM)MAKELONG(TRUE, 0));
*/

#if defined(_AMD64_)
#define WLP_WNDPROC   GWLP_WNDPROC
#define WLP_USERDATA  GWLP_USERDATA
#else
#define WLP_WNDPROC   GWL_WNDPROC
#define WLP_USERDATA  GWL_USERDATA
#endif

//#ifndef STXT            // Secured text ?  // Derived type check reveals all anyway
//#define STXT(val) val
//#endif

enum EWNotify {wnChildren=1, wnDeepChld=2, wnDeepPrnt=4};
enum EEdgeSnap {esLeft=1, esRight=2, esTop=4, esBottom=8};

struct SWDim   // TODO: Rework to support anchoring
{
 int PosX; 
 int PosY; 
 int Width; 
 int Height;
};
//==========================================================================================================================
//                                            BASE OBJECT CLASS
//==========================================================================================================================
class CObjBase
{
protected:
 char*      TypeName;
 UINT       ChildCnt  = 0;
 PVOID      pUserData = nullptr;
 HMODULE    hOwnerMod = nullptr;   // A module that owns this object
 CObjBase*  OwnerObj  = nullptr; 
 CObjBase** ChildLst  = nullptr;

//------------------------------------------------------------------------------------------------------------
void SetHInstance(void)
{
 auto Ptr = &CObjBase::SetHInstance; 
 GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, *(LPCSTR*)&Ptr, &this->hOwnerMod);  // Try to make a current module an owner
 if(!this->hOwnerMod)this->hOwnerMod = GetModuleHandleA(NULL);  // Make the EXE module an owner
}
//------------------------------------------------------------------------------------------------------------
// Resizes child components list if there is no a free one
void InsertChildObj(CObjBase* Obj)
{ 
 if(this->ChildLst)
  {
   for(UINT ctr=0;ctr < this->ChildCnt;ctr++)
    { 
     if(!this->ChildLst[ctr])
      {
       this->ChildLst[ctr] = Obj;    // TODO: Add functions to control order of children controls?
       return;   // An empty child slot found
      }
    }
   this->ChildLst = (CObjBase**)HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, this->ChildLst, (++this->ChildCnt * sizeof(CObjBase*)));  
  }
   else { this->ChildLst = (CObjBase**)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(CObjBase*)); this->ChildCnt = 1; }   // First child 
 this->ChildLst[this->ChildCnt-1] = Obj;   // A new slot created
}
//------------------------------------------------------------------------------------------------------------

public:
//------------------------------------------------------------------------------------------------------------
static const char* TypeId(void){static const char* Type = __func__; return Type;}    // TODO: Use compile time hash of this string
bool IsDerivedFrom(const char* FromId)
{
 return true;
}
//------------------------------------------------------------------------------------------------------------
HMODULE GetHInstance(void)
{
 if(!this->hOwnerMod)this->SetHInstance();
 return this->hOwnerMod;
}
//------------------------------------------------------------------------------------------------------------
void* operator new(size_t Size){return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, Size);}
void  operator delete(void* Ptr) { HeapFree(GetProcessHeap(), 0, Ptr); }
//------------------------------------------------------------------------------------------------------------
template<typename T> static void DeleteCtrl(T& pCtrl){auto Ptr=pCtrl; pCtrl=nullptr; delete(Ptr);}
template<typename P, typename C> static void SetCallback(P& Ptr, C Callback){Ptr = static_cast<P>(Callback);}  
template<class T, typename... Args> T* AddObj(Args&&... args)     // NOTE: copy of this method`s body is created for every instance!   // Move allocator for children to a separate proc and NEW to a controls themselves?          //TODO: Order of visual objects affects their TabStop?
{
 T* Ctrl = new T();
 Ctrl->OwnerObj  = this;            // Must be done BEFORE call to 'create' method   
 Ctrl->hOwnerMod = this->hOwnerMod;    
 if(Ctrl->Create(args...) < 0){ delete(Ctrl); return nullptr; }
 this->InsertChildObj(Ctrl);
 return Ctrl;
}
//------------------------------------------------------------------------------------------------------------
// DelObj
//------------------------------------------------------------------------------------------------------------
CObjBase(void){ }
//------------------------------------------------------------------------------------------------------------
virtual ~CObjBase()
{
 if(this->ChildCnt && this->ChildLst) 
  {
   for(UINT ctr=0;ctr < this->ChildCnt;ctr++){if(this->ChildLst[ctr])delete(this->ChildLst[ctr]);}
   delete(this->ChildLst);        //HeapFree(GetProcessHeap(), 0, this->ChildLst);
  }
}
//------------------------------------------------------------------------------------------------------------

};

//==========================================================================================================================
//                                               BASE WINDOW CLASS
//==========================================================================================================================
class CWndBase: public CObjBase     
{
protected:
 bool       bClosed     = false;
 HWND       hWindow     = nullptr;   // Handle to this window control
 ATOM       WndClass    = 0;         // NULL for a system windows
 WNDPROC    OrigWndProc = DefWindowProcA;

CWndBase* GetOwnerWnd(void){return reinterpret_cast<CWndBase*>(this->OwnerObj);}
//------------------------------------------------------------------------------------------------------------
static LRESULT CALLBACK WndProxyProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
 CWndBase* This = (CWndBase*)GetWindowLongPtrA(hWnd, WLP_USERDATA);    // Or use a window`s extra bytes instead?
 if(!This && ((Msg == WM_NCCREATE)||(Msg == WM_CREATE)))     // WM_NCCREATE then WM_CREATE or only one of them
  {
   This = (CWndBase*)((CREATESTRUCT*)lParam)->lpCreateParams;
   UINT  ClsExLen = GetClassLongPtrA(hWnd, GCL_CBCLSEXTRA); 
   PVOID OProc    = (PVOID)GetClassLongPtrA(hWnd, ClsExLen-sizeof(PVOID));   
   if(!OProc)SetClassLongPtrA(hWnd,ClsExLen-sizeof(PVOID),(ULONG_PTR)This->OrigWndProc);   // Assign original WndProc to the class
     else This->OrigWndProc = (WNDPROC)OProc;             
   SetWindowLongPtrA(hWnd, WLP_USERDATA, (ULONG_PTR)This);    // What about MDI windows? 
   This->hWindow = hWnd;   // This assignment duplicated in CreateWnd after CreateWindowEx returns
  } 
 if(This)
  {
   LRESULT Res = 0;
   if(This->WindowProc(hWnd, Msg, wParam, lParam, Res))return Res;
  }
 return DefWindowProcA(hWnd, Msg, wParam, lParam);
}
//------------------------------------------------------------------------------------------------------------
// Used to superclass system controls classes
//
ATOM SuperClassSysCtrl(LPSTR OrigClassName, LPSTR ClassName, UINT ExStyles=0)  // hInstance is NULL to superclass a system controls
{
 WNDCLASSEXA wcls;  
 WNDCLASSEXA ewcls; 

 wcls.cbSize   = sizeof(WNDCLASSEX);
 ewcls.cbSize  = sizeof(WNDCLASSEX);
 if(!GetClassInfoExA(NULL,OrigClassName,&wcls))return 0;     // Wrong system class?
 if(!this->hOwnerMod)this->SetHInstance();   
 if(ATOM MyClass  = GetClassInfoExA(this->hOwnerMod,ClassName,&ewcls))   // Returns an ATOM cast to BOOL as said on MSDN     // CLASS is already registered
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
 this->WndClass     = RegisterClassExA(&wcls);
 return this->WndClass;
}
//------------------------------------------------------------------------------------------------------------
HWND CreateWnd(LPCSTR ClassName, LPCSTR WindowName, DWORD Style, DWORD ExStyle, int PosX, int PosY, int Width, int Height, HWND hParentWnd=NULL)
{      
 if(!this->hOwnerMod)this->SetHInstance();
 if(this->OwnerObj)hParentWnd = this->GetOwnerWnd()->hWindow;   // Overrides
 if(this->hWindow = CreateWindowExA(ExStyle, ClassName, WindowName, Style, PosX, PosY, Width, Height, hParentWnd, 0, this->hOwnerMod, this))     // 'this' is passed as LPARAM but not used in WM_CREATE message because WNDPROC is not subclassed yet
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
   if(!this->ChildLst[ctr]->IsDerivedFrom(CWndBase::TypeId()))continue;  // Skip non windowed components
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
 if(!this->OwnerObj->IsDerivedFrom(CWndBase::TypeId()))return;
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
CWndBase(void){ }
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
HWND GetOwnerHandle(void){return (this->OwnerObj)?(((CWndBase*)this->OwnerObj)->hWindow):(NULL);}
//------------------------------------------------------------------------------------------------------------
virtual ~CWndBase()    // A thread cannot use DestroyWindow to destroy a window created by a different thread. 
{                      // 'DestroyWindow' may fail here because all child windows is already deleted by parent`s 'DestroyWindow' call
 if(this->hWindow)DestroyWindow(this->hWindow);    // If the specified window is a parent or owner window, DestroyWindow automatically destroys the associated child or owned windows when it destroys the parent or owner window.
 if(this->WndClass)UnregisterClassA((LPCSTR)this->WndClass, this->hOwnerMod);   // Restore original WindowProc before this?   // Will work only if there is no windows left of that class
}  
//------------------------------------------------------------------------------------------------------------
virtual bool WindowProc(HWND& hWnd, UINT& Msg, WPARAM& wParam, LPARAM& lParam, LRESULT& lResult)      // Returns TRUE if the message processed
{
 switch(Msg)
 {
   case WM_CLOSE:
     if(this->OnClose)(this->*OnClose)(this);   // If called after CallWindowProcA then all children controls are already destroyed and it is impossible to read anything from them in OnClose handler  
     break;
//  case WM_SETCURSOR:
//    break;
 }

 lResult = CallWindowProcA(this->OrigWndProc, hWnd, Msg, wParam, lParam);   // Process on original WindowProc
 switch(Msg)
  {
//   case WM_CLOSE:
//     if(this->OnClose)(this->*OnClose)(this);     
//     break;

   case WM_VSCROLL:  // Hello! We need these in a child control handler, not here!
   case WM_HSCROLL:
    if(lParam)       // Pass it down to a child Scroll Bar    // TODO: Pass other scroll bar messages back to it
     {
      if(CWndBase* CldThis = (CWndBase*)GetWindowLongPtrA((HWND)lParam, WLP_USERDATA)) 
       {
        hWnd   = CldThis->GetHandle();
        lParam = NULL;        // Prevents this message from looping WindowProc
        CldThis->WindowProc(hWnd, Msg, wParam, lParam, lResult);
       }
     }
    break;

   case WM_SIZE:       // WM_SIZING later
    if(this->OnResize)(this->*OnResize)(this, wParam, lParam & 0xFFFF, lParam >> 16);
    break;

   case WM_PAINT:         // Never received for main windows! Only for controls?
   case WM_ERASEBKGND:    // If you set the class background brush to NULL, however, the system sends a WM_ERASEBKGND message to your window procedure whenever the window background must be drawn, letting you draw a custom background.
    if(this->OnPaint)(this->*OnPaint)(this);
    break;

   case WM_XBUTTONDOWN:
    if(this->OnMouseBtnDn)(this->*OnMouseBtnDn)(this, ((wParam >> 16) & XBUTTON1)?(MK_XBUTTON1):(MK_XBUTTON2), wParam & 0xFFFF, lParam & 0xFFFF, lParam >> 16);
    break;
   case WM_MBUTTONDOWN:
    if(this->OnMouseBtnDn)(this->*OnMouseBtnDn)(this, MK_MBUTTON, wParam & 0xFFFF, lParam & 0xFFFF, lParam >> 16);
    break;
   case WM_RBUTTONDOWN:
    if(this->OnMouseBtnDn)(this->*OnMouseBtnDn)(this, MK_RBUTTON, wParam & 0xFFFF, lParam & 0xFFFF, lParam >> 16);
    break;
   case WM_LBUTTONDOWN:
    if(this->OnMouseBtnDn)(this->*OnMouseBtnDn)(this, MK_LBUTTON, wParam & 0xFFFF, lParam & 0xFFFF, lParam >> 16);
    break;

   case WM_KEYDOWN:
   case WM_SYSKEYDOWN:   // For F10
    if(this->OnKeyDn)(this->*OnKeyDn)(this, wParam, lParam & 0xFFFF, (lParam >> 16) & 0xFF, lParam & (1 << 24), lParam & (1 << 30));
    break;
  }
 return true;
}      
//------------------------------------------------------------------------------------------------------------
// NOTE: Only system window classes support these by default (All of them?)
virtual HFONT GetFont(void){return (HFONT)SendMessageA(this->hWindow, WM_GETFONT, NULL, NULL);}
virtual void  SetFont(HFONT Font, bool Redraw=false){SendMessageA(this->hWindow, WM_SETFONT, (LPARAM)Font, Redraw);}
//------------------------------------------------------------------------------------------------------------
virtual BOOL  Show(bool Show){return ShowWindow(this->hWindow, (Show)?(SW_SHOW):(SW_HIDE));}
//------------------------------------------------------------------------------------------------------------
/*virtual HCURSOR SetCursor(HCURSOR cur)
{
 HCURSOR res = (HCURSOR)SetClassLongPtrW(this->hWindow,GCLP_HCURSOR,(LONG_PTR)cur);   // No effect and a bad idea to to it globally
 SendMessageW(this->hWindow, WM_SETCURSOR, 0, (LPARAM)cur);
 return res;
} */
//------------------------------------------------------------------------------------------------------------
int GetTextLen(void)
{       
 return SendMessageA(this->GetHandle(),WM_GETTEXTLENGTH,0,0);
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
 if(len > Size)len = len;
 if(sizeof(*Buffer) == sizeof(wchar_t))return SendMessageW(this->GetHandle(),WM_GETTEXT,len+1,(LPARAM)Buffer);    // No need for specialization because it will still fail if you pass a wrong buffer type 
  else if(sizeof(*Buffer) == sizeof(char))return SendMessageA(this->GetHandle(),WM_GETTEXT,len+1,(LPARAM)Buffer);
 return -1;
}
//------------------------------------------------------------------------------------------------------------

};
//==========================================================================================================================
//                                               WINDOW FORM CLASS
//==========================================================================================================================
// TODO: Unicode flag (RegisterClassExW) // Affects child sys windows?
class CWndForm: public CWndBase    // TODO: Partial update of controls, do not redraw all
{
 HFONT Font     = nullptr;   // Custom class windows 
//------------------------------------------------------------------------------------------------------------

public:
//------------------------------------------------------------------------------------------------------------
CWndForm(void){ }
//------------------------------------------------------------------------------------------------------------
int Create(LPCSTR WndName, SWDim& Wdim, HWND hParentWnd, DWORD Style, DWORD ExStyle, HICON hWndIcon=LoadIconA(NULL, IDI_APPLICATION), HBRUSH hBgrBrush=GetSysColorBrush(COLOR_BTNFACE))
{
 WNDCLASSEX wcls; 
 char WCName[256];

 lstrcpyA(&WCName[1], "CSWForm");
 WCName[0] = (hBgrBrush)?('S'):('N'); 
 if(!this->hOwnerMod)this->SetHInstance();
 if(!(this->WndClass = GetClassInfoExA(this->hOwnerMod,(LPSTR)&WCName,&wcls)))  // Returns an ATOM cast to BOOL as said on MSDN     // CLASS is already registered  // UnregisterClassA on it will fail unless there is no windows of this class left
  {
   wcls.cbSize        = sizeof(WNDCLASSEX);
   wcls.style         = CS_OWNDC|CS_HREDRAW|CS_VREDRAW|CS_GLOBALCLASS;
   wcls.cbClsExtra    = sizeof(PVOID);   // To store DefWindowProc
   wcls.cbWndExtra    = 0;
   wcls.hInstance     = this->hOwnerMod; 
   wcls.hIcon         = hWndIcon;                 
   wcls.hCursor       = LoadCursorA(0, (LPCSTR)IDC_ARROW);
   wcls.hbrBackground = hBgrBrush;//GetSysColorBrush(COLOR_BTNFACE); // (HBRUSH)(COLOR_BACKGROUND+1);  
   wcls.lpszClassName = (LPSTR)&WCName;
   wcls.lpszMenuName  = 0;
   wcls.hIconSm       = 0;
   wcls.lpfnWndProc   = (WNDPROC)&CWndBase::WndProxyProc;   // Belongs to this window CLASS  // WM_CREATE is passed to class`s proc because only there it is specified before a window created
   this->WndClass     = RegisterClassExA(&wcls);
   if(!this->WndClass)return -1;
  }
 return (this->CreateWnd((LPCSTR)this->WndClass, WndName, Style, ExStyle, Wdim.PosX, Wdim.PosY, Wdim.Width, Wdim.Height, hParentWnd))?(0):(-2);
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
    if((bool)lParam){InvalidateRect(hWnd, NULL, TRUE); lResult = CallWindowProcA(this->OrigWndProc, hWnd, WM_PAINT, 0, 0);}
    break;
  }        
 return true;  
}
//------------------------------------------------------------------------------------------------------------
};
//==========================================================================================================================
//                                           BASE CHILD WINDOW CLASS   (Subject to layout rules)
//==========================================================================================================================
class CCldBase: public CWndBase     // Parent window must notify its children when resized!!!   // Or any child, when moved or resized
{
 BYTE ESnap;
 RECT SnapDist;

//------------------------------------------------------------------------------------------------------------
void ResetSnapByParentRect(RECT* prec, RECT* crec)
{
 int pwidth  = prec->left   - prec->right;
 int pheight = prec->bottom - prec->top;
 this->SnapDist.top    = crec->top;
 this->SnapDist.left   = crec->left;
 this->SnapDist.right  = pwidth  - crec->right;
 this->SnapDist.bottom = pheight - crec->bottom;
}
//------------------------------------------------------------------------------------------------------------
void UpdateSnapByParentRect(RECT* prec)
{
/*
      int pwidth  = prec.left   - prec.right;
      int pheight = prec.bottom - prec.top;

*/
}
//------------------------------------------------------------------------------------------------------------

public:
CCldBase(void){ }
//------------------------------------------------------------------------------------------------------------
virtual bool WindowProc(HWND& hWnd, UINT& Msg, WPARAM& wParam, LPARAM& lParam, LRESULT& lResult)
{
 CWndBase::WindowProc(hWnd, Msg, wParam, lParam, lResult);  // Let base class to process the messages before we extend them
 switch(Msg)
  {
   case WM_CREATE:
    {
     RECT prec, crec;
     CREATESTRUCT* cs = (CREATESTRUCT*)lParam;
     GetClientRect(this->GetOwnerHandle(), &prec);   // Take client area of parent
     crec.top    = cs->y;
     crec.left   = cs->x;
     crec.right  = crec.left + cs->cx;
     crec.bottom = crec.top  + cs->cy;
     this->ResetSnapByParentRect(&prec, &crec);
    }
    break;
   case WM_SIZE:
    if(hWnd == this->GetOwnerHandle())  // Parent window resized
     {
      RECT prec;
      GetClientRect(hWnd, &prec);
      this->UpdateSnapByParentRect(&prec);    // WM_SIZE will be sent if this component resized?
     }
    else if(hWnd == this->GetHandle())  // Resized itself - keep it snapped
     {
      //
      // Update snaps
      //
      this->NotifyChildren(hWnd, Msg, wParam, lParam);
     }
    break;
  }
 return true;
}
//------------------------------------------------------------------------------------------------------------

};

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
int Create(LPCSTR EdText, SWDim& Wdim, DWORD Style, DWORD ExStyle)
{        
 if(!this->SuperClassSysCtrl("STATIC", "CSWStatic"))return -1;
 return (this->CreateWnd((LPCSTR)this->WndClass, EdText, Style|WS_CHILD, ExStyle, Wdim.PosX, Wdim.PosY, Wdim.Width, Wdim.Height, NULL))?(0):(-2);    // WS_EX_CLIENTEDGE  WS_EX_WINDOWEDGE
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
CSWEdit(void){ }
//------------------------------------------------------------------------------------------------------------
int Create(LPCSTR EdText, SWDim& Wdim, DWORD Style, DWORD ExStyle)
{        
 if(!this->SuperClassSysCtrl("EDIT", "CSWEdit"))return -1;
 return (this->CreateWnd((LPCSTR)this->WndClass, EdText, Style|WS_CHILD, ExStyle, Wdim.PosX, Wdim.PosY, Wdim.Width, Wdim.Height, NULL))?(0):(-2);    // WS_EX_CLIENTEDGE  WS_EX_WINDOWEDGE
}
//------------------------------------------------------------------------------------------------------------
virtual bool WindowProc(HWND& hWnd, UINT& Msg, WPARAM& wParam, LPARAM& lParam, LRESULT& lResult)
{  
 CCldBase::WindowProc(hWnd, Msg, wParam, lParam, lResult);   
// LRESULT Result = 0;
// bool    NoOrig = false;
 switch(Msg)
  {
   case WM_CHAR:
//    if(this->OnMouseBtnUp)(this->GetOwnerWnd()->*OnMouseBtnUp)(this, Msg, wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
    Sleep(1);
    break;
  } 
// if(NoOrig)return Result;        
 return true;
}
//------------------------------------------------------------------------------------------------------------


};
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
int Create(LPCSTR BtnName, SWDim& Wdim, DWORD Style, DWORD ExStyle)  // Separate from constructor to allow some additional configuration
{ 
// DWORD Style = WS_CHILD|BS_CENTER|BS_DEFPUSHBUTTON; 
 if(!this->SuperClassSysCtrl("BUTTON", "CSWButton"))return -1;
 return (this->CreateWnd((LPCSTR)this->WndClass, BtnName, Style|WS_CHILD, ExStyle, Wdim.PosX, Wdim.PosY, Wdim.Width, Wdim.Height, 0))?(0):(-2);
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
 int res = SendMessageA(this->GetHandle(),BM_GETCHECK,0,0);
 if(res == BST_CHECKED)return 1;
 if(res == BST_UNCHECKED)return 0;
 return -1;
}
//------------------------------------------------------------------------------------------------------------
int GetState(void)
{
 return SendMessageA(this->GetHandle(),BM_GETSTATE,0,0);
}
//------------------------------------------------------------------------------------------------------------
void SetChecked(int Chk)
{
 int Val = BST_INDETERMINATE;
 if(Chk > 0)Val = BST_CHECKED;
  else if(Chk == 0)Val = BST_UNCHECKED;
 SendMessageA(this->GetHandle(),BM_SETCHECK,Val,0);
}
//------------------------------------------------------------------------------------------------------------

};
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
int Create(SWDim& Wdim, DWORD Style, bool AutoUpd=true)
{
 this->AUpd = AutoUpd;
 if(!this->SuperClassSysCtrl("SCROLLBAR", "CSWScrBar"))return -1;
 return (this->CreateWnd((LPCSTR)this->WndClass, "", Style|WS_CHILD, 0, Wdim.PosX, Wdim.PosY, Wdim.Width, Wdim.Height, NULL))?(0):(-2);
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
//==========================================================================================================================
//                                                 COMBO BOX CLASS
//==========================================================================================================================
/*
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
int Create(LPCSTR Text, SWDim& Wdim, DWORD Style, DWORD ExStyle)  // Separate from constructor to allow some additional configuration
{                     
 if(!this->SuperClassSysCtrl("ComboBox", "CSWComboBox"))return -1;
 return (this->CreateWnd((LPCSTR)this->WndClass, Text, Style|WS_CHILD, ExStyle, Wdim.PosX, Wdim.PosY, Wdim.Width, Wdim.Height, 0))?(0):(-2);
}           
//------------------------------------------------------------------------------------------------------------
virtual bool WindowProc(HWND& hWnd, UINT& Msg, WPARAM& wParam, LPARAM& lParam, LRESULT& lResult)    // No Effect!!!!!!!!!
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
}
//------------------------------------------------------------------------------------------------------------
};
//==========================================================================================================================
//                                          DATE TIME PICKER CLASS
//==========================================================================================================================
class CSWDTPicker: public CCldBase
{
public:
// void (_fastcall CWndBase::*OnMouseBtnUp)(CWndBase* Sender, WORD WMsg, WORD KeyEx, int x, int y) = nullptr;    // If a callback returns true then no need to pass this message next to original WindowProc

CSWDTPicker(void){ }
//------------------------------------------------------------------------------------------------------------
int Create(LPCSTR Text, SWDim& Wdim, DWORD Style, DWORD ExStyle)  // Separate from constructor to allow some additional configuration
{                     
 if(!this->SuperClassSysCtrl("SysDateTimePick32", "CSWDTPicker"))return -1;
 return (this->CreateWnd((LPCSTR)this->WndClass, Text, Style|WS_CHILD, ExStyle, Wdim.PosX, Wdim.PosY, Wdim.Width, Wdim.Height, 0))?(0):(-2);
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
 int res = SendMessageA(this->GetHandle(),DTM_GETSYSTEMTIME,0,(LPARAM)stime);
 if(GDT_VALID == res)return 1;
 if(GDT_NONE == res)return 0;
 return -1;
}
//------------------------------------------------------------------------------------------------------------

};
//==========================================================================================================================






/*
HWND CreateSimpleToolbar(HWND hWndParent)
{
    const int ImageListID = 0;
    const int numButtons = 3;
    const DWORD buttonStyles = BTNS_AUTOSIZE;

    HWND hWndToolbar = CreateWindowEx(0x80, TOOLBARCLASSNAME, NULL, WS_CHILD | TBSTYLE_FLAT | WS_BORDER, 0, 0, 0, 0, hWndParent, NULL, hInst, NULL);
    if (hWndToolbar == NULL)
        return NULL;

    HIMAGELIST hImageList = ImageList_Create(75, 66, ILC_COLOR, numButtons, 0);

    BITMAP BNew = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_NEW));
    HBITMAP BOpen = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_OPEN));
    HBITMAP BSave = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_SAVE));
    ImageList_Add(hImageList, BNew, NULL);
    ImageList_Add(hImageList, BOpen, NULL);
    ImageList_Add(hImageList, BSave, NULL);
    SendMessage(hWndToolbar, TB_SETHOTIMAGELIST, 0, (LPARAM)hImageList);

    TBBUTTON tbButtons[numButtons] =
    {
        { 0, IDM_NEW, TBSTATE_ENABLED,
        buttonStyles, {TBSTYLE_BUTTON}, 0, (INT_PTR)L"Новый" },
        { 1, IDM_OPEN, TBSTATE_ENABLED,
        buttonStyles, {TBSTYLE_BUTTON}, 0, (INT_PTR)L"Открыть"},
        { 2, IDM_SAVE, TBSTATE_ENABLED,
        buttonStyles, {TBSTYLE_BUTTON}, 0, (INT_PTR)L"Сохранить"}
    };

    SendMessage(hWndToolbar, TB_SETBITMAPSIZE, 0, MAKELONG(75, 66));
    SendMessage(hWndToolbar, TB_SETBUTTONSIZE, 0, MAKELONG(75, 66));
    SendMessage(hWndToolbar, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);
    SendMessage(hWndToolbar, TB_ADDBUTTONS, (WPARAM)numButtons, (LPARAM)&tbButtons);

    SendMessage(hWndToolbar, TB_AUTOSIZE, 0, 0);
    ShowWindow(hWndToolbar, TRUE);
    return hWndToolbar;
}



*/

















//---------------------------------------------------------------------------

