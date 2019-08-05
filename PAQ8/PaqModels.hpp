
//////////////////////////// Models //////////////////////////////

// All of the models below take a Mixer as a parameter and write predictions to it.

//////////////////////////// matchModel ///////////////////////////

// matchModel() finds the longest matching context and returns its length

struct SCtx_matchModel
{
// Context (Must come first)
 int& c0;
 int& bpos;
 Buf& buf;
 Mixer& m; 
//------------
 Array<int> t;  // hash table of pointers to contexts
 int h;  // hash of last 7 bytes
 int ptr;  // points to next byte of match if any
 int len;  // length of match, or 0 if no match
 int result; 
 
 SmallStationaryContextMap scm1;

 SCtx_matchModel(Mixer& _m): m(_m), t(_m.MEM), scm1(0x20000), h(0), ptr(0), len(0), result(0), c0(_m.c0), bpos(_m.bpos), buf(_m.buf)  {}

int matchModel(void) 
{
  const int MAXLEN=65534;  // longest allowed match + 1

  if (!bpos) {
    h=h*997*8+buf(1)+1&t.size()-1;  // update context hash
    if (len) ++len, ++ptr;
    else {  // find match
      ptr=t[h];
      if (ptr && buf.pos-ptr<buf.size())
        while (buf(len+1)==buf[ptr-len-1] && len<MAXLEN) ++len;
    }
    t[h]=buf.pos;  // update hash table
    result=len;
//    if (result>0 && !(result&0xfff)) PRINTF("pos=%d len=%d ptr=%d\n", pos, len, ptr);
    scm1.set(buf.pos);
  }

  // predict
  if (len)
  {
   if (buf(1)==buf[ptr-1] && c0==buf[ptr]+256>>8-bpos)
   {
    if (len>MAXLEN) len=MAXLEN;
    if (buf[ptr]>>7-bpos&1)
    {
     m.add(CLog::ilog(len)<<2);
     m.add(min(len, 32)<<6);
    }
    else 
    {
     m.add(-(CLog::ilog(len)<<2));
     m.add(-(min(len, 32)<<6));
    }
   }
   else
   {
    len=0;
    m.add(0);
    m.add(0);
   }
  }
  else
  {
   m.add(0);
   m.add(0);
  }

  scm1.mix(m);
  return result;
}
};


//////////////////////////// picModel //////////////////////////

// Model a 1728 by 2376 2-color CCITT bitmap image, left to right scan,
// MSB first (216 bytes per row, 513216 bytes total).  Insert predictions
// into m.

struct SCtx_picModel
{
 static const int N=3;  // number of contexts
// Context (Must come first)
 int& y;
 int& bpos;
 Buf& buf;
 Mixer& m; 
//-----------
 U32 r0, r1, r2, r3;  // last 4 rows, bit 8 is over current pixel
 Array<U8> t;  // model: cxt -> state
 int cxt[N];   // contexts
 StateMap sm[N];
 

 SCtx_picModel(Mixer& _m): m(_m), t(0x10200), y(_m.y), bpos(_m.bpos), buf(_m.buf) {}

void picModel(void) 
{

  // update the model
  int i;
  for ( i=0; i<N; ++i)
    t[cxt[i]]=nex(t[cxt[i]],y);

  // update the contexts (pixels surrounding the predicted one)
  r0+=r0+y;
  r1+=r1+((buf(215)>>(7-bpos))&1);
  r2+=r2+((buf(431)>>(7-bpos))&1);
  r3+=r3+((buf(647)>>(7-bpos))&1);
  cxt[0]=r0&0x7|r1>>4&0x38|r2>>3&0xc0;
  cxt[1]=0x100+(r0&1|r1>>4&0x3e|r2>>2&0x40|r3>>1&0x80);
  cxt[2]=0x200+(r0&0x3f^r1&0x3ffe^r2<<2&0x7f00^r3<<5&0xf800);

  // predict
  for ( i=0; i<N; ++i)
    m.add(Stretch::s(sm[i].p(y, t[cxt[i]])));
}
};

//////////////////////////// wordModel /////////////////////////

// Model English text (words and columns/end of line)
struct SCtx_wordModel
{
// Context (Must come first)
 int& c4;
 int& bpos;
 Buf& buf;
 Mixer& m;
//--------------
 U32 frstchar=0, spafdo=0, spaces=0, spacecount=0, words=0, wordcount=0;
 U32 word0=0, word1=0, word2=0, word3=0, word4=0, word5=0;  // hashes
 U32 text0=0;  // hash stream of letters
 ContextMap cm;
 int nl1=-3, nl=-2;  // previous, current newline position
 
 SCtx_wordModel(Mixer& _m): m(_m), cm(_m.MEM*16, 20+3+3+6+1+1+1+1-1), c4(_m.c4), bpos(_m.bpos), buf(_m.buf) {} 

void wordModel(void) 
{
  // Update word hashes
  if (bpos==0) {
    int c=c4&255;
    if (spaces&0x80000000) --spacecount;
	if (words&0x80000000) --wordcount;
	spaces=spaces*2;
	words=words*2;

    if (c>='A' && c<='Z')
      c+='a'-'A';
    if (c>='a' && c<='z'  || c>=128) {
                ++words, ++wordcount;
      word0=word0*263*32+c;
      text0=text0*997*16+c;
    }
    else if (word0) {
      word5=word4*23;
      word4=word3*19;
      word3=word2*17;
      word2=word1*13;
      word1=word0*11;
      word0=0;
     
    }
    
if( c=='.' || c=='!' || c=='?' || c==',' || c==';' || c==':') spafdo=0; else { ++spafdo; spafdo=min(63,spafdo); }
    if (c==32 || c==10) { ++spaces, ++spacecount; if (c==10) nl1=nl, nl=buf.pos-1;}
    int col=min(255, buf.pos-nl), above=buf[nl1+col]; // text column context
    if (col<=2) {
		if (col==2) frstchar=min(c,96); else frstchar=0;
	}

	cm.set(spafdo|col<<8 );
    cm.set(spafdo|spaces<<8 ); //
    cm.set(frstchar<<11|c);
    cm.set(col<<8|frstchar);
 //cm.set(words&127)
    cm.set(spaces&0x7fff);
    cm.set(frstchar<<7);
    cm.set(spaces&0xff);
    cm.set(c*64+spacecount/2);
    U32 h=wordcount*64+spacecount;
    cm.set((c<<13)+h);
    cm.set(h);
    cm.set(h+spafdo*8);

U32 d=c4&0xffff;
//cm.set(d);
cm.set(d<<9|frstchar);

h=word0*271+buf(1);
    cm.set(h);
    cm.set(word0);
    cm.set(h+word1);
    cm.set(word0+word1*31);
    cm.set(h+word1+word2*29);
    cm.set(text0&0xffffff);
    cm.set(text0&0xfffff);

    cm.set(h+word2);
    cm.set(h+word3);
    cm.set(h+word4);
    cm.set(h+word5);
    cm.set(buf(1)|buf(3)<<8|buf(5)<<16);
    cm.set(buf(2)|buf(4)<<8|buf(6)<<16);

    cm.set(h+word1+word3);
    cm.set(h+word2+word3);

    // Text column models
    cm.set(col<<16|buf(1)<<8|above);
    cm.set(buf(1)<<8|above);
    cm.set(col<<8|buf(1));
    cm.set(col*(c==32));
    cm.set(col);
  }
  cm.mix(m);
}
};

