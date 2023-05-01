
#pragma once

//---------------------------------------------------------------------------
// OpenSSL key sizes:
//    rc4                128 bit RC4
//    rc4-64             64  bit RC4
//    rc4-40             40  bit RC4
//
/*static void EncDecMsgRC4(unsigned char* Msg, unsigned int MsgLen, unsigned char* Key, unsigned int KeyLen)      // RC4
{
 static const unsigned int MAXIDX = 256;   // Max RC4 key length
 int IndexArr[MAXIDX];

 for(unsigned int ctr=0;ctr < MAXIDX;ctr++)IndexArr[ctr] = ctr;  // Init RC4 SBOX
 for(unsigned int vKBIdx=0,vBIdx=0,vIndexA=0;vIndexA < MAXIDX;vKBIdx++,vIndexA++)  // Reorganize array
  {
   if(vKBIdx >= KeyLen)vKBIdx = 0;
   vBIdx = (unsigned char)(IndexArr[vIndexA] + vBIdx + Key[vKBIdx]); // % KeyLen //  & (MAXIDX-1);
   unsigned char val = IndexArr[vIndexA];
   IndexArr[vIndexA] = IndexArr[vBIdx];       // SWAP
   IndexArr[vBIdx] = val;
  }
 for(unsigned int vCIdxA=0,vCIdxB=0,vIndexA=0;vIndexA < MsgLen;vIndexA++)
  {
   vCIdxA = (unsigned char)(vCIdxA + 1);  // & (MAXIDX-1);
   vCIdxB = (unsigned char)(vCIdxB + IndexArr[vCIdxA]);   // & (MAXIDX-1);
   unsigned char val = IndexArr[vCIdxA];
   IndexArr[vCIdxA] = IndexArr[vCIdxB];
   IndexArr[vCIdxB] = val;
   Msg[vIndexA] ^= (IndexArr[(unsigned char)(IndexArr[vCIdxB] + IndexArr[vCIdxA])]);   //  & (MAXIDX-1)
  }
} */
/*static void EncDecMsgRC4(unsigned char* Msg, unsigned int MsgLen, unsigned char* Key, unsigned int KeyLen)      // RC4
{
 static const unsigned int MAXIDX = 256;   // Max RC4 key length
 int IndexArr[MAXIDX];

 for(unsigned int ctr=0;ctr < MAXIDX;ctr++)IndexArr[ctr] = ctr;  // Init RC4 SBOX
 for(unsigned int KBIdx=0,BIdx=0,IndexA=0;IndexA < MAXIDX;KBIdx++,IndexA++)  // Reorganize array
  {
   if(KBIdx >= KeyLen)KBIdx = 0;
   BIdx = (unsigned char)(IndexArr[IndexA] + BIdx + Key[KBIdx]); // % KeyLen //  & (MAXIDX-1);
   unsigned char val = IndexArr[IndexA];
   IndexArr[IndexA] = IndexArr[BIdx];       // SWAP
   IndexArr[BIdx] = val;
  }
 for(unsigned int CIdxA=0,CIdxB=0,IndexA=0;IndexA < MsgLen;IndexA++)
  {
   CIdxA = (unsigned char)(CIdxA + 1);  // & (MAXIDX-1);
   CIdxB = (unsigned char)(CIdxB + IndexArr[CIdxA]);   // & (MAXIDX-1);
   unsigned char val = IndexArr[CIdxA];
   IndexArr[CIdxA] = IndexArr[CIdxB];
   IndexArr[CIdxB] = val;
   Msg[IndexA] ^= (IndexArr[(unsigned char)(IndexArr[CIdxB] + IndexArr[CIdxA])]);   //  & (MAXIDX-1)
  }
} */

static void EncDecMsgRC4(unsigned char* Msg, unsigned int MsgLen, unsigned char* Key, unsigned int KeyLen)   // RC4
{
 const unsigned int MAXIDX = 256;   // Max RC4 key length
 int IndexArr[MAXIDX];
 
 for(unsigned int ctr=0;ctr < MAXIDX;ctr++)IndexArr[ctr] = ctr;  // Init RC4 SBOX
 for(unsigned int KBIdx=0,BIdx=0,IndexA=0;IndexA < MAXIDX;KBIdx++,IndexA++)  // Reorganize array
  {
   if(KBIdx >= KeyLen)KBIdx = 0;
   int valb = IndexArr[IndexA];
   BIdx = (unsigned char)(valb + BIdx + Key[KBIdx]); // % KeyLen //  & (MAXIDX-1);
   IndexArr[IndexA] = IndexArr[BIdx];       // SWAP
   IndexArr[BIdx]   = valb;
  }
 for(unsigned int CIdxA=0,CIdxB=0,IndexA=0;IndexA < MsgLen;IndexA++)
  {
   CIdxA = (unsigned char)(++CIdxA);  // & (MAXIDX-1);
   int valb = IndexArr[CIdxA];
   CIdxB = (unsigned char)(CIdxB + valb);   // & (MAXIDX-1);
   int vala = IndexArr[CIdxB];
   IndexArr[CIdxA] = vala;
   IndexArr[CIdxB] = valb;
   Msg[IndexA] ^= (IndexArr[(unsigned char)(vala + valb)]);   //  & (MAXIDX-1) 
  }
}
//---------------------------------------------------------------------------



