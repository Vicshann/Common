
#ifndef SHA1H
#define SHA1H

#if !defined(SHA1_LITTLE_ENDIAN) && !defined(SHA1_BIG_ENDIAN)
#define SHA1_LITTLE_ENDIAN
#endif

//------------------------------------------------------------------------------
class CSHA1       // NOTE: Base independant
{
public:
 static const int HashSize = 20;

private:
typedef unsigned char  UINT_8;
typedef unsigned int   UINT_32;

 // Member variables
 UINT_32 m_state[5];
 UINT_32 m_count[2];
 UINT_8  m_buffer[64];
 UINT_8  m_digest[20];
 union
  {
   UINT_8  c[64];
   UINT_32 l[16];
  } m_block;
//------------------------------------------------------------------------------
#ifdef _MSC_VER
static inline UINT_32 ROL32(UINT_32 p_val32, int p_nBits){return _rotl(p_val32,p_nBits);}
#else
static inline UINT_32 ROL32(UINT_32 p_val32, int p_nBits){return ((p_val32)<<(p_nBits))|((p_val32)>>(32-(p_nBits)));}
#endif

#ifdef SHA1_LITTLE_ENDIAN
template<typename T> UINT_32 SHABLK0(T i) {return m_block.l[i] = (ROL32(m_block.l[i],24) & 0xFF00FF00) | (ROL32(m_block.l[i],8) & 0x00FF00FF);}   
#else
template<typename T> UINT_32 SHABLK0(T i) {return m_block.l[i];}
#endif

template<typename T> UINT_32 SHABLK(T i) {return m_block.l[i&15] = ROL32(m_block.l[(i+13)&15] ^ m_block.l[(i+8)&15] ^ m_block.l[(i+2)&15] ^ m_block.l[i&15],1);}

// SHA-1 rounds
template<typename T> void S_R0(T& v,T& w,T& x,T& y,T& z,int i) {z+=((w&(x^y))^y)+SHABLK0(i)+0x5A827999+ROL32(v,5);w=ROL32(w,30);}
template<typename T> void S_R1(T& v,T& w,T& x,T& y,T& z,int i) {z+=((w&(x^y))^y)+SHABLK(i)+0x5A827999+ROL32(v,5);w=ROL32(w,30);}
template<typename T> void S_R2(T& v,T& w,T& x,T& y,T& z,int i) {z+=(w^x^y)+SHABLK(i)+0x6ED9EBA1+ROL32(v,5);w=ROL32(w,30);}
template<typename T> void S_R3(T& v,T& w,T& x,T& y,T& z,int i) {z+=(((w|x)&y)|(w&x))+SHABLK(i)+0x8F1BBCDC+ROL32(v,5);w=ROL32(w,30);}
template<typename T> void S_R4(T& v,T& w,T& x,T& y,T& z,int i) {z+=(w^x^y)+SHABLK(i)+0xCA62C1D6+ROL32(v,5);w=ROL32(w,30);}
//------------------------------------------------------------------------------
static UINT_8 HalfToHex(UINT_8 Half, bool UpCase)
{
 if(Half <= 9)return 0x30 + Half;
 return ((int)!UpCase * 0x20) + 0x41 + (Half - 10);
}
//------------------------------------------------------------------------------
static void mcopy(void* dst, void* src, unsigned int len)
{
 for(unsigned int ctr=0,tot=len/sizeof(void*);ctr < tot;ctr++,((void**&)dst)++,((void**&)src)++)*((void**)dst) = *((void**)src);
 for(unsigned int ctr=0,tot=len%sizeof(void*);ctr < tot;ctr++,((char*&)dst)++,((char*&)src)++)*((char*)dst) = *((char*)src);
}
//------------------------------------------------------------------------------
void Transform(UINT_32* pState, UINT_8* pBuffer)
{
 UINT_32 a = pState[0], b = pState[1], c = pState[2], d = pState[3], e = pState[4];

 CSHA1::mcopy(&m_block.c, pBuffer, 64);

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
 UINT_32 j = ((m_count[0] >> 3) & 0x3F);
 if((m_count[0] += (uLen << 3)) < (uLen << 3))++m_count[1]; // Overflow
 m_count[1] += (uLen >> 29);

 UINT_32 i;
 if((j + uLen) > 63)
  {
   i = 64 - j;
   CSHA1::mcopy(&m_buffer[j], pbData, i);
   this->Transform(m_state, m_buffer);
   for(;(i + 63) < uLen; i += 64)this->Transform(m_state, &pbData[i]);
   j = 0;
  }
   else i = 0;
 if((uLen - i) != 0)CSHA1::mcopy(&m_buffer[j], &pbData[i], uLen - i);
}
//------------------------------------------------------------------------------
void Final(void)
{
 UINT_8 pbFinalCount[8];
 UINT_8 fins[] = {128,0};
 for(UINT_32 i = 0; i < 8; ++i)pbFinalCount[i] = static_cast<UINT_8>((m_count[((i >= 4) ? 0 : 1)] >> ((3 - (i & 3)) * 8) ) & 0xFF); // Endian independent
 this->Update((UINT_8*)&fins[0], 1); // "\200"
 while((m_count[0] & 504) != 448)this->Update((UINT_8*)&fins[1], 1);  // "\0"
 this->Update(pbFinalCount, 8); // Cause a Transform()
 for(UINT_32 i = 0; i < HashSize; ++i)m_digest[i] = static_cast<UINT_8>((m_state[i >> 2] >> ((3 - (i & 3)) * 8)) & 0xFF);
}
//------------------------------------------------------------------------------
unsigned char* GetHash(void){return reinterpret_cast<unsigned char*>(&m_digest);}
void GetHashStr(char* str, bool UpCase) // str size must be >= 65 bytes!
{
 for(int ctr =0;ctr < HashSize;ctr++,str+=2)
  {
   BYTE val = m_digest[ctr];
   str[1] = HalfToHex(val & 0x0F, UpCase);
   str[0] = HalfToHex(val >> 4, UpCase);
  }
 *str = 0;
}
//------------------------------------------------------------------------------
};

#endif