//////////////////////////// recordModel ///////////////////////

// Model 2-D data with fixed record length.  Also order 1-2 models
// that include the distance to the last match.
struct SCtx_recordModel
{
// Context (Must come first)
 int& c4;
 int& bpos;
 Buf& buf;
 Mixer& m;
//----------
 int cpos1[256], cpos2[256], cpos3[256], cpos4[256];
 int wpos1[0x10000]; // buf(1..2) -> last position
 int rlen=2, rlen1=3, rlen2=4;  // run length and 2 candidates
 int rcount1=0, rcount2=0;  // candidate counts
 ContextMap cm, cn, co, cp;
 
 SCtx_recordModel(Mixer& _m): m(_m), cm(32768, 3), cn(32768/2, 3), co(32768*2, 3), cp(_m.MEM, 3), c4(_m.c4), bpos(_m.bpos), buf(_m.buf) {}

void recordModel(void) 
{

  // Find record length
  if (!bpos) {
    int w=c4&0xffff, c=w&255, d=w>>8;
#if 1
    int r=buf.pos-cpos1[c];
    if (r>1 && r==cpos1[c]-cpos2[c]
        && r==cpos2[c]-cpos3[c] && r==cpos3[c]-cpos4[c]
        && (r>15 || (c==buf(r*5+1)) && c==buf(r*6+1))) {
      if (r==rlen1) ++rcount1;
      else if (r==rlen2) ++rcount2;
      else if (rcount1>rcount2) rlen2=r, rcount2=1;
      else rlen1=r, rcount1=1;
    }
    if (rcount1>15 && rlen!=rlen1) rlen=rlen1, rcount1=rcount2=0;
    if (rcount2>15 && rlen!=rlen2) rlen=rlen2, rcount1=rcount2=0;

    // Set 2 dimensional contexts
    ASSERT(rlen>0);
#endif
    cm.set(c<<8| (min(255, buf.pos-cpos1[c])/4) );
    cm.set(w<<9| CLog::llog(buf.pos-wpos1[w])>>2);
    
    cm.set(rlen|buf(rlen)<<10|buf(rlen*2)<<18);
    cn.set(w|rlen<<8);
    cn.set(d|rlen<<16);
    cn.set(c|rlen<<8);
    co.set(buf(1)<<8|min(255, buf.pos-cpos1[buf(1)]));
    co.set(buf(1)<<17|buf(2)<<9|CLog::llog(buf.pos-wpos1[w])>>2);
    int col=buf.pos%rlen;
    co.set(buf(1)<<8|buf(rlen));

    //cp.set(w*16);
    //cp.set(d*32);
    //cp.set(c*64);

    cp.set(rlen|buf(rlen)<<10|col<<18);
    cp.set(rlen|buf(1)<<10|col<<18);
    cp.set(col|rlen<<12);

    // update last context positions
    cpos4[c]=cpos3[c];
    cpos3[c]=cpos2[c];
    cpos2[c]=cpos1[c];
    cpos1[c]=buf.pos;
    wpos1[w]=buf.pos;
  }
  cm.mix(m);
  cn.mix(m);
  co.mix(m);
  cp.mix(m);
}
};

//////////////////////////// sparseModel ///////////////////////

// Model order 1-2 contexts with gaps.
struct SCtx_sparseModel
{
// Context (Must come first)
 int& c4;
 int& bpos;
 Buf& buf;
 Mixer& m; 
//----------------
 ContextMap cm;
 int mask = 0;

SCtx_sparseModel(Mixer& _m):  m(_m), cm(_m.MEM*2, 48+2+2+1), c4(_m.c4), bpos(_m.bpos), buf(_m.buf) {}

void sparseModel(int seenbefore, int howmany) 
{
  if (bpos==0) {
    cm.set( c4&0x00f0f0f0);
    cm.set((c4&0xf0f0f0f0)+1);
    cm.set((c4&0x00f8f8f8)+2);
    cm.set((c4&0xf8f8f8f8)+3);
    cm.set((c4&0x00e0e0e0)+4);
    cm.set((c4&0xe0e0e0e0)+5);
    cm.set((c4&0x00f0f0ff)+6);
     cm.set((c4&0xC3CCC38C)+7);
    cm.set((c4&0x0081CC81)+8);

    cm.set(seenbefore);
    cm.set(howmany);
    cm.set(c4&0x00ff00ff);
    cm.set(c4&0xff0000ff);
        cm.set(c4&0x00c10081);
    cm.set(c4&0x810000c1);
    cm.set(buf(1)|buf(5)<<8);
    cm.set(buf(1)|buf(6)<<8);
    cm.set(buf(3)|buf(6)<<8);
    cm.set(buf(4)|buf(8)<<8);
    
    for (int i=1; i<8; ++i) {
      cm.set((buf(i+1)<<8)|buf(i+2));
      cm.set((buf(i+1)<<8)|buf(i+3));
      cm.set(seenbefore|buf(i)<<8);
    }

    int fl = 0;
    if( c4&0xff != 0 ){
           if( isalpha( c4&0xff ) ) fl = 1;
      else if( ispunct( c4&0xff ) ) fl = 2;
      else if( isspace( c4&0xff ) ) fl = 3;
      else if( c4&0xff == 0xff ) fl = 4;
      else if( c4&0xff < 16 ) fl = 5;
      else if( c4&0xff < 64 ) fl = 6;
      else fl = 7;
    }
    mask = (mask<<3)|fl;
    cm.set(mask);
    cm.set(mask<<8|buf(1));
    cm.set(mask<<17|buf(2)<<8|buf(3));
    cm.set(mask&0x1ff|((c4&0xf0f0f0f0)<<9));
  }
  cm.mix(m);
}
};

//////////////////////////// distanceModel ///////////////////////

