
//////////////////////////// Mixer /////////////////////////////
// All of the models take a Mixer as a parameter and write predictions to it.
// Mixer m(N, M, S=1, w=0) combines models using M neural networks with
//   N inputs each, of which up to S may be selected.  If S > 1 then
//   the outputs of these neural networks are combined using another
//   neural network (with parameters S, 1, 1).  If S = 1 then the
//   output is direct.  The weights are initially w (+-32K).
//   It is used as follows:
// m.update() trains the network where the expected output is the
//   last bit (in the global variable y).
// m.add(stretch(p)) inputs prediction from one of N models.  The
//   prediction should be positive to predict a 1 bit, negative for 0,
//   nominally +-256 to +-2K.  The maximum allowed value is +-32K but
//   using such large values may cause overflow if N is large.
// m.set(cxt, range) selects cxt as one of 'range' neural networks to
//   use.  0 <= cxt < range.  Should be called up to S times such
//   that the total of the ranges is <= M.
// m.p() returns the output prediction that the next bit is 1 as a
//   12 bit number (0 to 4095).

class Mixer 
{
  const int N, M, S;   // max inputs, max contexts, max context sets
  Array<short, 16> tx; // N inputs from add()
  Array<short, 16> wx; // N*M weights
  Array<int> cxt;  // S contexts
  int ncxt;        // number of contexts (0 to S)
  int base;        // offset of next context
  int nx;          // Number of inputs in tx, 0 to N
  Array<int> pr;   // last result (scaled 12 bits)
  Mixer* mp;       // points to a Mixer to combine results

// dot_product returns dot product t*w of n elements.  n is rounded
// up to a multiple of 8.  Result is scaled down by 8 bits.
//#ifdef NOASM  // no assembly language
 int dot_product(short *t, short *w, int n) {
  int sum=0;
  n=(n+7)&-8;
  for (int i=0; i<n; i+=2)
    sum+=(t[i]*w[i]+t[i+1]*w[i+1]) >> 8;
  return sum;
}
//#else  // The NASM version uses MMX and is about 8 times faster.
//extern "C" int dot_product(short *t, short *w, int n);  // in NASM
//#endif

// Train neural network weights w[n] given inputs t[n] and err.
// w[i] += t[i]*err, i=0..n-1.  t, w, err are signed 16 bits (+- 32K).
// err is scaled 16 bits (representing +- 1/2).  w[i] is clamped to +- 32K
// and rounded.  n is rounded up to a multiple of 8.
//#ifdef NOASM
 void train(short *t, short *w, int n, int err) {
  n=(n+7)&-8;
  for (int i=0; i<n; ++i) {
    int wt=w[i]+((t[i]*err*2>>16)+1>>1);
    if (wt<-32768) wt=-32768;
    if (wt>32767) wt=32767;
    w[i]=wt;
  }
}
//#else
//extern "C" void train(short *t, short *w, int n, int err);  // in NASM
//#endif


public:
// Context
 int& y;
 int& c0;
 int& c4;
 int& bpos;
 Buf& buf;
 Random& rnd;
 const U32 MEM;

 Mixer(U32 _Mem, Random& _rnd, Buf& _buf, int& _y, int& _c0, int& _c4, int& _bpos, int n, int m, int s=1, int w=0): N((n+7)&-8), M(m), S(s), tx(N), wx(N*M), cxt(S), ncxt(0), base(0), nx(0), pr(S), mp(0),
                                                                                                 rnd(_rnd), buf(_buf), y(_y), c0(_c0), c4(_c4), bpos(_bpos), MEM(_Mem)
{
  ASSERT(n>0 && N>0 && (N&7)==0 && M>0);
  int i;
  for ( i=0; i<S; ++i)
    pr[i]=2048;
  for ( i=0; i<N*M; ++i)
    wx[i]=w;
  if (S>1) mp=new Mixer(_Mem, _rnd, _buf, _y, _c0, _c4, _bpos, S, 1, 1, 0x7fff);
}

  // Adjust weights to minimize coding cost of last prediction
  void update(void) 
{
    for (int i=0; i<ncxt; ++i) {
      int err=((y<<12)-pr[i])*7;
      ASSERT(err>=-32768 && err<32768);
      if (err) train(&tx[0], &wx[cxt[i]*N], nx, err);
    }
    nx=base=ncxt=0;
}

  // Input x (call up to N times)
  void add(int x) {
    ASSERT(nx<N);
    tx[nx++]=x;
  }

  // Set a context (call S times, sum of ranges <= M)
  void set(int cx, int range) {
    ASSERT(range>=0);
    ASSERT(ncxt<S);
    ASSERT(cx>=0);
    ASSERT(base+cx<M);
    cxt[ncxt++]=base+cx;
    base+=range;
  }

