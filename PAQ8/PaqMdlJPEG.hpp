
//////////////////////////// jpegModel /////////////////////////

// Model JPEG. Return 1-257 if a JPEG file is detected or else 0.
// Only the baseline and 8 bit extended Huffman coded DCT modes are
// supported.  The model partially decodes the JPEG image to provide
// context for the Huffman coded symbols.

// Print a JPEG segment at buf[p...] for debugging
/*void dump(const char* msg, int p) {
  PRINTF("%s:", msg);
  int len=buf[p+2]*256+buf[p+3];
  for (int i=0; i<len+2; ++i)
    PRINTF(" %02X", buf[p+i]);
  PRINTF("\n");
} */

struct SCtx_jpegModel
{
struct HUF {U32 min, max; int val;}; // Huffman decode tables
  // huf[Tc][Th][m] is the minimum, maximum+1, and pointer to codes for
  // coefficient type Tc (0=DC, 1=AC), table Th (0-3), length m+1 (m=0-15)

// Context (Must come first)
 int& y;
 int& c0;
 int& c4;
 int& bpos;
 Buf& buf;
 Mixer& m;
//-------------------
  // Context model
  static const int N=28; // size of t, number of contexts
  BH<9> t;  // context hash -> bit history
    // As a cache optimization, the context does not include the last 1-2
    // bits of huffcode if the length (huffbits) is not a multiple of 3.
    // The 7 mapped values are for context+{"", 0, 00, 01, 1, 10, 11}.
  Array<U32> cxt;  // context hashes
  Array<U8*> cp;  // context pointers
  StateMap sm[N];
  Mixer m1;
  APM a1, a2;

  // State of parser
  enum {SOF0=0xc0, SOF1, SOF2, SOF3, DHT, RST0=0xd0, SOI=0xd8, EOI, SOS, DQT, DNL, DRI, APP0=0xe0, COM=0xfe, FF};  // Second byte of 2 byte codes
  int jpeg=0;  // 1 if JPEG is header detected, 2 if image data
  int next_jpeg=0;  // updated with jpeg on next byte boundary
  int app;  // Bytes remaining to skip in APPx or COM field
  int sof=0, sos=0, data=0;  // pointers to buf
  Array<int> ht;  // pointers to Huffman table headers
  int htsize=0;  // number of pointers in ht

  // Huffman decode state
  U32 huffcode=0;  // Current Huffman code including extra bits
  int huffbits=0;  // Number of valid bits in huffcode
  int huffsize=0;  // Number of bits without extra bits
  int rs=-1;  // Decoded huffcode without extra bits.  It represents
    // 2 packed 4-bit numbers, r=run of zeros, s=number of extra bits for
    // first nonzero code.  huffcode is complete when rs >= 0.
    // rs is -1 prior to decoding incomplete huffcode.
  int mcupos=0;  // position in MCU (0-639).  The low 6 bits mark
    // the coefficient in zigzag scan order (0=DC, 1-63=AC).  The high
    // bits mark the block within the MCU, used to select Huffman tables.

  // Decoding tables
  Array<HUF> huf;  // Tc*64+Th*16+m -> min, max, val
  int mcusize=0;  // number of coefficients in an MCU
  int linesize=0; // width of image in MCU
  int hufsel[2][10];  // DC/AC, mcupos/64 -> huf decode table
  Array<U8> hbuf;  // Tc*1024+Th*256+hufcode -> RS

  // Image state
  Array<int> color;  // block -> component (0-3)
  Array<int> pred;  // component -> last DC value
  int dc=0;  // DC value of the current block
  int width=0;  // Image width in MCU
  int row=0, column=0;  // in MCU (column 0 to width-1)
  Buf cbuf; // Rotating buffer of coefficients, coded as:
    // DC: level shifted absolute value, low 4 bits discarded, i.e.
    //   [-1023...1024] -> [0...255].
    // AC: as an RS code: a run of R (0-15) zeros followed by an S (0-15)
    //   bit number, or 00 for end of block (in zigzag order).
    //   However if R=0, then the format is ssss11xx where ssss is S,
    //   xx is the first 2 extra bits, and the last 2 bits are 1 (since
    //   this never occurs in a valid RS code).
  int cpos=0;  // position in cbuf
  U32 huff1=0, huff2=0, huff3=0, huff4=0;  // hashes of last codes
  int rs1, rs2, rs3, rs4;  // last 4 RS codes
  int ssum=0, ssum1=0, ssum2=0, ssum3=0, ssum4=0;
    // sum of S in RS codes in block and last 4 values