// Model for modelling distances between symbols
struct SCtx_distanceModel
{
// Context (Must come first)
 int& c4;
 int& bpos;
 Buf& buf;
 Mixer& m; 
//----------
 ContextMap cr;
 int pos00=0,pos20=0,posnl=0;

SCtx_distanceModel(Mixer& _m): m(_m), cr(_m.MEM, 3), c4(_m.c4), bpos(_m.bpos), buf(_m.buf) {}

void distanceModel(void) 
{
  if( bpos == 0 ){   
    int c=c4&0xff;
    if(c==0x00)pos00=buf.pos;
    if(c==0x20)pos20=buf.pos;
    if(c==0xff||c=='\r'||c=='\n')posnl=buf.pos;
    cr.set(min(buf.pos-pos00,255)|(c<<8));
    cr.set(min(buf.pos-pos20,255)|(c<<8));
    cr.set(min(buf.pos-posnl,255)|(c<<8)+234567);
  }
  cr.mix(m);
}
};

//////////////////////////// bmpModel /////////////////////////////////

// Model a 24-bit color uncompressed .bmp or .tif file.  Return
// width in pixels if an image file is detected, else 0.
struct SCtx_bmpModel
{
 static const int SC=0x20000;
// Context (Must come first)
 int& c4;
 int& bpos;
 Buf& buf;
 Mixer& m; 
//-----------------
 int w=0;  // width of image in bytes (pixels * 3)
 int eoi=0;     // end of image
 U32 tiff=0;    // offset of tif header
 SmallStationaryContextMap scm1, scm2, scm3, scm4, scm5, scm6, scm7, scm8, scm9;
 ContextMap cm;

 SCtx_bmpModel(Mixer& _m): m(_m), cm(_m.MEM*4, 10), scm1(SC), scm2(SC), scm3(SC), scm4(SC), scm5(SC), scm6(SC), scm7(SC), scm8(SC), scm9(SC*2), c4(_m.c4), bpos(_m.bpos), buf(_m.buf) {}

// 32-bit little endian number at buf(i)..buf(i-3)
inline U32 i4(int i) {
  ASSERT(i>3);
  return buf(i)+256*buf(i-1)+65536*buf(i-2)+16777216*buf(i-3);
}

// 16-bit
inline int i2(int i) {
  ASSERT(i>1);
  return buf(i)+256*buf(i-1);
}

// Square buf(i)
inline int sqrbuf(int i) {
  ASSERT(i>0);
  return buf(i)*buf(i);
}

int bmpModel(void) 
{      
  // Detect .bmp file header (24 bit color, not compressed)
  if (!bpos && buf(54)=='B' && buf(53)=='M'
      && i4(44)==54 && i4(40)==40 && i4(24)==0) {
    w=(i4(36)+3&-4)*3;  // image width
    const int height=i4(32);
    eoi=buf.pos;
    if (w<0x30000 && height<0x10000) {
      eoi=buf.pos+w*height;  // image size in bytes
      PRINTF("BMP %dx%d ", w/3, height);
    }
    else
      eoi=buf.pos;
  }

  // Detect .tif file header (24 bit color, not compressed).
  // Parsing is crude, won't work with weird formats.
  if (!bpos) {
    if (c4==0x49492a00) tiff=buf.pos;  // Intel format only
    if (buf.pos-tiff==4 && c4!=0x08000000) tiff=0; // 8=normal offset to directory
    if (tiff && buf.pos-tiff==200) {  // most of directory should be read by now
      int dirsize=i2(buf.pos-tiff-4);  // number of 12-byte directory entries
      w=0;
      int bpp=0, compression=0, width=0, height=0;
      for (int i=tiff+6; i<buf.pos-12 && --dirsize>0; i+=12) {
        int tag=i2(buf.pos-i);  // 256=width, 257==height, 259: 1=no compression
          // 277=3 samples/pixel
        int tagfmt=i2(buf.pos-i-2);  // 3=short, 4=long
        int taglen=i4(buf.pos-i-4);  // number of elements in tagval
        int tagval=i4(buf.pos-i-8);  // 1 long, 1-2 short, or points to array
        if ((tagfmt==3||tagfmt==4) && taglen==1) {
          if (tag==256) width=tagval;
          if (tag==257) height=tagval;
          if (tag==259) compression=tagval; // 1 = no compression
          if (tag==277) bpp=tagval;  // should be 3
        }
      }
      if (width>0 && height>0 && width*height>50 && compression==1
          && (bpp==1||bpp==3))
        eoi=tiff+width*height*bpp, w=width*bpp;
      if (eoi>buf.pos)
        PRINTF("TIFF %dx%dx%d ", width, height, bpp);
      else
        tiff=w=0;
    }
  }
  if (buf.pos>eoi) return w=0;

  // Select nearby pixels as context
  if (!bpos) {
    ASSERT(w>3);
    int color=buf.pos%3;
    int mean=buf(3)+buf(w-3)+buf(w)+buf(w+3);
    const int var=sqrbuf(3)+sqrbuf(w-3)+sqrbuf(w)+sqrbuf(w+3)-mean*mean/4>>2;
    mean>>=2;
    const int logvar=CLog::ilog(var);
    int i=0;
    cm.set(hash(++i, buf(3)>>4, buf(w)>>4, color));
    cm.set(hash(++i, buf(3), buf(1)>>3, color));
    cm.set(hash(++i, buf(3), buf(2)>>3, color));
    cm.set(hash(++i, buf(w), buf(1)>>3, color));
    cm.set(hash(++i, buf(w), buf(2)>>3, color));
//    cm.set(hash(++i, buf(3)+buf(w)>>1, color));
    cm.set(hash(++i, buf(3)+buf(w)>>3, buf(1)>>5, buf(2)>>5, color));
    cm.set(hash(++i, mean, logvar>>5, color));
    cm.set(hash(++i, buf(1), buf(2), color));
    cm.set(hash(++i, buf(3)+buf(1)-buf(4), color));
    cm.set(hash(++i, buf(w)+buf(1)-buf(w+1), color));
    scm1.set(buf(3)+buf(w)>>1);
    scm2.set(buf(3)+buf(w)-buf(w+3));
    scm3.set(buf(3)+buf(w-3)-buf(w));
    scm4.set(buf(3)*2-buf(6));
    scm5.set(buf(w)*2-buf(w*2));
    scm6.set(buf(w+3)*2-buf(w*2+6));
    scm7.set(buf(w-3)*2-buf(w*2-6));
    scm8.set(buf(w-3)+buf(1)-buf(w-2));
    scm9.set(mean>>1|logvar<<1&0x180);
  }

  // Predict next bit
  scm1.mix(m);
  scm2.mix(m);
  scm3.mix(m);
  scm4.mix(m);
  scm5.mix(m);
  scm6.mix(m);
  scm7.mix(m);
  scm8.mix(m);
  scm9.mix(m);
  cm.mix(m);   
  return w;
}
};

