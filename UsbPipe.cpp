//---------------------------------------------------------------------------
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


#pragma hdrstop

#include "UsbPipe.h"

// This is the GUID for the USB device class
DEFINE_GUID(GUID_DEVINTERFACE_USB_DEVICE, 0xA5DCBF10L, 0x6530, 0x11D2, 0x90, 0x1F, 0x00, 0xC0, 0x4F, 0xB9, 0x51, 0xED);
// (A5DCBF10-6530-11D2-901F-00C04FB951ED)

//---------------------------------------------------------------------------
int _stdcall EnumDevicesUSB(LPSTR Buffer, UINT BufSize)
{
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
bool _stdcall GetVidPid(LPSTR DevPath, WORD* Vid, WORD* Pid)
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