  IntBuf cbuf2;
  Array<int> adv_pred, sumu, sumv;
  Array<int> ls;  // block -> distance to previous block
  Array<int> lcp, zpos;

    //for parsing Quantization tables
  int dqt_state = -1, dqt_end = 0, qnum = 0;
  Array<U8> qtab; // table
  Array<int> qmap; // block -> table number
//------------------------
 int sumu2[8], sumv2[8], sumu3[8], sumv3[8], kx[32];
 int hbcount=2;
  

 SCtx_jpegModel(Mixer& _m): m(_m), y(_m.y), c0(_m.c0), c4(_m.c4), bpos(_m.bpos), buf(_m.buf),
                            t(_m.MEM), cxt(N), cp(N), m1(_m.MEM, _m.rnd, _m.buf, _m.y, _m.c0, _m.c4, _m.bpos, 32, 770, 3), a1(0x8000), a2(0x10000),
                            ht(8), huf(128), hbuf(2048), color(10), pred(4), cbuf(0x20000), cbuf2(0x20000),
                            adv_pred(7), sumu(8), sumv(8), ls(10), lcp(4), zpos(64), qtab(256), qmap(10) {}

// Detect invalid JPEG data.  The proper response is to silently
// fall back to a non-JPEG model.
#define JASSERT(x) if (!(x)) { \
/*  PRINTF("JPEG error at %d, line %d: %s\n", pos, __LINE__, #x); */ \
  jpeg=0; \
  return next_jpeg;}

void update_k(int v1, int v2, int &k1, int &k2) {
  int a, b, c;
  a=abs(v1*(k1-1)+v2*(8-(k1-1)))/8;
  b=abs(v1*(k1+0)+v2*(8-(k1+0)))/8;
  c=abs(v1*(k1+1)+v2*(8-(k1+1)))/8;
  if (k1==0) a=b; else if (k1==8) c=b;
  if (a<b && a<c) k2--;
  if (c<a && c<b) k2++;
  if (k2<-2) {k1--;k2=0;}
  if (k2>+2) {k1++;k2=0;}
}


