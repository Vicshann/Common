//---------------------------------------------------------------------------
#include "NetUtils.h"

#define PATHDLMR 0x2F
#define PATHDLML 0x5C

#pragma pack(push,1)
struct my_tcp_keepalive {
    u_long  onoff;
    u_long  keepalivetime;
    u_long  keepaliveinterval;
};
#pragma pack(pop)

//===========================================================================
//                        NET Utils
//---------------------------------------------------------------------------
CMiniStr __stdcall netExtractPagePath(LPSTR CPath)
{
 CMiniStr str  = CPath;
 LPSTR    cptr = str.c_str();
 for(int ctr=0;ctr<str.Length();ctr++)
  {
   if(cptr[ctr] == ':'){ctr += 2;continue;}
   if((cptr[ctr] == PATHDLMR)||(cptr[ctr] == PATHDLML))return &cptr[ctr];
  }
 return str;
}
//---------------------------------------------------------------------------
CMiniStr __stdcall netExtractHostName(LPSTR CPath)
{
 CMiniStr str  = CPath;
 LPSTR    cptr = str.c_str();
 LPSTR    base = cptr;
 for(int ctr=0;ctr<str.Length()+1;ctr++)
  {
   if(cptr[ctr] == ':'){ctr += 2;base = &cptr[ctr+1];continue;}
   if((cptr[ctr] == PATHDLMR)||(cptr[ctr] == PATHDLML)){cptr[ctr] = 0;return base;}
  }
 return base;
}
//---------------------------------------------------------------------------
CMiniStr __stdcall MakeRequestHttpPOST(LPSTR Host, LPSTR Page, PBYTE Content, UINT ContLen)
{
 CMiniStr str;

 str = str + "POST " + Page + " HTTP/1.1\r\n";    // "/term2/xmlutf.jsp"   "https://w.qiwi.ru/term2/xmlutf.jsp";
 str = str + "Connection: keep-alive\r\n";
 str = str + "Content-Type: application/x-www-form-urlencoded\r\n";  // "application/x-www-form-urlencoded"  // "Content-Type: text/XML\r\n";
 str = str + "Content-Length: " + ContLen + "\r\n";
 str = str + "Host: " + Host + "\r\n";
 str = str + "Accept: text/html, */*\r\n";
// str = str + "Accept-Encoding: gzip, deflate\r\n";
// str = str + "User-Agent: " + SoftId + "\r\n";
 str = str + "\r\n";  // Must be at end of this header list
 if(Content && ContLen)
  {
   UINT coffs = str.Length(); //  str = str + (Content;
   str.SetLength(coffs+ContLen);
   memcpy(&str.c_data()[coffs],Content,ContLen);
  }
 return str;
}
//---------------------------------------------------------------------------
ULONG __stdcall netServerDataExchange(SOCKET Soc, PBYTE DataToSend, PBYTE RecBuffer, ULONG DataLength, ULONG RecBufLen, PULONG Remains)
{
 ULONG DLeft = 0;
 ULONG Total = 0;
 if(DataToSend && DataLength){if(SOCKET_ERROR == send(Soc,(LPSTR)DataToSend,DataLength,0))return 0;}
 for(ULONG Len = 1;RecBufLen;)
  {
   if(SOCKET_ERROR == recv(Soc,(char*)&RecBuffer[Total],Len,0))break;  // Can`t read
   Total     += Len;
   RecBufLen -= Len;
   if(!RecBufLen && !Remains)break; // Fully readed
   if(ioctlsocket(Soc,FIONREAD,&DLeft) || !DLeft)break;  // No more Data
   Len = (DLeft > RecBufLen)?(RecBufLen):(DLeft);
  }
 if(Remains)*Remains = DLeft;
 return Total;
}
//---------------------------------------------------------------------------
ULONG __stdcall netServerDataExchange(SOCKET Soc, CMiniStr* Req, CMiniStr* Rsp, int Timeout)
{
 ULONG DLeft = 0;
 ULONG Total = 0;
 if(Req && Req->Length()){if(SOCKET_ERROR == send(Soc,(LPSTR)Req->c_data(),Req->Length(),0))return 0;}
 if(!Rsp)return 0;
 Rsp->Clear();
 for(ULONG Len = (int)(Timeout <= 0);;)
  {
   while(!Len && (Timeout > 0)){Timeout -= 250; Sleep(250); ioctlsocket(Soc,FIONREAD,&Len);}
   Rsp->SetLength(Rsp->Length()+Len, 0);
   if(SOCKET_ERROR == recv(Soc,(char*)&Rsp->c_data()[Total],Len,0))break;  // Can`t read
   Total += Len;
   if(ioctlsocket(Soc,FIONREAD,&DLeft) || !DLeft)break;  // No more Data
   Len = DLeft;
  }
 return Total;
}
//---------------------------------------------------------------------------
/*ULONG __stdcall netServerDataExchangeGZip(SOCKET Soc, PBYTE DataToSend, PBYTE RecBuffer, ULONG DataLength, ULONG RecBufLen, PULONG Remains)
{
 ULONG DLeft = 0;
 ULONG Total = 0;

 ULONG GZLen = DataLength+(DataLength/3)+32;
 PBYTE pGZip = (PBYTE)malloc(GZLen+16);
 memset(pGZip,0,GZLen);
// --- Compress Output Data
 gzFile hGZStream = gzbufopen(&pGZip[10], GZLen-32, TRUE);
 UINT Compr = gzbufwrite(hGZStream, DataToSend, DataLength);
 pGZip[0] = 0x1F;
 pGZip[1] = 0x8B;
 pGZip[2] = 0x08;
 ((PDWORD)&pGZip[Compr+10])[0] = gzget_crc32(hGZStream);
 ((PDWORD)&pGZip[Compr+10])[1] = DataLength;
 gzbufclose(hGZStream);
// --- Send Output Data
 int res = send(Soc,(LPSTR)pGZip,Compr+18,0);
 //int err = WSAGetLastError();
 free(pGZip);
 if(SOCKET_ERROR == res)return 0;
// --- Receive Response Data
 for(ULONG Len = 1,RecLen = RecBufLen;RecLen;)
  {
   if(SOCKET_ERROR == recv(Soc,&RecBuffer[Total],Len,0))break;  // Can`t read
   Total  += Len;
   RecLen -= Len;
   if(!RecLen && !Remains)break; // Fully readed
   if(ioctlsocket(Soc,FIONREAD,&DLeft) || !DLeft)break;  // No more Data
   Len = (DLeft > RecLen)?(RecLen):(DLeft);
  }
 if(Remains)*Remains = DLeft;
 if(DLeft)return Total;
// --- Decompress Received Data
 GZLen = (((Total*3)<(1024*1024))?(1024*1024):(Total*3));  // !!!!!!! Get size from stream?
 pGZip = (PBYTE)malloc(GZLen+16);

 hGZStream     = gzbufopen(&RecBuffer[10], Total-10, FALSE);  // -10 bytes of header
 ULONG Uncompr = gzbufread(hGZStream, pGZip, GZLen);
 gzbufclose(hGZStream);
 if(Uncompr > RecBufLen)DLeft = Uncompr - RecBufLen;
 if(Remains)*Remains = DLeft;
 Total = (Uncompr - DLeft);
 memcpy(RecBuffer,pGZip,Total);
 free(pGZip);
 return Total;
}*/
//---------------------------------------------------------------------------
bool __stdcall netWaitReadData(SOCKET Soc, UINT msec)
{
 fd_set  fd;
 timeval ti;
 ti.tv_sec   = msec / 1000;
 ti.tv_usec  = msec % 1000;
 fd.fd_count = 1;
 fd.fd_array[0] = Soc;
 return (SOCKET_ERROR != select(0,&fd,NULL,NULL,((msec)?(&ti):(NULL))));
}
//---------------------------------------------------------------------------
bool __stdcall netWaitWriteData(SOCKET Soc, UINT msec)
{
 fd_set  fd;
 timeval ti;
 ti.tv_sec   = msec / 1000;
 ti.tv_usec  = msec % 1000;
 fd.fd_count = 1;
 fd.fd_array[0] = Soc;
 return (SOCKET_ERROR != select(0,NULL,&fd,NULL,((msec)?(&ti):(NULL))));
}
//---------------------------------------------------------------------------
bool __stdcall netSetKeepAlive(SOCKET Soc, BOOL Enable, UINT katime, UINT kaintv)
{
 my_tcp_keepalive kal;
 DWORD BytesRet = 0;
 kal.onoff = Enable;
 kal.keepalivetime = 10000;
 kal.keepaliveinterval = 1000;                                    
 return (WSAIoctl(Soc, 0x98000004, &kal, sizeof(my_tcp_keepalive), 0, 0, &BytesRet, 0, 0) != SOCKET_ERROR);    // 0x98000004 = SIO_KEEPALIVE_VALS  // Mstcpip.h
}
//---------------------------------------------------------------------------
bool __stdcall netIsSocketAlive(SOCKET Soc)
{
 fd_set  readfds;
 timeval timeout; 

 readfds.fd_array[0] = Soc;
 readfds.fd_count    = 1;                 
 timeout.tv_sec  = 0;
 timeout.tv_usec = 100;
 return (select(0, &readfds, 0, 0, &timeout) == 0);
}
//---------------------------------------------------------------------------
bool __stdcall netHaveDataToRead(SOCKET Soc)
{
 u_long SocData = 0;
 return (!ioctlsocket(Soc, FIONREAD, &SocData) && SocData);   // 0x4004667F
}
//---------------------------------------------------------------------------
int __stdcall netEmptyReadBuf(SOCKET Soc)
{
 ULONG DLeft = 0;
 ULONG Total = 0;
 if(ioctlsocket(Soc,FIONREAD,&DLeft) != 0)return -1;
 if(!DLeft)return 0;
 BYTE Buffer[1024];
 do
  {
   int Len = (DLeft > sizeof(Buffer))?(sizeof(Buffer)):(DLeft);
   Len     = recv(Soc, (char*)&Buffer,Len,0);
   Total  += Len;
   DLeft  -= Len;
  }
   while((long)DLeft > 0);
 return Total;
}
//---------------------------------------------------------------------------
int __stdcall netWriteData(SOCKET Soc, CMiniStr& Data)
{
 return (int)(send(Soc, (char*)Data.c_data(), Data.Length(), 0) != SOCKET_ERROR) - 1;
}
//---------------------------------------------------------------------------
int __stdcall netReadData(SOCKET Soc, CMiniStr& Data, UINT WaitSec, UINT MaxRead)
{
 UINT   Total   = 0;
 u_long SocData = 0;

 Data.Clear();
 for(UINT ctr=0;ctr < WaitSec;ctr++){if(ioctlsocket(Soc, FIONREAD, &SocData))return -1; if(!SocData)Sleep(1000); else break;}
 if(!SocData || !MaxRead)return 0; 
 for(UINT DLeft=SocData;Total < MaxRead;)
  {
   Data.SetLength(Total + SocData);
   int Res = recv(Soc, (char *)&Data.c_data()[Total], SocData, 0);
   if((Res == SOCKET_ERROR) || !Res)return -2;
   Total += Res;
   if(ioctlsocket(Soc, FIONREAD, &SocData) || !SocData)break;
   if((Total+SocData) > MaxRead)SocData = MaxRead - Total;
  }
 return Total;
}
//---------------------------------------------------------------------------
SOCKET __stdcall netOpenConnection(LPSTR CPath, USHORT port, PBYTE IpAddr)
{
 CMiniStr str = netExtractHostName(CPath);
 ULONG IAddr = inet_addr(str.c_str());
 if(INADDR_NONE == IAddr)
  {
   if(HOSTENT *host=(HOSTENT*)gethostbyname(str.c_str()))IAddr = ((PULONG*)host->h_addr_list)[0][0];
	 else return INVALID_SOCKET;
  }
									   
 SOCKET soc = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);  //WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
 if(soc == INVALID_SOCKET)return INVALID_SOCKET;

 SOCKADDR_IN SockAddr;
 memset(&SockAddr,0,sizeof(SockAddr));
 SockAddr.sin_family = AF_INET;
 SockAddr.sin_port   = htons(port);
 SockAddr.sin_addr.S_un.S_addr = IAddr;  //*((PDWORD)&he->h_addr_list[0][0]);
 if(IpAddr)
  {
   PBYTE IData = (PBYTE)&IAddr;
   IpAddr[0] = IData[0];
   IpAddr[1] = IData[1];
   IpAddr[2] = IData[2];
   IpAddr[3] = IData[3];
  }
 if(connect(soc,(struct sockaddr*)&SockAddr,sizeof(SockAddr))){closesocket(soc);return INVALID_SOCKET;}      // WSAConnect(soc,(struct sockaddr*)&SockAddr,sizeof(SockAddr),NULL,NULL,NULL,NULL)
 return soc;
}
//---------------------------------------------------------------------------
void __stdcall netCloseConnection(SOCKET soc)
{
 closesocket(soc);
}
//---------------------------------------------------------------------------
bool __stdcall SetSocTimeout(SOCKET soc, UINT sec)
{
 DWORD sock_timeout = sec*1000;  // UNIX: const struct timeval sock_timeout={.tv_sec=10, .tv_usec=0};
 return (setsockopt(soc, SOL_SOCKET, SO_RCVTIMEO, (char*)&sock_timeout, sizeof(sock_timeout)) == 0);
}
//---------------------------------------------------------------------------


