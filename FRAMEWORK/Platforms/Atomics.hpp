
#pragma once

//#include <intrin.h>

/*   // Such things increase compilation time
#ifdef _X64BIT
 static_assert((sizeof(N) == sizeof(char))&&(sizeof(N) == sizeof(short))&&(sizeof(N) == sizeof(long))&&(sizeof(N) == sizeof(long long)), "Operand size mismatch");
#else
 static_assert((sizeof(N) == sizeof(char))&&(sizeof(N) == sizeof(short))&&(sizeof(N) == sizeof(long)), "Operand size mismatch");
#endif

https://stackoverflow.com/questions/286629/what-is-a-memory-fence
The Linux kernel uses a gcc extension (asm __volatile__("": : :"memory")) to create a full compiler optimization barrier.
(.NET CLR) volatile reads are acquire fences, writes are release fences. Interlocked ops are full as is the MemoryBarrier method.

https://www.albahari.com/threading/part4.aspx#_NonBlockingSynch

std::atomic

Early AMD64 processors lacked the CMPXCHG16B instruction.

https://gcc.gnu.org/onlinedocs/gcc/_005f_005fsync-Builtins.html

These built-in functions perform the operation suggested by the name, and returns the value that had previously been in memory.
Built-in Function: type __sync_fetch_and_add (type *ptr, type value, ...)
Built-in Function: type __sync_fetch_and_sub (type *ptr, type value, ...)
Built-in Function: type __sync_fetch_and_or (type *ptr, type value, ...)
Built-in Function: type __sync_fetch_and_and (type *ptr, type value, ...)
Built-in Function: type __sync_fetch_and_xor (type *ptr, type value, ...)
Built-in Function: type __sync_fetch_and_nand (type *ptr, type value, ...)

These built-in functions perform the operation suggested by the name, and return the new value.  
Built-in Function: type __sync_add_and_fetch (type *ptr, type value, ...)
Built-in Function: type __sync_sub_and_fetch (type *ptr, type value, ...)
Built-in Function: type __sync_or_and_fetch (type *ptr, type value, ...)
Built-in Function: type __sync_and_and_fetch (type *ptr, type value, ...)
Built-in Function: type __sync_xor_and_fetch (type *ptr, type value, ...)
Built-in Function: type __sync_nand_and_fetch (type *ptr, type value, ...)     // GCC 4.4 and later implement __sync_nand_and_fetch as *ptr = ~(*ptr & value) instead of *ptr = ~*ptr & value.

warning: implicit use of sequentially-consistent atomic may incur stronger memory barriers than necessary [-Watomic-implicit-seq-cst]    // NOTE: All above are have 'seq-cst' memory model implicitly

// These built-in functions perform an atomic compare and swap. That is, if the current value of *ptr is oldval, then write newval into *ptr.
// The `bool` version returns true if the comparison is successful and newval is written. The `val` version returns the contents of *ptr before the operation.

https://gcc.gnu.org/wiki/Atomic/GCCMM/AtomicSync
https://gcc.gnu.org/onlinedocs/gcc/_005f_005fatomic-Builtins.html  // C++11 memory model
// Note that the `__atomic` builtins assume that programs will conform to the C++11 memory model. In particular, they assume that programs are free of data races. See the C++11 standard for detailed requirements.
// The `__atomic` builtins can be used with any integral scalar or pointer type that is 1, 2, 4, or 8 bytes in length. 16-byte integral types are also allowed if `__int128` (see 128-bit Integers) is supported by the architecture.
          __atomic_add_fetch (&Ptr[2], 4, 0); //__sync_fetch_and_add(&Ptr[2], 4);

https://gcc.gnu.org/onlinedocs/gcc/_005f_005fatomic-Builtins.html

Built-in Function: type __atomic_add_fetch (type *ptr, type val, int memorder)
Built-in Function: type __atomic_sub_fetch (type *ptr, type val, int memorder)
Built-in Function: type __atomic_and_fetch (type *ptr, type val, int memorder)
Built-in Function: type __atomic_xor_fetch (type *ptr, type val, int memorder)
Built-in Function: type __atomic_or_fetch (type *ptr, type val, int memorder)
Built-in Function: type __atomic_nand_fetch (type *ptr, type val, int memorder)

Built-in Function: type __atomic_fetch_add (type *ptr, type val, int memorder)
Built-in Function: type __atomic_fetch_sub (type *ptr, type val, int memorder)
Built-in Function: type __atomic_fetch_and (type *ptr, type val, int memorder)
Built-in Function: type __atomic_fetch_xor (type *ptr, type val, int memorder)
Built-in Function: type __atomic_fetch_or (type *ptr, type val, int memorder)
Built-in Function: type __atomic_fetch_nand (type *ptr, type val, int memorder)

Built-in Function: type __atomic_load_n (type *ptr, int memorder)
This built-in function implements an atomic load operation. It returns the contents of *ptr.

The valid memory order variants are __ATOMIC_RELAXED, __ATOMIC_SEQ_CST, __ATOMIC_ACQUIRE, and __ATOMIC_CONSUME.

Built-in Function: void __atomic_load (type *ptr, type *ret, int memorder)
This is the generic version of an atomic load. It returns the contents of *ptr in *ret.

Built-in Function: void __atomic_store_n (type *ptr, type val, int memorder)
This built-in function implements an atomic store operation. It writes val into *ptr.

The valid memory order variants are __ATOMIC_RELAXED, __ATOMIC_SEQ_CST, and __ATOMIC_RELEASE.

Built-in Function: void __atomic_store (type *ptr, type *val, int memorder)
This is the generic version of an atomic store. It stores the value of *val into *ptr.

Built-in Function: type __atomic_exchange_n (type *ptr, type val, int memorder)
This built-in function implements an atomic exchange operation. It writes val into *ptr, and returns the previous contents of *ptr.

All memory order variants are valid.

Built-in Function: void __atomic_exchange (type *ptr, type *val, type *ret, int memorder)
This is the generic version of an atomic exchange. It stores the contents of *val into *ptr. The original value of *ptr is copied into *ret.

Built-in Function: bool __atomic_compare_exchange_n (type *ptr, type *expected, type desired, bool weak, int success_memorder, int failure_memorder)
This built-in function implements an atomic compare and exchange operation. This compares the contents of *ptr with the contents of *expected. If equal, the operation is a read-modify-write operation that writes desired into *ptr. If they are not equal, the operation is a read and the current contents of *ptr are written into *expected. weak is true for weak compare_exchange, which may fail spuriously, and false for the strong variation, which never fails spuriously. Many targets only offer the strong variation and ignore the parameter. When in doubt, use the strong variation.

If desired is written into *ptr then true is returned and memory is affected according to the memory order specified by success_memorder. There are no restrictions on what memory order can be used here.

Otherwise, false is returned and memory is affected according to failure_memorder. This memory order cannot be __ATOMIC_RELEASE nor __ATOMIC_ACQ_REL. It also cannot be a stronger order than that specified by success_memorder.

Built-in Function: bool __atomic_compare_exchange (type *ptr, type *expected, type *desired, bool weak, int success_memorder, int failure_memorder)
This built-in function implements the generic version of __atomic_compare_exchange. The function is virtually identical to __atomic_compare_exchange_n, except the desired value is also a pointer.

*/
//---------------------------------------------------------------------------
/*template<typename N> constexpr FINLINE N FCALLCNV InterlockedAdd(volatile N* Val, N Num)   // Should fail compilation because of missing return value if type is not of any specified sizes
{
#ifdef _ISGCC    // Else try MSVC or fail
 return __sync_fetch_and_add(Val, Num);     // Same name for all sizes
#else           // Else try MSVC or fail
//  if constexpr (sizeof(N) == 8)return _InterlockedExchangeAdd8((char*)Val, (char)Num);
//    else _InterlockedExchangeAdd((long*)Val, (long)Num); 
/* switch(sizeof(N))
  {
   case 8:  return _InterlockedExchangeAdd8((char*)Val, (char)Num);
   case 16: return _InterlockedExchangeAdd16((short*)Val, (short)Num);
   case 32: return _InterlockedExchangeAdd((long*)Val, (long)Num);
#ifdef _X64BIT
   case 64: return 0;
#endif
  }  */
//#endif
// return 0;
//} */
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------

