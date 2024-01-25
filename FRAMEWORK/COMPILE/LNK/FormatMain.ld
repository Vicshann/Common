
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

/* Small memory model is required on ARM, to avoid page relative addressing. So a last and largest payload should come AFTER data */
  .text : { *(.exebeg); *(entry); *(.text); *(.text.*); *(.rodata); *(.rodata.*); *(.rodend); }  /* 'rodend' is required for hashing to decrypt payload */
  .data : { *(.data); *(.data.*) }
  .bss : { *(.bss); *(.bss.*); FILL(0xB6DCAEB3); . = ALIGN(.,16); *(.exeend); }    /* Executable size, align to 16 for better joining */
  .bin : { *(.plbeg); *(.payload); *(.plend); }  /* Any additional, align to 16 for better joining */
/* Put all useless sections here*/
  /DISCARD/ :
  {
/*   *(*)  */
   *(.note*);
   *(.comment*);
   *(.dynamic*);
   *(.dynsym*);
   *(.dynstr*);
   *(.hash*);
   *(.gnu.hash*);
   *(.ARM.exidx);
   *(.ARM.attributes);
 /*  *(.shstrtab*); Mandatory - no way to avoid sections header */
  }
/* Export the code size.  ABSOLUTE(ALIGN(.)) ; +8 for the variable itself  Made negative for obfuscation*/
/* Unaccessible from C++ with 'tiny' memory model (unknown relocation against symbol) */

 /* PROVIDE_HIDDEN(__LNK_BINSIZE = (ALIGN(. + 4,16))); */
 /*. = ALIGN(.,16); */
}

