

// TODO: Get rid of memory allocations, templatize to use a static number sizes
// TODO: Support native word sizes (8 bytes on x64)
struct NMontgomery
{
/*  // WordArr is a bit array, stored as WORDs
0   15 - 0     L
1   31 - 16
2   47 - 32
3   63 - 48    H
...
*/
struct WordArr
{
 PWORD DataPtr;
 WORD  WordsCtr;

 WordArr(void)
  {
   DataPtr = NULL;
   WordsCtr = 0;
  }
};

//---------------------------------------------------------------------------
static void __fastcall ResizeWordArray(WordArr *aWordArr, int aSize)
{
 aWordArr->WordsCtr = aSize;
 if(aWordArr->DataPtr)aWordArr->DataPtr = (PWORD)HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,aWordArr->DataPtr,(aSize*2)+8);
   else aWordArr->DataPtr = (PWORD)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,(aSize*2)+8);
}
//---------------------------------------------------------------------------
// Shift bits with carry bit  // Carry goes into D0 of a first WORD
static void __fastcall ShiftArrBitsLeft(WordArr *Arr, bool Carry)
{
 for(UINT v16 = 0, i = Carry; v16 < Arr->WordsCtr; v16++, i >>= 16) // After 'i >>= 16' 'i' will become an one bit value of bit 15 from last (Arr->DataPtr[v16] << 1)
  {
   i |= Arr->DataPtr[v16] << 1;   // *2   // Will take bit 15 of Arr->DataPtr[v16] into high half of 'i'
   Arr->DataPtr[v16] = i;   // Store the shifted WORD
  }
}
//---------------------------------------------------------------------------
static void __fastcall SubtractArrAsBigUInt(WordArr *DatArr, WordArr *KeyArr)
{
 for(UINT v33 = 0, l = 0; l < DatArr->WordsCtr; ++l)
  {
   UINT v35 = (l < KeyArr->WordsCtr)?(KeyArr->DataPtr[l]):(0);  // Take WORD from KeyArr or 0 if KeyArr is smaller
   UINT v36 = v35 + v33;     // Carry from last subtraction
   v33  = DatArr->DataPtr[l] < v36;    // True if from Smaller subtracting Greater
   DatArr->DataPtr[l] -= v36;
  }
}
//---------------------------------------------------------------------------
// Subtracts only a Smaller from Greater and does one shift left
//
static void __fastcall SubtractArraysEx(WordArr *ArrA, WordArr *ArrB, WordArr *ArrC)
{
 UINT v22 = ArrB->WordsCtr - 1;   // Start from High bits
 if(v22 < ArrC->WordsCtr)return;  // Cant work with that
 while(!ArrB->DataPtr[v22])   // Until first bits with some value
  {
   v22--;
   if(v22 < ArrC->WordsCtr )   // Must match
	{
	 if( ArrC->DataPtr[v22] > ArrB->DataPtr[v22] ){ShiftArrBitsLeft(ArrA, 0); return;}   // Will not subtract a Greater from a Smaller value
	 while(v22 && (ArrC->DataPtr[v22] >= ArrB->DataPtr[v22]))   // Only '=' in fact is tolerated
	  {
	   v22--;
	   if ( ArrC->DataPtr[v22] > ArrB->DataPtr[v22] ){ShiftArrBitsLeft(ArrA, 0); return;}   // Will not subtract a Greater from a Smaller value
	  }
	 break;
	}
  }
 ShiftArrBitsLeft(ArrA, 1);
 SubtractArrAsBigUInt(ArrB, ArrC);
}
//---------------------------------------------------------------------------
// Shifts aInputArr into aOutArrB and subtracts aKeyArrB from aOutArrB after each shift (Onlo Amaller from Greater)
// aOutArrA is a bitmas of each subtraction done
//
// aArrArgA First time is same as BlkBody but 17 words (Rest are zero)
// aKeyArrB is never changes
// aOutArrA = Shift result  (Bitmask of each subtraction done)
// aOutArrB = Subtract result
//
static void __fastcall ShiftInDataWithKey(WordArr *aInputArr, WordArr *aKeyArrB, WordArr *aOutArrA, WordArr *aOutArrB)
{
 memset(aOutArrA->DataPtr,0,2*aOutArrA->WordsCtr);
 memset(aOutArrB->DataPtr,0,2*aOutArrB->WordsCtr);
 for(DWORD vWordsCtrA=aInputArr->WordsCtr; vWordsCtrA;)   // For each WORD of input
  {
   --vWordsCtrA;            // From High to Low bits
   int vBitIdx = 16;
   do
	{
	 --vBitIdx;
	 ShiftArrBitsLeft(aOutArrB, (aInputArr->DataPtr[vWordsCtrA] >> vBitIdx) & 1);     // From High to Low bit   // Shift in one bit from InputArr into OutputArr
	 if(aOutArrB->WordsCtr >= aKeyArrB->WordsCtr)      // Ensures a positive result of substraction?     // Without thih the function would be simple shifting Input data in Output Array from Low to high
	  {
	   if(aOutArrB->WordsCtr > aKeyArrB->WordsCtr)SubtractArraysEx(aOutArrA, aOutArrB, aKeyArrB);  // Will be: aOutArrB -= aKeyArrB
      }
	   else SubtractArraysEx(aOutArrA, aKeyArrB, aOutArrB);  // Less  // Never happens?   // Will be: aKeyArrB -= aOutArrB
	}
	while(vBitIdx);
  }
}
//---------------------------------------------------------------------------
static void __fastcall CopyWordArr(WordArr *DstArr, WordArr *SrcArr)
{
 UINT Len = (DstArr->WordsCtr > SrcArr->WordsCtr)?(SrcArr->WordsCtr):(DstArr->WordsCtr);
 memcpy(DstArr->DataPtr,SrcArr->DataPtr,Len*2);
}
//---------------------------------------------------------------------------
static void __fastcall MoveWordArr(WordArr *DstArr, WordArr *SrcArr) // Zeroes Src array
{
 SrcArr->DataPtr[SrcArr->WordsCtr] = 0;
 UINT Len = (DstArr->WordsCtr > SrcArr->WordsCtr)?(SrcArr->WordsCtr):(DstArr->WordsCtr);
 memcpy(DstArr->DataPtr,SrcArr->DataPtr,Len*2);
 memset(SrcArr->DataPtr,0,Len*2);
}
//---------------------------------------------------------------------------
// Reassigns and clears ArrB
// Puts ArrA into ArrC
static void __fastcall ReassignWArrays(WordArr *ArrA, WordArr *ArrB, WordArr *ArrC)
{
 ArrB->WordsCtr = ArrA->WordsCtr;
 if( ArrA->WordsCtr > 0 )memset(ArrB->DataPtr,0,ArrA->WordsCtr*2);   
 if( ArrA->WordsCtr != ArrC->WordsCtr )ResizeWordArray(ArrC, ArrA->WordsCtr);    // Alreay zeroed
 CopyWordArr(ArrC, ArrA);
}
//---------------------------------------------------------------------------
// ArrA is Dest here
// Multiplies ArrB * ArrC and adds result to ArrA
//
static void __fastcall BigUIntMultAdd(WordArr *ArrA, WordArr *ArrB, WordArr *ArrC)
{
 if(!ArrA->WordsCtr)return;
 for(UINT v51 = 0;v51 < ArrA->WordsCtr;v51++)  // For each element of Dest
  {
   for(UINT m = 0; m < ArrC->WordsCtr;m++)   // Use each element of ArrC for alculation
	{
	 UINT v54 = ArrB->DataPtr[v51] * ArrC->DataPtr[m];  // 65536 * 65535 is in range of DWORD
	 for(UINT n = v51 + m; v54; n++)
	  {
	   UINT v57 = ArrA->DataPtr[n] + v54;   // UINT because overflow of WORD is required
	   ArrA->DataPtr[n] = v57;
	   v54  = v57 >> 16;
	  }
	}
  }
}
//---------------------------------------------------------------------------
// Montgomery
// Input numbers are expected in LittleEndian format
//
static void __fastcall MontgomeryExponentiate(WordArr *aValue, WordArr *aExponent, WordArr *aModulus, WordArr *aOutArr)
{
 WordArr vWordBufA;
 WordArr vWordBufB;
 WordArr vWordBufC;
 WordArr vWordBufD;

 vWordBufA.WordsCtr = 2 * aValue->WordsCtr + 1;
 UINT FullLen = (vWordBufA.WordsCtr + 1);     // TODO: Protect for encryption with randomized size
 vWordBufA.DataPtr  = (WORD *)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,FullLen * 2);
 vWordBufD.WordsCtr = vWordBufA.WordsCtr;
 vWordBufD.DataPtr  = (WORD *)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,FullLen * 2);
 vWordBufB.WordsCtr = vWordBufA.WordsCtr;
 vWordBufB.DataPtr  = (WORD *)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,FullLen * 2);
 vWordBufC.WordsCtr = vWordBufA.WordsCtr;
 vWordBufC.DataPtr  = (WORD *)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,FullLen * 2);
 *vWordBufA.DataPtr = 1;

 UINT v69 = 1;
 for(int v23=aExponent->WordsCtr - 1;v23 >= 0;v23--)
  {
   for(UINT BCtr = 0;BCtr < 16;BCtr++)
	{
	 if( !v69 )     // Round Transform   // Transforms existing data in Crypto Arrays     // If already had some bit 1 previosly (Will be done for rest of bits)
	  {
	   CopyWordArr(&vWordBufD, &vWordBufA);
	   MoveWordArr(&vWordBufB, &vWordBufA);
	   BigUIntMultAdd(&vWordBufA, &vWordBufB, &vWordBufD);
	   ReassignWArrays(&vWordBufA, &vWordBufB, &vWordBufC);   // vWordBufC is output (Copied from vWordBufA) and vWordBufB is set to zero
	   ShiftInDataWithKey(&vWordBufC, aModulus, &vWordBufB, &vWordBufA);   // vWordBufB and vWordBufA is output   // vWordBufB is not used!!!!!!!!!!!!
	  }
	 if( aExponent->DataPtr[v23] & (0x8000 >> BCtr) )   // Transform in SrcData   // Adds user`s data to Crypto Arrays if Exponent bit is 1  // Test each bit, from high to low
	  {
	   MoveWordArr(&vWordBufB, &vWordBufA);
	   BigUIntMultAdd(&vWordBufA, &vWordBufB, aValue);    // vWordBufA is Dest
	   ReassignWArrays(&vWordBufA, &vWordBufB, &vWordBufC);   // vWordBufC is output (Copied from vWordBufA) and vWordBufB is set to zero
	   ShiftInDataWithKey(&vWordBufC, aModulus, &vWordBufB, &vWordBufA);  // vWordBufB and vWordBufA is output    // vWordBufB is not used!!!!!!!!!!!!
	   v69 = 0;
	  }
	}
  }