struct SCtx_model8bit
{
 static const int SC=0x20000;
// Context (Must come first)
 int& bpos;
 Buf& buf;
 Mixer& m;
//-----------
 SmallStationaryContextMap scm1, scm2, scm3, scm4, scm5, scm6, scm7;
 ContextMap cm;
 
 SCtx_model8bit(Mixer& _m): m(_m), cm(_m.MEM*4, 32+4), scm1(SC), scm2(SC), scm3(SC), scm4(SC), scm5(SC), scm6(SC*2), scm7(SC), bpos(_m.bpos), buf(_m.buf) {}

// Square buf(i)
inline int sqrbuf(int i) {
  ASSERT(i>0);
  return buf(i)*buf(i);
}

void model8bit(int w) 
{
	
	// Select nearby pixels as context
	if (!bpos) {
		ASSERT(w>3);
		int mean=buf(1)+buf(w-1)+buf(w)+buf(w+1);
		const int var=sqrbuf(1)+sqrbuf(w-1)+sqrbuf(w)+sqrbuf(w+1)-mean*mean/4>>2;
		mean>>=2;
		const int logvar=CLog::ilog(var);
		int i=0;
		// 2 x 
		cm.set(hash(++i, buf(1)>>2, buf(w)>>2));
		cm.set(hash(++i, buf(1)>>2, buf(2)>>2));
		cm.set(hash(++i, buf(w)>>2, buf(w*2)>>2));
		cm.set(hash(++i, buf(1)>>2, buf(w-1)>>2));
		cm.set(hash(++i, buf(w)>>2, buf(w+1)>>2));
		cm.set(hash(++i, buf(w+1)>>2, buf(w+2)>>2));
		cm.set(hash(++i, buf(w+1)>>2, buf(w*2+2)>>2));
		cm.set(hash(++i, buf(w-1)>>2, buf(w*2-2)>>2));
		
        cm.set(hash(++i, buf(1)>>2, buf(w-2)>>2));
		cm.set(hash(++i, buf(1)>>2, buf(w+2)>>2));
		cm.set(hash(++i, buf(1)+ buf(w-2)>>1));
		cm.set(hash(++i, buf(1)+buf(w+2)>>1));
		
		cm.set(hash(++i, buf(1)+buf(w)>>1));
		cm.set(hash(++i, buf(1)+buf(2)>>1));
		cm.set(hash(++i, buf(w)+buf(w*2)>>1));
		cm.set(hash(++i, buf(1)+buf(w-1)>>1));
		cm.set(hash(++i, buf(w)+buf(w+1)>>1));
		cm.set(hash(++i, buf(w+1)+buf(w+2)>>1));
		cm.set(hash(++i, buf(w+1)+buf(w*2+2)>>1));
		cm.set(hash(++i, buf(w-1)+buf(w*2-2)>>1));

		// 3 x
		cm.set(hash(++i, buf(w)>>2, buf(1)>>2, buf(w-1)>>2));
		cm.set(hash(++i, buf(w-1)>>2, buf(w)>>2, buf(w+1)>>2));
		cm.set(hash(++i, buf(1)>>2, buf(w-1)>>2, buf(w*2-1)>>2));

		// mixed
		cm.set(hash(++i, buf(3)+buf(w)>>1, buf(1)>>2, buf(2)>>2));
		cm.set(hash(++i, buf(2)+buf(1)>>1,buf(w)+buf(w*2)>>1,buf(w-1)>>2));
		cm.set(hash(++i, buf(2)+buf(1)>>2,buf(w-1)+buf(w)>>2));
		cm.set(hash(++i, buf(2)+buf(1)>>1,buf(w)+buf(w*2)>>1));
		cm.set(hash(++i, buf(2)+buf(1)>>1,buf(w-1)+buf(w*2-2)>>1));
		cm.set(hash(++i, buf(2)+buf(1)>>1,buf(w+1)+buf(w*2+2)>>1));
		cm.set(hash(++i, buf(w)+buf(w*2)>>1,buf(w-1)+buf(w*2+2)>>1));
		cm.set(hash(++i, buf(w-1)+buf(w)>>1,buf(w)+buf(w+1)>>1));
		cm.set(hash(++i, buf(1)+buf(w-1)>>1,buf(w)+buf(w*2)>>1));
		cm.set(hash(++i, buf(1)+buf(w-1)>>2,buf(w)+buf(w+1)>>2));

		cm.set(hash(++i, (buf(1)-buf(w-1)>>1)+buf(w)>>2));
		cm.set(hash(++i, (buf(w-1)-buf(w)>>1)+buf(1)>>2));
		cm.set(hash(++i, -buf(1)+buf(w-1)+buf(w)>>2));

		scm1.set(buf(1)+buf(w)>>1);
		scm2.set(buf(1)+buf(w)-buf(w+1)>>1);
		scm3.set(buf(1)*2-buf(2)>>1);
		scm4.set(buf(w)*2-buf(w*2)>>1);
		scm5.set(buf(1)+buf(w)-buf(w-1)>>1);
		scm6.set(mean>>1|logvar<<1&0x180);		 
	}

	// Predict next bit
	scm1.mix(m);
	scm2.mix(m);
	scm3.mix(m);
	scm4.mix(m);
	scm5.mix(m);
	scm6.mix(m);
	scm7.mix(m); // Amazingly but improves compression!	
	cm.mix(m);
	//return w;
}
};

//////////////////////////// pgmModel /////////////////////////////////

// Model a 8-bit grayscale uncompressed binary .pgm and 8-bit color
// uncompressed .bmp images.  Return width in pixels if an image file
// is detected, else 0.

#define ISWHITESPACE(i) (buf(i) == ' ' || buf(i) == '\t' || buf(i) == '\n' || buf(i) == '\r')
#define ISCRLF(i) (buf(i) == '\n' || buf(i) == '\r')

struct SCtx_pgmModel
{
// Context (Must come first)
 int& c0;
 int& bpos;
 Buf& buf;
 Mixer& m;
//---------
 int h = 0;		// height of image in bytes (pixels)
 int w = 0;		// width of image in bytes (pixels)
 int eoi = 0;     // end of image
 int pgm  = 0;    // offset of pgm header
 int pgm_hdr[3];  // 0 - Width, 1 - Height, 2 - Max value
 int pgm_ptr;		// which record in header should be parsed next
 int col = 0;     // Have to be static???

