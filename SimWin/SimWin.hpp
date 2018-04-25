
#pragma once

#include <windows.h>
#include <Windowsx.h>
#include <Commctrl.h>

//==========================================================================================================================
// NOTE: No multithreading yet!

/*
 BEHAVIOUR notes:
  When a window moved, it may affect dimentions of its siblings if some alignment are used and its own
  When a window resize, its children and sibling also may be resized



 TODO: 'Stick to edge' feature. Set child`s stick to left and no stick to right and when a window resized, child is moved to left with it 
       Set child`s stick to right and no stick to left and when a window resized, child is stayed at right position
       Set child`s stick to left and stick to right and when a window resized, child is resized with it horizontally 

WM_SETFONT

------------------------------------------------------------------
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

struct SWDim
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
 GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, *reinterpret_cast<LPCSTR*>(&Ptr), &this->hOwnerMod);  // Try to make a current module an owner
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
void* operator new(size_t Size){return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, Size);}
void  operator delete(void* Ptr) { HeapFree(GetProcessHeap(), 0, Ptr); }
//------------------------------------------------------------------------------------------------------------
template<typename T> static void DeleteCtrl(T& pCtrl){auto Ptr=pCtrl; pCtrl=nullptr; delete(Ptr);}
template<typename P, typename C> static void SetCallback(P& Ptr, C Callback){Ptr = static_cast<P>(Callback);}  
template<class T, typename... Args> T* AddObj(Args&&... args)     // NOTE: copy of this method`s body is created for every instance!   // Move allocator for children to a separate proc and NEW to a controls themselves?
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
 HWND       hWindow     = nullptr;   // Handle to this window control
 ATOM       WndClass    = 0;         // NULL for a system windows
 WNDPROC    OrigWndProc = DefWindowProcA;

CWndBase* GetOwnerWnd(void){return reinterpret_cast<CWndBase*>(this->OwnerObj);}
//------------------------------------------------------------------------------------------------------------
static LRESULT CALLBACK WndProxyProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
 CWndBase* This = (CWndBase*)GetWindowLongPtrA(hWnd, WLP_USERDATA);    // Ur use a window`s extra bytes instead?
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
ATOM GetSuperClass(LPSTR OrigClassName, LPSTR ClassName, UINT ExStyles=0)  // hInstance is NULL to superclass a system controls
{
 WNDCLASSEXA wcls;  
 WNDCLASSEXA ewcls; 

 wcls.cbSize   = sizeof(WNDCLASSEX);
 ewcls.cbSize  = sizeof(WNDCLASSEX);
 if(!GetClassInfoExA(NULL,OrigClassName,&wcls))return 0;     // Wrong system class?
 if(!this->hOwnerMod)this->SetHInstance();   
 if(ATOM MyClass  = GetClassInfoExA(this->hOwnerMod,ClassName,&ewcls))   // Returns an ATOM cast to BOOL as said on MSDN     // CLASS is already registered
  {        
   this->WndClass = MyClass;
   return this->WndClass;         // ATOM of an existing class
  }
 this->OrigWndProc  = wcls.lpfnWndProc;          // WM_CREATE will assign 'THIS' pointer to a window and there we assign original lpfnWndProc to a CLASS
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
HWND GetHandle(void){return this->hWindow;}
HWND GetOwnerHandle(void){return (this->OwnerObj)?(((CWndBase*)this->OwnerObj)->hWindow):(NULL);}
//------------------------------------------------------------------------------------------------------------
virtual ~CWndBase()    // A thread cannot use DestroyWindow to destroy a window created by a different thread. 
{                      // 'DestroyWindow' may fail here because all child windows is already deleted by parent`s 'DestroyWindow' call
 if(this->hWindow)DestroyWindow(this->hWindow);    // If the specified window is a parent or owner window, DestroyWindow automatically destroys the associated child or owned windows when it destroys the parent or owner window.
 if(this->WndClass)UnregisterClassA((LPCSTR)this->WndClass, this->hOwnerMod);   // Restore original WindowProc before this?
}  
//------------------------------------------------------------------------------------------------------------
virtual bool WindowProc(HWND& hWnd, UINT& Msg, WPARAM& wParam, LPARAM& lParam, LRESULT& lResult)      // Returns TRUE if the message processed
{
 lResult = CallWindowProcA(this->OrigWndProc, hWnd, Msg, wParam, lParam);   // Process on original WindowProc
 return true;
}      
//------------------------------------------------------------------------------------------------------------
// NOTE: Only system window classes support these by default (All of them?)
virtual HFONT GetFont(void){return (HFONT)SendMessageA(this->hWindow, WM_GETFONT, NULL, NULL);}
virtual void  SetFont(HFONT Font, bool Redraw=false){SendMessageA(this->hWindow, WM_SETFONT, (LPARAM)Font, Redraw);}
//------------------------------------------------------------------------------------------------------------
virtual BOOL  Show(bool Show){return ShowWindow(this->hWindow, (Show)?(SW_SHOW):(SW_HIDE));}
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
//                                                  EDIT CLASS
//==========================================================================================================================
class CSWEdit: public CCldBase
{
public:
CSWEdit(void){ }
//------------------------------------------------------------------------------------------------------------
int Create(LPCSTR EdText, SWDim& Wdim, bool Passw=false)
{
 DWORD Style = 0;           
 if(Passw)Style |= ES_PASSWORD;
 if(!this->GetSuperClass("EDIT", "CSWEdit"))return -1;
 return (this->CreateWnd((LPCSTR)this->WndClass, EdText, Style|WS_CHILD, 0, Wdim.PosX, Wdim.PosY, Wdim.Width, Wdim.Height, NULL))?(0):(-2);
}
//------------------------------------------------------------------------------------------------------------
};
//==========================================================================================================================
//                                                 BUTTON CLASS
//==========================================================================================================================
class CSWButton: public CCldBase
{
public:
// bool (_fastcall CWndBase::*OnMouseBtnUp)(CWndBase* Sender, LRESULT* Result, WORD WMsg, WORD KeyEx, int x, int y);    // If a callback returns true then no need to pass this message next to original WindowProc
 void (_fastcall CWndBase::*OnMouseBtnUp)(CWndBase* Sender, WORD WMsg, WORD KeyEx, int x, int y) = nullptr;    // If a callback returns true then no need to pass this message next to original WindowProc

CSWButton(void){ }
//------------------------------------------------------------------------------------------------------------
int Create(LPCSTR BtnName, SWDim& Wdim)  // Separate from constructor to allow some additional configuration
{ 
 DWORD Style = WS_CHILD|BS_CENTER|BS_DEFPUSHBUTTON; 
 if(!this->GetSuperClass("BUTTON", "CSWButton"))return -1;
 return (this->CreateWnd((LPCSTR)this->WndClass, BtnName, Style, 0, Wdim.PosX, Wdim.PosY, Wdim.Width, Wdim.Height, 0))?(0):(-2);
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
int Create(LPCSTR WndName, SWDim& Wdim, HWND hParentWnd, DWORD Style, DWORD ExStyle)
{
 WNDCLASSEX wcls; 

 if(!this->hOwnerMod)this->SetHInstance();
 wcls.cbSize        = sizeof(WNDCLASSEX);
 wcls.style         = CS_OWNDC|CS_HREDRAW|CS_VREDRAW|CS_GLOBALCLASS;
 wcls.cbClsExtra    = sizeof(PVOID);   // To store DefWindowProc
 wcls.cbWndExtra    = 0;
 wcls.hInstance     = this->hOwnerMod; 
 wcls.hIcon         = 0;                 
 wcls.hCursor       = LoadCursor(0, (LPCSTR)IDC_ARROW);
 wcls.hbrBackground = GetSysColorBrush(COLOR_BTNFACE); // (HBRUSH)(COLOR_BACKGROUND+1);  
 wcls.lpszClassName = "CSWForm";
 wcls.lpszMenuName  = 0;
 wcls.hIconSm       = 0;
 wcls.lpfnWndProc   = (WNDPROC)&CWndBase::WndProxyProc;   // Belongs to this window CLASS  // WM_CREATE is passed to class`s proc because only there it is specified before a window created
 this->WndClass     = RegisterClassExA(&wcls);
 if(!this->WndClass)return -1;
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

