
//====================================================================================
#pragma once
 
// This is free and unencumbered software released into the public domain.
//
// Anyone is free to copy, modify, publish, use, compile, sell, or distribute this software, either in source code form or as a compiled
// binary, for any purpose, commercial or non-commercial, and by any means.
//
// In jurisdictions that recognize copyright laws, the author or authors of this software dedicate any and all copyright interest in the
// software to the public domain. We make this dedication for the benefit of the public at large and to the detriment of our heirs and
// successors. We intend this dedication to be an overt act of relinquishment in perpetuity of all present and future rights to this
// software under copyright law.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
// OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.

// For more information, please refer to <http://unlicense.org/>
//------------------------------------------------------------------------------------

#include "Common\ThirdParty\MiniDDK\include\hidclass.h"
#include "Common\ThirdParty\MiniDDK\include\hidpi.h"


#pragma pack(push,1)
enum EHidCDescFlg {cdfIsAbsolute=0x08, cdfIsRange=0x10, cdfIsAlias=0x20, cdfIsStringRange=0x40, cdfIsDesignatorRange=0x80};
struct SHID_VCAPS
{
/*2C*/ WORD  UsagePage;
/*2D*/ UCHAR Unknown0;
/*2E*/ UCHAR ReportID;
/*2F*/ UCHAR Unknown1;
/*30*/ WORD  BitSize;
/*32*/ WORD  ReportCount;     // if !(Flags & 0x10) 
/*34*/ DWORD Unknown2;
/*38*/ WORD  BitField;
/*3A*/ DWORD Unknown3;
/*3E*/ WORD  LinkCollection; 
/*40*/ WORD  LinkUsagePage;
/*42*/ WORD  LinkUsage;
/*44*/ DWORD Flags;
/*48*/ DWORD Unknown[8];
/*68*/ WORD  UsageMin;
/*6A*/ WORD  UsageMax;
/*6C*/ WORD  StringMin;
/*6E*/ WORD  StringMax;
/*70*/ WORD  DesignatorMin;
/*72*/ WORD  DesignatorMax;
/*74*/ WORD  DataIndexMin;
/*76*/ WORD  DataIndexMax;
/*78*/ UCHAR HasNull;
/*79*/ UCHAR Unknown4;
/*7A*/ UCHAR Unknown5;
/*7B*/ UCHAR Unknown6;
/*7C*/ DWORD LogicalMin;
/*80*/ DWORD LogicalMax;
/*84*/ DWORD PhysicalMin;
/*88*/ DWORD PhysicalMax;
/*8C*/ DWORD Units;
/*90*/ DWORD UnitsExp;
};

struct SHID_REPORT_DESC
{
 WORD FirstIdx;  // 0000
 WORD UnknwnA;   // 1E00
 WORD Total;     // 1800
 WORD UnknwnB;   // 1900
};

struct SHID_COLLECTION_DESCRIPTOR
{
/*00*/ DWORD MrkA;   // 'PdiH'  // "HidP"
/*04*/ DWORD MrkB;   // 'RDK '  // " KDR"
/*08*/ WORD  UnknwnA[4];
/*10*/ SHID_REPORT_DESC RepDesc[3];  // HIDP_REPORT_TYPE: HidP_Input, HidP_Output, HidP_Feature
/*28*/ DWORD UnknwnB;
/*2C*/ SHID_VCAPS Array[0]; // All structures to which ranges refers SHID_REPORT_DESC::FirstIdx and SHID_REPORT_DESC::Total
};

struct SHIDP_PREPARSED_BUF
{
 PVOID pHidD_Hello;
 SHID_COLLECTION_DESCRIPTOR* Data;   // HIDP_PREPARSED_DATA is &HIDP_PREPARSED_BUF::Data
};
#pragma pack(pop)

