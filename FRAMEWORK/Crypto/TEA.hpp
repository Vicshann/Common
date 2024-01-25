
#pragma once

//------------------------------------------------------------------------------------------------------------
// For any Delta ShL and ShR suggested to be 6 and 9
// Delta: 0xC6EF3720 for 32 rounds
// Delta: 0x9E3779B9 = (sqrt(5)-1) / 2 * 0x100000000
//
template<uint32 Delta=0x9E3779B9, unsigned int Rounds=32, unsigned int ShL=4, unsigned int ShR=5> struct SXTEA
{
// The 'Extended Tiny Encryption Algorithm' (XTEA) by David Wheeler and Roger Needham of the Cambridge Computer Laboratory.
// XTEA is a Feistel cipher with XOR and AND addition as the non-linear mixing functions.
// Takes 64 bits (8 Bytes block) of data in Data[0] and Data[1].
// Returns 64 bits of encrypted data in Data[0] and Data[1].
// Takes 128 bits of key in Key[0] - Key[3]. (16 bytes)

_finline static void EncryptBlock(uint32* _RST Data, const uint32* _RST Key, const uint32 Dlt=Delta)
{
 uint32 Val0=Data[0], Val1=Data[1];
 for(uint32 sum=0,rnd=Rounds;rnd;rnd--)
  {
   Val0 += ((Val1<<ShL ^ Val1>>ShR) + Val1) ^ (sum + Key[sum & 3]);
   sum  += Dlt;
   Val1 += ((Val0<<ShL ^ Val0>>ShR) + Val0) ^ (sum + Key[(sum>>11) & 3]);
  }
 Data[0]=Val0; Data[1]=Val1;
}
//------------------------------------------------------------------------------------------------------------
_finline static void DecryptBlock(uint32* _RST Data, const uint32* _RST Key, const uint32 Dlt=Delta)
{
 uint32 Val0=Data[0], Val1=Data[1];
 for(uint32 sum=Dlt*Rounds,rnd=Rounds;rnd;rnd--)
  {
   Val1 -= ((Val0<<ShL ^ Val0>>ShR) + Val0) ^ (sum + Key[(sum>>11) & 3]);
   sum  -= Dlt;
   Val0 -= ((Val1<<ShL ^ Val1>>ShR) + Val1) ^ (sum + Key[sum & 3]);
  }
 Data[0]=Val0; Data[1]=Val1;
}
//------------------------------------------------------------------------------------------------------------
_finline static void Encrypt(void* Data, const void* Key, const size_t DataSize, const uint32 Dlt=Delta)
{
 const uint64* Pointer = (uint64*)Data;    // 64bit block
 const uint64* EndPtr  = (uint64*)((uint8*)Data + (DataSize & (size_t)~7));   // Skip unaligned bytes at the end (Aligned to 8)
 for(;Pointer < EndPtr;Pointer++)EncryptBlock((uint32*)Pointer, (uint32*)Key, Dlt);
}
//------------------------------------------------------------------------------------------------------------
_finline static void Decrypt(void* Data, const void* Key, const size_t DataSize, const uint32 Dlt=Delta)
{
 const uint64* Pointer = (uint64*)Data;    // 64bit block
 const uint64* EndPtr  = (uint64*)((uint8*)Data + (DataSize & (size_t)~7));   // Skip unaligned bytes at the end (Aligned to 8)
 for(;Pointer < EndPtr;Pointer++)DecryptBlock((uint32*)Pointer, (uint32*)Key, Dlt);
}
//------------------------------------------------------------------------------------------------------------

};

using STEA = SXTEA<0,32,6,9>;

