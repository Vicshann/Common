
#define ASSERT(x)
#define PRINTF(...)


#define CALCMEM(lvl) (0x10000<<lvl)

#ifndef SEEK_SET
#define SEEK_SET        0               /* seek to an absolute position */
#define SEEK_CUR        1               /* seek relative to current position */
#define SEEK_END        2               /* seek relative to end of file */
#endif  /* ifndef SEEK_SET */


// 8, 16, 32 bit unsigned types (adjust as appropriate)
typedef unsigned char  U8;
typedef unsigned short U16;
typedef unsigned int   U32;

// This two std functions are invisible for some reason
static inline int _fastcall abs(int n){return n < 0 ? -n : n;}
static inline int _fastcall atoi(const char *str)
{
 int sign = 1, base = 0, i = 0;  
 if(!str)return 0;   
 while(str[i] == 0x20)i++;   // Skip whitespaces at begin               
 if (str[i] == '-' || str[i] == '+')sign = 1 - 2 * (str[i++] == '-');  // sign of number         
 while (str[i] >= '0' && str[i] <= '9')  // checking for valid input 
  {       
   if(base > MAXINT / 10 || (base == MAXINT / 10 && str[i] - '0' > 7))return (sign == 1)?(MAXINT):(MININT);    // handling overflow test case  
   base = 10 * base + (str[i++] - '0'); 
  } 
 return base * sign; 
}


// min, max functions
//#ifndef WINDOWS
//inline int min(int a, int b) {return a<b?a:b;}
//inline int max(int a, int b) {return a<b?b:a;}
//#endif

enum EMode {COMPRESS, DECOMPRESS};
enum EFiletype {DEFAULT, JPEG, BMPFILE1, BMPFILE4, BMPFILE8, BMPFILE24, TIFFFILE, PGMFILE, RGBFILE, EXE, TEXT, WAVFILE, MRBFILE, TXTUTF8};

class STRM
{

public:
static const int PEOF      = -1;
//static const int SEEK_SET = 0;
//static const int SEEK_CUR = 1;
//static const int SEEK_END = 2;

 STRM(void){}
 ~STRM(){}
//-----------
virtual int  getc(void){return 0;}
virtual int  putc(int Character){return 0;}
virtual int  fseek(unsigned long Offset, int Origin){return 0;}
virtual unsigned long fsize(void){return 0;}
virtual unsigned long ftell(void){return 0;}
virtual unsigned long fwrite(void* Buffer, unsigned long ElementSize, unsigned long ElementCount){return 0;}    // Into the stream
virtual unsigned long fread(void*  Buffer, unsigned long ElementSize, unsigned long ElementCount){return 0;}    // From the stream
};

