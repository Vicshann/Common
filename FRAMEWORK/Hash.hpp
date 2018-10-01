
#pragma once

// http://www.sanmayce.com/Fastest_Hash/
//---------------------------------------------------------------------------
uint32_t FNV1A_Hash_YoshimitsuTRIADii(const char *str, uint32_t wrdlen)
{
    const uint32_t PRIME = 709607;
    uint32_t hash32 = 2166136261;
    uint32_t hash32B = 2166136261;
    uint32_t hash32C = 2166136261;
    const char *p = str;
    uint32_t Loop_Counter;
    uint32_t Second_Line_Offset;

if (wrdlen >= 24) {
    Loop_Counter = (wrdlen/24);
    Loop_Counter++;
    Second_Line_Offset = wrdlen-(Loop_Counter)*(3*4);
    for(; Loop_Counter; Loop_Counter--, p += 3*sizeof(uint32_t)) {
		hash32 = (hash32 ^ (_rotl_KAZE(*(uint32_t *)(p+0),5) ^ *(uint32_t *)(p+0+Second_Line_Offset))) * PRIME;        
		hash32B = (hash32B ^ (_rotl_KAZE(*(uint32_t *)(p+4+Second_Line_Offset),5) ^ *(uint32_t *)(p+4))) * PRIME;        
		hash32C = (hash32C ^ (_rotl_KAZE(*(uint32_t *)(p+8),5) ^ *(uint32_t *)(p+8+Second_Line_Offset))) * PRIME;        
    }
		hash32 = (hash32 ^ _rotl_KAZE(hash32C,5) ) * PRIME;
} else {
    // 1111=15; 10111=23
    if (wrdlen & 4*sizeof(uint32_t)) {	
		hash32 = (hash32 ^ (_rotl_KAZE(*(uint32_t *)(p+0),5) ^ *(uint32_t *)(p+4))) * PRIME;        
		hash32B = (hash32B ^ (_rotl_KAZE(*(uint32_t *)(p+8),5) ^ *(uint32_t *)(p+12))) * PRIME;        
		p += 8*sizeof(uint16_t);
    }
    // Cases: 0,1,2,3,4,5,6,7,...,15
    if (wrdlen & 2*sizeof(uint32_t)) {
		hash32 = (hash32 ^ *(uint32_t*)(p+0)) * PRIME;
		hash32B = (hash32B ^ *(uint32_t*)(p+4)) * PRIME;
		p += 4*sizeof(uint16_t);
    }
    // Cases: 0,1,2,3,4,5,6,7
    if (wrdlen & sizeof(uint32_t)) {
		hash32 = (hash32 ^ *(uint16_t*)(p+0)) * PRIME;
		hash32B = (hash32B ^ *(uint16_t*)(p+2)) * PRIME;
		p += 2*sizeof(uint16_t);
    }
    if (wrdlen & sizeof(uint16_t)) {
        hash32 = (hash32 ^ *(uint16_t*)p) * PRIME;
        p += sizeof(uint16_t);
    }
    if (wrdlen & 1) 
        hash32 = (hash32 ^ *p) * PRIME;
}
    hash32 = (hash32 ^ _rotl_KAZE(hash32B,5) ) * PRIME;
    return hash32 ^ (hash32 >> 16);
}
//---------------------------------------------------------------------------
uint32_t FNV1A_Hash_YoshimitsuTRIAD(const char *str, uint32_t wrdlen)
{
    const uint32_t PRIME = 709607;
    uint32_t hash32 = 2166136261;
    uint32_t hash32B = 2166136261;
    uint32_t hash32C = 2166136261;
    //uint32_t hash32D = 2166136261;
    const char *p = str;

    for(; wrdlen >= 3*2*sizeof(uint32_t); wrdlen -= 3*2*sizeof(uint32_t), p += 3*2*sizeof(uint32_t)) {
		hash32  = (hash32 ^ (_rotl(*(uint32_t *)(p+0),5) ^ *(uint32_t *)(p+4))) * PRIME;        
		hash32B = (hash32B ^ (_rotl(*(uint32_t *)(p+8),5) ^ *(uint32_t *)(p+12))) * PRIME;        
		hash32C = (hash32C ^ (_rotl(*(uint32_t *)(p+16),5) ^ *(uint32_t *)(p+20))) * PRIME;        
		//hash32D = (hash32D ^ (_rotl(*(uint32_t *)(p+24),5) ^ *(uint32_t *)(p+28))) * PRIME;        
    }
	if (p != str) {
		hash32 = (hash32 ^ _rotl(hash32C,5) ) * PRIME;
		//hash32B = (hash32B ^ _rotl(hash32D,5) ) * PRIME;
	}

    // 1111=15; 10111=23
    if (wrdlen & 4*sizeof(uint32_t)) {	
		hash32 = (hash32 ^ (_rotl(*(uint32_t *)(p+0),5) ^ *(uint32_t *)(p+4))) * PRIME;        
		hash32B = (hash32B ^ (_rotl(*(uint32_t *)(p+8),5) ^ *(uint32_t *)(p+12))) * PRIME;        
		p += 8*sizeof(uint16_t);
    }
    // Cases: 0,1,2,3,4,5,6,7,...,15
    if (wrdlen & 2*sizeof(uint32_t)) {
		hash32 = (hash32 ^ *(uint32_t*)(p+0)) * PRIME;
		hash32B = (hash32B ^ *(uint32_t*)(p+4)) * PRIME;
		p += 4*sizeof(uint16_t);
    }
    // Cases: 0,1,2,3,4,5,6,7
    if (wrdlen & sizeof(uint32_t)) {
		hash32 = (hash32 ^ *(uint16_t*)(p+0)) * PRIME;
		hash32B = (hash32B ^ *(uint16_t*)(p+2)) * PRIME;
		p += 2*sizeof(uint16_t);
    }
    if (wrdlen & sizeof(uint16_t)) {
        hash32 = (hash32 ^ *(uint16_t*)p) * PRIME;
        p += sizeof(uint16_t);
    }
    if (wrdlen & 1) 
        hash32 = (hash32 ^ *p) * PRIME;

    hash32 = (hash32 ^ _rotl(hash32B,5) ) * PRIME;
    return hash32 ^ (hash32 >> 16);
}
//---------------------------------------------------------------------------
uint32_t FNV1A_Hash_Yoshimura(const char *str, SIZE_T wrdlen)
{
    const uint32_t PRIME = 709607;
    uint32_t hash32 = 2166136261;
    uint32_t hash32B = 2166136261;
    const char *p = str;
    uint32_t Loop_Counter;
    uint32_t Second_Line_Offset;

if (wrdlen >= 2*2*sizeof(uint32_t)) {
    Second_Line_Offset = wrdlen-((wrdlen>>4)+1)*(2*4); // ((wrdlen>>1)>>3)
    Loop_Counter = (wrdlen>>4);
    //if (wrdlen%16) Loop_Counter++;
    Loop_Counter++;
    for(; Loop_Counter; Loop_Counter--, p += 2*sizeof(uint32_t)) {
		// revision 1:
		//hash32 = (hash32 ^ (_rotl(*(uint32_t *)(p+0),5) ^ *(uint32_t *)(p+4))) * PRIME;        
		//hash32B = (hash32B ^ (_rotl(*(uint32_t *)(p+0+Second_Line_Offset),5) ^ *(uint32_t *)(p+4+Second_Line_Offset))) * PRIME;        
		// revision 2:
		hash32 = (hash32 ^ (_rotl(*(uint32_t *)(p+0),5) ^ *(uint32_t *)(p+0+Second_Line_Offset))) * PRIME;        
		hash32B = (hash32B ^ (_rotl(*(uint32_t *)(p+4+Second_Line_Offset),5) ^ *(uint32_t *)(p+4))) * PRIME;        
    }
} else {
    // Cases: 0,1,2,3,4,5,6,7,...,15
    if (wrdlen & 2*sizeof(uint32_t)) {
		hash32 = (hash32 ^ *(uint32_t*)(p+0)) * PRIME;
		hash32B = (hash32B ^ *(uint32_t*)(p+4)) * PRIME;
		p += 4*sizeof(uint16_t);
    }
    // Cases: 0,1,2,3,4,5,6,7
    if (wrdlen & sizeof(uint32_t)) {
		hash32 = (hash32 ^ *(uint16_t*)(p+0)) * PRIME;
		hash32B = (hash32B ^ *(uint16_t*)(p+2)) * PRIME;
		p += 2*sizeof(uint16_t);
    }
    if (wrdlen & sizeof(uint16_t)) {
        hash32 = (hash32 ^ *(uint16_t*)p) * PRIME;
        p += sizeof(uint16_t);
    }
    if (wrdlen & 1) 
        hash32 = (hash32 ^ *p) * PRIME;
}
    hash32 = (hash32 ^ _rotl(hash32B,5) ) * PRIME;
    return hash32 ^ (hash32 >> 16);
}
//---------------------------------------------------------------------------
//Yoshimitsu is beyond hope - he fights as a true underdog - nothing to lose everything to gain, sharing with those in need - a true copylefter.
uint32_t FNV1A_Hash_Yoshimitsu(const char *str, uint32_t wrdlen)
{
    const uint32_t PRIME = 709607;
    uint32_t hash32 = 2166136261;
    uint32_t hash32B = 2166136261;
    uint32_t hash32C = 2166136261;
    uint32_t hash32D = 2166136261;
    const char *p = str;

    for(; wrdlen >= 4*2*sizeof(uint32_t); wrdlen -= 4*2*sizeof(uint32_t), p += 4*2*sizeof(uint32_t)) {
        //hash32 = (hash32 ^ (_rotl_KAZE(*(DWORD *)(p+0),5) ^ *(DWORD *)(p+4))) * PRIME;        
        //hash32B = (hash32B ^ (_rotl_KAZE(*(DWORD *)(p+8),5) ^ *(DWORD *)(p+12))) * PRIME;        
//        hash32 = _rotl_KAZE( (hash32 ^ (_rotl_KAZE(*(DWORD *)(p+0),5) ^ *(DWORD *)(p+4))) , 19); // 1<<19 = 524288 ~ 709607; * -> <<'s
//        hash32B = _rotl_KAZE( (hash32B ^ (_rotl_KAZE(*(DWORD *)(p+8),5) ^ *(DWORD *)(p+12))) , 19); 
		//3*2*sizeof(DWORD)
		//hash32 = ( (hash32 ^ (_rotl(*(DWORD *)(p+0),5) ^ *(DWORD *)(p+4))) * PRIME ) ^ ( *(DWORD *)(p+8) * PRIME );
		//hash32B = ( (hash32B ^ (_rotl(*(DWORD *)(p+12),5) ^ *(DWORD *)(p+16))) * PRIME ) ^ ( *(DWORD *)(p+20) * PRIME );

		//4*2*sizeof(DWORD)
		hash32 = (hash32 ^ (_rotl(*(uint32_t *)(p+0),5) ^ *(uint32_t *)(p+4))) * PRIME;        
		hash32B = (hash32B ^ (_rotl(*(uint32_t *)(p+8),5) ^ *(uint32_t *)(p+12))) * PRIME;        
		hash32C = (hash32C ^ (_rotl(*(uint32_t *)(p+16),5) ^ *(uint32_t *)(p+20))) * PRIME;        
		hash32D = (hash32D ^ (_rotl(*(uint32_t *)(p+24),5) ^ *(uint32_t *)(p+28))) * PRIME;        
    }
	if (p != str) {
		hash32 = (hash32 ^ _rotl(hash32C,5) ) * PRIME;
		hash32B = (hash32B ^ _rotl(hash32D,5) ) * PRIME;
	}

    // 1111=15; 10111=23
    if (wrdlen & 4*sizeof(uint32_t)) {	
		hash32 = (hash32 ^ (_rotl(*(uint32_t *)(p+0),5) ^ *(uint32_t *)(p+4))) * PRIME;        
		hash32B = (hash32B ^ (_rotl(*(uint32_t *)(p+8),5) ^ *(uint32_t *)(p+12))) * PRIME;        
		p += 8*sizeof(uint16_t);
    }
    // Cases: 0,1,2,3,4,5,6,7,...,15
    if (wrdlen & 2*sizeof(uint32_t)) {
		hash32 = (hash32 ^ *(uint32_t*)(p+0)) * PRIME;
		hash32B = (hash32B ^ *(uint32_t*)(p+4)) * PRIME;
		p += 4*sizeof(uint16_t);
    }
    // Cases: 0,1,2,3,4,5,6,7
    if (wrdlen & sizeof(uint32_t)) {
		hash32 = (hash32 ^ *(uint16_t*)(p+0)) * PRIME;
		hash32B = (hash32B ^ *(uint16_t*)(p+2)) * PRIME;
		p += 2*sizeof(uint16_t);
    }
    if (wrdlen & sizeof(uint16_t)) {
        hash32 = (hash32 ^ *(uint16_t*)p) * PRIME;
        p += sizeof(uint16_t);
    }
    if (wrdlen & 1) 
        hash32 = (hash32 ^ *p) * PRIME;

    hash32 = (hash32 ^ _rotl(hash32B,5) ) * PRIME;
    return hash32 ^ (hash32 >> 16);
}
//---------------------------------------------------------------------------
// The jester as a symbol:
// The root of the word "fool" is from the Latin follis, which means "bag of wind" or that which contains air or breath.
// In Tarot, "The Fool" is the first card of the Major Arcana.
// The tarot depiction of the Fool includes a man, (or less often, a woman), juggling unconcernedly or otherwise distracted, with a dog (sometimes cat) at his heels.
// The fool is in the act of unknowingly walking off the edge of a cliff, precipice or other high place.
// Another Tarot character is Death. In the Middle Ages Death is often shown in Jester's garb because "The last laugh is reserved for death." Also, Death humbles everyone just as Jesters make fun of everyone regardless of standing.
// In literature, the jester is symbolic of common sense and of honesty, notably in King Lear, the court jester is a character used for insight and advice on the part of the monarch, taking advantage of his license to mock and speak freely to dispense frank observations and highlight the folly of his monarch.
// This presents a clashing irony as a "greater" man could dispense the same advice and find himself being detained in the dungeons or even executed. Only as the lowliest member of the court can the jester be the monarch's most useful adviser.
// Distinction was made between fools and clowns, or country bumpkins.
// The fool's status was one of privilege within a royal or noble household.
// His folly could be regarded as the raving of a madman but was often deemed to be divinely inspired.
// The 'natural' fool was touched by God. Much to Gonerill's annoyance, Lear's 'all-licensed' Fool enjoys a privileged status. His characteristic idiom suggests he is a 'natural' fool, not an artificial one, though his perceptiveness and wit show that he is far from being an idiot, however 'touched' he might be.
// The position of the Joker playing card, as a wild card which has no fixed place in the hierarchy of King, Queen, Knave, etc. might be a remnant of the position of the court jester.
// This lack of any place in the hierarchy meant Kings could trust the counsel of the jesters, as they had no vested interest in any region, estate or church.
// Source: en.wikipedia.org
#define ROL(x, n) (((x) << (n)) | ((x) >> (32-(n))))
UINT FNV1A_Hash_Jesteress(const char *str, SIZE_T wrdlen)
{
    const UINT PRIME = 709607;
    UINT hash32 = 2166136261;
    const char *p = str;

    // Idea comes from Igor Pavlov's 7zCRC, thanks.
/*
    for(; wrdlen && ((unsigned)(ptrdiff_t)p&3); wrdlen -= 1, p++) {
        hash32 = (hash32 ^ *p) * PRIME;
    }
*/
    for(; wrdlen >= 2*sizeof(DWORD); wrdlen -= 2*sizeof(DWORD), p += 2*sizeof(DWORD)) {
        hash32 = (hash32 ^ (ROL(*(DWORD *)p,5)^*(DWORD *)(p+4))) * PRIME;        
    }
    // Cases: 0,1,2,3,4,5,6,7
    if (wrdlen & sizeof(DWORD)) {
        hash32 = (hash32 ^ *(DWORD*)p) * PRIME;
        p += sizeof(DWORD);
    }
    if (wrdlen & sizeof(WORD)) {
        hash32 = (hash32 ^ *(WORD*)p) * PRIME;
        p += sizeof(WORD);
    }
    if (wrdlen & 1) 
        hash32 = (hash32 ^ *p) * PRIME;
    
    return hash32 ^ (hash32 >> 16);
}
//---------------------------------------------------------------------------
// "No High-Speed Limit", says Tesla.
// 
// As a youth, Tesla exhibited a peculiar trait that he considered the basis of all his invention.
// He had an abnormal ability, usually involuntary, to visualize scenes, people, and things so vividly that he was sometimes unsure of what was real and what imaginary.
// Strong flashes of light often accompanied these images.
// Tormented, he would move his hand in front of his eyes to determine whether the objects were simply in his mind or outside.
// He considered the strange ability an affliction at first, but for an inventor it could be a gift.
// Tesla wrote of these phenomena and of his efforts to find an explanation for them, since no psychologist or physiologist was ever able to help him.
// "The theory I have formulated," he wrote much later, is that the images were the result of a reflex action from the brain on the retina under great excitation.
// They certainly were not hallucinations, for in other respects I was normal and composed.
// To give an idea of my distress, suppose that I had witnessed a funeral or some such nerve-wracking spectacle.
// Then, inevitably, in the stillness of the night, a vivid picture of the scene would thrust itself before my eyes and persist despite all my efforts to banish it.
// Sometimes it would even remain fixed in space though I pushed my hand through it.
// /Excerpts from 'Tesla: Master of Lightning', by Margaret Cheney and Robert Uth/
// 
// "It is not the time," said Dr. Tesla yesterday, "to go into the details of this thing.
// It is founded on a principle that means great things in peace, it can be used for great things in war.
// But I repeat, this is no time to talk of such things."
// /New York Times, Dec. 8, 1915/
// 
// On the occasion of his 75th birthday, Tesla talked about new developments.
// "I am working now upon two things," he said.
// "First, an explanation based upon pure mathematics of certain things which Professor Einstein has also attempted to explain.
// My conclusions in certain respects differ from and to that extent tend to disprove the Einstein Theory . . . 
// My explanations of natural phenomena are not so involved as his.
// They are simpler, and when I am ready to make a full announcement it will be seen that I have proved my conclusions."
// "Secondly, I am working to develop a new source of power.
// When I say a new source, I mean that I have turned for power to a source which no previous scientist has turned, to the best of my knowledge.
// The conception, the idea when it first burst upon me was a tremendous shock."
// "It will throw light on many puzzling phenomena of the cosmos, and may prove also of great industrial value, particularly in creating a new and virtually unlimited market for steel."
// Tesla said it will come from an entirely new and unsuspected source, and will be for all practical purposes constant day and night, and at all times of the year.
// The apparatus for capturing the energy and transforming it will partake both of mechanical and electrical features, and will be of ideal simplicity.
// Tesla has already conceived a means that will make it possible for man to transmit energy in large amounts, thousands of horsepower, from one planet to another, absolutely regardless of distance.
// He considered that nothing can be more important than interplanetary communication.
// It will certainly come some day, and the certitude that there are other human beings in the universe, working, suffering, struggling, like ourselves, will produce a magic effect on mankind and will form the foundation of a universal brotherhood that will last as long as humanity itself.
// /Time, July 20, 1931/
// 
// Dr. Nikola Tesla asserted in an interview with Hugo Gernsback that speeds greater than that of light, which are considered impossible by the Einstein theory of relativity, have been produced.
// Stating that the Einstein theory is erroneous in many respects, Dr. Tesla stated as early as 1900, in his patent 787,412, that the current of his radio-power transmitter passed over the surface of the earth with a speed of 292,830 miles a second.
// According to the Einstein theory, the highest possible speed is 186,300 miles a second.
// Tesla indicated knowledge of speeds several times greater than light, and had apparatus designed to project so-called electrons with a speed equal to twice that of light.
// /The Literary Digest, Nov. 7, 1931/
// 
// He had discovered the so-called cosmic ray in 1896, at least five years before any other scientist took it up and twenty years before it became popular among scientists, and he is now convinced that many of the cosmic particles travel fifty times faster than light, some of them 500 times faster.
// /New York World-Telegram, July 11, 1935/
 
