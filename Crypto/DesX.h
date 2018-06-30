//---------------------------------------------------------------------------
/* blatantly stolen from somwhere on the internet... i don't remember where, from RSA i think.*/

#ifndef DesXH
#define DesXH

#include <Windows.h>
//---------------------------------------------------------------------------
#define ID_OK   0
#define RE_LEN  1

typedef struct {
  UINT32 subkeys_enc[32];                                             /* subkeys */
  UINT32 subkeys_dec[32];                                             /* subkeys */
  UINT32 inputWhitener[2];                                 /* input whitener */
  UINT32 outputWhitener[2];                               /* output whitener */
  UINT32 iv[2];                                       /* initializing vector */
  UINT32 originalIV[2];                        /* for restarting the context */
} DESX_CBC_CTX;

void DESX_CBCInit(DESX_CBC_CTX *context, UINT8 *key, UINT8 *iv);
int  DESX_DecryptBlock(DESX_CBC_CTX *context, UINT8 *output, UINT8 *input);
int  DESX_CBCUpdate(DESX_CBC_CTX *context, UINT8 *output, UINT8 *input, size_t len, int encrypt);
void DESX_CBCEncryptBlk(DESX_CBC_CTX *context, UINT8 *output, UINT8 *input);
void DESX_CBCDecryptBlk(DESX_CBC_CTX *context, UINT8 *output, UINT8 *input);
void DESX_CBCRestart(DESX_CBC_CTX *context);

void desfunc(UINT32 *block, UINT32 *ks);
void scrunch(UINT32 *into, UINT8 *outof);
void unscrunch(UINT8 *into, UINT32 *outof);
void deskey(UINT32 subkeys[32], UINT8 key[8], int encrypt);

PBYTE GetClorox(void);
//---------------------------------------------------------------------------
#endif
