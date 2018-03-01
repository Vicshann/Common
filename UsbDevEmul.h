
//====================================================================================
#pragma once
 
#ifndef UsbDevEmulH
#define UsbDevEmulH
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
 
#include <Windows.h>
#include <setupapi.h>
#include <initguid.h>

//====================================================================================
struct CUsbDevEmul
{
 HDEVINFO hCurUsbDevInf;  
 int  UsbDevInfoIdx; 
 BYTE DevPathStr[99];       
 BYTE ClassGuidA[16];     // Device setup class GUID (For SetupDiGetClassDevs)         
 BYTE ClassGuidB[16];     // The GUID of the device's setup class (For ProcSetupDiEnumDeviceInfo)         
 BYTE FakeGuid[16];

public:
void Initialize(LPSTR DevPath, PBYTE GClassA, PBYTE GClassB, PBYTE GFake)
{
 memset(this,0,sizeof(CUsbDevEmul));
 memcpy(&this->ClassGuidA,GClassA,16);
 memcpy(&this->ClassGuidB,GClassB,16);
 memcpy(&this->FakeGuid,GFake,16);
 lstrcpyn((LPSTR)&this->DevPathStr,DevPath,90);
 this->UsbDevInfoIdx = -1;
}
//---------------------------------------------------
bool After_SetupDiGetClassDevs(GUID *ClassGuid, HDEVINFO DInf)
{
 if(!DInf || !ClassGuid || (memcmp(ClassGuid,&this->ClassGuidA,sizeof(GUID))!=0))return false;
 LOGMSG("UsbDev Class is found: %p",DInf); 
 this->hCurUsbDevInf = DInf; 
 this->UsbDevInfoIdx = -1;
 return true;
}
//---------------------------------------------------
int After_SetupDiEnumDeviceInfo(HDEVINFO DeviceInfoSet, DWORD MemberIndex, PSP_DEVINFO_DATA DeviceInfoData, int* Res)
{
 int LastErr = GetLastError();
 LOGMSG("Result=%u, UsbDevInfoIdx=%i, MemberIndex=%i, DeviceInfoSet=%p, hCurUsbDevInf=%p",Res,this->UsbDevInfoIdx,MemberIndex,DeviceInfoSet,this->hCurUsbDevInf);
 if(*Res && this->hCurUsbDevInf)
  {
   if(memcmp(&DeviceInfoData->ClassGuid,&this->ClassGuidB,sizeof(GUID))==0){LOGMSG("Class GUID is found: %p",DeviceInfoData); this->hCurUsbDevInf = NULL;}  // A real device is present  // Abort emulation?
  } 
 if(!*Res && (this->UsbDevInfoIdx < 0) /*&& (MemberIndex < 1)*/ && (DeviceInfoSet == this->hCurUsbDevInf) && (LastErr==ERROR_NO_MORE_ITEMS))  // Set hCurUsbDevInf to NULL if a real device is found
  {
   SetLastError(0);      
   *Res = 1;
   DeviceInfoData->cbSize   = sizeof(SP_DEVINFO_DATA);
   DeviceInfoData->Reserved = 0;
   DeviceInfoData->DevInst  = NULL; 
   memcpy(&DeviceInfoData->ClassGuid,&this->ClassGuidB,sizeof(GUID));      // Make a fake record
   this->UsbDevInfoIdx = MemberIndex;
   LOGMSG("Set fake SP_DEVINFO_DATA: %p",DeviceInfoData); 
   return true;
  }
// if((MemberIndex > 0)&&(this->UsbDevInfoIdx < 0)){this->hCurUsbDevInf = NULL;}    // ??????????????????????????????   
 return false;
}
//---------------------------------------------------
bool After_ProcSetupDiEnumDeviceInterfaces(HDEVINFO DeviceInfoSet, DWORD MemberIndex, PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData, int* Res)
{
 if(*Res || (MemberIndex != this->UsbDevInfoIdx) || (DeviceInfoSet != this->hCurUsbDevInf) || (GetLastError()!=ERROR_NO_MORE_ITEMS))return false;   // this->UsbDevInfoIdx must be for a fake entry
 SetLastError(0);      
 *Res = 1;  
 DeviceInterfaceData->cbSize   = sizeof(SP_DEVICE_INTERFACE_DATA);
 DeviceInterfaceData->Reserved = 0;
 DeviceInterfaceData->Flags    = SPINT_DEFAULT|SPINT_ACTIVE;
 memcpy(&DeviceInterfaceData->InterfaceClassGuid,&this->FakeGuid,sizeof(GUID));
 LOGMSG("Match No more Items To fake!",0); 
 return true;         
}
//---------------------------------------------------
int Before_SetupDiGetDeviceInterfaceDetail(HDEVINFO DeviceInfoSet, PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData, PSP_DEVICE_INTERFACE_DETAIL_DATA_A DeviceInterfaceDetailData, DWORD DeviceInterfaceDetailDataSize, PDWORD RequiredSize)
{
 if((DeviceInfoSet != this->hCurUsbDevInf) || (memcmp(&DeviceInterfaceData->InterfaceClassGuid,&this->FakeGuid,sizeof(GUID))!=0))return -1;  
 SetLastError(0);
 if(!DeviceInterfaceDetailData && !DeviceInterfaceDetailDataSize)
  {
   if(RequiredSize)*RequiredSize = 512;
   return 0;
  }  
 this->hCurUsbDevInf = NULL;      
 return 1;
}
//---------------------------------------------------
bool After_SetupDiGetDeviceInterfaceDetail(PSP_DEVICE_INTERFACE_DETAIL_DATA_A DeviceInterfaceDetailData, int Res)
{
 if(!Res || !DeviceInterfaceDetailData || (memcmp(&DeviceInterfaceDetailData->DevicePath,&this->DevPathStr,26)!=0))return false;   // 26 =  "\\\\?\\usb#vid_XXXX&pid_YYYY#"   // String compare case???
 this->hCurUsbDevInf = NULL;  
 this->UsbDevInfoIdx = -1;
 return true;
}
//---------------------------------------------------

};


//====================================================================================
#endif