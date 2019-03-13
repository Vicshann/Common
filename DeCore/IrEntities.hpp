
#pragma once

//===========================================================================
// NOTES: 
// - All Enum elements used as bit masks for fast comparision
// - Allocator of IStream should NOT split single allocations across memory blocks!
// - A Entities must NOT contain any objects, only pointers to an objects are allowed!
// - Anything in EFlags may be combined and may be combined with ETypes. Anything in ETypes should be combined only with EFlags 
//
template<typename T> struct SBaseItem   // Size: 4     // Redefine here operators new and delete if there are BUGS in originals
{
 UINT8  SLen;        // Size of an entire items array   // Max is 64 // Bit 7 is free to store some special flag ?
 UINT8  SExt;        // Bit 1 for each value marks it as UINT64/x64 Pointer   // Helps to store only low half of UINT64 if High half is empty
 UINT16 Flags;       // Limit is 16 mask bits!
     
//...........................................................................
protected:
UINT32* GetValArr(void){return (UINT32*)&((UINT8*)this)[sizeof(T)];}
//...........................................................................
template<typename T> static UINT CountReqSlotsForVal(T Val)
{
 if constexpr (Is_Pointer<T>::value)return (sizeof(void*) / sizeof(UINT32));    // Always Full-sized
 UINT Cnt = (bool)((UINT32)Val & 0xFFFFFFFF);
 if constexpr (sizeof(T) > sizeof(UINT32))Cnt += (int)((bool)((UINT64)Val & ~(UINT64)0xFFFFFFFF)) + (int)((bool)!Cnt);   // If we have nonzero HI part them we must have and low part as well
 return Cnt;
}
//...........................................................................
UINT GetValSlot(UINT ValIdx){return (*this->GetValArr() >> (ValIdx*4)) & 0x0F;}   // Index is a value index, not a UINT32 index. A value can occupy more than one slot   // (Idx > 7) will result in Num=0    // ValMsk is stored in first slot and must be added to ValStream before other values (Because of f this an offset of first Val will be > 0)
UINT GetValSize(UINT ValIdx){return sizeof(UINT32) + (((this->SExt >> ValIdx) & 1) * sizeof(UINT32));}   // No check if value is present
bool IsExtValLen(UINT ValIdx){return (this->SExt & (1 << ValIdx));}          
//...........................................................................
template<typename T> T GetValue(UINT ValIdx)             // Supports only UINT32/UINT64 but UINT64 can be stored as UINT32      
{
 UINT32* ValArr = this->GetValArr();
 UINT Num = this->GetValSlot(ValIdx);      // (*ValArr >> (ValIdx*4)) & 0x0F;
 if(Num)
  {
   if(this->IsExtValLen(ValIdx))return (T)(*(UINT64*)&ValArr[Num]);       // Use constexpr if and check IsExtValLen only for sizeof(T) > 4? // In that case some UINT64 slots can be set only half with UINT32 by mistake
    else return (T)(*(UINT32*)&ValArr[Num]);
  }
 return 0;     
}
//...........................................................................
template<typename T> void SetValue(UINT ValIdx, T Val)   // Supports only UINT32/UINT64 but UINT64 can be stored as UINT32   
{
 UINT32* ValArr = this->GetValArr();
 UINT Num = this->GetValSlot(ValIdx);      // (*ValArr >> (ValIdx*4)) & 0x0F;
 if(Num)
  {
   if(this->IsExtValLen(ValIdx))*(UINT64*)&ValArr[Num] = (UINT64)Val;
    else *(UINT32*)&ValArr[Num] = (UINT32)Val;
  }   
}
//...........................................................................
void    InitValue(void* Val, UINT Idx, UINT Slots=1)     // Idx is 0-7; No checks     // Max 8 values, 15 slots of UINT32  // ValMsk is in slot 0   // On x64 each pointer will take two slots
{
 if(!Slots || !Val)return;
 UINT32* ValArr = this->GetValArr(); 
 UINT Num    = (this->SLen / sizeof(UINT32));    // Index of first slot
 *ValArr    |= (Num << (Idx*4));    
 this->SLen += (sizeof(UINT32)*Slots);
 if(Slots > 1)
  {
   *(UINT64*)&ValArr[Num] = *(UINT64*)Val;       // Should be an error if more than 2 slots
   this->SExt |= (1 << Idx);                     // Mark it as extended
  }
   else if(Slots)*(UINT32*)&ValArr[Num] = *(UINT32*)Val;
}
//...........................................................................
static T* Allocate(IStream* Strm, UINT ValSlots)    // 'Create' method of each class should know how many ValSlots is needed to allocate an entire object on stream at once
{                                                   // All of ValSlots MUST BE initialized using InitValue or size of the object will be incorrect!
 ValSlots  += (bool)ValSlots;  // One for ValMsk    // Can describe max 8 value fields. 4 bits for each field. (0-Not Present. Val-1 Is index in ValArray) 
 size_t slt = (ValSlots*sizeof(UINT32));
 size_t Len = sizeof(T)+slt; 
 size_t Off = Strm->GetPos(); 
 if(Strm->Write(nullptr, Len) != Len)return nullptr;     // Writes entire derived object there
 T* This = (T*)Strm->Data(Off, &Len);
 FillMem(This, 0, Len);
 This->SLen = int((bool)ValSlots) * sizeof(UINT32);      // One for ValMsk
 return This;
}
//...........................................................................
T* ToStream(IStream* Strm)
{

}
//...........................................................................
T* FindSame(IStream* Strm)      // A binary tree will be much faster for duplicate search?
{

}
//...........................................................................

public:
SBaseItem(void) = delete; // will never be generated    // Constructors are meaningless here. Those obgects are created in stream and most be gurantieed not to be split
//...........................................................................
size_t  GetSize(void) {return this->SLen + sizeof(T);} 
//...........................................................................
T* Clone(IStream* DstStrm)
{
 size_t Len = 0; 
 size_t Off = Strm->GetPos();
 Strm->Write(this, this->GetSize());            // Writes entire derived object there
 return (T*)Strm->Data(Off, &Len);
}
//...........................................................................
bool IsExist(void){return (bool)this->Type;}  // Type is a bitmask
//...........................................................................

 bool IsSame(const T* item){if(item == this)return true; else return !memcmp(this,item,sizeof(T));}    // Returns TRUE if equal     // memcmp is Much faster but you must leave no garbage between members of this struct
 bool IsRelated(const T* item){return this->IsSame(item);}   // TODO: Do full relationship check, not just memory comparing!

 void  operator =  (const T &ent) {this->Clone(&ent);}
 void  operator =  (const T *ent) {this->Clone(ent);}
 bool  operator == (const T &ent) {return this->IsSame(&ent);}
 bool  operator == (const T *ent) {return this->IsSame(ent);}

};
//===========================================================================
// NOTES: 
//
struct SIECondition : public SBaseItem<SIECondition>  // <, >, ==, !=, ...
{                  
enum EFlags {fNone=0,            // Ignore FlagMask
             fNot     = 0x0001,  // (Same as Z flag)
             fLess    = 0x0002,
             fGreat   = 0x0004,
             fEqual   = 0x0008,
             fSpecial = 0x0010,  // Some unknow type of condition. Analyze can be done by custom analyzer using FlagMask field
};   // Combination
enum ETypes {tNone=0,            // Opposite to 'Not', cannot be used in combination (Same as NZ flag)
             tAnd     = 0x0100,  // Perfom a bitwise AND on FlagMask
             tXor     = 0x0200,  // Perfom a bitwise XOR on FlagMask
             tOr      = 0x0400,  // Perfom a bitwise OR  on FlagMask
             tCompare = 0x0800,  // Compare FlagMask
};   // One of 

//...........................................................................
static SIECondition* Create(IStream* Strm, UINT flags, size_t hint, UINT32 fmask=0)
{
 CStream<UINT8, CBufStatic<(sizeof(SIECondition)+(sizeof(UINT32)*8))> > TmppObj;
 UINT s_hint  = CountReqSlotsForVal(hint); 
 UINT s_fmask = CountReqSlotsForVal(fmask);
 SIECondition* Temp = SIECondition::Allocate(&TmppObj, s_hint+s_fmask);   
 Temp->Flags  = flags;                                                                 
 Temp->InitValue(&hint,  0, s_hint);            // An User defined Hint value - Can be used as pointer to some struct if need so
 Temp->InitValue(&fmask, 1, s_fmask);           // Mask of checked flags, must be related to operation`s resulting FlagMask (After bitwise AND must remain unchanged)   // 32 bits should be enough for most architectures
 SIECondition* This = Temp->FindSame(Strm);
 if(!This)This = Temp->ToStream(Strm);
 return This;
}
//...........................................................................
size_t GetHint(void){return this->GetValue<size_t>(0);}
//...........................................................................
UINT32 GetMask(void){return this->GetValue<UINT32>(1);}       
//...........................................................................
//...........................................................................
void SetHint(size_t Val){this->SetValue<size_t>(0, Val);}  // Cannot set value wider than original
//...........................................................................
void SetMask(UINT32 Val){this->SetValue<UINT32>(1, Val);}       
//...........................................................................

};
//===========================================================================
// NOTES:
// - Set 'Numberic' field to some uniqe value  
// - All registers and uniqe memory addresses will be stored as separate SIEOperand instances
//
struct SIEOperand : public SBaseItem<SIEOperand>       // EAX, @R18, #0BE34, [046C7], ...
{                       
enum EOpdFlags {fNone=0,
                fNumber    = 0x0001,   // The 'Numberic' field is valid, use it if needed  (Real number, not a virtual as for register A, for example, and can be used by a code generator)
                fPointer   = 0x0002,   // The Value is used for addressing a memory location (Any assignments must be preserved, not collapsed)  (BitSize is size of data pointed to and need to check if it is part of something(register bank, for example))
                fSigned    = 0x0004,   // Hint from a disassembler, force display the number as signed
                fBitValue  = 0x0008,   // The operand is a bit(s) // For bit addressing set BitSize to number of bits, BitOffs to index of lowest bit and Numberic to identify a memory or register location
                fStack     = 0x0010,   // Stack addressing, use instead of 'Pointer' as a hint for stack tracing
                fLocal     = 0x0020,   // The variable is local for subroutine(Stack, mostly) and can be optimized out, if needed  (Make sence only with a 'Pointer' flag for a stack or memory variables)
                fGlobal    = 0x0040,   // The variable is global (interprocedural) and cannot be optimized out (Make sence only with a 'Pointer' flag for a stack or memory variables)
                fTemporary = 0x0080,   // Must stay invisible in decompiled code an be fully collapsed at the end of decompilation with no assignments left  // The parser may declare a Temp variables for suboperations
                fForceKeep = 0x0100,   // If the variable assignment has been done the always keep it and use for linking, do not collapse onto expressions
//                fFullHEX   = 0x0200,   // Hint from a disassembler, display as fill sized HEX value, don`t trim to highest nonzero byte (0x000000FF instead 0xFF)
//                fHexFormat = 0x0400,   // Hint from a disassembler, force display the number as HEX
//                fDecFormat = 0x0800,   // Hint from a disassembler, force display the number as DEC  
};   // Combination
enum EOpdTypes {tNone=0,               // Not present ??? Use 'Excluded' flag instead
                tStatic    = 0x1000,   // The name is just copied unchanged, no collapsing used  (i.e String)
                tConstant  = 0x2000,   // Can be used in operations but cannot be a destination operand
                tVariable  = 0x4000,   // Can be used in operations and be a destination operand
};   // One of

//...........................................................................
static SIEOperand* Create(IStream* Strm, UINT flags, size_t hint, UINT64 numb=0, UINT32 size=0, UINT32 offs=0)
{
 CStream<UINT8, CBufStatic<(sizeof(SIEOperand)+(sizeof(UINT32)*8))> > TmppObj;
 UINT s_OpTr = 0;
 UINT s_hint = CountReqSlotsForVal(hint); 
 UINT s_numb = CountReqSlotsForVal(numb);
 UINT s_size = CountReqSlotsForVal(size);
 UINT s_offs = CountReqSlotsForVal(offs);
 if(flags & fBitValue)s_OpTr = sizeof(void*) / sizeof(UINT32);
 SIEOperand* Temp = SIEOperand::Allocate(&TmppObj, s_hint+s_numb+s_size+s_offs+(s_OpTr*3));   
 Temp->Flags = flags;                                                              
 Temp->InitValue(&hint, 0, s_hint);           // An User defined Hint value - Can be used as pointer to some struct if need so
 Temp->InitValue(&numb, 1, s_numb);           // Used for a operands distinction when collapsing, must be uniqe for same operands (register numbers or memory locations) // Faster, than comparing by names  // For registers this is a Virtual Offset in register bank, real or not
 Temp->InitValue(&size, 2, s_size);           // Size in bits of a numberic operand. Used for assignments size control and register access (Helps treat AL as part of EAX, for example)
 Temp->InitValue(&offs, 3, s_offs);           // Offset of lowest bit of operand inside of Parent operand type
 if(flags & fBitValue)
  {
   void* Val = nullptr;
   Temp->InitValue(&Val, 4, s_OpTr);    // Pointer to any first operand contained (overlapped) with current, other accessible trough 'SiblingOp' field (i.e AL in EAX)
   Temp->InitValue(&Val, 5, s_OpTr);    // Parent operand pointer, i.e. EAX for AL or AH   // !!! Do relationchip analysis BEFORE operand collapsing  // TODO: Operand relationchip !!!!!!!!!!!!
   Temp->InitValue(&Val, 6, s_OpTr);    // i.e. AH for AL // All 3 pointers must be assigned(If needed) after all operands created for a target machine  // !!! Or just check later bit overlapping (BitSize,BitOffs) which must be set appropriate for registers and calculated later for a memory/array addresses?
  }
 SIEOperand* This = Temp->FindSame(Strm);
 if(!This)This = Temp->ToStream(Strm);
 return This;
}
//...........................................................................
size_t      GetHint(void){return this->GetValue<size_t>(0);}
//...........................................................................
UINT64      GetNumberic(void){return this->GetValue<UINT64>(1);}
//...........................................................................
UINT32      GetBitSize(void){return this->GetValue<UINT32>(2);}  
//...........................................................................
UINT32      GetBitOffs(void){return this->GetValue<UINT32>(3);}
//...........................................................................
SIEOperand* GetChildOp(void){return this->GetValue<SIEOperand*>(4);}
//...........................................................................
SIEOperand* GetParentOp(void){return this->GetValue<SIEOperand*>(5);}
//...........................................................................
SIEOperand* GetSiblingOp(void){return this->GetValue<SIEOperand*>(6);}
//........................................................................... 
//...........................................................................
void SetHint(size_t Val){this->SetValue<size_t>(0, Val);}  // Cannot set value wider than original
//...........................................................................
void SetNumberic(UINT64 Val){this->SetValue<UINT64>(1, Val); }  // Cannot set value wider than original 
//...........................................................................
void SetBitSize(UINT32 Val){this->SetValue<UINT32>(2, Val);}       
//...........................................................................
void SetBitOffs(UINT32 Val){this->SetValue<UINT32>(3, Val);}       
//...........................................................................
void SetChildOp(SIEOperand* Val){this->SetValue<SIEOperand*>(4, Val);}
//...........................................................................
void SetParentOp(SIEOperand* Val){this->SetValue<SIEOperand*>(5, Val);}
//...........................................................................
void SetSiblingOp(SIEOperand* Val){this->SetValue<SIEOperand*>(6, Val);}
//...........................................................................
bool IsGlobal(void){return (this->Flags & fGlobal);}
bool IsVariable(void){return (this->Flags & tVariable);}
bool IsPointerTo(void){return (this->Flags & fPointer);}
bool IsMustKeepDef(void){return (this->Flags & (fGlobal|fPointer|fForceKeep));}
//...........................................................................

};
//---------------------------------------------------------------------------
// NOTES:
// - A conditional operations, separate from a conditions checks and usage of various flags for conditions make impossible to correctly decompile such operations in some situations.
//
struct SIEOperation : public SBaseItem<SIEOperation>      // +, -, =, *, ...
{                 
enum EFlags {fNone=0,          
             fModFlags = 0x0001,   // Changes the machine specific flags (FlagMask is valid)
};   // Combination
enum ETypes {tNone=0,              // Not present, has no meaning and can be skipped by the Decompiler
             tStatic   = 0x0100,   // The name is just copied unchanged, no collapsing used
             tAssign   = 0x0200,   // Assignment operation (Starting direct assignment, meaning ValA = ValB) Any other operations (opStandart) is mostly mean ValA = ValX op ValY
             tStandart = 0x0400,   // A standart operation (+,-,*,...) with Left and Right operands, UHint contains info for a Code Generator about it
             tSpecial  = 0x0800,   // Has no meaning for the Decompiler, but not same as 'opNone'.  Use it for a special uniqe operations
             tJump     = 0x1000,   // Jump to address or index
             tCall     = 0x2000,   // Subroutine call, the arguments list must be determined after decompilation of this(all?) subroutine
             tReturn   = 0x4000,   // Return from the subroutine (Take Value from Right value of top nearest 'otTempVar' assignment)
};   // One of 
  
//...........................................................................
static SIEOperation* Create(IStream* Strm, UINT flags, size_t hint, UINT32 fmask=0)
{
 CStream<UINT8, CBufStatic<(sizeof(SIEOperation)+(sizeof(UINT32)*8))> > TmppObj;
 UINT s_hint  = CountReqSlotsForVal(hint); 
 UINT s_fmask = CountReqSlotsForVal(fmask);
 SIEOperation* Temp = SIEOperation::Allocate(&TmppObj, s_hint+s_fmask);   
 Temp->Flags  = flags;                                                             
 Temp->InitValue(&hint,  0, s_hint);            // An User defined Hint value - Can be used as pointer to some struct if need so
 Temp->InitValue(&fmask, 1, s_fmask);           // Resulting FlagMask of the operation  (Possibly Affected flags)
 SIEOperation* This = Temp->FindSame(Strm);
 if(!This)This = Temp->ToStream(Strm);
 return This;
}
//...........................................................................
size_t GetHint(void){return this->GetValue<size_t>(0);}
//...........................................................................
UINT32 GetMask(void){return this->GetValue<UINT32>(1);}       
//...........................................................................
//...........................................................................
void SetHint(size_t Val){this->SetValue<size_t>(0, Val);}  // Cannot set value wider than original
//...........................................................................
void SetMask(UINT32 Val){this->SetValue<UINT32>(1, Val);}       
//...........................................................................

};
//===========================================================================
// Formats: LeftValue Operation RightValue
//          LeftValue
// 
// NOTES:
// - Operands with a different numberic values(constants and addresses) can result in creation of large amount of SIEInstruction objects
//
struct SIEInstruction : public SBaseItem<SIEInstruction>
{                           
enum EFlags {fNone=0,
             fSOpIndex   = 0x0001,    // Operand of jump is actually index of SubOperation not an address (To one address can be assigned whole list of suboperations)
             fBranDirect = 0x0002,    // The instruction is a direct Jump/Call (Any conrol transfer instruction where target address may be calculated from the operands) // This flag saves from analyzing argument flags
             fBreakFlow  = 0x0004,    // The instrucrion Exits from the control flow (To unknow direction with no return if this is not a RET operation)  // Any RET or decided by the Analyzer/User
             fNotSep     = 0x0008,    // Hint from a disassembler, do not separate by spaces Left and Right operand from the Operation 'Value' (Used for some Static operands)
};
enum ETypes {tNone=0};      

//...........................................................................
static SIEInstruction* Create(IStream* Strm, UINT flags, size_t hint, SIEOperation* oper=nullptr, SIEOperand* dest=nullptr, SIEOperand* left=nullptr, SIEOperand* right=nullptr, SIECondition* cond=nullptr)
{
 CStream<UINT8, CBufStatic<(sizeof(SIEInstruction)+(sizeof(UINT32)*8))> > TmppObj;
 UINT s_hint  = CountReqSlotsForVal(hint); 
 UINT s_oper  = CountReqSlotsForVal(oper);
 UINT s_cond  = CountReqSlotsForVal(cond);
 UINT s_dest  = CountReqSlotsForVal(dest);
 UINT s_left  = CountReqSlotsForVal(left);
 UINT s_right = CountReqSlotsForVal(right);                                                           
 SIEInstruction* Temp = SIEInstruction::Allocate(&TmppObj, s_hint+s_oper+s_cond+s_dest+s_left+s_right);  
 Temp->Flags  = flags;                                                                 
 Temp->InitValue(&hint,  0, s_hint);            // An User defined Hint value - Can be used as pointer to some struct if need so
 Temp->InitValue(&oper,  1, s_oper);            // Current operation description
 Temp->InitValue(&cond,  2, s_cond);            // Condition for this operation, if used // Executed only if the condition match (Just take Left and Right values from top nearest operation with 'ModFlags', no other solution exist!)
 Temp->InitValue(&dest,  3, s_dest);            // Pointer to a Destination operand instance (i.e OpDest-OpLeft-Operation-OpRight is 'A = B + C' or 'A = A + B')
 Temp->InitValue(&left,  4, s_left);            // Pointer to a Left operand instance
 Temp->InitValue(&right, 5, s_right);           // Pointer to a Right operand instance (i.e. NULL for assignments like A = 5)
 SIEInstruction* This = Temp->FindSame(Strm);
 if(!This)This = Temp->ToStream(Strm);
 return This;
}
//...........................................................................
size_t        GetHint(void){return this->GetValue<size_t>(0);}
//...........................................................................
SIEOperation* GetOper(void){return this->GetValue<SIEOperation*>(1);}      // Using pointers will increase memory consumption on x64 and harder to serialize 
//...........................................................................
SIECondition* GetCond(void){return this->GetValue<SIECondition*>(2);}
//...........................................................................
SIEOperand*   GetDest(void){return this->GetValue<SIEOperand*>(3);}
//...........................................................................
SIEOperand*   GetLeft(void){return this->GetValue<SIEOperand*>(4);}
//...........................................................................
SIEOperand*   GetRight(void){return this->GetValue<SIEOperand*>(5);}
//...........................................................................
//...........................................................................
void SetHint(size_t Val){this->SetValue<size_t>(0, Val);}  // Cannot set value wider than original
//...........................................................................
void SetOper(SIEOperation* Val){this->SetValue<SIEOperation*>(1, Val);}       
//...........................................................................
void SetCond(SIECondition* Val){this->SetValue<SIECondition*>(2, Val);}
//...........................................................................
void SetDest(SIEOperand* Val){this->SetValue<SIEOperand*>(3, Val);}
//...........................................................................
void SetLeft(SIEOperand* Val){this->SetValue<SIEOperand*>(4, Val);}
//...........................................................................
void SetRight(SIEOperand* Val){this->SetValue<SIEOperand*>(5, Val);}
//...........................................................................

};
//===========================================================================
//
class CInstruction : public SBaseItem<CInstruction>
{
 ULONG Index;                   // Sequental index, must be uniqe (Used in a suboperational jumps calculation) // Has no meaning for analysis and may become unsorted   // Useful for debugging - graph dumping
 ULONG Address;                 // Must be uniqe (Used in an operational jumps calculation)  // Has no meaning for analysis and may become unsorted   // Useful for debugging - graph dumping
 ULONG InstrOffs;               // Instruction instance offset (SIEInstruction)

};
//===========================================================================
