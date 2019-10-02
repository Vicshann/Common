
#pragma once

#include "Utils.h"
#include "FormatPE.h"
//====================================================================================
struct SKCbk
{
 enum ECbkVal {cbBefore,cbAfter,cbRetNow,cbContinue};
 int (_cdecl *pCallback)(void* This, void* ProcAddr, void* RetVal, int Value, ...);   // RetVal is used to store a return value or a temporary pointer to pass to between cbBefore and cbAfter
};
//====================================================================================
namespace NKRNL
{
#undef RtlCopyMemory
#undef RtlMoveMemory
#undef RtlFillMemory
#undef RtlZeroMemory

#include <ntddk.h>
//#include <basetsd.h>
//#include <windef.h>
#include <usbdrivr.h>

#define RETADDR ::_ReturnAddress()         

struct SEXP
{
 static inline SKCbk* _fastcall DriverObjectToCbkPtr(PDRIVER_OBJECT DrvObj){return (SKCbk*)DrvObj->FastIoDispatch;}
 static inline SKCbk* _fastcall DeviceObjectToCbkPtr(PDEVICE_OBJECT DevObj){return DriverObjectToCbkPtr(DevObj->DriverObject);}
//------------------------------------------------------------------------------------
//                                  NTOSKRNL.exe
//------------------------------------------------------------------------------------
static LONG FASTCALL ObfDereferenceObject(PVOID Object)
{
 DBGMSG("***: %p",RETADDR);
 return 0;  // ????????? 
}
//------------------------------------------------------------------------------------
static LONG FASTCALL ObfReferenceObject(PVOID Object)
{
 DBGMSG("***: %p",RETADDR);
 return 0;  // ????????? 
}
//------------------------------------------------------------------------------------
static VOID FASTCALL ExAcquireFastMutex(PFAST_MUTEX FastMutex)
{
 DBGMSG("WARNING!!!: %p",RETADDR); 
}
//------------------------------------------------------------------------------------
static VOID FASTCALL ExReleaseFastMutex(PFAST_MUTEX FastMutex)
{
 DBGMSG("WARNING!!!: %p",RETADDR); 
}
//------------------------------------------------------------------------------------
static PVOID _stdcall ExAllocatePoolWithTag(POOL_TYPE PoolType, SIZE_T NumberOfBytes, ULONG Tag)
{
 DBGMSG("***: %p",RETADDR); 
 return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, NumberOfBytes);
}
//------------------------------------------------------------------------------------
static VOID _stdcall ExFreePoolWithTag(PVOID P, ULONG Tag)
{  
 DBGMSG("***: %p, %p",RETADDR, P);              
 HeapFree(GetProcessHeap(), 0, P);
}
//------------------------------------------------------------------------------------
static VOID _stdcall _ExFreePool(PVOID P)     //  Name chenged because of 'define' to ExFreePoolWithTag (POOL_TAGGING)
{
 DBGMSG("***: %p",RETADDR); 
 ExFreePoolWithTag(P, 0);
}
//------------------------------------------------------------------------------------
static PDEVICE_OBJECT _stdcall IoAttachDeviceToDeviceStack(PDEVICE_OBJECT SourceDevice, PDEVICE_OBJECT TargetDevice)
{
 DBGMSG("!!!: %p",RETADDR);
 return TargetDevice;
}
//------------------------------------------------------------------------------------
static PIRP _stdcall IoBuildSynchronousFsdRequest(ULONG MajorFunction, PDEVICE_OBJECT DeviceObject, PVOID Buffer, ULONG Length, PLARGE_INTEGER StartingOffset, PKEVENT Event, PIO_STATUS_BLOCK IoStatusBlock)
{
 DBGMSG("***: %p",RETADDR);  
 return NULL;
}
//------------------------------------------------------------------------------------
static PIRP _stdcall IoBuildDeviceIoControlRequest(ULONG IoControlCode, PDEVICE_OBJECT DeviceObject, PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength, BOOLEAN InternalDeviceIoControl, PKEVENT Event, PIO_STATUS_BLOCK IoStatusBlock)
{
 DBGMSG("!!!: %p",RETADDR);
 PIRP NewIRP = (PIRP)ExAllocatePoolWithTag(NonPagedPool, sizeof(IRP), 0);             
 if(!NewIRP)return NULL;
 PIO_STACK_LOCATION NewIoSackPair = (PIO_STACK_LOCATION)ExAllocatePoolWithTag(NonPagedPool, sizeof(IO_STACK_LOCATION)*2, 0); // 2 is enough????? 
 NewIRP->Tail.Overlay.CurrentStackLocation = &NewIoSackPair[1];    
 PIO_STACK_LOCATION NextIrpStack  = IoGetNextIrpStackLocation(NewIRP);  
 NextIrpStack->MajorFunction = (InternalDeviceIoControl)?(IRP_MJ_INTERNAL_DEVICE_CONTROL):(IRP_MJ_DEVICE_CONTROL);    
 NextIrpStack->Parameters.DeviceIoControl.IoControlCode      = IoControlCode;
 NextIrpStack->Parameters.DeviceIoControl.InputBufferLength  = InputBufferLength;
 NextIrpStack->Parameters.DeviceIoControl.OutputBufferLength = OutputBufferLength;
 switch(IoControlCode & 3)     // TransferType
  {                                                                                         
   case METHOD_BUFFERED:  
    {
     DBGMSG("METHOD_BUFFERED");
     ULONG BufLen = (OutputBufferLength > InputBufferLength)?(OutputBufferLength):(InputBufferLength);
     if(BufLen)
      {                                                                        
       NewIRP->AssociatedIrp.SystemBuffer = ExAllocatePoolWithTag(NonPagedPool, BufLen, 0);     
       if(!NewIRP->AssociatedIrp.SystemBuffer){ExFreePoolWithTag(NewIRP,0); ExFreePoolWithTag(NewIoSackPair,0); return NULL;}
       if(InputBuffer)memcpy(NewIRP->AssociatedIrp.SystemBuffer, InputBuffer, InputBufferLength);
       NewIRP->Flags = IRP_BUFFERED_IO|IRP_DEALLOCATE_BUFFER;
       if(OutputBuffer)NewIRP->Flags |= IRP_INPUT_OPERATION;   
       NewIRP->UserBuffer = OutputBuffer;
      }
       else {NewIRP->Flags = 0; NewIRP->UserBuffer = NULL;}
    }
    break;
   case METHOD_IN_DIRECT: 
   case METHOD_OUT_DIRECT: 
    {
     DBGMSG("%s",((IoControlCode & 3)>1)?("METHOD_OUT_DIRECT"):("METHOD_IN_DIRECT"));
     if(InputBuffer)
      {
       NewIRP->AssociatedIrp.SystemBuffer = (PIRP)ExAllocatePoolWithTag(NonPagedPool, InputBufferLength, 0);                  
       if(!NewIRP->AssociatedIrp.SystemBuffer){ExFreePoolWithTag(NewIRP,0); ExFreePoolWithTag(NewIoSackPair,0); return NULL;}
       memcpy(NewIRP->AssociatedIrp.SystemBuffer, InputBuffer, InputBufferLength);
       NewIRP->Flags = IRP_BUFFERED_IO|IRP_DEALLOCATE_BUFFER;
      }
       else NewIRP->Flags = 0;
     if(OutputBuffer)
      {
       NewIRP->MdlAddress = (PMDL)ExAllocatePoolWithTag(NonPagedPool, OutputBufferLength, 0);    
       if(!NewIRP->MdlAddress){ExFreePoolWithTag(NewIRP,0); ExFreePoolWithTag(NewIoSackPair,0); return NULL;}
      }
    }
    break;
   case METHOD_NEITHER: 
     DBGMSG("METHOD_NEITHER");
     NewIRP->UserBuffer = OutputBuffer;
     NextIrpStack->Parameters.DeviceIoControl.Type3InputBuffer = InputBuffer; 
    break;
  }

 NewIRP->UserIosb  = IoStatusBlock;
 NewIRP->UserEvent = Event;
 NewIRP->Tail.Overlay.Thread = (PETHREAD)GetCurrentThread();         // ?????????????????????? !!!!!!!!!!!!!!!!!!!!!!
 DBGMSG("Created IRP: %p",NewIRP);
 return NewIRP;
}
//------------------------------------------------------------------------------------
static NTSTATUS _stdcall IoCreateDevice(PDRIVER_OBJECT DriverObject, ULONG DeviceExtensionSize, PUNICODE_STRING DeviceName, DEVICE_TYPE DeviceType, ULONG DeviceCharacteristics, BOOLEAN Exclusive, PDEVICE_OBJECT* DeviceObject)
{
 DBGMSG("***: %p",RETADDR);
 *DeviceObject = (PDEVICE_OBJECT)ExAllocatePoolWithTag(NonPagedPool, sizeof(DEVICE_OBJECT), 0); 
 (*DeviceObject)->DeviceExtension = ExAllocatePoolWithTag(NonPagedPool, DeviceExtensionSize, 0);    
 (*DeviceObject)->DriverObject    = DriverObject;
 if(DriverObject->DeviceObject)
  {
   PDEVICE_OBJECT CurDevObj = DriverObject->DeviceObject;
   while(CurDevObj->NextDevice)CurDevObj = CurDevObj->NextDevice;
   CurDevObj->NextDevice = *DeviceObject;
  }
   else DriverObject->DeviceObject = *DeviceObject;

 PVOID  Value    = NULL;
 SKCbk* ThisCbk  = DriverObjectToCbkPtr(DriverObject);
 bool   CbkValid = ThisCbk && ThisCbk->pCallback;
 if(CbkValid)
  {
   if(ThisCbk->pCallback(ThisCbk, &IoCreateDevice, &Value, SKCbk::cbBefore, DriverObject, DeviceExtensionSize, DeviceName, DeviceType, DeviceCharacteristics, Exclusive, DeviceObject) == SKCbk::cbRetNow)return (NTSTATUS)Value;
  }
 DBGMSG("Done");
 return STATUS_SUCCESS;
}
//------------------------------------------------------------------------------------
static VOID _stdcall IoDeleteDevice(PDEVICE_OBJECT DeviceObject)
{
 DBGMSG("***: %p",RETADDR);  
 HeapFree(GetProcessHeap(), 0, DeviceObject->DeviceExtension);
 HeapFree(GetProcessHeap(), 0, DeviceObject);
}
//------------------------------------------------------------------------------------
static VOID _stdcall IoDetachDevice(PDEVICE_OBJECT TargetDevice)
{
 DBGMSG("!!!: %p",RETADDR); 
}
//------------------------------------------------------------------------------------
static PDEVICE_OBJECT _stdcall IoGetAttachedDeviceReference(PDEVICE_OBJECT DeviceObject)
{
 DBGMSG("!!!: %p",RETADDR);
 return NULL;
}
//------------------------------------------------------------------------------------
static PEPROCESS _stdcall IoGetCurrentProcess(void)
{
 static char cproc[0x1000];     // ????????????????????????
 DBGMSG("***: %p",RETADDR);
 lstrcpyA(cproc, "System");
 return (PEPROCESS)&cproc;
}
//------------------------------------------------------------------------------------
static VOID _stdcall IoInvalidateDeviceRelations(PDEVICE_OBJECT DeviceObject, DEVICE_RELATION_TYPE Type)
{
 DBGMSG("!!!: %p",RETADDR); 
}
//------------------------------------------------------------------------------------
static NTSTATUS _stdcall IoRegisterDeviceInterface(PDEVICE_OBJECT PhysicalDeviceObject, GUID *InterfaceClassGuid, PUNICODE_STRING ReferenceString, PUNICODE_STRING SymbolicLinkName)
{
 DBGMSG("!!!: %p",RETADDR);
 return STATUS_SUCCESS;
}
//------------------------------------------------------------------------------------
static VOID _stdcall IoRequestDeviceEject(PDEVICE_OBJECT PhysicalDeviceObject)
{
 DBGMSG("***: %p",RETADDR); 
}
//------------------------------------------------------------------------------------
static NTSTATUS _stdcall IoSetDeviceInterfaceState(PUNICODE_STRING SymbolicLinkName, BOOLEAN Enable)
{
 DBGMSG("!!!: %p",RETADDR);
 return STATUS_SUCCESS;
}
//------------------------------------------------------------------------------------
static NTSTATUS FASTCALL IofCallDriver(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
 DBGMSG("WARNING!!!: %p",RETADDR);
 PVOID  Value    = NULL;
 SKCbk* ThisCbk  = DeviceObjectToCbkPtr(DeviceObject);
 bool   CbkValid = ThisCbk && ThisCbk->pCallback;
 if(CbkValid)                                                                                                 
  {
   if(ThisCbk->pCallback(ThisCbk, &IofCallDriver, &Value, SKCbk::cbBefore, DeviceObject, Irp) == SKCbk::cbRetNow){DBGMSG("WARNING!!!: Skipping driver call with %p",DeviceObject->DriverObject); return (NTSTATUS)Value;}
  }
                   
 IoSetNextIrpStackLocation(Irp);         
 PIO_STACK_LOCATION NxtIrpStack = IoGetCurrentIrpStackLocation(Irp); 
 NxtIrpStack->DeviceObject = DeviceObject;
 NTSTATUS Res = DeviceObject->DriverObject->MajorFunction[NxtIrpStack->MajorFunction](DeviceObject, Irp);  
 DBGMSG("Result of Driver Call: %08X",Res);

 if(CbkValid)
  {
   if(ThisCbk->pCallback(ThisCbk, &IofCallDriver, &Value, SKCbk::cbAfter, DeviceObject, Irp, Res) == SKCbk::cbRetNow)return (NTSTATUS)Value;
  }
 return Res; 
}
//------------------------------------------------------------------------------------
static VOID FASTCALL IofCompleteRequest(PIRP Irp, CCHAR PriorityBoost)
{
 DBGMSG("***: %p",RETADDR);
 if(Irp)
  {
   if(Irp->Size)
    {
     HeapFree(GetProcessHeap(), 0, Irp->Tail.Overlay.CurrentStackLocation);
     HeapFree(GetProcessHeap(), 0, Irp);
    }
  }
}
//------------------------------------------------------------------------------------
static KIRQL _stdcall KeAcquireSpinLockRaiseToDpc(PKSPIN_LOCK SpinLock)
{
 DBGMSG("***: %p",RETADDR);
 PCRITICAL_SECTION pCritSec = (PCRITICAL_SECTION)*SpinLock;
 if(!*SpinLock)
  {
   pCritSec  = (PCRITICAL_SECTION)ExAllocatePoolWithTag(NonPagedPool, sizeof(CRITICAL_SECTION), 0);   
   InitializeCriticalSection(pCritSec);
   *SpinLock = (KSPIN_LOCK)pCritSec;
  }
 EnterCriticalSection(pCritSec);
 return 3;
}
//------------------------------------------------------------------------------------
static VOID _stdcall KeBugCheckEx(ULONG BugCheckCode, ULONG_PTR BugCheckParameter1, ULONG_PTR BugCheckParameter2, ULONG_PTR BugCheckParameter3, ULONG_PTR BugCheckParameter4)
{
 DBGMSG("***: %p",RETADDR);
}
//------------------------------------------------------------------------------------
static VOID _stdcall KeInitializeEvent(PRKEVENT Event, EVENT_TYPE Type, BOOLEAN State)
{
 DBGMSG("***: %p",RETADDR);
 *(HANDLE*)Event = CreateEventA(NULL, Type == 0, State, NULL);   
}
//------------------------------------------------------------------------------------
static LONG _stdcall KeResetEvent(PRKEVENT Event)
{
 DBGMSG("***: %p",RETADDR);
 BOOL res = WaitForSingleObject(*(HANDLE*)Event, 0) == 0;
 ResetEvent(*(HANDLE*)Event);   
 return res;
}
//------------------------------------------------------------------------------------
static LONG _stdcall KeSetEvent(PRKEVENT Event, KPRIORITY Increment, BOOLEAN Wait)
{
 DBGMSG("***: %p",RETADDR);
 SetEvent(*(HANDLE*)Event);
 return 0;
}
//------------------------------------------------------------------------------------
static VOID _stdcall KeClearEvent(PRKEVENT Event)
{
 DBGMSG("***: %p",RETADDR);
 ResetEvent(*(HANDLE*)Event);
}
//------------------------------------------------------------------------------------
static NTSTATUS _stdcall KeDelayExecutionThread(KPROCESSOR_MODE WaitMode, BOOLEAN Alertable, PLARGE_INTEGER Interval)
{
 DBGMSG("***: %p",RETADDR);
 Sleep(Interval->QuadPart);
 return STATUS_SUCCESS;
}
//------------------------------------------------------------------------------------
static VOID _stdcall KeEnterCriticalRegion(void) 
{
 DBGMSG("***: %p",RETADDR);
}
//------------------------------------------------------------------------------------
static VOID _stdcall KeLeaveCriticalRegion(void) 
{
 DBGMSG("***: %p",RETADDR);
}
//------------------------------------------------------------------------------------
static VOID _stdcall KeReleaseSpinLock(PKSPIN_LOCK SpinLock, KIRQL NewIrql)
{
 DBGMSG("***: %p",RETADDR);
 if(*SpinLock)LeaveCriticalSection((PCRITICAL_SECTION)*SpinLock);
}
//------------------------------------------------------------------------------------
static NTSTATUS _stdcall KeWaitForSingleObject(PVOID Object, KWAIT_REASON WaitReason, KPROCESSOR_MODE WaitMode, BOOLEAN Alertable, PLARGE_INTEGER Timeout)
{
 DBGMSG("***: %p",RETADDR);
 if(Timeout)WaitForSingleObjectEx(Object, Timeout->QuadPart, Alertable);
  else WaitForSingleObjectEx(Object, INFINITE, Alertable);
 return STATUS_SUCCESS;
}
//------------------------------------------------------------------------------------
static NTSTATUS _stdcall PoCallDriver(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
 DBGMSG("***: %p",RETADDR);
 return STATUS_SUCCESS;
}
//------------------------------------------------------------------------------------
static NTSTATUS _stdcall PoRequestPowerIrp(PDEVICE_OBJECT DeviceObject, UCHAR MinorFunction, POWER_STATE PowerState, PREQUEST_POWER_COMPLETE CompletionFunction, PVOID Context, PIRP *Irp)
{
 DBGMSG("***: %p",RETADDR);
 return STATUS_SUCCESS;
}
//------------------------------------------------------------------------------------
static POWER_STATE _stdcall PoSetPowerState(PDEVICE_OBJECT DeviceObject, POWER_STATE_TYPE Type, POWER_STATE State)
{
 POWER_STATE ps;
 DBGMSG("***: %p",RETADDR);
 ps.DeviceState = PowerDeviceUnspecified;        // ????????????????????
 return ps;
}
//------------------------------------------------------------------------------------
static VOID _stdcall PoStartNextPowerIrp(PIRP Irp)
{
 DBGMSG("***: %p",RETADDR);
}
//------------------------------------------------------------------------------------
//                                  USBD.SYS
//------------------------------------------------------------------------------------
static PURB _stdcall USBD_CreateConfigurationRequestEx(PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor, PUSBD_INTERFACE_LIST_ENTRY InterfaceList)
{
 DBGMSG("WARNING!!!: %p",RETADDR);
 UINT TotalEndps  = 0;
 UINT TotalDescrs = 0; 
 while(InterfaceList[TotalDescrs].InterfaceDescriptor)TotalEndps += InterfaceList[TotalDescrs++].InterfaceDescriptor->bNumEndpoints;     // Last entry is NULL
 ULONG UrbSize = ((sizeof(USBD_INTERFACE_INFORMATION) - sizeof(USBD_PIPE_INFORMATION)) * TotalDescrs) + (sizeof(USBD_PIPE_INFORMATION) * TotalEndps) + sizeof(URB);  
 PURB  NewURB  = (PURB)ExAllocatePoolWithTag(NonPagedPool, UrbSize+4096, 0x206B6444);   // ' kdD'
 if(!NewURB)return NULL;
 memset(NewURB, 0, sizeof(URB));      //  URB_FUNCTION_SELECT_CONFIGURATION is 0
 NewURB->UrbHeader.Length = UrbSize;  
 NewURB->UrbSelectConfiguration.ConfigurationDescriptor = ConfigurationDescriptor;
 PUSBD_INTERFACE_INFORMATION UsbIntfInf = &NewURB->UrbSelectConfiguration.Interface;
 for(UINT Idx=0;Idx < TotalDescrs;Idx++)
  {
   InterfaceList[Idx].Interface = UsbIntfInf;
   UsbIntfInf->InterfaceNumber  = InterfaceList[Idx].InterfaceDescriptor->bInterfaceNumber;
   UsbIntfInf->AlternateSetting = InterfaceList[Idx].InterfaceDescriptor->bAlternateSetting;
   UsbIntfInf->NumberOfPipes    = InterfaceList[Idx].InterfaceDescriptor->bNumEndpoints;                   
   UsbIntfInf->Length           = (sizeof(USBD_PIPE_INFORMATION) * UsbIntfInf->NumberOfPipes) + (sizeof(USBD_INTERFACE_INFORMATION) - sizeof(USBD_PIPE_INFORMATION));   
   for(ULONG ctr=0;ctr < UsbIntfInf->NumberOfPipes;ctr++)UsbIntfInf->Pipes[ctr].MaximumTransferSize = 0x1000;
   UsbIntfInf = (PUSBD_INTERFACE_INFORMATION)((PBYTE)UsbIntfInf + UsbIntfInf->Length);
  }
 return NewURB;
}
//------------------------------------------------------------------------------------
static PUSB_INTERFACE_DESCRIPTOR _stdcall USBD_ParseConfigurationDescriptorEx(PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor, PVOID StartPosition, LONG InterfaceNumber, LONG AlternateSetting, LONG InterfaceClass, LONG InterfaceSubClass, LONG InterfaceProtocol)
{
 DBGMSG("WARNING!!!: %p",RETADDR);
 return (ConfigurationDescriptor == StartPosition)?((PUSB_INTERFACE_DESCRIPTOR)((PBYTE)StartPosition + 9)):(NULL);      // NOTE: Incomplete!!!!!!!!!!!!!
}
//------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------

};  // SEXP
}    // NKRNL
//====================================================================================
//
//
//
//------------------------------------------------------------------------------------
struct SKBase: public SKCbk
{
 
//------------------------------------------------------------------------------------
// TODO: CurrentStackLocation should be persistent
static NTSTATUS IoControlDeviceObject(NKRNL::PDEVICE_OBJECT DevObj, NKRNL::PFILE_OBJECT FileObj, DWORD dwIoControlCode, LPVOID lpInBuffer, DWORD nInBufferSize, LPVOID lpOutBuffer, DWORD nOutBufferSize, LPDWORD lpBytesReturned)
{
 NKRNL::IO_STACK_LOCATION IoStack;
 NKRNL::IRP ReqIRP;
 
// DBGMSG("hDevice=%p, IoControlCode=%08X, InBufferSize=%08X, OutBufferSize=%08X:",hDevice,dwIoControlCode,nInBufferSize,nOutBufferSize);
 memset(&IoStack, 0, sizeof(IoStack));
 memset(&ReqIRP,  0, sizeof(ReqIRP));
 ReqIRP.Tail.Overlay.CurrentStackLocation = &IoStack;
 IoStack.FileObject = FileObj;

 void* TmpOutBuf = NULL;
 if((dwIoControlCode & 3) == METHOD_BUFFERED)
  {
   UINT BufSize = (nInBufferSize > nOutBufferSize)?(nInBufferSize):(nOutBufferSize); 
   if(BufSize)TmpOutBuf = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, BufSize + 8);    
   if(lpInBuffer)memcpy(TmpOutBuf, lpInBuffer, nInBufferSize);
   ReqIRP.AssociatedIrp.SystemBuffer = TmpOutBuf;
  }