class MSTRM: public STRM
{
 unsigned long Pos;
 unsigned long Len;
 unsigned long FullLen;
 unsigned long PreAllBlk;
 unsigned char* DataBuf;
//------------------------
void* GrowSize(unsigned long AddSize)    // Loses an original buffer if 'Out of Memory'
{
 unsigned long EndSize = this->Pos + AddSize;      // May not be end of buffer if seeked
 if(EndSize >  this->Len)this->Len = EndSize;      // Growed  size
 if(EndSize <= this->FullLen)return this->DataBuf; // No need to enlarge the buffer
 if(((EndSize - this->FullLen) > this->PreAllBlk))this->PreAllBlk *= 2;      // Grow preallocation on each overflowed reallocation
 this->FullLen = this->Len + this->PreAllBlk;
 if(!this->DataBuf)this->DataBuf = (unsigned char*)malloc(this->FullLen);
   else this->DataBuf = (unsigned char*)realloc(this->DataBuf, this->FullLen);
 return this->DataBuf;
}
//------------------------
void FreeBuffer(void)
{
 if(this->DataBuf)free(this->DataBuf);
}
void ResetBuffer(void)
{ 
 this->Pos = this->Len = this->FullLen = 0;
 this->PreAllBlk = 1024;
 this->FreeBuffer();
 this->DataBuf = 0;
}
//------------------------

public:
~MSTRM()
{
 this->FreeBuffer();
}
//------------------------
MSTRM(unsigned long PreAllLen=1024)
{
 this->PreAllBlk = PreAllLen;
 this->DataBuf = 0;
 this->Pos = this->Len = this->FullLen = 0;
}
//------------------------
void* GetBuffer(unsigned long* Size)
{
 if(Size)*Size = this->Len;
 return this->DataBuf;
}
//------------------------
int AssignFrom(void* Buffer, unsigned long Size)
{
 this->ResetBuffer(); 
 if(!this->GrowSize(Size))return -1;
 if(Buffer)memcpy(this->DataBuf, Buffer, Size);
 return 0;
}
//------------------------
virtual int  getc(void)
{
 if(this->Pos >= this->Len)return PEOF;  
 return this->DataBuf[this->Pos++];
}
//------------------------
virtual int  putc(int Character)
{
 if(!this->GrowSize(1))return PEOF;   // From current pos
 return this->DataBuf[this->Pos++] = Character;
}
//------------------------
virtual int  fseek(unsigned long Offset, int Origin)
{
 if(Origin == SEEK_SET)
  {
   if(Offset > this->Len)return PEOF;
   this->Pos = Offset;
  }
 else if(Origin == SEEK_CUR)
  {
   if((Offset+this->Pos) > this->Len)return PEOF;
   this->Pos += Offset;
  }
 else if(Origin == SEEK_END)
  {
   if(Offset > this->Len)return PEOF;
   this->Pos = this->Len - Offset;
  }
 else return PEOF;
 return 0;
}
//------------------------
virtual unsigned long fsize(void){return this->Len;}
virtual unsigned long ftell(void){return this->Pos;}
virtual unsigned long fread(void*  Buffer, unsigned long ElementSize, unsigned long ElementCount)
{
 unsigned long DLen = ElementSize * ElementCount;
 if((this->Pos + DLen) > this->Len)DLen = this->Len - this->Pos;    // Rest of the stream
 memcpy(Buffer, &this->DataBuf[this->Pos], DLen);
 this->Pos += DLen;
 return DLen / ElementSize;
}
//------------------------
virtual unsigned long fwrite(void* Buffer, unsigned long ElementSize, unsigned long ElementCount)
{
 unsigned long DLen = ElementSize * ElementCount;
 if(!this->GrowSize(DLen))return PEOF;     // From current pos
 memcpy(&this->DataBuf[this->Pos], Buffer, DLen);
 this->Pos += DLen;
 return DLen / ElementSize;
}
//------------------------

};


//////////////////////////// Array ////////////////////////////

// Array<T, ALIGN> a(n); creates n elements of T initialized to 0 bits.
// Constructors for T are not called.
// Indexing is bounds checked if assertions are on.
// a.size() returns n.
// a.resize(n) changes size to n, padding with 0 bits or truncating.
// a.push_back(x) appends x and increases size by 1, reserving up to size*2.
// a.pop_back() decreases size by 1, does not free memory.
// Copy and assignment are not supported.
// Memory is aligned on a ALIGN byte boundary (power of 2), default is none.

template <class T, int ALIGN=0> class Array {
private:
  int n;     // user size
  int reserved;  // actual size
  char *ptr; // allocated memory, zeroed
  T* data;   // start of n elements of aligned data

void create(int i)  // create with size i
{
  n=reserved=i;
  if (i<=0) {
    data=0;
    ptr=0;
    return;
  }
  const int sz=ALIGN+n*sizeof(T);
//  programChecker.alloc(sz);
  ptr = (char*)calloc(sz, 1);
//  if (!ptr) quit("Out of memory");
  data = (ALIGN ? (T*)(ptr+ALIGN-(((long)ptr)&(ALIGN-1))) : (T*)ptr);
  ASSERT((char*)data>=ptr && (char*)data<=ptr+ALIGN);
}

public:
  explicit Array(int i=0) {create(i);}   
  ~Array()
{
//  programChecker.alloc(-ALIGN-n*sizeof(T));
  free(ptr);
}

  T& operator[](int i) {
#ifndef NDEBUG
//    if (i<0 || i>=n) fPRINTF(stderr, "%d out of bounds %d\n", i, n), quit();
#endif
    return data[i];
  }
  const T& operator[](int i) const {
#ifndef NDEBUG
 //   if (i<0 || i>=n) fPRINTF(stderr, "%d out of bounds %d\n", i, n), quit();
#endif
    return data[i];
  }
  int size() const {return n;}
  void resize(int i)  // change size to i
{
  if (i<=reserved) {
    n=i;
    return;
  }
  char *saveptr=ptr;
  T *savedata=data;
  int saven=n;
  create(i);
  if (saveptr) {
    if (savedata) {
      memcpy(data, savedata, sizeof(T)*min(i, saven));
//      programChecker.alloc(-ALIGN-n*sizeof(T));
    }
    free(saveptr);
  }
}
  void pop_back() {if (n>0) --n;}  // decrement size
  void push_back(const T& x)  // increment size, append x
{
  if (n==reserved) {
    int saven=n;
    resize(max(1, n*2));
    n=saven;
  }
  data[n++]=x;
}
private:
  Array(const Array&);  // no copy or assignment
  Array& operator=(const Array&);
};

