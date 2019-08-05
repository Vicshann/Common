
/////////////////////////// Filters /////////////////////////////////
//
// Before compression, data is encoded in blocks with the following format:
//
//   <type> <size> <encoded-data>
//
// Type is 1 byte (type Filetype): DEFAULT=0, JPEG, EXE
// Size is 4 bytes in big-endian format.
// Encoded-data decodes to <size> bytes.  The encoded size might be
// different.  Encoded data is designed to be more compressible.
//
//   void encode(FILE* in, FILE* out, int n);
//
// Reads n bytes of in (open in "rb" mode) and encodes one or
// more blocks to temporary file out (open in "wb+" mode).
// The file pointer of in is advanced n bytes.  The file pointer of
// out is positioned after the last byte written.
//
//   en.setFile(FILE* out);
//   int decode(Encoder& en);
//
// Decodes and returns one byte.  Input is from en.decompress(), which
// reads from out if in COMPRESS mode.  During compression, n calls
// to decode() must exactly match n bytes of in, or else it is compressed
// as type 0 without encoding.
//
//   Filetype detect(FILE* in, int n, Filetype type);
//
// Reads n bytes of in, and detects when the type changes to
// something else.  If it does, then the file pointer is repositioned
// to the start of the change and the new type is returned.  If the type
// does not change, then it repositions the file pointer n bytes ahead
// and returns the old type.
//
// For each type X there are the following 2 functions:
//
//   void encode_X(FILE* in, FILE* out, int n, ...);
//
// encodes n bytes from in to out.
//
//   int decode_X(Encoder& en);
//
// decodes one byte from en and returns it.  decode() and decode_X()
// maintain state information using static variables.
#define bswap(x)	\
+   ((((x) & 0xff000000) >> 24) | \
+    (((x) & 0x00ff0000) >>  8) | \
+    (((x) & 0x0000ff00) <<  8) | \
+    (((x) & 0x000000ff) << 24))
/*U32 GetCDWord(FILE *f)
{
    U16 w = getc(f);
	w=w | (getc(f)<<8);
	if(w&1){
		U16 w1 = getc(f);
	    w1=w1 | (getc(f)<<8);
		return ((w1<<16)|w)>>1;
	}
    return w>>1;
}
U8 GetCWord(FILE *f)
{
    U8 b=getc(f);
    if(b&1) return ((getc(f)<<8)|b)>>1;
    return b>>1;
}
*/
// Detect EXE or JPEG data
static EFiletype detect(STRM* in, int n, EFiletype type) 
{
  U32 buf1=0, buf0=0;  // last 8 bytes
  unsigned long start=in->ftell();

  // For EXE detection
  Array<int> abspos(256),  // CALL/JMP abs. addr. low byte -> last offset
    relpos(256);    // CALL/JMP relative addr. low byte -> last offset
  int e8e9count=0;  // number of consecutive CALL/JMPs
  int e8e9pos=0;    // offset of first CALL or JMP instruction
  int e8e9last=0;   // offset of most recent CALL or JMP
  // For BMP detection
  int bmp=0,bmp2=0,bmp0=0,bsize=0,imgbpp=0,bmpx=0,bmpy=0,bmpimgoff=0,imgcomp=-1;
  // For PGM detection
  int pgm=0,psize=0,pgmcomment=0,pgmw=0,pgmh=0,pgmsize=0,pgm_ptr=0,pgmc=0;
  char pgm_buf[32];
  // For JPEG detection
  int soi=0, sof=0, sos=0, app=0;  // position where found
  // For .RGB detection
  int rgbi=0,rgbSTORAGE=-1,rgbBPC=0,rgbDIMENSION=0,rgbZSIZE=0,rgbXSIZE=0,rgbYSIZE=0,rgbsize=0;
  // For TEXT
  int txtStart=0,txtLen=0,txtOff=0,txtPrev2=-1,txtPrev3=-1,txtIsUTF8=-1,txtUTF8b2=0,txtUTF8b3=0;
  const int txtMinLen=80; // minimal text to be detected
  // For WAVE
  long  wavchunkSize=0;
  //short wavwFormatTag=0;
  //U16 wavwChannels=0,wavwBlockAlign=0,wavwBitsPerSample=0;
  //U32 wavdwSamplesPerSec=0,wavdwAvgBytesPerSec=0;
  int wavi=0,wavsize=0,wavdatas=0;
  // For MRB detection
  int mrb=0,mrb0=0,mrbsize=0,mrbPictureType=0,mrbPackingMethod=0,mrbTell=0;
  
  for (int i=0; i<n; ++i) {
    int c=in->getc();
    if (c==STRM::PEOF) return (EFiletype)(-1);
    buf1=buf1<<8|buf0>>24;
    buf0=buf0<<8|c;

    // Detect JPEG by code SOI APPx (FF D8 FF Ex) followed by
    // SOF0 (FF C0 xx xx 08) and SOS (FF DA) within a reasonable distance.
    // Detect end by any code other than RST0-RST7 (FF D9-D7) or
    // a byte stuff (FF 00).

    if (!soi && i>=3 && (buf0&0xfffffff0)==0xffd8ffe0) soi=i, app=i+2;
    if (soi) {
        if (app==i && ((buf0&0xfff00000)==0xffe00000 || (buf0&0xffff0000)==0xfffe0000))
          app=i+(buf0&0xffff)+2;    
        if (app<i && i-soi<0x10000 && (buf1&0xff)==0xff
            && (buf0&0xff0000ff)==0xc0000008)
          sof=i;
        if (sof && sof>soi && i-soi<0x10000 && i-sof<0x1000
            && (buf0&0xffff)==0xffda) {
          sos=i;
          if (type!=JPEG) return in->fseek(start+soi-3, SEEK_SET), JPEG;
        }
    }
    if (type==JPEG && sos && i>sos && (buf0&0xff00)==0xff00
        && (buf0&0xff)!=0 && (buf0&0xf8)!=0xd0)
      return DEFAULT;

	// Detect .bmp image
    
    if ((buf0&0xFFFF)==16973) bmp=i;                //possible 'BM'
    if (bmp){
		if ((i-bmp)==4) bsize=bswap(buf0);          //image size
		if ((i-bmp)==12) bmpimgoff=bswap(buf0);
        if ((i-bmp)==16 && buf0!=0x28000000) bmp=imgbpp=bsize=0,imgcomp=-1; //if windows bmp
		if ((i-bmp)==20){
			bmpx=bswap(buf0);                       //image x size
			if (bmpx==0) bmp=imgbpp=bsize=0,imgcomp=-1; // Test big size?
		}
		if ((i-bmp)==24){
			bmpy=bswap(buf0);                       //image y size
			if (bmpy==0) bmp=imgbpp=bsize=0,imgcomp=-1;
		}	
		if ((i-bmp)==27) imgbpp=c;                  //image bpp
		if ((i-bmp)==31){
                         imgcomp=buf0;              //image compression 0=none, 1=RLE-8, 2=RLE-4		
                         if (imgcomp!=0) bmp=imgbpp=bsize=0,imgcomp=-1;}
		if ((type==BMPFILE1 || type==BMPFILE4 || type==BMPFILE8 || type==BMPFILE24 ) && (imgbpp==1 || imgbpp==4 || imgbpp==8 || imgbpp==24) && imgcomp==0){
            int tbsize=0;
            if (imgbpp==1)
                if (bsize !=(tbsize=(((((bmpx-1)>>5)+1)*4*bmpy)+bmpimgoff))) bsize=tbsize;
            if (imgbpp==4)
                if (bsize !=(tbsize=(((((bmpx-1)>>3)+1)*4*bmpy)+bmpimgoff))) bsize=tbsize;
            if (imgbpp==8)
                if (bsize !=(tbsize=(bmpx*bmpy+bmpimgoff))) bsize=tbsize;
            if (imgbpp==24)
                if (bsize !=(tbsize=(bmpx*bmpy*3+bmpimgoff))) bsize=tbsize;
			return in->fseek(start+bsize, SEEK_SET),DEFAULT;
		}
		if (imgbpp==1 && imgcomp==0){
			return 	in->fseek(start+bmp-1, SEEK_SET),BMPFILE1;
		}
		if (imgbpp==4 && imgcomp==0){
			return 	in->fseek(start+bmp-1, SEEK_SET),BMPFILE4;
		}
		if (imgbpp==8 && imgcomp==0){
			return 	in->fseek(start+bmp-1, SEEK_SET),BMPFILE8;
		}
		if (imgbpp==24 && imgcomp==0){
			return 	in->fseek(start+bmp-1, SEEK_SET),BMPFILE24;
		}
    }
  /*   // Detect .bmp image without 'BM' 
    if (buf0==0x28000000 && !bmp) bmp2=i;           //if windows bmp
    if (bmp2){
		if ((i-bmp2)==20-16){
			bmpx=bswap(buf0);                       //image x size
			if (bmpx==0) bmp2=imgbpp=bsize=0,imgcomp=-1; // Test big size?
		}
		if ((i-bmp2)==24-16){
			bmpy=bswap(buf0);                       //image y size
			if (bmpy==0) bmp2=imgbpp=bsize=0,imgcomp=-1;
		}	
		if ((i-bmp2)==27-16) imgbpp=c;                  //image bpp
		if ((i-bmp2)==31-16){
                         imgcomp=buf0;  //image compression 0=none
                                        //                  1=RLE-8
                                        //                  2=RLE-4		
                         if (imgcomp!=0) bmp2=imgbpp=bsize=0,imgcomp=-1;
                         // ignore x bit image if witdh or heigh is smaller then 128 (.ico)
                         if (/*imgbpp==8 &&*/ /*(bmpy<128 || bmpx<128)) bmp2=imgbpp=bsize=0,imgcomp=-1;
                         }
		if ((type==BMPFILE1 || type==BMPFILE8)  &&  (imgbpp==1 || imgbpp==8)  && imgcomp==0){
            if (imgbpp==1) bsize =((((bmpx-1)>>5)+1)*4*bmpy)+40+2;
            if (imgbpp==8) bsize =bmpx*bmpy+40+1024;
			return fseek(in, start+bsize, SEEK_SET),DEFAULT;
		}
		if (imgbpp==1 && imgcomp==0){
			return 	fseek(in, start+bmp2-3, SEEK_SET),BMPFILE1;
		}
		if (imgbpp==8 && imgcomp==0){
    		return 	fseek(in, start+bmp2-3, SEEK_SET),BMPFILE8;
		}
    }*/

    // Detect .pgm image
    if ((buf0&0xFFFFFF)==0x50350A) pgm=i;           //possible 'P5 '
    if (pgm){
		if ((i-pgm)==1 && c==0x23) pgmcomment=1; //pgm comment
		//not tested without comment
		if (!pgmcomment && c==0x20 && !pgmw && pgm_ptr) {
			pgm_buf[pgm_ptr++]=0;
			pgmw=atoi(pgm_buf);
			if (pgmw==0) pgm=pgm_ptr=pgmw=pgmh=pgmc=pgmcomment=0;			
			pgm_ptr=0;
		}
		if (!pgmcomment && c==0x0a && !pgmh && pgm_ptr){
			pgm_buf[pgm_ptr++]=0;
			pgmh=atoi(pgm_buf);
			if (pgmh==0) pgm=pgm_ptr=pgmw=pgmh=pgmc=pgmcomment=0;
			pgm_ptr=0;
		}
		if (!pgmcomment && c==0x0a && !pgmc && pgm_ptr){
			pgm_buf[pgm_ptr++]=0;
			pgmc=atoi(pgm_buf);
			pgm_ptr=0;
		}
		if (!pgmcomment) pgm_buf[pgm_ptr++]=c;
		if (pgm_ptr>=32) pgm=pgm_ptr=pgmw=pgmh=pgmc=pgmcomment=0;
		if (pgmcomment && c==0x0a) pgmcomment=0;
		if (type==PGMFILE && pgmw && pgmh && pgmc){
			pgmsize=pgmw *pgmh +pgm+i-1;
			return in->fseek(start+pgmsize, SEEK_SET),DEFAULT;
		}
     	if (pgmw && pgmh && pgmc){
		     return in->fseek(start+pgm-2, SEEK_SET),PGMFILE;
        }
    }
    // Detect .rgb image
	if ((buf0&0xFFFF)==0x01DA) rgbi=i;
    if (rgbi){
        if ((i-rgbi)==1)
		    if (c==0 || c==1)
			    rgbSTORAGE=c; //0 uncompressed, 1 RLE compressed
            else
			    rgbi=rgbBPC=rgbDIMENSION=rgbZSIZE=rgbXSIZE=rgbYSIZE=0,rgbSTORAGE=-1;
	    if ((i-rgbi)==2)
		    if  (c==1 || c==2) 
    			rgbBPC=c;
       		else
      			rgbi=rgbBPC=rgbDIMENSION=rgbZSIZE=rgbXSIZE=rgbYSIZE=0,rgbSTORAGE=-1;
        if ((i-rgbi)==4) 
        	if ((buf0&0xFFFF)==1 || (buf0&0xFFFF)==2 || (buf0&0xFFFF)==3) 
		    	rgbDIMENSION=buf0&0xFFFF;
		    else
			    rgbi=rgbBPC=rgbDIMENSION=rgbZSIZE=rgbXSIZE=rgbYSIZE=0,rgbSTORAGE=-1;
    	if ((i-rgbi)==6) 
	    	if ((buf0&0xFFFF)>0) 
		    	rgbXSIZE=buf0&0xFFFF;
		    else
			    rgbi=rgbBPC=rgbDIMENSION=rgbZSIZE=rgbXSIZE=rgbYSIZE=0,rgbSTORAGE=-1;
    	if ((i-rgbi)==8) 
		if ((buf0&0xFFFF)>0) 
			rgbYSIZE=buf0&0xFFFF,rgbsize=rgbYSIZE*rgbXSIZE+512;
		else
			rgbi=rgbBPC=rgbDIMENSION=rgbZSIZE=rgbXSIZE=rgbYSIZE=0,rgbSTORAGE=-1;
    	if ((i-rgbi)==10) 
	    	if ((buf0&0xFFFF)==1 || (buf0&0xFFFF)==3 || (buf0&0xFFFF)==4)  // 1 indicates greyscale
		    															   // 3 indicates RGB
			    														   // 4 indicates RGB and Alpha
    			rgbZSIZE=buf0&0xFFFF;
	    	else
		    	rgbi=rgbBPC=rgbDIMENSION=rgbZSIZE=rgbXSIZE=rgbYSIZE=0,rgbSTORAGE=-1;
		if (rgbsize != 0  && (i-rgbi)>0 && ((i-rgbi)>rgbsize)){
			if (type==RGBFILE  && rgbZSIZE==1 && rgbSTORAGE==0 ){ //uncompressed greyscale
				return in->fseek(start+rgbsize, SEEK_SET),DEFAULT;
			}
			if (rgbZSIZE==1 && rgbSTORAGE==0){
				return 	in->fseek(start+rgbi-1, SEEK_SET),RGBFILE;
			}
		}
    }
    /*
    //
    // detect .MRB file (inside .hlp files)
	//
	if ((buf0&0xFFFF)==0x6c70 || (buf0&0xFFFF)==0x6C50) mrb=i; 
	if (mrb) {
    if ((i-mrb)==1 && !c==1)	mrb=0; //if not 1 image per/file
	//                     5=DDB   6=DIB   8=metafile
	if ( (i-mrb)==7 && (c==5 || c==6 || c==8)) 
		mrbPictureType=c;
	//                     0=uncomp 1=RunLen 2=LZ77 3=both
	if ( (i-mrb)==8 && (c==0 || c==1 || c==2 || c==3)) 
		mrbPackingMethod=c;
	if ( (i-mrb)==10){
		if (mrbPictureType==6 && (mrbPackingMethod==1 || mrbPackingMethod==2))
	{
		//save ftell
		mrbTell=ftell(in)-2;
		fseek(in,mrbTell,SEEK_SET);
		unsigned long Xdpi=GetCDWord(in);
		unsigned long Ydpi=GetCDWord(in);
        unsigned long Planes=GetCWord(in);
		unsigned long BitCount=GetCWord(in);
        unsigned long Width=GetCDWord(in);
		unsigned long Height=GetCDWord(in);
        unsigned long ColorsUsed=GetCDWord(in);
		unsigned long ColorsImportant=GetCDWord(in);
        unsigned long CompressedSize=GetCDWord(in);
		unsigned long HotspotSize=GetCDWord(in);
		int CompressedOffset=getc(in)<<24;
            CompressedOffset|=getc(in)<<16;
            CompressedOffset|=getc(in)<<8;
            CompressedOffset|=getc(in);
	    int HotspotOffset=getc(in)<<24;
            HotspotOffset|=getc(in)<<16;
            HotspotOffset|=getc(in)<<8;
            HotspotOffset|=getc(in);
	    CompressedOffset=bswap(CompressedOffset);
		HotspotOffset=bswap(HotspotOffset);
		mrbsize=CompressedSize+ftell(in)-mrbTell+10+(1<<BitCount)*4; // ignore HotspotSize
		if (BitCount!=8) {
			mrbPictureType=mrb=mrbsize=0;
		    mrbTell=mrbTell+2;
		    fseek(in,mrbTell,SEEK_SET);
		}
	}
		else if (mrbPictureType==8 && (mrbPackingMethod==1 || mrbPackingMethod==2))
		{
			mrbTell=ftell(in)-2;
			fseek(in,mrbTell,SEEK_SET);
			U16 MappingMode=GetCWord(in);
			U16 Width=getc(in) | getc(in)<<8;
			U16 Height=getc(in) | getc(in)<<8;
			U32 DecompressedSize=GetCDWord(in); //can be used to allocate buffer
			U32 CompressedSize=GetCDWord(in);
			U32 HotspotSize=GetCDWord(in);//	 0 if none are defined
			int CompressedOffset=getc(in)<<24;
		        CompressedOffset|=getc(in)<<16;
				CompressedOffset|=getc(in)<<8;
				CompressedOffset|=getc(in);
			int HotspotOffset=getc(in)<<24;
		        HotspotOffset|=getc(in)<<16;
				HotspotOffset|=getc(in)<<8;
				HotspotOffset|=getc(in);
			CompressedOffset=bswap(CompressedOffset);
			HotspotOffset=bswap(HotspotOffset);
			mrbsize=CompressedSize+ftell(in)-mrbTell+10; // ignore HotspotSize
		}
	}
	if (type==MRBFILE   &&  (mrbPictureType==6 || mrbPictureType==8) && mrbsize){
		return fseek(in, start+mrbsize, SEEK_SET),DEFAULT;
	}
	if ((mrbPictureType==6) && mrbsize){
		return 	fseek(in, start+mrb-1, SEEK_SET),MRBFILE;
	}
}*/
    //TIFF support needed
    // Detect .tiff file
    
    
    // Detect .wav file
	if (buf0==0x52494646 ) wavi=i; //'RIFF'
    if (wavi){
        if ((i-wavi)==4){ 
           wavsize=bswap(buf0);
           if (wavsize==0) wavi=wavdatas=wavsize=0;
        }
	    if ((i-wavi)==8  && (!buf0=='WAVE')) wavi=wavdatas=wavsize=0; //'WAVE'
        if ((i-wavi)==12 && (!buf0=='fmt ')) wavi=wavdatas=wavsize=0; //'fmt '
		if ((i-wavi)==16) {
	    	wavchunkSize=bswap(buf0);
			if (wavchunkSize==0) wavi=wavdatas=wavsize=0;}
    	if ((i-wavi)==18 && ((buf0&0xFFFF)!=0x100))  wavi=wavdatas=wavsize=0; // 1=if no comression
    	if ((i-wavi)==36 && !buf0=='data') wavi=wavdatas=wavsize=0;    //'data' 
		if ((i-wavi)==40) {
	    	wavdatas=bswap(buf0);
			if (wavdatas==0 || (wavsize<(wavdatas+0x24))) wavi=wavdatas=wavsize=0;}
        if (!wavsize == 0 && !wavdatas==0){
			if (type==WAVFILE) return in->fseek(start+wavsize, SEEK_SET),DEFAULT;
			return 	in->fseek(start+wavi-3, SEEK_SET),WAVFILE;
		}
    }
    // Detect EXE if the low order byte (little-endian) XX is more
    // recently seen (and within 4K) if a relative to absolute address
    // conversion is done in the context CALL/JMP (E8/E9) XX xx xx 00/FF
    // 4 times in a row.  Detect end of EXE at the last
    // place this happens when it does not happen for 64KB.

    if ((buf1&0xfe)==0xe8 && (buf0+1&0xfe)==0) {
      int r=buf0>>24;  // relative address low 8 bits
      int a=(buf0>>24)+i&0xff;  // absolute address low 8 bits
      int rdist=i-relpos[r];
      int adist=i-abspos[a];
      if (adist<rdist && adist<0x1000 && abspos[a]>5) {
        e8e9last=i;
        ++e8e9count;
        if (e8e9pos==0 || e8e9pos>abspos[a]) e8e9pos=abspos[a];
      }
      else e8e9count=0;
      if (type!=EXE && e8e9count>=4 && e8e9pos>5)
        return in->fseek(start+e8e9pos-5, SEEK_SET), EXE;
      abspos[a]=i;
      relpos[r]=i;
    }
    if (type==EXE && i-e8e9last>0x1000)
      return in->fseek(start+e8e9last, SEEK_SET), DEFAULT;
     

    // Detect text
    // do not detect if some other detection is in progress
    if (txtStart==0 && !soi && !pgm && !rgbi && !bmp && !bmp2 && !wavi && !mrb &&
     ((c<128 && c>32) || c==10 || c==13 || c==0x12 || c==9 )) txtStart=1,txtOff=i;
    if (txtStart) {
       if ((c<128 && c>=32) || c==10 || c==13 || c==0x12 || c==9) {
            ++txtLen;
       }
       else if (c>=0xc2 && c<=0xdf && txtPrev2==-1){ //if possible UTF8 2 byte
            txtPrev2=c;
             ++txtLen;
             txtUTF8b2=1;
            }
       else if (c>=0xc2 && c<=0xdf && txtUTF8b2==2){
            txtPrev2=c; //c2-df
             ++txtLen;
             txtUTF8b2=1;
            }
       else if (txtPrev2>=0xc2 && txtPrev2<=0xdf && c>=0x80 && c<=0xbf && txtUTF8b2==1){
            txtPrev2=c; //80-bf
             ++txtLen;
             txtUTF8b2=2;
             txtIsUTF8=1;
            }
       else if (c>=0xe0 && c<=0xef && txtPrev3==-1){ //if possible UTF8 3 byte
            txtPrev3=c;
             ++txtLen;
             txtUTF8b3=1;
            }
       else if (c>=0xe0 && c<=0xef && txtUTF8b3==3){
            txtPrev3=c; //c2-df
             ++txtLen;
             txtUTF8b3=1;
            }
       else if (txtPrev3>=0xe0 && txtPrev3<=0xef && c>=0x80 && c<=0xbf && txtUTF8b3==1){
            txtPrev3=c; //80-bf
             ++txtLen;
             txtUTF8b3=2;
            }
       else if (txtPrev3>=0x80 && txtPrev3<=0xbf && c>=0x80 && c<=0xbf && txtUTF8b3==2){
            txtPrev3=c; //80-bf
             ++txtLen;
             txtUTF8b3=3;
             txtIsUTF8=1;
            }
       else if (txtLen<txtMinLen) {
            txtStart=txtLen=txtOff=txtUTF8b2=txtUTF8b3=0,txtPrev2=txtPrev3=txtIsUTF8=-1;
       }
       else if (txtLen>=txtMinLen) {
           if (txtUTF8b2==1) --txtLen;
           if (txtUTF8b3==1) --txtLen,--txtLen;
           if (txtUTF8b3==2) --txtLen;
		   if (type==TEXT ||type== TXTUTF8 ) return in->fseek(start+txtLen, SEEK_SET),DEFAULT;

           if (txtIsUTF8==1)
            return in->fseek(start+txtOff, SEEK_SET),TXTUTF8;
           else
            return in->fseek(start+txtOff, SEEK_SET),TEXT;
       }
    } 
  }
  if (txtStart) {
      if (txtLen<txtMinLen) {
            txtStart=txtLen=txtOff=0;
       }
       else if (txtLen>=txtMinLen) {
		   if (type==TEXT ||type== TXTUTF8 ) return in->fseek(start+txtLen, SEEK_SET),DEFAULT;

           if (txtIsUTF8==1)
            return in->fseek(start+txtOff, SEEK_SET),TXTUTF8;
           else
            return in->fseek(start+txtOff, SEEK_SET),TEXT;
       }
    }

  return type;
}
