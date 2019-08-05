
//////////////////////////// Predictor /////////////////////////

// A Predictor estimates the probability that the next bit of
// uncompressed data is 1.  Methods:
// p() returns P(1) as a 12 bit number (0-4095).
// update(y) trains the predictor with the actual bit (0 or 1).

class Predictor 
{
  int pr;  // next prediction
  APM1 a, a1, a2, a3, a4, a5, a6;
  SModelsCtx Models;

// Context
 int& y;
 int& c0;
 int& c4;
 int& bpos;
 Buf& buf;
                                            
public:
 Predictor(int lvl, int& _y, int& _c0, int& _c4, int& _bpos, Buf& _buf, Random& _rnd): pr(2048), y(_y), c0(_c0), c4(_c4), bpos(_bpos), buf(_buf),  Models(lvl, CALCMEM(lvl), _y, _c0, _c4, _bpos, _buf, _rnd),
                                                                     a(256), a1(0x10000), a2(0x10000), a3(0x10000), a4(0x10000), a5(0x10000), a6(0x10000) {}
int p() const {ASSERT(pr>=0 && pr<4096); return pr;}
void update(void) 
{
  // Update global context: pos, bpos, c0, c4, buf
  c0+=c0+y;
  if (c0>=256) {
    buf[buf.pos++]=c0;
    c4=(c4<<8)+c0-256;
    c0=1;
  }
  bpos=(bpos+1)&7;

  // Filter the context model with APMs
  int pr0=Models.contextModel2();

  pr=a.p(y, pr0, c0);
  
  int pr1=a1.p(y, pr0, c0+256*buf(1));
  int pr2=a2.p(y, pr0, c0^hash(buf(1), buf(2))&0xffff);
  int pr3=a3.p(y, pr0, c0^hash(buf(1), buf(2), buf(3))&0xffff);
  pr0=pr0+pr1+pr2+pr3+2>>2;
  
      pr1=a4.p(y, pr, c0+256*buf(1));
      pr2=a5.p(y, pr, c0^hash(buf(1), buf(2))&0xffff);
      pr3=a6.p(y, pr, c0^hash(buf(1), buf(2), buf(3))&0xffff);
  pr=pr+pr1+pr2+pr3+2>>2;

  pr=pr+pr0+1>>1;
}
};


//////////////////////////// Encoder ////////////////////////////

// An Encoder does arithmetic encoding.  Methods:
// Encoder(COMPRESS, f) creates encoder for compression to archive f, which
//   must be open past any header for writing in binary mode.
// Encoder(DECOMPRESS, f) creates encoder for decompression from archive f,
//   which must be open past any header for reading in binary mode.
// code(i) in COMPRESS mode compresses bit i (0 or 1) to file f.
// code() in DECOMPRESS mode returns the next decompressed bit from file f.
//   Global y is set to the last bit coded or decoded by code().
// compress(c) in COMPRESS mode compresses one byte.
// decompress() in DECOMPRESS mode decompresses and returns one byte.
// flush() should be called exactly once after compression is done and
//   before closing f.  It does nothing in DECOMPRESS mode.
// size() returns current length of archive
// setFile(f) sets alternate source to FILE* f for decompress() in COMPRESS
//   mode (for testing transforms).
// If level (global) is 0, then data is stored without arithmetic coding.

class Encoder 
{
public:
struct SDecodeCtx
{
 EFiletype type=DEFAULT;
 unsigned long blklen=0;
 unsigned long len=0;          // int
// Exe decoder specific
 int offset=0, q=0;  // decode state: file offset, queue size
 int size=0;   // where to stop coding
 int begin=0;  // offset in file
 U8 c[5];  // queue of last 5 bytes, c[0] at front
}dctx;


// Static tables initialize
 CLog    llg;
 Stretch str;

private:
  Random rnd;
  Predictor predictor;
  const EMode mode;       // Compress or decompress?
  STRM* archive;         // Compressed data file
  U32 x1, x2;            // Range, initially [0, 1), scaled by 2^32
  U32 x;                 // Decompress mode: last 4 input bytes of archive
  STRM *alt;             // decompress() source in COMPRESS mode
  int level;
// Global context set by Predictor and available to all models.
  int y = 0;      // Last bit, 0 or 1, set by encoder
  int c0 = 1;     // Last 0-7 bits of the partial byte with a leading 1 bit (1-255)
  int c4 = 0;     // Last 4 whole bytes, packed.  Last byte is bits 0-7.
  int bpos = 0;   // bits in c0 (0 to 7)
  Buf buf;        // Rotating input queue set by Predictor    

