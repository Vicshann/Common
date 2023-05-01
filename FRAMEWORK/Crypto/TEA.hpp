
#pragma once

//------------------------------------------------------------------------------------------------------------
// For any Delta ShL and ShR suggested to be 6 and 9
// Delta: 0xC6EF3720 for 32 rounds
// Delta: 0x9E3779B9 = (sqrt(5)-1) / 2 * 0x100000000
//
template<unsigned int Delta=0x9E3779B9, unsigned int Rounds=32, unsigned int ShL=4, unsigned int ShR=5> struct SXTEA
{
// The 'Extended Tiny Encryption Algorithm' (XTEA) by David Wheeler and Roger Needham of the Cambridge Computer Laboratory.
// XTEA is a Feistel cipher with XOR and AND addition as the non-linear mixing functions.
// Takes 64 bits (8 Bytes block) of data in Data[0] and Data[1].
// Returns 64 bits of encrypted data in Data[0] and Data[1].
// Takes 128 bits of key in Key[0] - Key[3]. (16 bytes)

_finline static void EncryptBlock(unsigned int *Data, unsigned int *Key, unsigned int Dlt=Delta)
{
 for(unsigned long sum=0,rnd=Rounds;rnd;rnd--)
  {
   Data[0] += ((Data[1]<<ShL ^ Data[1]>>ShR) + Data[1]) ^ (sum + Key[sum & 3]);
   sum += Dlt;
   Data[1] += ((Data[0]<<ShL ^ Data[0]>>ShR) + Data[0]) ^ (sum + Key[sum>>11 & 3]);
  }
}
//------------------------------------------------------------------------------------------------------------
_finline static void DecryptBlock(unsigned int *Data, unsigned int *Key, unsigned int Dlt=Delta)
{
 for(unsigned long sum=Dlt*Rounds,rnd=Rounds;rnd;rnd--)
  {
   Data[1] -= ((Data[0]<<ShL ^ Data[0]>>ShR) + Data[0]) ^ (sum + Key[sum>>11 & 3]);
   sum -= Dlt;
   Data[0] -= ((Data[1]<<ShL ^ Data[1]>>ShR) + Data[1]) ^ (sum + Key[sum & 3]);
  }
}
//------------------------------------------------------------------------------------------------------------
_finline static void Encrypt(void* Data, void* Key, unsigned long DataSize, unsigned int Dlt=Delta)
{
 unsigned long long *Pointer = (unsigned long long*)Data;
 for(unsigned long Index=0,Total=DataSize>>3;Index < Total;Index++)EncryptBlock((unsigned int*)&Pointer[Index], (unsigned int*)Key, Dlt);
}
//------------------------------------------------------------------------------------------------------------
_finline static void Decrypt(void* Data, void* Key, unsigned long DataSize, unsigned int Dlt=Delta)
{
 unsigned long long *Pointer = (unsigned long long*)Data;
 for(unsigned long Index=0,Total=DataSize>>3;Index < Total;Index++)DecryptBlock((unsigned int*)&Pointer[Index], (unsigned int*)Key, Dlt);
}
//------------------------------------------------------------------------------------------------------------

};

using STEA = SXTEA<0,32,6,9>;