//////////////////////////// rnd ///////////////////////////////
// 32-bit pseudo random number generator
class Random               // This sequence MUST match for pack/unpack of each archive   // Can be used by multiple threads?
{
  U32 table[64];       //Array<U32> table;
  int i;
public:
  Random(void)   //: table(64) 
{
  i=0;
  table[0]=123456789;
  table[1]=987654321;
  for(int j=0; j<62; j++) table[j+2]=table[j+1]*11+table[j]*23/16;    
}
  U32 operator()() {
    return ++i, table[i&63]=table[i-24&63]^table[i-55&63];
  }
};

////////////////////////////// Buf /////////////////////////////

// Buf(n) buf; creates an array of n bytes (must be a power of 2).
// buf[i] returns a reference to the i'th byte with wrap (no out of bounds).
// buf(i) returns i'th byte back from pos (i > 0) 
// buf.size() returns n.

//static inline int pos;  // Number of input bytes in buf (not wrapped)

class Buf       
{
 Array<U8> b;

public:
 int pos = 0;  // Number of input bytes in buf (not wrapped)   // Was static!!!   // Should be same for multiple instances?  // Only ModelJPEG creates another instance

  Buf(int i=0): b(i) {}  
  void setsize(int i) {
    if (!i) return;
    ASSERT(i>0 && (i&(i-1))==0);
    b.resize(i);
  }
  U8& operator[](int i) {
    return b[i&b.size()-1];
  }
  int operator()(int i) const {
    ASSERT(i>0);
    return b[pos-i&b.size()-1];
  }
  int size() const {
    return b.size();
  }
};

// IntBuf(n) is a buffer of n int (must be a power of 2).
// intBuf[i] returns a reference to i'th element with wrap.

class IntBuf {
  Array<int> b;
public:
  IntBuf(int i=0): b(i) {}    
  int& operator[](int i) {
    return b[i&b.size()-1];
  }
};

///////////////////////// state table ////////////////////////

// State table:
//   nex(state, 0) = next state if bit y is 0, 0 <= state < 256
//   nex(state, 1) = next state if bit y is 1
//   nex(state, 2) = number of zeros in bit history represented by state
//   nex(state, 3) = number of ones represented
//
// States represent a bit history within some context.
// State 0 is the starting state (no bits seen).
// States 1-30 represent all possible sequences of 1-4 bits.
// States 31-252 represent a pair of counts, (n0,n1), the number
//   of 0 and 1 bits respectively.  If n0+n1 < 16 then there are
//   two states for each pair, depending on if a 0 or 1 was the last
//   bit seen.
// If n0 and n1 are too large, then there is no state to represent this
// pair, so another state with about the same ratio of n0/n1 is substituted.
// Also, when a bit is observed and the count of the opposite bit is large,
// then part of this count is discarded to favor newer data over old.