 SCtx_pgmModel(Mixer& _m): m(_m), c0(_m.c0), bpos(_m.bpos), buf(_m.buf) {}

int pgmModel(SCtx_model8bit& Mdl8Bit)   // Mixer should be same! 
{
	int isws;				// is white space
	char v_buf[32];			
	int  v_ptr;
	if (!bpos)
	{
		if(buf(3)=='P' && buf(2)=='5' && ISWHITESPACE(1)) // Detect PGM file
		{
			pgm = buf.pos;
			pgm_ptr = 0;
			return w = 0; // PGM header just detected, not enough info to get header yet
		}else 
			if(pgm && pgm_ptr!=3) 		// PGM detected, let's parse header records
			{ 
				for (int i = pgm; i<buf.pos-1 && pgm_ptr<3; i++)
				{
					// Skip white spaces
					while ((isws = ISWHITESPACE(buf.pos-i)) && i<buf.pos-1) i++; 
					if(isws) break; // buffer end is reached

					// Skip comments
					if(buf(buf.pos-i)=='#')
					{ 
						do {
							i++;
						}while(!ISCRLF(buf.pos-i) && i<buf.pos-1);
					}else
					{ 
						// Get header record as a string into v_buf
						v_ptr = 0;
						do {
							v_buf[v_ptr++] = buf(buf.pos-i);
							i++;
						}while(!(isws = ISWHITESPACE(buf.pos-i)) && i<buf.pos-1 && v_ptr<32);

						if(isws)
						{
							pgm_hdr[pgm_ptr++] = atoi(v_buf);
							pgm = i; // move pointer 
						}
					}
				}

				// Header is finished, next byte is first pixel
				if(pgm_ptr==3)
				{ 
					if(pgm_hdr[2] == 255 && pgm_hdr[0]>0 && pgm_hdr[1]>0)
					{
						w = pgm_hdr[0];
						h = pgm_hdr[1];
						eoi = buf.pos+w*h;
						PRINTF("PGM %dx%d",w,h);
					}
				}
			}
	}
	if (buf.pos>eoi) return w=0;
    Mdl8Bit.model8bit(w);
	  if (++col>=8) col=0; // reset after every 24 columns?
	  m.set(2, 8);
	  m.set(col, 8);
	  m.set(buf(w)+buf(1)>>4, 32);
	  m.set(c0, 256);
	return w;
}
};

struct SCtx_bmpModel8
{
// Context (Must come first)
 int& c0;
 int& bpos;
 Buf& buf;
 Mixer& m; 
//------------
 int h = 0;		// height of image in bytes (pixels)
 int w = 0;		// width of image in bytes (pixels)
 int eoi = 0;     // end of image
 int col = 0;
 int ibmp=0,w1=0;

 SCtx_bmpModel8(Mixer& _m): m(_m), c0(_m.c0), bpos(_m.bpos), buf(_m.buf) {}

// 32-bit little endian number at buf(i)..buf(i-3)
inline U32 i4(int i) {
  ASSERT(i>3);
  return buf(i)+256*buf(i-1)+65536*buf(i-2)+16777216*buf(i-3);
}

int bmpModel8(SCtx_model8bit& Mdl8Bit)  // Mixer should be same! 
{
	 if (bpos==0) {
        //  8-bit .bmp images               data offset      windows bmp    compression   bpp
		if (buf(54)=='B' && buf(53)=='M' && (i4(44)< 1079) && i4(40)==40 && i4(24)==0 && buf(26)==8){
            w1=i4(36);  // image width 
			h=i4(32);   // image height
			ibmp=buf.pos+i4(44)-54;
        }
        else if (i4(40)==40 && i4(24)==0 && (buf(26)==8)){
            w1=i4(36);  // image width 
			h=i4(32);   // image height
			ibmp=buf.pos+1024-40;
        }
        if (ibmp==buf.pos) {
			w=w1;
			eoi=buf.pos+w*h;
			PRINTF("BMP(8-bit) %dx%d",w,h);
			ibmp=0;
		}
	 }
	if (buf.pos>eoi) return w=0;
	  Mdl8Bit.model8bit(w);
	  if (++col>=8) col=0; // reset after every 24 columns?
	  m.set(2, 8);
	  m.set(col, 8);
	  m.set(buf(w)+buf(1)>>4, 32);
	  m.set(c0, 256);
	  return w;
}
};

struct SCtx_rgbModel8
{
// Context (Must come first)
 int& c0;
 int& bpos;
 Buf& buf;
 Mixer& m;
//----------------
 int w   = 0;		// width of image in bytes (pixels)
 int eoi = 0;       // end of image
 int col = 0;

 SCtx_rgbModel8(Mixer& _m): m(_m), c0(_m.c0), bpos(_m.bpos), buf(_m.buf) {}

// 16-bit
inline int i2(int i) {
  ASSERT(i>1);
  return buf(i)+256*buf(i-1);
}

int rgbModel8(SCtx_model8bit& Mdl8Bit)  // Mixer should be same! 
{
	int h = 0;		        // height of image in bytes (pixels)
	 // for .rgb gray images
	 if (bpos==0) {
		if (buf(507)==1 && buf(506)==218 && buf(505)==0 && i2(496)==1)
        {
			w=(buf(501)&255)*256|(buf(500)&255); // image width
			h=(buf(499)&255)*256|(buf(498)&255);  // image height
			eoi=buf.pos+w*h;
			PRINTF("RGB(8-bit) %dx%d",w,h);
		}
	 }
	if (buf.pos>eoi) return w=0;
	  Mdl8Bit.model8bit(w);
	  if (++col>=8) col=0; // reset after every 24 columns?
	  m.set(2, 8);
	  m.set(col, 8);
	  m.set(buf(w)+buf(1)>>4, 32);
	  m.set(c0, 256);
	  return w;
}
};

struct SCtx_bmpModel1
{
 static const int N=4+1;  // number of contexts
// Context (Must come first)
 int& y;
 int& bpos;
 Buf& buf;
 Mixer& m;
//------------
 U32 r0, r1, r2, r3;  // last 4 rows, bit 8 is over current pixel
 Array<U8> t;  // model: cxt -> state
 int cxt[N];   // contexts
 StateMap sm[N];
	 int h = 0;		  // height of image in bytes (pixels)
	 int w = 0;		  // width of image in bytes (pixels)
	 int eoi = 0;     // end of image
	 int ibmp=0;
	 int brow=0;

