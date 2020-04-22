
#pragma once
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
#include "WinUsb.h"
#include "Utils.h"

class CUsbPipe
{
public:
 HANDLE  hDev;
 HMODULE hWinUsb;
 WINUSB_INTERFACE_HANDLE usbHandle;
// WINUSB_INTERFACE_HANDLE intfHandle;
 UCHAR PipeID;
 OVERLAPPED wovp;
 OVERLAPPED rovp;
 PBYTE CurDevPath[MAX_PATH];

 BOOL (__stdcall *pWinUsb_Initialize)(HANDLE,PWINUSB_INTERFACE_HANDLE);
 BOOL (__stdcall *pWinUsb_Free)(WINUSB_INTERFACE_HANDLE);
 BOOL (__stdcall *pWinUsb_GetAssociatedInterface)(WINUSB_INTERFACE_HANDLE,UCHAR,PWINUSB_INTERFACE_HANDLE);
 BOOL (__stdcall *pWinUsb_ReadPipe)(WINUSB_INTERFACE_HANDLE,UCHAR,PUCHAR,ULONG,PULONG,LPOVERLAPPED);
 BOOL (__stdcall *pWinUsb_WritePipe)(WINUSB_INTERFACE_HANDLE,UCHAR,PUCHAR,ULONG,PULONG,LPOVERLAPPED);
 BOOL (__stdcall *pWinUsb_ResetPipe)(WINUSB_INTERFACE_HANDLE,UCHAR);
 BOOL (__stdcall *pWinUsb_AbortPipe)(WINUSB_INTERFACE_HANDLE,UCHAR);
 BOOL (__stdcall *pWinUsb_GetOverlappedResult)(WINUSB_INTERFACE_HANDLE,LPOVERLAPPED,LPDWORD,BOOL);
 BOOL (__stdcall *pWinUsb_GetDescriptor)(WINUSB_INTERFACE_HANDLE,UCHAR,UCHAR,USHORT,PUCHAR,ULONG,PULONG);
 BOOL (__stdcall *pWinUsb_QueryInterfaceSettings)(WINUSB_INTERFACE_HANDLE,UCHAR,PUSB_INTERFACE_DESCRIPTOR);
 BOOL (__stdcall *pWinUsb_QueryPipe)(WINUSB_INTERFACE_HANDLE,UCHAR,UCHAR,PWINUSB_PIPE_INFORMATION);
 BOOL (__stdcall *pWinUsb_SetPipePolicy)(WINUSB_INTERFACE_HANDLE,UCHAR,ULONG,ULONG,PVOID);
 BOOL (__stdcall *pWinUsb_ControlTransfer)(WINUSB_INTERFACE_HANDLE,WINUSB_SETUP_PACKET,PUCHAR,ULONG,PULONG,LPOVERLAPPED);

//----------------------------------
int SetIntfPolUnk(WINUSB_INTERFACE_HANDLE uiHandle)
{
 WINUSB_PIPE_INFORMATION pipeinf;
 USB_INTERFACE_DESCRIPTOR idescr;
 if(pWinUsb_QueryInterfaceSettings(uiHandle, 0, &idescr))
  {
   for(int ctr=0;ctr < idescr.bNumEndpoints;ctr++)
	{
	 if(pWinUsb_QueryPipe(uiHandle, 0, ctr, &pipeinf))
	  {
	   if((UINT)(pipeinf.PipeType-2) <= 1)  // 2=UsbdPipeTypeBulk, 3=UsbdPipeTypeInterrupt
		{
		 Sleep(100);
		}
		 else  // 0=UsbdPipeTypeControl, 1=UsbdPipeTypeIsochronous
		  {
		   Sleep(100);
		  }
	   DWORD Value = 1;
	   if(!pWinUsb_SetPipePolicy(uiHandle, pipeinf.PipeId, SHORT_PACKET_TERMINATE, sizeof(DWORD), &Value))
		{
		 int err = GetLastError();
		 Sleep(100); // Log error
		}
	  }
	}
  }
 return 0; 
}
//----------------------------------

public:
CUsbPipe(void)
{
 memset(this,0,sizeof(CUsbPipe));
 this->hWinUsb    = NULL;
 this->usbHandle  = NULL;
// this->intfHandle = NULL;
 this->hDev = INVALID_HANDLE_VALUE;
 this->wovp.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
 this->rovp.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
}
//----------------------------------
~CUsbPipe()
{
 CloseHandle(this->wovp.hEvent);
 CloseHandle(this->rovp.hEvent);
 if(this->hWinUsb)this->Uninitialize();
}
//----------------------------------
int Initialize(void)
{
 this->hWinUsb = LoadLibrary("WinUsb.dll");
 if(!this->hWinUsb)return -1;
 *((PVOID*)&pWinUsb_Initialize) = GetProcAddress(this->hWinUsb,"WinUsb_Initialize");
 *((PVOID*)&pWinUsb_Free) = GetProcAddress(this->hWinUsb,"WinUsb_Free");
 *((PVOID*)&pWinUsb_GetAssociatedInterface) = GetProcAddress(this->hWinUsb,"WinUsb_GetAssociatedInterface");
 *((PVOID*)&pWinUsb_GetOverlappedResult) = GetProcAddress(this->hWinUsb,"WinUsb_GetOverlappedResult");
 *((PVOID*)&pWinUsb_GetDescriptor)  = GetProcAddress(this->hWinUsb,"WinUsb_GetDescriptor");
 *((PVOID*)&pWinUsb_ReadPipe)  = GetProcAddress(this->hWinUsb,"WinUsb_ReadPipe");
 *((PVOID*)&pWinUsb_WritePipe) = GetProcAddress(this->hWinUsb,"WinUsb_WritePipe");
 *((PVOID*)&pWinUsb_ResetPipe) = GetProcAddress(this->hWinUsb,"WinUsb_ResetPipe");
 *((PVOID*)&pWinUsb_AbortPipe) = GetProcAddress(this->hWinUsb,"WinUsb_AbortPipe");
 *((PVOID*)&pWinUsb_QueryInterfaceSettings) = GetProcAddress(this->hWinUsb,"WinUsb_QueryInterfaceSettings");
 *((PVOID*)&pWinUsb_QueryPipe) = GetProcAddress(this->hWinUsb,"WinUsb_QueryPipe");
 *((PVOID*)&pWinUsb_SetPipePolicy) = GetProcAddress(this->hWinUsb,"WinUsb_SetPipePolicy");
 *((PVOID*)&pWinUsb_ControlTransfer) = GetProcAddress(this->hWinUsb,"WinUsb_ControlTransfer");
}
//----------------------------------
void Uninitialize(void)
{
 this->CloseDevice();
 FreeLibrary(this->hWinUsb);
 this->hWinUsb = NULL;
}
//----------------------------------
int OpenDevice(LPSTR DevPath)
{
 this->CloseDevice();
 this->hDev = CreateFile(DevPath,GENERIC_WRITE|GENERIC_READ,FILE_SHARE_WRITE|FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL|FILE_FLAG_OVERLAPPED,NULL);
 if(INVALID_HANDLE_VALUE == this->hDev)return -1;
 if(!pWinUsb_Initialize(this->hDev,&this->usbHandle)){this->Uninitialize(); return -2;}
 ULONG Transferred = 0;
 BYTE  Buffer[4096];
 if(pWinUsb_GetDescriptor(this->usbHandle, USB_DEVICE_DESCRIPTOR_TYPE, 0, 0, (PBYTE)&Buffer, sizeof(Buffer), &Transferred))
  {
   USB_DEVICE_DESCRIPTOR* desc = (USB_DEVICE_DESCRIPTOR*)&Buffer;
   Sleep(100);
  }
 this->SetIntfPolUnk(this->usbHandle);
 WINUSB_INTERFACE_HANDLE intfHandle;
 for(int ctr=0;pWinUsb_GetAssociatedInterface(this->usbHandle, ctr, &intfHandle);ctr++)
  {
   this->SetIntfPolUnk(intfHandle);
   pWinUsb_Free(intfHandle);
  }  

 int res;
 UCHAR DIndex = 4; // First Name

 res = pWinUsb_GetDescriptor(this->usbHandle, USB_STRING_DESCRIPTOR_TYPE, DIndex, 0, (PBYTE)&Buffer, 255, &Transferred);   // Name: USB-SDC Converter
 res = this->SendControlPk(0xC0, 0x01, 0, 0, (PBYTE)&Buffer, sizeof(Buffer));
 res = this->SendControlPk(0xC0, 0x01, 0, 0, (PBYTE)&Buffer, sizeof(Buffer));

 res = pWinUsb_GetDescriptor(this->usbHandle, USB_STRING_DESCRIPTOR_TYPE, DIndex, 0, (PBYTE)&Buffer, 255, &Transferred);   // Name: SDC Primary
 res = pWinUsb_ResetPipe(this->usbHandle,0x82);
 res = pWinUsb_ResetPipe(this->usbHandle,0x02);
 res = this->SendControlPk(0xC0, 0x01, 0, 0, (PBYTE)&Buffer, sizeof(Buffer));

 lstrcpy((LPSTR)&CurDevPath, DevPath);
 return 1;
}
//----------------------------------
int CloseDevice(void)
{
// if(this->intfHandle && pWinUsb_AbortPipe)pWinUsb_AbortPipe(this->intfHandle, this->PipeID);
// if(this->usbHandle && pWinUsb_AbortPipe)pWinUsb_AbortPipe(this->usbHandle, this->PipeID);
// if(this->intfHandle && pWinUsb_Free){pWinUsb_Free(this->intfHandle); this->intfHandle = NULL;}
 if(this->usbHandle && pWinUsb_Free){pWinUsb_Free(this->usbHandle); this->usbHandle = NULL;}
 if(INVALID_HANDLE_VALUE != this->hDev){CloseHandle(this->hDev); this->hDev = INVALID_HANDLE_VALUE;}
}
//----------------------------------
int GetCurrentName(PWSTR Name, UINT Index)   // Must be 256  // GetDescriptor: bmRequestType = 0x80
{               
 BYTE  Buffer[260];
 ULONG Transferred = 0;
 Name[0] = 0;
 int res = pWinUsb_GetDescriptor(this->usbHandle, USB_STRING_DESCRIPTOR_TYPE, Index, 0, (PBYTE)&Buffer, sizeof(Buffer), &Transferred);   // Name: USB-SDC Converter
 if(!res || (Transferred < 3))return -1;
 int len = (Buffer[0] - 2) / sizeof(WCHAR);
 lstrcpynW(Name,(PWSTR)&Buffer[2],len+1);
 return len;
}
//----------------------------------
int SendControlPk(BYTE RequestType, BYTE Request, WORD Value, WORD Index, PBYTE Buffer, UINT BufSize, UINT Timeout=1000)
{
 DWORD DVal = Timeout;
 ULONG Transferred = 0;
 WINUSB_SETUP_PACKET SetupPk = {RequestType,Request,Value,Index,BufSize}; // C0=Request From Device, Vendor cpecific
 if(!pWinUsb_SetPipePolicy(this->usbHandle, 0, PIPE_TRANSFER_TIMEOUT, sizeof(DWORD), &DVal))return -1;
 if(!pWinUsb_ControlTransfer(this->usbHandle, SetupPk, Buffer, BufSize, &Transferred, NULL))return -2;
 return Transferred;
}             
//----------------------------------
int WriteUsbData(PBYTE Buffer, UINT BufSize, UINT EndpNum, UINT TimeoutMs, UINT* Transferred)
{
 UINT WrPipeId = (EndpNum & 0x0F); // bEndpointAddress:  Dir=00:(OUT)FromHost
 DWORD Transf  = 0;
 if(Transferred)*Transferred = 0;
 if(!pWinUsb_WritePipe(this->usbHandle, WrPipeId, Buffer, BufSize, 0, &this->wovp) && (GetLastError() != ERROR_IO_PENDING))return -1;
 switch(WaitForSingleObject(this->wovp.hEvent, TimeoutMs))
  {
   case WAIT_OBJECT_0:  // 0x000
	if(!pWinUsb_GetOverlappedResult(this->usbHandle, &this->wovp, &Transf, TRUE))return -2;
	break;
   case WAIT_TIMEOUT:   // 0x102
	pWinUsb_AbortPipe(this->usbHandle, WrPipeId);
	break;
   default: pWinUsb_AbortPipe(this->usbHandle, WrPipeId);
  }
 if(Transferred)*Transferred = Transf;
 return 0;
}
//----------------------------------
int ReadUsbData(PBYTE Buffer, UINT BufSize, UINT EndpNum, UINT TimeoutMs, UINT* Transferred)
{
 UINT RrPipeId = (EndpNum & 0x0F)|0x80; // bEndpointAddress:  Dir=80:(IN)ToHost - Request data
 DWORD Transf  = 0;
 if(Transferred)*Transferred = 0;
 if(!pWinUsb_ReadPipe(this->usbHandle,RrPipeId,Buffer,BufSize,0,&this->rovp) && (GetLastError() != ERROR_IO_PENDING))return -1;
 switch(WaitForSingleObject(this->rovp.hEvent, TimeoutMs))
  {
   case WAIT_OBJECT_0:  // 0x000
	if(!pWinUsb_GetOverlappedResult(this->usbHandle, &this->rovp, &Transf, TRUE))return -2;
	break;
   case WAIT_TIMEOUT:   // 0x102
    pWinUsb_AbortPipe(this->usbHandle, RrPipeId);
	break;
   default: pWinUsb_AbortPipe(this->usbHandle, RrPipeId);
  }
 if(Transferred)*Transferred = Transf;
 return 0;
}
//---------------------------------------------------------------------------
static bool GetVidPid(LPSTR DevPath, WORD* Vid, WORD* Pid)
{
 BYTE Path[256];
 lstrcpyn((LPSTR)&Path, DevPath, sizeof(Path));
 CharLower((LPSTR)&Path);
 bool HaveVid = false;
 bool HavePid = false;
 for(int ctr=0;ctr < sizeof(Path);ctr++)
  {
   if(memcmp(&Path[ctr],"vid_",4)==0){*Vid = HexStrToNum((LPSTR)&Path[ctr+4]); HaveVid = true;}
	else if(memcmp(&Path[ctr],"pid_",4)==0){*Pid = HexStrToNum((LPSTR)&Path[ctr+4]); HavePid = true;}
   if(HaveVid && HavePid)return true;
  }
 return false;
}
//---------------------------------------------------------------------------
static int EnumDevicesUSB(LPSTR Buffer, UINT BufSize)
{
// This is the GUID for the USB device class
DEFINE_GUID(GUID_DEVINTERFACE_USB_DEVICE, 0xA5DCBF10L, 0x6530, 0x11D2, 0x90, 0x1F, 0x00, 0xC0, 0x4F, 0xB9, 0x51, 0xED);
// (A5DCBF10-6530-11D2-901F-00C04FB951ED)
	
	HDEVINFO                         hDevInfo;
	SP_DEVICE_INTERFACE_DATA         DevIntfData;
	PSP_DEVICE_INTERFACE_DETAIL_DATA DevIntfDetailData;
	SP_DEVINFO_DATA                  DevData;
	DWORD dwSize, dwType, dwMemberIdx;
	int diCount = 0;

	// We will try to get device information set for all USB devices that have a
	// device interface and are currently present on the system (plugged in).
	hDevInfo = SetupDiGetClassDevs(&GUID_DEVINTERFACE_USB_DEVICE, NULL, 0, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
	if (hDevInfo != INVALID_HANDLE_VALUE)
	{
		// Prepare to enumerate all device interfaces for the device information
		// set that we retrieved with SetupDiGetClassDevs(..)
		DevIntfData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
		dwMemberIdx = 0;
		
		// Next, we will keep calling this SetupDiEnumDeviceInterfaces(..) until this
		// function causes GetLastError() to return  ERROR_NO_MORE_ITEMS. With each
		// call the dwMemberIdx value needs to be incremented to retrieve the next
		// device interface information.

		SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &GUID_DEVINTERFACE_USB_DEVICE, dwMemberIdx, &DevIntfData);
		while(GetLastError() != ERROR_NO_MORE_ITEMS)
		{
			// As a last step we will need to get some more details for each
			// of device interface information we are able to retrieve. This
			// device interface detail gives us the information we need to identify
			// the device (VID/PID), and decide if it's useful to us. It will also
			// provide a DEVINFO_DATA structure which we can use to know the serial
			// port name for a virtual com port.
			DevData.cbSize = sizeof(DevData);
			
			// Get the required buffer size. Call SetupDiGetDeviceInterfaceDetail with
			// a NULL DevIntfDetailData pointer, a DevIntfDetailDataSize
			// of zero, and a valid RequiredSize variable. In response to such a call,
			// this function returns the required buffer size at dwSize.
			SetupDiGetDeviceInterfaceDetail(hDevInfo, &DevIntfData, NULL, 0, &dwSize, NULL);

			// Allocate memory for the DeviceInterfaceDetail struct. Don't forget to
			// deallocate it later!
			DevIntfDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwSize);
			DevIntfDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
			if (SetupDiGetDeviceInterfaceDetail(hDevInfo, &DevIntfData,DevIntfDetailData, dwSize, &dwSize, &DevData))
			{
				// Finally we can start checking if we've found a useable device,
				// by inspecting the DevIntfDetailData->DevicePath variable.
				// The DevicePath looks something like this:
				//
				// \\?\usb#vid_04d8&pid_0033#5&19f2438f&0&2#{a5dcbf10-6530-11d2-901f-00c04fb951ed}
				//
			BYTE NameBuf[256];
			if(!SetupDiGetDeviceRegistryProperty(hDevInfo,&DevData,SPDRP_DEVICEDESC,NULL,(PBYTE)&NameBuf,sizeof(NameBuf),NULL))lstrcpy((LPSTR)&NameBuf, "Unknown");
            int err = GetLastError();
			UINT PLen = lstrlen((LPSTR)&NameBuf);
			if(PLen > BufSize)goto Exit;
			lstrcpy(Buffer,(LPSTR)&NameBuf);
			Buffer += PLen+1;
			PLen = lstrlen(DevIntfDetailData->DevicePath);
			if(PLen > BufSize)goto Exit;
			lstrcpy(Buffer,DevIntfDetailData->DevicePath);
			Buffer += PLen+1;
			diCount++;
			}
			HeapFree(GetProcessHeap(), 0, DevIntfDetailData);
			// Continue looping
			SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &GUID_DEVINTERFACE_USB_DEVICE, ++dwMemberIdx, &DevIntfData);
		}
Exit:
		SetupDiDestroyDeviceInfoList(hDevInfo);
	}
 *Buffer = 0;	
 return diCount;
}
//---------------------------------------------------------------------------

};

//---------------------------------------------------------------------------

/*
				// The VID for Velleman Projects is always 10cf. The PID is variable
				// for each device:
				//
				//    -------------------
				//    | Device   | PID  |
				//    -------------------
				//    | K8090    | 8090 |
				//    | VMB1USB  | 0b1b |
				//    -------------------
				//
				// As you can see it contains the VID/PID for the device, so we can check
				// for the right VID/PID with string handling routines.

				if (NULL != _tcsstr((TCHAR*)DevIntfDetailData->DevicePath, _T("vid_10cf&pid_8090")))
				{
					// To find out the serial port for our K8090 device,
					// we'll need to check the registry:

					hKey = SetupDiOpenDevRegKey(
								hDevInfo,
								&DevData,
								DICS_FLAG_GLOBAL,
								0,
								DIREG_DEV,
								KEY_READ
							);

					dwType = REG_SZ;
					dwSize = sizeof(lpData);
					RegQueryValueEx(hKey, _T("PortName"), NULL, &dwType, lpData, &dwSize);
					RegCloseKey(hKey);

					// Eureka!
					wprintf(_T("Found a device on port '%s'\n"), lpData);
				}
*/