#if 1 // change to #if 0 to generate this table at run time (4% slower)
static const inline U8 State_table[256][4]={
  {  1,  2, 0, 0},{  3,  5, 1, 0},{  4,  6, 0, 1},{  7, 10, 2, 0}, // 0-3
  {  8, 12, 1, 1},{  9, 13, 1, 1},{ 11, 14, 0, 2},{ 15, 19, 3, 0}, // 4-7
  { 16, 23, 2, 1},{ 17, 24, 2, 1},{ 18, 25, 2, 1},{ 20, 27, 1, 2}, // 8-11
  { 21, 28, 1, 2},{ 22, 29, 1, 2},{ 26, 30, 0, 3},{ 31, 33, 4, 0}, // 12-15
  { 32, 35, 3, 1},{ 32, 35, 3, 1},{ 32, 35, 3, 1},{ 32, 35, 3, 1}, // 16-19
  { 34, 37, 2, 2},{ 34, 37, 2, 2},{ 34, 37, 2, 2},{ 34, 37, 2, 2}, // 20-23
  { 34, 37, 2, 2},{ 34, 37, 2, 2},{ 36, 39, 1, 3},{ 36, 39, 1, 3}, // 24-27
  { 36, 39, 1, 3},{ 36, 39, 1, 3},{ 38, 40, 0, 4},{ 41, 43, 5, 0}, // 28-31
  { 42, 45, 4, 1},{ 42, 45, 4, 1},{ 44, 47, 3, 2},{ 44, 47, 3, 2}, // 32-35
  { 46, 49, 2, 3},{ 46, 49, 2, 3},{ 48, 51, 1, 4},{ 48, 51, 1, 4}, // 36-39
  { 50, 52, 0, 5},{ 53, 43, 6, 0},{ 54, 57, 5, 1},{ 54, 57, 5, 1}, // 40-43
  { 56, 59, 4, 2},{ 56, 59, 4, 2},{ 58, 61, 3, 3},{ 58, 61, 3, 3}, // 44-47
  { 60, 63, 2, 4},{ 60, 63, 2, 4},{ 62, 65, 1, 5},{ 62, 65, 1, 5}, // 48-51
  { 50, 66, 0, 6},{ 67, 55, 7, 0},{ 68, 57, 6, 1},{ 68, 57, 6, 1}, // 52-55
  { 70, 73, 5, 2},{ 70, 73, 5, 2},{ 72, 75, 4, 3},{ 72, 75, 4, 3}, // 56-59
  { 74, 77, 3, 4},{ 74, 77, 3, 4},{ 76, 79, 2, 5},{ 76, 79, 2, 5}, // 60-63
  { 62, 81, 1, 6},{ 62, 81, 1, 6},{ 64, 82, 0, 7},{ 83, 69, 8, 0}, // 64-67
  { 84, 71, 7, 1},{ 84, 71, 7, 1},{ 86, 73, 6, 2},{ 86, 73, 6, 2}, // 68-71
  { 44, 59, 5, 3},{ 44, 59, 5, 3},{ 58, 61, 4, 4},{ 58, 61, 4, 4}, // 72-75
  { 60, 49, 3, 5},{ 60, 49, 3, 5},{ 76, 89, 2, 6},{ 76, 89, 2, 6}, // 76-79
  { 78, 91, 1, 7},{ 78, 91, 1, 7},{ 80, 92, 0, 8},{ 93, 69, 9, 0}, // 80-83
  { 94, 87, 8, 1},{ 94, 87, 8, 1},{ 96, 45, 7, 2},{ 96, 45, 7, 2}, // 84-87
  { 48, 99, 2, 7},{ 48, 99, 2, 7},{ 88,101, 1, 8},{ 88,101, 1, 8}, // 88-91
  { 80,102, 0, 9},{103, 69,10, 0},{104, 87, 9, 1},{104, 87, 9, 1}, // 92-95
  {106, 57, 8, 2},{106, 57, 8, 2},{ 62,109, 2, 8},{ 62,109, 2, 8}, // 96-99
  { 88,111, 1, 9},{ 88,111, 1, 9},{ 80,112, 0,10},{113, 85,11, 0}, // 100-103
  {114, 87,10, 1},{114, 87,10, 1},{116, 57, 9, 2},{116, 57, 9, 2}, // 104-107
  { 62,119, 2, 9},{ 62,119, 2, 9},{ 88,121, 1,10},{ 88,121, 1,10}, // 108-111
  { 90,122, 0,11},{123, 85,12, 0},{124, 97,11, 1},{124, 97,11, 1}, // 112-115
  {126, 57,10, 2},{126, 57,10, 2},{ 62,129, 2,10},{ 62,129, 2,10}, // 116-119
  { 98,131, 1,11},{ 98,131, 1,11},{ 90,132, 0,12},{133, 85,13, 0}, // 120-123
  {134, 97,12, 1},{134, 97,12, 1},{136, 57,11, 2},{136, 57,11, 2}, // 124-127
  { 62,139, 2,11},{ 62,139, 2,11},{ 98,141, 1,12},{ 98,141, 1,12}, // 128-131
  { 90,142, 0,13},{143, 95,14, 0},{144, 97,13, 1},{144, 97,13, 1}, // 132-135
  { 68, 57,12, 2},{ 68, 57,12, 2},{ 62, 81, 2,12},{ 62, 81, 2,12}, // 136-139
  { 98,147, 1,13},{ 98,147, 1,13},{100,148, 0,14},{149, 95,15, 0}, // 140-143
  {150,107,14, 1},{150,107,14, 1},{108,151, 1,14},{108,151, 1,14}, // 144-147
  {100,152, 0,15},{153, 95,16, 0},{154,107,15, 1},{108,155, 1,15}, // 148-151
  {100,156, 0,16},{157, 95,17, 0},{158,107,16, 1},{108,159, 1,16}, // 152-155
  {100,160, 0,17},{161,105,18, 0},{162,107,17, 1},{108,163, 1,17}, // 156-159
  {110,164, 0,18},{165,105,19, 0},{166,117,18, 1},{118,167, 1,18}, // 160-163
  {110,168, 0,19},{169,105,20, 0},{170,117,19, 1},{118,171, 1,19}, // 164-167
  {110,172, 0,20},{173,105,21, 0},{174,117,20, 1},{118,175, 1,20}, // 168-171
  {110,176, 0,21},{177,105,22, 0},{178,117,21, 1},{118,179, 1,21}, // 172-175
  {110,180, 0,22},{181,115,23, 0},{182,117,22, 1},{118,183, 1,22}, // 176-179
  {120,184, 0,23},{185,115,24, 0},{186,127,23, 1},{128,187, 1,23}, // 180-183
  {120,188, 0,24},{189,115,25, 0},{190,127,24, 1},{128,191, 1,24}, // 184-187
  {120,192, 0,25},{193,115,26, 0},{194,127,25, 1},{128,195, 1,25}, // 188-191
  {120,196, 0,26},{197,115,27, 0},{198,127,26, 1},{128,199, 1,26}, // 192-195
  {120,200, 0,27},{201,115,28, 0},{202,127,27, 1},{128,203, 1,27}, // 196-199
  {120,204, 0,28},{205,115,29, 0},{206,127,28, 1},{128,207, 1,28}, // 200-203
  {120,208, 0,29},{209,125,30, 0},{210,127,29, 1},{128,211, 1,29}, // 204-207
  {130,212, 0,30},{213,125,31, 0},{214,137,30, 1},{138,215, 1,30}, // 208-211
  {130,216, 0,31},{217,125,32, 0},{218,137,31, 1},{138,219, 1,31}, // 212-215
  {130,220, 0,32},{221,125,33, 0},{222,137,32, 1},{138,223, 1,32}, // 216-219
  {130,224, 0,33},{225,125,34, 0},{226,137,33, 1},{138,227, 1,33}, // 220-223
  {130,228, 0,34},{229,125,35, 0},{230,137,34, 1},{138,231, 1,34}, // 224-227
  {130,232, 0,35},{233,125,36, 0},{234,137,35, 1},{138,235, 1,35}, // 228-231
  {130,236, 0,36},{237,125,37, 0},{238,137,36, 1},{138,239, 1,36}, // 232-235
  {130,240, 0,37},{241,125,38, 0},{242,137,37, 1},{138,243, 1,37}, // 236-239
  {130,244, 0,38},{245,135,39, 0},{246,137,38, 1},{138,247, 1,38}, // 240-243
  {140,248, 0,39},{249,135,40, 0},{250, 69,39, 1},{ 80,251, 1,39}, // 244-247
  {140,252, 0,40},{249,135,41, 0},{250, 69,40, 1},{ 80,251, 1,40}, // 248-251
  {140,252, 0,41}};  // 252, 253-255 are reserved

