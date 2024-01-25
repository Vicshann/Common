
#pragma once

//------------------------------------------------------------------------------------------------------------
static int UpdateTZOffsUTC(sint64 CurTimeUTC)  
{
 int df = NAPI::open("/etc/localtime",PX::O_RDONLY,0);
// DBGDBG("TZFILE %lli open: %i",CurTimeUTC,df);
 if(df < 0)return df;
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
