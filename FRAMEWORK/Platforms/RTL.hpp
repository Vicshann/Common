
#pragma once

extern "C" void __cdecl _purecall(void){}

//---------------------------------------------------------------------------
extern "C" void*  __cdecl memset(void* _Dst, int _Val, NFWK::size_t _Size)      // TODO: Aligned, SSE by MACRO
{
 NFWK::size_t ALen = _Size/sizeof(NFWK::size_t);
 NFWK::size_t BLen = _Size%sizeof(NFWK::size_t);
 NFWK::size_t DVal =_Val & 0xFF;               // Bad and incorrect For x32: '(size_t)_Val * 0x0101010101010101;'         // Multiply by 0x0101010101010101 to copy the lowest byte into all other bytes
 if(DVal)
  {
   DVal = _Val | (_Val << 8) | (_Val << 16) | (_Val << 24);
#ifdef _ARCH_X64
   DVal |= DVal << 32;
#endif
  }
 for(NFWK::size_t ctr=0;ctr < ALen;ctr++)((NFWK::size_t*)_Dst)[ctr] = DVal;
 for(NFWK::size_t ctr=(ALen*sizeof(NFWK::size_t));ctr < _Size;ctr++)((char*)_Dst)[ctr] = DVal;
 return _Dst;
}
//---------------------------------------------------------------------------

