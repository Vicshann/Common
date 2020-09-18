
#pragma once
/*
  Copyright (c) 2020 Victor Sheinmann, Vicshann@gmail.com

  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), 
  to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
  and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. 
*/

//---------------------------------------------------------------------------
#include <Windows.h>
#include <intrin.h>
#include <emmintrin.h>        // for _mm_storeu_si128
#include "ntdll.h"

#include "Utils.h"

extern char* _cdecl gcvt(double f, size_t ndigit, char* buf);

struct NCMN
{
template<typename T> inline static long  AddrToRelAddr(T CmdAddr, UINT CmdLen, T TgtAddr){return -((CmdAddr + CmdLen) - TgtAddr);}
template<typename T> inline static T     RelAddrToAddr(T CmdAddr, UINT CmdLen, long TgtOffset){return ((CmdAddr + CmdLen) + TgtOffset);}

template <typename T> inline static T RotL(T Value, unsigned char Shift){return (Value << Shift) | (Value >> ((sizeof(T) * 8U) - Shift));}
template <typename T> inline static T RotR(T Value, unsigned char Shift){return (Value >> Shift) | (Value << ((sizeof(T) * 8U) - Shift));}

template<typename N, typename M> inline static M NumToPerc(N Num, M MaxVal){return (((Num)*100)/(MaxVal));}
template<typename P, typename M> inline static M PercToNum(P Per, M MaxVal){return (((Per)*(MaxVal))/100);}                         

template<class N, class M> inline static M AlignFrwd(N Value, M Alignment){return ((((Value)/(Alignment))+((bool)((Value)%(Alignment))))*(Alignment));}
template<class N, class M> inline static M AlignBkwd(N Value, M Alignment){return (((Value)/(Alignment))*(Alignment));}

// 2,4,8,16,...
template<typename N> inline static N AlignP2Frwd(N Value, unsigned int Alignment){return (Value+(Alignment-1)) & ~(Alignment-1);}
template<typename N> inline static N AlignP2Bkwd(N Value, unsigned int Alignment){return Value & ~(Alignment-1);}

//---------------------------------------------------------------------------
#include "UTF.hpp"
#include "HDE.hpp"
#include "StrUtils.hpp"
#include "FormatPE.hpp"
//#include "BdsCompat.hpp"         // Rarely used. Let it be outside for now
#include "CompileTime.hpp"
#include "InjDllLdr.hpp"
#include "NtDllEx.hpp"
#include "UniHook.hpp"
#include "Patcher.hpp"
#include "ShMemIPC.hpp"
#include "GhostDbg.hpp"

#include "MiniString.h"
#include "FileStream.h"
#include "json.h"
//#include "Base64.hpp"
#include "BaseP2.hpp"

#ifndef _AMD64_
#include "wow64ext.hpp"
#endif
};
//---------------------------------------------------------------------------

typedef NCMN::CJSonItem CJSonItem;
typedef NCMN::CMiniStr  CMiniStr;
typedef NCMN::CJSonItem CJSonItem;

typedef NCMN::NShMem NShMem;
typedef NCMN::NGhDbg NGhDbg;
                              
typedef NCMN::NInjLdr NInjLdr;
typedef NCMN::NUNIHK  NUNIHK;      
typedef NCMN::NNTDLL  NNTDLL;      
typedef NCMN::NPEFMT  NPEFMT; 
typedef NCMN::NSTR    NSTR;
typedef NCMN::NCTM    NCTM;
typedef NCMN::NUTF    NUTF;
typedef NCMN::NSIGP   NSIGP;
typedef NCMN::NSTR    NSTR;
//typedef NCMN::NBase64 NBase64;
typedef NCMN::NBP2::NBase64 NBase64;
typedef NCMN::NBP2 NBP2;
#ifndef _AMD64_
typedef NCMN::NWOW64E NWOW64E;
#endif
//---------------------------------------------------------------------------
