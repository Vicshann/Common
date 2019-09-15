
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

#define _WIN32_WINNT 0x0500

#include <windows.h>
#include <AclApi.h>
#include <WinIoCtl.h>

//---------------------------------------------------------------------------
#define FILE_DEVICE_CUSTOM  0x00008000

__inline DWORD MakeCtrlCode(DWORD DevType, DWORD Access, DWORD Function, DWORD Method){return((DevType << 16) | (Access << 14) | (Function << 2) | Method);}
//---------------------------------------------------------------------------
union IoCtrlCode
{
 DWORD dwControlCode;
 struct
  {
   DWORD TransferType   : 2;
   DWORD FunctionCode   : 12;
   DWORD RequiredAccess : 2;
   DWORD DeviceType     : 16;
  };
 IoCtrlCode(unsigned int Code=0){this->dwControlCode = Code;}
 IoCtrlCode(int TransType, int FuncCode, int ReqAccess, int DevType)
  {
   this->TransferType   = TransType;
   this->FunctionCode   = FuncCode;
   this->RequiredAccess = ReqAccess;
   this->DeviceType     = DevType;
  }
};
//---------------------------------------------------------------------------
class CSrvControl
{
 bool      Autodel;
 HANDLE    hDriver;
 SC_HANDLE hSCManager;
 SC_HANDLE hSCService;

public:
CSrvControl(bool RemoveOnDel=false, PWSTR SrvPath=NULL, PWSTR SrvName=NULL)
{
 this->hDriver = INVALID_HANDLE_VALUE;
 this->Autodel = RemoveOnDel;
 this->hSCManager = this->hSCService = NULL;
 if(SrvPath)this->CreateSrv(SrvPath, SrvName); // Only as 'Demand Start' service
}
//---------------------------------------------------------------------------
~CSrvControl()
{
 this->CloseDriver();
 if(Autodel)
  {
   this->StopSrv();
   this->RemoveService();
  }
}
//---------------------------------------------------------------------------
int StartSrv(void)
{
 if(::StartServiceW(this->hSCService, 0, NULL)||(GetLastError()==ERROR_SERVICE_ALREADY_RUNNING))return ERROR_SUCCESS;
 return GetLastError();
}
//---------------------------------------------------------------------------
int StopSrv(void)
{
 SERVICE_STATUS SrvStat;
 if(ControlService(this->hSCService, SERVICE_CONTROL_STOP, &SrvStat))return ERROR_SUCCESS;
 return GetLastError();
}
//---------------------------------------------------------------------------
int RemoveService(void)
{
 if(!this->hSCService || !this->hSCManager)return ERROR_INVALID_HANDLE;
 DWORD ErrCode = ERROR_SUCCESS;
 this->CloseDriver();     // Close a driver if opened
 this->StopSrv();         // Stop the Service if running
 if(!DeleteService(this->hSCService))ErrCode = GetLastError();
 CloseServiceHandle(this->hSCService);
 CloseServiceHandle(this->hSCManager);
 this->hSCService = this->hSCManager = NULL;
 if(ErrCode != ERROR_SERVICE_MARKED_FOR_DELETE)return ErrCode;
 return ERROR_SUCCESS;
}
//---------------------------------------------------------------------------
int CreateSrv(PWSTR SrvModPath, PWSTR ServiceName=NULL, PWSTR ServiceDesc=NULL, DWORD SrvType=SERVICE_WIN32_OWN_PROCESS, DWORD StartType=SERVICE_DEMAND_START, bool OpenOnly=false) // SERVICE_BOOT_START is only for SERVICE_FILE_SYSTEM_DRIVER ?
{
 wchar_t NameBuf[MAX_PATH];

// if(!SrvModPath)return ERROR_INVALID_NAME;
 if(SrvModPath && !ServiceName)
  {  // Make a driver name from a file name
   NameBuf[0] = 0;
   for(int ctr=(lstrlenW(SrvModPath)-1);ctr > 0;ctr--){if((SrvModPath[ctr] == 0x5C)||(SrvModPath[ctr] == 0x2F)){lstrcpyW(NameBuf, &SrvModPath[ctr+1]);break;}} // Remove path
   for(int ctr=(lstrlenW(NameBuf)-1);ctr > 0;ctr--){if(NameBuf[ctr] == 0x2E){NameBuf[ctr]=0;break;}}  // Remove extension
   ServiceName = NameBuf;
  }
 if(!(this->hSCManager = OpenSCManagerW(NULL,NULL,SC_MANAGER_ALL_ACCESS)))return GetLastError();
 if(!(this->hSCService = OpenServiceW(this->hSCManager, ServiceName, SERVICE_ALL_ACCESS)))   // Try to create service if it is failed to open
  {
   DWORD ErrCode = GetLastError();
   if((ErrCode != ERROR_SERVICE_DOES_NOT_EXIST) || OpenOnly)return ErrCode;
   if(!SrvModPath)return -1;     // INVALID_PATH
   if(!(this->hSCService = CreateServiceW(this->hSCManager,ServiceName,ServiceName,SERVICE_ALL_ACCESS,SrvType,(StartType==SERVICE_BOOT_START)?(SERVICE_DEMAND_START):(StartType),SERVICE_ERROR_NORMAL,SrvModPath,NULL, NULL, NULL, NULL, NULL)))return GetLastError();
   //if(StartType != SERVICE_BOOT_START)return ERROR_SUCCESS;
   if(ServiceDesc && *ServiceDesc)ChangeServiceConfig2W(this->hSCService,SERVICE_CONFIG_DESCRIPTION,&ServiceDesc);
  }
 if(SrvModPath && ChangeServiceConfigW(this->hSCService,SERVICE_NO_CHANGE,StartType,SERVICE_NO_CHANGE,SrvModPath,NULL,NULL,NULL,NULL,NULL,ServiceName))return ERROR_SUCCESS;   // Update module path 
 this->AdjustAccessRights();  // For what this is needed?
 return GetLastError();
}
//---------------------------------------------------------------------------
int AdjustAccessRights(void)
{
 int                  bError         = false;
 int                  bDaclPresent   = false;
 int                  bDaclDefaulted = false;
 PACL                 pNewAcl        = NULL;
 PACL                 pacl           = NULL;
 DWORD                dwSize         = 0;
 PSECURITY_DESCRIPTOR psd            = NULL;
 SECURITY_DESCRIPTOR  sd;
 EXPLICIT_ACCESS      ea;

 if(!hSCService)return false;
 // Find out how much memory to allocate for psd. psd can't be NULL so we let it point to itself.
 psd = (PSECURITY_DESCRIPTOR)&psd;
 if(!QueryServiceObjectSecurity(hSCService, DACL_SECURITY_INFORMATION, psd, 0, &dwSize))
  {
   if(GetLastError() == ERROR_INSUFFICIENT_BUFFER)bError = !(psd = (PSECURITY_DESCRIPTOR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwSize));
     else bError = true;
  }
 // Get the current security descriptor.
 if(!bError && !QueryServiceObjectSecurity(hSCService, DACL_SECURITY_INFORMATION, psd, dwSize, &dwSize))bError = true;
 // Get the DACL.
 if(!bError && !GetSecurityDescriptorDacl(psd, &bDaclPresent, &pacl, &bDaclDefaulted))bError = true;
 // Build the ACE.
 if(!bError)
  {
   SID_IDENTIFIER_AUTHORITY SIDAuthWorld = SECURITY_WORLD_SID_AUTHORITY;
   PSID                     pSIDEveryone;
   // Create a SID for the Everyone group.
   if(!AllocateAndInitializeSid(&SIDAuthWorld,1,SECURITY_WORLD_RID,0,0,0,0,0,0,0,&pSIDEveryone))bError = true;
	 else
	  {
	   ea.grfAccessMode                    = SET_ACCESS;
	   ea.grfAccessPermissions             = SERVICE_ALL_ACCESS;
	   ea.grfInheritance                   = NO_INHERITANCE;
	   ea.Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
	   ea.Trustee.pMultipleTrustee         = NULL;
	   ea.Trustee.TrusteeForm              = TRUSTEE_IS_SID;
	   ea.Trustee.TrusteeType              = TRUSTEE_IS_GROUP;
	   ea.Trustee.ptstrName                = (char*)pSIDEveryone;
	   if(SetEntriesInAcl(1, &ea, pacl, &pNewAcl) != ERROR_SUCCESS)bError = true;
	  }
   FreeSid(pSIDEveryone);
  }
 // Initialize a new Security Descriptor.
 if(!bError && !InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION))bError = true;
 // Set the new DACL in the Security Descriptor.
 if(!bError && !SetSecurityDescriptorDacl(&sd, TRUE, pNewAcl, FALSE))bError = true;
 // Set the new DACL for the service object.
 if(!bError && !SetServiceObjectSecurity(hSCService, DACL_SECURITY_INFORMATION, &sd))bError = true;

