// This is free and unencumbered software released into the public domain.
//
// Anyone is free to copy, modify, publish, use, compile, sell, or distribute this software, either in source code form or as a compiled
// binary, for any purpose, commercial or non-commercial, and by any means.
//
// In jurisdictions that recognize copyright laws, the author or authors of this software dedicate any and all copyright interest in the
// software to the public domain. We make this dedication for the benefit of the public at large and to the detriment of our heirs and
// successors. We intend this dedication to be an overt act of relinquishment in perpetuity of all present and future rights to this
// software under copyright law.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
// OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.

// For more information, please refer to <http://unlicense.org/>

#include "ExtraProt.h"
#include "Utils.h"
#include "SimWin.hpp"
#include "Sha256.h"
#include "Base64.hpp"
#include "MiniIni.hpp"
#include "MiniString.h"

int  BreakPassWnd = 0;
PASSCALLBACK Callback = NULL;
BYTE CurPassHash[SHA256::HashBytes];


class CPasswForm: public CWndForm
{
// <-- GENERATED
public:
 HFONT hDefFont;
// --> GENERATED

public:
CSWEdit*   Edit;
CSWButton* BtnOk;
CSWButton* BtnCancel;
   
//------------------------------------------------------------------------------------------------------------
CPasswForm(HWND hParent, int PosX, int PosY, int Width, int Height)
{
// <-- GENERATED
 this->hDefFont = CreateFontA(
					  12,   // Height
					   6,   // Width
                       0,
                       0,
                 FW_BOLD,
                   false,
                   false,
				   false,
         DEFAULT_CHARSET,
      OUT_DEFAULT_PRECIS,
     CLIP_DEFAULT_PRECIS,
           PROOF_QUALITY,
 FIXED_PITCH | FF_MODERN,
        "Times New Roman");

 this->Create("Enter the Password", SWDim{600, 700, 500, 600}, hParent, WS_CLIPSIBLINGS|WS_BORDER|WS_MINIMIZEBOX|WS_OVERLAPPEDWINDOW, WS_EX_CONTROLPARENT);    // Ex: WS_EX_APPWINDOW WS_EX_CLIENTEDGE WS_EX_TOPMOST
 this->SetFont(hDefFont);
 this->BtnOk     = this->AddObj<CSWButton>("OK", SWDim{15, 40, 75, 25});
 this->BtnCancel = this->AddObj<CSWButton>("CANCEL", SWDim{105, 40, 75, 25});
 this->Edit      = this->AddObj<CSWEdit>("", SWDim{7, 10, 180, 18}, ES_PASSWORD);
 
 SetCallback(BtnOk->OnMouseBtnUp, &CPasswForm::BtnOk_OnMouseBtnUp);  
 SetCallback(BtnCancel->OnMouseBtnUp, &CPasswForm::BtnCancel_OnMouseBtnUp);  

 this->Edit->Show(true);  
 this->BtnOk->Show(true);
 this->BtnCancel->Show(true); 

 this->Show(true);
// --> GENERATED
}
//------------------------------------------------------------------------------------------------------------
~CPasswForm()
{
 DeleteObject(this->hDefFont);
}
//------------------------------------------------------------------------------------------------------------
void _fastcall BtnOk_OnMouseBtnUp(CWndBase* Sender, WORD WMsg, WORD KeyEx, int x, int y)
{
 BreakPassWnd = 1;
}
//------------------------------------------------------------------------------------------------------------
void _fastcall BtnCancel_OnMouseBtnUp(CWndBase* Sender, WORD WMsg, WORD KeyEx, int x, int y)
{
 BreakPassWnd = 2;
}
//------------------------------------------------------------------------------------------------------------

};                       
//------------------------------------------------------------------------------------------------------------





