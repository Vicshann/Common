
#pragma once

// https://github.com/embedded2014/elf-loader/blob/master/elf.h
// https://github.com/torvalds/linux/blob/master/include/uapi/linux/elf.h
// https://gist.github.com/x0nu11byt3/bcb35c3de461e5fb66173071a2379779
// https://github.com/0xAX/linux-insides
// https://gist.github.com/CMCDragonkai/10ab53654b2aa6ce55c11cfc5b2432a4

template<typename PHT=PTRCURRENT> struct NFMTELF
{
using PVOID    = SPTR<void, PHT>;
using SIZE_T   = decltype(TypeToUnsigned<PHT>());  //  SPTR<uint,   PHT>;
using SSIZE_T  = decltype(TypeToSigned<PHT>());    //  SPTR<sint,   PHT>;

//static constexpr uint  EI_NIDENT = 16;
static constexpr uint8  ELFMAG[] = {0x7F,'E','L','F'};
static constexpr uint32 SIGN_ELF = 0x464C457F;   // LE, need conditioning
//------------------------------------------------------------------------------------------------------------
enum EHFlags
{
//  EI_CLASS = 4,    // File class byte index
  ELFCLASSNONE = 0,    // Invalid class
  ELFCLASS32 = 1,    // 32-bit objects
  ELFCLASS64 = 2,    // 64-bit objects
//  ELFCLASSNUM = 3,
//  EI_DATA = 5,    // Data encoding byte index
  ELFDATANONE = 0,    // Invalid data encoding
  ELFDATA2LSB = 1,    // 2's complement, little endian
  ELFDATA2MSB = 2,    // 2's complement, big endian
//  ELFDATANUM = 3,
//  EI_VERSION = 6,    // File version byte index
  // Value must be EV_CURRENT
//  EI_OSABI = 7,    // OS ABI identification index
  ELFOSABI_NONE = 0,    // UNIX System V ABI
  ELFOSABI_SYSV = 0,    // Alias.
  ELFOSABI_HPUX = 1,    // HP-UX
  ELFOSABI_NETBSD = 2,    // NetBSD.
  ELFOSABI_GNU = 3,    // Object uses GNU ELF extensions.
  ELFOSABI_LINUX = ELFOSABI_GNU,    // Compatibility alias.
  ELFOSABI_SOLARIS = 6,    // Sun Solaris.
  ELFOSABI_AIX = 7,    // IBM AIX.
  ELFOSABI_IRIX = 8,    // SGI Irix.
  ELFOSABI_FREEBSD = 9,    // FreeBSD.
  ELFOSABI_TRU64 = 10,    // Compaq TRU64 UNIX.
  ELFOSABI_MODESTO = 11,    // Novell Modesto.
  ELFOSABI_OPENBSD = 12,    // OpenBSD.
  ELFOSABI_ARM_AEABI = 64,    // ARM EABI
  ELFOSABI_ARM = 97,    // ARM
  ELFOSABI_STANDALONE = 255,    // Standalone (embedded) application
//  EI_ABIVERSION = 8,    // ABI version index
//  EI_PAD = 9,    // Byte index of padding bytes

  // Legal values for e_type (object file type).
  ET_NONE   = 0,         // No file type
  ET_REL    = 1,         // Relocatable file
  ET_EXEC   = 2,         // Executable file
  ET_DYN    = 3,         // Shared object file    // Always use this (The kernel won`t mind loading it as an executable and the loader will be happy too if LD_PRELOAD or importing is used to load the module)    // This indicats the position-independent executables
  ET_CORE   = 4,         // Core file
  ET_NUM    = 5,         // Number of defined types
  ET_LOOS   = 0xfe00,    // OS-specific range start
  ET_HIOS   = 0xfeff,    // OS-specific range end
  ET_LOPROC = 0xff00,    // Processor-specific range start
  ET_HIPROC = 0xffff,    // Processor-specific range end

// These constants are for the segment types stored in the image headers
  PT_NULL    = 0,
  PT_LOAD    = 1,
  PT_DYNAMIC = 2,
  PT_INTERP  = 3,
  PT_NOTE    = 4,
  PT_SHLIB   = 5,
  PT_PHDR    = 6,
  PT_TLS     = 7,               // Thread local storage segment
  PT_LOOS    = 0x60000000,      // OS-specific
  PT_HIOS    = 0x6fffffff,      // OS-specific
  PT_LOPROC  = 0x70000000,
  PT_HIPROC  = 0x7fffffff,
  PT_GNU_EH_FRAME = (PT_LOOS + 0x474e550),
  PT_GNU_STACK    = (PT_LOOS + 0x474e551),
  PT_GNU_RELRO    = (PT_LOOS + 0x474e552),
  PT_GNU_PROPERTY = (PT_LOOS + 0x474e553),

  // Legal values for e_machine (architecture).
  EM_NONE = 0,    // No machine
  EM_M32 = 1,    // AT&T WE 32100
  EM_SPARC = 2,    // SUN SPARC
  EM_386 = 3,    // Intel 80386
  EM_68K = 4,    // Motorola m68k family
  EM_88K = 5,    // Motorola m88k family
  EM_IAMCU = 6,    // Intel MCU
  EM_860 = 7,    // Intel 80860
  EM_MIPS = 8,    // MIPS R3000 big-endian
  EM_S370 = 9,    // IBM System/370
  EM_MIPS_RS3_LE = 10,    // MIPS R3000 little-endian
  // reserved 11-14
  EM_PARISC = 15,    // HPPA
  // reserved 16
  EM_VPP500 = 17,    // Fujitsu VPP500
  EM_SPARC32PLUS = 18,    // Sun's "v8plus"
  EM_960 = 19,    // Intel 80960
  EM_PPC = 20,    // PowerPC
  EM_PPC64 = 21,    // PowerPC 64-bit
  EM_S390 = 22,    // IBM S390
  EM_SPU = 23,    // IBM SPU/SPC
  // reserved 24-35
  EM_V800 = 36,    // NEC V800 series
  EM_FR20 = 37,    // Fujitsu FR20
  EM_RH32 = 38,    // TRW RH-32
  EM_RCE = 39,    // Motorola RCE
  EM_ARM = 40,    // ARM
  EM_FAKE_ALPHA = 41,    // Digital Alpha
  EM_SH = 42,    // Hitachi SH
  EM_SPARCV9 = 43,    // SPARC v9 64-bit
  EM_TRICORE = 44,    // Siemens Tricore
  EM_ARC = 45,    // Argonaut RISC Core
  EM_H8_300 = 46,    // Hitachi H8/300
  EM_H8_300H = 47,    // Hitachi H8/300H
  EM_H8S = 48,    // Hitachi H8S
  EM_H8_500 = 49,    // Hitachi H8/500
  EM_IA_64 = 50,    // Intel Merced
  EM_MIPS_X = 51,    // Stanford MIPS-X
  EM_COLDFIRE = 52,    // Motorola Coldfire
  EM_68HC12 = 53,    // Motorola M68HC12
  EM_MMA = 54,    // Fujitsu MMA Multimedia Accelerator
  EM_PCP = 55,    // Siemens PCP
  EM_NCPU = 56,    // Sony nCPU embeeded RISC
  EM_NDR1 = 57,    // Denso NDR1 microprocessor
  EM_STARCORE = 58,    // Motorola Start*Core processor
  EM_ME16 = 59,    // Toyota ME16 processor
  EM_ST100 = 60,    // STMicroelectronic ST100 processor
  EM_TINYJ = 61,    // Advanced Logic Corp. Tinyj emb.fam
  EM_X86_64 = 62,    // AMD x86-64 architecture
  EM_PDSP = 63,    // Sony DSP Processor
  EM_PDP10 = 64,    // Digital PDP-10
  EM_PDP11 = 65,    // Digital PDP-11
  EM_FX66 = 66,    // Siemens FX66 microcontroller
  EM_ST9PLUS = 67,    // STMicroelectronics ST9+ 8/16 mc
  EM_ST7 = 68,    // STmicroelectronics ST7 8 bit mc
  EM_68HC16 = 69,    // Motorola MC68HC16 microcontroller
  EM_68HC11 = 70,    // Motorola MC68HC11 microcontroller
  EM_68HC08 = 71,    // Motorola MC68HC08 microcontroller
  EM_68HC05 = 72,    // Motorola MC68HC05 microcontroller
  EM_SVX = 73,    // Silicon Graphics SVx
  EM_ST19 = 74,    // STMicroelectronics ST19 8 bit mc
  EM_VAX = 75,    // Digital VAX
  EM_CRIS = 76,    // Axis Communications 32-bit emb.proc
  EM_JAVELIN = 77,    // Infineon Technologies 32-bit emb.proc
  EM_FIREPATH = 78,    // Element 14 64-bit DSP Processor
  EM_ZSP = 79,    // LSI Logic 16-bit DSP Processor
  EM_MMIX = 80,    // Donald Knuth's educational 64-bit proc
  EM_HUANY = 81,    // Harvard University machine-independent object files
  EM_PRISM = 82,    // SiTera Prism
  EM_AVR = 83,    // Atmel AVR 8-bit microcontroller
  EM_FR30 = 84,    // Fujitsu FR30
  EM_D10V = 85,    // Mitsubishi D10V
  EM_D30V = 86,    // Mitsubishi D30V
  EM_V850 = 87,    // NEC v850
  EM_M32R = 88,    // Mitsubishi M32R
  EM_MN10300 = 89,    // Matsushita MN10300
  EM_MN10200 = 90,    // Matsushita MN10200
  EM_PJ = 91,    // picoJava
  EM_OPENRISC = 92,    // OpenRISC 32-bit embedded processor
  EM_ARC_COMPACT = 93,    // ARC International ARCompact
  EM_XTENSA = 94,    // Tensilica Xtensa Architecture
  EM_VIDEOCORE = 95,    // Alphamosaic VideoCore
  EM_TMM_GPP = 96,    // Thompson Multimedia General Purpose Proc
  EM_NS32K = 97,    // National Semi. 32000
  EM_TPC = 98,    // Tenor Network TPC
  EM_SNP1K = 99,    // Trebia SNP 1000
  EM_ST200 = 100,    // STMicroelectronics ST200
  EM_IP2K = 101,    // Ubicom IP2xxx
  EM_MAX = 102,    // MAX processor
  EM_CR = 103,    // National Semi. CompactRISC
  EM_F2MC16 = 104,    // Fujitsu F2MC16
  EM_MSP430 = 105,    // Texas Instruments msp430
  EM_BLACKFIN = 106,    // Analog Devices Blackfin DSP
  EM_SE_C33 = 107,    // Seiko Epson S1C33 family
  EM_SEP = 108,    // Sharp embedded microprocessor
  EM_ARCA = 109,    // Arca RISC
  EM_UNICORE = 110,    // PKU-Unity & MPRC Peking Uni. mc series
  EM_EXCESS = 111,    // eXcess configurable cpu
  EM_DXP = 112,    // Icera Semi. Deep Execution Processor
  EM_ALTERA_NIOS2 = 113,    // Altera Nios II
  EM_CRX = 114,    // National Semi. CompactRISC CRX
  EM_XGATE = 115,    // Motorola XGATE
  EM_C166 = 116,    // Infineon C16x/XC16x
  EM_M16C = 117,    // Renesas M16C
  EM_DSPIC30F = 118,    // Microchip Technology dsPIC30F
  EM_CE = 119,    // Freescale Communication Engine RISC
  EM_M32C = 120,    // Renesas M32C
  // reserved 121-130
  EM_TSK3000 = 131,    // Altium TSK3000
  EM_RS08 = 132,    // Freescale RS08
  EM_SHARC = 133,    // Analog Devices SHARC family
  EM_ECOG2 = 134,    // Cyan Technology eCOG2
  EM_SCORE7 = 135,    // Sunplus S+core7 RISC
  EM_DSP24 = 136,    // New Japan Radio (NJR) 24-bit DSP
  EM_VIDEOCORE3 = 137,    // Broadcom VideoCore III
  EM_LATTICEMICO32 = 138,    // RISC for Lattice FPGA
  EM_SE_C17 = 139,    // Seiko Epson C17
  EM_TI_C6000 = 140,    // Texas Instruments TMS320C6000 DSP
  EM_TI_C2000 = 141,    // Texas Instruments TMS320C2000 DSP
  EM_TI_C5500 = 142,    // Texas Instruments TMS320C55x DSP
  EM_TI_ARP32 = 143,    // Texas Instruments App. Specific RISC
  EM_TI_PRU = 144,    // Texas Instruments Prog. Realtime Unit
  // reserved 145-159
  EM_MMDSP_PLUS = 160,    // STMicroelectronics 64bit VLIW DSP
  EM_CYPRESS_M8C = 161,    // Cypress M8C
  EM_R32C = 162,    // Renesas R32C
  EM_TRIMEDIA = 163,    // NXP Semi. TriMedia
  EM_QDSP6 = 164,    // QUALCOMM DSP6
  EM_8051 = 165,    // Intel 8051 and variants
  EM_STXP7X = 166,    // STMicroelectronics STxP7x
  EM_NDS32 = 167,    // Andes Tech. compact code emb. RISC
  EM_ECOG1X = 168,    // Cyan Technology eCOG1X
  EM_MAXQ30 = 169,    // Dallas Semi. MAXQ30 mc
  EM_XIMO16 = 170,    // New Japan Radio (NJR) 16-bit DSP
  EM_MANIK = 171,    // M2000 Reconfigurable RISC
  EM_CRAYNV2 = 172,    // Cray NV2 vector architecture
  EM_RX = 173,    // Renesas RX
  EM_METAG = 174,    // Imagination Tech. META
  EM_MCST_ELBRUS = 175,    // MCST Elbrus
  EM_ECOG16 = 176,    // Cyan Technology eCOG16
  EM_CR16 = 177,    // National Semi. CompactRISC CR16
  EM_ETPU = 178,    // Freescale Extended Time Processing Unit
  EM_SLE9X = 179,    // Infineon Tech. SLE9X
  EM_L10M = 180,    // Intel L10M
  EM_K10M = 181,    // Intel K10M
  // reserved 182
  EM_AARCH64 = 183,    // ARM AARCH64
  // reserved 184
  EM_AVR32 = 185,    // Amtel 32-bit microprocessor
  EM_STM8 = 186,    // STMicroelectronics STM8
  EM_TILE64 = 187,    // Tileta TILE64
  EM_TILEPRO = 188,    // Tilera TILEPro
  EM_MICROBLAZE = 189,    // Xilinx MicroBlaze
  EM_CUDA = 190,    // NVIDIA CUDA
  EM_TILEGX = 191,    // Tilera TILE-Gx
  EM_CLOUDSHIELD = 192,    // CloudShield
  EM_COREA_1ST = 193,    // KIPO-KAIST Core-A 1st gen.
  EM_COREA_2ND = 194,    // KIPO-KAIST Core-A 2nd gen.
  EM_ARC_COMPACT2 = 195,    // Synopsys ARCompact V2
  EM_OPEN8 = 196,    // Open8 RISC
  EM_RL78 = 197,    // Renesas RL78
  EM_VIDEOCORE5 = 198,    // Broadcom VideoCore V
  EM_78KOR = 199,    // Renesas 78KOR
  EM_56800EX = 200,    // Freescale 56800EX DSC
  EM_BA1 = 201,    // Beyond BA1
  EM_BA2 = 202,    // Beyond BA2
  EM_XCORE = 203,    // XMOS xCORE
  EM_MCHP_PIC = 204,    // Microchip 8-bit PIC(r)
  // reserved 205-209
  EM_KM32 = 210,    // KM211 KM32
  EM_KMX32 = 211,    // KM211 KMX32
  EM_EMX16 = 212,    // KM211 KMX16
  EM_EMX8 = 213,    // KM211 KMX8
  EM_KVARC = 214,    // KM211 KVARC
  EM_CDP = 215,    // Paneve CDP
  EM_COGE = 216,    // Cognitive Smart Memory Processor
  EM_COOL = 217,    // Bluechip CoolEngine
  EM_NORC = 218,    // Nanoradio Optimized RISC
  EM_CSR_KALIMBA = 219,    // CSR Kalimba
  EM_Z80 = 220,    // Zilog Z80
  EM_VISIUM = 221,    // Controls and Data Services VISIUMcore
  EM_FT32 = 222,    // FTDI Chip FT32
  EM_MOXIE = 223,    // Moxie processor
  EM_AMDGPU = 224,    // AMD GPU
  // reserved 225-242
  EM_RISCV = 243,    // RISC-V

  EM_BPF = 247,        // Linux BPF -- in-kernel virtual machine
  EM_CSKY = 252,     // C-SKY
  EM_NUM = 253,


  EV_NONE = 0,                // Invalid ELF version
  EV_CURRENT = 1,                // Current version
  EV_NUM = 2,
};
//------------------------------------------------------------------------------------------------------------
// AUX vector types
enum EAFlags
{
// Legal values for a_type (entry type).
 AT_NULL              = 0,    // End of vector
 AT_IGNORE            = 1,    // Entry should be ignored
 AT_EXECFD            = 2,    // File descriptor of program
 AT_PHDR              = 3,    // Program headers for program     // &phdr[0]            // <<<<<
 AT_PHENT             = 4,    // Size of program header entry    // sizeof(phdr[0])
 AT_PHNUM             = 5,    // Number of program headers       // # phdr entries
 AT_PAGESZ            = 6,    // System page size                // getpagesize(2)      // <<<<<
 AT_BASE              = 7,    // Base address of interpreter     // ld.so base addr     // <<<<<
 AT_FLAGS             = 8,    // Flags                           // processor flags
 AT_ENTRY             = 9,    // Entry point of program          // a.out entry point   // <<<<<
 AT_NOTELF            = 10,   // Program is not ELF
 AT_UID               = 11,   // Real uid
 AT_EUID              = 12,   // Effective uid
 AT_GID               = 13,   // Real gid
 AT_EGID              = 14,   // Effective gid
 AT_CLKTCK            = 17,   // Frequency of times()
// Some more special a_type values describing the hardware.
 AT_PLATFORM          = 15,   // String identifying platform.
 AT_HWCAP             = 16,   // Machine-dependent hints about processor capabilities.
// This entry gives some information about the FPU initialization performed by the kernel.
 AT_FPUCW             = 18,   // Used FPU control word.
// Cache block sizes.
 AT_DCACHEBSIZE       = 19,   // Data cache block size.
 AT_ICACHEBSIZE       = 20,   // Instruction cache block size.
 AT_UCACHEBSIZE       = 21,   // Unified cache block size.
// A special ignored value for PPC, used by the kernel to control the interpretation of the AUXV. Must be > 16.
 AT_IGNOREPPC         = 22,   // Entry should be ignored.
 AT_SECURE            = 23,   // Boolean, was exec setuid-like?
 AT_BASE_PLATFORM     = 24,   // String identifying real platforms.
 AT_RANDOM            = 25,   // Address of 16 random bytes.                // <<<<<
 AT_HWCAP2            = 26,   // More machine-dependent hints about processor capabilities.
 AT_EXECFN            = 31,   // Filename of executable.                      // <<<<<
// Pointer to the global system page used for system calls and other nice things.
 AT_SYSINFO           = 32,
 AT_SYSINFO_EHDR      = 33,   // The base address of the vDSO           // <<<<<
// Shapes of the caches.  Bits 0-3 contains associativity; bits 4-7 contains log2 of line size; mask those to get cache size.
 AT_L1I_CACHESHAPE    = 34,
 AT_L1D_CACHESHAPE    = 35,
 AT_L2_CACHESHAPE     = 36,
 AT_L3_CACHESHAPE     = 37,
// Shapes of the caches, with more room to describe them. *GEOMETRY are comprised of cache line size in bytes in the bottom 16 bits and the cache associativity in the next 16 bits.
 AT_L1I_CACHESIZE     = 40,
 AT_L1I_CACHEGEOMETRY = 41,
 AT_L1D_CACHESIZE     = 42,
 AT_L1D_CACHEGEOMETRY = 43,
 AT_L2_CACHESIZE      = 44,
 AT_L2_CACHEGEOMETRY  = 45,
 AT_L3_CACHESIZE      = 46,
 AT_L3_CACHEGEOMETRY  = 47,
 AT_MINSIGSTKSZ       = 51,   // Stack needed for signal delivery (AArch64).
};
//------------------------------------------------------------------------------------------------------------
// Legal values for d_tag (dynamic entry type).
enum EDTags
{
 DT_NULL            = 0,            // Marks end of dynamic section
 DT_NEEDED          = 1,            // Name of needed library
 DT_PLTRELSZ        = 2,            // Size in bytes of PLT relocs
 DT_PLTGOT          = 3,            // Processor defined value
 DT_HASH            = 4,            // Address of symbol hash table
 DT_STRTAB          = 5,            // Address of string table
 DT_SYMTAB          = 6,            // Address of symbol table
 DT_RELA            = 7,            // Address of Rela relocs
 DT_RELASZ          = 8,            // Total size of Rela relocs
 DT_RELAENT         = 9,            // Size of one Rela reloc
 DT_STRSZ           = 10,           // Size of string table
 DT_SYMENT          = 11,           // Size of one symbol table entry
 DT_INIT            = 12,           // Address of init function
 DT_FINI            = 13,           // Address of termination function
 DT_SONAME          = 14,           // Name of shared object
 DT_RPATH           = 15,           // Library search path (deprecated)
 DT_SYMBOLIC        = 16,           // Start symbol search here
 DT_REL             = 17,           // Address of Rel relocs
 DT_RELSZ           = 18,           // Total size of Rel relocs
 DT_RELENT          = 19,           // Size of one Rel reloc
 DT_PLTREL          = 20,           // Type of reloc in PLT
 DT_DEBUG           = 21,           // For debugging, unspecified
 DT_TEXTREL         = 22,           // Reloc might modify .text
 DT_JMPREL          = 23,           // Address of PLT relocs
 DT_BIND_NOW        = 24,           // Process relocations of object
 DT_INIT_ARRAY      = 25,           // Array with addresses of init fct
 DT_FINI_ARRAY      = 26,           // Array with addresses of fini fct
 DT_INIT_ARRAYSZ    = 27,           // Size in bytes of DT_INIT_ARRAY
 DT_FINI_ARRAYSZ    = 28,           // Size in bytes of DT_FINI_ARRAY
 DT_RUNPATH         = 29,           // Library search path
 DT_FLAGS           = 30,           // Flags for the object being loaded
 DT_ENCODING        = 32,           // Start of encoded range
 DT_PREINIT_ARRAY   = 32,           // Array with addresses of preinit fct
 DT_PREINIT_ARRAYSZ = 33,           // size in bytes of DT_PREINIT_ARRAY
 DT_NUM             = 34,           // Number used
 DT_LOOS            = 0x6000000d,   // Start of OS-specific
 DT_HIOS            = 0x6ffff000,   // End of OS-specific
 DT_LOPROC          = 0x70000000,   // Start of processor-specific
 DT_HIPROC          = 0x7fffffff,   // End of processor-specific
//  DT_PROCNUM  DT_MIPS_NUM // Most used by any processor
};
//------------------------------------------------------------------------------------------------------------
#pragma pack( push, 1 )     // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Check alignment! <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
template<typename TAOFFS=SIZE_T> struct SElf_Hdr
{
// uint8   ident[EI_NIDENT];       // Magic number and other info
 uint32  magic;                  // 7F 45 4C 46  //.ELF
 uint8   arch;                   // class: 1=x32, 2=x64
 uint8   bord;                   // 01 — LSB (Least Significant Bit); 02 — MSB (Most Significant Bit, big-endian)
 uint8   hvers;                  // Always 1
 uint8   abiver;                 // Always 0 (SystemV)?
 uint8   abitype;                //
 uint8   reserved[7];

 uint16  type;                   // Object file type    // ET_*
 uint16  machine;                // Architecture
 uint32  version;                // Object file version    // Does not matter. Should be same as hvers
 TAOFFS  entry;                  // Entry point offset     //  ???virtual address  (Offset from base?)
 TAOFFS  phoff;                  // Program header table file offset
 TAOFFS  shoff;                  // Section header table file offset
 uint32  flags;                  // Processor-specific flags
 uint16  ehsize;                 // ELF header size in bytes  (This structure)
 uint16  phentsize;              // Program header table entry size
 uint16  phnum;                  // Program header table entry count
 uint16  shentsize;              // Section header table entry size
 uint16  shnum;                  // Section header table entry count
 uint16  shstrndx;               // Section header string table index

//----------------------------------------------------
template<typename T> T ReadVal(const T& vptr)     // TODO: byte order check
{
 return vptr;
}
//------------------------------------------------------------------------------------------------------------
template<typename T> void WriteVal(T& vptr, T val)   // TODO: byte order check
{
 vptr = val;
}
//----------------------------------------------------
};
using Elf_Hdr   = SElf_Hdr<SIZE_T>;
using Elf32_Hdr = SElf_Hdr<uint32>;
using Elf64_Hdr = SElf_Hdr<uint64>;

//------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------

struct SAuxVecRec
{
  SIZE_T type;                // Entry type
  union
    {
      SIZE_T val;                // Integer value
      PVOID  ptr;                // Pointer value
    };
};
//------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------
// This info is needed when parsing the symbol table
//#define STB_LOCAL  0
//#define STB_GLOBAL 1
//#define STB_WEAK   2
//
//#define STT_NOTYPE  0
//#define STT_OBJECT  1
//#define STT_FUNC    2
//#define STT_SECTION 3
//#define STT_FILE    4
//#define STT_COMMON  5
//#define STT_TLS     6
//
//#define ELF_ST_BIND(x)		((x) >> 4)
//#define ELF_ST_TYPE(x)		((x) & 0xf)
//#define ELF32_ST_BIND(x)	ELF_ST_BIND(x)
//#define ELF32_ST_TYPE(x)	ELF_ST_TYPE(x)
//#define ELF64_ST_BIND(x)	ELF_ST_BIND(x)
//#define ELF64_ST_TYPE(x)	ELF_ST_TYPE(x)

struct Elf_Dyn
{
 SSIZE_T tag;     //  entry tag value
 union {
    SSIZE_T val;
    SIZE_T  ptr;
  } un;
};
//------------------------------------------------------------------------------------------------------------
// The following are used with relocations
//#define ELF32_R_SYM(x) ((x) >> 8)
//#define ELF32_R_TYPE(x) ((x) & 0xff)

//#define ELF64_R_SYM(i)			((i) >> 32)
//#define ELF64_R_TYPE(i)			((i) & 0xffffffff)

struct Elf_Rel
{
 SIZE_T offset;
 SIZE_T info;
};

struct Elf_Rela
{
 SIZE_T  offset;  // Location at which to apply the action
 SIZE_T  info;    // index and type of relocation
 SSIZE_T addend;  // Constant addend used to compute value
};

struct Elf32_Sym
{
 uint32 name;
 uint32 value;
 uint32 size;
 uint8  info;
 uint8  other;
 uint16 shndx;
};

struct Elf64_Sym
{
 uint32 name;    // Symbol name, index in string tbl
 uint8  info;	 // Type and binding attributes
 uint8  other;	 // No defined meaning, 0
 uint16 shndx;   // Associated section index
 uint64 value;   // Value of the symbol
 uint64 size;    // Associated symbol size
};
//------------------------------------------------------------------------------------------------------------
struct Elf32_Phdr
{
 uint32 type;
 uint32 offset;
 uint32 vaddr;
 uint32 paddr;
 uint32 filesz;
 uint32 memsz;
 uint32 flags;
 uint32 align;
};

struct Elf64_Phdr
{
 uint32 type;
 uint32 flags;
 uint64 offset;		// Segment file offset
 uint64 vaddr;		// Segment virtual address
 uint64 paddr;		// Segment physical address
 uint64 filesz;		// Segment size in file
 uint64 memsz;		// Segment size in memory
 uint64 align;		// Segment alignment, file & memory
};
//------------------------------------------------------------------------------------------------------------
struct Elf_Shdr
{
 uint32 name;		// Section name, index in string tbl
 uint32 type;		// Type of section
 SIZE_T flags;		// Miscellaneous section attributes
 SIZE_T addr;		// Section virtual addr at execution
 SIZE_T offset;		// Section file offset
 SIZE_T size;		// Size of section in bytes
 uint32 link;		// Index of another section
 uint32 info;		// Additional section information
 SIZE_T addralign;	// Section alignment
 SIZE_T entsize;	// Entry size if section holds table
};
//------------------------------------------------------------------------------------------------------------
// Note header in a PT_NOTE section
struct Elf32_Nhdr
{
 uint32	namesz;   // Name size
 uint32	descsz;   // Content size
 uint32	type;     // Content type
};
//------------------------------------------------------------------------------------------------------------
#pragma pack( pop )
//------------------------------------------------------------------------------------------------------------
static bool IsValidHeaderELF(vptr Base)
{
 Elf_Hdr* hdr = (Elf_Hdr*)Base;
 if(hdr->magic != SIGN_ELF)return false;
 if(!hdr->arch || (hdr->arch > ELFCLASS64))return false;
 if(!hdr->bord || (hdr->bord > ELFDATA2MSB))return false;
 if(hdr->hvers != 1)return false;
 if(hdr->abiver != 0)return false;   // ??? Always 0?
 if(*(uint64*)&hdr->abitype)return false;    // ??? Always 0?
 if(hdr->ReadVal(hdr->version) != hdr->hvers)return false;    // TODO: ByteOrderRead
 return true;
}
//------------------------------------------------------------------------------------------------------------
static bool IsValidHeaderELF(vptr Base, uint Size)
{
 if(!IsValidHeaderELF(Base))return false;
 Elf_Hdr* hdr = (Elf_Hdr*)Base;
 if(Size < hdr->ReadVal(hdr->ehsize))return false;
 if(Size < (hdr->ReadVal(hdr->phoff) + (hdr->ReadVal(hdr->phnum) * hdr->ReadVal(hdr->phentsize))))return false;
 if(Size < (hdr->ReadVal(hdr->shoff) + (hdr->ReadVal(hdr->shnum) * hdr->ReadVal(hdr->shentsize))))return false;
 // TODO: validate all sections
 return true;
}
//------------------------------------------------------------------------------------------------------------
static bool IsModuleX64(vptr Base)  // NOTE: The header must be already validated
{
 Elf_Hdr* hdr = (Elf_Hdr*)Base;
 return hdr->arch == ELFCLASS64;   // 2
}
//------------------------------------------------------------------------------------------------------------
//static bool IsRvaInSection(SSecHdr* Sec, uint Rva)
//------------------------------------------------------------------------------------------------------------
//static uint GetSections(void* Base, SSecHdr** FirstSec)
//------------------------------------------------------------------------------------------------------------
//static uint RvaToFileOffset(void* Base, uint Rva)
//------------------------------------------------------------------------------------------------------------
//static uint FileOffsetToRva(void* Base, uint Offset)
//------------------------------------------------------------------------------------------------------------
//static uint SizeOfSections(void* Base, uint MaxSecs=(uint)-1, bool RawSize=false)
//------------------------------------------------------------------------------------------------------------
//static bool GetSectionForAddress(void* Base, void* Address, SSecHdr** ResSec)
//------------------------------------------------------------------------------------------------------------
//static bool GetModuleSection(void* Base, char *SecName, SSecHdr** ResSec)
//------------------------------------------------------------------------------------------------------------
// https://github.com/kushaldas/elfutils/blob/master/libdwfl/elf-from-memory.c
// NOTE: Target ELF in memory may be of different arch
// NOTE: Assumed thar PHeaders in memory accessible at the same offset as in the file
// NOTE: Sections will most likely be at the end of file and they are not define memory mapping anyway
//
static uint GetModuleSizeInMem(vptr Base)
{
 if(!IsValidHeaderELF(Base))return 0;
 size_t MaxOffs = 0;   // Will include BSS
 size_t MinOffs = -1;
 if(((Elf_Hdr*)Base)->arch == ELFCLASS64)
  {
   Elf64_Hdr* hdr = (Elf64_Hdr*)Base;
   Elf64_Phdr* phdrs = (Elf64_Phdr*)((uint8*)Base + hdr->ReadVal(hdr->phoff));
   for(uint16 idx=0,tot=hdr->ReadVal(hdr->phnum);idx < tot;idx++)  // Or add phentsize???
    {
//     DBGDBG("Seg: %p",&phdrs[idx]);
     uint32 type  = hdr->ReadVal(phdrs[idx].type);
     if(type != PT_LOAD)continue;
     size_t boffs = hdr->ReadVal(phdrs[idx].vaddr);           // Alignment?
     size_t eoffs = boffs + hdr->ReadVal(phdrs[idx].memsz);   // Alignment?
     if(boffs < MinOffs)MinOffs = boffs;
     if(eoffs > MaxOffs)MaxOffs = eoffs;
    }
  }
  else
   {
    Elf32_Hdr* hdr = (Elf32_Hdr*)Base;
    Elf32_Phdr* phdrs = (Elf32_Phdr*)((uint8*)Base + hdr->ReadVal(hdr->phoff));
    for(uint16 idx=0,tot=hdr->ReadVal(hdr->phnum);idx < tot;idx++)  // Or add phentsize???
     {
//      DBGDBG("Seg: %p",&phdrs[idx]);
      uint32 type  = hdr->ReadVal(phdrs[idx].type);
      if(type != PT_LOAD)continue;
      size_t boffs = hdr->ReadVal(phdrs[idx].vaddr);           // Alignment?
      size_t eoffs = boffs + hdr->ReadVal(phdrs[idx].memsz);   // Alignment?
      if(boffs < MinOffs)MinOffs = boffs;
      if(eoffs > MaxOffs)MaxOffs = eoffs;
     }
   }
 MaxOffs = AlignP2Frwd(MaxOffs, MEMPAGESIZE);
 MinOffs = AlignP2Bkwd(MinOffs, MEMPAGESIZE);
 if(MinOffs){MaxOffs -= MinOffs; MinOffs = 0;}   // Usually default base is 0 but not have to be
 return AlignP2Frwd(MaxOffs - MinOffs, MEMPAGESIZE);
}
//------------------------------------------------------------------------------------------------------------
// static void* GetLoadedModuleEntryPoint(void* Base)
//------------------------------------------------------------------------------------------------------------
// static uint GetModuleEntryOffset(void* Base, bool Raw)
//------------------------------------------------------------------------------------------------------------
// static void* ModuleAddressToBase(void* Addr)   // NOTE: Will find a PE header or crash trying:)    // Not all module sections may be present in memory (discarded)
//------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------

};

using ELF = NFMTELF<uint>;
//============================================================================================================