  // Compress bit y or return decompressed bit
  int code(int i=0) 
{
    int p=predictor.p();
    ASSERT(p>=0 && p<4096);
    p+=p<2048;
    U32 xmid=x1 + (x2-x1>>12)*p + ((x2-x1&0xfff)*p>>12);
    ASSERT(xmid>=x1 && xmid<x2);
    if (mode==DECOMPRESS) y=x<=xmid; else y=i;
    y ? (x2=xmid) : (x1=xmid+1);
    predictor.update();
    while (((x1^x2)&0xff000000)==0) {  // pass equal leading bytes of range
      if (mode==COMPRESS) archive->putc(x2>>24);
      x1<<=8;
      x2=(x2<<8)+255;
      if (mode==DECOMPRESS) x=(x<<8)+(archive->getc()&255);  // EOF is OK
    }
    return y;
  }

public:
Encoder(int lvl, EMode m, STRM* f): level(lvl), mode(m), archive(f), x1(0), x2(0xffffffff), x(0), alt(0), predictor(lvl, y, c0, c4, bpos, buf, rnd) 
{
  if (level>0 && mode==DECOMPRESS) {  // x = first 4 bytes of archive
    for (int i=0; i<4; ++i)
      x=(x<<8)+(archive->getc()&255);
  }
//  for (int i=0; i<1024; ++i)dt[i]=16384/(i+i+3);     // Moved to StateMap, the only place it is used
 buf.setsize((CALCMEM(level))*8);   // Why 8; Why it was outside of the Encoder?
}
  void ResetDecoderCtx(void){memset(&dctx,0,sizeof(dctx));}
  U32 getMem(void){return CALCMEM(level);}
  EMode getMode() const {return mode;}
  unsigned long GetCurrDecBlkLen(void){return dctx.blklen;}
  unsigned long size() const {return archive->ftell();}  // length of archive so far
  void flush()  // call this when compression is finished
{
  if (mode==COMPRESS && level>0)
    archive->putc(x1>>24);  // Flush first unequal byte of range
}
  void setFile(STRM* f) {alt=f;}

  // Compress one byte
  void compress(int c) {
    ASSERT(mode==COMPRESS);
    if (level==0)
      archive->putc(c);
    else 
      for (int i=7; i>=0; --i)
        code((c>>i)&1);
  }

  // Decompress and return one byte
  int decompress() {
    if (mode==COMPRESS) {
      ASSERT(alt);
      return alt->getc();
    }
    else if (level==0)
      return archive->getc();
    else {
      int c=0;
      for (int i=0; i<8; ++i)
        c+=c+code();
      return c;
    }
  }
};


// Default encoding as self
static inline void encode_default(STRM* in, STRM* out, int len) {
  while (len--) out->putc(in->getc());
}

static inline int decode_default(Encoder& en) {
  return en.decompress();
}

// JPEG encode as self.  The purpose is to shield jpegs from exe transform.
static inline void encode_jpeg(STRM* in, STRM* out, int len) {
  while (len--) out->putc(in->getc());
}

static inline int decode_jpeg(Encoder& en) {
  return en.decompress();
}
// BMP encode as self.
static inline void encode_bmp(STRM* in, STRM* out, int len) {
  while (len--) out->putc(in->getc());
}

static inline int decode_bmp(Encoder& en) {
  return en.decompress();
}

// PGM encode as self.
static inline void encode_pgm(STRM* in, STRM* out, int len) {
  while (len--) out->putc(in->getc());
}

static inline int decode_pgm(Encoder& en) {
  return en.decompress();
}

// RGB encode as self.
static inline void encode_rgb(STRM* in, STRM* out, int len) {
  while (len--) out->putc(in->getc());
}

static inline int decode_rgb(Encoder& en) {
  return en.decompress();
}

// EXE transform: <encoded-size> <begin> <block>...
// Encoded-size is 4 bytes, MSB first.
// begin is the offset of the start of the input file, 4 bytes, MSB first.
// Each block applies the e8e9 transform to strings falling entirely
// within the block starting from the end and working backwards.
// The 5 byte pattern is E8/E9 xx xx xx 00/FF (x86 CALL/JMP xxxxxxxx)
// where xxxxxxxx is a relative address LSB first.  The address is
// converted to an absolute address by adding the offset mod 2^25
// (in range +-2^24).