//====================================================================================
void _stdcall InitPassword(UINT Passw, PASSCALLBACK PClbk)
{                   
 SHA256 sha;  
 BYTE ValBuff[128];
 int ValLen = 0;
 Callback = PClbk;
 LPSTR NVal = DecNumToStrU(Passw, (LPSTR)&ValBuff, &ValLen); 
 sha.reset();
 sha.add(NVal, ValLen);
 sha.getHash(CurPassHash);
}
//------------------------------------------------------------------------------------
bool _stdcall RequestPassword(void)
{
 static int TryCtr = 0;
 BYTE PassBuf[128];
 BYTE Hash[SHA256::HashBytes];
 SHA256   sha;

 RECT dwr;
 HWND MainWnd = GetDesktopWindow();
 GetWindowRect(MainWnd,&dwr);                
 CPasswForm* Dlg = new CPasswForm(MainWnd,dwr.left, dwr.top, (dwr.right - dwr.left), (dwr.bottom - dwr.top));

 int PassLen = 0;
 for(;;)    // Try Loop
  {
   while(!BreakPassWnd)
    {
     MSG Msg;
     int res = GetMessage(&Msg, 0, 0, 0);
     if(!res || res == -1)break;
     TranslateMessage(&Msg);
     DispatchMessageA(&Msg);
    }
   if(BreakPassWnd != 1){TerminateProcess(GetCurrentProcess(), 0); return false;}   // Cancel clicked
   PassLen = GetWindowTextA(Dlg->Edit->GetHandle(), (LPSTR)&PassBuf, sizeof(PassBuf));
   sha.reset();
   sha.add((LPSTR)&PassBuf, PassLen);
   sha.getHash(Hash);
   if(!memcmp(&CurPassHash, &Hash, sizeof(Hash)))break;
   if(++TryCtr >= 3)
    {
     if(Callback)Callback(1);
     SelfRemove(1);
     if(Callback)Callback(0);
     MainWnd = (HWND)-1;
     BreakPassWnd = 2;
     break;
    } 
   BreakPassWnd = 0;
  }
 if(!PassLen)TerminateProcess(GetCurrentProcess(), 0);
 if(BreakPassWnd == 2)MainWnd = (HWND)-2;     // Corrupt the window handle :)
 if(MainWnd != GetDesktopWindow())TerminateProcess(GetCurrentProcess(), 0);
 delete(Dlg);
 return true;
}
//====================================================================================
DWORD _stdcall FileTimeToPassword(PFILETIME FTime)
{
 BYTE TimeArray[8];

 *(PDWORD)&TimeArray[0] = FTime->dwLowDateTime;
 *(PDWORD)&TimeArray[4] = FTime->dwHighDateTime;

 BYTE Tmp = TimeArray[0];
 for(int ctr=0;ctr < 7;ctr++)
  {
   TimeArray[ctr] += Tmp;
   Tmp += (TimeArray[ctr] >> 2) & 0x0F;
  }
 TimeArray[7] += Tmp;
 return *(PDWORD)&TimeArray[0] ^ *(PDWORD)&TimeArray[4];
}
//------------------------------------------------------------------------------------
bool _stdcall EncryptString(DWORD Passw, CMiniStr& Str)
{
 BYTE TmpBuf[64];
 int  PassLen = 0;

 LPSTR PassStr = DecNumToStrU(Passw, (LPSTR)&TmpBuf, &PassLen);
 if(!PassStr || !PassLen)return false;
 BYTE KeyByte = (Passw & 0xF) + ((Passw >> 12) & 0xF) + ((Passw >> 16) & 0xF) + ((Passw >> 28) & 0xF);
 LPSTR StrPtr = Str.c_str();
 for(int ctr=0,pidx=0,tot=Str.Length();ctr < tot;ctr++,pidx++)
  {
   if(pidx >= PassLen)pidx = 0;           
   BYTE Val = (PassStr[PassLen - pidx - 1] + ~StrPtr[ctr]);
   Val = (Val << 4)|(Val >> 4);   // Swap halves
   StrPtr[ctr] = KeyByte + (PassStr[pidx] ^ (ctr + Val));
  }
 CBase64::Encode(Str);
 return true;
}
//------------------------------------------------------------------------------------
bool _stdcall DecryptString(DWORD Passw, CMiniStr& Str)
{
 BYTE TmpBuf[64];
 int  PassLen = 0;

 LPSTR PassStr = DecNumToStrU(Passw, (LPSTR)&TmpBuf, &PassLen);
 if(!PassStr || !PassLen)return false;
 BYTE KeyByte = (Passw & 0xF) + ((Passw >> 12) & 0xF) + ((Passw >> 16) & 0xF) + ((Passw >> 28) & 0xF);
 CBase64::Decode(Str);
 LPSTR StrPtr = Str.c_str();
 for(int ctr=0,pidx=0,tot=Str.Length();ctr < tot;ctr++,pidx++)
  {
   if(pidx >= PassLen)pidx = 0;           
   BYTE Val = ((PassStr[pidx] ^ (StrPtr[ctr] - KeyByte)) - ctr);
   Val = ((Val << 4)|(Val >> 4));  // Swap halves
   StrPtr[ctr] = ~(Val - PassStr[PassLen - pidx - 1]);
  }
 return true;
}
//------------------------------------------------------------------------------------
int _stdcall ProtRefreshINIValueInt(LPSTR SectionName, LPSTR ValueName, int Default, LPSTR FileName)
{
 BYTE OutStr[256]; // [sp+0h] [bp-200h]@1

 if(ProtRefreshINIValueStr(SectionName, ValueName, "", (LPSTR)&OutStr, sizeof(OutStr), FileName) == 0 )return DecStrToNum<int>((LPSTR)&OutStr);
 return Default;
}
//------------------------------------------------------------------------------------
int _stdcall ProtRefreshINIValueStr(LPSTR SectionName, LPSTR ValueName, LPSTR Default, LPSTR RetString, DWORD Size, LPSTR FileName)
{
 static DWORD Password = NULL;
 CMiniStr IniData;
 UINT ResSize;

 HMODULE hExeModule = GetModuleHandle(NULL);
 LPSTR CfgRes = (LPSTR)GetResource(hExeModule, (LPSTR)1, RT_MANIFEST, &ResSize);
 if(!CfgRes){SelfRemove(1); return -3;}
 IniData.cAssign(CfgRes,ResSize);
 int dpos = IniData.Pos("requestedExecutionLevel");
 if(dpos <= 0){SelfRemove(1); return -3;}
 dpos = IniData.Pos('>',CMiniStr::ComparatorE,dpos);
 if(dpos <= 0){SelfRemove(1); return -3;}
 IniData.Delete(0,dpos+1);
 dpos = IniData.Pos('<',CMiniStr::ComparatorE,dpos);
 if(dpos <= 0){SelfRemove(1); return -3;}
 IniData.SetLength(dpos);

 if(!Password)
  {
   WIN32_FILE_ATTRIBUTE_DATA FAttrs;
   BYTE ExePath[MAX_PATH];
   GetModuleFileName(hExeModule, (LPSTR)&ExePath, sizeof(ExePath));
   GetFileAttributesEx((LPSTR)&ExePath,GetFileExInfoStandard,&FAttrs);
   Password = FileTimeToPassword(&FAttrs.ftCreationTime);
   CMiniStr DecStr = IniData;
   DecryptString(Password, DecStr);
   if(DecStr.Pos(SectionName) < 0)
    {
     Password = FileTimeToPassword(&FAttrs.ftLastWriteTime);
     DecStr   = IniData;
     DecryptString(Password, DecStr);
     if(DecStr.Pos(SectionName) < 0)Password = FileTimeToPassword(&FAttrs.ftLastAccessTime);
    }
  }
 DecryptString(Password, IniData);
 dpos = IniData.Pos(SectionName);
 if(dpos < 0){SelfRemove(1); return -1;}

 CMiniIni ini;
 if(!ini.ParseString(IniData.c_str())){SelfRemove(1); return -2;}
 if(ini.GetValue(SectionName,ValueName,RetString,Size) <= 0){lstrcpyn(RetString,Default,Size); return 1;}     // Allow a missing parameters(Defaults)?
 return 0;
}
//====================================================================================
void _stdcall SelfRemove(bool Term)
{
 HMODULE hExeModule = NULL;
 HANDLE hBFile;
 DWORD  Result;
 char   CurDirPath[MAX_PATH];
 char   ModulePath[MAX_PATH];
 char   ModuleName[MAX_PATH];
 char   BatchFPath[MAX_PATH];
 char   BatchFile[1024];
 STARTUPINFO         si;
 PROCESS_INFORMATION pi;

 GetTempPath(sizeof(BatchFPath),BatchFPath);
 lstrcat(BatchFPath,"DelHelper.bat");

 GetModuleFileName(NULL,ModulePath,sizeof(ModulePath));
 GetShortPathName(ModulePath,CurDirPath,sizeof(CurDirPath));
 CharToOem(CurDirPath,CurDirPath);
 GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCSTR)SelfRemove, &hExeModule);
 GetModuleFileNameA(hExeModule, ModulePath, sizeof(ModulePath));

 for(DWORD ctr = lstrlen(CurDirPath);ctr > 0;ctr--)
  {
   if(CurDirPath[ctr] == '\\')
	{
	 CurDirPath[ctr] = 0;
	 lstrcpy(ModuleName,CurDirPath+ctr+1);
	 for(DWORD num = lstrlen(CurDirPath);num > 0;num--)
	 if(CurDirPath[num] == '\\')break;
	 break;
    }
  }  

 LPSTR DllName = GetFileName(ModulePath);  
 wsprintf(BatchFile,"@echo off\r\n:loop1\r\ndel %s\r\nif exist %s goto loop1\r\n:loop2\r\ndel %s\r\nif exist %s goto loop2\r\ndel %s\r\n",&ModuleName,&ModuleName,DllName,DllName,&BatchFPath);

 DeleteFile(BatchFPath);
 hBFile = CreateFile(BatchFPath, GENERIC_WRITE, NULL, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
 WriteFile(hBFile,BatchFile,strlen(BatchFile),&Result, NULL);
 CloseHandle(hBFile);

 strcat(CurDirPath,"\\");
 OemToChar(CurDirPath,CurDirPath);
 ZeroMemory(&si,sizeof(si));
 si.cb          = sizeof(si);
 si.wShowWindow = SW_HIDE;   //SW_SHOWNORMAL;
 si.dwFlags     = STARTF_USESHOWWINDOW;
 CreateProcess(NULL,BatchFPath,NULL,NULL,false,IDLE_PRIORITY_CLASS|CREATE_NEW_CONSOLE,NULL,CurDirPath,&si,&pi);
 if(Term)TerminateProcess(GetCurrentProcess(), 0);
}
//====================================================================================



