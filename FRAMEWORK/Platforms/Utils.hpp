#pragma once


//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
static constexpr wchar PATHDLMR = 0x2F;    //  '/'
static constexpr wchar PATHDLML = 0x5C;    //  '\'

template<typename T> constexpr _finline static bool IsFilePathDelim(T val){return ((val == (T)PATHDLML)||(val == (T)PATHDLMR));}

template<typename T> constexpr _finline static bool IsDirSpec(T Name){return (((Name[0] == '.')&&(!Name[1]||IsFilePathDelim(Name[1])))||((Name[0] == '.')&&(Name[1] == '.')&&(!Name[2]||IsFilePathDelim(Name[1]))));}

template<typename D, typename S> constexpr _finline static bool AssignFilePath(D DstPath, S BasePath, S FilePath)  // TODO: Should return length
{
 if(!FilePath || !FilePath[0])return false;
 if(FilePath[1] != ':')    // No drive-absolute path
  {
   if(!IsFilePathDelim(FilePath[0]))
    {
     NSTR::StrCopy(DstPath, BasePath);
     S Ptr = (IsFilePathDelim(FilePath[0]))?(&FilePath[1]):(&FilePath[0]);
     NSTR::StrCnat(DstPath, Ptr);
    }
     else NSTR::StrCopy(DstPath, FilePath);  // File system absolute path (Unix way)
  }
   else NSTR::StrCopy(DstPath, FilePath);  // Drive-absolute path
 return true;
}
//---------------------------------------------------------------------------
template<typename T> constexpr _finline static sint TrimFilePath(T Path)
{
 sint SLast = -1;
 for(sint ctr=0;Path[ctr];ctr++)
  {
   if((Path[ctr] == PATHDLMR)||(Path[ctr] == PATHDLML))SLast = ctr;
  }
 SLast++;
 if(SLast > 0)Path[SLast] = 0;
 return SLast;
}
//---------------------------------------------------------------------------
template<typename T> constexpr _finline static T GetFileName(T FullPath, uint Length=(uint)-1)    // TODO: Just scan forward, no StrLen and backward scan  // Set constexpr 'IF' in case a T is a str obj an its size is known?
{
 sint LastDel = -1;
 for(sint ctr=0,val=FullPath[ctr];val && Length;ctr++,Length--,val=FullPath[ctr]){if(IsFilePathDelim((wchar)val))LastDel=ctr;}
 return &FullPath[LastDel+1];
}
//---------------------------------------------------------------------------
template<typename T> constexpr _finline static T GetFileExt(T FullPath, uint Length=(uint)-1)
{
 sint LastDel = -1;
 sint ctr = 0;
 for(uint8 val=FullPath[ctr];val && Length;ctr++,Length--,val=FullPath[ctr]){if(val=='.')LastDel=ctr;}
 if(LastDel < 0)LastDel = ctr-1;
 return &FullPath[LastDel+1];
}
//---------------------------------------------------------------------------
_finline static uint SizeOfWStrAsUtf8(const wchar* str, uint size=uint(-1), uint term=uint(0))
{
 uint ResLen = 0;
 wchar terml = wchar(term >> 16);  // Usually 0
 wchar terme = wchar(term);
 for(uint SrcIdx = 0;(str[SrcIdx] ^ terme) && (str[SrcIdx] > terml) && (SrcIdx < size);)
  {
   uint32 Val;
   charb tmp[6];
   SrcIdx += NUTF::ChrUtf16To32(&Val, str, 0, SrcIdx);  // Can read 2 WCHARs
   ResLen += NUTF::ChrUtf32To8(tmp, &Val, 0, 0);
  }
 return ResLen;
}
//---------------------------------------------------------------------------
_finline static uint WStrToUtf8(achar* dst, const wchar* str, XRef<uint> dlen, XRef<uint> slen, uint term=uint(0))
{
 uint SrcIdx = 0;
 uint DstIdx = 0;
 wchar terml = wchar(term >> 16);  // Usually 0
 wchar terme = wchar(term);
 while((str[SrcIdx] ^ terme) && (str[SrcIdx] > terml) && (DstIdx < dlen) && (SrcIdx < slen))
  {
   uint32 Val;
   SrcIdx += NUTF::ChrUtf16To32(&Val, str, 0, SrcIdx);
   DstIdx += NUTF::ChrUtf32To8(dst, &Val, DstIdx);   // NOTE: May overflow +3 bytes!
  }
 slen = SrcIdx;
 dlen = DstIdx;
 return DstIdx;
}
//---------------------------------------------------------------------------
