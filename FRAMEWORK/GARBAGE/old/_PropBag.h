
#ifndef PropBagH
#define PropBagH

#pragma once

// Methods` name must resolve to RTTI type
// Integers are guaranteed to be a pointer size
// ptPointer is type without a serializable value, loaded as NULL
// Methods are set by a Component`s constructor and called by a user code
// Callback are set by a user code` constructor and called by a component

enum EPropType {ptNone,ptFloat,ptInteger,ptUInteger,ptPointer,ptString,ptPropBag};  

//---------------------------------------------------------------------------
template<typename Alloca> struct SPropBag  // Components are derived from this class and have pointers to all predefined Props
{
#if defined(_WIN64) || defined(_AMD64_)    // 8 bytes      
typedef double        VALFLT;
typedef long          VALINT;
typedef unsigned long VALUINT;
#else    // 4 bytes
#if defined(_FORCEPROP64)
typedef double           VALFLT;
typedef __int64          VALINT;
typedef unsigned __int64 VALUINT;
#else 
typedef float         VALFLT;
typedef long          VALINT;
typedef unsigned long VALUINT;
#endif
#endif
//---------------------------------------------------------------------------
struct SProp    // Size: X64(8 + 2)=10, X32(4 + 2)=6    // Props will be used everywhere so we must squeeze max memory 
{
 union
  {
   VALFLT  PropFlt;
   VALINT  PropInt;
   VALUINT PropUInt;
   PVOID   PropPtr;
   CU8Str  PropStr;    // Simple UTF8 String (Contains data before STR pointer)  // Size is one PVOID
   SPropBag* PropBag;  // Represents any component, derived from SPropBag   // Allocated by owner PropBag  // Must have only one owner! // Same child component cannot belong to two different parent components!
  };
 UINT16 Type    : 3;   // Max 8 types
 UINT16 NameIdx : 13;  // Max 8192 unique names   // Stored in Names Cache (Ref Counted)  // Can be missing   // Names searched in cache first an then compared by an index
//----------------------------------------
EPropType AssignNew(SPropBag* Value){this->PropBag  = Value; this->PropBag->DoRef(); return ptPropBag;}  // All PropBags must be allocated in heap
EPropType AssignNew(CU8Str*   Value){return ptString;}   // ????????????????????
EPropType AssignNew(LPSTR     Value){this->PropStr.Initialize((PBYTE)Value); return ptString;}
EPropType AssignNew(PWSTR     Value){this->PropStr.Initialize((PBYTE)Value); return ptString;}
EPropType AssignNew(PVOID     Value){this->PropPtr  = Value; return ptPointer;}
EPropType AssignNew(VALFLT    Value){this->PropFlt  = Value; return ptFloat;}
EPropType AssignNew(VALINT    Value){this->PropInt  = Value; return ptInteger;}
EPropType AssignNew(VALUINT   Value){this->PropUInt = Value; return ptUInteger;}
//----------------------------------------
template<typename T> EPropType Initialize(LPSTR Name, T Value, CStrCache* NamesCache)
{
 this->Name.AssignFrom(NamesCache->AddStr(Name));
 return this->AssignNew(Value);
}
//----------------------------------------
void Free(EPropType Type, CStrCache* NamesCache)
{
 if(Type == ptPropBag)this->PropBag->Release();
 NamesCache->DelStr(this->Name.c_str());
}
//----------------------------------------
};
//---------------------------------------------------------------------------

// UINT       Count;     // Useless
// ULONG      Type;      // User defined  // Or let it be another prop?  // A derived classes can implement this if needed
 UINT       RefCtr;
 CStrCache* NamesCache;  // Taken from parent
 MemUtil::SMemChain<SProp, Alloca>* Props;      

//----------------------------------------
SPropBag()

//----------------------------------------
void DoRef(void){this->RefCtr++;}
//----------------------------------------
void Release(void)
{
 if(this->RefCtr)this->RefCtr--;
 if(!this->RefCtr)MemUtil::FreeMem(this);
}
//----------------------------------------
void Initialize(CStrCache* nc)  // Name always will be a first prop so no need for a NULL check of Props
{
 this->NamesCache = nc;
 this->Props = NULL; 
}
//----------------------------------------
void Free(CStrCache* NamesCache)
{
 for(SProp* prop=this->Props;prop;)
  {
   SProp* Cur = prop;
   prop = prop->Next;
  }
}
//----------------------------------------
template<typename V> SProp* AddProp(LPSTR Name, V Value)
{
 if(!this->Props)this->Props = this->Props->Create();

}
//----------------------------------------

//----------------------------------------
SProp* GetPropByName(LPSTR Name)  // Names are case sensitive, so there are a simple compare until a NULL character
{

}
//----------------------------------------
SProp* GetPropByIndex(UINT Index)
{

}
//----------------------------------------
int RemoveProp(LPSTR Name)
{

}
//----------------------------------------
int RemoveProp(UINT Index)
{
 SProp* prop = this->GetPropByIndex(Index);
 if(!prop)return -1;

}
//----------------------------------------
/*
  WORD Type    : 3;  // Prop Type (0 - 7)
  WORD NameLen : 5;  // Max name len = 31  // 0 is for unnamed props
  WORD PropLen : 8;  // Max prop len = 255  // Number values are packed
  Name
  Value
*/
void SaveTo(SBinStream* Strm)
{

}
//----------------------------------------
void LoadFrom(SBinStream* Strm)
{

}
//----------------------------------------

};
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#endif
