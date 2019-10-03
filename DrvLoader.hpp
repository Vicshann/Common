
#pragma once
/*
  Copyright (c) 2019 Victor Sheinmann, Vicshann@gmail.com

  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), 
  to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
  and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. 
*/

#include "ntdll.h"
#include "Utils.h"
//---------------------------------------------------------------------------

class CDrvLoader        
{
static inline const wchar_t* SrvRegRoot = L"\\Registry\\Machine\\";
 wchar_t DriverPath[MAX_PATH];
 wchar_t DrvSrvPath[MAX_PATH];

//--------------------------------------
int CreateSrvRecord(bool RefreshIfExist=true)
{
 HKEY  hKey;
 DWORD dwDisposition;
 DBGMSG("Executable path: %ls", &this->DriverPath); 
 if(RegCreateKeyExW(HKEY_LOCAL_MACHINE, this->DrvSrvPath, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, &dwDisposition) != ERROR_SUCCESS){DBGMSG("Failed to create the service record: %ls", &this->DrvSrvPath); return -3;}
 if(dwDisposition != REG_CREATED_NEW_KEY)       // Already there!
  {
   DBGMSG("Service already exist: %ls", &this->DrvSrvPath); 
   if(!RefreshIfExist){RegCloseKey(hKey); return 0;}
  }    

 int Res = 0;
 DWORD dwPathSize = (lstrlenW(this->DriverPath) + 1) * sizeof(WCHAR);
 if(RegSetValueExW(hKey, L"ImagePath", 0, REG_BINARY, (PBYTE)&this->DriverPath, dwPathSize)){Res--; DBGMSG("Failed to set: ImagePath");}  // The Kernel doesn`t care and some antiviruses(WR-SA) allow      // Was REG_EXPAND_SZ
// if(RegQueryValueExW(hKey,L"ImagePath",NULL,NULL,NULL,NULL)){Res--; DBGMSG("ImagePath has disappeared!");}              // ERROR_ACCESS_DENIED -> RegCreateKeyExW:KEY_WRITE

 DWORD dwServiceType = 1;
 if(RegSetValueExW(hKey, L"Type", 0, REG_DWORD, (PBYTE)&dwServiceType, sizeof(DWORD))){Res--; DBGMSG("Failed to set: Type");}

// DWORD dwSrvErrCtrl = 1;
// DWORD dwServiceStart = 3;
// RegSetValueExW(hKey, L"ErrorControl", 0, REG_DWORD, (const BYTE *)&dwSrvErrCtrl, sizeof(DWORD));
// RegSetValueExW(hKey, L"Start", 0, REG_DWORD, (const BYTE *)&dwServiceStart, sizeof(DWORD));

 RegCloseKey(hKey);
 DBGMSG("%s",(Res < 0)?"Failed":"Done");
 return Res;
}
//--------------------------------------
int RemoveSrvRecord(void)
{
 if(RegDeleteKeyRecursive(HKEY_LOCAL_MACHINE, this->DrvSrvPath) < 0){DBGMSG("Failed!"); return -1;}
 DBGMSG("Done");
 return 0;
}
//--------------------------------------
// When KeepSrvRec is false if your process crashes then the driver won`t be in the system after reboot
int DriverOperate(bool Load, bool AllowRefresh=true, bool KeepSrvRec=false)
{
 UNICODE_STRING SrvPath;
 wchar_t Path[MAX_PATH];
 if(!this->pNtLoadDriver || !this->pNtUnloadDriver)
  {
   HMODULE hNtdll = GetModuleHandleA("ntdll.dll");
   if(!this->pNtLoadDriver)*(PVOID*)&this->pNtLoadDriver = GetProcAddress(hNtdll, "NtLoadDriver");
   if(!this->pNtUnloadDriver)*(PVOID*)&this->pNtUnloadDriver = GetProcAddress(hNtdll, "NtUnloadDriver");
  }
 if(this->CreateSrvRecord(AllowRefresh) < 0)return -1;
 lstrcpyW(Path, SrvRegRoot);
 lstrcatW(Path, this->DrvSrvPath);
 SrvPath.Buffer = Path;
 SrvPath.Length = lstrlenW(Path) * sizeof(WCHAR);  
 SrvPath.MaximumLength = SrvPath.Length + sizeof(WCHAR);
 DBGMSG("SrvPath: %ls",&Path);
 NTSTATUS res = Load ? this->pNtLoadDriver(&SrvPath) : this->pNtUnloadDriver(&SrvPath);        
 if(!KeepSrvRec)this->RemoveSrvRecord();                    
 if(Load && (STATUS_IMAGE_ALREADY_LOADED == res)){DBGMSG("Already loaded: %ls",&this->DriverPath); return 0;}
 if(!Load && (STATUS_OBJECT_NAME_NOT_FOUND == res)){DBGMSG("Not loaded: %ls",&this->DriverPath); return 0;}
 if(res != STATUS_SUCCESS){DBGMSG("Failed(%u): %08X",(int)Load,res); return (int)res;}
 DBGMSG("Done(%u)",(int)Load);
 return 0;
}
//--------------------------------------

public:
NTSTATUS (NTAPI *pNtLoadDriver)(IN PUNICODE_STRING DriverServiceName);
NTSTATUS (NTAPI *pNtUnloadDriver)(IN PUNICODE_STRING DriverServiceName);
//--------------------------------------         
CDrvLoader(void)
{
 memset(this,0,sizeof(CDrvLoader));
}
//--------------------------------------         
int Initialize(PWSTR SrvName, PWSTR DrvPath, PVOID pLoadDriver=NULL, PVOID pUnloadDriver=NULL)
{
 if(SetProcessPrivilegeState(true, SE_LOAD_DRIVER_NAME, GetCurrentProcess()) < 0){DBGMSG("Failed to enable SeLoadDriverPrivilege!"); return -1;}
 lstrcpyW(this->DrvSrvPath, L"System\\CurrentControlSet\\Services\\");
 lstrcatW(this->DrvSrvPath, SrvName);
 if(DrvPath[1] == ':')lstrcatW(this->DriverPath, L"\\??\\");
 lstrcpyW(this->DriverPath, DrvPath);
 DBGMSG("Done");
 return 0;
}
//--------------------------------------
int LoadDriver(void){return this->DriverOperate(true);}
int UnLoadDriver(void){return this->DriverOperate(false);}
//--------------------------------------


};
//---------------------------------------------------------------------------
