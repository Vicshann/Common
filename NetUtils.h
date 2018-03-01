//---------------------------------------------------------------------------

#ifndef NetUtilsH
#define NetUtilsH

#define WIN32_LEAN_AND_MEAN

#include <Winsock2.h>
#include <Wininet.h>
#include "MiniString.h"


//---------------------------------------------------------------------------
CMiniStr __stdcall netExtractPagePath(LPSTR CPath);
CMiniStr __stdcall netExtractHostName(LPSTR CPath);
CMiniStr __stdcall MakeRequestHttpPOST(LPSTR Host, LPSTR Page, PBYTE Content, UINT ContLen);
ULONG    __stdcall netServerDataExchange(SOCKET Soc, CMiniStr* Req, CMiniStr* Rsp, int Timeout);
ULONG    __stdcall netServerDataExchange(SOCKET Soc, PBYTE DataToSend, PBYTE RecBuffer, ULONG DataLength, ULONG RecBufLen, PULONG Remains);
ULONG    __stdcall netServerDataExchangeGZip(SOCKET Soc, PBYTE DataToSend, PBYTE RecBuffer, ULONG DataLength, ULONG RecBufLen, PULONG Remains);
SOCKET   __stdcall netOpenConnection(LPSTR CPath, USHORT port, PBYTE IpAddr=NULL);
void     __stdcall netCloseConnection(SOCKET soc);
bool     __stdcall netWaitReadData(SOCKET Soc, UINT msec);
bool     __stdcall netWaitWriteData(SOCKET Soc, UINT msec);
int      __stdcall netEmptyReadBuf(SOCKET Soc);
bool     __stdcall netHaveDataToRead(SOCKET Soc);
bool     __stdcall netIsSocketAlive(SOCKET Soc);
bool     __stdcall netSetKeepAlive(SOCKET Soc, BOOL Enable, UINT katime=60000, UINT kaintv=5000);
int      __stdcall netReadData(SOCKET Soc, CMiniStr& Data, UINT WaitSec=3, UINT MaxRead=0xFFFFFF);
int      __stdcall netWriteData(SOCKET Soc, CMiniStr& Data);
bool     __stdcall SetSocTimeout(SOCKET soc, UINT sec);
//---------------------------------------------------------------------------
#endif
