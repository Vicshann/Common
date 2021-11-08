#pragma once


//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
static constexpr wchar PATHDLMR = 0x2F;    //  '/'
static constexpr wchar PATHDLML = 0x5C;    //  '\'

static _finline bool IsFilePathDelim(wchar val){return ((val == PATHDLML)||(val == PATHDLMR));}

template<typename T> static _finline bool IsDirSpec(T Name){return (((Name[0] == '.')&&(!Name[1]||IsFilePathDelim(Name[1])))||((Name[0] == '.')&&(Name[1] == '.')&&(!Name[2]||IsFilePathDelim(Name[1]))));}

template<typename D, typename S> static bool AssignFilePath(D DstPath, S BasePath, S FilePath)  // TODO: Should return length
{
 if(!FilePath || !FilePath[0])return false;
 if(FilePath[1] != ':')
  {
   NSTR::StrCopy(DstPath, BasePath);
   S Ptr = (IsFilePathDelim(FilePath[0]))?(&FilePath[1]):(&FilePath[0]);
   NSTR::StrCnat(DstPath, Ptr);
  }
   else NSTR::StrCopy(DstPath, FilePath);
 return true;
}
//---------------------------------------------------------------------------
template<typename T> sint TrimFilePath(T Path)
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
template<typename T> T GetFileName(T FullPath, uint Length=-1)    // TODO: Just scan forward, no StrLen and backward scan  // Set constexpr 'IF' in case a T is a str obj an its size is known?
{
 sint LastDel = -1;
 for(sint ctr=0,val=FullPath[ctr];val && Length;ctr++,Length--,val=FullPath[ctr]){if(IsFilePathDelim(val))LastDel=ctr;}
 return &FullPath[LastDel+1];
}
//---------------------------------------------------------------------------
template<typename T> T GetFileExt(T FullPath, uint Length=-1)
{
 sint LastDel = -1;
 sint ctr = 0;
 for(uint8 val=FullPath[ctr];val && Length;ctr++,Length--,val=FullPath[ctr]){if(val=='.')LastDel=ctr;}
 if(LastDel < 0)LastDel = ctr-1;
 return &FullPath[LastDel+1];
}
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------