static inline int encode_exe(STRM* in, STRM* out, int len, int begin) 
{
  const int BLOCK=0x10000;
  Array<U8> blk(BLOCK);
  out->putc(len>>24); out->putc(len>>16); out->putc(len>>8); out->putc(len);           // fPRINTF(out, "%c%c%c%c", len>>24, len>>16, len>>8, len); // size, MSB first
  out->putc(begin>>24); out->putc(begin>>16); out->putc(begin>>8); out->putc(begin);   // fPRINTF(out, "%c%c%c%c", begin>>24, begin>>16, begin>>8, begin); 

  // Transform
  for (int offset=0; offset<len; offset+=BLOCK) {
    int size=min(len-offset, BLOCK);
    int bytesRead=in->fread(&blk[0], 1, size);
    if (bytesRead!=size)return -1; // quit("encode_exe read error");
    for (int i=bytesRead-1; i>=4; --i) {
      if ((blk[i-4]==0xe8||blk[i-4]==0xe9) && (blk[i]==0||blk[i]==0xff)) {
        int a=(blk[i-3]|blk[i-2]<<8|blk[i-1]<<16|blk[i]<<24)+offset+begin+i+1;
        a<<=7;
        a>>=7;
        blk[i]=a>>24;
        blk[i-1]=a>>16;
        blk[i-2]=a>>8;
        blk[i-3]=a;
      }
    }
    out->fwrite(&blk[0], 1, bytesRead);
  }
 return 0;
}

static inline int decode_exe(Encoder& en)    
{
 const int BLOCK=0x10000;   // block size
 int& offset=en.dctx.offset;   // decode state: file offset
 int& q=en.dctx.q;  // queue size
 int& size=en.dctx.size;   // where to stop coding
 int& begin=en.dctx.begin;  // offset in file
 U8*  c = (U8*)&en.dctx.c;  // queue of last 5 bytes, c[0] at front
 
  //SDecodeCtx& ctx = en.dctx;
  // Read size from first 4 bytes, MSB first
  while (offset==size && q==0) 
  {
    offset=0;
    size=en.decompress()<<24;
    size|=en.decompress()<<16;
    size|=en.decompress()<<8;
    size|=en.decompress();
    begin=en.decompress()<<24;
    begin|=en.decompress()<<16;
    begin|=en.decompress()<<8;
    begin|=en.decompress();
  }

  // Fill queue
  while (offset<size && q<5) {
    memmove(c+1, c, 4);
    c[0]=en.decompress();
    ++q;
    ++offset;
  }

  // E8E9 transform: E8/E9 xx xx xx 00/FF -> subtract location from x
  if (q==5 && (c[4]==0xe8||c[4]==0xe9) && (c[0]==0||c[0]==0xff) && ((offset-1^offset-5)&-BLOCK)==0) { // not crossing block boundary
    int a=(c[3]|c[2]<<8|c[1]<<16|c[0]<<24)-offset-begin;
    a<<=7;
    a>>=7;
    c[3]=a;
    c[2]=a>>8;
    c[1]=a>>16;
    c[0]=a>>24;
  }

  // return oldest byte in queue
  ASSERT(q>0 && q<=5);
  return c[--q];
}



// Split n bytes into blocks by type.  For each block, output
// <type> <size> and call encode_X to convert to type X.
static inline void encode(STRM* in, STRM* out, int n) 
{
  EFiletype type=DEFAULT;
  unsigned long begin=in->ftell();     // int
  while (n>0) {
    EFiletype nextType=detect(in, n, type);
    unsigned long end=in->ftell();    // int
    in->fseek(begin, SEEK_SET);
    unsigned long len= end - begin;      // int len=int(end-begin);     
    if (end > begin) {                  
      out->putc(type); out->putc(len>>24); out->putc(len>>16); out->putc(len>>8); out->putc(len);  //  fPRINTF(out, "%c%c%c%c%c", type, len>>24, len>>16, len>>8, len);
      switch(type) {
        case JPEG: encode_jpeg(in, out, len); break;
        case BMPFILE1:
        case BMPFILE4:
		case BMPFILE8:
		case BMPFILE24:
			encode_bmp(in, out, len); break;
		case PGMFILE: encode_pgm(in, out, len); break;
		case RGBFILE: encode_rgb(in, out, len); break;
        case EXE:  encode_exe(in, out, len, begin); break;
        default:   encode_default(in, out, len); break;
      }
    }
    n-=len;
    type=nextType;
    begin=end;
  }
}

// Decode <type> <len> <data>...
static inline int decode(Encoder& en)     
{      
  while (en.dctx.len==0) {        // TODO: Stream end detection
    en.dctx.type=(EFiletype)en.decompress();
    en.dctx.blklen=en.decompress()<<24;         // NOTE: Size is signed here!
    en.dctx.blklen|=en.decompress()<<16;
    en.dctx.blklen|=en.decompress()<<8;
    en.dctx.blklen|=en.decompress();    
    en.dctx.len = en.dctx.blklen;         //    if (en.dctx.len<0) en.dctx.len=1;        // ?????????
  }
  --en.dctx.len;
  switch (en.dctx.type) {
    case JPEG: return decode_jpeg(en);
    case BMPFILE1:
    case BMPFILE4:
    case BMPFILE8:
    case BMPFILE24:
		return decode_bmp(en);
    case PGMFILE: return decode_pgm(en);
	case RGBFILE: return decode_rgb(en);
    case EXE:  return decode_exe(en);
    default:   return decode_default(en);
  }
}