#define nex(state,sel) State_table[state][sel]

// The code used to generate the above table at run time (4% slower).
// To print the table, uncomment the 4 lines of print statements below.
// In this code x,y = n0,n1 is the number of 0,1 bits represented by a state.
#else

class StateTable {
  Array<U8> ns;  // state*4 -> next state if 0, if 1, n0, n1
  enum {B=5, N=64}; // sizes of b, t
  static const int b[B];  // x -> max y, y -> max x
  static U8 t[N][N][2];  // x,y -> state number, number of states
  int num_states(int x, int y);  // compute t[x][y][1]
  void discount(int& x);  // set new value of x after 1 or y after 0
  void next_state(int& x, int& y, int b);  // new (x,y) after bit b
public:
  int operator()(int state, int sel) {return ns[state*4+sel];}
  StateTable();
} nex;

const int StateTable::b[B]={42,41,13,6,5};  // x -> max y, y -> max x
U8 StateTable::t[N][N][2];

int StateTable::num_states(int x, int y) {
  if (x<y) return num_states(y, x);
  if (x<0 || y<0 || x>=N || y>=N || y>=B || x>=b[y]) return 0;

  // States 0-30 are a history of the last 0-4 bits
  if (x+y<=4) {  // x+y choose x = (x+y)!/x!y!
    int r=1;
    for (int i=x+1; i<=x+y; ++i) r*=i;
    for (int i=2; i<=y; ++i) r/=i;
    return r;
  }

  // States 31-255 represent a 0,1 count and possibly the last bit
  // if the state is reachable by either a 0 or 1.
  else
    return 1+(y>0 && x+y<16);
}

