
#pragma once

#include <windows.h>
#include <Windowsx.h>
#include <Commctrl.h>

#include "CompileTime.hpp"     

//==========================================================================================================================
// TODO:
//  Make everything UNICODE
//  Add HFONT in constructors and take it from parent window if NULL 
//  Use CObj factory and some decent memory allocator
// 
// 
// 
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

#ifndef WM_DPICHANGED
#define WM_DPICHANGED       0x02E0
#endif

struct SWIN
{
private:
static inline decltype(ChangeWindowMessageFilterEx)* pChangeWindowMessageFilterEx = nullptr;


public:
enum EWNotify {wnChildren=1, wnDeepChld=2, wnDeepPrnt=4};
enum EEdgeSnap {esLeft=1, esRight=2, esTop=4, esBottom=8};
enum EObjFlg {flDerWndForm=0x80000000, flDerWndBase=0x40000000, flDerVisBase=0x20000000, flDerCldBase=0x10000000, flDerTrayIcon=0x08000000};

struct SWDim   // TODO: Rework to support anchoring
{
 int PosX; 
 int PosY; 
 int Width; 
 int Height;
};

static size_t ReallocBufIfNeeded(BYTE*& Buf, size_t NewSize, size_t OldSize)
{
 if(NewSize <= OldSize)return OldSize;
 if(Buf){HeapFree(GetProcessHeap(),0,Buf); Buf=nullptr;}
 if(!Buf)Buf = (BYTE*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,NewSize+8);
 return NewSize;
}

//==========================================================================================================================
class CObjBase;
class CWndBase;

#include "BaseObject.hpp"
#include "BaseVisual.hpp"
#include "BaseWindow.hpp"
#include "BaseFormWnd.hpp"
#include "BaseChildWnd.hpp"

#include "Button.hpp"
#include "Edit.hpp"
#include "StaticText.hpp"
#include "ComboBox.hpp"
#include "ScrollBar.hpp"
#include "DTPicker.hpp"
#include "PopupMenu.hpp"
#include "TrayIcon.hpp"
#include "Timer.hpp"
//==========================================================================================================================
//------------------------------------------------------------------------
static inline bool Initialize(bool DpiAware)
{
 INITCOMMONCONTROLSEX icex;                   // Move to Gen section?
 icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
 icex.dwICC  = ICC_STANDARD_CLASSES|ICC_DATE_CLASSES;
 if(DpiAware)     // Win7+   // Makes everything too small in 4K
  {
   decltype(SetProcessDPIAware)* pSetProcessDPIAware = (decltype(SetProcessDPIAware)*)GetProcAddress(GetModuleHandle("User32.dll"), "SetProcessDPIAware"); 
   if(pSetProcessDPIAware)pSetProcessDPIAware();   // I create a window 400 x 250 and get 230 x 144. Why? How to work with this?
  }
 return InitCommonControlsEx(&icex); 
}
//------------------------------------------------------------------------
}; // NSIMWIN

//==========================================================================================================================


namespace NSWIN     // Just a name container, expect to be included and leak into some other namespace
{
 using EWNotify     = SWIN::EWNotify;
 using SWDim        = SWIN::SWDim;

 using CWndForm     = SWIN::CWndForm;
 using CSWEdit      = SWIN::CSWEdit;
 using CSWButton    = SWIN::CSWButton;
 using CSWStatic    = SWIN::CSWStatic;
 using CSWScrBar    = SWIN::CSWScrBar;
 using CSWDTPicker  = SWIN::CSWDTPicker;
 using CSWComboBox  = SWIN::CSWComboBox;
 using CSWTrayIcon  = SWIN::CSWTrayIcon;
 using CSWTimer  = SWIN::CSWTimer;    
}
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

