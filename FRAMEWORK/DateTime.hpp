
#pragma once

struct NDT
{
union FILETIME      // Windows, maps to uint64
{
 struct
  {
   uint32 dwLowDateTime;
   uint32 dwHighDateTime;
  };
  uint64 QuadPart;
};

struct SYSTEMTIME   // Windows
{
 uint16 Year;
 uint16 Month;
 uint16 DayOfWeek;
 uint16 Day;
 uint16 Hour;
 uint16 Minute;
 uint16 Second;
 uint16 Milliseconds;
};

struct DT: public SYSTEMTIME
{
 uint32 Nanoseconds;   // 1000000 Nanoseconds in 1 Millisecond
};

//using TS = PX::timespec<uint64>;      // Unreachable, ouner is namespece, not a struct :(

static constexpr int64  MAXTIME64  = 0x793406fff;        // number of seconds from 00:00:00, 01/01/1970 UTC to 23:59:59. 12/31/3000 UTC
static constexpr uint64 EPOCH_BIAS = 0x019DB1DED53E8000; // Number of 100 nanosecond units from 1/1/1601 to 1/1/1970     // 0x019DB1DED53E8000 116444736000000000

static constexpr uint64 NSEC_IN_MSEC    = 1000000;
static constexpr uint64 MICSEC_IN_SEC   = 1000000;
static constexpr uint64 SECS_TO_FT_MULT = 10000000;   // Number of 100-nanosecond units in a second
static constexpr uint64 TIME_T_BASE     = EPOCH_BIAS / SECS_TO_FT_MULT;

static constexpr uint16 _finline MakeOffsTZ(uint16 Hour, uint16 Minute){return (Hour<<8)|Minute;}
//---------------------------------------------------------------------------
//Return the time as seconds elapsed since midnight, January 1, 1970, or -1 in the case of an error.
//
/*static int64 GetTime64(bool Local)   // C++Builder fails 64bit consts!!!
{
 UINT64 ft, fu;
 if(Local)
  {
   GetSystemTimeAsFileTime((FILETIME*)&fu);
   FileTimeToLocalFileTime((FILETIME*)&fu,(FILETIME*)&ft);
  }
   else GetSystemTimeAsFileTime((FILETIME*)&ft);
 __int64 tim = (__int64)((ft - EPOCH_BIAS) / 10000000i64);
 if(tim > MAXTIME64)tim = (__int64)(-1);
 return tim;
}*/
//---------------------------------------------------------------------------
/*UINT64 FindTimeTBase(void)
{
 // Find 1st Jan 1970 as a FILETIME
 SYSTEMTIME st = {};
 FILETIME ft;
// memset(&st,0,sizeof(st));
 st.wYear=1970;
 st.wMonth=1;
 st.wDay=1;
 SystemTimeToFileTime(&st, &ft);
 return FileTimeToT64(&ft);
} */
//---------------------------------------------------------------------------
// Time64 is the number of seconds elapsed since midnight, January 1, 1970
// UnixTime is the number of seconds since the Unix epoch, which was midnight (00:00:00) on January 1, 1970, in Coordinated Universal Time (UTC)
// FILETIME is the number of 100-nanosecond intervals since the start of the year 1601 in the Gregorian calendar.
//
static uint64 _finline FileTimeToTime64(uint64 ft)
{
 return ft / SECS_TO_FT_MULT;
}
//---------------------------------------------------------------------------
static uint64 _finline Time64ToFileTime(uint64 pt)
{
 return pt * SECS_TO_FT_MULT;
}
//---------------------------------------------------------------------------
static time_t _finline FileTimeToUnixTime(uint64 ft)
{
 return (ft - EPOCH_BIAS) / SECS_TO_FT_MULT; //   ull.QuadPart / 10000000ULL - 11644473600ULL;
}
//---------------------------------------------------------------------------
static uint64 _finline UnixTimeToFileTime(time_t ut)
{
 return (ut * SECS_TO_FT_MULT) + EPOCH_BIAS;
}
//---------------------------------------------------------------------------
// 1 second = 1,000000000 Nano seconds
// Probably incorrect for nanoseconds field (Where nsecs field is explained?)
static uint64 FileTimeToUnixTime(uint64 ft, uint64* nsecs)
{
 uint64 ubase = (ft - EPOCH_BIAS);  // In 100-nanosecond units
 if(nsecs)*nsecs = (ubase % SECS_TO_FT_MULT) * 100;   // Use remaining units and convert them to full nanoseconds
 return ubase / SECS_TO_FT_MULT;
}
//---------------------------------------------------------------------------
static uint8 CalcDayOfWeek(uint16 year, uint8 month, uint8 day)
{
 if(month <= 2){year -= 1; month += 12;}  // Consider Jan and Feb to be months 13 and 14 of prev year
 uint cc = year / 100;       // Current century
 uint cy = year % 100;       // Year of the current century
 uint ze = day + (26 * (month + 1) / 10) + cy + (cy / 4) + (5 * cc) + (cc / 4);   // Zeller's congruence
 return ((ze + 5) % 7) + 1;  // Get DoW
}
//---------------------------------------------------------------------------
// Fills Timespec from DateTime
template<typename T> static time_t DateTimeToUnixTime(const DT* dt, T* ts)
{
 uint y = dt->Year;
 uint m = dt->Month;
 uint d = dt->Day;
 if(m <= 2){y -= 1;m += 12;}     // Consider Jan and Feb to be months 13 and 14 of prev year
 uint32 t = (365 * y) + (y / 4) - (y / 100) + (y / 400);  // Years to days
 t += (30 * m) + (3 * (m + 1) / 5) + d;  // Months to days
 t -= 719561;  // Unix EPOCH (January 1st, 1970)
 t *= 86400;   // Days to seconds
 t += (3600 * dt->Hour) + (60 * dt->Minute) + dt->Second;  // Add everything as seconds
 if(ts){ts->secs = t; ts->nsecs = ((uint)dt->Milliseconds * NSEC_IN_MSEC) + (uint64)dt->Nanoseconds;}
 return t;
}
//---------------------------------------------------------------------------
// tz_offs format: HHMM (Hours|Minutes) (0530)
// T is STSpec or STVal
template<typename T> static void UnixTimeToDateTime(const T* ts, DT* dt, sint32 tz_offs=0) // tz_offs=GetTZOffsUTC()
{
 time_t t = ts->sec + tz_offs;    //((((tz_offs >> 8) * 60) + (tz_offs & 0xFF)) * 60);     // + (tz_offset * 60 * 60);
 if(t < 1)t = 0;  // No negative time support!

 dt->Second = t % 60;
 t /= 60;
 dt->Minute = t % 60;
 t /= 60;
 dt->Hour   = t % 24;
 t /= 24;

 // Unix time to date
 uint32 a = (uint32) ((4 * t + 102032) / 146097 + 15);
 uint32 b = (uint32) (t + 2442113 + a - (a / 4));
 uint32 c = (20 * b - 2442) / 7305;
 uint32 d = b - 365 * c - (c / 4);
 uint32 e = d * 1000 / 30601;
 uint32 f = d - e * 30 - e * 601 / 1000;

 if(e <= 13){e -= 1;c -= 4716;}   // Consider Jan and Feb to be months 13 and 14 of prev year
  else {e -= 13;c -= 4715;}

 dt->Year  = c;
 dt->Month = e;
 dt->Day   = f;

 constexpr bool HaveNSecs = requires(const T* t) {t->nsec;};
 if constexpr (HaveNSecs)    // T is STSpec
  {
   dt->Milliseconds = ts->nsec / NSEC_IN_MSEC;   // Nanoseconds
   dt->Nanoseconds  = ts->nsec % NSEC_IN_MSEC;
  }
   else  // T is STVal
    {
     dt->Milliseconds = ts->usec / 1000;  // Microseconds
     dt->Nanoseconds  = ts->usec % 1000;  // Microseconds instead of Nanoseconds
    }
 if(!dt->DayOfWeek)dt->DayOfWeek = CalcDayOfWeek(c, e, f);    // Optional
}
//---------------------------------------------------------------------------
sint CompareDateTime(const DT* dt1, const DT* dt2)
{
      if(dt1->Year < dt2->Year)return -1;
 else if(dt1->Year > dt2->Year)return 1;
 else if(dt1->Month < dt2->Month)return -1;
 else if(dt1->Month > dt2->Month)return 1;
 else if(dt1->Day < dt2->Day)return -1;
 else if(dt1->Day > dt2->Day)return 1;
 else if(dt1->Hour < dt2->Hour)return -1;
 else if(dt1->Hour > dt2->Hour)return 1;
 else if(dt1->Minute < dt2->Minute)return -1;
 else if(dt1->Minute > dt2->Minute)return 1;
 else if(dt1->Second < dt2->Second)return -1;
 else if(dt1->Second > dt2->Second)return 1;
 else if(dt1->Milliseconds < dt2->Milliseconds)return -1;
 else if(dt1->Milliseconds > dt2->Milliseconds)return 1;
 else if(dt1->Nanoseconds < dt2->Nanoseconds)return -1;
 else if(dt1->Nanoseconds > dt2->Nanoseconds)return 1;
 return 0;
}
//---------------------------------------------------------------------------
/*void UnixTimeToSystemTime(UINT64 ut, SYSTEMTIME* pst)
{
 uint64 ft = UnixTimeToFileTime(ut);
 FileTimeToSystemTime(&ft, pst);
}  */
//---------------------------------------------------------------------------
/*UINT64 SystemTimeToT64(SYSTEMTIME *pst)
{
 FILETIME ft;
 //FILETIME ftl;
 SystemTimeToFileTime(pst, &ft);
 //LocalFileTimeToFileTime(&ftLocal, &ft);
 return (FileTimeToT64(&ft) - TIME_T_BASE);

//  (*pt) -= TIME_T_BASE;

// FILETIME ft;
// SystemTimeToFileTime(pst, &ft);
// return FileTimeToT64(&ft);
}*/
//---------------------------------------------------------------------------
/*void Time64ToSystemTime(UINT64 pt, SYSTEMTIME *pst)
{
 FILETIME ft;
 pt += FindTimeTBase();
 T64ToFileTime(pt,&ft);
 FileTimeToSystemTime(&ft,pst);
}*/
//---------------------------------------------------------------------------
/*void T64ToLocalSysTime(UINT64 pt, SYSTEMTIME *pst)
{
 FILETIME ft;
 FILETIME lft;
 pt += FindTimeTBase();
 T64ToFileTime(pt,&ft);
 FileTimeToLocalFileTime(&ft, &lft);
 FileTimeToSystemTime(&lft,pst);
} */
//---------------------------------------------------------------------------
// 2015-11-27 20:43:48
/*LPSTR _stdcall UnixDateTimeToStr(UINT64 DateTime, LPSTR Buffer)
{
 SYSTEMTIME systime;
 T64ToSystemTime(DateTime, &systime);
 wsprintf(Buffer,"%.4u-%.2u-%.2u %.2u:%.2u:%.2u",systime.wYear,systime.wMonth,systime.wDay,systime.wHour,systime.wMinute,systime.wSecond);
 return Buffer;
} */
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
};