// Revision 2 (main mix should be refined), written 2012-Jun-16, thanks to Maciej Adamczyk for his testbed and openness.
 
#define ROL64(x, n) (((x) << (n)) | ((x) >> (64-(n))))
#define ROL(x, n) (((x) << (n)) | ((x) >> (32-(n))))
uint64_t FNV1A_Hash_Tesla(const char *str, size_t wrdlen)
{
	const uint64_t PRIME = 11400714819323198393ULL;
	uint64_t hash64 = 2166136261U; // should be much bigger
	uint64_t hash64B = 2166136261U; // should be much bigger
	const char *p = str;
	
	for(; wrdlen >= 2*2*2*sizeof(uint32_t); wrdlen -= 2*2*2*sizeof(uint32_t), p += 2*2*2*sizeof(uint32_t)) {
		hash64 = (hash64 ^ (ROL64(*(unsigned long long *)(p+0),5-0)^*(unsigned long long *)(p+8))) * PRIME;
		hash64B = (hash64B ^ (ROL64(*(unsigned long long *)(p+8+8),5-0)^*(unsigned long long *)(p+8+8+8))) * PRIME;
	}

        hash64 = (hash64 ^ hash64B); // Some mix, the simplest is given, maybe the B-line should be rolled by 32bits before xoring.

	// Cases: 0,1,2,3,4,5,6,7,... 15,... 31
	if (wrdlen & (2*2*sizeof(uint32_t))) {
		hash64 = (hash64 ^ (ROL(*(uint32_t *)(p+0),5-0)^*(uint32_t *)(p+4))) * PRIME;		
		hash64 = (hash64 ^ (ROL(*(uint32_t *)(p+4+4),5-0)^*(uint32_t *)(p+4+4+4))) * PRIME;		
		p += 2*2*sizeof(uint32_t);
	}
	if (wrdlen & (2*sizeof(uint32_t))) {
		hash64 = (hash64 ^ (ROL(*(uint32_t *)p,5-0)^*(uint32_t *)(p+4))) * PRIME;		
		p += 2*sizeof(uint32_t);
	}
	if (wrdlen & sizeof(uint32_t)) {
		hash64 = (hash64 ^ *(uint32_t*)p) * PRIME;
		p += sizeof(uint32_t);
	}
	if (wrdlen & sizeof(uint16_t)) {
		hash64 = (hash64 ^ *(uint16_t*)p) * PRIME;
		p += sizeof(uint16_t);
	}
	if (wrdlen & 1) 
		hash64 = (hash64 ^ *p) * PRIME;
	
	return hash64 ^ (hash64 >> 32);
}
//---------------------------------------------------------------------------
// FNV1A_YoshimitsuTRIADiiXMM revision 1+ aka FNV1A_SaberFatigue, copyleft 2013-Apr-26 Kaze.
// Targeted purpose: x-gram table lookups for Leprechaun r17.
// Targeted machine: assuming SSE2 is present always - no non-SSE2 counterpart.
//#include <emmintrin.h> //SSE2
//#include <smmintrin.h> //SSE4.1
//#include <immintrin.h> //AVX
#define xmmload(p) _mm_load_si128((__m128i const*)(p))
#define xmmloadu(p) _mm_loadu_si128((__m128i const*)(p))
#define _rotl_KAZE128(x, n) _mm_or_si128(_mm_slli_si128(x, n) , _mm_srli_si128(x, 128-n))
#define _rotl_KAZE32(x, n) (((x) << (n)) | ((x) >> (32-(n))))
#define XMM_KAZE_SSE2
uint32_t FNV1A_Hash_YoshimitsuTRIADiiXMM(const char *str, uint32_t wrdlen)
{
    const uint32_t PRIME = 709607;
    uint32_t hash32 = 2166136261;
    uint32_t hash32B = 2166136261;
    uint32_t hash32C = 2166136261;
    const char *p = str;
    uint32_t Loop_Counter;
    uint32_t Second_Line_Offset;

#if defined(XMM_KAZE_SSE2) || defined(XMM_KAZE_SSE4) || defined(XMM_KAZE_AVX)
    __m128i xmm0;
    __m128i xmm1;
    __m128i xmm2;
    __m128i xmm3;
    __m128i xmm4;
    __m128i xmm5;
    __m128i hash32xmm = _mm_set1_epi32(2166136261);
    __m128i hash32Bxmm = _mm_set1_epi32(2166136261);
    __m128i hash32Cxmm = _mm_set1_epi32(2166136261);
    __m128i PRIMExmm = _mm_set1_epi32(709607);
#endif

#if defined(XMM_KAZE_SSE2) || defined(XMM_KAZE_SSE4) || defined(XMM_KAZE_AVX)
if (wrdlen >= 4*24) { // Actually 4*24 is the minimum and not useful, 200++ makes more sense.
    Loop_Counter = (wrdlen/(4*24));
    Loop_Counter++;
    Second_Line_Offset = wrdlen-(Loop_Counter)*(4*3*4);
    for(; Loop_Counter; Loop_Counter--, p += 4*3*sizeof(uint32_t)) {
	xmm0 = xmmloadu(p+0*16);
	xmm1 = xmmloadu(p+0*16+Second_Line_Offset);
	xmm2 = xmmloadu(p+1*16);
	xmm3 = xmmloadu(p+1*16+Second_Line_Offset);
	xmm4 = xmmloadu(p+2*16);
	xmm5 = xmmloadu(p+2*16+Second_Line_Offset);
#if defined(XMM_KAZE_SSE2)
		hash32xmm = _mm_mullo_epi16(_mm_xor_si128(hash32xmm , _mm_xor_si128(_rotl_KAZE128(xmm0,5) , xmm1)) , PRIMExmm);       
		hash32Bxmm = _mm_mullo_epi16(_mm_xor_si128(hash32Bxmm , _mm_xor_si128(_rotl_KAZE128(xmm3,5) , xmm2)) , PRIMExmm);        
		hash32Cxmm = _mm_mullo_epi16(_mm_xor_si128(hash32Cxmm , _mm_xor_si128(_rotl_KAZE128(xmm4,5) , xmm5)) , PRIMExmm);      
#else
		hash32xmm = _mm_mullo_epi32(_mm_xor_si128(hash32xmm , _mm_xor_si128(_rotl_KAZE128(xmm0,5) , xmm1)) , PRIMExmm);       
		hash32Bxmm = _mm_mullo_epi32(_mm_xor_si128(hash32Bxmm , _mm_xor_si128(_rotl_KAZE128(xmm3,5) , xmm2)) , PRIMExmm);        
		hash32Cxmm = _mm_mullo_epi32(_mm_xor_si128(hash32Cxmm , _mm_xor_si128(_rotl_KAZE128(xmm4,5) , xmm5)) , PRIMExmm);      
#endif
    }
#if defined(XMM_KAZE_SSE2)
	hash32xmm = _mm_mullo_epi16(_mm_xor_si128(hash32xmm , hash32Bxmm) , PRIMExmm);       
 	hash32xmm = _mm_mullo_epi16(_mm_xor_si128(hash32xmm , hash32Cxmm) , PRIMExmm);       
#else
	hash32xmm = _mm_mullo_epi32(_mm_xor_si128(hash32xmm , hash32Bxmm) , PRIMExmm);       
 	hash32xmm = _mm_mullo_epi32(_mm_xor_si128(hash32xmm , hash32Cxmm) , PRIMExmm);       
#endif
	hash32 = (hash32 ^ hash32xmm.m128i_u32[0]) * PRIME;
	hash32B = (hash32B ^ hash32xmm.m128i_u32[3]) * PRIME;
	hash32 = (hash32 ^ hash32xmm.m128i_u32[1]) * PRIME;
	hash32B = (hash32B ^ hash32xmm.m128i_u32[2]) * PRIME;
} else if (wrdlen >= 24)
#else
if (wrdlen >= 24)
#endif
{
    Loop_Counter = (wrdlen/24);
    Loop_Counter++;
    Second_Line_Offset = wrdlen-(Loop_Counter)*(3*4);
    for(; Loop_Counter; Loop_Counter--, p += 3*sizeof(uint32_t)) {
		hash32 = (hash32 ^ (_rotl_KAZE32(*(uint32_t *)(p+0),5) ^ *(uint32_t *)(p+0+Second_Line_Offset))) * PRIME;        
		hash32B = (hash32B ^ (_rotl_KAZE32(*(uint32_t *)(p+4+Second_Line_Offset),5) ^ *(uint32_t *)(p+4))) * PRIME;        
		hash32C = (hash32C ^ (_rotl_KAZE32(*(uint32_t *)(p+8),5) ^ *(uint32_t *)(p+8+Second_Line_Offset))) * PRIME;        
    }
		hash32 = (hash32 ^ _rotl_KAZE32(hash32C,5) ) * PRIME;
} else {
    // 1111=15; 10111=23
    if (wrdlen & 4*sizeof(uint32_t)) {	
		hash32 = (hash32 ^ (_rotl_KAZE32(*(uint32_t *)(p+0),5) ^ *(uint32_t *)(p+4))) * PRIME;        
		hash32B = (hash32B ^ (_rotl_KAZE32(*(uint32_t *)(p+8),5) ^ *(uint32_t *)(p+12))) * PRIME;        
		p += 8*sizeof(uint16_t);
    }
    // Cases: 0,1,2,3,4,5,6,7,...,15
    if (wrdlen & 2*sizeof(uint32_t)) {
		hash32 = (hash32 ^ *(uint32_t*)(p+0)) * PRIME;
		hash32B = (hash32B ^ *(uint32_t*)(p+4)) * PRIME;
		p += 4*sizeof(uint16_t);
    }
    // Cases: 0,1,2,3,4,5,6,7
    if (wrdlen & sizeof(uint32_t)) {
		hash32 = (hash32 ^ *(uint16_t*)(p+0)) * PRIME;
		hash32B = (hash32B ^ *(uint16_t*)(p+2)) * PRIME;
		p += 2*sizeof(uint16_t);
    }
    if (wrdlen & sizeof(uint16_t)) {
        hash32 = (hash32 ^ *(uint16_t*)p) * PRIME;
        p += sizeof(uint16_t);
    }
    if (wrdlen & 1) 
        hash32 = (hash32 ^ *p) * PRIME;
}
    hash32 = (hash32 ^ _rotl_KAZE32(hash32B,5) ) * PRIME;
    return hash32 ^ (hash32 >> 16);
}
//---------------------------------------------------------------------------
