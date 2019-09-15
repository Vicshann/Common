//---------------------------------------------------------------------------

#ifndef NetUtilsH
#define NetUtilsH

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
