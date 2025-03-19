#pragma once

// ssl.h

#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
//#include <winsock2.h>
//#include <ws2tcpip.h>
#include <wincrypt.h>
#include <tchar.h>
#define SECURITY_WIN32
#include <security.h>
#include <schnlsp.h>

// Z template class
template <class T>class Z
	{
   private:

   	T* d;
      unsigned int ss;

   public:

   	Z(int s = 0)
			{
			if (!s)
				s = 1;
			d = new T[s];
			memset(d,0,s*sizeof(T));
			ss = s;
			}
		~Z()
			{
	      delete[] d;
			}

      operator T*()
			{
			return d;
			}

	void _clear()
		{
		ZeroMemory(d,ss*sizeof(T));
		}
	void clear()
		{
		ZeroMemory(d,ss*sizeof(T));
		}

	int bs()
		{
		return ss*sizeof(T);
		}

	int is()
		{
		return ss;
		}

	void Resize(unsigned int news)
		{
		if (news == ss)
			return; // same size

		// Create buffer to store existing data
		T* newd = new T[news];
		int newbs = news*sizeof(T);
		ZeroMemory((void*)newd, newbs);

		if (ss < news)
			// we created a larger data structure
			memcpy((void*)newd,d,ss*sizeof(T));
		else
			// we created a smaller data structure
			memcpy((void*)newd,d,news*sizeof(T));
		delete[] d;
		d = newd;
		ss = news;
		}

	void AddResize(int More)
		{
		Resize(ss + More);
		}

   };

//* PENDING:
//* Certificate Chain 
//------------------------------------------------------------------------------------------------------------

