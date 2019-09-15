
#pragma once	

/*
  Copyright (c) 2018 Victor Sheinmann, Vicshann@gmail.com

  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), 
  to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
  and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. 
*/


/*

DeCore does optimization on an Operands stream, significantly reducing the number of excess operation on some hardware architectures(i.e. PowerPC or Motorola variants) or in case of obfuscation.
Control flow instructions, which orphaned after an Operand Optimization are also removed.
If some functions share some parts of a code(tails) that parts are copied in each of them and optimized separately afterwards. 

Objects:
 'Address Space' (UINT64) - Defines a range of bytes. For an instruction argument tracing. Registers bank and IO space usually implemented as a separate AddrSpace object from a memory AddrSpace object
                             Operand ranges are traced and linked only inside same Address Space. A real pointer to memory can be assigned(Not as serializable prop). Owns list of 'Operand Range' objects, not pointers to them(to have separate lists for registers for faster search)
 'Operand Range' (UINT64) - A range of bits in owner 'Address Space' to use as an operand. Can be relative a named marker in some 'Address Space'(A Register or a Constant) 
                            Platform Property determines a byte order for byte access(Dynamical Prop). Ref counting is not used because it can consume too much memory(UINT64 for each object)
                            Overlaps of operands are possible and considered while linking. All unique accesses to some constant addresses are separate operands of type (ConstAddr). (Such types are custom) 
                            'Processor Flags' also defined as OperandRange, even if it is only one bit. Because on many architectures flags can be modified by direct access to a memory location or to a Register.
                            

 'Micro Operation'  - A custom constructed operation on some operand groups. Have two groups of operands: Source and Destination. Operands must be registered first in an OperandsList
                      (Does Ref counting on them? No need. They still can be useful if some other function will be added later (i.e. Revealed by emulation))
                      Any type, with which a 'Micro Operation' marked are custom and used in a callback for Decompilation/Emulation outside DeCore.

List of found procedures is stored. A stream of 'Micro Operation' object copies is assigned to each of them as 'DynMicroOp'. 
'DynMicroOp' and 'DynOpRange' also can hold a custom context for emulation.
Copies needed because a list of operands may be changed by transformation and operations themselves can be removed.
Transform: Sequential assignments to same range should be grouped (i.e. Halves assignment to a register on PowerPC)

A memory access operands can be marked as collapsible or not. i.e. Stack access operands can be collapsed in most cases, but a global variable access shouldn`t because same 
location can be accessed from an another thread or an interrupt handler. Same for IO ports or some hardware timer registers 

*/

#pragma pack(push,1)

#pragma pack(pop)
//=====================================================================================================================