 IoStack.Parameters.DeviceIoControl.IoControlCode      = dwIoControlCode;
 IoStack.Parameters.DeviceIoControl.InputBufferLength  = nInBufferSize;
 IoStack.Parameters.DeviceIoControl.OutputBufferLength = nOutBufferSize;
 IoStack.Parameters.DeviceIoControl.Type3InputBuffer   = lpInBuffer;
 if(lpBytesReturned)*lpBytesReturned = 0;
 NTSTATUS Res = DevObj->DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL](DevObj, &ReqIRP);
 if(lpBytesReturned)*lpBytesReturned = ReqIRP.IoStatus.Information;
 if(TmpOutBuf)
  {
   if(lpOutBuffer)memcpy(lpOutBuffer, TmpOutBuf, nOutBufferSize);
   HeapFree(GetProcessHeap(), 0, TmpOutBuf);
  }
// DBGMSG("Result=%08X:",Res);
 return Res;
}
//------------------------------------------------------------------------------------
// TODO: CurrentStackLocation should be persistent
static NTSTATUS OpenDeviceObject(NKRNL::PDEVICE_OBJECT DevObj, NKRNL::PFILE_OBJECT FileObj)
{
 NKRNL::IO_STACK_LOCATION IoStack; 
 NKRNL::IRP CrtIrp;

 memset(&IoStack, 0, sizeof(IoStack));
 memset(&CrtIrp,  0, sizeof(CrtIrp));
 CrtIrp.Tail.Overlay.CurrentStackLocation = &IoStack;
 IoStack.FileObject = FileObj;    
 return DevObj->DriverObject->MajorFunction[IRP_MJ_CREATE](DevObj, &CrtIrp);
}
//------------------------------------------------------------------------------------
// On x64 drivers access KUSER_SHARED_DATA directly by a pointer
//
static void FixX64_KUSER_SHARED_DATA(HMODULE hMod)
{
 static BYTE FakeUserData[0x400];
 PBYTE CurPtr = (PBYTE)hMod;
 PBYTE EndPtr = &CurPtr[GetRealModuleSize(CurPtr) - 16];
 memset(&FakeUserData, 0xFF, sizeof(FakeUserData));      // OK
 while(CurPtr < EndPtr)
  {
   if(((*(PWORD)CurPtr & 0xF0FF) == 0xB048) && (*(PUINT64)(CurPtr + 2) & 0xFFFFFFFFFFFFF000ui64) == 0xFFFFF78000000000ui64)
    {
     *(PUINT64)(CurPtr + 2) = (UINT64)&FakeUserData;   // Replace KUSER_SHARED_DATA address
     DBGMSG("Module=%p, Addr=%p",hMod, CurPtr);      
    }
   CurPtr++;
  }
}
//------------------------------------------------------------------------------------
static UINT RedirectModuleImports(HMODULE hMod, SImportRec* Table)     // NOTE: Unoptimized!  // TODO: Optimize and move to FormatPE.h
{
static SImportRec ImportList[] = {
// Replaced USB imports
   {NULL, "USBD.SYS", "USBD_ParseConfigurationDescriptorEx", &NKRNL::SEXP::USBD_ParseConfigurationDescriptorEx},
   {NULL, "USBD.SYS", "USBD_CreateConfigurationRequestEx", &NKRNL::SEXP::USBD_CreateConfigurationRequestEx},
// Replaced Kernel imports
   {NULL, "NTOSKRNL.exe", "KeReleaseSpinLock", &NKRNL::SEXP::KeReleaseSpinLock},
   {NULL, "NTOSKRNL.exe", "KeAcquireSpinLockRaiseToDpc", &NKRNL::SEXP::KeAcquireSpinLockRaiseToDpc},
   {NULL, "NTOSKRNL.exe", "IoAttachDeviceToDeviceStack", &NKRNL::SEXP::IoAttachDeviceToDeviceStack},
   {NULL, "NTOSKRNL.exe", "IoDeleteDevice", &NKRNL::SEXP::IoDeleteDevice},
   {NULL, "NTOSKRNL.exe", "IoDetachDevice", &NKRNL::SEXP::IoDetachDevice},
   {NULL, "NTOSKRNL.exe", "IoRegisterDeviceInterface", &NKRNL::SEXP::IoRegisterDeviceInterface},
   {NULL, "NTOSKRNL.exe", "KeSetEvent", &NKRNL::SEXP::KeSetEvent},
   {NULL, "NTOSKRNL.exe", "ExFreePool", &NKRNL::SEXP::_ExFreePool},
   {NULL, "NTOSKRNL.exe", "ExAllocatePoolWithTag", &NKRNL::SEXP::ExAllocatePoolWithTag},
   {NULL, "NTOSKRNL.exe", "IofCompleteRequest", &NKRNL::SEXP::IofCompleteRequest},
   {NULL, "NTOSKRNL.exe", "IoSetDeviceInterfaceState", &NKRNL::SEXP::IoSetDeviceInterfaceState},
   {NULL, "NTOSKRNL.exe", "KeWaitForSingleObject", &NKRNL::SEXP::KeWaitForSingleObject},
   {NULL, "NTOSKRNL.exe", "IofCallDriver", &NKRNL::SEXP::IofCallDriver},
   {NULL, "NTOSKRNL.exe", "KeInitializeEvent", &NKRNL::SEXP::KeInitializeEvent},
   {NULL, "NTOSKRNL.exe", "PoCallDriver", &NKRNL::SEXP::PoCallDriver},
   {NULL, "NTOSKRNL.exe", "PoStartNextPowerIrp", &NKRNL::SEXP::PoStartNextPowerIrp},
   {NULL, "NTOSKRNL.exe", "PoSetPowerState", &NKRNL::SEXP::PoSetPowerState},
   {NULL, "NTOSKRNL.exe", "PoRequestPowerIrp", &NKRNL::SEXP::PoRequestPowerIrp},
   {NULL, "NTOSKRNL.exe", "KeClearEvent", &NKRNL::SEXP::KeClearEvent},
   {NULL, "NTOSKRNL.exe", "IoCreateDevice", &NKRNL::SEXP::IoCreateDevice},
   {NULL, "NTOSKRNL.exe", "KeResetEvent", &NKRNL::SEXP::KeResetEvent},
   {NULL, "NTOSKRNL.exe", "IoBuildDeviceIoControlRequest", &NKRNL::SEXP::IoBuildDeviceIoControlRequest},
   {NULL, "NTOSKRNL.exe", "KeDelayExecutionThread", &NKRNL::SEXP::KeDelayExecutionThread},
   {NULL, "NTOSKRNL.exe", "KeBugCheckEx", &NKRNL::SEXP::KeBugCheckEx},
   {NULL, "NTOSKRNL.exe", "IoInvalidateDeviceRelations", &NKRNL::SEXP::IoInvalidateDeviceRelations},
   {NULL, "NTOSKRNL.exe", "IoGetCurrentProcess", &NKRNL::SEXP::IoGetCurrentProcess},
   {NULL, "NTOSKRNL.exe", "ExReleaseFastMutex", &NKRNL::SEXP::ExReleaseFastMutex},
   {NULL, "NTOSKRNL.exe", "KeLeaveCriticalRegion", &NKRNL::SEXP::KeLeaveCriticalRegion},
   {NULL, "NTOSKRNL.exe", "ExAcquireFastMutex", &NKRNL::SEXP::ExAcquireFastMutex},
   {NULL, "NTOSKRNL.exe", "IoRequestDeviceEject", &NKRNL::SEXP::IoRequestDeviceEject},
   {NULL, "NTOSKRNL.exe", "KeEnterCriticalRegion", &NKRNL::SEXP::KeEnterCriticalRegion},
   {NULL, "NTOSKRNL.exe", "ObfReferenceObject", &NKRNL::SEXP::ObfReferenceObject},
   {NULL, "NTOSKRNL.exe", "IoBuildSynchronousFsdRequest", &NKRNL::SEXP::IoBuildSynchronousFsdRequest},
   {NULL, "NTOSKRNL.exe", "IoGetAttachedDeviceReference", &NKRNL::SEXP::IoGetAttachedDeviceReference},
   {NULL, "NTOSKRNL.exe", "ObfDereferenceObject", &NKRNL::SEXP::ObfDereferenceObject},
   {NULL, "NTOSKRNL.exe", "ExFreePoolWithTag", &NKRNL::SEXP::ExFreePoolWithTag},
// Redirected imports
   {"ntdll.dll", "NTOSKRNL.exe", "RtlFreeUnicodeString", NULL},
   {"ntdll.dll", "NTOSKRNL.exe", "RtlCopyUnicodeString", NULL},
   {"ntdll.dll", "NTOSKRNL.exe", "RtlAnsiStringToUnicodeString", NULL},
   {"ntdll.dll", "NTOSKRNL.exe", "RtlInitAnsiString", NULL},
   {"ntdll.dll", "NTOSKRNL.exe", "RtlInitUnicodeString", NULL},
   {"ntdll.dll", "NTOSKRNL.exe", "RtlFreeAnsiString", NULL},
   {"ntdll.dll", "NTOSKRNL.exe", "RtlUnicodeStringToAnsiString", NULL},
   {"ntdll.dll", "NTOSKRNL.exe", "RtlQueryRegistryValues", NULL},
   {"ntdll.dll", "NTOSKRNL.exe", "strncmp", NULL},
   {"ntdll.dll", "NTOSKRNL.exe", "swprintf", NULL},
   {"ntdll.dll", "NTOSKRNL.exe", "ZwQueryValueKey", NULL},
   {"ntdll.dll", "NTOSKRNL.exe", "ZwClose", NULL},
   {"ntdll.dll", "NTOSKRNL.exe", "ZwEnumerateKey", NULL},
   {"ntdll.dll", "NTOSKRNL.exe", "ZwQueryKey", NULL},
   {"ntdll.dll", "NTOSKRNL.exe", "ZwOpenKey", NULL},
   {NULL, NULL, NULL, NULL}
 };

 if(!Table)
  {
   static bool DoOnce = true;
   Table = ImportList;
   if(DoOnce)
    {
     DoOnce = false;
     for(int ctr=0;ImportList[ctr].ProcName;ctr++)
      {
       if(ImportList[ctr].Pointer || !ImportList[ctr].RealLibName)continue;      // Local function
       HMODULE hLib = GetModuleHandleA(ImportList[ctr].RealLibName);
       if(!hLib)hLib = LoadLibraryA(ImportList[ctr].RealLibName);
       if(!hLib){DBGMSG("Failed to load: %s", ImportList[ctr].RealLibName); continue;}
       ImportList[ctr].Pointer = GetProcAddress(hLib, ImportList[ctr].ProcName);
       DBGMSG("Imported: LibName=%s(%s), ProcName=%s, Pointer=%p", ImportList[ctr].RealLibName,ImportList[ctr].LibName, ImportList[ctr].ProcName, ImportList[ctr].Pointer);
      }
    }
  }

 UINT Total = 0;
 for(int ctr=0;Table[ctr].ProcName;ctr++)
  {
   PVOID Prev = ResolveImportRecord(hMod, &Table[ctr]); 
   DBGMSG("Resolved: LibName=%s, ProcName=%s, Pointer=%p, Prev=%p", Table[ctr].LibName, Table[ctr].ProcName, Table[ctr].Pointer, Prev);
   Total += (bool)Prev;
  }
 return Total;
}
//------------------------------------------------------------------------------------

};
//====================================================================================

