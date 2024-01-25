
#pragma once

#include "VMProtectSDK.h"
#include "MiniString.h"
#include "SHA1.hpp"
#include "Crc32.hpp"
#include "Paq8.hpp"
#include "sshbn.hpp"

//------------------------------------------------------------------------------------------------------------
namespace NVMP
{
#include "Base64.hpp"
//typedef NBP2::NBase64 NBase64;

enum eChunks 
{
    SERIAL_CHUNK_VERSION                = 0x01, //  1 byte of data - version
    SERIAL_CHUNK_USER_NAME              = 0x02, //  1 + N bytes - length + N bytes of customer's name (without enging \0).
    SERIAL_CHUNK_EMAIL                  = 0x03, //  1 + N bytes - length + N bytes of customer's email (without ending \0).
    SERIAL_CHUNK_HWID                   = 0x04, //  1 + N bytes - length + N bytes of hardware id (N % 4 == 0)
    SERIAL_CHUNK_EXP_DATE               = 0x05, //  4 bytes - (year << 16) + (month << 8) + (day)
    SERIAL_CHUNK_RUNNING_TIME_LIMIT     = 0x06, //  1 byte - number of minutes
    SERIAL_CHUNK_PRODUCT_CODE           = 0x07, //  8 bytes - used for decrypting some parts of exe-file
    SERIAL_CHUNK_USER_DATA              = 0x08, //  1 + N bytes - length + N bytes of user data
    SERIAL_CHUNK_MAX_BUILD              = 0x09, //  4 bytes - (year << 16) + (month << 8) + (day)

