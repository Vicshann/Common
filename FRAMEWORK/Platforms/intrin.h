
// This file should be included by Clang`s include_next in its intrin.h

// Those functions must be reimplemented in ASM if compiling without '-fms-extensions'

#ifdef __x86_64__
unsigned char __readgsbyte(unsigned long);
unsigned short __readgsword(unsigned long);
unsigned long __readgsdword(unsigned long);
unsigned long long __readgsqword(unsigned long);
#endif

#ifdef __i386__
unsigned char __readfsbyte(unsigned long);
unsigned short __readfsword(unsigned long);
unsigned long __readfsdword(unsigned long);
unsigned long long __readfsqword(unsigned long);
#endif


