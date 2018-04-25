// ssl.h
#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>
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





