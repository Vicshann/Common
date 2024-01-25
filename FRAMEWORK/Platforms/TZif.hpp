
#pragma once

//---------------------------------------------------------------------------
// https://man7.org/linux/man-pages/man5/tzfile.5.html
// https://datatracker.ietf.org/doc/html/rfc8536
// https://github.com/Boruch-Baum/zdump-3-/
// http://www.iana.org/time-zones
// Linux/BSD: etc/localtime
//
struct STZF
{
 static const inline uint32 TZFID = 0x545A6966;  // TZif
#pragma pack(push, 1)
struct SLTT    // local time type record
{
 int32 utoff;  // Number of seconds to be added to UT in order to determine local time  [-24:59:59, 25:59:59]
 uint8 dst;    // indicating whether local time should be considered Daylight Saving Time (DST)
 uint8 idx;    // unsigned integer specifying a zero-based index into the series of time zone designation octets
};

template<typename T=uint32> struct SLSRec
{
 T     occur;    // UNIX leap time value specifying the time at which a leap-second correction occurs
 int32 corr;     // integer specifying the value of LEAPCORR on or after the occurrence
};

// In the version 1 data block, time values are 32 bits. In the version 2+ data block, present only in version 2 and3 files, time values are 64 bits
struct SHdrTZ       // NOTE: Misalignment will be on each access!
{
 achar    magicnumber[5];  // = TZif2 or TZif\0
 achar    reserved[15];    // nulls
 uint32   isutcnt;         // number of UTC/local indicators stored in the file.      // MUST either be zero or equal to "typecnt".
 uint32   isstdcnt;        // number of standard/wall indicators stored in the file.   // MUST either be zero or equal to "typecnt".
 uint32   leapcnt;         // number of leap seconds for which data is stored in the file.
 uint32   timecnt;         // number of "transition times" for which data is stored in the file.
 uint32   typecnt;         // number of "local time types" for which data is stored in the file (must not be zero).
 uint32   charcnt;         // number of  characters  of  "timezone  abbreviation  strings" stored in the file.
// Not part of the file:
 uint32   TimeSize;
 union
  {
 uint64*  TransTimes;
 uint32*  TransTimes32; // timecnt x sizeof(int32)  // TIME_SIZE
  };
 uint8*   TransTypes;   // timecnt
 SLTT*    LTTRecs;      // typecnt x 6
 achar*   TZDesigns;    // charcnt
 vptr     LeapSecRecs;  // leapcnt x (TIME_SIZE + 4)  // SLSRec*
 uint8*   StdWallIndic; // isstdcnt
 uint8*   UTLocIndic;   // isutcnt
 size_t   Version;
};
#pragma pack(pop)
//---------------------------------------------------------------------------
template<typename T> static T ReadField(void* ptr)
{
 if constexpr(!NCFG::IsBigEnd)return RevByteOrder<T>(*(T*)ptr);
  else return *(T*)ptr;
}
template<typename T> static T ReadField(void* ptr, uint& offs)
{
 T res = ReadField<T>((uint8*)ptr + offs);
 offs += sizeof(T);
 return res;
}
//---------------------------------------------------------------------------
static uint8* ParseHdrTZF(SHdrTZ* hdr, void* tzf)
{
 if(ReadField<uint32>(tzf) != TZFID)return nullptr;

 hdr->isutcnt  = ReadField<uint32>(&((SHdrTZ*)tzf)->isutcnt);
 hdr->isstdcnt = ReadField<uint32>(&((SHdrTZ*)tzf)->isstdcnt);

 hdr->leapcnt  = ReadField<uint32>(&((SHdrTZ*)tzf)->leapcnt);
 hdr->timecnt  = ReadField<uint32>(&((SHdrTZ*)tzf)->timecnt);
 hdr->typecnt  = ReadField<uint32>(&((SHdrTZ*)tzf)->typecnt);
 hdr->charcnt  = ReadField<uint32>(&((SHdrTZ*)tzf)->charcnt);

 uint8 ver  = ((uint8*)tzf)[4];
 hdr->TimeSize = ver?sizeof(int64):sizeof(int32);
 hdr->Version  = ver?(ver - '0'):1;
 return (uint8*)&((SHdrTZ*)tzf)->TimeSize;
}
//---------------------------------------------------------------------------
template<typename T> static uint8* ParseBodyTZF(SHdrTZ* hdr, uint8* Ptr)
{
 if(hdr->timecnt)hdr->TransTimes    = (uint64*)Ptr; Ptr += sizeof(T) * hdr->timecnt;    // Always 32bit, take full time values from extended header
 if(hdr->timecnt)hdr->TransTypes    = Ptr;          Ptr += sizeof(uint8) * hdr->timecnt;
 if(hdr->typecnt)hdr->LTTRecs       = (SLTT*)Ptr;   Ptr += sizeof(SLTT) * hdr->typecnt;
 if(hdr->charcnt)hdr->TZDesigns     = (achar*)Ptr;  Ptr += sizeof(uint8) * hdr->charcnt;
 if(hdr->leapcnt)hdr->LeapSecRecs   = Ptr;          Ptr += ((hdr->Version > 1)?sizeof(SLSRec<uint64>):sizeof(SLSRec<uint32>)) * hdr->leapcnt;
 if(hdr->isstdcnt)hdr->StdWallIndic = Ptr;          Ptr += sizeof(uint8) * hdr->isstdcnt;
 if(hdr->isutcnt)hdr->UTLocIndic    = Ptr;          Ptr += sizeof(uint8) * hdr->isutcnt;
 return Ptr;
}
//---------------------------------------------------------------------------
static sint ParseTZF(SHdrTZ* hdr, void* tzf)
{
 uint8* Ptr = ParseHdrTZF(hdr, tzf);
 if(!Ptr)return -1;
 Ptr = ParseBodyTZF<uint32>(hdr, Ptr);
 Ptr = ParseHdrTZF(hdr, Ptr);    // Counters expected to be same as in V1 header
 if(!Ptr)return sint(hdr->Version);    // No extended header (V1)
 Ptr = ParseBodyTZF<uint64>(hdr, Ptr);
 return sint(hdr->Version);
}
//---------------------------------------------------------------------------
template<typename T> static SLTT* GetTZFor(SHdrTZ* hdr, sint64 whence)
{
 int tc = (int)hdr->timecnt;
 int l = 1, r = tc;
 if(r == 0)return &hdr->LTTRecs[0];
 int i = (l+r)>>1;  // >> /2
 while(i > l)
  {
   if(l >= tc){ i = tc; break; }
   if(l == i) break;
   sint64 tv = ReadField<T>(&((T*)hdr->TransTimes)[i]);
   if(tv == whence){ i++; break; }
    else if(tv > whence) r = i-1;
    else l = i+1;
   i = (l+r)>>1;    // >> /2
  }
 if(i > tc)i = tc;
 if(whence > (sint64)ReadField<T>(&((T*)hdr->TransTimes)[i]))i++;
 return &hdr->LTTRecs[hdr->TransTypes[i-1]];
}
//---------------------------------------------------------------------------
static sint32 GetTimeZoneOffset(void* tzf, sint64 whence)  // In seconds
{
 SHdrTZ hdr = {};
 if(ParseTZF(&hdr, tzf) < 0)return -1;
 SLTT*  rec = (hdr.Version > 1)?GetTZFor<sint64>(&hdr, whence):GetTZFor<sint32>(&hdr, whence);
 return ReadField<sint32>(&rec->utoff);
}
};
//---------------------------------------------------------------------------
