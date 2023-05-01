
#pragma once


#if !defined(SHA1_LITTLE_ENDIAN) && !defined(SHA1_BIG_ENDIAN)
#define SHA1_LITTLE_ENDIAN
#endif

//------------------------------------------------------------------------------
class CSHA1       // NOTE: Base independant
{
public:
 static const int HashSize = 20;

private:
 // Member variables
 uint32 m_state[5];
 uint32 m_count[2];
 uint8  m_buffer[64];
 uint8  m_digest[20];
 union
  {
   uint8  c[64];
   uint32 l[16];
  } m_block;
//------------------------------------------------------------------------------
/*#ifdef _MSC_VER
static inline uint32 ROL32(uint32 p_val32, int p_nBits){return _rotl(p_val32,p_nBits);}
#else
static inline uint32 ROL32(uint32 p_val32, int p_nBits){return ((p_val32)<<(p_nBits))|((p_val32)>>(32-(p_nBits)));}
#endif */

#define ROL32 RotL<uint32>

#ifdef SHA1_LITTLE_ENDIAN
template<typename T> uint32 SHABLK0(T i) {return m_block.l[i] = (ROL32(m_block.l[i],24) & 0xFF00FF00) | (ROL32(m_block.l[i],8) & 0x00FF00FF);}
#else
template<typename T> uint32 SHABLK0(T i) {return m_block.l[i];}
#endif

template<typename T> uint32 SHABLK(T i) {return m_block.l[i&15] = ROL32(m_block.l[(i+13)&15] ^ m_block.l[(i+8)&15] ^ m_block.l[(i+2)&15] ^ m_block.l[i&15],1);}

// SHA-1 rounds
template<typename T> void S_R0(T& v,T& w,T& x,T& y,T& z,int i) {z+=((w&(x^y))^y)+SHABLK0(i)+0x5A827999+ROL32(v,5);w=ROL32(w,30);}
template<typename T> void S_R1(T& v,T& w,T& x,T& y,T& z,int i) {z+=((w&(x^y))^y)+SHABLK(i)+0x5A827999+ROL32(v,5);w=ROL32(w,30);}
template<typename T> void S_R2(T& v,T& w,T& x,T& y,T& z,int i) {z+=(w^x^y)+SHABLK(i)+0x6ED9EBA1+ROL32(v,5);w=ROL32(w,30);}
template<typename T> void S_R3(T& v,T& w,T& x,T& y,T& z,int i) {z+=(((w|x)&y)|(w&x))+SHABLK(i)+0x8F1BBCDC+ROL32(v,5);w=ROL32(w,30);}
template<typename T> void S_R4(T& v,T& w,T& x,T& y,T& z,int i) {z+=(w^x^y)+SHABLK(i)+0xCA62C1D6+ROL32(v,5);w=ROL32(w,30);}
//------------------------------------------------------------------------------
static uint8 HalfToHex(uint8 Half, bool UpCase)
{
 if(Half <= 9)return 0x30 + Half;
 return ((int)!UpCase * 0x20) + 0x41 + (Half - 10);
}
//------------------------------------------------------------------------------
void Transform(uint32* pState, uint8* pBuffer)
{
 uint32 a = pState[0], b = pState[1], c = pState[2], d = pState[3], e = pState[4];
 memcpy(&m_block.c, pBuffer, 64);

 // 4 rounds of 20 operations each, loop unrolled
 S_R0(a,b,c,d,e, 0); S_R0(e,a,b,c,d, 1); S_R0(d,e,a,b,c, 2); S_R0(c,d,e,a,b, 3);
 S_R0(b,c,d,e,a, 4); S_R0(a,b,c,d,e, 5); S_R0(e,a,b,c,d, 6); S_R0(d,e,a,b,c, 7);
 S_R0(c,d,e,a,b, 8); S_R0(b,c,d,e,a, 9); S_R0(a,b,c,d,e,10); S_R0(e,a,b,c,d,11);
 S_R0(d,e,a,b,c,12); S_R0(c,d,e,a,b,13); S_R0(b,c,d,e,a,14); S_R0(a,b,c,d,e,15);
 S_R1(e,a,b,c,d,16); S_R1(d,e,a,b,c,17); S_R1(c,d,e,a,b,18); S_R1(b,c,d,e,a,19);
 S_R2(a,b,c,d,e,20); S_R2(e,a,b,c,d,21); S_R2(d,e,a,b,c,22); S_R2(c,d,e,a,b,23);
 S_R2(b,c,d,e,a,24); S_R2(a,b,c,d,e,25); S_R2(e,a,b,c,d,26); S_R2(d,e,a,b,c,27);
 S_R2(c,d,e,a,b,28); S_R2(b,c,d,e,a,29); S_R2(a,b,c,d,e,30); S_R2(e,a,b,c,d,31);
 S_R2(d,e,a,b,c,32); S_R2(c,d,e,a,b,33); S_R2(b,c,d,e,a,34); S_R2(a,b,c,d,e,35);
 S_R2(e,a,b,c,d,36); S_R2(d,e,a,b,c,37); S_R2(c,d,e,a,b,38); S_R2(b,c,d,e,a,39);
 S_R3(a,b,c,d,e,40); S_R3(e,a,b,c,d,41); S_R3(d,e,a,b,c,42); S_R3(c,d,e,a,b,43);
 S_R3(b,c,d,e,a,44); S_R3(a,b,c,d,e,45); S_R3(e,a,b,c,d,46); S_R3(d,e,a,b,c,47);
 S_R3(c,d,e,a,b,48); S_R3(b,c,d,e,a,49); S_R3(a,b,c,d,e,50); S_R3(e,a,b,c,d,51);
 S_R3(d,e,a,b,c,52); S_R3(c,d,e,a,b,53); S_R3(b,c,d,e,a,54); S_R3(a,b,c,d,e,55);
 S_R3(e,a,b,c,d,56); S_R3(d,e,a,b,c,57); S_R3(c,d,e,a,b,58); S_R3(b,c,d,e,a,59);
 S_R4(a,b,c,d,e,60); S_R4(e,a,b,c,d,61); S_R4(d,e,a,b,c,62); S_R4(c,d,e,a,b,63);
 S_R4(b,c,d,e,a,64); S_R4(a,b,c,d,e,65); S_R4(e,a,b,c,d,66); S_R4(d,e,a,b,c,67);
 S_R4(c,d,e,a,b,68); S_R4(b,c,d,e,a,69); S_R4(a,b,c,d,e,70); S_R4(e,a,b,c,d,71);
 S_R4(d,e,a,b,c,72); S_R4(c,d,e,a,b,73); S_R4(b,c,d,e,a,74); S_R4(a,b,c,d,e,75);
 S_R4(e,a,b,c,d,76); S_R4(d,e,a,b,c,77); S_R4(c,d,e,a,b,78); S_R4(b,c,d,e,a,79);

 // Add the working vars back into state
 pState[0] += a;
 pState[1] += b;
 pState[2] += c;
 pState[3] += d;
 pState[4] += e;
}
//------------------------------------------------------------------------------

public:
CSHA1(void){Reset();}
~CSHA1(){Reset();}
//------------------------------------------------------------------------------
void Reset(void)
{
 // SHA1 initialization constants
 m_state[0] = 0x67452301;
 m_state[1] = 0xEFCDAB89;
 m_state[2] = 0x98BADCFE;
 m_state[3] = 0x10325476;
 m_state[4] = 0xC3D2E1F0;

 m_count[0] = 0;
 m_count[1] = 0;
}
//------------------------------------------------------------------------------
void Update(unsigned char* pbData, unsigned int uLen)
{
 uint32 j = ((m_count[0] >> 3) & 0x3F);
 if((m_count[0] += (uLen << 3)) < (uLen << 3))++m_count[1]; // Overflow
 m_count[1] += (uLen >> 29);

 uint32 i;
 if((j + uLen) > 63)
  {
   i = 64 - j;
   memcpy(&m_buffer[j], pbData, i);
   this->Transform(m_state, m_buffer);
   for(;(i + 63) < uLen; i += 64)this->Transform(m_state, &pbData[i]);
   j = 0;
  }
   else i = 0;
 if((uLen - i) != 0)memcpy(&m_buffer[j], &pbData[i], uLen - i);
}
//------------------------------------------------------------------------------
void Final(void)
{
 uint8 pbFinalCount[8];
 uint8 fins[] = {128,0};
 for(uint32 i = 0; i < 8; ++i)pbFinalCount[i] = static_cast<uint8>((m_count[((i >= 4) ? 0 : 1)] >> ((3 - (i & 3)) * 8) ) & 0xFF); // Endian independent
 this->Update((uint8*)&fins[0], 1); // "\200"
 while((m_count[0] & 504) != 448)this->Update((uint8*)&fins[1], 1);  // "\0"
 this->Update(pbFinalCount, 8); // Cause a Transform()
 for(uint32 i = 0; i < HashSize; ++i)m_digest[i] = static_cast<uint8>((m_state[i >> 2] >> ((3 - (i & 3)) * 8)) & 0xFF);
}
//------------------------------------------------------------------------------
unsigned char* GetHash(void){return reinterpret_cast<unsigned char*>(&m_digest);}

void GetHashStr(char* str, bool UpCase) // str size must be >= 65 bytes!
{
 for(int ctr =0;ctr < HashSize;ctr++,str+=2)
  {
   uint8 val = m_digest[ctr];
   str[1] = HalfToHex(val & 0x0F, UpCase);
   str[0] = HalfToHex(val >> 4, UpCase);
  }
 *str = 0;
}
//------------------------------------------------------------------------------
};