 SCtx_bmpModel1(Mixer& _m): m(_m), t(0x10200), y(_m.y), bpos(_m.bpos), buf(_m.buf) {}

// 32-bit little endian number at buf(i)..buf(i-3)
inline U32 i4(int i) {
  ASSERT(i>3);
  return buf(i)+256*buf(i-1)+65536*buf(i-2)+16777216*buf(i-3);
}

void bmpModel1(void) 
{      

	 if (bpos==0) {
        //  1-bit .bmp images                data offset      windows bmp    compression   bpp
		if (buf(54)=='B' && buf(53)=='M' && (i4(44)==0x3e) && i4(40)==40 && i4(24)==0 && buf(26)==1){
            w=i4(36);  // image width 
			h=i4(32);   // image height
			ibmp=buf.pos+i4(44)-62;
        }
        if (i4(40)==40 && i4(24)==0 && buf(26)==1){
            w=i4(36);  // image width 
			h=i4(32);   // image height
			ibmp=buf.pos+2;
        }
        if (ibmp==buf.pos) {
			brow=((((w-1)>>5)+1)*4);
			eoi=buf.pos+((((w-1)>>5)+1)*4*h);
			PRINTF("BMP(1-bit) %dx%d ",w,h);
			ibmp=0;
		}
	 }
  if (buf.pos>eoi) return;
  // update the model 
  int i;
  for ( i=0; i<N; ++i)
    t[cxt[i]]=nex(t[cxt[i]],y);

  // update the contexts (pixels surrounding the predicted one)
  r0+=r0+y;
  r1+=r1+((buf(brow-1)>>(7-bpos))&1);
  r2+=r2+((buf(brow+brow-1)>>(7-bpos))&1);
  r3+=r3+((buf(brow+brow+brow-1)>>(7-bpos))&1);
  cxt[0]=r0&0x7|r1>>4&0x38|r2>>3&0xc0;
  cxt[1]=0x100+(r0&1|r1>>4&0x3e|r2>>2&0x40|r3>>1&0x80);
  cxt[2]=0x200+(r0&0x3f^r1&0x3ffe^r2<<2&0x7f00^r3<<5&0xf800);
  cxt[3]=0x400+(r0&0x3e^r1&0x0C0C^r2&0xc800);  //?
  cxt[4]=0x800+(r1&0x30^r3&0x0c0c|r0&3);       //?

  // predict
  for ( i=0; i<N; ++i)
    m.add(Stretch::s(sm[i].p(y, t[cxt[i]])));
    return;  
}
};



//////////////////////////// exeModel /////////////////////////

// Model x86 code.  The contexts are sparse containing only those
// bits relevant to parsing (2 prefixes, opcode, and mod and r/m fields
// of modR/M byte).

struct SCtx_exeModel
{
 static const int N=12;
// Context (Must come first)
 int& bpos;
 Buf& buf;
 Mixer& m;
//-----------
 ContextMap cm;

 SCtx_exeModel(Mixer& _m): m(_m), cm(_m.MEM, N), bpos(_m.bpos), buf(_m.buf) {}

// Get context at buf(i) relevant to parsing 32-bit x86 code
U32 execxt(int i, int x=0) 
{
  int prefix=(buf(i+2)==0x0f)+2*(buf(i+2)==0x66)+3*(buf(i+2)==0x67)+4*(buf(i+3)==0x0f)+8*(buf(i+3)==0x66)+12*(buf(i+3)==0x67);
  int opcode=buf(i+1);
  int modrm=i ? buf(i)&0xc7 : 0;
  return prefix|opcode<<4|modrm<<12|x<<20;
}

void exeModel(void) 
{
  if (!bpos) {
    for (int i=0; i<N; ++i)
      cm.set(execxt(i, buf(1)*(i>4)));
  }
  cm.mix(m);
}
};


//////////////////////////// indirectModel /////////////////////

// The context is a byte string history that occurs within a 1 or 2 byte context.
struct SCtx_indirectModel
{
// Context (Must come first)
 int& c4;
 int& bpos;
 Mixer& m;
//-----------------
 ContextMap cm;
 U32 t1[256];
 U16 t2[0x10000];
  
 SCtx_indirectModel(Mixer& _m): m(_m), cm(_m.MEM, 6), c4(_m.c4), bpos(_m.bpos) {}

void indirectModel(void) 
{


  if (!bpos) {
    U32 d=c4&0xffff, c=d&255;
    U32& r1=t1[d>>8];
    r1=r1<<8|c;
    U16& r2=t2[c4>>8&0xffff];
    r2=r2<<8|c;
    U32 t=c|t1[c]<<8;
    cm.set(t&0xffff);
    cm.set(t&0xffffff);
    cm.set(t);
    cm.set(t&0xff00);
    t=d|t2[d]<<16;
    cm.set(t&0xffffff);
    cm.set(t);

  }
  cm.mix(m);
}
};

//////////////////////////// dmcModel //////////////////////////

// Model using DMC.  The bitwise context is represented by a state graph,
// initilaized to a bytewise order 1 model as in 
// http://plg.uwaterloo.ca/~ftp/dmc/dmc.c but with the following difference:
// - It uses integer arithmetic.
// - The threshold for cloning a state increases as memory is used up.
// - Each state maintains both a 0,1 count and a bit history (as in a
//   context model).  The 0,1 count is best for stationary data, and the
//   bit history for nonstationary data.  The bit history is mapped to
//   a probability adaptively using a StateMap.  The two computed probabilities
//   are combined.
// - When memory is used up the state graph is reinitialized to a bytewise
//   order 1 context as in the original DMC.  However, the bit histories
//   are not cleared.

struct SCtx_dmcModel
{
#pragma pack(push,1)
struct DMCNode {  // 12 bytes
  unsigned int nx[2];  // next pointers
  U8 state;  // bit history
  unsigned int c0:12, c1:12;  // counts * 256
};
#pragma pack(pop)

// Context (Must come first)
 int& y;
 int& bpos;
 Mixer& m;

 int top=0, curr=0;  // allocated, current node
 Array<DMCNode> t;  // state graph
 StateMap sm;
 int threshold=256;

