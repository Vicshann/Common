
#pragma once

/*
  Copyright (c) 2018 Victor Sheinmann, Vicshann@gmail.com

  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), 
  to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
  and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. 
*/

// TODO: Use Platform endianess by default
template<int NumOfBits, bool IsSignedInt, typename ElemType=unsigned int> class CBigInt
{
 static const unsigned int NumOfParts = (NumOfBits / sizeof(ElemType))+(bool)(NumOfBits % sizeof(ElemType));   // static_assert(NumOfBits >= (sizeof(ElemType)*8), "NumOfBits is too small for ElemType!" );

 ElemType IntParts[NumOfParts];

static constexpr bool IsPlatformLE(void){int val=1; return *(char*)&val;}   // Old style: #define ISLE(((union { unsigned x; unsigned char c; }){1}).c)  // Or #if 'ABCD' == 0x41424344

ElemType __cdecl ReverseBytes64(ElemType Val)  
{
 ElemType res;
 for(int ctr=0;ctr < sizeof(ElemType);ctr++)((unsigned char*)&res)[ctr] = ((unsigned char*)&Val)[sizeof(ElemType)-1-ctr];
 return res;
} 
//------------------------------------------------------------------------------------------------------------
static void ReverseCopy(ElemType* Dst, ElemType* Src, unsigned int Count)   // Platform dependant!  // Cross platform compile time way to determine endianess?
{
 for(unsigned int ctr=0;ctr < Count;ctr++)Dst[(Count-1)-ctr] = ReverseBytes64(Src[ctr]); 
}
//------------------------------------------------------------------------------------------------------------
static void BigAddU(ElemType* DstNum, ElemType* SrcNum)
{
 bool HaveCarry = false;
 for(unsigned int ctr=0;ctr < NumOfParts;ctr++) 
  {
   ElemType DVal = DstNum[ctr];
   ElemType DSum = DVal + (ElemType)HaveCarry;
   if(DVal <= DSum)
    {
     ElemType SVal = SrcNum[ctr]; 
     DstNum[ctr] = SVal + DSum;
     HaveCarry = SVal > (SVal + DSum);             
    }
     else DstNum[ctr] = SrcNum[ctr];       
  }
}
//------------------------------------------------------------------------------------------------------------
static void BigSubU(ElemType* DstNum, ElemType* SrcNum)
{
 bool HaveCarry = false;   
 for(unsigned int ctr=0;ctr < NumOfParts;ctr++) 
  {
   ElemType DSVal = DstNum[ctr] - (ElemType)HaveCarry;
   if(DSVal <= ~(ElemType)HaveCarry)                      
    {
     ElemType SVal = SrcNum[ctr];
     ElemType RVal = DSVal - SVal;                
     DstNum[ctr] = RVal;
     HaveCarry = RVal > ~SVal;  
    }
     else DstNum[ctr] = ~SrcNum[ctr];
  }  
}
//------------------------------------------------------------------------------------------------------------
static unsigned int BigCountValParts(ElemType* BigNum)  // How many parts(ElemType) with value the number have 
{
 int Ctr = NumOfParts;
 for(;Ctr > 0;Ctr--)
  {
   if(BigNum[Ctr-1])break;
  }
 return Ctr;
}
//------------------------------------------------------------------------------------------------------------
static int BigCmpU(ElemType* Num, ElemType* Tgt)
{
 for(int ctr=NumOfParts-1;ctr >= 0;ctr--)
  {
   ElemType Val = Tgt[ctr];
   if(Num[ctr] > Val)return 1;    // Greater
   if(Num[ctr] < Val)return -1;   // Less
  }
 return 0;  // Equal
}
//------------------------------------------------------------------------------------------------------------
static void BigDivRemU(ElemType* DstNum, ElemType* SrcNum)        // DstNum = DstNum % SrcNum
{
 for(;;)
  {
   if(BigCmpU(DstNum, SrcNum) < 0)break;
   BigSubU(DstNum, SrcNum);
  }
}
//------------------------------------------------------------------------------------------------------------
static void BigPower(ElemType* BigMulA, ElemType* BigMulB, ElemType* BigMod)
{
 ElemType TmpBuf[NumOfParts];
 ElemType WrkBuf[NumOfParts];
 memset(WrkBuf, 0, sizeof(WrkBuf));
 memcpy(TmpBuf, BigMulB, sizeof(TmpBuf));
 if(unsigned int VPtCnt = BigCountValParts(BigMulA))
  {
   for(unsigned int Idx=0;Idx < VPtCnt;Idx++)       
    {
     ElemType Value = BigMulA[Idx];
     for(unsigned int Ctr=0;Ctr < NumOfParts;Ctr++,Value >>= 1) 
      {
       if(!Value && ((Idx+1) >= VPtCnt))break;                
       if(Value & 1)
        {
         BigAddU(WrkBuf, TmpBuf);         // res = (res*Value) % Modulus; 
         BigDivRemU(WrkBuf, BigMod);
        }
       BigAddU(TmpBuf, TmpBuf);    // pow    // Value = (Value*Value) % Modulus;
       BigDivRemU(TmpBuf, BigMod);          
      }    
    }
  }
 memcpy(BigMulB, WrkBuf, sizeof(WrkBuf));
}
//------------------------------------------------------------------------------------------------------------
static void BigPowMod(unsigned char* aValue, unsigned char* aExponent, unsigned char* aModulus, unsigned char* aDstBuf)
{
 ElemType vSrcValue[NumOfParts]; 
 ElemType vExponent[NumOfParts]; 
 ElemType vModulus[NumOfParts]; 
 ElemType vBigRes[NumOfParts];
  
 ReverseCopy(vSrcValue, (ElemType*)aValue, NumOfParts);         // License
 ReverseCopy(vExponent, (ElemType*)aExponent, NumOfParts);
 ReverseCopy(vModulus,  (ElemType*)aModulus, NumOfParts);

 memset(&vBigRes,0,sizeof(vBigRes)); 
 vBigRes[0] = 1;            // Make Big One
 if(unsigned int DParts = BigCountValParts(vExponent))    // Anything ^ 0 = 1
  {
   for(unsigned int CtrA=0;CtrA < DParts;CtrA++)    // Valued parts of exponent
    {
     bool Flag = (CtrA+1) < DParts;  
     ElemType ExpVal = vExponent[CtrA];
     for(unsigned int CtrB=0;(ExpVal || Flag) && (CtrB < (sizeof(ElemType)*8));CtrB++,ExpVal >>= 1)   // ExpVal /= 2    // ExpCtr (Number of bits in ElemType)
      {
       if(ExpVal & 1)BigPower(vSrcValue, vBigRes, vModulus);       
       BigPower(vSrcValue, vSrcValue, vModulus);
      }
    }
  }
 ReverseCopy((ElemType*)aDstBuf, vBigRes, NumOfParts);
}
//------------------------------------------------------------------------------------------------------------

public:
//------------------------------------------------------------------------------------------------------------
CBigInt(void):IntParts(0) {}
//------------------------------------------------------------------------------------------------------------
void PowMod(unsigned char* aValue, unsigned char* aExponent, unsigned char* aModulus, unsigned char* aDstBuf)
{
 BigPowMod(aValue, aExponent, aModulus, aDstBuf);
}
//------------------------------------------------------------------------------------------------------------


};
//------------------------------------------------------------------------------------------------------------