// New value of count x if the opposite bit is observed
void StateTable::discount(int& x) {
  if (x>2) x=ilog(x)/6-1;
}

// compute next x,y (0 to N) given input b (0 or 1)
void StateTable::next_state(int& x, int& y, int b) {
  if (x<y)
    next_state(y, x, 1-b);
  else {
    if (b) {
      ++y;
      discount(x);
    }
    else {
      ++x;
      discount(y);
    }
    while (!t[x][y][1]) {
      if (y<2) --x;
      else {
        x=(x*(y-1)+(y/2))/y;
        --y;
      }
    }
  }
}

// Initialize next state table ns[state*4] -> next if 0, next if 1, x, y
StateTable::StateTable(): ns(1024) {

  // Assign states
  int state=0;
  for (int i=0; i<256; ++i) {
    for (int y=0; y<=i; ++y) {
      int x=i-y;
      int n=num_states(x, y);
      if (n) {
        t[x][y][0]=state;
        t[x][y][1]=n;
        state+=n;
      }
    }
  }

  // Print/generate next state table
  state=0;
  for (int i=0; i<N; ++i) {
    for (int y=0; y<=i; ++y) {
      int x=i-y;
      for (int k=0; k<t[x][y][1]; ++k) {
        int x0=x, y0=y, x1=x, y1=y;  // next x,y for input 0,1
        int ns0=0, ns1=0;
        if (state<15) {
          ++x0;
          ++y1;
          ns0=t[x0][y0][0]+state-t[x][y][0];
          ns1=t[x1][y1][0]+state-t[x][y][0];
          if (x>0) ns1+=t[x-1][y+1][1];
          ns[state*4]=ns0;
          ns[state*4+1]=ns1;
          ns[state*4+2]=x;
          ns[state*4+3]=y;
        }
        else if (t[x][y][1]) {
          next_state(x0, y0, 0);
          next_state(x1, y1, 1);
          ns[state*4]=ns0=t[x0][y0][0];
          ns[state*4+1]=ns1=t[x1][y1][0]+(t[x1][y1][1]>1);
          ns[state*4+2]=x;
          ns[state*4+3]=y;
        }
          // uncomment to print table above
//        PRINTF("{%3d,%3d,%2d,%2d},", ns[state*4], ns[state*4+1], 
//          ns[state*4+2], ns[state*4+3]);
//        if (state%4==3) PRINTF(" // %d-%d\n  ", state-3, state);
        ASSERT(state>=0 && state<256);
        ASSERT(t[x][y][1]>0);
        ASSERT(t[x][y][0]<=state);
        ASSERT(t[x][y][0]+t[x][y][1]>state);
        ASSERT(t[x][y][1]<=6);
        ASSERT(t[x0][y0][1]>0);
        ASSERT(t[x1][y1][1]>0);
        ASSERT(ns0-t[x0][y0][0]<t[x0][y0][1]);
        ASSERT(ns0-t[x0][y0][0]>=0);
        ASSERT(ns1-t[x1][y1][0]<t[x1][y1][1]);
        ASSERT(ns1-t[x1][y1][0]>=0);
        ++state;
      }
    }
  }
//  PRINTF("%d states\n", state); exit(0);  // uncomment to print table above
}

#endif

///////////////////////////// Squash //////////////////////////////

// return p = 1/(1 + exp(-d)), d scaled by 8 bits, p scaled by 12 bits
static inline int squash(int d) {
  static const int t[33]={1,2,3,6,10,16,27,45,73,120,194,310,488,747,1101,1546,2047,2549,2994,3348,3607,3785,3901,3975,4022,4050,4068,4079,4085,4089,4092,4093,4094};
  if (d>2047) return 4095;
  if (d<-2047) return 0;
  int w=d&127;
  d=(d>>7)+16;
  return (t[d]*(128-w)+t[(d+1)]*w+64) >> 7;
}

