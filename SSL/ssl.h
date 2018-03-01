// ssl.h
#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <wincrypt.h>
#include <tchar.h>
#define SECURITY_WIN32
#include <security.h>
#include <schnlsp.h>

#include "z.h"

class SSL_SOCKET
	{
	public:
		SOCKET X;

		void Initialize(SOCKET,int,PCCERT_CONTEXT = 0);
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
		void NoFail(HRESULT);




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


	};





