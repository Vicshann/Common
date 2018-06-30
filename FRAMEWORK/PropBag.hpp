
#ifndef PropBagH
#define PropBagH

#pragma once


/*
  Any object can be inherited from a Prop
  Only a PropFactory creates and destroys props, PropBag only stores a pointer to it
  NameFactory manages all operation with storing and matching a names(or hashes of them in optimized mode)
  StringFactory manages all string prop values
  MemFactory manages allocation and deallocation of memory blocks
  All factories can be preconfigured specifically before passing to a PropBag
  Factories are templated and configured only statically that there will be no need to store their pointers for each prop
  ÇProps will notify their owner on deletion


If i use a multithreading, then a pointer to a prop taken in one thread can become invalid if that prop is removed in another. 
So, need always return not a prop, but some simple CPropRef object which can increase a prop`s ref for its time of live and prevent if from being removed.

A prop factory should be accessible from each prop? By placing its pointer to a brginneng of each large prop memory block perhaps(that we not need to store a ptr to owner with each prop)? 
A prop`s ouner should be accessible as well? 
A prop`s owner should be its factory(Inherited from a base CPropBag ?) so having a CProp object will allow getting its ouner?

Let assume that i used Union to define a prop types. Then either there will be some overhead in 
checkeing prop types or you will write something like this 'if(mprop.GetPropEnum("My Prop").vfloat == 5.7)...' that have same lack of type safety as 'if(mprop.GetProp<float>("My Prop") == 5.7)...'

It is no recommended to use a prop deletion functionality, but it shuold be possible and optimal with an appropriate memory manager

Do not take a big structures by value it is slow because of copying, take a CPropRef of them
CPropName uses hash instead of name in RELEASE build if specified? Or always use a hash? Same names not a problem? What about a hash collision, should there be a check before adding a prop?
Use some global name string factory for names? Definently don`t want to keep similair names in memory

If we use some kind of HANDLE instead of CPropRef then we have to lock an entire CPropBag when accessing(removing) a prop

Conclusion: deletion of props is a bad idea for a prop bag! Without this we can minimize threaad locking to allocation of a new memory chain blocks only
*/
class CPropBase
{

virtual ~CPropBase() = 0;


};
//---------------------------------------------------------------------------













// Methods` name must resolve to RTTI type
// Integers are guaranteed to be a pointer size
// ptPointer is type without a serializable value, loaded as NULL
// Methods are set by a Component`s constructor and called by a user code
// Callback are set by a user code` constructor and called by a component
// PropBag props have a virtual destructor and RefCtr and can be any object
// The idea is that you don`t have to pass pointer to a prop owner it wil be calculated from its address
// PropBag destruction order?

// NOTE: enums are leaking into outer scope  // Good old enums are illogical entities :)
enum EPropType {ptNone,      // A empty prop, ready to be reused if nonsequental placement is allowed(else just skipped) // A 'compact' operation will be possible if invalidating pointers is OK
                ptFloat,
                ptInteger,
                ptUInteger,
                ptPointer,   // A raw unserializable pointer
                ptPlain,     // Pointer to plain data structure (CPlainVal incapsulator class)   // Anything that cannot be derived from CPropBag(A system structs) and don`t require a resource releasing
                ptString,    // A string stored in a StringFactory       // A facrory is calculated from string object address  // CFactory::FactoryDescFromAddr // Dereferenced on destruction // Memory allocated in chunks an a factory base ptr is placed in each chunk descriptor
                ptPropBag    // Pointer to an abstract PropBag object    // Support a virtual destructor
               };  
//---------------------------------------------------------------------------
// Memory allocaor is CMemChunk as it is preserves pointers when allocating an additional chunks. But direct array enumeration access is not possible
class CPropBag    // Components are derived from this class and have pointers to all predefined Props
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

//
//
//
//---------------------------------------------------------------------------
struct SProp    // Size: X64(8 + 2)=10, X32(4 + 2)=6    // Props will be used everywhere so we must squeeze max memory? 
{
 union
  {
   VALFLT    PropFlt;
   VALINT    PropInt;
   VALUINT   PropUInt;
   void*     PropPtr;
   CU8Str*   PropStr;    // Simple UTF8 String (Contains data before STR pointer)  // Size is one PVOID
   CPropBag* PropBag;    // Represents any component, derived from SPropBag   // Allocated by owner PropBag  // Must have only one owner! // Same child component cannot belong to two different parent components!
  };
 UINT16 Type    : 3;   // Max 8 types
 UINT16 NameIdx : 13;  // Max 8192 unique names   // Stored in Names Cache (Ref Counted)  // Can be missing   // Names searched in cache first an then compared by an index
//----------------------------------------
EPropType AssignNew(CPropBag* Value){this->PropBag  = Value; this->PropBag->DoRef(); return ptPropBag;}  // All PropBags must be allocated in heap
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
//
//
//
// UINT       Count;     // Useless
// ULONG      Type;      // User defined  // Or let it be another prop?  // A derived classes can implement this if needed
 UINT       RefCtr;
 CStrCache* NamesCache;  // Taken from a parent
 MemUtil::SMemChain<SProp, Alloca>* Props;      

//----------------------------------------
SPropBag()
//----------------------------------------


//----------------------------------------
static CPropBag* Create()       // Expected to be
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
