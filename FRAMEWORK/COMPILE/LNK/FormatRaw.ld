
/*
OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")


https://wiki.osdev.org/Linker_Scripts
https://lld.llvm.org/ELF/linker_script.html
https://stackoverflow.com/questions/28837199/can-i-use-preprocessor-directives-in-ld-file

In practice the default linker behaviour will most significantly differ from ld.bfd by sorting ro-data before ro-code rather than after. This tends towards 3 PT_LOAD program headers ro, ro+exec, rw.
NOTE: LLD puts rodata first to merge it with ELF header
      We want to merge it with .text to have any raw bytecode executable and put it after .text to have first intruction as EP
*/

/*ENTRY(SysEntry) */

/*
Remove useless programs for ELF header, PHDRs and empty GNU stack (to make it RW only)
Don`t care if stack will be executable on x32 architectures
*/
PHDRS
{
    READONLY PT_LOAD FILEHDR PHDRS;
    READWRITE PT_LOAD;
}

SECTIONS
{
/*
https://stackoverflow.com/questions/15400910/ld-linker-script-producing-huge-binary

The first output section appears to share a memory page with the output file headers.
If the first section overlaps with the headers, LD emits a full page of pad bytes
before outputting the first section to avoid a conflict

https://stackoverflow.com/questions/64961708/is-there-a-way-to-remove-segment-gnu-stack
*/
  . = SIZEOF_HEADERS;
/*  offset = .;
  .raw : { LONG(SIZEOF_HEADERS) }   */
/*  .text : { *(.text); *(.text.*) }
  .rodata : { *(.rodata); *(.rodata.*) }   */

  .text : { *(entry); *(.text); *(.text.*); *(.rodata); *(.rodata.*) }
  .data : { *(.data); *(.data.*) }
  .bss : { *(.bss); *(.bss.*); }    /*If BSS is a last section in the binary it will be excluded*/
  .bin : { FILL(0xB6DCAEB3); . = ALIGN(.,16); *(binsize);}  /* Some data after BSS is needed or it will be thrown out. Filling will be removed by the Builder */
/* Put all useless sections here*/
  /DISCARD/ :
  {
   *(.note*);
   *(.hash*);
   *(.gnu.hash*);
   *(.dynsym*);  /*Import/export*/
   *(.dynstr*);  /*Strings referenced from .dynsym*/
   *(.dynamic*); /* References imported libraries. Disabled only by '-Wl,--no-pie'*/
   *(.comment*);
   *(.ARM.exidx*);  /*The section containing information for unwinding the stack*/
   *(.ARM.attributes);
  }
/* Export the code size.  ABSOLUTE(ALIGN(.)) ; +8 for the variable itself  Made negative for obfuscation*/
/* Unaccessible from C++ with 'tiny' memory model (unknown relocation against symbol)
  PROVIDE_HIDDEN(__LNK_BINSIZE = -(ALIGN(.,8)+8)); */
}