    SERIAL_CHUNK_END                    = 0xFF  //  4 bytes - checksum: the first four bytes of sha-1 hash from the data before that chunk
};

enum // constants. not a good idea to make them public. it is better to refactor this
{
    SERIAL_SIZE_PRODUCT_CODE = 8,
    SERIAL_SIZE_HWID = 8
};

enum VMProtectErrors
{
    ALL_RIGHT = 0,
    UNSUPPORTED_ALGORITHM = 1,
    UNSUPPORTED_NUMBER_OF_BITS = 2,
    USER_NAME_IS_TOO_LONG = 3,
    EMAIL_IS_TOO_LONG = 4,
    USER_DATA_IS_TOO_LONG = 5,
    HWID_HAS_BAD_SIZE = 6,
    PRODUCT_CODE_HAS_BAD_SIZE = 7,
    SERIAL_NUMBER_TOO_LONG = 8,
    BAD_PRODUCT_INFO = 9,
    BAD_SERIAL_NUMBER_INFO = 10,
    BAD_SERIAL_NUMBER_CONTAINER = 11,
    NOT_EMPTY_SERIAL_NUMBER_CONTAINER = 12,
    BAD_PRIVATE_EXPONENT = 13,
    BAD_MODULUS = 14,
    UTF8_ERROR = 15
};
enum VMProtectSerialNumberFlags
{
    HAS_USER_NAME       = 0x00000001,
    HAS_EMAIL           = 0x00000002,
    HAS_EXP_DATE        = 0x00000004,
    HAS_MAX_BUILD_DATE  = 0x00000008,
    HAS_TIME_LIMIT      = 0x00000010,
    HAS_HARDWARE_ID     = 0x00000020,
    HAS_USER_DATA       = 0x00000040,
    SN_FLAGS_PADDING    = 0xFFFFFFFF
};

enum VMProtectAlgorithms
{
    ALGORITHM_RSA = 0,
    ALGORITHM_PADDING = 0xFFFFFFFF
};

static inline DWORD MakeDate(DWORD y, DWORD m, DWORD d){return (DWORD)((y << 16) | (m << 8) | d);}
//#define MAKEDATE(y, m, d) (DWORD)((y << 16) + (m << 8) + d)

#pragma pack(push, 1)
struct VMProtectProductInfo
{
 VMProtectAlgorithms algorithm;
 SIZE_T      nBits;
 SIZE_T      nPrivateSize;
 byte        *pPrivate;
 SIZE_T      nModulusSize;
 byte        *pModulus;
 SIZE_T      nProductCodeSize;
 byte        *pProductCode;
};
struct VMProtectSerialNumberInfo
{
 INT         flags;
 wchar_t     *pUserName;
 wchar_t     *pEMail;
 DWORD       dwExpDate;
 DWORD       dwMaxBuildDate;
 BYTE        nRunningTimeLimit;
 SIZE_T      nProductCodeSize;
 BYTE        *pProductCode;
 SIZE_T      nHardwareIDLength;
 BYTE        *pHardwareID;
 SIZE_T      nUserDataLength;
 BYTE        *pUserData;
};
#pragma pack(pop)

//------------------------------------------------------------------------------------
static int rand_c(unsigned long *ctx)
{
 if (*ctx == 0)*ctx = 123459876;
 long hi = *ctx / 127773;
 long lo = *ctx % 127773;
 long x = 16807 * lo - 2836 * hi;
 if (x < 0)x += 0x7fffffff;
 return ((*ctx = x) % ((unsigned long)RAND_MAX + 1));
}
static int rand_r(unsigned int *ctx)
{
 unsigned long val = (unsigned long) *ctx;
 int r = rand_c(&val);
 *ctx = (unsigned int) val;
 return (r);
}
static int do_rand(unsigned long seed=0)
{
 static unsigned long next = 1;
 if(seed)next = seed;
 return (rand_c(&next));
}
//------------------------------------------------------------------------------------------------------------
template<typename T> static T ReverseBytes(T Val)
{
 T Res;
 for(int ctr=0;ctr < sizeof(T);ctr++)((unsigned char*)&Res)[ctr] = ((unsigned char*)&Val)[sizeof(T)-ctr-1];
 return Res;
}
//------------------------------------------------------------------------------------------------------------
static int EncodeIdString(CMiniStr& idstr, BYTE XorKey=0)
{
 for(UINT ctr=0;ctr < idstr.Length();ctr++)idstr.c_data()[ctr] ^= XorKey;
 DWORD Nonce = do_rand((GetTickCount() & 0xFFF) * idstr.Length()); 
 DWORD RandA = ~Nonce;  
 idstr.cInsert((char*)&RandA,0,sizeof(DWORD));
 RandA = (Nonce * idstr.Length()); 
 idstr.cAppend((char*)&RandA,sizeof(DWORD));

 NPAQ8::MSTRM SrcStrm, DstStrm;
 SrcStrm.AssignFrom(idstr.c_data(), idstr.Length());
 if(NPAQ8::strm_compress(7, SrcStrm, DstStrm) < 0)return -1;
 unsigned long Size = 0;
 void* Buffer = DstStrm.GetBuffer(&Size); 
 if(Size >= 8)*(UINT*)&((PBYTE)Buffer)[0] ^= *(UINT*)&((PBYTE)Buffer)[Size-4];
 DWORD crc = CCRC32::DoCRC32((PBYTE)Buffer, Size);
 idstr.cAssign((char*)&crc, sizeof(DWORD));
 idstr.cAppend((char*)Buffer, Size);
           //  idstr.ToFile("C:\\Test.bin");
 NBase64::Encode(idstr);  
 return 0;
}
//------------------------------------------------------------------------------------------------------------
static int DecodeIdString(CMiniStr& idstr, BYTE XorKey=0)
{
 NBase64::Decode(idstr);
 if(idstr.Length() < 8)return -1;
 DWORD crc = CCRC32::DoCRC32((PBYTE)&idstr.c_data()[sizeof(DWORD)], idstr.Length()-sizeof(DWORD));
 if(crc != *(PDWORD)idstr.c_data())return -2;
 if(idstr.Length() >= 12)*(UINT*)&idstr.c_data()[sizeof(DWORD)] ^= *(UINT*)&idstr.c_data()[idstr.Length()-4];
 NPAQ8::MSTRM SrcBuf, DstBuf;
 SrcBuf.AssignFrom(&idstr.c_data()[sizeof(DWORD)], idstr.Length()-sizeof(DWORD));
 if(NPAQ8::strm_decompress(7, 0, SrcBuf, DstBuf) < 0)return -3;    // Fastest (Same size)
 unsigned long Size = 0;
 void* Buffer = DstBuf.GetBuffer(&Size);
 if(Size <= 8)return -4;
 idstr.cAssign(&((char*)Buffer)[4], Size-8);
 for(UINT ctr=0;ctr < idstr.Length();ctr++)idstr.c_data()[ctr] ^= XorKey;
 return 0;
}
//------------------------------------------------------------------------------------------------------------
static int GenerateHwidString(CMiniStr& idstr, BYTE XorKey=0)
{
 int nSize = VMProtectGetCurrentHWID(NULL, 0); // get the required buffer size
 if(nSize <= 0)return -1;
 idstr.SetLength(nSize);   // allocate memory for the buffer
 nSize = VMProtectGetCurrentHWID(idstr.c_str(), nSize); // obtain the identifier  // INCLUDES ZERO!  // wEiqarn579WPsgLmwragpYr/bbja6gat/s
 if(nSize <= 4)return -2;
 idstr.SetLength(nSize-1);
// idstr = "wEiqarn579WPsgLmwragpYr/bbja6gat/s";     // Debug !!!
 NBase64::Decode(idstr);
 if(idstr.Length() <= 3)return -3;
     //    idstr.ToFile("N:\\__RawHWID.bin");   // Debug!!!!!!!!!
 if(EncodeIdString(idstr, XorKey) < 0)return -4;
 return 0;
}
//------------------------------------------------------------------------------------
static int TextFromClipboard(CMiniStr& Text)
{
 Text.Clear();
 if(!OpenClipboard(NULL))return -1;
 if(!IsClipboardFormatAvailable(CF_TEXT)){CloseClipboard(); return -2;}
 HANDLE hCBTxt = GetClipboardData(CF_TEXT);
 PVOID DataPointer = GlobalLock(hCBTxt);
 UINT  DataSize    = GlobalSize(hCBTxt);      // Size of the buffer, not the text itself
 if(DataSize)DataSize = lstrlenA((char*)DataPointer);   // May be zeroes at the end
 if(DataPointer && DataSize)Text.cAssign((char*)DataPointer, DataSize);
 GlobalUnlock(hCBTxt);
 CloseClipboard();
 return 0;
}
//------------------------------------------------------------------------------------
static int TextToClipboard(CMiniStr& Text, bool clr=true)
{
 if(!Text.Length())return -1;
 if(!OpenClipboard(NULL))return -2;
 if(clr)EmptyClipboard();
 HANDLE hBufferTxt = GlobalAlloc(GHND,Text.Length()+4);
 PVOID DataPointer = GlobalLock(hBufferTxt);
 if(!DataPointer){CloseClipboard(); return -3;}
 memcpy(DataPointer,Text.c_data(),Text.Length());
 GlobalUnlock(hBufferTxt);
 SetClipboardData(CF_TEXT,hBufferTxt);
 CloseClipboard();
 GlobalFree(hBufferTxt);
 return 0;
}
//------------------------------------------------------------------------------------------------------------
static void SaveStrAsHex(CMiniStr& Str, LPSTR FilePath)
{
 CMiniStr temps;
 temps.SetLength(Str.Length()*2);
 ByteArrayToHexStr(Str.c_data(), temps.c_str(), Str.Length(), true);
 temps.ToFile(FilePath);
}
//------------------------------------------------------------------------------------
//                                  Serial Number 
//------------------------------------------------------------------------------------
static int ToUTF8(wchar_t *pText, CMiniStr& StrOut)
{
 StrOut.Clear();
 if(!pText || !pText[0])return -1; // empty input - empty output
 SIZE_T nLen = wcslen(pText);
 int nSize = WideCharToMultiByte(CP_UTF8, 0, pText, nLen, NULL, 0, NULL, NULL);
 if(nSize == 0)return -2;    // guard check for &vData[0]

 StrOut.SetLength(nSize);    // std::vector<byte>   vData(nSize, 0);
 WideCharToMultiByte(CP_UTF8, 0, pText, nLen, (char *)StrOut.c_data(), StrOut.Length(), NULL, NULL);
 return nSize;
}
//------------------------------------------------------------------------------------
static VMProtectErrors BuildSerialNumber(const VMProtectSerialNumberInfo &info, CMiniStr& vData)
{
 CMiniStr temps;

 vData.Clear();

 vData.AddChars(SERIAL_CHUNK_VERSION);  
 vData.AddChars(1);  

  if(info.flags & HAS_USER_NAME)
  {                                
   vData.AddChars(SERIAL_CHUNK_USER_NAME);  
   if(ToUTF8(info.pUserName, temps) < 0)return UTF8_ERROR;       
   if(temps.Length() > 255)return USER_NAME_IS_TOO_LONG;
   vData.AddChars(temps.Length());
   vData.cAppend((char*)temps.c_str(), temps.Length());    
  }

  if(info.flags & HAS_EMAIL)
  {
   vData.AddChars(SERIAL_CHUNK_EMAIL);  
   if(ToUTF8(info.pEMail, temps) < 0)return UTF8_ERROR;       
   if(temps.Length() > 255) return EMAIL_IS_TOO_LONG;
   vData.AddChars(temps.Length());
   vData.cAppend((char*)temps.c_str(), temps.Length()); 
  }

  if(info.flags & HAS_HARDWARE_ID)   // The length of the data block must be a multiple of 4. The maximum block length is 32 bytes.
  {
   vData.AddChars(SERIAL_CHUNK_HWID);  
   if(info.nHardwareIDLength % 4)return HWID_HAS_BAD_SIZE;
   vData.AddChars(info.nHardwareIDLength);
   vData.cAppend((char*)info.pHardwareID, info.nHardwareIDLength);
  }

  if(info.flags & HAS_EXP_DATE)
  {
   vData.AddChars(SERIAL_CHUNK_EXP_DATE);  
   vData.cAppend((char*)&info.dwExpDate, 4);
  }

  if(info.flags & HAS_TIME_LIMIT)
  {
   vData.AddChars(SERIAL_CHUNK_RUNNING_TIME_LIMIT);   
   vData.AddChars(info.nRunningTimeLimit);   
  }

  {                
   vData.AddChars(SERIAL_CHUNK_PRODUCT_CODE);   
   vData.cAppend((char*)info.pProductCode, info.nProductCodeSize);                 
  }

 if(info.flags & HAS_USER_DATA)
  {
   vData.AddChars(SERIAL_CHUNK_USER_DATA);   
   if(info.nUserDataLength > 255)return USER_DATA_IS_TOO_LONG;
   vData.AddChars(info.nUserDataLength); 
   vData.cAppend((char*)info.pUserData, info.nUserDataLength);   
  }

 if(info.flags & HAS_MAX_BUILD_DATE)
  {
   vData.AddChars(SERIAL_CHUNK_MAX_BUILD);    
   vData.cAppend((char*)&info.dwExpDate, 4);  
  }

 // compute hash
 CSHA1 csa;
 csa.Update(vData.c_data(), vData.Length());
 csa.Final();
 vData.AddChars((char)SERIAL_CHUNK_END);      // add CRC chunk
 UINT32 Hash = ReverseBytes(*(UINT32*)csa.GetHash());
 vData.cAppend((char*)&Hash, 4);
    
 return ALL_RIGHT;
}
//------------------------------------------------------------------------------------
static VMProtectErrors AddPadding(CMiniStr& vData, SIZE_T nMaxBytes)
{
 size_t nMinPadding = 8 + 3;
 size_t nMaxPadding = nMinPadding + 16;
 if(vData.Length() + nMinPadding > nMaxBytes)return SERIAL_NUMBER_TOO_LONG;

 size_t nMaxPaddingAccordingToMaxBytes = nMaxBytes - vData.Length();
 if(nMaxPaddingAccordingToMaxBytes < nMaxPadding) nMaxPadding = nMaxPaddingAccordingToMaxBytes;
 do_rand(GetTickCount());
 size_t nPaddingBytes = nMinPadding;
 if(nMaxPadding > nMinPadding) nPaddingBytes += do_rand() % (nMaxPadding - nMinPadding);
 vData.cInsert(NULL, 0, nPaddingBytes);  
 vData[0] = 0; vData[1] = 2; vData[nPaddingBytes - 1] = 0;
 for(size_t i = 2; i < nPaddingBytes - 1; i++)
 {
  byte b = 0;
  while(!b) b = do_rand() % 255;
  vData[i] = b;
 }

 while (vData.Length() < nMaxBytes) vData.AddChars(do_rand() % 255);
 return ALL_RIGHT;
}
//------------------------------------------------------------------------------------
static VMProtectErrors VMProtectGenerateSerialNumber(VMProtectProductInfo *pProductInfo, VMProtectSerialNumberInfo *pSerialInfo, CMiniStr& SerNumOut)
{
 SerNumOut.Clear();
 // minimal input check
 if(!pProductInfo)return BAD_PRODUCT_INFO;
 if(!pSerialInfo)return BAD_SERIAL_NUMBER_INFO;    

 // check crypt parameters
 if(pProductInfo->algorithm != ALGORITHM_RSA)return UNSUPPORTED_ALGORITHM;
 if(pProductInfo->nBits % 16 != 0 || pProductInfo->nBits < 1024 || pProductInfo->nBits > 16384)return UNSUPPORTED_NUMBER_OF_BITS;

 // check private & modulus
 if(pProductInfo->nPrivateSize == 0 || pProductInfo->pPrivate == NULL)return BAD_PRIVATE_EXPONENT;
 if(pProductInfo->nModulusSize == 0 || pProductInfo->pModulus == NULL)return BAD_MODULUS;

 // check product code
 if(pProductInfo->nProductCodeSize != SERIAL_SIZE_PRODUCT_CODE)return PRODUCT_CODE_HAS_BAD_SIZE;

 CMiniStr    v;
 pSerialInfo->pProductCode = pProductInfo->pProductCode;
 pSerialInfo->nProductCodeSize = pProductInfo->nProductCodeSize;
 VMProtectErrors res = BuildSerialNumber(*pSerialInfo, v);
 if(res != ALL_RIGHT)return res;
 res = AddPadding(v, pProductInfo->nBits / 8);
 if(res != ALL_RIGHT)return res;

 // let's crypt
 SSHBN::Bignum e = SSHBN::bignum_from_bytes(pProductInfo->pPrivate, pProductInfo->nPrivateSize);
 SSHBN::Bignum n = SSHBN::bignum_from_bytes(pProductInfo->pModulus, pProductInfo->nModulusSize);
 SSHBN::Bignum x = SSHBN::bignum_from_bytes(v.c_data(), v.Length());
 if(SSHBN::bignum_cmp(n, x) < 0) // is it possible after check in AddPadding()?
  {
   SSHBN::freebn(e);
   SSHBN::freebn(n);
   SSHBN::freebn(x);
   return SERIAL_NUMBER_TOO_LONG; // data is too long to crypt
  }

 SSHBN::Bignum y = SSHBN::modpow(x, e, n);
 int nBytes;
 byte *pRes = SSHBN::bignum_to_bytes(y, &nBytes);
 SerNumOut.cAssign((char*)pRes, nBytes);    
 
 delete [] pRes;
 SSHBN::freebn(y);
 SSHBN::freebn(x);
 SSHBN::freebn(e);
 SSHBN::freebn(n);

 return ALL_RIGHT;
}
//------------------------------------------------------------------------------------


}