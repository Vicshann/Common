
#pragma once
 
class CPropBag;

template<typename T> class CProp
{
 CPropBag* Bag;
 UINT PropIdx;

T* GetPropPtr(UINT Idx)
{

}

public:
//------------------------------------------------------------------------------
CProp(CPropBag* Owner, char* PropName, bool TypeSafe=true)
{

}
//------------------------------------------------------------------------------
 operator   const T&(){return (char*)this->c_str();}
//------------------------------------------------------------------------------
};




