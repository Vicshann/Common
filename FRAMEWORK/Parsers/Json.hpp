
#pragma once
 


class CJsnItm  // Based on MemChain
{
union UJsnVal
{
 bool 		BoolVal;
 double		FloatVal;   // 64 bit
 __int64	IntVal;     // 64 bit    
 unsigned __int64 UIntVal;  // 64 bit    // Any pointers can be safely stored here  
 char*    StrVal;           // 32/64 bit
 CJsnItm* Items;            // 32/64 bit // Array/Object collection of values


};


};



