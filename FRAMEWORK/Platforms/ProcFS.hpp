
struct NPFS
{
//------------------------------------------------------------------------------------------------------------
static PX::fdsc_t OpenProcFile(sint ProcId, const achar* PFile)  // cat /proc/self/maps
{
   //    return NPTM::NAPI::open("D:/TEST/maplist.txt",PX::O_RDONLY,0);    // <<< DBG
 achar buf[32];
 achar path[64];

 int offs = NSTR::StrCopy(path,"/proc/");
 if(ProcId > 0)offs += NSTR::StrCopy(&path[offs],NCNV::DecNumToStrU<int>(ProcId, buf));
   else offs += NSTR::StrCopy(&path[offs], "self");
 path[offs++] = '/';
 NSTR::StrCopy(&path[offs], PFile);
 return NPTM::NAPI::open(path,PX::O_RDONLY,0);
}
//------------------------------------------------------------------------------------------------------------
static sint ParseMMapLine(const achar* Line, uint Size, SMemRange* Range)
{
 while(*Line && (*Line <= 0x20))Line++;
 if(!*Line)return -1;
// uint Size = 0;
// RangeBeg
 size_t rb = NCNV::HexStrToNum<size_t>(Line, &Size);
 Line += Size;
 if((*Line != '-')&&(*Line != 0x20))return -2;
// RangeEnd
 size_t re = NCNV::HexStrToNum<size_t>(++Line, &Size);
 if(Size < 4)return -3;
 Line += Size;
// Perms
 while(*Line && (*Line <= 0x20))Line++;
 if(!*Line)return -4;
 if((*Line != '-')&&(*Line != 'r'))return -5;       // rwxp / ----
 uint32 pm = 0;
 if(Line[0] != '-')pm |= mmRead;     // r
 if(Line[1] != '-')pm |= mmWrite;    // w
 if(Line[2] != '-')pm |= mmExec;     // x
 if(Line[3] == '-')pm |= mmShared;   // p
 Line += 4;
 if(*Line != 0x20)return -6;
 while(*Line && (*Line <= 0x20))Line++;
 if(!*Line)return -7;
// Offset
size_t offs = NCNV::HexStrToNum<size_t>(Line, &Size);
 if(Size < 4)return -8;
 Line += Size;
 if(*Line != 0x20)return -9;
 while(*Line && (*Line <= 0x20))Line++;
 if(!*Line)return -10;
// DevH + DevL
 size_t dh = NCNV::HexStrToNum<size_t>(Line, &Size);
 Line += Size;
 if(*Line != ':')return -11;
 Line++;
 size_t dl = NCNV::HexStrToNum<size_t>(Line, &Size);
 Line += Size;
 if(*Line != 0x20)return -12;
 while(*Line && (*Line <= 0x20))Line++;
 if(!*Line)return -13;
// INODE
 size_t in = NCNV::DecStrToNum<size_t>(Line, &Size);
 Line += Size;
 if(*Line != 0x20)return -14;
 const achar* Beg = nullptr;
 const achar* End = nullptr;
 if(in)
  {
   while(*Line && (*Line <= 0x20))Line++;
   if(*Line == '/')
    {
     Beg = Line;
     while(*Line && (*Line >= 0x20)){if(*Line == 0x20)End=Line; Line++;}
     if(Line[-1] != 0x20)End=Line;
    }
  }

 Range->RangeBeg = rb;
 Range->RangeEnd = re;
 Range->FMOffs   = offs;
 Range->Mode     = pm;
 Range->INode    = in;
 Range->DevH     = dh;
 Range->DevL     = dl;
 if(Beg)
   {
    Range->FPathLen = End - Beg;
    Range->FPath    = (achar*)Beg;
   }
    else {Range->FPath = nullptr; Range->FPathLen = 0;}
 return 0;
}
//------------------------------------------------------------------------------------------------------------
// NOTE: Range->FPath and Range->FPathLen must be set to actual buffer address and size or NULL if not needed
//
static sint FindMappedRangeByAddr(sint ProcId, size_t Addr, SMemRange* Range)    // TODO: Move to ProcFS.hpp, wrap in MemUtils.hpp (Read,Write,Enumerate a process memory)
{
 int mfd = NPFS::OpenProcFile(ProcId, "maps");
 if(mfd < 0)return mfd;
 achar buf[2048];
 Range->INode = 0;  // To avoid the path copy skipping
 achar* FPath = Range->FPath;
 uint32 FPathLen = Range->FPathLen;
 for(ssize_t offs=0;;)
  {
   achar* Line;
   sint res = ReadFileLine(mfd, buf, sizeof(buf), &offs, &Line);
   if(res < 0)break;
   res = ParseMMapLine(Line, res, Range);
   if(res < 0)break;
   if((Addr >= Range->RangeBeg)&&(Addr < Range->RangeEnd))  // Is this correct method to compare?
    {
     NPTM::NAPI::close(mfd);
     if(Range->FPathLen)
      {
       if(FPath && FPathLen)
        {
         Range->FPathLen = Min<size_t>(Range->FPathLen,FPathLen-1);
         NSTR::StrCopy(FPath, Range->FPath, Range->FPathLen+1);
         Range->FPath = FPath;
        }
         else Range->FPath = nullptr;
      }
     return Range->FPathLen + sizeof(SMemRange) + 1;
    }
  }
 NPTM::NAPI::close(mfd);
 return -1;   // Not found
}
//------------------------------------------------------------------------------------------------------------
// Several ranges at which the specified file is mapped. Full or partial path. Starting from specified addr
// NOTE: ModPath can be in same buffer as MappedRanges. The buffer must be aligned as alignas(sizeof(vptr))
// NOTE: BSS segments will not be included (inode is 0 for them). Section headers will probably be missing too
//
static sint FindMappedRangesByPath(sint ProcId, size_t Addr, const achar* ModPath, SMemMap* MappedRanges, size_t BufSize)  // OpenBSD: /dev/kmem
{
 int mfd = NPFS::OpenProcFile(ProcId, "maps");
 if(mfd < 0){MappedRanges->NextAddr = MappedRanges->RangesCnt = 0; return mfd;}
 uint32 mplen = NSTR::StrLen(ModPath);
 uint32 cur_inode = 0;
 sint res = -1000;
 sint Total = 0;
 bool HaveMatch = false;
 SMemRange* DstRange = MappedRanges->Ranges;
 SMemRange TmpRange;    // A temporary to allow file path to be stored in the same buffer
 size_t NextAddr  = 0;
 size_t RangesCnt = 0;  // To avoid the path copy skipping
 for(;;)
  {
   achar* Line;
   res = ReadFileLine(mfd, (achar*)MappedRanges->TmpBufPtr, MappedRanges->TmpBufLen, &MappedRanges->TmpBufOffs, &Line);
   if(res < 0){if((res != -PX::ENOMEM) && RangesCnt)res=0; break;}
   if(!res)break;
   res = ParseMMapLine(Line, res, &TmpRange);
   if(res < 0)break;
   if(HaveMatch)   // The range is belong to the same file
    {
     if(TmpRange.INode == cur_inode)
      {
       if(BufSize < sizeof(SMemRange)){NextAddr = TmpRange.RangeBeg; break;}     // No more space for a record (At least one more record is in the stream)
       TmpRange.FPath = MappedRanges->Ranges[0].FPath;
       memcpy(&MappedRanges->Ranges[RangesCnt++], &TmpRange, sizeof(SMemRange));
       BufSize -= sizeof(SMemRange);
       Total   += sizeof(SMemRange);
       continue;
      }
     break;   // No more mapped regions of the same file
    }
   if(!TmpRange.INode || (TmpRange.FPathLen < mplen))continue;    // Not a mapped file
   if(TmpRange.RangeBeg < Addr)continue;
   if(NSTR::IsStrEqualCS(&TmpRange.FPath[TmpRange.FPathLen - mplen], ModPath, mplen))  // Is this correct method to compare?
    {
     cur_inode = TmpRange.INode;       // First match
     HaveMatch = true;
     uint RLen = (TmpRange.FPathLen + sizeof(SMemMap)+sizeof(SMemRange)+1);
     if(RLen > BufSize){NextAddr = Addr; break;}     // No space for the first record
     achar* srcstr   = TmpRange.FPath;   // In the line buffer
     TmpRange.FPath  = (achar*)MappedRanges + (BufSize - (TmpRange.FPathLen+1));      // Put the path at the end of the buffer
     NSTR::StrCopy(TmpRange.FPath, srcstr, TmpRange.FPathLen+1);
     memcpy(&MappedRanges->Ranges[RangesCnt++], &TmpRange, sizeof(SMemRange));
     BufSize  -= RLen;  // Space left for next ranges
     Total    += RLen;
    }
  }
 NPTM::NAPI::close(mfd);
 if(HaveMatch)
  {
   MappedRanges->NextAddr  = NextAddr;
   MappedRanges->RangesCnt = RangesCnt;  // To avoid the path copy skipping
  }
   else MappedRanges->NextAddr = MappedRanges->RangesCnt = 0;
 if(res >= 0)return Total;     // For statictic: there will be gap between records and strings
 return res;   // Not found
}
//------------------------------------------------------------------------------------------------------------
// Sets MappedRanges->NextAddr to be passed to AddrFrom for next call if there are more records
static sint ReadMappedRanges(sint ProcId, size_t AddrFrom, size_t AddrTo, SMemMap* MappedRanges, size_t BufSize)   // OpenBSD: /dev/kmem
{
 int mfd = NPFS::OpenProcFile(ProcId, "maps");
 if(mfd < 0)return mfd;
 size_t stroffs = BufSize;
 uint32 cur_inode = 0;
 sint res = -1000;
 sint Total = 0;
 SMemRange* Range = MappedRanges->Ranges;
 MappedRanges->NextAddr  = 0;
 MappedRanges->RangesCnt = 0;  // To avoid the path copy skipping
 BufSize -= sizeof(SMemMap);
 for(;;)
  {
   achar* Line;
   res = ReadFileLine(mfd, (achar*)MappedRanges->TmpBufPtr, MappedRanges->TmpBufLen, &MappedRanges->TmpBufOffs, &Line);
   if(res < 0){if((res != -PX::ENOMEM) && MappedRanges->RangesCnt)res=0; break;}
   if(!res)break;
   res = ParseMMapLine(Line, res, Range);
   if(res < 0)break;
   if(Range->RangeBeg < AddrFrom)continue;
   if(Range->RangeBeg >= AddrTo)break;
   if(Range->INode)  // The range belongs to a mapped file
    {
     if(Range->INode != cur_inode)  // Need to add a new path
      {
       uint len = Range->FPathLen + 1;
       if((len + sizeof(SMemRange)) > BufSize){MappedRanges->NextAddr = Range->RangeBeg; break;}  // No space
       cur_inode = Range->INode;
       achar* srcstr = Range->FPath;   // In the line buffer
       Range->FPath  = (achar*)MappedRanges + (stroffs - len);      // Put the path at the end of the buffer
       NSTR::StrCopy(Range->FPath, srcstr, len);
       stroffs -= len;
       BufSize -= len;
       Total   += len;
      }
       else   // Same path
        {
         Range->FPath    = Range[-1].FPath;
         Range->FPathLen = Range[-1].FPathLen;
        }
    }
   if(sizeof(SMemRange) > BufSize){MappedRanges->NextAddr = Range->RangeBeg; break;}  // No space
   MappedRanges->RangesCnt++;
   BufSize -= sizeof(SMemRange);
   Total   += sizeof(SMemRange);
   Range++;
  }
 NPTM::NAPI::close(mfd);
 if(res >= 0)return (MappedRanges->RangesCnt)?(Total+sizeof(SMemMap)):(Total);    // For statictic: there will be gap between records and strings
 return res;   // Not found
}
//------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------------------
};

