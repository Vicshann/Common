
//============================================================================================================
#if defined(PLT_WIN_USR)
static sint GetEnvVar(sint& AOffs, achar* DstBuf, uint DstCCnt=uint(-1))   // Enumerate and copy     // NOTE: Copying is inefficient!
{
 uint SrcLen = 0;
 const syschar* evar = GetEnvVar(AOffs, &SrcLen);
 if(!evar)return 0;
 if(!DstBuf)return (sint)SizeOfWStrAsUtf8(evar, SrcLen, 0) + 4;       // Calculate buffer size for current name+value    // +Space for terminating 0
 WStrToUtf8(DstBuf, evar, DstCCnt, SrcLen, 0);
 return (sint)DstCCnt;
}
//------------------------------------------------------------------------------------------------------------
static sint GetEnvVar(const achar* Name, achar* DstBuf, uint DstCCnt=uint(-1))   // Find by name and copy   // NOTE: Copying is inefficient!
{
 sint AOffs = 0;
 uint  Size = 0;
 bool Unnamed = !Name || !*Name;
 syschar* evar = nullptr;
 while((evar=(syschar*)GetEnvVar(AOffs, &Size)))
  {
   sint spos = NSTR::ChrOffset(evar, '=');
   if(spos < 0)continue;  // No separator!
   if((!spos && Unnamed) || NSTR::IsStrEqualCS(Name, evar, (size_t)spos))
    {
     uint offs = uint(spos+1);
     Size -= offs;
     if(!DstBuf)return (sint)SizeOfWStrAsUtf8(&evar[offs], Size, 0) + 4;       // Calculate buffer size for current value    // +Space for terminating 0
     WStrToUtf8(DstBuf, &evar[offs], DstCCnt, Size, 0);
     return (sint)DstCCnt;
    }
  }
 return -1;
}
//------------------------------------------------------------------------------------------------------------
static const syschar* GetEnvVar(sint& AOffs, uint* Size=nullptr)      // Enumerate
{
 if(AOffs < 0)return nullptr;    // Already finished
 const wchar* vars = &NPTM::GetEnvP()[AOffs];
 if(!*vars){AOffs = -1; return nullptr;}   // No EVars!
 uint VarLen = NSTR::StrLen(vars);
 if(Size)*Size = VarLen;
 VarLen += uint(AOffs + 1);
 if(vars[VarLen])AOffs = (sint)VarLen;
  else AOffs = -1;      // End of the EVAR list
 return vars;
}
//------------------------------------------------------------------------------------------------------------
static const syschar* GetEnvVar(const achar* Name, uint* Size=nullptr)          // Find by name     // (No Unicode support in names!)
{
 sint AOffs = 0;
 bool Unnamed = !Name || !*Name;
 syschar* evar = nullptr;
 while((evar=(syschar*)GetEnvVar(AOffs, Size)))
  {
   sint spos = NSTR::ChrOffset(evar, '=');
   if(spos < 0)continue;  // No separator!
   if((!spos && Unnamed) || NSTR::IsStrEqualCS(Name, evar, (size_t)spos))
    {
     if(Size)*Size -= uint(spos+1);
     return &evar[spos+1];
    }
  }
 return nullptr;
}
//------------------------------------------------------------------------------------------------------------
// AppleInfo on MacOS
static sint GetAuxInfo(uint InfoID, void* DstBuf, size_t BufSize)
{
 //GetAuxVRec(size_t Type)
 return -1;
}
//------------------------------------------------------------------------------------------------------------
#elif defined(PLT_LIN_USR)    // BSD, MacOS ?
static sint GetEnvVar(sint& AOffs, achar* DstBuf, uint DstCCnt=uint(-1))   // Enumerate and copy     // NOTE: Copying is inefficient!
{
 return UELF::GetStrFromArr(&AOffs, NPTM::GetEnvP(), DstBuf, DstCCnt);
}
//------------------------------------------------------------------------------------------------------------
static _ninline sint GetEnvVar(const achar* Name, achar* DstBuf, uint DstCCnt=uint(-1))   // Find by name and copy   // NOTE: Copying is inefficient!  // NOTE: Broken on ARM64 if inlined! Why?
{
 const  achar** vars = NPTM::GetEnvP();
 if(!vars)return -2;
 bool Unnamed = !Name || !*Name;
 for(;const achar* evar = *vars;vars++)
  {
   sint spos = NSTR::ChrOffset(evar, '=');
   if(spos < 0)continue;  // No separator!
   if((!spos && Unnamed) || NSTR::IsStrEqualCS(Name, evar, (uint)spos))return (sint)NSTR::StrCopy(DstBuf, &evar[spos+1], DstCCnt);
  }
 return -1;
}
//------------------------------------------------------------------------------------------------------------
static const syschar* GetEnvVar(sint& AOffs, uint* Size=nullptr)      // Enumerate
{
 if(AOffs < 0)return nullptr;    // Already finished
 const achar** vars = NPTM::GetEnvP();
 if(!vars)return nullptr;
 const syschar* val = vars[AOffs++];
 if(!val){AOffs = -1; return nullptr;}  // End of list reached
 if(Size)*Size = NSTR::StrLen(val);
 return val;
}
//------------------------------------------------------------------------------------------------------------
static const syschar* GetEnvVar(const achar* Name, uint* Size=nullptr)          // Find by name
{
 const achar** vars = NPTM::GetEnvP();
 if(!vars)return nullptr;
 bool Unnamed = !Name || !*Name;
 for(;const achar* evar = *vars;vars++)
  {
   sint spos = NSTR::ChrOffset(evar, '=');
   if(spos < 0)continue;  // No separator!
   if((!spos && Unnamed) || NSTR::IsStrEqualCS(Name, evar, (uint)spos))
    {
     if(Size)*Size = NSTR::StrLen(&evar[spos+1]);
     return &evar[spos+1];
    }
  }
 return nullptr;
}
//------------------------------------------------------------------------------------------------------------
// AppleInfo on MacOS
static sint GetAuxInfo(uint InfoID, vptr DstBuf=nullptr, size_t BufSize=0)
{
 if(!fwsinf.AuxInf)return -3;
 if(DstBuf && (BufSize < sizeof(size_t)))return -2;
 for(ELF::SAuxVecRec* Rec=fwsinf.AuxInf;Rec->type != ELF::AT_NULL;Rec++)
  {
   if(Rec->type == InfoID)
    {
     if(!DstBuf)return Rec->val;
     *(size_t*)DstBuf = Rec->val;
     return 0;
    }
  }
 return -1;
}
//------------------------------------------------------------------------------------------------------------
#else
static sint GetEnvVar(sint& AOffs, achar* DstBuf, uint DstCCnt=uint(-1)){return -1;}
static _ninline sint GetEnvVar(const achar* Name, achar* DstBuf, uint DstCCnt=uint(-1)){return -1;}
static const syschar* GetEnvVar(sint& AOffs, uint* Size=nullptr){return nullptr;}
static const syschar* GetEnvVar(const achar* Name, uint* Size=nullptr){return nullptr;}
static sint GetAuxInfo(uint InfoID, vptr DstBuf, size_t BufSize){return -1;}
//------------------------------------------------------------------------------------------------------------
#endif
//------------------------------------------------------------------------------------------------------------