  // predict next bit
  int p(void) 
{
    while (nx&7) tx[nx++]=0;  // pad
    if (mp) {  // combine outputs
      mp->update();
      for (int i=0; i<ncxt; ++i) {
        pr[i]=squash(dot_product(&tx[0], &wx[cxt[i]*N], nx)>>5);
        mp->add(Stretch::s(pr[i]));
      }
      mp->set(0, 1);
      return mp->p();
    }
    else {  // S=1 context
      return pr[0]=squash(dot_product(&tx[0], &wx[0], nx)>>8);
    }
  }
~Mixer() {
  delete mp;
}

};

//////////////////////////// APM1 //////////////////////////////
// APM1 maps a probability and a context into a new probability
// that bit y will next be 1.  After each guess it updates
// its state to improve future guesses.  Methods:
//
// APM1 a(N) creates with N contexts, uses 66*N bytes memory.
// a.p(pr, cx, rate=7) returned adjusted probability in context cx (0 to
//   N-1).  rate determines the learning rate (smaller = faster, default 7).
//   Probabilities are scaled 12 bits (0-4095).

class APM1 {
  int index;     // last p, context
  const int N;   // number of contexts
  Array<U16> t;  // [N][33]:  p, context -> p
public:
// maps p, cxt -> p initially
APM1(int n): index(0), N(n), t(n*33) 
{
  for (int i=0; i<N; ++i)
    for (int j=0; j<33; ++j)
      t[i*33+j] = i==0 ? squash((j-16)*128)*16 : t[j];
}
  int p(int y, int pr=2048, int cxt=0, int rate=7)  
{
    ASSERT(pr>=0 && pr<4096 && cxt>=0 && cxt<N && rate>0 && rate<32);
    pr=Stretch::s(pr);
    int g=(y<<16)+(y<<rate)-y-y;
    t[index] += g-t[index] >> rate;
    t[index+1] += g-t[index+1] >> rate;
    const int w=pr&127;  // interpolation weight (33 points)
    index=(pr+2048>>7)+cxt*33;
    return t[index]*(128-w)+t[index+1]*w >> 11;
  }
};


//////////////////////////// StateMap, APM //////////////////////////
// A StateMap maps a context to a probability.  Methods:
//
// Statemap sm(n) creates a StateMap with n contexts using 4*n bytes memory.
// sm.p(y, cx, limit) converts state cx (0..n-1) to a probability (0..4095).
//     that the next y=1, updating the previous prediction with y (0..1).
//     limit (1..1023, default 1023) is the maximum count for computing a
//     prediction.  Larger values are better for stationary sources.

class StateMap 
{
static inline int dt[1024];  // i -> 16K/(i+3)

protected:
  const int N;  // Number of contexts
  int cxt;      // Context of last prediction
  Array<U32> t;       // cxt -> prediction in high 22 bits, count in low 10 bits
  inline void update(int limit, int y) 
{
    ASSERT(cxt>=0 && cxt<N);
    U32 *p=&t[cxt], p0=p[0];
    int n=p0&1023, pr=p0>>10;  // count, prediction
    if (n<limit) ++p0;
    else p0=p0&0xfffffc00|limit;;
    p0+=(((y<<22)-pr)>>3)*dt[n]&0xfffffc00;
    p[0]=p0;
  }

public:
StateMap(int n=256): N(n), cxt(0), t(n) 
{
  for (int i=0; i<N; ++i)t[i]=1<<31;
  if(dt[0])return;
  for (int i=0; i<1024; ++i)dt[i]=16384/(i+i+3); 
}

  // update bit y (0..1), predict next bit in context cx
  int p(int y, int cx, int limit=1023) {   
    ASSERT(cx>=0 && cx<N);
    ASSERT(limit>0 && limit<1024);
    update(limit, y);
    return t[cxt=cx]>>20;
  }
};

// An APM maps a probability and a context to a new probability.  Methods:
//
// APM a(n) creates with n contexts using 96*n bytes memory.
// a.pp(y, pr, cx, limit) updates and returns a new probability (0..4095)
//     like with StateMap.  pr (0..4095) is considered part of the context.
//     The output is computed by interpolating pr into 24 ranges nonlinearly
//     with smaller ranges near the ends.  The initial output is pr.
//     y=(0..1) is the last bit.  cx=(0..n-1) is the other context.
//     limit=(0..1023) defaults to 255.

class APM: public StateMap {
public:
APM(int n): StateMap(n*24) 
{
  for (int i=0; i<N; ++i) {
    int p=((i%24*2+1)*4096)/48-2048;
    t[i]=(U32(squash(p))<<20)+6;
  }
}
  int p(int y, int pr, int cx, int limit=255) {   
   // ASSERT(y>>1==0);
    ASSERT(pr>=0 && pr<4096);
    ASSERT(cx>=0 && cx<N/24);
    ASSERT(limit>0 && limit<1024);
    update(limit, y);
    pr=(Stretch::s(pr)+2048)*23;
    int wt=pr&0xfff;  // interpolation weight of next element
    cx=cx*24+(pr>>12);
    ASSERT(cx>=0 && cx<N-1);
    cxt=cx+(wt>>11);
	pr=(t[cx]>>13)*(0x1000-wt)+(t[cx+1]>>13)*wt>>19;
    return pr;
  }
};



