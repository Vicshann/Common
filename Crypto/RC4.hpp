
#pragma once

static void EncDecMsgRC4(BYTE* Msg, UINT MsgLen, BYTE* Key, UINT KeyLen)      // RC4
{
static const unsigned int MAXIDX = 256;
	
 int IndexArr[MAXIDX];

// if(Key.Length() > MAXIDX)Key.SetLength(MAXIDX);        // Max RC4 key length
 for(int ctr=0;ctr < MAXIDX;ctr++)IndexArr[ctr] = ctr;  // Init RC4 SBOX
 for(int vBIdx=0,vIndexA=0;vIndexA < MAXIDX;vIndexA++)  // Reorganize array
  {
   vBIdx = (IndexArr[vIndexA] + vBIdx + Key[vIndexA % KeyLen]) % MAXIDX;
   BYTE v15 = IndexArr[vIndexA];
   IndexArr[vIndexA] = IndexArr[vBIdx];       // SWAP
   IndexArr[vBIdx] = v15;
  }
 for(int vCIdxA=0,vCIdxB=0,vIndexA=0;vIndexA < MsgLen;vIndexA++)
  {
   vCIdxA = (vCIdxA + 1) % MAXIDX;
   vCIdxB = (vCIdxB + IndexArr[vCIdxA]) % MAXIDX;
   BYTE v24 = IndexArr[vCIdxA];
   IndexArr[vCIdxA] = IndexArr[vCIdxB];
   IndexArr[vCIdxB] = v24;
   Msg[vIndexA] ^= (IndexArr[(IndexArr[vCIdxB] + IndexArr[vCIdxA]) % MAXIDX]);
  }
}
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------