/*    // TEST on Windows
 {
  NPTM::SMemRange Range;
  uint8 buf[8192];
  uint8 tmp[2048];

  Range.FPathLen = sizeof(buf);
  Range.FPath    = (achar*)&buf;
  NPTM::SMemMap* MappedRanges = (NPTM::SMemMap*)&buf;
  int res = NPTM::NPFS::FindMappedRangeByAddr(0, 0x7f95d59042, &Range);

  memset(MappedRanges,0,sizeof(buf));
  MappedRanges->TmpBufLen  = sizeof(tmp);
  MappedRanges->TmpBufOffs = 0;
  MappedRanges->TmpBufPtr  = tmp;
  MappedRanges->RangesCnt  = 0;
  MappedRanges->NextAddr   = 0;
  res = NPTM::NPFS::FindMappedRangesByPath(0, 0, "/aarch64-linux-gnu/libnss_files-2.28.so", MappedRanges, sizeof(buf));

  memset(MappedRanges,0,sizeof(buf));
  MappedRanges->TmpBufLen  = sizeof(tmp);
  MappedRanges->TmpBufOffs = 0;
  MappedRanges->TmpBufPtr  = tmp;
  MappedRanges->RangesCnt  = 0;
  MappedRanges->NextAddr   = 0;
  res = NPTM::NPFS::ReadMappedRanges(0, 0, -1, MappedRanges, sizeof(buf));
  MappedRanges = 0;

 }
*/
