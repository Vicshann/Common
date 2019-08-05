
/////////////////////////// ContextMap /////////////////////////
//
// A ContextMap maps contexts to a bit histories and makes predictions
// to a Mixer.  Methods common to all classes:
//
// ContextMap cm(M, C); creates using about M bytes of memory (a power
//   of 2) for C contexts.
// cm.set(cx);  sets the next context to cx, called up to C times
//   cx is an arbitrary 32 bit value that identifies the context.
//   It should be called before predicting the first bit of each byte.
// cm.mix(m) updates Mixer m with the next prediction.  Returns 1
//   if context cx is found, else 0.  Then it extends all the contexts with
//   global bit y.  It should be called for every bit:
//
//     if (bpos==0) 
//       for (int i=0; i<C; ++i) cm.set(cxt[i]);
//     cm.mix(m);
//
// The different types are as follows:
//
// - RunContextMap.  The bit history is a count of 0-255 consecutive
//     zeros or ones.  Uses 4 bytes per whole byte context.  C=1.
//     The context should be a hash.
// - SmallStationaryContextMap.  0 <= cx < M/512.
//     The state is a 16-bit probability that is adjusted after each
//     prediction.  C=1.
// - ContextMap.  For large contexts, C >= 1.  Context need not be hashed.

// Predict to mixer m from bit history state s, using sm to map s to
// a probability.
static inline int mix2(Mixer& m, int s, StateMap& sm) {
  int p1=sm.p(m.y, s);
  int n0=-!nex(s,2);
  int n1=-!nex(s,3);
  int st=Stretch::s(p1)>>2;
  m.add(st);
  p1>>=4;
  int p0=255-p1;
  m.add(p1-p0);
  m.add(st*(n1-n0));
  m.add((p1&n0)-(p0&n1));
  m.add((p1&n1)-(p0&n0));
  return s>0;
}

// A RunContextMap maps a context into the next byte and a repeat
// count up to M.  Size should be a power of 2.  Memory usage is 3M/4.
class RunContextMap 
{
// Context (Must come first)
 int& c0;
 int& bpos;
 Buf& buf;
//-------------
  BH<4> t;
  U8* cp;

public:
  RunContextMap(int m, int& _c0, int& _bpos, Buf& _buf): t(m/4), c0(_c0), bpos(_bpos), buf(_buf) {cp=t[0]+1;}
  void set(U32 cx) {  // update count
    if (cp[0]==0 || cp[1]!=buf(1)) cp[0]=1, cp[1]=buf(1);
    else if (cp[0]<255) ++cp[0];
    cp=t[cx]+1;
  }
  int p() {  // predict next bit
    if (cp[1]+256>>8-bpos==c0)
      return ((cp[1]>>7-bpos&1)*2-1)*CLog::ilog(cp[0]+1)*8;
    else
      return 0;
  }
  int mix(Mixer& m) {  // return run length
    m.add(p());
    return cp[0]!=0;
  }
};

// Context is looked up directly.  m=size is power of 2 in bytes.
// Context should be < m/512.  High bits are discarded.
class SmallStationaryContextMap 
{
  Array<U16> t;
  int cxt;
  U16 *cp;
public:
  SmallStationaryContextMap(int m): t(m/2), cxt(0) {
    ASSERT((m/2&m/2-1)==0); // power of 2?
    for (int i=0; i<t.size(); ++i)
      t[i]=32768;
    cp=&t[0];
  }
  void set(U32 cx) {
    cxt=cx*256&t.size()-256;
  }
  void mix(Mixer& m, int rate=7) {
    *cp += (m.y<<16)-*cp+(1<<rate-1) >> rate;
    cp=&t[cxt+m.c0];
    m.add(Stretch::s(*cp>>4));
  }
};

