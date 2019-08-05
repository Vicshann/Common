#pragma once

#include <windows.h>

#pragma warning(disable:4554)           //  warning C4554: '>>': check operator precedence for possible error; use parentheses to clarify precedence

namespace NPAQ8     // paq8o10t
{
#include "PaqUtils.hpp"
#include "PaqFilters.hpp"
#include "PaqMixer.hpp"
#include "PaqCtxMap.hpp"
#include "PaqModels.hpp"
#include "PaqEncoder.hpp"   

//==================================================================================
// Compress a file
// BUG: At level 8 will crash compressing str of size 33 bytes
static inline int strm_compress(STRM& SrcStrm, Encoder* en, unsigned long DataSize=0) 
{
 if(en->getMode() != COMPRESS)return -1;
 long start=en->size();

 // Transform and test in blocks
 const int BLOCK=en->getMem()*64;   // MEM*64  
 if(!DataSize)      // Rest of source stream
  {
   unsigned long savepos = SrcStrm.ftell();
   SrcStrm.fseek(0,SEEK_END);
   DataSize = SrcStrm.ftell();    // 4GB only!
   SrcStrm.fseek(savepos,SEEK_SET);
  }
 for(unsigned long i=0; DataSize > 0; i+=BLOCK) 
  {
   unsigned long size=BLOCK;
   if(size > DataSize) size=DataSize;
   MSTRM tmp;                 // Base stream is memory stream
   unsigned long savepos = SrcStrm.ftell();
   encode(&SrcStrm, &tmp, size);

    // Test transform
   tmp.fseek(0,SEEK_SET);    //rewind(tmp);
   en->setFile(&tmp);
   SrcStrm.fseek(savepos, SEEK_SET);
   long j;
   int c1=0, c2=0;
   for (j=0; j<size; ++j)
    {
      if ((c1=decode(*en))!=(c2=SrcStrm.getc())) break;
    }
    // Test fails, compress without transform
    if (j!=size || tmp.getc()!=STRM::PEOF) 
    {
//      PRINTF("Transform fails at %ld, input=%d decoded=%d, skipping...\n", i+j, c2, c1);
      en->compress(0);              // Block type
      en->compress(size>>24);       // Block size
      en->compress(size>>16);
      en->compress(size>>8);
      en->compress(size);
      SrcStrm.fseek(savepos, SEEK_SET);
      for (int j=0; j<size; ++j) {
//        printStatus(i+j);
        en->compress(SrcStrm.getc());
      }
    }
    
    else {      // Test succeeds, decode(encode(f)) == f, compress tmp
      tmp.fseek(0,SEEK_SET);  //rewind(tmp);
      int c;
      j=0;
      while ((c=tmp.getc())!=STRM::PEOF) {
//        printStatus(i+j++);
        en->compress(c);
      }
    }
    DataSize-=size;
 }
 en->flush();
 return 0;
}
//----------------------------------------------------------------------------------
// Decompress a file
static inline int strm_decompress(STRM& DstStrm, Encoder* en, unsigned long DataSize=0) 
{
 if(en->getMode() != DECOMPRESS)return -1;
 //     PRINTF("Extracting %s %ld -> ", filename, filesize);
 en->ResetDecoderCtx();
 DstStrm.putc(decode(*en));
 if(!DataSize)DataSize = en->GetCurrDecBlkLen();  // Current block only  
 for (unsigned long i=1; i < DataSize; ++i) 
  {
//        printStatus(i);
        DstStrm.putc(decode(*en));
  }
//      PRINTF("done        \n");
 return 0; 
}
//----------------------------------------------------------------------------------
static inline int strm_compress(int Lvl, STRM& SrcStrm, STRM& DstStrm)     // Single file
{
 if(Lvl < 0 || Lvl > 9)return -1;
 Encoder* en = new Encoder(Lvl, COMPRESS, &DstStrm);   
 int res = strm_compress(SrcStrm, en, 0);
 delete(en);
 return res;
}
//----------------------------------------------------------------------------------
static inline int strm_decompress(int Lvl, unsigned long DataSize, STRM& SrcStrm, STRM& DstStrm)           // Single file
{
 if(Lvl < 0 || Lvl > 9)return -1;
 Encoder* en = new Encoder(Lvl, DECOMPRESS, &SrcStrm);   
 int res = strm_decompress(DstStrm, en, DataSize);
 delete(en);
 return res;
}
//----------------------------------------------------------------------------------


}