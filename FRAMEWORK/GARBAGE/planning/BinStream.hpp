
#ifndef BinStreamH
#define BinStreamH

#pragma once
//---------------------------------------------------------------------------
/*
  BYTE Type    : 3;  // Data Type (0 - 7)
  BYTE NameLen : 5;  // Max name len = 31  // 0 is for unnamed props

  BYTE Extra: 1;  // If set, then there one byte of extra size follows (Max 4096) // Used for long strings
  BYTE Size : 4;  // Unused for PropBags


*/

struct SBinStream
{



PVOID GetPointer(PUINT Size)
{

}


};
//---------------------------------------------------------------------------
#endif