//////////////////////////// Stretch ///////////////////////////////

// Inverse of squash. d = ln(p/(1-p)), d scaled by 8 bits, p by 12 bits.
// d has range -2047 to 2047 representing -8 to 8.  p has range 0 to 4095.

class Stretch 
{
  static inline short t[4096];   //  Array<short> t;
public:
Stretch(void)         //: t(4096) {
{
  if(t[0])return;
  int pi=0;
  for (int x=-2047; x<=2047; ++x) {  // invert squash()
    int i=squash(x);
    for (int j=pi; j<=i; ++j)
      t[j]=x;
    pi=i+1;
  }
  t[4095]=2047;
}

 /* int operator()(int p) const {
    ASSERT(p>=0 && p<4096);
    return t[p];
  } */
 static int s(int p) {
    ASSERT(p>=0 && p<4096);
    return t[p];
  }
};


///////////////////////////// ilog //////////////////////////////
// ilog(x) = round(log2(x) * 16), 0 <= x < 64K
class CLog {
 static inline U8 t[65536];   // 64K of static memory!!!     //Array<U8> t;       // TODO: optionally static buffer
public:
// Compute lookup table by numerical integration of 1/x
CLog(void)      //: t(65536) 
{
  if(t[2])return;
  U32 x=14155776;
  for (int i=2; i<65536; ++i) {
    x+=774541002/(i*2-1);  // numerator is 2^29/ln 2
    t[i]=x>>24;
  }
}

static int ilog(U16 x){return t[x];}         //int operator()(U16 x) const {return t[x];}

static int llog(U32 x) 
{
  if (x>=0x1000000)
    return 256+ilog(x>>16);
  else if (x>=0x10000)
    return 128+ilog(x>>8);
  else
    return ilog(x);
}

};


//////////////////////////// hash //////////////////////////////

// Hash 2-5 ints.
static inline U32 hash(U32 a, U32 b, U32 c=0xffffffff, U32 d=0xffffffff, U32 e=0xffffffff) 
{
  U32 h=a*200002979u+b*30005491u+c*50004239u+d*70004807u+e*110002499u;
  return h^h>>9^a>>2^b>>3^c>>4^d>>5^e>>6;
}

///////////////////////////// BH ////////////////////////////////

// A BH maps a 32 bit hash to an array of B bytes (checksum and B-2 values)
//
// BH bh(N); creates N element table with B bytes each.
//   N must be a power of 2.  The first byte of each element is
//   reserved for a checksum to detect collisions.  The remaining
//   B-1 bytes are values, prioritized by the first value.  This
//   byte is 0 to mark an unused element.
//   
// bh[i] returns a pointer to the i'th element, such that
//   bh[i][0] is a checksum of i, bh[i][1] is the priority, and
//   bh[i][2..B-1] are other values (0-255).
//   The low lg(n) bits as an index into the table.
//   If a collision is detected, up to M nearby locations in the same
//   cache line are tested and the first matching checksum or
//   empty element is returned.
//   If no match or empty element is found, then the lowest priority
//   element is replaced.

// 2 byte checksum with LRU replacement (except last 2 by priority)
template <int B> class BH {
  enum {M=8};  // search limit
  Array<U8, 64> t; // elements
  U32 n; // size-1
public:
  BH(int i): t(i*B), n(i-1) {
    ASSERT(B>=2 && i>0 && (i&(i-1))==0); // size a power of 2?
  }
  U8* operator[](U32 i)
{
  int chk=(i>>16^i)&0xffff;
  i=i*M&n;
  U8 *p;
  U16 *cp;
  int j;
  for (j=0; j<M; ++j) {
    p=&t[(i+j)*B];
    cp=(U16*)p;
    if (p[2]==0) {*cp=chk;break;}
    if (*cp==chk) break;  // found
  }
  if (j==0) return p+1;  // front
  U8 tmp[B];  // element to move to front      // Was static!!!
  if (j==M) {
    --j;
    memset(tmp, 0, B);
    *(U16*)tmp=chk;
    if (M>2 && t[(i+j)*B+2]>t[(i+j-1)*B+2]) --j;
  }
  else memcpy(tmp, cp, B);
  memmove(&t[(i+1)*B], &t[i*B], j*B);
  memcpy(&t[i*B], tmp, B);
  return &t[i*B+1];
}
};
