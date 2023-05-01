
#pragma once

/*
https://www.ibm.com/docs/en/ztpf/1.1.0.15?topic=sessions-ssl-record-format
https://commandlinefanatic.com/cgi-bin/showarticle.cgi?article=art059


Format of an SSL record:
   Byte   0       = SSL record type
   Bytes 1-2      = SSL version (major/minor)
   Bytes 3-4      = Length of data in the record (excluding the header itself). The maximum SSL supports is 16384 (16K).

Format of an SSL handshake record:
   Byte   0       = SSL record type = 22 (SSL3_RT_HANDSHAKE)
   Bytes 1-2      = SSL version (major/minor)
   Bytes 3-4      = Length of data in the record (excluding the header itself).
   Byte   5       = Handshake type
   Bytes 6-8      = Length of data to follow in this record
   Bytes 9-n      = Command-specific data
*/
namespace NTLS
{
//--------------------------------------------------------------------------- 
enum ERecType
{
SSL3_RT_CHANGE_CIPHER_SPEC = 0x14,
SSL3_RT_ALERT              = 0x15,
SSL3_RT_HANDSHAKE          = 0x16,
SSL3_RT_APPLICATION_DATA   = 0x17,
TLS1_RT_HEARTBEAT          = 0x18,
};

enum ESSLVer
{
TLS1_VERSION   = 0x0103,    //   03 01
TLS1_1_VERSION = 0x0203,    //   03 02
TLS1_2_VERSION = 0x0303,    //   03 03
};

enum EHSType
{
SSL3_MT_HELLO_REQUEST         =   0, // (x'00')
SSL3_MT_CLIENT_HELLO          =   1, // (x'01')
SSL3_MT_SERVER_HELLO          =   2, // (x'02')
SSL3_MT_NEWSESSION_TICKET     =   4, // (x'04')
SSL3_MT_CERTIFICATE           =  11, // (x'0B')
SSL3_MT_SERVER_KEY_EXCHANGE   =  12, // (x'0C')
SSL3_MT_CERTIFICATE_REQUEST   =  13, // (x'0D')
SSL3_MT_SERVER_DONE           =  14, // (x'0E')
SSL3_MT_CERTIFICATE_VERIFY    =  15, // (x'0F')
SSL3_MT_CLIENT_KEY_EXCHANGE   =  16, // (x'10')
SSL3_MT_FINISHED              =  20, // (x'14')
};


enum EOther
{
// The ChangeCipherspec type has the following value:
SSL3_MT_CCS      = 1,
// The heartbeat type has the following values:
TLS1_HB_REQUEST  = 1,
TLS1_HB_RESPONSE = 2,
// Format of an SSL alert record:
SSL3_AL_WARNINGS = 1,
SSL3_AL_FATAL    = 2,
};


enum EAlerts
{
SSL3_AD_CLOSE_NOTIFY                    =   0,   //   x'00'
SSL3_AD_UNEXPECTED_MESSAGE              =  10,   //   x'0A'
SSL3_AD_BAD_RECORD_MAC                  =  20,   //   x'14'
TLS1_AD_DECRYPTION_FAILED               =  21,   //   x'15'
TLS1_AD_RECORD_OVERFLOW                 =  22,   //   x'16'
SSL3_AD_DECOMPRESSION_FAILURE           =  30,   //   x'1E'
SSL3_AD_HANDSHAKE_FAILURE               =  40,   //   x'28'
SSL3_AD_NO_CERTIFICATE                  =  41,   //   x'29'
SSL3_AD_BAD_CERTIFICATE                 =  42,   //   x'2A'
SSL3_AD_UNSUPPORTED_CERTIFICATE         =  43,   //   x'2B'
SSL3_AD_CERTIFICATE_REVOKED             =  44,   //   x'2C'
SSL3_AD_CERTIFICATE_EXPIRED             =  45,   //   x'2D'
SSL3_AD_CERTIFICATE_UNKNOWN             =  46,   //   x'2E'
SSL3_AD_ILLEGAL_PARAMETER               =  47,   //   x'2F'
TLS1_AD_UNKNOWN_CA                      =  48,   //   x'30'
TLS1_AD_ACCESS_DENIED                   =  49,   //   x'31'
TLS1_AD_DECODE_ERROR                    =  50,   //   x'32'
TLS1_AD_DECRYPT_ERROR                   =  51,   //   x'33'
TLS1_AD_EXPORT_RESTRICTION              =  60,   //   x'3C'
TLS1_AD_PROTOCOL_VERSION                =  70,   //   x'46'
TLS1_AD_INSUFFICIENT_SECURITY           =  71,   //   x'47'
TLS1_AD_INTERNAL_ERROR                  =  80,   //   x'50'
TLS1_AD_USER_CANCELLED                  =  90,   //   x'5A'
TLS1_AD_NO_RENEGOTIATION                = 100,   //   x'64'
TLS1_AD_UNSUPPORTED_EXTENSION           = 110,   //   x'6E'   
TLS1_AD_CERTIFICATE_UNOBTAINABLE        = 111,   //   x'6F' 
TLS1_AD_UNRECOGNIZED_NAME               = 112,   //   x'70'
TLS1_AD_BAD_CERTIFICATE_STATUS_RESPONSE = 113,   //   x'71'
TLS1_AD_BAD_CERTIFICATE_HASH_VALUE      = 114,   //   x'72'
TLS1_AD_UNKNOWN_PSK_IDENTITY            = 115,   //   x'73'
};
//--------------------------------------------------------------------------- 

//--------------------------------------------------------------------------- 

//--------------------------------------------------------------------------- 

//--------------------------------------------------------------------------- 
};