public:
int jpegModel(void)    
{
  const static U8 zzu[64]={  // zigzag coef -> u,v
    0,1,0,0,1,2,3,2,1,0,0,1,2,3,4,5,4,3,2,1,0,0,1,2,3,4,5,6,7,6,5,4,
    3,2,1,0,1,2,3,4,5,6,7,7,6,5,4,3,2,3,4,5,6,7,7,6,5,4,5,6,7,7,6,7};
  const static U8 zzv[64]={
    0,0,1,2,1,0,0,1,2,3,4,3,2,1,0,0,1,2,3,4,5,6,5,4,3,2,1,0,0,1,2,3,
    4,5,6,7,7,6,5,4,3,2,1,2,3,4,5,6,7,7,6,5,4,3,4,5,6,7,7,6,5,6,7,7};

  // Be sure to quit on a byte boundary
  if (!bpos) next_jpeg=jpeg>1;
  if (bpos && !jpeg) return next_jpeg;
  if (!bpos && app>0) --app;
  if (app>0) return next_jpeg;
  if (!bpos) {

    // Parse.  Baseline DCT-Huffman JPEG syntax is:
    // SOI APPx... misc... SOF0 DHT... SOS data EOI
    // SOI (= FF D8) start of image.
    // APPx (= FF Ex) len ... where len is always a 2 byte big-endian length
    //   including the length itself but not the 2 byte preceding code.
    //   Application data is ignored.  There may be more than one APPx.
    // misc codes are DQT, DNL, DRI, COM (ignored).
    // SOF0 (= FF C0) len 08 height width Nf [C HV Tq]...
    //   where len, height, width (in pixels) are 2 bytes, Nf is the repeat
    //   count (1 byte) of [C HV Tq], where C is a component identifier
    //   (color, 0-3), HV is the horizontal and vertical dimensions
    //   of the MCU (high, low bits, packed), and Tq is the quantization
    //   table ID (not used).  An MCU (minimum compression unit) consists
    //   of 64*H*V DCT coefficients for each color.
    // DHT (= FF C4) len [TcTh L1...L16 V1,1..V1,L1 ... V16,1..V16,L16]...
    //   defines Huffman table Th (1-4) for Tc (0=DC (first coefficient)
    //   1=AC (next 63 coefficients)).  L1..L16 are the number of codes
    //   of length 1-16 (in ascending order) and Vx,y are the 8-bit values.
    //   A V code of RS means a run of R (0-15) zeros followed by S (0-15)
    //   additional bits to specify the next nonzero value, negative if
    //   the first additional bit is 0 (e.g. code x63 followed by the
    //   3 bits 1,0,1 specify 7 coefficients: 0, 0, 0, 0, 0, 0, 5.
    //   Code 00 means end of block (remainder of 63 AC coefficients is 0).
    // SOS (= FF DA) len Ns [Cs TdTa]... 0 3F 00
    //   Start of scan.  TdTa specifies DC/AC Huffman tables (0-3, packed
    //   into one byte) for component Cs matching C in SOF0, repeated
    //   Ns (1-4) times.
    // EOI (= FF D9) is end of image.
    // Huffman coded data is between SOI and EOI.  Codes may be embedded:
    // RST0-RST7 (= FF D0 to FF D7) mark the start of an independently
    //   compressed region.
    // DNL (= FF DC) 04 00 height
    //   might appear at the end of the scan (ignored).
    // FF 00 is interpreted as FF (to distinguish from RSTx, DNL, EOI).

    // Detect JPEG (SOI, APPx)
    if (!jpeg && buf(4)==FF && buf(3)==SOI && buf(2)==FF && buf(1)>>4==0xe) {
      jpeg=1;
      app=sos=sof=htsize=data=mcusize=linesize=0;
      huffcode=huffbits=huffsize=mcupos=cpos=0, rs=-1;
      memset(&huf[0], 0, huf.size()*sizeof(HUF));
      memset(&pred[0], 0, pred.size()*sizeof(int));
    }

    // Detect end of JPEG when data contains a marker other than RSTx
    // or byte stuff (00).
    if (jpeg && data && buf(2)==FF && buf(1) && (buf(1)&0xf8)!=RST0) {
      JASSERT(buf(1)==EOI);
      jpeg=0;
    }
    if (!jpeg) return next_jpeg;

    // Detect APPx or COM field
    if (!data && !app && buf(4)==FF && (buf(3)>>4==0xe || buf(3)==COM))
      app=buf(2)*256+buf(1)+2;

    // Save pointers to sof, ht, sos, data,
    if (buf(5)==FF && buf(4)==SOS) {
      int len=buf(3)*256+buf(2);
      if (len==6+2*buf(1) && buf(1) && buf(1)<=4)  // buf(1) is Ns
        sos=buf.pos-5, data=sos+len+2, jpeg=2;
    }
    if (buf(4)==FF && buf(3)==DHT && htsize<8) ht[htsize++]=buf.pos-4;
    if (buf(4)==FF && buf(3)==SOF0) sof=buf.pos-4;

    // Parse Quantizazion tables
    if (buf(4)==FF && buf(3)==DQT)
      dqt_end=buf.pos+buf(2)*256+buf(1)-1, dqt_state=0;
    else if (dqt_state>=0) {
      if (buf.pos>=dqt_end)
        dqt_state = -1;
      else {
        if (dqt_state%65==0)
          qnum = buf(1);
        else {
          JASSERT(buf(1)>0);
          JASSERT(qnum>=0 && qnum<4);
          qtab[qnum*64+((dqt_state%65)-1)]=buf(1)-1;
        }
        dqt_state++;
      }
    }

    // Restart
    if (buf(2)==FF && (buf(1)&0xf8)==RST0) {
      huffcode=huffbits=huffsize=mcupos=0, rs=-1;
      memset(&pred[0], 0, pred.size()*sizeof(int));
    }
  }

  {
    // Build Huffman tables
    // huf[Tc][Th][m] = min, max+1 codes of length m, pointer to byte values
    if (buf.pos==data && bpos==1) {
      JASSERT(htsize>0);
      int i;
      for ( i=0; i<htsize; ++i) {
        int p=ht[i]+4;  // pointer to current table after length field
        int end=p+buf[p-2]*256+buf[p-1]-2;  // end of Huffman table
        int count=0;  // sanity check
        while (p<end && end<buf.pos && end<p+2100 && ++count<10) {
          int tc=buf[p]>>4, th=buf[p]&15;
          if (tc>=2 || th>=4) break;
          JASSERT(tc>=0 && tc<2 && th>=0 && th<4);
          HUF* h=&huf[tc*64+th*16]; // [tc][th][0]; 
          int val=p+17;  // pointer to values
          int hval=tc*1024+th*256;  // pointer to RS values in hbuf
          int j;
          for ( j=0; j<256; ++j) // copy RS codes
            hbuf[hval+j]=buf[val+j];
          int code=0;
          for ( j=0; j<16; ++j) {
            h[j].min=code;
            h[j].max=code+=buf[p+j+1];
            h[j].val=hval;
            val+=buf[p+j+1];
            hval+=buf[p+j+1];
            code*=2;
          }
          p=val;
          JASSERT(hval>=0 && hval<2048);
        }
        JASSERT(p==end);
      }
      huffcode=huffbits=huffsize=0, rs=-1;

      // Build Huffman table selection table (indexed by mcupos).
      // Get image width.
      if (!sof && sos) return next_jpeg;
      int ns=buf[sos+4];
      int nf=buf[sof+9];
      JASSERT(ns<=4 && nf<=4);
      mcusize=0;  // blocks per MCU
      int hmax=0;  // MCU horizontal dimension
      for ( i=0; i<ns; ++i) {
        for (int j=0; j<nf; ++j) {
          if (buf[sos+2*i+5]==buf[sof+3*j+10]) { // Cs == C ?
            int hv=buf[sof+3*j+11];  // packed dimensions H x V
            if (hv>>4>hmax) hmax=hv>>4;
            hv=(hv&15)*(hv>>4);  // number of blocks in component C
            JASSERT(hv>=1 && hv+mcusize<=10);
            while (hv) {
              JASSERT(mcusize<10);
              hufsel[0][mcusize]=buf[sos+2*i+6]>>4&15;
              hufsel[1][mcusize]=buf[sos+2*i+6]&15;
              JASSERT(hufsel[0][mcusize]<4 && hufsel[1][mcusize]<4);
              color[mcusize]=i;
              int tq=buf[sof+3*j+12];  // quantization table index (0..3)
              JASSERT(tq>=0 && tq<4);
              qmap[mcusize]=tq; // quantizazion table mapping
              --hv;
              ++mcusize;
            }
          }
        }
      }
      JASSERT(hmax>=1 && hmax<=10);
      int j;
      for ( j=0; j<mcusize; ++j) {
        ls[j]=0;
        for (int i=1; i<mcusize; ++i) if (color[(j+i)%mcusize]==color[j]) ls[j]=i;
        ls[j]=mcusize-ls[j]<<6;
      }
      for ( j=0; j<64; ++j) zpos[zzu[j]+8*zzv[j]]=j;
      width=buf[sof+7]*256+buf[sof+8];  // in pixels
      int height=buf[sof+5]*256+buf[sof+6];
      PRINTF("JPEG %dx%d ", width, height);
      width=(width-1)/(hmax*8)+1;  // in MCU
      JASSERT(width>0);
      mcusize*=64;  // coefficients per MCU
      row=column=0;
    }
  }


  // Decode Huffman
  {
    if (mcusize && buf(1+(!bpos))!=FF) {  // skip stuffed byte
      JASSERT(huffbits<=32);
      huffcode+=huffcode+y;
      ++huffbits;
      if (rs<0) {
        JASSERT(huffbits>=1 && huffbits<=16);
        const int ac=(mcupos&63)>0;
        JASSERT(mcupos>=0 && (mcupos>>6)<10);
        JASSERT(ac==0 || ac==1);
        const int sel=hufsel[ac][mcupos>>6];
        JASSERT(sel>=0 && sel<4);
        const int i=huffbits-1;
        JASSERT(i>=0 && i<16);
        const HUF *h=&huf[ac*64+sel*16]; // [ac][sel];
        JASSERT(h[i].min<=h[i].max && h[i].val<2048 && huffbits>0);
        if (huffcode<h[i].max) {
          JASSERT(huffcode>=h[i].min);
          int k=h[i].val+huffcode-h[i].min;
          JASSERT(k>=0 && k<2048);
          rs=hbuf[k];
          huffsize=huffbits;
        }
      }
      if (rs>=0) {
        if (huffsize+(rs&15)==huffbits) { // done decoding
          huff4=huff3;
          huff3=huff2;
          huff2=huff1;
          huff1=hash(huffcode, huffbits);
          rs4=rs3;
          rs3=rs2;
          rs2=rs1;
          rs1=rs;
          int x=0;  // decoded extra bits
          if (mcupos&63) {  // AC
            if (rs==0) { // EOB
              mcupos=mcupos+63&-64;
              JASSERT(mcupos>=0 && mcupos<=mcusize && mcupos<=640);
              while (cpos&63) {
                cbuf2[cpos]=0;
                cbuf[cpos++]=0;
              }
            }
            else {  // rs = r zeros + s extra bits for the next nonzero value
                    // If first extra bit is 0 then value is negative.
              JASSERT((rs&15)<=10);
              const int r=rs>>4;
              const int s=rs&15;
              JASSERT(mcupos>>6==mcupos+r>>6);
              mcupos+=r+1;
              x=huffcode&(1<<s)-1;
              if (s && !(x>>s-1)) x-=(1<<s)-1;
              for (int i=r; i>=1; --i) {
                cbuf2[cpos]=0;
                cbuf[cpos++]=i<<4|s;
              }
              cbuf2[cpos]=x;
              cbuf[cpos++]=s<<4|huffcode<<2>>s&3|12;
              ssum+=s;
            }
          }
          else {  // DC: rs = 0S, s<12
            JASSERT(rs<12);
            ++mcupos;
            x=huffcode&(1<<rs)-1;
            if (rs && !(x>>rs-1)) x-=(1<<rs)-1;
            JASSERT(mcupos>=0 && mcupos>>6<10);
            const int comp=color[mcupos>>6];
            JASSERT(comp>=0 && comp<4);
            dc=pred[comp]+=x;
            JASSERT((cpos&63)==0);
            cbuf2[cpos]=dc;
            cbuf[cpos++]=dc+1023>>3;
            ssum4=ssum3;
            ssum3=ssum2;
            ssum2=ssum1;
            ssum1=ssum;
            ssum=rs;
          }
          JASSERT(mcupos>=0 && mcupos<=mcusize);
          if (mcupos>=mcusize) {
            mcupos=0;
            if (++column==width) column=0, ++row;
          }
          huffcode=huffsize=huffbits=0, rs=-1;


          // UPDATE_ADV_PRED !!!!
          {
            const int acomp=mcupos>>6, q=64*qmap[acomp];
            const int zz=mcupos&63, cpos_dc=cpos-zz;
            const static int we[8]={181, 282, 353, 456, 568, 671, 742, 767};
            if (zz == 0) {
              for (int i=0; i<8; i++) {
                update_k(sumv2[i], sumv3[i], kx[i], kx[i+16]);
                update_k(sumu2[i], sumu3[i], kx[i+8], kx[i+24]);
                sumu2[i]=sumv2[i]=sumu3[i]=sumv3[i]=0;
              }
              int cpos_dc_ls_acomp = cpos_dc-ls[acomp];
              int cpos_dc_mcusize_width = cpos_dc-mcusize*width;
              for (int i=0; i<64; i++) {
                sumu2[zzu[i]]+=we[zzv[i]]*(zzv[i]&1?-1:+1)*(qtab[q+i]+1)*cbuf2[cpos_dc_mcusize_width+i];
                sumv2[zzv[i]]+=we[zzu[i]]*(zzu[i]&1?-1:+1)*(qtab[q+i]+1)*cbuf2[cpos_dc_ls_acomp+i];
                sumu3[zzu[i]]+=(zzv[i]?(zzv[i]&1?-256:256):181)*(qtab[q+i]+1)*cbuf2[cpos_dc_mcusize_width+i];
                sumv3[zzv[i]]+=(zzu[i]?(zzu[i]&1?-256:256):181)*(qtab[q+i]+1)*cbuf2[cpos_dc_ls_acomp+i];
              }
            } else {
              sumu2[zzu[zz-1]]-=we[zzv[zz-1]]*(qtab[q+zz-1]+1)*cbuf2[cpos-1];
              sumv2[zzv[zz-1]]-=we[zzu[zz-1]]*(qtab[q+zz-1]+1)*cbuf2[cpos-1];
              sumu3[zzu[zz-1]]-=(zzv[zz-1]?256:181)*(qtab[q+zz-1]+1)*cbuf2[cpos-1];
              sumv3[zzv[zz-1]]-=(zzu[zz-1]?256:181)*(qtab[q+zz-1]+1)*cbuf2[cpos-1];
            }
            for (int i=0; i<8; ++i) {
              int k=kx[i];
              sumv[i]=(sumv2[i]*k+sumv3[i]*(8-k))/8;
              k=kx[i+8];
              sumu[i]=(sumu2[i]*k+sumu3[i]*(8-k))/8;
            }

            for (int i=0; i<3; ++i)
              for (int st=0; st<8; ++st) {
                const int zz2 = min(zz+st, 63);
                int p=(sumu[zzu[zz2]]*i+sumv[zzv[zz2]]*(2-i))/2;
                p/=(qtab[q+zz2]+1)*181;
                if (zz2==0) p-=cbuf2[cpos_dc-ls[acomp]], p=(p<0?-1:+1)*CLog::ilog(14*abs(p)+1)/10;
                else p=(p<0?-1:+1)*CLog::ilog(10*abs(p)+1)/10;
                if (st==0) {
                  adv_pred[i]=p;
                  adv_pred[i+4]=p/4;
                }
                else if (abs(p)>abs(adv_pred[i])+1) {
                  adv_pred[i]+=st*2+(p>0)<<6;
                  if (abs(p/4)>abs(adv_pred[i+4])+1) adv_pred[i+4]+=st*2+(p>0)<<6;
                  break;
                }
              }
            x=2*sumu[zzu[zz]]+2*sumv[zzv[zz]];
            for (int i=0; i<8; ++i) {
              if (zzu[zz]<i) x-=sumu[i];
              if (zzv[zz]<i) x-=sumv[i];
            }
            x/=(qtab[q+zz]+1)*181;
            if (zz==0) x-=cbuf2[cpos_dc-ls[acomp]];
            adv_pred[3]=(x<0?-1:+1)*CLog::ilog(10*abs(x)+1)/10;

            for (int i=0; i<4; ++i) {
              const int a=(i&1?zzv[zz]:zzu[zz]), b=(i&2?2:1);
              if (a<b) x=255;
              else {
                const int zz2=zpos[zzu[zz]+8*zzv[zz]-(i&1?8:1)*b];
                x=(qtab[q+zz2]+1)*cbuf2[cpos_dc+zz2]/(qtab[q+zz]+1);
                x=(x<0?-1:+1)*CLog::ilog(8*abs(x)+1)/8;
              }
              lcp[i]=x;
            }
            if (column==0) adv_pred[1]=adv_pred[2], adv_pred[0]=1;
            if (row==0) adv_pred[1]=adv_pred[0], adv_pred[2]=1;
          } // !!!!

        }
      }
    }
  }

  // Estimate next bit probability
  if (!jpeg || !data) return next_jpeg;
  if (buf(1+(!bpos))==FF) {
    m.add(128);
    return 1;
  }

  // Update model
  if (cp[N-1]) {
    for (int i=0; i<N; ++i)
      *cp[i]=nex(*cp[i],y);
  }
  m1.update();

  // Update context
  const int comp=color[mcupos>>6];
  const int coef=(mcupos&63)|comp<<6;
  const int hc=(huffcode*2+(comp==0))|1<<(huffbits+1);
  if (++hbcount>2 || huffbits==0) hbcount=0;
  JASSERT(coef>=0 && coef<256);
  const int zu=zzu[mcupos&63], zv=zzv[mcupos&63];
  if (hbcount==0) {
    int n=0;
    cxt[0]=hash(++n, hc, coef, adv_pred[2]);
    cxt[1]=hash(++n, hc, coef, adv_pred[0]);
    cxt[2]=hash(++n, hc, coef, adv_pred[1]);
    cxt[3]=hash(++n, hc, rs1, adv_pred[2]);
    cxt[4]=hash(++n, hc, rs1, adv_pred[0]);
    cxt[5]=hash(++n, hc, rs1, adv_pred[1]);
    cxt[6]=hash(++n, hc, adv_pred[2], adv_pred[0]);
    cxt[7]=hash(++n, hc, cbuf[cpos-width*mcusize], adv_pred[3]);
    cxt[8]=hash(++n, hc, cbuf[cpos-ls[mcupos>>6]], adv_pred[3]);
    cxt[9]=hash(++n, hc, lcp[0], lcp[1], adv_pred[1]);
    cxt[10]=hash(++n, hc, lcp[0], lcp[1], coef);
    cxt[11]=hash(++n, hc, zu, lcp[0], lcp[2]/3);
    cxt[12]=hash(++n, hc, zv, lcp[1], lcp[3]/3);
    cxt[13]=hash(++n, hc, mcupos>>2, min(3, mcupos&63));
    cxt[14]=hash(++n, hc, coef, column>>1);
    cxt[15]=hash(++n, hc, column>>2, lcp[0]+256*(lcp[2]/3), lcp[1]+256*(lcp[3]/3));
    cxt[16]=hash(++n, hc, ssum>>4, coef);
    cxt[17]=hash(++n, hc, rs1, coef);
    cxt[18]=hash(++n, hc, mcupos>>3, ssum3>>3, adv_pred[3]);
    cxt[19]=hash(++n, hc, lcp[0]/3, lcp[1]/3, adv_pred[5]);
    cxt[20]=hash(++n, hc, cbuf[cpos-width*mcusize], adv_pred[6]);
    cxt[21]=hash(++n, hc, cbuf[cpos-ls[mcupos>>6]], adv_pred[4]);
    cxt[22]=hash(++n, hc, adv_pred[2]);
    cxt[23]=hash(n, hc, adv_pred[0]);
    cxt[24]=hash(n, hc, adv_pred[1]);
    cxt[25]=hash(++n, hc, zv, lcp[1], adv_pred[6]);
    cxt[26]=hash(++n, hc, zu, lcp[0], adv_pred[4]);
    cxt[27]=hash(++n, hc, lcp[0], lcp[1], adv_pred[3]);
  }

  // Predict next bit
  m1.add(128);
  ASSERT(hbcount<=2);
 switch(hbcount)
  {
   case 0: for (int i=0; i<N; ++i) cp[i]=t[cxt[i]]+1, m1.add(Stretch::s(sm[i].p(y,*cp[i]))); break;
   case 1: { int hc=1+(huffcode&1)*3; for (int i=0; i<N; ++i) cp[i]+=hc, m1.add(Stretch::s(sm[i].p(y,*cp[i]))); } break;
   default: { int hc=1+(huffcode&1); for (int i=0; i<N; ++i) cp[i]+=hc, m1.add(Stretch::s(sm[i].p(y,*cp[i]))); } break;
  }

  m1.set(column==0, 2);
  m1.set(coef, 256);
  m1.set(hc&511, 512);
  int pr=m1.p();
  m.add(Stretch::s(pr));
  pr=a1.p(y, pr, hc&511|(adv_pred[1]&63)<<9, 1023);
  pr=a2.p(y, pr, hc&255|coef<<8, 255);
  m.add(Stretch::s(pr));
  return 2+(hc&255);
}

};