
#include "Montgomery.hpp"

// Just to test our ability to use Montgomery
// Modulus = PrimeA * PrimeB
// Exponent is any number between 1 and LCM(Modulus) and is not part of key pair creation(Just for encryption/decryption)
// LCM(Carmichael’s totient function): LCM(Modulus) = lcm(PrimeA-1, PrimeB-1)
// PrivKey = 'modular inverse' of Exponent and LCM(Modulus)   
// Ciphertext = (Message^Exponent) mod Modulus
// Here encryption is m = c^d mod n  // Se we are looking for this super secret 'd' exponent which is the PrivateKey
//
// HexStrToByteArray(Exponent, "00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000010001", ValSize);
// HexStrToByteArray(Modulus,  "BCA83D793F493C49DF558612E74C773198AB4901F20369BFAF1598D71E362EF13AB9BE3B4D4D73C63378542D23BEBA56AD4D589C1E7F151E25CF6F7A38F8FF1FF491D5D2DFC971617B6D9559406E3A5127B2AEBDDEA965E0DFCF4C50AE241CAF9E87BFE33B0DB619B5C395E3986E310A3278F990B4139A421AF74B3E4E1548250DEC8F1755B038E61069E2547983ED93878549B4A9F5FAA1BEF72A75A9929FA7240FB1E46B9587170EF993C29C35F1F145E55BFEC0DE85D2B9409D6599B1C348BF76DD441ABD53033475E3267F91647C2584D974D3AD7B8C0C33711556D6C2CF23BF7905B17A68C622A0580A623C1AF9F446294D5F2DE50721D85EB5F49B7013", ValSize);
// HexStrToByteArray(Value,    "8790AACADBDC380EEE9C3D1E10CB4A03FE3D97392C4F3ED47EBA6657181C7B6EBE5BB18B33468DD38389F5F936F4632905CEFE55D263CE6568A1B2B45F44796A244BAB756C3B0000D196C81F4669D11535FB23F20B9BB2EC9FD8B34B713FA2E7B638EF3B4BB80E0FFB4E2793232D85D1EED18E649511982BFEE2BA7D283F6068A9EC7917CE7348A39A67040CE1E0864986A17D9BED3EA5ACB94D568B11552483D5E3EA3B5A18AD38E362B2250D310E46C48C656F4CD55266A1341E6E81AB2E4FE263F8783AE619C6D43391E4833E50182F4565A11806E5E05A1E8317C2169B2B0AE3B55D25B626219801055097EAE779E46BCEF918D8224B95F59819760C2372", ValSize);
// NMontgomery::Exponentiate(Value, Exponent, Modulus, Result, ValSize, true);
//
class CRSA
{
public:
 enum EHashAlgo {haSHA256=1};

private:
 static const UINT MaxSize = 512;
 UINT  SizeInBytes;
 UINT8 Exponent[MaxSize];
 UINT8 Modulus[MaxSize];

//------------------------------------------------------------------------------
static UINT AlignSize(UINT DSize)
{
 for(UINT Idx=0;Idx < 32;Idx++)
  {
   UINT32 VLen = 1 << Idx;
   if(DSize <= VLen){DSize = VLen; break;}
  }
 return DSize;
}
//------------------------------------------------------------------------------
void SetValue(UINT8* DstVal, UINT8* SrcPtr, UINT SrcSize)
{
 memcpy(&DstVal[this->SizeInBytes - SrcSize], SrcPtr, SrcSize);
}
//------------------------------------------------------------------------------
// Returns string of format: "0001" + padding_string + "00" + algorithm_representation + hash
int HashDataForSign(EHashAlgo Alg, UINT8* Data, UINT Size, UINT8* Out)
{
 int HashLenInChars = 0; 
 switch(Alg)
  {
   case haSHA256:
    {
     UINT8  Desc[] = {0x30,0x31,0x30,0x0d,0x06,0x09,0x60,0x86,0x48,0x01,0x65,0x03,0x04,0x02,0x01,0x05,0x00,0x04,0x20};   // algorithm representation
     UINT8  Hash[32];
     CSha256 sha;
     sha.Update(Data, Size);
     sha.Final(Hash);
     memset(Out, 0xFF, this->SizeInBytes);
     UINT Offs = 0;
     Out[Offs++] = 0;
     Out[Offs++] = 1;
     Offs = (this->SizeInBytes - (sizeof(Hash)+sizeof(Desc)+1));
     Out[Offs++] = 0;
     memcpy(&Out[Offs], &Desc, sizeof(Desc));      // ByteArrayToHexStr((UINT8*)&Desc, &Out[Offs], sizeof(Desc), false);
     Offs += sizeof(Desc);
      memcpy(&Out[Offs], &Hash, sizeof(Hash));   //ByteArrayToHexStr((UINT8*)&Hash, &Out[Offs], sizeof(Hash), false);
     Offs += sizeof(Hash); 
     HashLenInChars = sizeof(Hash);
    }
    break;
   default: return -1;
  }
 DBGMSG("Digest value: %*D",this->SizeInBytes,Out);
 return HashLenInChars;
}
//------------------------------------------------------------------------------
public:
CRSA(void)
{
 this->SizeInBytes = 0;
}
//------------------------------------------------------------------------------
int LoadFromPem(char* PemStr, UINT Size=0)
{
 CMiniStr TmpStr;
 TmpStr.cAssign(PemStr, Size);
 TmpStr.RemoveChars("\r\n");
 int p = TmpStr.Pos("-----");
 if(p >= 0)
  {
   p = TmpStr.Pos("-----", p+5);
   if(p >= 0)
    {
     TmpStr.Delete(0, p+5);
     p = TmpStr.Pos("-----");
     if(p >= 0)TmpStr.SetLength(p);
    }
  }
 NBase64::Decode(TmpStr);
 return this->LoadFromDer(TmpStr.c_data(), TmpStr.Length());
}
//------------------------------------------------------------------------------
int LoadFromDer(UINT8* DerBin, UINT Size)
{
 PBYTE ModPtr = nullptr; 
 PBYTE ExpPtr = nullptr; 
 BYTE TmpExp[MaxSize] = {};
 BYTE TmpMod[MaxSize] = {};
#ifdef _DEBUG
 DumpBufferASN1(DerBin, Size);
#endif       
 long ModLen = GetTypeFromASN1(&ModPtr, DerBin, Size, 2, 0); // Modulus (Big endian)
 long ExpLen = GetTypeFromASN1(&ExpPtr, DerBin, Size, 2, 1); // Exponent (Big endian)
 return this->LoadFromBin(ModPtr, ExpPtr, ModLen, ExpLen);
}
//------------------------------------------------------------------------------
// BigEndian input
int LoadFromBin(UINT8* ModBin, UINT ModLen, UINT32 Exponent=65537)
{
 BYTE TmpExp[MaxSize] = {};
 for(UINT Idx=1;Idx <= sizeof(Exponent);Idx++,Exponent >>= 8)TmpExp[sizeof(TmpExp)-Idx] = Exponent & 0xFF;
 return this->LoadFromBin(ModBin, TmpExp, ModLen, sizeof(TmpExp));
}
//------------------------------------------------------------------------------
// BigEndian input
int LoadFromBin(UINT8* ModPtr, UINT8* ExpPtr, UINT ModLen, UINT ExpLen)
{
 if(!*ModPtr){ModPtr++; ModLen--;}    // Remove BigEndian signed ext 
 if(!*ExpPtr){ExpPtr++; ExpLen--;}
 if(ModLen > MaxSize){DBGMSG("Modulus is too big: %u", ModLen); return -8;}
 if(ExpLen > MaxSize){DBGMSG("Exponent is too big: %u", ExpLen); return -9;}

 UINT DSize = ModLen;
 if(ExpLen > DSize)DSize = ExpLen;

 for(UINT Idx=0;Idx < 32;Idx++)
  {
   UINT32 VLen = 1 << Idx;
   if(DSize <= VLen){DSize = VLen; break;}
  }
 memset(this,0,sizeof(CRSA));
 this->SizeInBytes = DSize;

 this->SetValue(this->Modulus, ModPtr, ModLen);     // Big endian only!
 DBGMSG("Modulus: %*D",ModLen,ModPtr);

 this->SetValue(this->Exponent, ExpPtr, ExpLen);    // Big endian only! // Skip any sign padding zeroes
 DBGMSG("Public exponent: %*D",ExpLen,ExpPtr);
 return 0;
}
//------------------------------------------------------------------------------
// Returns 0 if signature is OK, 1 if not, negative if error
int CheckSignature(UINT8* MsgData, UINT MsgSize, UINT8* SigData, UINT SigSize, EHashAlgo HashAlg)
{
 UINT8 Result[MaxSize];
 UINT8 Message[MaxSize];
 UINT8 Signature[MaxSize];
 int HashLen = this->HashDataForSign(haSHA256, MsgData, MsgSize, Message);
 if(HashLen < 0)return -1;
 if(SigSize != this->SizeInBytes)return -2;   // Mismatch
 this->SetValue(Signature, SigData, SigSize);

 NMontgomery::Exponentiate(Signature, this->Exponent, this->Modulus, Result, this->SizeInBytes, true);
 DBGMSG("Decrypted signature: %*D",this->SizeInBytes,&Result);
 return (memcmp(&Result[this->SizeInBytes - HashLen], &Message[this->SizeInBytes - HashLen], HashLen) != 0);
}
//------------------------------------------------------------------------------
};

/*
These values come from RFC3447:
MD2:     (0x)30 20 30 0c 06 08 2a 86 48 86 f7 0d 02 02 05 00 04 10 || H.
MD5:     (0x)30 20 30 0c 06 08 2a 86 48 86 f7 0d 02 05 05 00 04 10 || H.
SHA-1:   (0x)30 21 30 09 06 05 2b 0e 03 02 1a 05 00 04 14 || H.
SHA-256: (0x)30 31 30 0d 06 09 60 86 48 01 65 03 04 02 01 05 00 04 20 || H.
SHA-384: (0x)30 41 30 0d 06 09 60 86 48 01 65 03 04 02 02 05 00 04 30 || H.
SHA-512: (0x)30 51 30 0d 06 09 60 86 48 01 65 03 04 02 03 05 00 04 40 || H.
*/