
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
struct CDevEmulHlp
{
 HDEVINFO hCurDevInf;  
 PVOID LstFakeIntfDataPtr;        // PSP_DEVICE_INTERFACE_DATA
// int  UsbDevInfoIdx;    // Useless? // Just use a first fail if 'MemberIndex'
 BYTE DevPathStr[99];       
 BYTE ClassGuidA[16];     // Device setup class GUID (For SetupDiGetClassDevs)         
 BYTE ClassGuidB[16];     // The GUID of the device's setup class (For ProcSetupDiEnumDeviceInfo)         
 BYTE FakeGuid[16];

public:
void Initialize(LPSTR DevPath, PBYTE GClassA, PBYTE GClassB, PBYTE GFake)
{
 memset(this,0,sizeof(CDevEmulHlp));
 memcpy(&this->ClassGuidA,GClassA,16);
 memcpy(&this->ClassGuidB,GClassB,16);
 memcpy(&this->FakeGuid,GFake,16);
 lstrcpyn((LPSTR)&this->DevPathStr,DevPath,90);
// this->UsbDevInfoIdx = -1;
}
//---------------------------------------------------
int After_SetupDiGetClassDevs(GUID *ClassGuid, HDEVINFO DInf)
{
 if(!DInf || !ClassGuid || (memcmp(ClassGuid,&this->ClassGuidA,sizeof(GUID))!=0))return 0;
 LOGMSG("Device Class is found: %p",DInf); 
 this->hCurDevInf = DInf; 
// this->UsbDevInfoIdx = -1;  // Reset index in case a new device has been connected
 return 1;
}
//---------------------------------------------------
int After_SetupDiEnumDeviceInfo(HDEVINFO DeviceInfoSet, DWORD MemberIndex, PSP_DEVINFO_DATA DeviceInfoData, int* Res)   // Fake device at first failed MemberIndex
{
 int LastErr = GetLastError();
 LOGMSG("Result=%u, UsbDevInfoIdx=%i, MemberIndex=%i, DeviceInfoSet=%p, hCurDevInf=%p",*Res,/*this->UsbDevInfoIdx*/0,MemberIndex,DeviceInfoSet,this->hCurDevInf);
 if(*Res && this->hCurDevInf)
  {
   if(memcmp(&DeviceInfoData->ClassGuid,&this->ClassGuidB,sizeof(GUID))==0){LOGMSG("Class GUID is found: %p",DeviceInfoData); this->hCurDevInf = NULL;}  // A real device is present  // Abort emulation?
  } 
 if(!*Res /*&& (this->UsbDevInfoIdx < 0)*/ /*&& (MemberIndex < 1)*/ && (DeviceInfoSet == this->hCurDevInf) && (LastErr==ERROR_NO_MORE_ITEMS))  // Set hCurDevInf to NULL if a real device is found
  {
   SetLastError(0);      
   *Res = 1;
   DeviceInfoData->cbSize   = sizeof(SP_DEVINFO_DATA);
   DeviceInfoData->Reserved = 0;
   DeviceInfoData->DevInst  = NULL; 
   memcpy(&DeviceInfoData->ClassGuid,&this->ClassGuidB,sizeof(GUID));      // Make a fake record
//   this->UsbDevInfoIdx = MemberIndex;
   LOGMSG("Set fake SP_DEVINFO_DATA: %p",DeviceInfoData); 
   return true;
  }
// if((MemberIndex > 0)&&(this->UsbDevInfoIdx < 0)){this->hCurDevInf = NULL;}    // ??????????????????????????????   
 return false;
}
//---------------------------------------------------
int After_ProcSetupDiEnumDeviceInterfaces(HDEVINFO DeviceInfoSet, DWORD MemberIndex, PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData, int* Res)  // Fake device at first failed MemberIndex
{
 int LastErr = GetLastError();
 LOGMSG("Result=%u, DeviceInfoSet=%p, hCurDevInf=%p, LastErr=%u",*Res,DeviceInfoSet,this->hCurDevInf,LastErr);
 if(*Res || /*(MemberIndex != this->UsbDevInfoIdx) ||*/ (DeviceInfoSet != this->hCurDevInf) || (GetLastError()!=ERROR_NO_MORE_ITEMS))return 0;   // this->UsbDevInfoIdx must be for a fake entry
 SetLastError(0);      
 *Res = 1;  
 this->LstFakeIntfDataPtr      = DeviceInterfaceData;
 DeviceInterfaceData->cbSize   = sizeof(SP_DEVICE_INTERFACE_DATA);
 DeviceInterfaceData->Reserved = 0;
 DeviceInterfaceData->Flags    = SPINT_DEFAULT|SPINT_ACTIVE;
 memcpy(&DeviceInterfaceData->InterfaceClassGuid,&this->FakeGuid,sizeof(GUID));
 LOGMSG("Match No more Items To fake!",0); 
 return 1;         
}
//---------------------------------------------------   
template<typename T> int Before_SetupDiGetDeviceInterfaceDetail(HDEVINFO DeviceInfoSet, PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData, T DeviceInterfaceDetailData, DWORD DeviceInterfaceDetailDataSize, PDWORD RequiredSize)   // T= PSP_DEVICE_INTERFACE_DETAIL_DATA_A/W
{
 if((DeviceInfoSet != this->hCurDevInf) || (DeviceInterfaceData != this->LstFakeIntfDataPtr) || (memcmp(&DeviceInterfaceData->InterfaceClassGuid,&this->FakeGuid,sizeof(GUID))!=0))return -1;    // SetupDiEnumDeviceInterfaces must return a fake HDEVINFO entry and a Fake GUID after all existing device entries
 SetLastError(0);
 if(!DeviceInterfaceDetailData && !DeviceInterfaceDetailDataSize)
  {
   if(RequiredSize)*RequiredSize = 512;
   return 0;
  }  
 LOGMSG("Resetting hCurDevInf: %p",this->hCurDevInf); 
 this->hCurDevInf      = NULL;  
 this->LstFakeIntfDataPtr = NULL;    
 return 1;
}
//---------------------------------------------------
template<typename T> int After_SetupDiGetDeviceInterfaceDetail(PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData, T DeviceInterfaceDetailData, int Res)         // T= PSP_DEVICE_INTERFACE_DETAIL_DATA_A/W
{
 if(!Res || (DeviceInterfaceData != this->LstFakeIntfDataPtr) || !DeviceInterfaceDetailData || (memcmp(&DeviceInterfaceDetailData->DevicePath,&this->DevPathStr,26)!=0))return 0;   // 26 =  "\\\\?\\usb#vid_XXXX&pid_YYYY#"   // String compare case???
 LOGMSG("Resetting hCurDevInf: %p",this->hCurDevInf); 
 this->hCurDevInf      = NULL;  
 this->LstFakeIntfDataPtr = NULL;
// this->UsbDevInfoIdx = -1;
 return 1;
}
//---------------------------------------------------

};


//====================================================================================
#endif