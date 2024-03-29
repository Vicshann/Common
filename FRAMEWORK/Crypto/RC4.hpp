
#pragma once

//---------------------------------------------------------------------------
// OpenSSL key sizes:
//    rc4                128 bit RC4
//    rc4-64             64  bit RC4
//    rc4-40             40  bit RC4
//
static void EncDecMsgRC4(unsigned char* Msg, unsigned int MsgLen, unsigned char* Key, unsigned int KeyLen)  //__attribute__ ((optnone))  // RC4   // Can be constexpr?
{
 const unsigned int MAXIDX = 256;   // Max RC4 key length
 uint32 IndexArr[MAXIDX];    // TODO: Try uint8

 for(unsigned int ctr=0;ctr < MAXIDX;ctr++)IndexArr[ctr] = (uint32)ctr;  // Init RC4 SBOX
 for(unsigned int KBIdx=0,BIdx=0,IndexA=0;IndexA < MAXIDX;KBIdx++,IndexA++)  // Reorganize array
  {
   if(KBIdx >= KeyLen)KBIdx = 0;
   uint32 valb = IndexArr[IndexA];
   BIdx = (unsigned char)(valb + BIdx + Key[KBIdx]); // % KeyLen //  & (MAXIDX-1);
   IndexArr[IndexA] = IndexArr[BIdx];       // SWAP
   IndexArr[BIdx]   = valb;
  }
 for(unsigned int CIdxA=0,CIdxB=0,IndexA=0;IndexA < MsgLen;IndexA++)
  {
   CIdxA = (unsigned char)(++CIdxA);  // & (MAXIDX-1);
   uint32 valb = IndexArr[CIdxA];
   CIdxB = (unsigned char)(CIdxB + valb);   // & (MAXIDX-1);
   uint32 vala = IndexArr[CIdxB];
   IndexArr[CIdxA] = vala;
   IndexArr[CIdxB] = valb;
   Msg[IndexA] ^= (IndexArr[(unsigned char)(vala + valb)]);   //  & (MAXIDX-1)
  }
}
//---------------------------------------------------------------------------



