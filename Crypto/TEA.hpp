
#pragma once


//---------------------------------------------------------------------------
namespace NXTEA
{
// The 'Extended Tiny Encryption Algorithm' (XTEA) by David Wheeler and Roger Needham of the Cambridge Computer Laboratory.                      
// XTEA is a Feistel cipher with XOR and AND addition as the non-linear mixing functions.                                                         
// Takes 64 bits (8 Bytes block) of data in Data[0] and Data[1].             
// Returns 64 bits of encrypted data in Data[0] and Data[1].                 
// Takes 128 bits of key in Key[0] - Key[3].                                 

static void _fastcall EncryptBlock(unsigned long *Data, unsigned long *Key, unsigned long Rounds=32)
{
 unsigned long delta = 0x9E3779B9;
 unsigned long sum   = 0;
 while(Rounds--)
  {
   Data[0] += ((Data[1]<<4 ^ Data[1]>>5) + Data[1]) ^ (sum + Key[sum&3]);
   sum += delta;
   Data[1] += ((Data[0]<<4 ^ Data[0]>>5) + Data[0]) ^ (sum + Key[sum>>11 & 3]);
  }
}
//-----------------------------------------------------------------------------
static void _fastcall DecryptBlock(unsigned long *Data, unsigned long *Key, unsigned long Rounds=32)
{
 unsigned long delta = 0x9E3779B9;
 unsigned long sum   = delta * Rounds;       // 0xC6EF3720 for 32 rounds
 while(Rounds--)
  {
   Data[1] -= ((Data[0]<<4 ^ Data[0]>>5) + Data[0]) ^ (sum + Key[sum>>11 & 3]);
   sum -= delta;
   Data[0] -= ((Data[1]<<4 ^ Data[1]>>5) + Data[1]) ^ (sum + Key[sum&3]);
  }
}
//-----------------------------------------------------------------------------
static void _fastcall Encrypt(void* Data, void* Key, unsigned long DataSize)
{
 unsigned __int64 *Pointer = (unsigned __int64*)Data;
 for(unsigned long Index=0,Total=DataSize/8;Index < Total;Index++)EncryptBlock((unsigned long*)&Pointer[Index], (unsigned long*)Key);
}
//-----------------------------------------------------------------------------
static void _fastcall Decrypt(void* Data, void* Key, unsigned long DataSize)
{
 unsigned __int64 *Pointer = (unsigned __int64*)Data;
 for(unsigned long Index=0,Total=DataSize/8;Index < Total;Index++)DecryptBlock((unsigned long*)&Pointer[Index], (unsigned long*)Key);
}
//-----------------------------------------------------------------------------

};
//-----------------------------------------------------------------------------


