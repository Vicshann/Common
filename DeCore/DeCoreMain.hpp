
#pragma once

//#ifndef DecompilerCoreH
//#define DecompilerCoreH

//---------------------------------------------------------------------------
#include "Framework.hpp"


//#include "Containers\\DynArray.h"
//#include "Containers\\DynBuffer.h"
//#include "Containers\\MinString.h"
//#include "Containers\\LinkedList.h"
//#include "Containers\\StringList.h"

#pragma warning(disable:4800)          // forcing value to bool 'true' or 'false' (performance warning)     // TODO: Reenable it afterwards

namespace NDeCore
{
using namespace NFRM;

#pragma pack( push, 1 )                //  Saves some memory

#include "IrEntities.hpp"
//#include "Objects\\BaseLists.h"
//#include "Objects\\BaseArrays.h"
//#include "Objects\\EntityLists.h"
//#include "Objects\\InstrDefsList.h"
//#include "Objects\\InstructionItem.h"
//#include "Objects\\BranchVisList.h"
//#include "Objects\\EntryDefsList.h"
//#include "Objects\\DecompItem.h"
//#include "Objects\\EntryItem.h"

#pragma pack( pop )
}
//--------------------------------------------------------------------------
//                  COMPILER`S LINEAR STUPIDITY FIXES
//--------------------------------------------------------------------------
/*static __inline void __fastcall AddRetOperand(CEntryPoint* Entry, SIEOperand* Oprnd)    // A STUPID LINEAR THINKING C++ standart designers!!! - see usage in CInstruction::LinkOperandTo
{
 Entry->RetValuesList()->PutNoDup(Oprnd);
} */
//--------------------------------------------------------------------------
/*static __inline bool IsDestOperandSame(CInstruction* Instr, SIEOperand* Oprnd)
{
 return Instr->Instr->OpDest->IsSame(Oprnd);
}*/
//--------------------------------------------------------------------------
/*static __inline SIEOperand* __fastcall GetDestOperand(CInstruction* Instr)
{
 return Instr->Instr->OpDest;
}
//--------------------------------------------------------------------------
static __inline void __fastcall CheckDefsAtPoint(SInsDefItem* DItm, CInstrDefsList* SrcList)   // Merges DefsList and marks DoubleRefOperand instructions (Used in CALL refs gathering for detection of arguments which require creation of temporary variavles (Argument which is fully defined in 'if-else' statement))
{          
 SInsDefItem* FItm = SrcList->Find(DItm->DefInstr->Instr->OpDest); // Find instruction with same Dest operand
 if(!FItm)return;                               // Does not adds anything to the list, just updates what is redefined
 if(FItm->DefInstr == DItm->DefInstr)return;    // Nothing is changed // What about 'LnkInstr' instruction?
 FItm->LnkInstr->Flags |= ifVarCandidate;       // A Link instruction? Even if it may not have a Dest operand at all!
 DItm->LnkInstr->Flags |= ifVarCandidate;       // A Link instruction? Even if it may not have a Dest operand at all!
 DItm->DefInstr = FItm->DefInstr;
 DItm->LnkInstr = FItm->LnkInstr;  
}  */
//--------------------------------------------------------------------------