 LocalFree((HLOCAL)pNewAcl);
 HeapFree(GetProcessHeap(), 0, (LPVOID)psd);
 return !bError;
}
//---------------------------------------------------------------------------
//                          DRIVER SPECIFIC
//---------------------------------------------------------------------------
int OpenDriver(void)
{
 wchar_t  SymLink[MAX_PATH];
 BYTE  SCfgBuf[1024];     // Must be enough for all our drivers
 DWORD BufLen;
 QUERY_SERVICE_CONFIGW *SrvConfig = (QUERY_SERVICE_CONFIGW*)&SCfgBuf;

 if(!QueryServiceConfigW(this->hSCService,SrvConfig,sizeof(SCfgBuf),&BufLen))return GetLastError();
 lstrcpyW(SymLink, L"\\\\.\\");
 lstrcatW(SymLink, SrvConfig->lpDisplayName);
 if((this->hDriver = CreateFileW(SymLink,GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,NULL,NULL))==INVALID_HANDLE_VALUE)return ERROR_INVALID_HANDLE;
 return ERROR_SUCCESS;
}
//---------------------------------------------------------------------------
int CloseDriver(void)
{
 if(this->hDriver == INVALID_HANDLE_VALUE)return ERROR_INVALID_HANDLE;
 CloseHandle(this->hDriver);
 this->hDriver = INVALID_HANDLE_VALUE;
 return ERROR_SUCCESS;
}
//---------------------------------------------------------------------------
int ControlDriver(DWORD Code, PVOID SndBuffer=NULL, UINT SndBufSize=0, PVOID ResBuffer=NULL, UINT ResBufSize=0)
{
 DWORD BytesReturned;
 IoCtrlCode Control;

 if(Code < 0x00010000)
  { // Given only Function Code
   Control.DeviceType     = FILE_DEVICE_CUSTOM;
   Control.FunctionCode   = Code;
   Control.TransferType   = METHOD_NEITHER;   // Most fastest; Must be enough for simple drivers
   Control.RequiredAccess = FILE_ANY_ACCESS;  // Do not require any specific access rights
  }
   else Control.dwControlCode = Code;  // Given normal control code
 if(DeviceIoControl(this->hDriver,Control.dwControlCode,SndBuffer,SndBufSize,ResBuffer,ResBufSize,&BytesReturned,NULL))return BytesReturned;
 return -1;  // 'GetLastError' can be called after
}
//---------------------------------------------------------------------------

};
//---------------------------------------------------------------------------

