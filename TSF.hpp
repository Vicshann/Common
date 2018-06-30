
//====================================================================================
#pragma once
 
#ifndef TSF_H
#define TSF_H
/*
  Copyright (c) 2018 Victor Sheinmann, Vicshann@gmail.com

  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), 
  to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
  and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. 
*/
 
#include <windows.h>
#include <ole2.h>
#include <olectl.h>
#include <msctf.h>


#define LENSTRTCHAR(str) (sizeof(str)/sizeof(TCHAR))
//====================================================================================
class CTextService : public ITfTextInputProcessor
{
public:
CTextService::CTextService()
{
 // DllAddRef();
 _cRef = 1;
}
//------------------------------------------------------------------------------------
CTextService::~CTextService()
{
 // DllRelease();  // InterlockedDecrement(&g_cRefDll) // EnterCriticalSection(&g_cs); // FreeGlobalObjects(); if no more refs
}
//----------------------------------------------------------------------------
// IUnknown
STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj)
{
 if(ppvObj == NULL)return E_INVALIDARG;
 *ppvObj = NULL;
 if(IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ITfTextInputProcessor))*ppvObj = (ITfTextInputProcessor *)this;
 if(*ppvObj){AddRef(); return S_OK;}
 return E_NOINTERFACE;
}
//------------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) AddRef(void)
{
 return ++_cRef;
}
//------------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) Release(void)
{
 LONG cr = --_cRef;
 if(_cRef == 0)delete this;
 return cr;
}
//------------------------------------------------------------------------------------
// ITfTextInputProcessor
STDMETHODIMP Activate(ITfThreadMgr *pThreadMgr, TfClientId tfClientId)
{
 return S_OK;
}
//------------------------------------------------------------------------------------
STDMETHODIMP Deactivate(void)
{
 return S_OK;
}
//------------------------------------------------------------------------------------
// CClassFactory factory callback
static HRESULT CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObj)
{
 CTextService *pCase;

 if(ppvObj == NULL)return E_INVALIDARG;
 *ppvObj = NULL;
 if(NULL != pUnkOuter)return CLASS_E_NOAGGREGATION;
 if((pCase = new CTextService) == NULL)return E_OUTOFMEMORY;
 HRESULT hr = pCase->QueryInterface(riid, ppvObj);
 pCase->Release(); // caller still holds ref if hr == S_OK
 return hr;
}

private:
 LONG _cRef;     // COM ref count
};


/* e7ea138e-69f8-11d7-a6ea-00065b84435c */
static const CLSID c_clsidTextService = {0xe7ea138e, 0x69f8, 0x11d7, {0xa6, 0xea, 0x00, 0x06, 0x5b, 0x84, 0x43, 0x5c}};
/* e7ea138f-69f8-11d7-a6ea-00065b84435c */
static const GUID c_guidProfile = {0xe7ea138f, 0x69f8, 0x11d7, {0xa6, 0xea, 0x00, 0x06, 0x5b, 0x84, 0x43, 0x5c}};

//====================================================================================
/*
  Useless as a global hooking method.
  The DLL only loaded when the service selected manually from a tray menu.
  And in system tray a icon will appear so selec a text service after it is installes
*/
class CTSF
{
 static const int    CLSID_STRLEN       = 38;  // strlen("{xxxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxx}")
 static const int    TEXTSERVICE_LANGID = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);  // Added to all languages (Giant list of languages will be added) // Hind: Find existing languages and install it only for them

#define TEXTSERVICE_DESC   "My test text service"
#define TEXTSERVICE_MODEL  TEXT("Apartment")
#define c_szInfoKeyPrefix  TEXT("CLSID\\")
#define c_szInProcSvr32    TEXT("InProcServer32")
#define c_szModelName      TEXT("ThreadingModel")

