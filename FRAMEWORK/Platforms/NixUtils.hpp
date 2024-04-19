
#pragma once

//------------------------------------------------------------------------------------------------------------
static int UpdateTZOffsUTC(sint64 CurTimeUTC)  // Returns timezone offset in seconds
{
 int df = NAPI::open("/etc/localtime",PX::O_RDONLY,0);
// DBGDBG("TZFILE %lli open: %i",CurTimeUTC,df);
 if(df < 0)
  {
   if(-df == PX::ENOENT)return ReadTZOffsFromFile();
   return df;
  }
 sint64 flen = NAPI::lseek(df, 0, PX::SEEK_END);     // Avoiding unreliable fstat
 if(flen < 0)return (int)flen;
 NAPI::lseek(df, 0, PX::SEEK_SET);
 uint8* filedata = (uint8*)StkAlloc(flen+8);
 sint rlen = NAPI::read(df, filedata, flen);
 NAPI::close(df);
 if(rlen < flen)return -PX::ENODATA;     // sizeof(SHdrTZ)
 sint32 offs = STZF::GetTimeZoneOffset(filedata,CurTimeUTC);
// DBGDBG("TZFILE %u offs: %i",rlen,offs);
 if(offs < 0)return -PX::EINVAL;
 fwsinf.UTCOffs = offs;
 return 0;
}
//------------------------------------------------------------------------------------------------------------
// NOTE: On mobile phones and other small devices that run Linux, the time zone is stored differently. It is written in /etc/TZ
// In the POSIX timezone format, the 3 letters are the timezone abbreviation (which is arbitrary) and the number is the number
// of hours the timezone is behind UTC. So UTC-8 means a timezone abbreviated "UTC" that is -8 hours behind the real UTC, or UTC + 8 hours.
//
static int ReadTZOffsFromFile(void)  // Returns timezone offset in seconds
{
 int df = NAPI::open("/etc/TZ",PX::O_RDONLY,0);
 if(df < 0)return df;
 achar buf[128];
 sint rlen = NAPI::read(df, buf, sizeof(buf)-1);
 NAPI::close(df);
 sint32 offs = 0;
 if(rlen)
  {
   buf[rlen] = 0;
//   DBGDBG("TZFILE %u: %s",rlen,&buf);
   for(sint idx=0;idx < rlen;idx++)
    {
     achar val = buf[idx];
     if((val >= '0')&&(val <= '9'))     // Most likely incorrect  // CST-8, UTC-8  // How to actually parse
      {
       offs = NCNV::DecStrToNum<int32>(&buf[idx]);   // Hours?
       offs = offs * 3600;  // Hours to seconds
       if(buf[idx-1] != '-')offs = -offs;   // hours behind
       break;
      }
    }
  }
 fwsinf.UTCOffs = offs;
 return 0;
}
//------------------------------------------------------------------------------------------------------------
static PX::fdsc_t OpenDevNull(void)
{
 return NPTM::NAPI::open("/dev/null",PX::O_RDWR,0);
}
//------------------------------------------------------------------------------------------------------------
static PX::fdsc_t OpenDevRand(void)
{
 return NPTM::NAPI::open("/dev/random",PX::O_RDONLY,0);
}
//------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------
