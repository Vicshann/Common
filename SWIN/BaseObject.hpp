//==========================================================================================================================
//                                            BASE OBJECT CLASS
//==========================================================================================================================
class CObjBase
{
protected:
 const char* TypeName  = nullptr;   // TODO: Add type name hash too
 UINT        Flags     = 0;
 UINT        ChildCnt  = 0;
 int         IdxInPar  = -1;         // Index in parent`s list (-1 if no parent)
 PVOID       pUserData = nullptr;
 HMODULE     hOwnerMod = nullptr;  // A module that owns this object
 CObjBase*   OwnerObj  = nullptr; 
 CObjBase**  ChildLst  = nullptr;

//------------------------------------------------------------------------------------------------------------
void SetHInstance(void)
{
 auto Ptr = &CObjBase::SetHInstance; 
 GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, *(LPCWSTR*)&Ptr, &this->hOwnerMod);  // Try to make a current module an owner
 if(!this->hOwnerMod)this->hOwnerMod = GetModuleHandleW(NULL);  // Make the EXE module an owner
}
//------------------------------------------------------------------------------------------------------------
void SetTypeID(auto TypeThis)
{
 this->TypeName = NCTM::TypeName<NCTM::RemoveRef<decltype(*TypeThis)>::T>();    // Useful only for debugging?
}
//------------------------------------------------------------------------------------------------------------
// Sometimes (Tray icon) we need to know component`s index before we create it
UINT FindFreeChildObjSlot(void)
{
 for(UINT ctr=0;ctr < this->ChildCnt;ctr++)
  { 
   if(!this->ChildLst[ctr])return ctr;   // An empty child slot found
  }
 return this->ChildCnt;   // Add new slot
}
//------------------------------------------------------------------------------------------------------------
// Resizes child components list if there is no a free one
// TODO: Add functions to control order of children controls?
void InsertChildObj(CObjBase* Obj, UINT SlotIdx)
{ 
 if(this->ChildLst)
  {
   if(SlotIdx == this->ChildCnt)this->ChildLst = (CObjBase**)HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, this->ChildLst, (++this->ChildCnt * sizeof(CObjBase*)));   // Enlarge to +1
  }
   else { this->ChildLst = (CObjBase**)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(CObjBase*)); this->ChildCnt = 1; }   // First child 
 this->ChildLst[SlotIdx] = Obj;   // A new slot created
}
//------------------------------------------------------------------------------------------------------------

public:
//------------------------------------------------------------------------------------------------------------
const char* TypeId(void){return this->TypeName;}             //static const char* Type = NCTM::TypeName<decltype(*this)>(); return Type;}    // TODO: Use compile time hash of this string
bool IsDerivedFrom(UINT FromMsk){return ((this->Flags & FromMsk) == FromMsk);}
bool IsDerivedFromAny(UINT FromMsk){return this->Flags & FromMsk;}
bool IsSameType(CObjBase* Obj){ return this->TypeName == Obj->TypeName; }
//------------------------------------------------------------------------------------------------------------
HMODULE GetHInstance(void)
{
 if(!this->hOwnerMod)this->SetHInstance();
 return this->hOwnerMod;
}
//------------------------------------------------------------------------------------------------------------
void* operator new(size_t Size){return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, Size);}
void  operator delete(void* Ptr) { HeapFree(GetProcessHeap(), 0, Ptr); }
//------------------------------------------------------------------------------------------------------------
template<typename T> static void DeleteCtrl(T& pCtrl){auto Ptr=pCtrl; pCtrl=nullptr; delete(Ptr);}
template<typename P, typename C> static void SetCallback(P& Ptr, C Callback){Ptr = static_cast<P>(Callback);}  
template<class T, typename... Args> T* AddObj(Args&&... args)     // NOTE: copy of this method`s body is created for every instance!   // Move allocator for children to a separate proc and NEW to a controls themselves?          //TODO: Order of visual objects affects their TabStop?
{
 T* Ctrl = new T();
 Ctrl->OwnerObj  = this;            // Must be done BEFORE call to 'create' method   
 Ctrl->hOwnerMod = this->hOwnerMod;  
 Ctrl->IdxInPar  = this->FindFreeChildObjSlot();
 if(Ctrl->Create(args...) < 0){ delete(Ctrl); return nullptr; }     // TODO: Do not delete failed objects to avoid NULL poiners
 this->InsertChildObj(Ctrl, Ctrl->IdxInPar);
 return Ctrl;
}
//------------------------------------------------------------------------------------------------------------
CObjBase* GetObj(UINT Idx)
{
 if(!this->ChildLst || (Idx >= this->ChildCnt))return nullptr;
 return this->ChildLst[Idx];
}
UINT GetObjCnt(void) {return this->ChildCnt;}
CObjBase** GetObjLst(void) {return this->ChildLst;}
//------------------------------------------------------------------------------------------------------------
HWND GetOwnerHandle(void){return (this->OwnerObj && this->OwnerObj->IsDerivedFromAny(flDerWndBase))?(((CWndBase*)this->OwnerObj)->GetHandle()):((HWND)-1);}     // -1 is invalid HWND ?
CWndBase* GetOwnerWnd(void){return reinterpret_cast<CWndBase*>(this->OwnerObj);}   // No extra checks - used in event handlers, need perfomance
//------------------------------------------------------------------------------------------------------------

// DelObj
//------------------------------------------------------------------------------------------------------------
CObjBase(void){ }
//------------------------------------------------------------------------------------------------------------
virtual ~CObjBase()
{
 if(this->ChildCnt && this->ChildLst) 
  {
   for(UINT ctr=0;ctr < this->ChildCnt;ctr++){if(this->ChildLst[ctr])delete(this->ChildLst[ctr]);}
   delete(this->ChildLst);        //HeapFree(GetProcessHeap(), 0, this->ChildLst);
  }
}
//------------------------------------------------------------------------------------------------------------

};