// Context map for large contexts.  Most modeling uses this type of context
// map.  It includes a built in RunContextMap to predict the last byte seen
// in the same context, and also bit-level contexts that map to a bit
// history state.
//
// Bit histories are stored in a hash table.  The table is organized into
// 64-byte buckets alinged on cache page boundaries.  Each bucket contains
// a hash chain of 7 elements, plus a 2 element queue (packed into 1 byte) 
// of the last 2 elements accessed for LRU replacement.  Each element has
// a 2 byte checksum for detecting collisions, and an array of 7 bit history
// states indexed by the last 0 to 2 bits of context.  The buckets are indexed
// by a context ending after 0, 2, or 5 bits of the current byte.  Thus, each
// byte modeled results in 3 main memory accesses per context, with all other
// accesses to cache.
//
// On bits 0, 2 and 5, the context is updated and a new bucket is selected.
// The most recently accessed element is tried first, by comparing the
// 16 bit checksum, then the 7 elements are searched linearly.  If no match
// is found, then the element with the lowest priority among the 5 elements 
// not in the LRU queue is replaced.  After a replacement, the queue is
// emptied (so that consecutive misses favor a LFU replacement policy).
// In all cases, the found/replaced element is put in the front of the queue.
//
// The priority is the state number of the first element (the one with 0
// additional bits of context).  The states are sorted by increasing n0+n1
// (number of bits seen), implementing a LFU replacement policy.
//
// When the context ends on a byte boundary (bit 0), only 3 of the 7 bit
// history states are used.  The remaining 4 bytes implement a run model
// as follows: <count:7,d:1> <b1> <unused> <unused> where <b1> is the last byte
// seen, possibly repeated.  <count:7,d:1> is a 7 bit count and a 1 bit
// flag (represented by count * 2 + d).  If d=0 then <count> = 1..127 is the 
// number of repeats of <b1> and no other bytes have been seen.  If d is 1 then 
// other byte values have been seen in this context prior to the last <count> 
// copies of <b1>.
//
// As an optimization, the last two hash elements of each byte (representing
// contexts with 2-7 bits) are not updated until a context is seen for
// a second time.  This is indicated by <count,d> = <1,0> (2).  After update,
// <count,d> is updated to <2,0> or <1,1> (4 or 3).

