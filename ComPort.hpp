
#pragma once

class CComPort
{
 HANDLE hComm;
 DCB ComPortParams;
 COMMTIMEOUTS ComPTO;

public:
CComPort(void){this->hComm = INVALID_HANDLE_VALUE;}
~CComPort(){this->Close();}
//---------------------------------------------------------------------------
bool IsOpened(void){return (this->hComm != INVALID_HANDLE_VALUE);}
//---------------------------------------------------------------------------
int Open(UINT Index, UINT BaudRate, UINT DataBits=8, UINT StopBits=0, UINT Parity=0, UINT RdIntTimeout=500, UINT RdTotTimeoutMult=1, UINT RdTotTimeoutConst=1000)
{
 BYTE CommPath[128];
 wsprintf((LPSTR)&CommPath,"COM%u",Index);
 this->Close();
 this->hComm = CreateFile((LPSTR)&CommPath,GENERIC_READ|GENERIC_WRITE,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
 if(this->hComm == INVALID_HANDLE_VALUE)return -8;
 return this->ConfigureComm(BaudRate,DataBits,StopBits,Parity,RdIntTimeout,RdTotTimeoutMult,RdTotTimeoutConst);
}
//---------------------------------------------------------------------------
int Close(void)
{
 if(this->hComm == INVALID_HANDLE_VALUE)return -1;
 this->Purge(true,true);
 CloseHandle(this->hComm);
 this->hComm = INVALID_HANDLE_VALUE;
 return 0;
}
//---------------------------------------------------------------------------
int Read(PBYTE Buf, UINT Len, UINT* Num)
{
 DWORD Result;
 if(Num)*Num = 0;
// LOGMSG("COM port reading %u bytes...",Len);
 if(!ReadFile(this->hComm,Buf,Len,&Result,NULL))return -1;
 if(Num)*Num = Result;
// LOGMSG("COM port read %u bytes",Result);
//   if(Result)DumpHexData(Buf, Result);
 return 0;
}
//---------------------------------------------------------------------------
int Write(PBYTE Buf, UINT Len, UINT* Num)
{
 DWORD Result;
 if(Num)*Num = 0;
// LOGMSG("COM port writing %u bytes...",Len);
  // if(Result)DumpHexData(Buf, Len);
 if(!WriteFile(this->hComm,Buf,Len,&Result,NULL))return -1;
 if(Num)*Num = Result;
// LOGMSG("COM port written %u bytes",Result);
 return 0;
}
//---------------------------------------------------------------------------
int Purge(bool Rx, bool Tx)
{
 UINT val = 0;
 if(Rx)val |= (PURGE_RXABORT|PURGE_RXCLEAR);
 if(Tx)val |= (PURGE_TXABORT|PURGE_TXCLEAR);
 PurgeComm(this->hComm,val);
 return 0;
}
//---------------------------------------------------------------------------
private:
int WaitComm(void)
{
 DWORD Mask = 0;
 if(!WaitCommEvent(this->hComm,&Mask,NULL))return 0;
 return Mask;
}
//---------------------------------------------------------------------------
int ConfigureComm(UINT BaudRate, UINT DataBits, UINT StopBits, UINT Parity, UINT RdIntTimeout, UINT RdTotTimeoutMult, UINT RdTotTimeoutConst)
{
 const int CommBufSize = 1024;

 if(!SetupComm(this->hComm,CommBufSize,CommBufSize))return -1;

 if(!GetCommTimeouts(this->hComm,&ComPTO))return -2;
 ComPTO.ReadIntervalTimeout         = RdIntTimeout; //0;    //500;  //2500;//MAXDWORD;
 ComPTO.ReadTotalTimeoutMultiplier  = RdTotTimeoutMult; //100;  //1;    // 100;//1; //RcvTimeout;//MAXDWORD;    //mul ReadBytes
 ComPTO.ReadTotalTimeoutConstant    = RdTotTimeoutConst; //1000; //1000; //1000;//1000; //this->CommTimeout;//MAXDWORD-1;//100;    //added to result prev
 ComPTO.WriteTotalTimeoutMultiplier = 0;    //TrmTimeout;   //mul Writebytes
 ComPTO.WriteTotalTimeoutConstant   = 0;
 if(!SetCommTimeouts(this->hComm,&ComPTO))return -3;

 if(!GetCommState(this->hComm,&ComPortParams))return -4;
 ComPortParams.BaudRate = BaudRate;       // current baud rate
 ComPortParams.fBinary = 1;             // binary mode, no EOF check
 ComPortParams.fParity = 0;             // enable parity checking      <<<<
 ComPortParams.fOutxCtsFlow = 0;        // CTS output flow control
 ComPortParams.fOutxDsrFlow = 0;        // DSR output flow control
 ComPortParams.fDtrControl = 1;         // DTR flow control type
 ComPortParams.fDsrSensitivity = 0;     // DSR sensitivity
 ComPortParams.fTXContinueOnXoff = 0;   // XOFF continues Tx
 ComPortParams.fOutX = 0;               // XON/XOFF out flow control
 ComPortParams.fInX = 0;                // XON/XOFF in flow control
 ComPortParams.fErrorChar = 0;          // enable error replacement
 ComPortParams.fNull = 0;               // enable null stripping
 ComPortParams.fRtsControl = 1;         // RTS flow control
 ComPortParams.fAbortOnError = 0;       // abort reads/writes on error
//    ComPortParams.fDummy2:17;            // reserved
//    ComPortParams.wReserved;             // not currently used
 ComPortParams.XonLim = 2048;           // transmit XON threshold
 ComPortParams.XoffLim = 512;           // transmit XOFF threshold
 ComPortParams.ByteSize = DataBits;            // number of bits/byte, 4-8
 ComPortParams.Parity = Parity;              // 0-4=no,odd,even,mark,space    <<<<
 ComPortParams.StopBits = StopBits;            // 0,1,2 = 1, 1.5, 2             <<<<
 ComPortParams.XonChar = 17;            // Tx and Rx XON character
 ComPortParams.XoffChar = 19;           // Tx and Rx XOFF character
 ComPortParams.ErrorChar = 0;           // error replacement character
 ComPortParams.EofChar = 0;             // end of input character
 ComPortParams.EvtChar = 0;             // received event character
//    ComPortParams.wReserved1;            // reserved; do not use
 ComPortParams.DCBlength = sizeof(DCB); // sizeof(DCB)
 if(!SetCommState(this->hComm,&ComPortParams))return -5;
// if(!PurgeComm(this->hComm,PURGE_TXABORT|PURGE_RXABORT|PURGE_TXCLEAR|PURGE_RXCLEAR))return -6;
// if(!SetCommMask(this->hComm,EV_RXCHAR|EV_TXEMPTY))return -7;
 return 0;
}
//---------------------------------------------------------------------------

};
//---------------------------------------------------------------------------