class SSL_SOCKET
	{
	private:
		int Type;
		HCERTSTORE hCS;
		SCHANNEL_CRED m_SchannelCred;
		CredHandle hCred;
		CtxtHandle hCtx;
		TCHAR dn[1000];
		SecBufferDesc sbin;
		SecBufferDesc sbout;
		bool InitContext;
		Z<char> ExtraData;
		int ExtraDataSize;
		Z<char> PendingRecvData;
		int PendingRecvDataSize;
		PCCERT_CONTEXT OurCertificate;
		bool IsExternalCert;
//		Z<char> ExtraDataSec;
//		int ExtraDataSecSize;

		static void nop() {}

	public:
		SOCKET X;

	/*	void Initialize(SOCKET,int,PCCERT_CONTEXT = 0);
		void SetDestinationName(TCHAR* n);
		int ClientInit(bool = false);
		int ClientLoop();
		int ServerInit(bool = false);
		int ServerLoop();

		void Destroy(void);
		int s_rrecv(char *b, int sz);
		int s_ssend(char* b, int sz);
		int s_recv(char *b, int sz);
		int s_send(char* b, int sz);
		int rrecv_p(char *b, int sz);
		int ssend_p(char* b, int sz);
		int recv_p(char *b, int sz);
		int send_p(char* b, int sz);

		int ClientOff();
		int ServerOff();

		SECURITY_STATUS Verify(PCCERT_CONTEXT);
		SECURITY_STATUS VerifySessionCertificate();
		void GetCertificateInfoString(TCHAR* s);
		static PCCERT_CONTEXT CreateOurCertificate();
		void NoFail(HRESULT);	 */



void  Initialize(SOCKET x,int Ty,PCCERT_CONTEXT pc=nullptr, HCERTSTORE hCSi=nullptr)
	{
	X = x;
	Type = Ty;
	hCS = hCSi;
	hCred.dwLower = 0;
	hCred.dwUpper = 0;
	hCtx.dwLower = 0;
	hCtx.dwUpper = 0;
	memset(dn,0,1000*sizeof(TCHAR));
	InitContext = false;
	ExtraDataSize = 0;
	PendingRecvDataSize = 0;
	OurCertificate = 0;
	IsExternalCert = false;
	if (pc)
		{
		OurCertificate = pc;
		IsExternalCert = true;
		}
	}
//------------------------------------------------------------------------------
void  Destroy(void)
	{
	if (Type == 0)
		ClientOff();
	else
		ServerOff();

	if (hCtx.dwLower || hCtx.dwLower)
		{
		DeleteSecurityContext(&hCtx);
		}

	if (hCred.dwLower || hCred.dwLower)
		{
		FreeCredentialHandle(&hCred);
		}

	if (OurCertificate && !IsExternalCert)
		{
		CertFreeCertificateContext(OurCertificate);
		OurCertificate = 0;
		}

	//if (hCS)
	//	CertCloseStore(hCS,0);
	hCS = 0;
	}
//------------------------------------------------------------------------------
void  SetDestinationName(TCHAR* n)
	{
	_tcscpy(dn,n);
	}
//------------------------------------------------------------------------------
int  ClientOff()
	{
	// Client wants to disconnect

	SECURITY_STATUS ss;
	Z<SecBuffer> OutBuffers(100);
	DWORD dwType = SCHANNEL_SHUTDOWN;
	OutBuffers[0].pvBuffer   = &dwType;
   OutBuffers[0].BufferType = SECBUFFER_TOKEN;
   OutBuffers[0].cbBuffer   = sizeof(dwType);

   sbout.cBuffers  = 1;
   sbout.pBuffers  = OutBuffers;
   sbout.ulVersion = SECBUFFER_VERSION;

	for(;;)
		{
		ss =  ApplyControlToken(&hCtx, &sbout);
		if (FAILED(ss))
			return -1;


	   DWORD           dwSSPIFlags;
		DWORD           dwSSPIOutFlags;
		dwSSPIFlags = ISC_REQ_SEQUENCE_DETECT  |  ISC_REQ_REPLAY_DETECT  | ISC_REQ_CONFIDENTIALITY  | ISC_RET_EXTENDED_ERROR | ISC_REQ_ALLOCATE_MEMORY | ISC_REQ_STREAM;

		OutBuffers[0].pvBuffer   = NULL;
		OutBuffers[0].BufferType = SECBUFFER_TOKEN;
		OutBuffers[0].cbBuffer   = 0;
		sbout.cBuffers  = 1;
		sbout.pBuffers  = OutBuffers;
		sbout.ulVersion = SECBUFFER_VERSION;

		ss = InitializeSecurityContext(&hCred,&hCtx,NULL,dwSSPIFlags,0,SECURITY_NATIVE_DREP, NULL,0,&hCtx,&sbout,&dwSSPIOutFlags,0);
		if (FAILED(ss))
			return -1;

	   PBYTE           pbMessage;
		DWORD           cbMessage;
		pbMessage = (BYTE *)(OutBuffers[0].pvBuffer);
		cbMessage = OutBuffers[0].cbBuffer;

		if (pbMessage != NULL && cbMessage != 0) 
			{
			int rval = ssend_p((char*)pbMessage, cbMessage);
			FreeContextBuffer(pbMessage);
			return rval;
			}
		break;
		}
	return 1;
	}
//------------------------------------------------------------------------------
int  ServerOff()
	{
	// Server wants to disconnect
	SECURITY_STATUS ss;
	Z<SecBuffer> OutBuffers(100);
	DWORD dwType = SCHANNEL_SHUTDOWN;
	OutBuffers[0].pvBuffer   = &dwType;
   OutBuffers[0].BufferType = SECBUFFER_TOKEN;
   OutBuffers[0].cbBuffer   = sizeof(dwType);

   sbout.cBuffers  = 1;
   sbout.pBuffers  = OutBuffers;
   sbout.ulVersion = SECBUFFER_VERSION;

	for(;;)
		{
		ss =  ApplyControlToken(&hCtx, &sbout);
		if (FAILED(ss))
			return -1;


	   DWORD           dwSSPIFlags;
		DWORD           dwSSPIOutFlags;
		dwSSPIFlags = ISC_REQ_SEQUENCE_DETECT  |  ISC_REQ_REPLAY_DETECT  | ISC_REQ_CONFIDENTIALITY  | ISC_RET_EXTENDED_ERROR | ISC_REQ_ALLOCATE_MEMORY | ISC_REQ_STREAM;

		OutBuffers[0].pvBuffer   = NULL;
		OutBuffers[0].BufferType = SECBUFFER_TOKEN;
		OutBuffers[0].cbBuffer   = 0;
		sbout.cBuffers  = 1;
		sbout.pBuffers  = OutBuffers;
		sbout.ulVersion = SECBUFFER_VERSION;

		ss = AcceptSecurityContext(&hCred,&hCtx,NULL,dwSSPIFlags,SECURITY_NATIVE_DREP, NULL,&sbout,&dwSSPIOutFlags,0);
		if (FAILED(ss))
			return -1;

	   PBYTE           pbMessage;
		DWORD           cbMessage;
		pbMessage = (BYTE *)(OutBuffers[0].pvBuffer);
		cbMessage = OutBuffers[0].cbBuffer;

		if (pbMessage != NULL && cbMessage != 0) 
			{
			int rval = ssend_p((char*)pbMessage, cbMessage);
			FreeContextBuffer(pbMessage);
			return rval;
			}
		break;
		}
	return 1;
	}
//------------------------------------------------------------------------------
int  rrecv_p(char *b, int sz)
{
   // same as recv, but forces reading ALL sz
   int rs = 0;
   for (;;)
   {
      int rval = recv(X, b + rs, sz - rs, 0);
      if (rval == 0 || rval == SOCKET_ERROR)
         return rs;
      rs += rval;
      if (rs == sz)
         return rs;
   }
}
//------------------------------------------------------------------------------
int  ssend_p(char *b, int sz)
{
   // same as send, but forces reading ALL sz
   int rs = 0;
   for (;;)
   {
      int rval = send(X, b + rs, sz - rs, 0);
      if (rval == 0 || rval == SOCKET_ERROR)
         return rs;
      rs += rval;
      if (rs == sz)
         return rs;
   }
}
//------------------------------------------------------------------------------
int  recv_p(char *b, int sz)
	{
   return recv(X, b,sz, 0);
	}
//------------------------------------------------------------------------------
int  send_p(char *b, int sz)
	{
   return send(X, b, sz, 0);
	}
//------------------------------------------------------------------------------
int  s_rrecv(char* b,int sz)
	{
   int rs = 0;
   for (;;)
		{
      int rval = s_recv(b + rs, sz - rs);
      if (rval == 0 || rval == SOCKET_ERROR)
         return rs;
      rs += rval;
      if (rs == sz)
         return rs;
		}
	}
//------------------------------------------------------------------------------
int  s_recv(char* b,int sz)
	{
	SecPkgContext_StreamSizes Sizes;
	SECURITY_STATUS ss = 0;
	ss = QueryContextAttributes(&hCtx,SECPKG_ATTR_STREAM_SIZES,&Sizes);
	if (FAILED(ss))
		return -1;

	int TotalR = 0;
	int pI = 0;
	SecBuffer Buffers[5] = {0};
	SecBuffer *     pDataBuffer;
	SecBuffer *     pExtraBuffer;
	Z<char> mmsg(Sizes.cbMaximumMessage*10);


	if (PendingRecvDataSize)
		{
		if (sz <= PendingRecvDataSize)
			{
			memcpy(b,PendingRecvData,sz);
			
			// 
			Z<char> dj(PendingRecvDataSize);
			memcpy(dj,PendingRecvData,PendingRecvDataSize);
			memcpy(PendingRecvData,dj + sz,PendingRecvDataSize - sz);
			PendingRecvDataSize -= sz;
			return sz;
			}
		// else , occupied already
		memcpy(b,PendingRecvData,PendingRecvDataSize);
		sz = PendingRecvDataSize;
		PendingRecvDataSize = 0;
		return sz;
		}

	for(;;)
		{
		unsigned int dwMessage = Sizes.cbMaximumMessage;
		
		if (dwMessage > Sizes.cbMaximumMessage)
			dwMessage = Sizes.cbMaximumMessage;

		int rval = 0;
		if (ExtraDataSize)
			{
			memcpy(mmsg + pI,ExtraData,ExtraDataSize);
			pI += ExtraDataSize;
			ExtraDataSize = 0;
			}
		else
			{
			rval = recv_p(mmsg + pI,dwMessage);
			if (rval == 0 || rval == -1)
				return rval;
			pI += rval;
			}


		Buffers[0].pvBuffer     = mmsg;
		Buffers[0].cbBuffer     = pI;
		Buffers[0].BufferType   = SECBUFFER_DATA;

		Buffers[1].BufferType   = SECBUFFER_EMPTY;
		Buffers[2].BufferType   = SECBUFFER_EMPTY;
		Buffers[3].BufferType   = SECBUFFER_EMPTY;

		sbin.ulVersion = SECBUFFER_VERSION;
		sbin.pBuffers = Buffers;
		sbin.cBuffers = 4;

		ss = DecryptMessage(&hCtx,&sbin,0,NULL);
		if (ss == SEC_E_INCOMPLETE_MESSAGE)
			continue;
		if (ss != SEC_E_OK && ss != SEC_I_RENEGOTIATE && ss != SEC_I_CONTEXT_EXPIRED)
			return -1;

		pDataBuffer  = NULL;
		pExtraBuffer = NULL;
		for (int i = 0; i < 4; i++) 
			{
			if (pDataBuffer == NULL && Buffers[i].BufferType == SECBUFFER_DATA) 
				{
				pDataBuffer = &Buffers[i];
				}
			if (pExtraBuffer == NULL && Buffers[i].BufferType == SECBUFFER_EXTRA) 
				{
				pExtraBuffer = &Buffers[i];
				}
			}
		if (pExtraBuffer)
			{
			ExtraDataSize = pExtraBuffer->cbBuffer;
			ExtraData.Resize(ExtraDataSize + 10);
			memcpy(ExtraData,pExtraBuffer->pvBuffer,ExtraDataSize);
			pI = 0;
			}

		if (ss == SEC_I_RENEGOTIATE)
			{
			ss = ClientLoop();
			if (FAILED(ss))
				return -1;
			}

	

		if (pDataBuffer == 0)
			break;

		TotalR = pDataBuffer->cbBuffer;
		if (TotalR <= sz)
			{
			memcpy(b,pDataBuffer->pvBuffer,TotalR);
			}
		else
			{
			TotalR = sz;
			memcpy(b,pDataBuffer->pvBuffer,TotalR);
			PendingRecvDataSize = pDataBuffer->cbBuffer - TotalR;
			PendingRecvData.Resize(PendingRecvDataSize + 100);
			PendingRecvData.clear();
			memcpy(PendingRecvData,(char*)pDataBuffer->pvBuffer + TotalR,PendingRecvDataSize);
			}


		break;
		}



	return TotalR;
	}
//------------------------------------------------------------------------------
int  s_ssend(char* b,int sz)
	{
	// QueryContextAttributes
	// Encrypt Message
	// ssend

	SecPkgContext_StreamSizes Sizes;
	SECURITY_STATUS ss = 0;
	ss = QueryContextAttributes(&hCtx,SECPKG_ATTR_STREAM_SIZES,&Sizes);
	if (FAILED(ss))
		return -1;

	Z<SecBuffer> Buffers(100);
	int mPos = 0;
	for(;;)
		{
		Z<char> mmsg(Sizes.cbMaximumMessage*2);
		Z<char> mhdr(Sizes.cbHeader*2);
		Z<char> mtrl(Sizes.cbTrailer*2);

		unsigned int dwMessage = sz - mPos;
		if (dwMessage == 0)
			break; // all ok!

		if (dwMessage > Sizes.cbMaximumMessage)
			{
			dwMessage = Sizes.cbMaximumMessage;
			}
		memcpy(mmsg,b + mPos,dwMessage);
		mPos += dwMessage;


		Buffers[0].pvBuffer     = mhdr;
		Buffers[0].cbBuffer     = Sizes.cbHeader;
		Buffers[0].BufferType   = SECBUFFER_STREAM_HEADER;
		Buffers[2].pvBuffer     = mtrl;
		Buffers[2].cbBuffer     = Sizes.cbTrailer;
		Buffers[2].BufferType   = SECBUFFER_STREAM_TRAILER;
		Buffers[3].pvBuffer     = 0;
		Buffers[3].cbBuffer     = 0;
		Buffers[3].BufferType   = SECBUFFER_EMPTY;
		Buffers[1].pvBuffer     = mmsg;
		Buffers[1].cbBuffer     = dwMessage;
		Buffers[1].BufferType   = SECBUFFER_DATA;

		sbin.ulVersion = SECBUFFER_VERSION;
		sbin.pBuffers = Buffers;
		sbin.cBuffers = 4;

		ss = EncryptMessage(&hCtx,0,&sbin,0);
		if (FAILED(ss))
			return -1;

		// Send this message
		int rval;
		rval = ssend_p((char*)Buffers[0].pvBuffer,Buffers[0].cbBuffer);
		if (rval != Buffers[0].cbBuffer)
			return rval;         
		rval = ssend_p((char*)Buffers[1].pvBuffer,Buffers[1].cbBuffer);
		if (rval != Buffers[1].cbBuffer)
			return rval;
		rval = ssend_p((char*)Buffers[2].pvBuffer,Buffers[2].cbBuffer);
		if (rval != Buffers[2].cbBuffer)
			return rval;
		}
	return sz;
	}
//------------------------------------------------------------------------------	
/*
 1 - On the first InitializeSecurityContext call you provide an output empty SECBUFFER_TOKEN buffer to hold the token. 
  Having SEC_I_CONTINUE_NEEDED received, you send the data you have in token output buffer to the remote party, 
  you receive data back in response and make another InitializeSecurityContext call providing input SECBUFFER_TOKEN buffer with received data. 
  If you get SEC_I_CONTINUE_NEEDED result once again, you repeat the whole thing - you send token data to remote party, receive response 
  and again you feed it into SChannel API to continue initialization. 
*/
int  ClientLoop()
{
 DWORD dwSSPIFlags  = ISC_REQ_SEQUENCE_DETECT|ISC_REQ_REPLAY_DETECT|ISC_REQ_CONFIDENTIALITY|ISC_RET_EXTENDED_ERROR|ISC_REQ_ALLOCATE_MEMORY|ISC_REQ_STREAM  | ISC_REQ_MANUAL_CRED_VALIDATION;
 SECURITY_STATUS ss = SEC_I_CONTINUE_NEEDED;
 Z<BYTE> t(0x11000);
 Z<SecBuffer> bufsi(100);
 Z<SecBuffer> bufso(100);
 int pt = 0;
          volatile int brec = 0;
          volatile int indx = 0;
            volatile  SECURITY_STATUS LastStat = 0;
 // Loop using InitializeSecurityContext until success
 for(;;)
  {
   LastStat = ss;
   if((ss != SEC_I_CONTINUE_NEEDED) && (ss != SEC_E_INCOMPLETE_MESSAGE) && (ss != SEC_I_INCOMPLETE_CREDENTIALS))break;
   if(!InitContext)    // Initialize sbout   // Prepare 'Client HELLO'
    {			
     bufso[0].pvBuffer   = NULL;
     bufso[0].BufferType = SECBUFFER_TOKEN;
     bufso[0].cbBuffer   = 0;
     sbout.ulVersion = SECBUFFER_VERSION;
     sbout.cBuffers  = 1;
     sbout.pBuffers  = bufso;
    }
	 else  // Get Some data from the remote site
	  {
       // Add also extradata?
       if(ExtraDataSize)
        {
         memcpy(t,ExtraData,ExtraDataSize);
         pt += ExtraDataSize;
         ExtraDataSize = 0;
        }

       PBYTE Blocks = t + pt; 
       for(UINT Rlen=0x10000,tot=0,NxtOffs=pt;pt < t.bs();)
        {
         PBYTE RdPtr = t + pt;
         Blocks[NxtOffs] = 0;
         int brec = recv(X,(char*)RdPtr,Rlen+5,0);    // <<<< Receive 'Server HELLO'
         if(brec <= 0)
            return brec;  
         pt += brec;
         while(NxtOffs < pt)
          {
           if((Blocks[NxtOffs] < 0x14) || (Blocks[NxtOffs] > 0x18))
             return -2;  // SSL3_RT_HANDSHAKE
           UINT DLen = ((UINT)Blocks[NxtOffs+3] << 8) | Blocks[NxtOffs+4]; // Packet len
           NxtOffs += DLen + 5;
          }
         if(brec == Rlen)break;  // No more data to read (1 extra byte is not received)
         Rlen = NxtOffs - pt;
         if(!Rlen)break;
        }

       // Put this data into the buffer so InitializeSecurityContext will do
       bufsi[0].BufferType = SECBUFFER_TOKEN;
       bufsi[0].cbBuffer   = pt;
       bufsi[0].pvBuffer   = t;

       bufsi[1].BufferType = SECBUFFER_EMPTY;
       bufsi[1].cbBuffer = 0;
       bufsi[1].pvBuffer = 0;

       sbin.ulVersion = SECBUFFER_VERSION;
       sbin.pBuffers  = bufsi;
       sbin.cBuffers  = 2;

       bufso[0].pvBuffer   = NULL;
       bufso[0].BufferType = SECBUFFER_TOKEN;
       bufso[0].cbBuffer   = 0;

       sbout.cBuffers  = 1;
       sbout.pBuffers  = bufso;
       sbout.ulVersion = SECBUFFER_VERSION;
      }
   DWORD dwSSPIOutFlags = 0;
                   indx++;
   ss = InitializeSecurityContext(
			&hCred,
			InitContext ? &hCtx : 0,
			dn,
			dwSSPIFlags,
			0,
			0,//SECURITY_NATIVE_DREP,
			InitContext ? &sbin : 0,
			0,
			InitContext ? 0 : &hCtx,
			&sbout,
			&dwSSPIOutFlags,
			0);

   if(ss == SEC_E_INCOMPLETE_MESSAGE)continue; // allow more
   if(FAILED(ss))                  // SEC_E_INVALID_TOKEN if not everything received!
     return -1;
   if(!InitContext && (ss != SEC_I_CONTINUE_NEEDED))
       return -1;
   pt = 0;
		// Handle possible ExtraData
/*		if (bufsi[1].BufferType == SECBUFFER_EXTRA)
			{
			ExtraDataSize = bufsi[1].cbBuffer;
			ExtraData.Resize(ExtraDataSize + 10);
			memcpy(ExtraData,bufsi[1].pvBuffer,ExtraDataSize);
			}
*/

   int rval = ssend_p((char*)bufso[0].pvBuffer,bufso[0].cbBuffer);  // Pass data to the remote site  // Send Credentials to Server (authenticate the server?)
   FreeContextBuffer(bufso[0].pvBuffer);
   if(rval != bufso[0].cbBuffer)
     return -1;

   if(!InitContext)InitContext = true;    // Send the data we got to the remote part   // Send 'Client HELLO'	
    else if(ss == S_OK)break; // wow!!		
  }
 return 0;
}
//------------------------------------------------------------------------------
int  ClientInit(bool NoLoop = false)
	{
	SECURITY_STATUS ss = 0;
	if (IsExternalCert)
		{
		;
		}
	else
		{
		OurCertificate = CreateOurCertificate();
		}

	// Configure our SSL SChannel
	memset(&m_SchannelCred,0,sizeof(m_SchannelCred));
	m_SchannelCred.dwVersion = SCHANNEL_CRED_VERSION;
	m_SchannelCred.dwFlags |= SCH_CRED_NO_DEFAULT_CREDS;
	m_SchannelCred.dwFlags |= SCH_CRED_NO_DEFAULT_CREDS | SCH_CRED_NO_SYSTEM_MAPPER | SCH_CRED_REVOCATION_CHECK_CHAIN;

	if (OurCertificate)
		{
		m_SchannelCred.cCreds     = 1;
		m_SchannelCred.paCred     = &OurCertificate;
		}

	// AcquireCredentialsHandle

	ss = AcquireCredentialsHandle(0,SCHANNEL_NAME,SECPKG_CRED_OUTBOUND,0,&m_SchannelCred,0,0,&hCred,0);
//	ss = AcquireCredentialsHandle(0,UNISP_NAME,SECPKG_CRED_OUTBOUND,0,&m_SchannelCred,0,0,&hCred,0);
	if (FAILED(ss))
		return 0;

	if (NoLoop)
		return 0;
	return ClientLoop();
	}
//------------------------------------------------------------------------------
int  ServerLoop()
	{
	// Loop AcceptSecurityContext
	SECURITY_STATUS ss = SEC_I_CONTINUE_NEEDED;
	Z<char> t(0x11000);
	Z<SecBuffer> bufsi(100);
	Z<SecBuffer> bufso(100);
	int pt = 0;

	// Loop using InitializeSecurityContext until success
	for(;;)
		{
		if (ss != SEC_I_CONTINUE_NEEDED && ss != SEC_E_INCOMPLETE_MESSAGE && ss != SEC_I_INCOMPLETE_CREDENTIALS)
			break;

		DWORD dwSSPIFlags = ISC_REQ_SEQUENCE_DETECT   |
                  ISC_REQ_REPLAY_DETECT     |
                  ISC_REQ_CONFIDENTIALITY   |
                  ISC_RET_EXTENDED_ERROR    |
                  ISC_REQ_ALLOCATE_MEMORY   |
                  ISC_REQ_STREAM;

		dwSSPIFlags |= ISC_REQ_MANUAL_CRED_VALIDATION;
	
		// Get Some data from the remote site
		int rval = recv(X,t + pt,0x10000,0);
		if (rval == 0 || rval == -1)
			return -1;
		pt += rval;

		// Put this data into the buffer so InitializeSecurityContext will do
		bufsi[0].BufferType = SECBUFFER_TOKEN;
		bufsi[0].cbBuffer = pt;
		bufsi[0].pvBuffer = t;
		bufsi[1].BufferType = SECBUFFER_EMPTY;
		bufsi[1].cbBuffer = 0;
		bufsi[1].pvBuffer = 0;
		sbin.ulVersion = SECBUFFER_VERSION;
		sbin.pBuffers = bufsi;
		sbin.cBuffers = 2;

		bufso[0].pvBuffer  = NULL;
		bufso[0].BufferType= SECBUFFER_TOKEN;
        bufso[0].cbBuffer  = 0;
		bufso[1].BufferType = SECBUFFER_EMPTY;
		bufso[1].cbBuffer = 0;
		bufso[1].pvBuffer = 0;
		sbout.cBuffers      = 2;
		sbout.pBuffers      = bufso;
		sbout.ulVersion     = SECBUFFER_VERSION;


		SEC_E_INTERNAL_ERROR;
		DWORD flg = 0;
		ss = AcceptSecurityContext(
			&hCred,
			InitContext ? &hCtx : 0,
			&sbin,
			ASC_REQ_ALLOCATE_MEMORY,0,
			InitContext ? 0 : &hCtx,
			&sbout,
			&flg,
			0);
	    DBGMSG("AcceptSecurityContext: %08X", ss);   //  SEC_E_DECRYPT_FAILURE 0x80090330; SEC_I_CONTINUE_NEEDED 0x00090312
		InitContext = true;

		if (ss == SEC_E_INCOMPLETE_MESSAGE)
			continue; // allow more

		pt = 0;

		if (FAILED(ss))
			return -1;

		if (InitContext == 0 && ss != SEC_I_CONTINUE_NEEDED)
			return -1;

		// Pass data to the remote site
		rval = ssend_p((char*)bufso[0].pvBuffer,bufso[0].cbBuffer);
		FreeContextBuffer(bufso[0].pvBuffer);
		if (rval != bufso[0].cbBuffer)
			return -1;

		if (ss == S_OK)
			break; // wow!!
		
		}
	return 0;
	}
//------------------------------------------------------------------------------
SECURITY_STATUS  Verify(PCCERT_CONTEXT px)
	{
	if (px == 0)
		return SEC_E_WRONG_PRINCIPAL;

	// Time
	int iRc = CertVerifyTimeValidity(NULL,px->pCertInfo);
	if (iRc != 0) 
		return SEC_E_CERT_EXPIRED;

	// Chain
	CERT_CHAIN_PARA ChainPara = {0};
   PCCERT_CHAIN_CONTEXT pChainContext = NULL;
	ChainPara.cbSize = sizeof(ChainPara);
	if (!CertGetCertificateChain(0,px,0,0,&ChainPara,0,0,&pChainContext)) 
		return SEC_E_INVALID_TOKEN;

/*		ZeroMemory(&polHttps, sizeof(HTTPSPolicyCallbackData));
		polHttps.cbStruct           = sizeof(HTTPSPolicyCallbackData);
		polHttps.dwAuthType         = AUTHTYPE_SERVER;
		polHttps.fdwChecks          = dwCertFlags;
		polHttps.pwszServerName     = pwszServerName;

		memset(&PolicyPara, 0, sizeof(PolicyPara));
		PolicyPara.cbSize            = sizeof(PolicyPara);
		PolicyPara.pvExtraPolicyPara = &polHttps;

		memset(&PolicyStatus, 0, sizeof(PolicyStatus));
		PolicyStatus.cbSize = sizeof(PolicyStatus);

	    if (!CertVerifyCertificateChainPolicy(
                            CERT_CHAIN_POLICY_SSL,
                            pChainContext,
                            &PolicyPara,
                            &PolicyStatus)) {
			Status = ::GetLastError();
			SetLastError(Status);
			break;
		}
*/

	PCCERT_CONTEXT j[2];
	j[0] = px;
	CERT_REVOCATION_STATUS cs = {0};
	cs.cbSize = sizeof(cs);
	SECURITY_STATUS ss = 
		CertVerifyRevocation(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,CERT_CONTEXT_REVOCATION_TYPE,
		 1,(void**)j,0,0,&cs);





    if (pChainContext) 
		CertFreeCertificateChain(pChainContext);

	return ss;
	}
//------------------------------------------------------------------------------
void  GetCertificateInfoString(TCHAR* s)
	{
	PCCERT_CONTEXT pRemoteCertContext = NULL;
	SECURITY_STATUS Status = QueryContextAttributes(&hCtx,SECPKG_ATTR_REMOTE_CERT_CONTEXT,(PVOID)&pRemoteCertContext);
	if (Status != SEC_E_OK) 
		return;

	CertGetNameString(   
		pRemoteCertContext,   
		CERT_NAME_FRIENDLY_DISPLAY_TYPE,   
		0,
		NULL,   
		s,   
		1000);
	CertFreeCertificateContext(pRemoteCertContext);
	}
//------------------------------------------------------------------------------
SECURITY_STATUS  VerifySessionCertificate()
	{
	PCCERT_CONTEXT pRemoteCertContext = NULL;
	SECURITY_STATUS Status = QueryContextAttributes(&hCtx,SECPKG_ATTR_REMOTE_CERT_CONTEXT,(PVOID)&pRemoteCertContext);
	if (Status != SEC_E_OK) 
		return Status;
	Status = Verify(pRemoteCertContext);
	CertFreeCertificateContext(pRemoteCertContext);
	return Status;
	}
//------------------------------------------------------------------------------
void  NoFail(HRESULT hr)
	{
//	if (FAILED(hr))
//		throw;
	}
//------------------------------------------------------------------------------
static PCCERT_CONTEXT  CreateOurCertificate(const wchar_t* Name=nullptr)  //L"Certificate")
{
  NCTM::CEStr EStrProv(MS_DEF_PROV_W);
  NCTM::CEStr EStrName(L"Certificate");
 // NCTM::CEStr EStrCont(L"Container");
  if(!Name)Name = EStrName.Decrypt();
  
	// CertCreateSelfSignCertificate(0,&SubjectName,0,0,0,0,0,0);
	HRESULT hr = 0;
	HCRYPTPROV hProv = NULL;
	PCCERT_CONTEXT p = 0;
	HCRYPTKEY hKey = 0;
	CERT_NAME_BLOB sib = { 0 };
	BOOL AX = 0;

	// Step by step to create our own certificate
//	try
//		{
		// Create the subject
		char cb[1000] = {0};
		sib.pbData = (BYTE*)cb; 
		sib.cbData = 1000;
		wchar_t cnbuf[512];
		lstrcpyW(cnbuf, ctENCSW(L"CN="));
		lstrcatW(cnbuf, Name);
		//wchar_t*	szSubject= L"CN=Certificate";
		if (!CertStrToNameW(CRYPT_ASN_ENCODING, cnbuf,0,0,sib.pbData,&sib.cbData,NULL))return NULL;
			//throw;
	

		// Acquire Context
        wchar_t  KeyContName[128];  // Each unique certificate must have unique key container or next call to CertCreateSelfSignCertificate will overwrite its private key resulting in SEC_E_DECRYPT_FAILURE error
        wchar_t* pszProviderName = EStrProv.Decrypt();   
		wchar_t* pszKeyContainerName = KeyContName;//EStrCont.Decrypt();   //L"Container";
        lstrcpyW(KeyContName, ctENCSW(L"Container_")); //PrintFmt(KeyContName,sizeof(KeyContName),""
        lstrcatW(KeyContName, Name);
        for(int idx=0;KeyContName[idx];idx++)
         if(wchar_t v=KeyContName[idx];v == '.')KeyContName[idx] = '_';  // TODO: Fix more chars
		if (!CryptAcquireContextW(&hProv,pszKeyContainerName,pszProviderName,PROV_RSA_FULL,CRYPT_NEWKEYSET | CRYPT_MACHINE_KEYSET))
			{
			hr = GetLastError();
			if (GetLastError() == NTE_EXISTS)
				{
				if (!CryptAcquireContextW(&hProv,pszKeyContainerName,pszProviderName,PROV_RSA_FULL,CRYPT_MACHINE_KEYSET))return NULL;
					//{
					//throw;
					//}
				}
			else return NULL;
				//throw;
			}

		// Generate KeyPair
		if (!CryptGenKey(hProv, AT_KEYEXCHANGE, CRYPT_EXPORTABLE, &hKey))return NULL;
			// throw;

		// Generate the certificate
		CRYPT_KEY_PROV_INFO kpi = {0};
		kpi.pwszContainerName = pszKeyContainerName;
		kpi.pwszProvName = pszProviderName;
		kpi.dwProvType = PROV_RSA_FULL;
		kpi.dwFlags = CERT_SET_KEY_CONTEXT_PROP_ID;
		kpi.dwKeySpec = AT_KEYEXCHANGE;

		SYSTEMTIME et;
		GetSystemTime(&et);
		et.wYear += 1;	  // Default is +1 anyway

		CERT_EXTENSIONS exts = {0};
		p = CertCreateSelfSignCertificate(hProv,&sib,0,&kpi,NULL,NULL,&et,&exts);

		AX = CryptFindCertificateKeyProvInfo(p,CRYPT_FIND_MACHINE_KEYSET_FLAG,NULL) ;
/*		hCS = CertOpenStore(CERT_STORE_PROV_MEMORY,0,0,CERT_STORE_CREATE_NEW_FLAG,0);
		AX = CertAddCertificateContextToStore(hCS,p,CERT_STORE_ADD_NEW,0);
		AX = CryptFindCertificateKeyProvInfo(p,CRYPT_FIND_MACHINE_KEYSET_FLAG,NULL);*/
//		}	
//	catch(...){}
	
	if (hKey)
		CryptDestroyKey(hKey);
	hKey = 0;
	
	if (hProv)
		CryptReleaseContext(hProv,0);
	hProv = 0;
	return p;
}
//==============================================================================
//                Server functions
//==============================================================================
static int ImportCertificate(PWSTR Name, PWSTR Store, PCCERT_CONTEXT* Cert, HCERTSTORE* hCSi)
{
 // Find certificate in the store
 // Open Certificate Store
 *hCSi = CertOpenSystemStoreW(0,Store);
 if (!*hCSi)
 	return -1;
  
 CERT_RDN cert_rdn;
 CERT_RDN_ATTR cert_rdn_attr;
 
 cert_rdn.cRDNAttr = 1;
 cert_rdn.rgRDNAttr = &cert_rdn_attr;
 
 cert_rdn_attr.pszObjId = szOID_COMMON_NAME;
 cert_rdn_attr.dwValueType = CERT_RDN_ANY_TYPE;
 cert_rdn_attr.Value.cbData = (DWORD)(lstrlenW(Name)*2);
 
 cert_rdn_attr.Value.pbData = (BYTE*)Name;
 *Cert = CertFindCertificateInStore(*hCSi, X509_ASN_ENCODING | PKCS_7_ASN_ENCODING , 0, CERT_FIND_SUBJECT_ATTR, &cert_rdn, NULL);    // CERT_FIND_SUBJECT_ATTR  CERT_FIND_ISSUER_ATTR       // -> CertFreeCertificateContext
 //CertCloseStore(hCSi, 0);
 return 0;		
}
//------------------------------------------------------------------------------
int  ServerInit(bool NoLoop=false)
	{
	SECURITY_STATUS ss = 0;

	if (IsExternalCert)
		{
		;
		}
	else
		{
		//BOOL AX;
		OurCertificate = CreateOurCertificate();
		}

	// Configure our SSL SChannel
	memset(&m_SchannelCred,0,sizeof(m_SchannelCred));
	m_SchannelCred.dwVersion  = SCHANNEL_CRED_VERSION;
	m_SchannelCred.dwFlags   |= SCH_CRED_NO_DEFAULT_CREDS;
	m_SchannelCred.dwFlags    = SCH_CRED_NO_DEFAULT_CREDS | SCH_CRED_NO_SYSTEM_MAPPER | SCH_CRED_REVOCATION_CHECK_CHAIN;
	m_SchannelCred.hRootStore = hCS;
	m_SchannelCred.dwMinimumCipherStrength = 128;




	if (OurCertificate)
		{
		m_SchannelCred.cCreds     = 1;
		m_SchannelCred.paCred     = &OurCertificate;
		}

	// AcquireCredentialsHandle

	ss = AcquireCredentialsHandle(0,ctENCSW(SCHANNEL_NAME),SECPKG_CRED_INBOUND,0,&m_SchannelCred,0,0,&hCred,0);   
//	ss = AcquireCredentialsHandle(0,UNISP_NAME,SECPKG_CRED_INBOUND,0,&m_SchannelCred,0,0,&hCred,0);
	if (FAILED(ss)) { DBGMSG("SECURITY_STATUS: %08X, RootStore: %p", ss, hCS);
		return -1;   }

	if (NoLoop)
		return 0;
    DBGMSG("Looping..., RootStore: %p", hCS);
	return ServerLoop();
	}
//------------------------------------------------------------------------------



	};