//-------------------------------------
 if(vWordBufA.WordsCtr != aOutArr->WordsCtr )
  {
   ResizeWordArray(aOutArr, vWordBufA.WordsCtr);
   memset(aOutArr->DataPtr,0,aOutArr->WordsCtr*2);
   CopyWordArr(aOutArr, &vWordBufA);
  }
   else CopyWordArr(aOutArr, &vWordBufA);
  HeapFree(GetProcessHeap(),0,vWordBufC.DataPtr);
  HeapFree(GetProcessHeap(),0,vWordBufB.DataPtr);
  HeapFree(GetProcessHeap(),0,vWordBufD.DataPtr);
  HeapFree(GetProcessHeap(),0,vWordBufA.DataPtr); // Not Actual // Hook operator_delete and watch for blocks of 36 bytes in size // ??_V@YAXPEAX@Z from MSVCR120.dll
}
//---------------------------------------------------------------------------
static void __fastcall Exponentiate(PBYTE aValue, PBYTE aExponent, PBYTE aModulus, PBYTE aOutArr, UINT NumSizeInBytes, bool IsBigEndianNums)
{
 WordArr ArrValue;
 WordArr ArrExponent;
 WordArr ArrModulus;
 WordArr ArrResult;
 ResizeWordArray(&ArrValue, NumSizeInBytes/2);
 ResizeWordArray(&ArrExponent, NumSizeInBytes/2);
 ResizeWordArray(&ArrModulus, NumSizeInBytes/2);
 ResizeWordArray(&ArrResult, NumSizeInBytes/2);
 memcpy(ArrValue.DataPtr,aValue,NumSizeInBytes);
 memcpy(ArrExponent.DataPtr,aExponent,NumSizeInBytes);
 memcpy(ArrModulus.DataPtr,aModulus,NumSizeInBytes);
 if(IsBigEndianNums)
  {
   ReverseBytes((PBYTE)ArrValue.DataPtr, NumSizeInBytes);
   ReverseBytes((PBYTE)ArrExponent.DataPtr, NumSizeInBytes);
   ReverseBytes((PBYTE)ArrModulus.DataPtr, NumSizeInBytes);
  }
 MontgomeryExponentiate(&ArrValue, &ArrExponent, &ArrModulus, &ArrResult);
 if(IsBigEndianNums)ReverseBytes((PBYTE)ArrResult.DataPtr, NumSizeInBytes);
 memcpy(aOutArr, ArrResult.DataPtr, NumSizeInBytes);
}
//---------------------------------------------------------------------------

};