 SCtx_dmcModel(Mixer& _m): m(_m), t(_m.MEM*2), y(_m.y), bpos(_m.bpos) {}

void dmcModel(void) 
{

  // clone next state
  if (top>0 && top<t.size()) {
    int next=t[curr].nx[y];
    int n=y?t[curr].c1:t[curr].c0;
    int nn=t[next].c0+t[next].c1;
    if (n>=threshold*2 && nn-n>=threshold*3) {
      int r=n*4096/nn;
      ASSERT(r>=0 && r<=4096);
      t[next].c0 -= t[top].c0 = t[next].c0*r>>12;
      t[next].c1 -= t[top].c1 = t[next].c1*r>>12;
      t[top].nx[0]=t[next].nx[0];
      t[top].nx[1]=t[next].nx[1];
      t[top].state=t[next].state;
      t[curr].nx[y]=top;
      ++top;
      if (top==m.MEM*2) threshold=512;
      if (top==m.MEM*3) threshold=768;
    }
  }

  // Initialize to a bytewise order 1 model at startup or when flushing memory
  if (top==t.size() && bpos==1) top=0;
  if (top==0) {
    ASSERT(t.size()>=65536);
    for (int i=0; i<256; ++i) {
      for (int j=0; j<256; ++j) {
        if (i<127) {
          t[j*256+i].nx[0]=j*256+i*2+1;
          t[j*256+i].nx[1]=j*256+i*2+2;
        }
        else {
          t[j*256+i].nx[0]=(i-127)*256;
          t[j*256+i].nx[1]=(i+1)*256;
        }
        t[j*256+i].c0=128;
        t[j*256+i].c1=128;
      }
    }
    top=65536;
    curr=0;
    threshold=256;
  }

  // update count, state
  if (y) {
    if (t[curr].c1<3800) t[curr].c1+=256;
  }
  else if (t[curr].c0<3800) t[curr].c0+=256;
  t[curr].state=nex(t[curr].state, y);
  curr=t[curr].nx[y];

  // predict
  const int pr1=sm.p(y, t[curr].state);
  const int n1=t[curr].c1;
  const int n0=t[curr].c0;
  const int pr2=(n1+5)*4096/(n0+n1+10);
  m.add(Stretch::s(pr1));
  m.add(Stretch::s(pr2));
}
};


struct SCtx_nestModel
{
// Context (Must come first)
 int& c4;
 int& bpos;
 Mixer& m; 
//-----------
 int ic=0, bc=0, pc=0,vc=0, qc=0, lvc=0, vcmc=0, llvc=0, tmc=0, wc=0;
 ContextMap cm;

 SCtx_nestModel(Mixer& _m): m(_m), cm(_m.MEM, 14-10), c4(_m.c4), bpos(_m.bpos) {}

int vcmap(int c)
{
  int lc = c;
  if (lc >= 'A' && lc <= 'Z')
    lc += 'a'-'A';
  if (lc == 'a' || lc == 'e' || lc == 'i' || lc == 'o' || lc == 'u')
    return 1;
  if (lc >= 'a' && lc <= 'z')
    return 2;
  if (lc == ' ' || lc == '.' || lc == ',' || lc == '\n')
    return 3;
  if (lc >= '0' && lc <= '9')
    return 4;
  if (lc == 'y')
    return 5;
  if (lc == '\'')
    return 6;
  return (c&32)?7:0;
}

void nestModel(void)
{
  if (bpos==0) {
    int c=c4&255;
    int matched=1, vv=vcmap(c);
    vc = (vc << 3) | vv;
    if (vv == lvc || vv == llvc)
      tmc += 1;
    else
      tmc = 0;
    if (vv == lvc)
      vcmc += 1;
    else {
      vcmc = 0;
      wc = (wc << 3) | vv;
      llvc=lvc;
      lvc = vv;
    }
    switch(c) {
    //  case ' ': qc = 0; break;
      case '(': ic += 513; break;
      case ')': ic -= 513; break;
      case '[': ic += 17; break;
      case ']': ic -= 17; break;
      case '<': ic += 23; /*qc += 34;*/ break;
      case '>': ic -= 23; /*qc /= 5;*/ break;
    //  case ':': pc = 20; break;
      case '{': ic += 22; break;
      case '}': ic -= 22; break;
   //   case '|': pc += 223; break;
    //  case '"': pc += 0x40; break;
    //  case '\'': pc += 0x42; break;
    //  case '\n': pc = qc = 0; break;
    //  case '.': pc = 0; break;
    //  case '!': pc = 0; break;
    //  case '?': pc = 0; break;
    //  case '#': pc += 0x08; break;
    //  case '%': pc += 0x76; break;
    ///  case '$': pc += 0x45; break;
    //  case '*': pc += 0x35; break;
    //  case '-': pc += 0x3; break;
    //  case '@': pc += 0x72; break;
    //  case '&': qc += 0x12; break;
    //  case ';': qc /= 3; break;
    //  case '\\': pc += 0x29; break;
    //  case '/': pc += 0x11;
   //             if (buf.size() > 1 && buf(1) == '<')
   //               qc += 74;
   //             break;
//      case ':': pc += 0x17; break;
 //     case '=': pc += 87; break;
      default: matched = 0;
    }
    if (matched)
      bc = 0;
    else
      bc += 1;
    if (bc > 300) {
      bc = 0;
      ic = 0;
      //pc = 0;
      //qc = 0;
    }
    cm.set(ic&0xffff);
    //cm.set((3*pc)&0xffff);
    cm.set((1*vc)&0xffff);
   // cm.set((781*qc+5*(tmc/2))&0xffff);
    cm.set(((13*vc+ic))&0xffff);
   // cm.set(((17*pc+7*ic))&0xffff);
   // cm.set(((vc/3+pc))&0xffff);
   // cm.set(((7*wc+qc))&0xffff);
   // cm.set(((3*ic+qc+97*vcmc))&0xffff);
  //  cm.set((3*vc+77*pc+373*ic+qc)&0xffff);
  //  cm.set((31*vc+27*pc+281*qc)&0xffff);
    cm.set((13*bc+((lvc+3*llvc)/2))&0xffff);
    //cm.set((13*vc+271*ic+qc+bc)&0xffff);
  //  cm.set((18*vc+871*ic+qc)&0xffff);
  //  cm.set((19*qc+713*pc+qc)&0xffff);
  }
  cm.mix(m);
}
};

#include "PaqMdlJPEG.hpp"

//////////////////////////// contextModel //////////////////////
//int primes[]={ 0, 257,251,241,239,233,229,227,223,211,199,197,193,191 };
//static U32 WRT_mpw[16]= { 3, 3, 3, 2, 2, 2, 1, 1,  1, 1, 1, 1, 1, 0, 0, 0 }, tri[4]={0,4,3,7}, trj[4]={0,6,6,12};
//static U32 WRT_mtt[16]= { 0, 0, 1, 2, 3, 4, 5, 5,  6, 6, 6, 6, 6, 7, 7, 7 };

//static inline int typesize[14]={0,0,0,0,0,0,0,0,0,0,0,0,0,0};
//static inline const char* typenames[14]={"", "jpeg ", "bmp1 ", "bmp4 ", "bmp8 ", "bmp24 ", "tiff ", "pgm ", "rgb ", "exe ", "text ", "wav ", "mrb ","utf8 "};
// This combines all the context models with a Mixer.

struct SModelsCtx
{
// Context (Must come first)
 int& y;
 int& c0;
 int& c4;
 int& bpos;
 Buf& buf;
 int level;
//-----------
 Mixer m;
 ContextMap cm;
 RunContextMap rcm7, rcm9, rcm10;
 U32 cxt[16];  // order 0-11 contexts
 EFiletype filetype=DEFAULT;
 int size=0;  // bytes remaining in block
 int col = 0;    // Have to be static?

