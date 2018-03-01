
//====================================================================================
#pragma once
 
#ifndef LSP_H
#define LSP_H
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
 
#include <windows.h>
#include <ws2spi.h>
#include <Sporder.h>
#include <Rpc.h>	
//#include <Wincrypt.h>
//#include <Winsock2.h>
//#include <oleauto.h>
//#include <Security.h>

static void (_stdcall *pStartupCallback)(WORD, LPWSPDATA, LPWSAPROTOCOL_INFOW, WSPUPCALLTABLE, LPWSPPROC_TABLE);	

//====================================================================================
class CLSP
{

public:

//------------------------------------------------------------------------------------
static LPWSAPROTOCOL_INFOW _stdcall GetProvider(LPINT lpnTotalProtocols)
{
 DWORD dwSize = 0;
 int nError;
 LPWSAPROTOCOL_INFOW pProtoInfo = NULL;
    
 // Get the length you want
 if((::WSCEnumProtocols(NULL, pProtoInfo, &dwSize, &nError) == SOCKET_ERROR)&&(nError != WSAENOBUFS))return NULL;	
 pProtoInfo = (LPWSAPROTOCOL_INFOW)::GlobalAlloc(GPTR, dwSize);
 *lpnTotalProtocols = ::WSCEnumProtocols(NULL, pProtoInfo, &dwSize, &nError);
 return pProtoInfo;
}
//------------------------------------------------------------------------------------
static void _stdcall FreeProvider(LPWSAPROTOCOL_INFOW pProtoInfo)
{
 ::GlobalFree(pProtoInfo);
}
//------------------------------------------------------------------------------------
static BOOL _stdcall IsExistProvider(GUID* ProviderGuid)
{
 LPWSAPROTOCOL_INFOW pProtoInfo;
 int nProtocols;
 pProtoInfo = GetProvider(&nProtocols);
 for(int i=0; i<nProtocols; i++)    // Get the directory ID of the tiered protocol according to Guid
  {
   if(memcmp(ProviderGuid, &pProtoInfo[i].ProviderId, sizeof(ProviderGuid)) == 0)
    {
     FreeProvider(pProtoInfo);
     return TRUE;
    }
  }
 FreeProvider(pProtoInfo);
 return FALSE;
}
//------------------------------------------------------------------------------------
static BOOL _stdcall RemoveProvider(GUID* ProviderGuid)
{
 LPWSAPROTOCOL_INFOW pProtoInfo;
 int nProtocols;
 DWORD dwLayeredCatalogId;

 pProtoInfo = GetProvider(&nProtocols);  
 int nError;
 for(int i=0; i<nProtocols; i++)    // Get the directory ID of the tiered protocol according to Guid
  {
   if(memcmp(ProviderGuid, &pProtoInfo[i].ProviderId, sizeof(ProviderGuid)) == 0)
    {
     dwLayeredCatalogId = pProtoInfo[i].dwCatalogEntryId;
     for(int i=0; i<nProtocols; i++)  // Remove the protocol chain
      {
       if((pProtoInfo[i].ProtocolChain.ChainLen > 1) && (pProtoInfo[i].ProtocolChain.ChainEntries[0] == dwLayeredCatalogId))::WSCDeinstallProvider(&pProtoInfo[i].ProviderId, &nError);			
      }		
     ::WSCDeinstallProvider(ProviderGuid, &nError);  // Remove the tiered protocol
     FreeProvider(pProtoInfo);
     return TRUE;
    }
  }
 FreeProvider(pProtoInfo);
 return FALSE;
}
//------------------------------------------------------------------------------------
static BOOL _stdcall InstallProvider(WCHAR* wszLSPName, WCHAR* pwszPathName, GUID* ProviderGuid)
{
 LPWSAPROTOCOL_INFOW pProtoInfo;
 int nProtocols;
 WSAPROTOCOL_INFOW OriginalProtocolInfo[3];
 DWORD			 dwOrigCatalogId[3];
 int nArrayCount = 0;
 DWORD dwLayeredCatalogId;		// The level ID of our hierarchical protocol
 int nError;
	
 // find our lower protocol and put the information in the array // enumerate all service provider providers
 pProtoInfo = GetProvider(&nProtocols);
 BOOL bFindUdp = FALSE;
 BOOL bFindTcp = FALSE;
 BOOL bFindRaw = FALSE;
 int i;
 for(i=0; i<nProtocols; i++)
  {
   if(pProtoInfo[i].iAddressFamily == AF_INET)
    {
     if(!bFindUdp && pProtoInfo[i].iProtocol == IPPROTO_UDP)
      {
       memcpy(&OriginalProtocolInfo[nArrayCount], &pProtoInfo[i], sizeof(WSAPROTOCOL_INFOW));
       OriginalProtocolInfo[nArrayCount].dwServiceFlags1 = OriginalProtocolInfo[nArrayCount].dwServiceFlags1 & (~XP1_IFS_HANDLES); 				
       dwOrigCatalogId[nArrayCount++] = pProtoInfo[i].dwCatalogEntryId;
       bFindUdp = TRUE;
      }
     if(!bFindTcp && pProtoInfo[i].iProtocol == IPPROTO_TCP)
      {
       memcpy(&OriginalProtocolInfo[nArrayCount], &pProtoInfo[i], sizeof(WSAPROTOCOL_INFOW));
       OriginalProtocolInfo[nArrayCount].dwServiceFlags1 = OriginalProtocolInfo[nArrayCount].dwServiceFlags1 & (~XP1_IFS_HANDLES); 				
       dwOrigCatalogId[nArrayCount++] = pProtoInfo[i].dwCatalogEntryId;
       bFindTcp = TRUE;
      } 
     if(!bFindRaw && pProtoInfo[i].iProtocol == IPPROTO_IP)
      {
       memcpy(&OriginalProtocolInfo[nArrayCount], &pProtoInfo[i], sizeof(WSAPROTOCOL_INFOW));
       OriginalProtocolInfo[nArrayCount].dwServiceFlags1 = OriginalProtocolInfo[nArrayCount].dwServiceFlags1 & (~XP1_IFS_HANDLES); 				
       dwOrigCatalogId[nArrayCount++] = pProtoInfo[i].dwCatalogEntryId;
       bFindRaw = TRUE;
      }
    }
  }  

 // install our tiered protocol and get a dwLayeredCatalogId // Just to find a lower structure of the agreement can be copied over
 WSAPROTOCOL_INFOW LayeredProtocolInfo;
 memcpy(&LayeredProtocolInfo, &OriginalProtocolInfo[0], sizeof(WSAPROTOCOL_INFOW));
 // Modify the protocol name, type, set the PFL_HIDDEN flag
 lstrcpyW(LayeredProtocolInfo.szProtocol, wszLSPName);
 LayeredProtocolInfo.ProtocolChain.ChainLen = LAYERED_PROTOCOL; // 0;
 LayeredProtocolInfo.dwProviderFlags |= PFL_HIDDEN;	
 if(::WSCInstallProvider(ProviderGuid, pwszPathName, &LayeredProtocolInfo, 1, &nError) == SOCKET_ERROR)return FALSE; // installation
	
 // Re-enumerate the protocol to obtain the directory ID of the hierarchical protocol
 FreeProvider(pProtoInfo);	
 pProtoInfo = GetProvider(&nProtocols);
 for(i=0; i<nProtocols; i++)
  {
   if(memcmp(&pProtoInfo[i].ProviderId, ProviderGuid, sizeof(GUID)) == 0){dwLayeredCatalogId = pProtoInfo[i].dwCatalogEntryId; break;}
  }

 // install the protocol chain
 // modify the protocol name, type
 WCHAR wszChainName[WSAPROTOCOL_LEN + 1];
 for(i=0; i<nArrayCount; i++)
  {
   wsprintfW(wszChainName, L"%ws over %ws", wszLSPName, OriginalProtocolInfo[i].szProtocol);
   lstrcpyW(OriginalProtocolInfo[i].szProtocol, wszChainName);
   if(OriginalProtocolInfo[i].ProtocolChain.ChainLen == 1)OriginalProtocolInfo[i].ProtocolChain.ChainEntries[1] = dwOrigCatalogId[i];	
    else
     {
	  for(int j = OriginalProtocolInfo[i].ProtocolChain.ChainLen; j>0; j--)OriginalProtocolInfo[i].ProtocolChain.ChainEntries[j] = OriginalProtocolInfo[i].ProtocolChain.ChainEntries[j-1];
     }
   OriginalProtocolInfo[i].ProtocolChain.ChainLen ++;
   OriginalProtocolInfo[i].ProtocolChain.ChainEntries[0] = dwLayeredCatalogId;	
  }
 // Get a Guid, install it
 GUID ProviderChainGuid;
 FreeProvider(pProtoInfo);
 if(::UuidCreate(&ProviderChainGuid) == RPC_S_OK)
  {
   if(::WSCInstallProvider(&ProviderChainGuid, pwszPathName, OriginalProtocolInfo, nArrayCount, &nError) == SOCKET_ERROR)return FALSE;			
  }
   else return FALSE;

// forward our protocol ahead and reorder the Winsock directory // re-enumerate the installed protocol 
 pProtoInfo = GetProvider(&nProtocols);

 DWORD dwIds[50];
 int nIndex = 0;

 for(i=0; i<nProtocols; i++)  // Add our protocol chain
  {
   if((pProtoInfo[i].ProtocolChain.ChainLen > 1) && (pProtoInfo[i].ProtocolChain.ChainEntries[0] == dwLayeredCatalogId))dwIds[nIndex++] = pProtoInfo[i].dwCatalogEntryId;
  }
 for(i=0; i<nProtocols; i++) // Add other protocols
  {
   if((pProtoInfo[i].ProtocolChain.ChainLen <= 1) || (pProtoInfo[i].ProtocolChain.ChainEntries[0] != dwLayeredCatalogId))dwIds[nIndex++] = pProtoInfo[i].dwCatalogEntryId;
  }
 
 if(nError = ::WSCWriteProviderOrder(dwIds, nIndex) != ERROR_SUCCESS)return FALSE;	// Reorder the Winsock directory
 FreeProvider(pProtoInfo);
 return TRUE;
}
//------------------------------------------------------------------------------------
static int _stdcall StartupProc(WORD wVersion, LPWSPDATA lpWSPData, LPWSAPROTOCOL_INFOW lpProtocolInfo, WSPUPCALLTABLE UpcallTable, LPWSPPROC_TABLE lpProcTable)
{	
 if(lpProtocolInfo->ProtocolChain.ChainLen <= 1)return WSAEPROVIDERFAILEDINIT;
//	g_pUpCallTable = UpcallTable;    // Save the function pointer pointer to call up (here we do not use it)
	
 WSAPROTOCOL_INFOW	NextProtocolInfo;
 int nTotalProtos;
 LPWSAPROTOCOL_INFOW pProtoInfo = GetProvider(&nTotalProtos);    // Any return before 'FreeProvider' will LEAK this pointer!
	
 DWORD dwBaseEntryId = lpProtocolInfo->ProtocolChain.ChainEntries[1];  // lower entry ID
 bool  upfound = false;
 for(int i=0; i<nTotalProtos; i++)     // enumerate the protocol and find the WSAPROTOCOL_INFOW structure of the underlying protocol
  {
   if(pProtoInfo[i].dwCatalogEntryId == dwBaseEntryId)
    {
     memcpy(&NextProtocolInfo, &pProtoInfo[i], sizeof(NextProtocolInfo));
     upfound = true;
     break;
    }
  }
 if(!upfound){LOGMSG("Can not find underlying protocol",0); return WSAEPROVIDERFAILEDINIT;}

 int nError;
 WCHAR szBaseProviderDll[MAX_PATH];
 int nLen = MAX_PATH;
 if(::WSCGetProviderPath(&NextProtocolInfo.ProviderId, szBaseProviderDll, &nLen, &nError) == SOCKET_ERROR){LOGMSG("WSCGetProviderPath() failed %d", nError); return WSAEPROVIDERFAILEDINIT;}   // Get the underlying provider DLL path
 if(!::ExpandEnvironmentStringsW(szBaseProviderDll, szBaseProviderDll, MAX_PATH)){LOGMSG("ExpandEnvironmentStrings() failed %d", ::GetLastError()); return WSAEPROVIDERFAILEDINIT;}

 HMODULE hModule = ::LoadLibraryW(szBaseProviderDll);  // load the underlying provider DLL
 if(hModule == NULL){LOGMSG("LoadLibrary() failed %d", ::GetLastError()); return WSAEPROVIDERFAILEDINIT;}

 LPWSPSTARTUP  pfnWSPStartup = NULL;
 pfnWSPStartup = (LPWSPSTARTUP)::GetProcAddress(hModule, "WSPStartup");     // import the WSPStartup function of the underlying provider
 if(pfnWSPStartup == NULL){LOGMSG("GetProcAddress() failed %d", ::GetLastError()); return WSAEPROVIDERFAILEDINIT;}

 LPWSAPROTOCOL_INFOW pInfo = lpProtocolInfo;
 if(NextProtocolInfo.ProtocolChain.ChainLen == BASE_PROTOCOL)pInfo = &NextProtocolInfo;

 int nRet = pfnWSPStartup(wVersion, lpWSPData, pInfo, UpcallTable, lpProcTable);        // call the WSPStartup function of the underlying provider
 if(nRet != ERROR_SUCCESS){LOGMSG("Underlying provider's WSPStartup() failed %d", nRet); return nRet;}

// g_NextProcTable = *lpProcTable;   // Save the function table of the lower provider
// lpProcTable->lpWSPSendTo = WSPSendTo; // Modify the function table passed to the upper, Hook interested in the function, here as an example, only Hook the WSPSendTo function. You can also Hook other functions, such as WSPSocket, WSPCloseSocket, WSPConnect, etc.
 if(pStartupCallback)pStartupCallback(wVersion, lpWSPData, lpProtocolInfo, UpcallTable, lpProcTable);

 FreeProvider(pProtoInfo);  // TODO: Make a struct to free this in a destructor
 return nRet;
}
//------------------------------------------------------------------------------------

};
//====================================================================================
#endif