class ContextMap 
{
  const int C;  // max number of contexts
  class E {  // hash element, 64 bytes
    U16 chk[7];  // byte context checksums
    U8 last;     // last 2 accesses (0-6) in low, high nibble
  public:
    U8 bh[7][7]; // byte context, 3-bit context -> bit history state
      // bh[][0] = 1st bit, bh[][1,2] = 2nd bit, bh[][3..6] = 3rd bit
      // bh[][0] is also a replacement priority, 0 = empty
    U8* get(U16 ch)  // Find element (0-6) matching checksum.   // Find or create hash element matching checksum ch
{
  if (chk[last&15]==ch) return &bh[last&15][0];
  int b=0xffff, bi=0;
  for (int i=0; i<7; ++i) {
    if (chk[i]==ch) return last=last<<4|i, (U8*)&bh[i][0];
    int pri=bh[i][0];
    if (pri<b && (last&15)!=i && last>>4!=i) b=pri, bi=i;
  }
  return last=0xf0|bi, chk[bi]=ch, (U8*)memset(&bh[bi][0], 0, 7);
}
      // If not found, insert or replace lowest priority (not last).
  };
  Array<E, 64> t;  // bit histories for bits 0-1, 2-4, 5-7
    // For 0-1, also contains a run count in bh[][4] and value in bh[][5]
    // and pending update count in bh[7]
  Array<U8*> cp;   // C pointers to current bit history
  Array<U8*> cp0;  // First element of 7 element array containing cp[i]
  Array<U32> cxt;  // C whole byte contexts (hashes)
  Array<U8*> runp; // C [0..3] = count, value, unused, unused
  StateMap *sm;    // C maps of state -> p
  int cn;          // Next context to set by set()
//////////////////  void update(U32 cx, int c);  // train model that context cx predicts c
// Update the model with bit y1, and predict next bit to mixer m.
// Context: cc=c0, bp=bpos, c1=buf(1), y1=y.
int mix1(Mixer& m, int cc, int bp, int c1, int y1) 
{
  // Update model with y
  int result=0;
  for (int i=0; i<cn; ++i) {
    if (cp[i]) {
      ASSERT(cp[i]>=&t[0].bh[0][0] && cp[i]<=&t[t.size()-1].bh[6][6]);
      ASSERT((long(cp[i])&63)>=15);
      int ns=nex(*cp[i], y1);
      if (ns>=204 && m.rnd() << (452-ns>>3)) ns-=4;  // probabilistic increment
      *cp[i]=ns;
    }

    // Update context pointers
    if (m.bpos>1 && runp[i][0]==0)
    {
     cp[i]=0;
    }
    else
    {
     switch(m.bpos)
     {
      case 1: case 3: case 6: cp[i]=cp0[i]+1+(cc&1); break;
      case 4: case 7: cp[i]=cp0[i]+3+(cc&3); break;
      case 2: case 5: cp0[i]=cp[i]=t[cxt[i]+cc&t.size()-1].get(cxt[i]>>16); break;
      default:
      {
       cp0[i]=cp[i]=t[cxt[i]+cc&t.size()-1].get(cxt[i]>>16);
       // Update pending bit histories for bits 2-7
       if (cp0[i][3]==2) {
         const int c=cp0[i][4]+256;
         U8 *p=t[cxt[i]+(c>>6)&t.size()-1].get(cxt[i]>>16);
         p[0]=1+((c>>5)&1);
         p[1+((c>>5)&1)]=1+((c>>4)&1);
         p[3+((c>>4)&3)]=1+((c>>3)&1);
         p=t[cxt[i]+(c>>3)&t.size()-1].get(cxt[i]>>16);
         p[0]=1+((c>>2)&1);
         p[1+((c>>2)&1)]=1+((c>>1)&1);
         p[3+((c>>1)&3)]=1+(c&1);
         cp0[i][6]=0;
       }
       // Update run count of previous context
       if (runp[i][0]==0)  // new context
         runp[i][0]=2, runp[i][1]=c1;
       else if (runp[i][1]!=c1)  // different byte in context
         runp[i][0]=1, runp[i][1]=c1;
       else if (runp[i][0]<254)  // same byte in context
         runp[i][0]+=2;
       else if (runp[i][0]==255)
         runp[i][0]=128;
       runp[i]=cp0[i]+3;
      } break;
     }
    }

    // predict from last byte in context
    if (runp[i][1]+256>>8-bp==cc) {
      int rc=runp[i][0];  // count*2, +1 if 2 different bytes seen
      int b=(runp[i][1]>>7-bp&1)*2-1;  // predicted bit + for 1, - for 0
      int c=CLog::ilog(rc+1)<<2+(~rc&1);
      m.add(b*c);
    }
    else
      m.add(0);

    // predict from bit context
   if(cp[i])
   {
    result+=mix2(m, *cp[i], sm[i]);
   }
   else
   {
    mix2(m, 0, sm[i]);
   }


  }
  if (bp==7) cn=0;
  return result;
}

    // mix() with global context passed as arguments to improve speed.
public:

// Construct using m bytes of memory for c contexts
 ContextMap(int m, int c=1): C(c), t(m>>6), cp(c), cp0(c), cxt(c), runp(c), cn(0) {    // m = memory in bytes, a power of 2, C = c
  ASSERT(m>=64 && (m&m-1)==0);  // power of 2?
  ASSERT(sizeof(E)==64);
  sm=new StateMap[C];
  for (int i=0; i<C; ++i) {
    cp0[i]=cp[i]=&t[0].bh[0][0];
    runp[i]=cp[i]+3;
  }
}
  ~ContextMap(){delete[] sm;}
  void set(U32 cx, int next=-1)   // set next whole byte context to cx   // Set the i'th context to cx
{
  int i=cn++;
  i&=next;
  ASSERT(i>=0 && i<C);
  cx=cx*987654323+i;  // permute (don't hash) cx to spread the distribution
  cx=cx<<16|cx>>16;
  cxt[i]=cx*123456791+i;
}
    // if next is 0 then set order does not matter
  int mix(Mixer& m) {return mix1(m, m.c0, m.bpos, m.buf(1), m.y);}
};