 SCtx_matchModel  _matchModel;
 SCtx_picModel    _picModel;
 SCtx_wordModel   _wordModel;
 SCtx_recordModel  _recordModel;
 SCtx_sparseModel   _sparseModel;
 SCtx_distanceModel _distanceModel;
 SCtx_jpegModel   _jpegModel;
 SCtx_bmpModel    _bmpModel;
 SCtx_model8bit  _model8bit;
 SCtx_pgmModel   _pgmModel;
 SCtx_bmpModel8  _bmpModel8;
 SCtx_rgbModel8  _rgbModel8;
 SCtx_bmpModel1  _bmpModel1;
 SCtx_exeModel  _exeModel;
 SCtx_indirectModel  _indirectModel;
 SCtx_dmcModel    _dmcModel;
 SCtx_nestModel   _nestModel;
//----------------

// NOTE: Constructorrs called in same order ar members appear in the class  (Mixer must come first)
SModelsCtx(int lvl, U32  Mem, int& _y, int& _c0, int& _c4, int& _bpos, Buf& _buf, Random& _rnd): level(lvl), cm(Mem*32, 9), rcm7(Mem, _c0, _bpos, _buf), rcm9(Mem, _c0, _bpos, _buf), rcm10(Mem, _c0, _bpos, _buf), m(Mem, _rnd, _buf, _y, _c0, _c4, _bpos, 800, 3088, 7, 128), y(_y), c0(_c0), c4(_c4), bpos(_bpos), buf(_buf),
  _matchModel(m), _picModel(m), _wordModel(m), _recordModel(m), _sparseModel(m), _distanceModel(m), _jpegModel(m), _bmpModel(m), _model8bit(m), _pgmModel(m), _bmpModel8(m), _rgbModel8(m), _bmpModel1(m), _exeModel(m), _indirectModel(m), _dmcModel(m), _nestModel(m) {}

int contextModel2(void) 
{

  // Parse filetype and size
  if (bpos==0) {
    --size;
    if (size==-1) filetype=(EFiletype)buf(1);
    if (size==-5) {
      size=buf(4)<<24|buf(3)<<16|buf(2)<<8|buf(1);
//      if (filetype<=14) /*PRINTF("\n(%s%d)", typenames[filetype], size),*/ typesize[filetype]+=size;       // Useless
      if (filetype==EXE) size+=8;
    }
  }

  m.update();
  m.add(256);

  // Test for special file types
  int ismatch=CLog::ilog(_matchModel.matchModel());  // Length of longest matching context
  switch (filetype)
	{
    //case DEFAULT:	break;
	case JPEG: {
			int isjpeg=_jpegModel.jpegModel();  // 1-257 if JPEG is detected, else 0
			if (isjpeg) {
				m.set(1, 8);
				m.set(isjpeg-1, 257);
				m.set(buf(1), 256);
				return m.p();
			}
			break;
		}
	case BMPFILE24:
	case TIFFFILE:{ 
			int isbmp=_bmpModel.bmpModel(); // Image width (bytes) if BMP or TIFF detected, or 0
			if (isbmp>0) {
				if (++col>=24) col=0;
				m.set(2, 8);
				m.set(col, 24);
				m.set(buf(isbmp)+buf(3)>>4, 32);
				m.set(c0, 256);
				return m.p();
			}
			break;
		}
	case BMPFILE8:{ 
			if (_bmpModel8.bmpModel8(_model8bit)>0) return m.p(); // Image width (bytes) if BMP8 detected, or 0 
			break;
		}
	case PGMFILE:{
			if (_pgmModel.pgmModel(_model8bit)>0) return m.p(); // Image width (bytes) if PGM (P5,PGM_MAXVAL = 255) detected, or 0
			break;
		}

	case BMPFILE1:{
			_bmpModel1.bmpModel1();
			break;
		} // 

	case RGBFILE:{ 
			if (_rgbModel8.rgbModel8(_model8bit)>0) return m.p(); // Image width (bytes) if RGB8 detected, or 0 
			break;
		}
	default: {
			break;
		}
  }
  // Normal model
  if (bpos==0) {
     int i=0;

    for ( i=15; i>0; --i)  // update order 0-11 context hashes
      cxt[i]=cxt[i-1]*257+(c4&255)+1;
    for ( i=0; i<7; ++i)
      cm.set(cxt[i]);
    rcm7.set(cxt[7]);
    cm.set(cxt[8]);
    rcm9.set(cxt[10]);
    rcm10.set(cxt[12]);
    cm.set(cxt[14]);
  }
  int order=cm.mix(m);
  
  rcm7.mix(m);
  rcm9.mix(m);
  rcm10.mix(m);

  if (level>=4) {
	switch (filetype)
	{
	case TXTUTF8:
	case TEXT: { 
			_sparseModel.sparseModel(ismatch,order);
		//	_distanceModel.distanceModel();
			_nestModel.nestModel();
			//_recordModel.recordModel();  
			_wordModel.wordModel();
			_indirectModel.indirectModel();
			_dmcModel.dmcModel();
			 break;
		}
	case EXE: {
			_sparseModel.sparseModel(ismatch,order);
		//	_distanceModel.distanceModel();
		//	_recordModel.recordModel();  
			//_wordModel.wordModel();
			_indirectModel.indirectModel();
			_dmcModel.dmcModel();
			_exeModel.exeModel();
			break;
		} 
	case BMPFILE1: break;
	default: { 
			_sparseModel.sparseModel(ismatch,order);
			_distanceModel.distanceModel();
			_picModel.picModel();
			_recordModel.recordModel();  
			//_wordModel.wordModel();
			_indirectModel.indirectModel();
			_dmcModel.dmcModel();
			break;
		} 
    }
	}





  order = order-2;
  if(order<0) order=0;

  U32 c1=buf(1), c2=buf(2), c3=buf(3), c;

  m.set(c1+8, 264);
  m.set(c0, 256);
  m.set(order+8*(c4>>5&7)+64*(c1==c2)+128*(filetype==EXE), 256);
  m.set(c2, 256);
  m.set(c3, 256);
  m.set(ismatch, 256);
  
  if(bpos)
  {	
    c=c0<<(8-bpos); if(bpos==1)c+=c3/2;
    c=(min(bpos,5))*256+c1/32+8*(c2/32)+(c&192);
  }
  else c=c3/128+(c4>>31)*2+4*(c2/64)+(c1&240);
  m.set(c, 1536);
  int pr=m.p();
  return pr;
}

};


