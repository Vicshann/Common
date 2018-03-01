/*++

Copyright (c) 2002 Microsoft Corporation

Module Name:

        wusb.h

Abstract:

        Public interface to winusb.dll

Environment:

        Kernel Mode Only

Notes:

        THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
        KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
        IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
        PURPOSE.

        Copyright (c) 2001 Microsoft Corporation.  All Rights Reserved.


Revision History:

        11/19/2002 : created

Authors:

--*/

#ifndef __WUSB_H__
#define __WUSB_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <winusbio.h>


typedef PVOID WINUSB_INTERFACE_HANDLE, *PWINUSB_INTERFACE_HANDLE;

   
#pragma pack(1)

typedef struct _WINUSB_SETUP_PACKET {
    UCHAR   RequestType;
    UCHAR   Request;
    USHORT  Value;
    USHORT  Index;
    USHORT  Length;
} WINUSB_SETUP_PACKET, *PWINUSB_SETUP_PACKET;

#pragma pack()



BOOL __stdcall
WinUsb_Initialize(
    IN HANDLE DeviceHandle,
    OUT PWINUSB_INTERFACE_HANDLE InterfaceHandle
    );


BOOL __stdcall
WinUsb_Free(
    IN  WINUSB_INTERFACE_HANDLE InterfaceHandle
    );


BOOL __stdcall
WinUsb_GetAssociatedInterface(
    IN WINUSB_INTERFACE_HANDLE InterfaceHandle,
    IN UCHAR AssociatedInterfaceIndex,
    OUT PWINUSB_INTERFACE_HANDLE AssociatedInterfaceHandle
    );



BOOL __stdcall
WinUsb_GetDescriptor(
    IN  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    IN  UCHAR DescriptorType,
    IN  UCHAR Index,
    IN  USHORT LanguageID,
    OUT PUCHAR Buffer,
    IN  ULONG BufferLength,
    OUT PULONG LengthTransferred
    );

BOOL __stdcall
WinUsb_QueryInterfaceSettings(
    IN WINUSB_INTERFACE_HANDLE InterfaceHandle,
    IN UCHAR AlternateInterfaceNumber,
    OUT PUSB_INTERFACE_DESCRIPTOR UsbAltInterfaceDescriptor
    );

BOOL __stdcall
WinUsb_QueryDeviceInformation(
    IN WINUSB_INTERFACE_HANDLE InterfaceHandle,
    IN ULONG InformationType,
    IN OUT PULONG BufferLength,
    OUT PVOID Buffer
    );

BOOL __stdcall
WinUsb_SetCurrentAlternateSetting(
    IN  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    IN  UCHAR SettingNumber
    );

BOOL __stdcall
WinUsb_GetCurrentAlternateSetting(
    IN  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    OUT PUCHAR SettingNumber
    );


BOOL __stdcall
WinUsb_QueryPipe(
    IN  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    IN  UCHAR AlternateInterfaceNumber,
    IN  UCHAR PipeIndex,
    OUT PWINUSB_PIPE_INFORMATION PipeInformation
    );


BOOL __stdcall
WinUsb_SetPipePolicy(
    IN  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    IN  UCHAR PipeID,
    IN  ULONG PolicyType,
    IN  ULONG ValueLength,
    IN  PVOID Value
    );

BOOL __stdcall
WinUsb_GetPipePolicy(
    IN WINUSB_INTERFACE_HANDLE InterfaceHandle,
    IN UCHAR PipeID,
    IN ULONG PolicyType,
    IN OUT PULONG ValueLength,
    OUT PVOID Value
    );

BOOL __stdcall
WinUsb_ReadPipe(
    IN  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    IN  UCHAR PipeID,
    IN  PUCHAR Buffer,
    IN  ULONG BufferLength,
    OUT PULONG LengthTransferred,
    IN  LPOVERLAPPED Overlapped
    );

BOOL __stdcall
WinUsb_WritePipe(
    IN  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    IN  UCHAR PipeID,
    IN  PUCHAR Buffer,
    IN  ULONG BufferLength,
    OUT PULONG LengthTransferred,
    IN  LPOVERLAPPED Overlapped    
    );

BOOL __stdcall
WinUsb_ControlTransfer(
    IN  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    IN  WINUSB_SETUP_PACKET SetupPacket,
    IN  PUCHAR Buffer,
    IN  ULONG BufferLength,
    OUT PULONG LengthTransferred,
    IN  LPOVERLAPPED Overlapped    
    );

BOOL __stdcall
WinUsb_ResetPipe(
    IN  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    IN  UCHAR PipeID
    );

BOOL __stdcall
WinUsb_AbortPipe(
    IN  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    IN  UCHAR PipeID
    );

BOOL __stdcall
WinUsb_FlushPipe(
    IN  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    IN  UCHAR PipeID
    );

BOOL __stdcall
WinUsb_SetPowerPolicy(
    IN  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    IN  ULONG PolicyType,
    IN  ULONG ValueLength,
    IN  PVOID Value
    );

BOOL __stdcall
WinUsb_GetPowerPolicy(
    IN WINUSB_INTERFACE_HANDLE InterfaceHandle,
    IN ULONG PolicyType,
    IN OUT PULONG ValueLength,
    OUT PVOID Value
    );

BOOL __stdcall
WinUsb_GetOverlappedResult(
    IN WINUSB_INTERFACE_HANDLE InterfaceHandle,
    IN LPOVERLAPPED lpOverlapped,
    OUT LPDWORD lpNumberOfBytesTransferred,
    BOOL bWait
    );
    

PUSB_INTERFACE_DESCRIPTOR __stdcall
WinUsb_ParseConfigurationDescriptor(
    IN PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor,
    IN PVOID StartPosition,
    IN LONG InterfaceNumber,
    IN LONG AlternateSetting,
    IN LONG InterfaceClass,
    IN LONG InterfaceSubClass,
    IN LONG InterfaceProtocol
    );

PUSB_COMMON_DESCRIPTOR __stdcall
WinUsb_ParseDescriptors(
    IN PVOID    DescriptorBuffer,
    IN ULONG    TotalLength,
    IN PVOID    StartPosition,
    IN LONG     DescriptorType
    );


#ifdef __cplusplus
}
#endif

                   
#endif //__WUSB_H__

