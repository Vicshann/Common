
#pragma once
 
#ifndef NetWrkH
#define NetWrkH
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
 

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

#include <windows.h>
#include <Winsock2.h>
#include <Wininet.h>
#include "ssl.h"
#ifndef NETWNOGZ
#include "gzip.h"
#endif
//#include "sha.h"
#include "StrUtils.hpp"
#include "Utils.h"
#include "MiniString.h"
#include "Json.h"


#pragma pack(push,1)
struct my_tcp_keepalive {
    u_long  onoff;
    u_long  keepalivetime;
    u_long  keepaliveinterval;
};
#pragma pack(pop)

//------------------------------------------------------------------------------------------------------------
enum ENetReqType {rtNONE, rtGET, rtPOST};

class CNetWrk
{
 SOCKET   soc;
 SSL_SOCKET SSoc;
 PCCERT_CONTEXT SslCert;
 ULONG  IPAddr = 0;
 USHORT Port = 0;
 bool     Connected = false;
 bool     UseHttps  = false;

public:
 UINT   Timeout = 3;
 CMiniStr Host;
 CMiniStr Path;

SOCKET GetSocket(void){return this->soc;}
//------------------------------------------------------------------------------------------------------------
bool  SetSocTimeout(UINT sec)
{
 DWORD sock_timeout = sec*1000;  // UNIX: const struct timeval sock_timeout={.tv_sec=10, .tv_usec=0};
 return (setsockopt(this->soc, SOL_SOCKET, SO_RCVTIMEO, (char*)&sock_timeout, sizeof(sock_timeout)) == 0);
}
//------------------------------------------------------------------------------------------------------------
bool SetKeepAlive(BOOL Enable, UINT katime, UINT kaintv)
{
 my_tcp_keepalive kal;
 DWORD BytesRet = 0;
 kal.onoff = Enable;
 kal.keepalivetime = 10000;
 kal.keepaliveinterval = 1000;                                    
 return (WSAIoctl(this->soc, 0x98000004, &kal, sizeof(my_tcp_keepalive), 0, 0, &BytesRet, 0, 0) != SOCKET_ERROR);    // 0x98000004 = SIO_KEEPALIVE_VALS  // Mstcpip.h
}
//---------------------------------------------------------------------------
bool IsSocketAlive(void)
{
 fd_set  readfds;
 timeval timeout; 

 readfds.fd_array[0] = this->soc;
 readfds.fd_count    = 1;                 
 timeout.tv_sec  = 0;
 timeout.tv_usec = 100;
 return (select(0, &readfds, 0, 0, &timeout) == 0);
}
//---------------------------------------------------------------------------
bool HaveDataToRead(void)
{
 u_long SocData = 0;
 return (!ioctlsocket(this->soc, FIONREAD, &SocData) && SocData);   // 0x4004667F
}
//---------------------------------------------------------------------------
int  EmptyReadBuf(void)
{
 ULONG DLeft = 0;
 ULONG Total = 0;
 if(ioctlsocket(this->soc,FIONREAD,&DLeft) != 0)return -1;
 if(!DLeft)return 0;
 BYTE Buffer[1024];
 do
  {
   int Len = (DLeft > sizeof(Buffer))?(sizeof(Buffer)):(DLeft);
   Len     = recv(this->soc, (char*)&Buffer,Len,0);
   Total  += Len;
   DLeft  -= Len;
  }
   while((long)DLeft > 0);
 return Total;
}
//------------------------------------------------------------------------------------------------------------

CNetWrk(void){}
~CNetWrk(){this->Disconnect();}
//------------------------------------------------------------------------------------------------------------
int Connect(LPSTR Url, USHORT port, PCCERT_CONTEXT Cert=NULL)
{
 this->AssignURL(Url, port);								   
 this->soc = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); 
 if(soc == INVALID_SOCKET){LOGMSG("Socket Error: %u",WSAGetLastError()); return -2;}