//==============================================================================
//_finline __attribute__ ((optnone))
template<typename T=uint8> static constexpr void FWICALL Digest_SHA1(uint8 *digest, const T *data, size_t databytes)     // _finline  // TODO: optional _finline
{
  uint32 H[]  = {0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0};
  uint32 didx = 0;
  uint64 databits  = ((uint64)databytes) * 8;
  uint32 loopcount = (databytes + 8) / 64 + 1;
  uint32 tailbytes = 64 * loopcount - databytes;
  uint8  datatail[128] = {0};

  // Pre-processing of data tail (includes padding to fill out 512-bit chunk):
  //   Add bit '1' to end of message (big-endian)
  //   Add 64-bit message length in bits at very end (big-endian)
  datatail[0] = 0x80;
  datatail[tailbytes - 8] = (uint8) (databits >> 56 & 0xFF);
  datatail[tailbytes - 7] = (uint8) (databits >> 48 & 0xFF);
  datatail[tailbytes - 6] = (uint8) (databits >> 40 & 0xFF);
  datatail[tailbytes - 5] = (uint8) (databits >> 32 & 0xFF);
  datatail[tailbytes - 4] = (uint8) (databits >> 24 & 0xFF);
  datatail[tailbytes - 3] = (uint8) (databits >> 16 & 0xFF);
  datatail[tailbytes - 2] = (uint8) (databits >> 8  & 0xFF);
  datatail[tailbytes - 1] = (uint8) (databits >> 0  & 0xFF);

  // Process each 512-bit chunk
  for (uint32 lidx = 0; lidx < loopcount; lidx++)
  {
    // Compute all elements in W
    uint32 W[80] = {0};  // Memset is a problem
    //memset (W, 0, sizeof (W));
    // Break 512-bit chunk into sixteen 32-bit, big endian words
    for (uint32 widx = 0; widx <= 15; widx++)
    {
      int32 wcount = 24;
      // Copy byte-per byte from specified buffer
      while (didx < databytes && wcount >= 0)
      {
        W[widx] += (((uint32)data[didx]) << wcount);
        didx++;
        wcount -= 8;
      }
      // Fill out W with padding as needed
      while (wcount >= 0)
      {
        W[widx] += (((uint32)datatail[didx - databytes]) << wcount);
        didx++;
        wcount -= 8;
      }
    }

    // Extend the sixteen 32-bit words into eighty 32-bit words, with potential optimization from:
    //   "Improving the Performance of the Secure Hash Algorithm (SHA-1)" by Max Locktyukhin
    for (uint32 widx = 16; widx <= 31; widx++)
    {
      W[widx] = RotL<uint32> ((W[widx - 3] ^ W[widx - 8] ^ W[widx - 14] ^ W[widx - 16]), 1);
    }
    for (uint32 widx = 32; widx <= 79; widx++)
    {
      W[widx] = RotL<uint32> ((W[widx - 6] ^ W[widx - 16] ^ W[widx - 28] ^ W[widx - 32]), 2);
    }

    // Main loop
    uint32 a = H[0];
    uint32 b = H[1];
    uint32 c = H[2];
    uint32 d = H[3];
    uint32 e = H[4];

    for (uint32 idx = 0; idx <= 79; idx++)
    {
     uint32 f = 0;
     uint32 k = 0;
      if (idx <= 19)
      {
        f = (b & c) | ((~b) & d);
        k = 0x5A827999;
      }
      else if (idx >= 20 && idx <= 39)
      {
        f = b ^ c ^ d;
        k = 0x6ED9EBA1;
      }
      else if (idx >= 40 && idx <= 59)
      {
        f = (b & c) | (b & d) | (c & d);
        k = 0x8F1BBCDC;
      }
      else if (idx >= 60 && idx <= 79)
      {
        f = b ^ c ^ d;
        k = 0xCA62C1D6;
      }
      uint32 temp = RotL<uint32> (a, 5) + f + e + k + W[idx];
      e = d;
      d = c;
      c = RotL<uint32> (b, 30);
      b = a;
      a = temp;
    }

    H[0] += a;
    H[1] += b;
    H[2] += c;
    H[3] += d;
    H[4] += e;
  }

  // Store binary digest in supplied buffer
    for (uint32 idx = 0; idx < 5; idx++)
    {
      digest[idx * 4 + 0] = (uint8) (H[idx] >> 24);
      digest[idx * 4 + 1] = (uint8) (H[idx] >> 16);
      digest[idx * 4 + 2] = (uint8) (H[idx] >> 8);
      digest[idx * 4 + 3] = (uint8) (H[idx]);
    }
}
//==============================================================================
// How to return uint8 array initializer directly?
// Hash a string literal
template<typename T, uint N> static consteval auto ctSHA1(const T(&str)[N])
{
 SDHldr<uint8, 20> v;
 uint len = (str[N-1])?(N):(N-1); // Exclude terminating 0
 Digest_SHA1<T>(v, &str[0], len);
 return v;
}
//==============================================================================



