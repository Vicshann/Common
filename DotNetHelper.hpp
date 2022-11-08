
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

#include <metahost.h>
//#include "ThirdParty\DotNet\mscoree.h"       // #include <mscoree.h>

#pragma comment(lib, "MSCorEE.lib")

#import "C:\Windows\Microsoft.NET\Framework\v2.0.50727\mscorlib.tlb" raw_interfaces_only \
    high_property_prefixes("_get","_put","_putref")		\
    rename("ReportEvent", "InteropServices_ReportEvent")

#ifdef _AMD64_ 
static void __cdecl _com_issue_error(long){}
#endif
//---------------------------------------------------------------------------
namespace NDotNetHlp
{

EXTERN_GUID(IID_AppDomain, 0X05F696DC, 0X2B29, 0X3663,   0XAD, 0X8B, 0XC4, 0X38, 0X9C, 0XF2, 0XA7, 0X13);    // __uuidof(_AppDomain);




//using namespace mscorlib;

typedef interface ICLRPrivAssembly ICLRPrivAssembly;
typedef interface ICLRPrivBinder ICLRPrivBinder;
typedef interface ICLRPrivBinder ICLRPrivBinder;

struct IAssemblyName;

    MIDL_INTERFACE("2601F621-E462-404C-B299-3E1DE72F8547")
    ICLRPrivResource : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetResourceType( 
            /* [retval][out] */ IID *pIID) = 0;
        
};

    MIDL_INTERFACE("5653946E-800B-48B7-8B09-B1B879B54F68")
    ICLRPrivAssemblyInfo : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetAssemblyName( 
            /* [in] */ DWORD cchBuffer,
            /* [out] */ LPDWORD pcchBuffer,
            /* [optional][string][out] */ LPWSTR wzBuffer) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAssemblyVersion( 
            /* [out] */ USHORT *pMajor,
            /* [out] */ USHORT *pMinor,
            /* [out] */ USHORT *pBuild,
            /* [out] */ USHORT *pRevision) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAssemblyPublicKey( 
            /* [in] */ DWORD cbBuffer,
            /* [out] */ LPDWORD pcbBuffer,
            /* [optional][length_is][size_is][out] */ BYTE *pbBuffer) = 0;
        
};

    MIDL_INTERFACE("2601F621-E462-404C-B299-3E1DE72F8542")
    ICLRPrivBinder : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE BindAssemblyByName( 
            /* [in] */ IAssemblyName *pAssemblyName,
            /* [retval][out] */ ICLRPrivAssembly **ppAssembly) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE VerifyBind( 
            /* [in] */ IAssemblyName *AssemblyName,
            /* [in] */ ICLRPrivAssembly *pAssembly,
            /* [in] */ ICLRPrivAssemblyInfo *pAssemblyInfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBinderFlags( 
            /* [retval][out] */ DWORD *pBinderFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBinderID( 
            /* [retval][out] */ UINT_PTR *pBinderId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FindAssemblyBySpec( 
            /* [in] */ LPVOID pvAppDomain,
            /* [in] */ LPVOID pvAssemblySpec,
            /* [out] */ HRESULT *pResult,
            /* [out] */ ICLRPrivAssembly **ppAssembly) = 0;
        
};

    MIDL_INTERFACE("2601F621-E462-404C-B299-3E1DE72F8543")
    ICLRPrivAssembly : public ICLRPrivBinder
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE IsShareable( 
            /* [retval][out] */ BOOL *pbIsShareable) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAvailableImageTypes( 
            /* [retval][out] */ LPDWORD pdwImageTypes) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetImageResource( 
            /* [in] */ DWORD dwImageType,
            /* [out] */ DWORD *pdwImageType,
            /* [retval][out] */ ICLRPrivResource **ppIResource) = 0;
        
};

    MIDL_INTERFACE("BC1B53A8-DCBC-43B2-BB17-1E4061447AE9")
    ICLRPrivRuntime : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetInterface( 
            /* [in] */ REFCLSID rclsid,
            /* [in] */ REFIID riid,
            /* [retval][iid_is][out] */ LPVOID *ppUnk) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateAppDomain( 
            /* [string][in] */ LPCWSTR pwzFriendlyName,
            /* [in] */ ICLRPrivBinder *pBinder,
            /* [retval][out] */ LPDWORD pdwAppDomainId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateDelegate( 
            /* [in] */ DWORD appDomainID,
            /* [string][in] */ LPCWSTR wszAssemblyName,
            /* [string][in] */ LPCWSTR wszClassName,
            /* [string][in] */ LPCWSTR wszMethodName,
            /* [retval][out] */ LPVOID *ppvDelegate) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ExecuteMain( 
            /* [in] */ ICLRPrivBinder *pBinder,
            /* [retval][out] */ int *pRetVal) = 0;
        
};


//===========================================================================
// mscoree.dll -> mscoreei.dll -> clr.dll
//
class CDotNetHost
{
 bool Initialized;
 ICLRMetaHost    *pMetaHost;      // ExitProcess
 ICorRuntimeHost *pRuntimeHost;                         // ICLRRuntimeHost *pRuntimeHost;   // ExecuteInDefaultAppDomain
 ICLRRuntimeInfo *pRuntimeInfo;   // GetProcAddress, LoadLibrary
 ICLRPrivRuntime *pPrivRuntime;   // ???
 mscorlib::_AppDomainPtr pDefaultAppDomain;
 mscorlib::_AssemblyPtr pAssembly;

//---------------------------------------------------------------------------
void ReleaseInterfaces(void)
{
 if(this->pAssembly)this->pAssembly->Release(); 
 if(this->pDefaultAppDomain)this->pDefaultAppDomain->Release(); 
 if(this->pRuntimeHost)this->pRuntimeHost->Release(); 
 if(this->pRuntimeInfo)this->pRuntimeInfo->Release(); 
 if(this->pMetaHost)this->pMetaHost->Release();
 this->pMetaHost    = nullptr;    
 this->pRuntimeHost = nullptr;
 this->pRuntimeInfo = nullptr;
 this->pDefaultAppDomain = nullptr;
 this->pAssembly = nullptr;
 DBGMSG("Done"); 
}
//---------------------------------------------------------------------------


public:
//---------------------------------------------------------------------------
CDotNetHost(void)
{
 this->Initialized  = false;
 this->pMetaHost    = nullptr;    
 this->pRuntimeHost = nullptr;
 this->pRuntimeInfo = nullptr; 
 this->pDefaultAppDomain = nullptr;  
 this->pAssembly = nullptr; 
}
//---------------------------------------------------------------------------
int ReleaseDotNET(PWSTR RTVervoid)
{
 if(!this->Initialized)return 1;
 this->ReleaseInterfaces();
 this->Initialized  = false;
 return 0;
}
//---------------------------------------------------------------------------
// http://www.rohitab.com/discuss/topic/43071-loading-net-pe-into-memory-from-native-process/ 
// https://github.com/mjrousos/SampleCoreCLRHost/blob/master/HostWithMscoree/host.cpp
// RTVer example: "v4.0.30319"
// CoreClr default path: %programfiles%\dotnet\shared\Microsoft.NETCore.App\
//
int InitDotNET(PWSTR RTVer)
{
 HRESULT hr;
 if(this->Initialized)return 1;

 hr = CLRCreateInstance(CLSID_CLRMetaHost, IID_ICLRMetaHost, (LPVOID*)&this->pMetaHost);     // S_OK
 if(FAILED(hr)){DBGMSG("CLRCreateInstance failed with %08X",hr); return -1;}
 hr = this->pMetaHost->GetRuntime(RTVer, IID_PPV_ARGS(&this->pRuntimeInfo));
 if(FAILED(hr)){DBGMSG("GetRuntime %ls failed with %08X",RTVer, hr); this->ReleaseInterfaces(); return -2;}

 BOOL bCLRStatus = 0;
 hr = this->pRuntimeInfo->IsLoadable(&bCLRStatus);
 if(FAILED(hr) || !bCLRStatus){DBGMSG("Runtime %ls failed withis not loadable %08X", RTVer, hr); this->ReleaseInterfaces(); return -3;}

 hr = this->pRuntimeInfo->IsLoaded(GetCurrentProcess(), &bCLRStatus);
 if(FAILED(hr)){DBGMSG("Failed to get status of runtime %ls: %08X", RTVer, hr); this->ReleaseInterfaces(); return -4;} 

 if(bCLRStatus){DBGMSG("Runtime %ls is already loaded", RTVer); }

 hr = pRuntimeInfo->GetInterface(CLSID_CLRRuntimeHost, IID_ICLRRuntimeHost, (void**)&this->pRuntimeHost);    // Cannot create ICLRPrivRuntime directly   //IID_PPV_ARGS(&this->pRuntimeHost));    // CorHost2::ICLRRuntimeHost vtable from clr.dll
 if(FAILED(hr)){DBGMSG("Failed to load runtime %ls: %08X", RTVer, hr); this->ReleaseInterfaces(); return -5;} 
    
 DWORD StartupFlags = 0;
 hr = this->pRuntimeInfo->IsStarted(&bCLRStatus, &StartupFlags);
 if(FAILED(hr)){DBGMSG("Failed to get status of runtime %ls: %08X", RTVer, hr); this->ReleaseInterfaces(); return -4;} 

 if(!bCLRStatus)
  {
   hr = this->pRuntimeHost->Start();
   if(FAILED(hr)){DBGMSG("Failed to start runtime %ls: %08X", RTVer, hr); this->ReleaseInterfaces(); return -5;} 
  }
   else { DBGMSG("Runtime %ls is already started", RTVer); }
  
 // See 'clr.dll :: CorHost2 *__fastcall CorHost2::CorHost2(CorHost2 *__hidden this)'
 // *((_QWORD *)this + 0) = &CorHost2::`vftable'{for `IPrivateManagedExceptionReporting'};
 // *((_QWORD *)this + 1) = &CorHost2::`vftable'{for `CorThreadpool'};
 // *((_QWORD *)this + 2) = &CorHost2::`vftable'{for `CorGCHost'};
 // *((_QWORD *)this + 3) = &CorHost2::`vftable'{for `CorConfiguration'};
 // *((_QWORD *)this + 4) = &CorHost2::`vftable'{for `CLRValidator'};
 // *((_QWORD *)this + 5) = &CorHost2::`vftable'{for `CorDebuggerInfo'};
 // *((_QWORD *)this + 6) = &CorHost2::`vftable'{for `ICLRRuntimeHost'};     // <<< We got this (pRuntimeHost points to it)
 // *((_QWORD *)this + 7) = &CorHost2::`vftable'{for `ICLRPrivRuntime'};     // <<< We need this
 // *((_QWORD *)this + 8) = &CorHost2::`vftable'{for `CorExecutionManager'};
 this->pPrivRuntime = (ICLRPrivRuntime*)&((void**)this->pRuntimeHost)[1];
   
 PVOID Ress = 0;
 hr = this->pPrivRuntime->CreateDelegate(1, L"???", L"type", L"proc", &Ress);




 // clr.dll HardCodedMetaSig

   //  pCLRRuntimeHost->UnloadAppDomain(domainId, true);

  //  pCLRRuntimeHost->Stop();

 this->Initialized = true;
 return 0;
}
//---------------------------------------------------------------------------
// https://github.com/etormadiv/HostingCLR/blob/master/HostingCLR/HostingCLR.cpp#L10
//Info: Assemblies that are calling Assembly.GetEntryAssembly() method will fail to load if exceptions are not catched!
//Info: Assembly.GetEntryAssembly() method will return null if loaded by this code, se it should NOT be used
//Info: Please fix any code that use Assembly.GetEntryAssembly() and change it typeof(MyType).Assembly instead
//Info: Please read https://msdn.microsoft.com/en-us/library/system.reflection.assembly.getentryassembly(v=vs.110).aspx#Anchor_1
//	
int Initialize(PWSTR RTVer)
{
 HRESULT hr;
 if(this->Initialized)return 1;

 hr = CLRCreateInstance(CLSID_CLRMetaHost, IID_ICLRMetaHost, (LPVOID*)&this->pMetaHost);     // S_OK
 if(FAILED(hr)){DBGMSG("CLRCreateInstance failed with %08X",hr); return -1;}
 hr = this->pMetaHost->GetRuntime(RTVer, IID_PPV_ARGS(&this->pRuntimeInfo));
 if(FAILED(hr)){DBGMSG("GetRuntime %ls failed with %08X",RTVer, hr); this->ReleaseInterfaces(); return -2;}

 BOOL bCLRStatus = 0;
 hr = this->pRuntimeInfo->IsLoadable(&bCLRStatus);
 if(FAILED(hr) || !bCLRStatus){DBGMSG("Runtime %ls failed withis not loadable %08X", RTVer, hr); this->ReleaseInterfaces(); return -3;}

 hr = this->pRuntimeInfo->IsLoaded(GetCurrentProcess(), &bCLRStatus);
 if(FAILED(hr)){DBGMSG("Failed to get status of runtime %ls: %08X", RTVer, hr); this->ReleaseInterfaces(); return -4;} 

 if(bCLRStatus){DBGMSG("Runtime %ls is already loaded", RTVer); }
         
 hr = pRuntimeInfo->GetInterface(CLSID_CorRuntimeHost, IID_ICorRuntimeHost, (void**)&this->pRuntimeHost);    // Cannot create ICLRPrivRuntime directly   //IID_PPV_ARGS(&this->pRuntimeHost));    // CorHost2::ICLRRuntimeHost vtable from clr.dll
 if(FAILED(hr)){DBGMSG("Failed to load runtime %ls: %08X", RTVer, hr); this->ReleaseInterfaces(); return -5;} 
                                                                    // 
 DWORD StartupFlags = 0;
 hr = this->pRuntimeInfo->IsStarted(&bCLRStatus, &StartupFlags);
 if(FAILED(hr)){DBGMSG("Failed to get status of runtime %ls: %08X", RTVer, hr); this->ReleaseInterfaces(); return -4;} 

 if(!bCLRStatus)
  {
   hr = this->pRuntimeHost->Start();
   if(FAILED(hr)){DBGMSG("Failed to start runtime %ls: %08X", RTVer, hr); this->ReleaseInterfaces(); return -5;} 
  }
   else { DBGMSG("Runtime %ls is already started", RTVer); }


 IUnknownPtr pAppDomainThunk = nullptr;
 hr = pRuntimeHost->GetDefaultDomain(&pAppDomainThunk); // Relese it?
 if(FAILED(hr)){DBGMSG("GetDefaultDomain failed with %08X", hr); this->ReleaseInterfaces(); return -6;} 
 hr = pAppDomainThunk->QueryInterface(IID_AppDomain, (void**)&this->pDefaultAppDomain);   // Equivalent of System.AppDomain.CurrentDomain in C#
 if(FAILED(hr)){DBGMSG("QueryInterface for IID_AppDomain failed with %08X", hr); this->ReleaseInterfaces(); return -7;}     
      
 this->Initialized = true;
 return 0;
}
//---------------------------------------------------------------------------
int LoadAssembly(PVOID AsmFilePath) 
{
 HRESULT hr;

 HANDLE hFile;
 if(!((PBYTE)AsmFilePath)[1])hFile = CreateFileW((PWSTR)AsmFilePath,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL|FILE_FLAG_SEQUENTIAL_SCAN,NULL);
   else hFile = CreateFileA((LPSTR)AsmFilePath,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL|FILE_FLAG_SEQUENTIAL_SCAN,NULL);
 if(hFile == INVALID_HANDLE_VALUE)return -1;
 DWORD Result   = 0;
 DWORD FileSize = GetFileSize(hFile,NULL); 
 if(!FileSize)return -2;
 SAFEARRAYBOUND bnd = { FileSize, 0 };
 SAFEARRAY* pSafeArray = SafeArrayCreate(VT_UI1, 1, &bnd);     // new SAFEARRAYBOUND{ lenRawData , 0 }
 if(!pSafeArray){CloseHandle(hFile); return -3;}
 void* pvData = nullptr;
 hr = SafeArrayAccessData(pSafeArray, &pvData);
 if(FAILED(hr)){DBGMSG("SafeArrayAccessData failed with %08X", hr); SafeArrayDestroy(pSafeArray); CloseHandle(hFile); return -4;} 
 if(!ReadFile(hFile,pvData,FileSize,&Result,NULL) || !Result)return -5;
 CloseHandle(hFile);
 hr = SafeArrayUnaccessData(pSafeArray);
 if(FAILED(hr)){DBGMSG("SafeArrayAccessData failed with %08X", hr); SafeArrayDestroy(pSafeArray); return -6;} 
 
 if(this->pAssembly){this->pAssembly->Release(); this->pAssembly = nullptr;} 
 hr = this->pDefaultAppDomain->Load_3(pSafeArray, &this->pAssembly);
 SafeArrayDestroy(pSafeArray);
 if(FAILED(hr)){DBGMSG("raw_Load_3 failed with %08X", hr); return -7;} 
 return 0;
}
//---------------------------------------------------------------------------
int ExecAssembly(PWSTR AsmFilePath) 
{
 bstr_t bstrAssemblyName(AsmFilePath);
 HRESULT hr = this->pDefaultAppDomain->ExecuteAssembly(bstrAssemblyName, NULL, NULL);    // pCurrentDomain
 if(FAILED(hr)){DBGMSG("ExecuteAssembly failed with %08X", hr); return -1;} 
 return 0;
}
//---------------------------------------------------------------------------
SAFEARRAY* newArguments(int argc, wchar_t** argv) 
{
	VARIANT args;
	args.vt = VT_ARRAY | VT_BSTR;
	args.parray = SafeArrayCreate(VT_BSTR, 1, new SAFEARRAYBOUND{ ULONG(argc) , 0 });
	for (int i = 0; i < argc; i++) SafeArrayPutElement(args.parray, (LONG*)&i, SysAllocString(argv[i]));

	SAFEARRAY* params = SafeArrayCreate(VT_VARIANT, 1, new SAFEARRAYBOUND{ 1, 0 });

	LONG indx = 0;
	SafeArrayPutElement(params, &indx, &args);
	return params;
}

int AsmExecEntryPoint(void)
{
 HRESULT hr;
 mscorlib::_MethodInfoPtr pMethodInfo = NULL;
 hr = this->pAssembly->get_EntryPoint(&pMethodInfo);
 if(FAILED(hr)){DBGMSG("get_EntryPoint failed with %08X", hr); return -1;} 
 VARIANT retVal = {0};	
 VARIANT obj = {0};	
 obj.vt = VT_NULL;

 SAFEARRAY *psaStaticMethodArgs = SafeArrayCreateVector(VT_VARIANT, 0, 1);  //TODO! Change cElement to the number of Main arguments
// wchar_t* Arg1 = L"Hello";
// SAFEARRAY *psaStaticMethodArgs = newArguments(1, &Arg1); 
       //Sleep(12000);
 hr = pMethodInfo->Invoke_3(obj, psaStaticMethodArgs, &retVal);   // EntryPoint.Invoke(null, new object[0]) 
 SafeArrayDestroy(psaStaticMethodArgs);


//    SAFEARRAY *pArrParams;
//    hr = entryp->GetParameters(&pArrParams);

// Load dotnet executable from file
//bstr_t bstrAssemblyName(L"C:\\dotnet.exe");
//hr = pCurrentDomain->ExecuteAssembly(bstrAssemblyName, NULL, NULL);
 return 0;
}
//---------------------------------------------------------------------------
// C:\Windows\Microsoft.NET\Framework64\v4.0.30319\
//     clr.dll
//     clrjit.dll
//     mscorlib.dll [.NET]
// 	
 // ReflectMethodObject       - The base object for the RuntimeMethodInfo class
// 
// Hook both CreateDelegate of DynamicMethod object
// Grab the byte array and sabotage the result that the delegate could not be actually called
// 
//int Execute()
//---------------------------------------------------------------------------
};
//===========================================================================


//---------------------------------------------------------------------------
//===========================================================================
//                           ClrJit
//---------------------------------------------------------------------------
#include "ThirdParty\DotNet\corjit.h"


/*typedef struct CORINFO_METHOD_STRUCT_*      CORINFO_METHOD_HANDLE;
typedef struct CORINFO_MODULE_STRUCT_*      CORINFO_MODULE_HANDLE;

enum CorInfoOptions
{
	CORINFO_OPT_INIT_LOCALS                 = 0x00000010, // zero initialize all variables
	CORINFO_GENERICS_CTXT_FROM_THIS         = 0x00000020, // is this shared generic code that access the generic context from the this pointer?  If so, then if the method has SEH then the 'this' pointer must always be reported and kept alive.
	CORINFO_GENERICS_CTXT_FROM_PARAMTYPEARG = 0x00000040, // is this shared generic code that access the generic context from the ParamTypeArg?  If so, then if the method has SEH then the 'ParamTypeArg' must always be reported and kept alive. Same as CORINFO_CALLCONV_PARAMTYPE
	CORINFO_GENERICS_CTXT_MASK              = (CORINFO_GENERICS_CTXT_FROM_THIS |
	CORINFO_GENERICS_CTXT_FROM_PARAMTYPEARG),
	CORINFO_GENERICS_CTXT_KEEP_ALIVE        = 0x00000080, // Keep the generics context alive throughout the method even if there is no explicit use, and report its location to the CLR
};

struct CORINFO_METHOD_INFO
{
	CORINFO_METHOD_HANDLE       ftn;
	CORINFO_MODULE_HANDLE       scope;
	BYTE *                      ILCode;
	unsigned                    ILCodeSize;
	unsigned short              maxStack;
	unsigned short              EHcount;
	CorInfoOptions              options;
};

typedef struct ILCodeBuffer
{
	LPBYTE						pBuffer;
	DWORD						dwSize;
	BOOL						bIsGeneric;
};  */
 


// clrjit.dll (.NET 4.0) ; mscorwks.dll (.NET 2.0+)
// ICorJitInfo* VFT is ??_7CEEJitInfo@@6B@ in clr.dll
static int    _stdcall CorJit_CompileMethod(struct CILJit* This, struct ICorJitInfo* pJitInfo, CORINFO_METHOD_INFO* pMethodInfo, UINT nFlags, LPBYTE* pEntryAddress, ULONG* pSizeOfCode);  // CorJitResult
const char*   _stdcall GetMethodName(ICorJitInfo* This, CORINFO_METHOD_HANDLE ftn, const char **moduleName);    // _stdcall or thiscall?    // TODO: Fix definition of ICorJitInfo interface in 'corinfo.h' because index of GetMethodName is wrong (.NET 4)
static void** _stdcall GetJit(void);
//---------------------------------------------------------------------------
static void** GetCompileMethodRef(void)
{
 HMODULE hModJit = GetModuleHandleA("clrjit.dll");
 if(!hModJit)hModJit = GetModuleHandleA("mscorjit.dll");
 if(!hModJit)return nullptr;  
 PVOID pGetJit = GetProcAddress(hModJit, "getJit");
 if(!pGetJit)return nullptr;  
 void** Ptr = ((decltype(GetJit)*)pGetJit)();        //It is OK to call it multiple times
 if(!Ptr)return nullptr; 
/* 
 NPEFMT::SECTION_HEADER* ResSec = nullptr;   
 if(!NPEFMT::GetModuleSection(hModJit, ".text", &ResSec))return nullptr;   
 PBYTE  TxtSecBase = (PBYTE)hModJit + ResSec->SectionRva;
 SIZE_T TxtSecSize = ResSec->PhysicalSize;
 if(!NPEFMT::GetModuleSection(hModJit, ".rdata", &ResSec))return nullptr;   
 PBYTE  DatSecBase = (PBYTE)hModJit + ResSec->SectionRva;
 SIZE_T DatSecSize = ResSec->PhysicalSize;

 PBYTE ProcPtr = (PBYTE)pGetJit;
 PVOID* IntfAddr = nullptr;     // It is INTERFACE, not VFT and THIS ptr is passed as an usual argument (stdcall on x32)
 for(UINT ctr=0;ctr < 32;ctr++)
  {
   PBYTE AVal = 

  }  */
 return &((void**)*Ptr)[0]; // Stored address of CILJit::compileMethod                              
}
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
}