public:
//------------------------------------------------------------------------------------
static BOOL CLSIDToString(REFGUID refGUID, TCHAR *pchA)
{
 static const BYTE GuidMap[] = {3, 2, 1, 0, '-', 5, 4, '-', 7, 6, '-', 8, 9, '-', 10, 11, 12, 13, 14, 15};
 static const TCHAR szDigits[] = TEXT("0123456789ABCDEF");
 TCHAR *p = pchA;
 const BYTE * pBytes = (const BYTE *) &refGUID;
 *p++ = TEXT('{');
 for (int i = 0; i < sizeof(GuidMap); i++)
  {
   if(GuidMap[i] != TEXT('-'))
    {
     *p++ = szDigits[ (pBytes[GuidMap[i]] & 0xF0) >> 4 ];
     *p++ = szDigits[ (pBytes[GuidMap[i]] & 0x0F) ];          
    }
      else   *p++ = TEXT('-');
  }
 *p++ = TEXT('}');
 *p   = TEXT('\0');
 return TRUE;
}
//------------------------------------------------------------------------------------
// RecurseDeleteKey is necessary because on NT RegDeleteKey doesn't work if the specified key has subkeys
//
static LONG _stdcall RecurseDeleteKey(HKEY hParentKey, LPCTSTR lpszKey)
{
 HKEY hKey;
 LONG lRes;
 FILETIME time;
 TCHAR szBuffer[256];
 DWORD dwSize = ARRAYSIZE(szBuffer);

 if(RegOpenKey(hParentKey, lpszKey, &hKey) != ERROR_SUCCESS)return ERROR_SUCCESS; // let's assume we couldn't open it because it's not there
 lRes = ERROR_SUCCESS;
 while (RegEnumKeyEx(hKey, 0, szBuffer, &dwSize, NULL, NULL, NULL, &time)==ERROR_SUCCESS)
  {
   szBuffer[ARRAYSIZE(szBuffer)-1] = '\0';
   lRes = RecurseDeleteKey(hKey, szBuffer);
   if(lRes != ERROR_SUCCESS)break;
   dwSize = ARRAYSIZE(szBuffer);
  }
 RegCloseKey(hKey);
 return lRes == ERROR_SUCCESS ? RegDeleteKey(hParentKey, lpszKey) : lRes;
}
//------------------------------------------------------------------------------------
static BOOL _stdcall RegisterProfiles(void)
{
 ITfInputProcessorProfiles *pInputProcessProfiles;

 HRESULT hr = CoCreateInstance(CLSID_TF_InputProcessorProfiles, NULL, CLSCTX_INPROC_SERVER, IID_ITfInputProcessorProfiles, (void**)&pInputProcessProfiles);
 if(hr != S_OK)return E_FAIL;
 hr = pInputProcessProfiles->Register(c_clsidTextService);
 if(hr == S_OK)hr = pInputProcessProfiles->AddLanguageProfile(c_clsidTextService, TEXTSERVICE_LANGID, c_guidProfile, _L(TEXTSERVICE_DESC), -1, NULL, -1, 0);    // No Icon(Default used)
 pInputProcessProfiles->Release();
 return (hr == S_OK);
}
//------------------------------------------------------------------------------------
static void _stdcall UnregisterProfiles(void)
{
 ITfInputProcessorProfiles *pInputProcessProfiles;
 HRESULT hr = CoCreateInstance(CLSID_TF_InputProcessorProfiles, NULL, CLSCTX_INPROC_SERVER, IID_ITfInputProcessorProfiles, (void**)&pInputProcessProfiles);
 if(hr != S_OK)return;
 pInputProcessProfiles->Unregister(c_clsidTextService);
 pInputProcessProfiles->Release();
}
//------------------------------------------------------------------------------------
static BOOL _stdcall RegisterServer(void)
{
 DWORD dw;
 HKEY hKey;
 HKEY hSubKey;
 BOOL fRet;
 TCHAR achIMEKey[LENSTRTCHAR(c_szInfoKeyPrefix) + CLSID_STRLEN];
 TCHAR achFileName[MAX_PATH];

 if(!CLSIDToString(c_clsidTextService, achIMEKey + LENSTRTCHAR(c_szInfoKeyPrefix) - 1))return FALSE;
 memcpy(achIMEKey, c_szInfoKeyPrefix, sizeof(c_szInfoKeyPrefix)-sizeof(TCHAR));
 if(fRet = RegCreateKeyEx(HKEY_CLASSES_ROOT, achIMEKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, &dw) == ERROR_SUCCESS)
  {
   fRet &= RegSetValueEx(hKey, NULL, 0, REG_SZ, (BYTE *)TEXTSERVICE_DESC, (lstrlen(TEXTSERVICE_DESC)+1)*sizeof(TCHAR)) == ERROR_SUCCESS;
   if(fRet &= RegCreateKeyEx(hKey, c_szInProcSvr32, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hSubKey, &dw) == ERROR_SUCCESS)
    {
     HMODULE hMod = NULL;
     GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,(TCHAR*)&RegisterServer,&hMod); 
     dw = GetModuleFileName(hMod, achFileName, LENSTRTCHAR(achFileName));
     fRet &= RegSetValueEx(hSubKey, NULL, 0, REG_SZ, (BYTE *)achFileName, (lstrlen(achFileName)+1)*sizeof(TCHAR)) == ERROR_SUCCESS;
     fRet &= RegSetValueEx(hSubKey, c_szModelName, 0, REG_SZ, (BYTE *)TEXTSERVICE_MODEL, (lstrlen(TEXTSERVICE_MODEL)+1)*sizeof(TCHAR)) == ERROR_SUCCESS;
     RegCloseKey(hSubKey);
    }
   RegCloseKey(hKey);
  }
 return fRet;
}
//------------------------------------------------------------------------------------
static void _stdcall UnregisterServer(void)
{
 TCHAR achIMEKey[LENSTRTCHAR(c_szInfoKeyPrefix) + CLSID_STRLEN];
 if (!CLSIDToString(c_clsidTextService, achIMEKey + LENSTRTCHAR(c_szInfoKeyPrefix) - 1))return;
 memcpy(achIMEKey, c_szInfoKeyPrefix, sizeof(c_szInfoKeyPrefix)-sizeof(TCHAR));
 RecurseDeleteKey(HKEY_CLASSES_ROOT, achIMEKey);
}
//------------------------------------------------------------------------------------
static BOOL _stdcall RegisterCategories(void)
{
 ITfCategoryMgr *pCategoryMgr;

 HRESULT hr = CoCreateInstance(CLSID_TF_CategoryMgr, NULL, CLSCTX_INPROC_SERVER, IID_ITfCategoryMgr, (void**)&pCategoryMgr);
 if(hr != S_OK)return FALSE;
 hr = pCategoryMgr->RegisterCategory(c_clsidTextService, GUID_TFCAT_TIP_KEYBOARD, c_clsidTextService);
 pCategoryMgr->Release();
 return (hr == S_OK);
}
//------------------------------------------------------------------------------------
static void _stdcall UnregisterCategories(void)
{
 ITfCategoryMgr *pCategoryMgr;

 HRESULT hr = CoCreateInstance(CLSID_TF_CategoryMgr, NULL, CLSCTX_INPROC_SERVER, IID_ITfCategoryMgr, (void**)&pCategoryMgr);
 if(hr != S_OK)return;
 pCategoryMgr->UnregisterCategory(c_clsidTextService, GUID_TFCAT_TIP_KEYBOARD, c_clsidTextService);
 pCategoryMgr->Release();
 return;
}
//------------------------------------------------------------------------------------

};
//====================================================================================
#endif