 SOCKADDR_IN SockAddr;
 memset(&SockAddr,0,sizeof(SockAddr));
 SockAddr.sin_family = AF_INET;
 SockAddr.sin_port   = htons(this->Port);
 SockAddr.sin_addr.S_un.S_addr = this->IPAddr;  
 if(connect(this->soc,(struct sockaddr*)&SockAddr,sizeof(SockAddr))){closesocket(this->soc); LOGMSG("Connect Error: %u",WSAGetLastError()); return -3;}   
 this->SslCert  = Cert;
 if(Cert)
  {
   this->SetSocTimeout(5);
   this->SSoc.Initialize(this->soc,0,this->SslCert);
   if(this->SSoc.ClientInit() != 0){LOGMSG("Failed to initialize SSL session!"); this->SSoc.ClientOff(); closesocket(this->soc); this->SSoc.Destroy(); return -4;}
   this->SetSocTimeout(this->Timeout);
   this->UseHttps = true;
  }
 this->Connected = true;
 return 0;
}
//------------------------------------------------------------------------------------------------------------
bool AssignURL(LPSTR Url, USHORT port)
{
 if(!Url || !*Url)return false;   // Pass NULL to skip name resolving to same address
 SplitURL(Url, this->Host, this->Path);
 this->IPAddr = inet_addr(this->Host.c_str());
 if(INADDR_NONE == this->IPAddr)
  {
   if(HOSTENT *host=(HOSTENT*)gethostbyname(this->Host.c_str()))this->IPAddr = ((PULONG*)host->h_addr_list)[0][0];
     else {LOGMSG("Host name resolving Error: %u",WSAGetLastError()); return -1;}
  }
 this->Port = port;  
 return true; 
}
//------------------------------------------------------------------------------------------------------------
static void SplitURL(LPSTR Url, CMiniStr& Host, CMiniStr& Path)
{
 LPSTR phost = NULL;
 LPSTR ppage = NULL;
 Host.Clear();
 Path.Clear();
 for(int ctr=0;Url[ctr];ctr++)
  {
   if(Url[ctr] == ':'){ctr += 2;phost = &Url[ctr+1];continue;}
   if((Url[ctr] == PATHDLMR)||(Url[ctr] == PATHDLML)){ppage = &Url[ctr]; break;}
  } 
 if(ppage)
  {
   Host.cAssign(phost,ppage-phost);
   Path = ppage;
  }
   else if(phost)Host = phost;
}
//------------------------------------------------------------------------------------------------------------
static int ComposeParams(CJSonItem& Params, CMiniStr& OutStr)
{
// OutStr.Clear();
 for(CJSonItem* Itm=Params.First();Itm;Itm=Params.Next(Itm))
  {
   if(OutStr.Length())OutStr += "&";
   OutStr += Itm->GetName();
   OutStr += "=";
   if(!Itm->IsValStr())OutStr += (int)Itm->GetValInt();    // No FLT types?
     else OutStr += Itm->GetValStr();
  }
 return 0;
}
//------------------------------------------------------------------------------------------------------------
static int ComposeHeader(CJSonItem& Header, CMiniStr& OutStr)
{
// OutStr.Clear();
 for(CJSonItem* Itm=Header.First();Itm;Itm=Header.Next(Itm))
  {
   OutStr += Itm->GetName();
   OutStr += ": ";
   if(!Itm->IsValStr())OutStr += (int)Itm->GetValInt();    // No FLT types?
     else OutStr += Itm->GetValStr();
   OutStr += "\r\n";
  }
 return 0;
}
//------------------------------------------------------------------------------------------------------------
int ComposeRequest(ENetReqType Type, CMiniStr& Header, CMiniStr& Params, CMiniStr& Data, CMiniStr& ResReq)
{
 ResReq.Clear(); 
 if(Type == rtGET)
  {
   ResReq += "GET " + this->Path;
   if(Params.Length())
    {
     ResReq += "?";
     ResReq += Params;
    }
  }
   else ResReq += "POST " + this->Path;
 ResReq += " HTTP/1.1\r\n";
 ResReq += Header;
 ResReq += "\r\n";
 if(Type == rtPOST){ResReq += Params; ResReq += "\r\n";}
 ResReq += Data;
 return 0;
}
//------------------------------------------------------------------------------------------------------------
int SendRequest(CMiniStr& Req)
{
 int res = (UseHttps)?(this->SSoc.s_ssend((char*)Req.c_data(),Req.Length())):(send(this->soc,(char*)Req.c_data(),Req.Length(),0));     // Send is max 16384 at time!
 if(res < 0){LOGMSG("Error: %i, %u",res,WSAGetLastError());}
 return res;
}
//------------------------------------------------------------------------------------------------------------
int GetResponse(CMiniStr& Rsp)
{
 int   rpos   = 0;
 int  Total   = 0; 
 Rsp.Clear(); 
 for(int RecvBlkLen=16384;(RecvBlkLen > 0);)
  {
   Rsp.SetLength(Total + RecvBlkLen);
   int rval = (UseHttps)?(this->SSoc.s_recv((char*)&Rsp.c_data()[Total],RecvBlkLen)):(recv(this->soc,(char*)&Rsp.c_data()[Total],RecvBlkLen,0));     
   if(!rval || (rval == SOCKET_ERROR))break;  // WSAETIMEDOUT == WSAGetLastError();
/*   if(!Total)  // First Block
    {
     rpos = GetSubStrOffsSimpleIC("200 OK", (CHAR*)Rsp.c_data(), 0, ((rval <= 16)?(rval):(16)));
     if(rpos < 0){LOGMSG("No '200 OK' in HTTP response header(Size=%u)!",rval); return -5;}
     rpos = GetSubStrOffsSimpleIC("\r\n\r\n", (CHAR*)StrRsp.c_data(), 0, ((StrRsp.Length() <= 128)?(StrRsp.Length()):(128)));
     if(rpos < 0){LOGMSG("No content!"); return -7;}      
    }*/ 
   Total += rval;
  }
 Rsp.SetLength(Total);
 return Rsp.Length();
}
//------------------------------------------------------------------------------------------------------------
int GetResponseContent(CMiniStr& Rsp, CMiniStr& Content)    // TODO: Decode Url encoded content?
{
 Content.Clear();
 if((Rsp.Length() < 5) || (StrCompareSimpleIC("HTTP/", Rsp.c_str()) >= 0)){Content = Rsp; return 1;}
 int epos = GetSubStrOffsSimpleIC("\r\n\r\n", (CHAR*)Rsp.c_data(), 0, Rsp.Length());
// int rpos = GetSubStrOffsSimpleIC("200 OK", (CHAR*)Rsp.c_data(), 0, (epos > 0)?(epos):(Rsp.Length()));
// if(rpos < 0){LOGMSG("No '200 OK' in HTTP response header(Size=%u)!",Rsp.Length());}
 if(epos < 0){Content = Rsp; return 2;}     // LOGMSG("No content!"); 
 bool GZipd = (GetSubStrOffsSimpleIC("Content-Encoding: gzip", (CHAR*)Rsp.c_data(), 0, epos) > 0);
 bool Chunk = (GetSubStrOffsSimpleIC("Transfer-Encoding: chunked", (CHAR*)Rsp.c_data(), 0, epos) > 0); 
 int DataOffs = epos + 4;  // \r\n\r\n
 if(Chunk)
  {
   while(DataOffs < Rsp.Length())
    {
     UINT CntLen = HexStrToNum<UINT>((LPSTR)&Rsp.c_data()[DataOffs]);  // Size of Chunk
     if(!CntLen)break;  // End chunk is 0
     int rpos = GetSubStrOffsSimpleIC("\r\n", (LPSTR)Rsp.c_data(), DataOffs, Rsp.Length());
     if(rpos < 0)break;
     DataOffs  = rpos + 2;   // \r\n
     Content.cAppend((LPSTR)&Rsp.c_data()[DataOffs], CntLen);
     DataOffs += CntLen + 2;   // \r\n
    }
  }
   else Content.cAssign((LPSTR)&Rsp.c_data()[DataOffs], Rsp.Length() - DataOffs);
#ifndef NETWNOGZ
 if(GZipd && (Content.c_data()[0] == 0x1F)&&(Content.c_data()[1] == 0x8B))
  {
   CMiniStr Str;
   UINT UnGzLen = *(PUINT)&Content.c_data()[Content.Length()-4];   // This size is stored in the last 4 bytes of the file. This will only provide the correct value if the compressed file was smaller than 4 Gb.  
   if(UnGzLen > 0x0FFFFFFF){LOGMSG("Bad GZip size: %08X",UnGzLen); return -9;}  // 268 mb
   Str.SetLength(UnGzLen);
   gzFile hGZStrm = gzbufopen(&Content.c_data()[10], Content.Length()-10, FALSE);  // -10 bytes of header
   ULONG Uncompr  = gzbufread(hGZStrm, Str.c_data(), Str.Length());
   gzbufclose(hGZStrm);
   if(Uncompr != UnGzLen){LOGMSG("Decompressed size mismatch: UnGzLen=%u, Uncompr=%u");}
   Content = Str;
  }
#endif
 return 0;
}
//------------------------------------------------------------------------------------------------------------
int Disconnect(void)
{
 if(!this->Connected)return -1;
 closesocket(this->soc); 
 if(this->UseHttps)this->SSoc.Destroy();
 this->Connected = false;
 return 0;
}
//------------------------------------------------------------------------------------------------------------
static ULONG netServerDataExchange(SOCKET Soc, PBYTE DataToSend, PBYTE RecBuffer, ULONG DataLength, ULONG RecBufLen, PULONG Remains)   // TEMPORARY
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
   if(ioctlsocket(Soc,FIONREAD,&DLeft) || !DLeft)break;  // No more Data    // Unreliable!
   Len = (DLeft > RecBufLen)?(RecBufLen):(DLeft);
  }
 if(Remains)*Remains = DLeft;
 return Total;
}
//------------------------------------------------------------------------------------------------------------
static ULONG netServerDataExchange(SOCKET Soc, CMiniStr* Req, CMiniStr* Rsp, int Timeout)
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

};
//===========================================================================================================






//------------------------------------------------------------------------------------------------------------
#endif