
//============================================================================================================
namespace NUHK
{
#define SH_UTIL_IS_THUMB(addr)   ((addr)&1u)
#define SH_UTIL_CLEAR_BIT0(addr) ((addr)&0xFFFFFFFE)
#define SH_UTIL_SET_BIT0(addr)   ((addr) | 1u)

#define SH_UTIL_ALIGN_4(pc) ((pc)&0xFFFFFFFC)
#define SH_UTIL_SIGN_EXTEND_32(n, len) ((SH_UTIL_GET_BIT_32(n, len - 1) > 0) ? ((n) | (0xFFFFFFFF << (len))) : n)
#define SH_UTIL_SIGN_EXTEND_64(n, len) ((SH_UTIL_GET_BIT_64(n, len - 1) > 0) ? ((n) | (0xFFFFFFFFFFFFFFFF << (len))) : n)

#define SH_UTIL_GET_BIT_16(n, idx)        ((uint16)((n) << (15u - (idx))) >> 15u)
#define SH_UTIL_GET_BITS_16(n, high, low) ((uint16)((n) << (15u - (high))) >> (15u - (high) + (low)))
#define SH_UTIL_GET_BIT_32(n, idx)        ((uint32)((n) << (31u - (idx))) >> 31u)
#define SH_UTIL_GET_BITS_32(n, high, low) ((uint32)((n) << (31u - (high))) >> (31u - (high) + (low)))
#define SH_UTIL_GET_BIT_64(n, idx)        ((uint64)((n) << (63u - (idx))) >> 63u)
#define SH_UTIL_GET_BITS_64(n, high, low) ((uint64)((n) << (63u - (high))) >> (63u - (high) + (low)))


static void sh_util_clear_cache(vptr addr, size_t len)
{
  __builtin___clear_cache((char *)addr, (char *)((uintptr_t)addr + len));  // LIBC::__aarch64_sync_cache_range()
}

static bool sh_util_is_thumb32(uintptr_t target_addr)
{
  uint16 opcode = *((uint16 *)target_addr);
  int tmp = opcode >> 11u;
  return (tmp == 0x1d) || (tmp == 0x1e) || (tmp == 0x1f);
}

static uint32 sh_util_ror(uint32 val, uint32 n, uint32 shift)
{
  uint32 m = shift % n;
  return (val >> m) | (val << (n - m));
}

static uint32 sh_util_arm_expand_imm(uint32 opcode)
{
  uint32 imm = SH_UTIL_GET_BITS_32(opcode, 7, 0);
  uint32 amt = 2 * SH_UTIL_GET_BITS_32(opcode, 11, 8);

  return amt == 0 ? imm : sh_util_ror(imm, 32, amt);
}

static int sh_util_write_inst(uintptr_t target_addr, void *inst, size_t inst_len)
{
  if (0 != NPTM::MemProtect((vptr)target_addr, inst_len, PX::PROT_READ | PX::PROT_WRITE | PX::PROT_EXEC))return -1;
    if ((4 == inst_len) && (0 == target_addr % 4))__atomic_store_n((uint32 *)target_addr, *((uint32 *)inst), __ATOMIC_SEQ_CST);
    else if ((8 == inst_len) && (0 == target_addr % 8))__atomic_store_n((uint64 *)target_addr, *((uint64 *)inst), __ATOMIC_SEQ_CST);
#ifdef __LP64__
    else if ((16 == inst_len) && (0 == target_addr % 16))__atomic_store_n((__int128 *)target_addr, *((__int128 *)inst), __ATOMIC_SEQ_CST);
#endif
    else memcpy((void *)target_addr, inst, inst_len);

    sh_util_clear_cache((vptr)target_addr, inst_len);

  return 0;  // OK
}
//------------------------------------------------------------------------------------------------------------
template<uint32 Flg, uint64 LHash, uint64 PHash, vptr HAddr, int ArgNum> class CUniHook    // Base hook class, not type aware
{
protected:
#ifdef CPU_ARM
#  ifdef ARCH_X32
#  include "HookArm32.hpp"
#  else
#  include "HookArm64.hpp"
#  endif
#elif CPU_X86
#error "Unimplemented!"
#else
#error "Unsupported!"
#endif

 SHookImpl impl;
// May be set by a template definition, forcing the hook into RW section
 uint32 Flags;         // Initial flags
// uint16 LibPathSize;
// uint16 ProcNameSize;
// uint32 LibPathHash;
// uint32 ProcNameHash;
 vptr   HProcAddr;     // Hook proc addr that may be assigned at compile time
 vptr   TProcAddr;     // Original proc addr that will redirect to HookProc
 alignas(4) uint8 Stub[64];  // SH_ENTER_SZ        256

//------------------------------------------------------------------------------------------------------------
int InstallInternal(void)   // Uses HProcAddr and TProcAddr
{
 impl.enter_addr = &this->Stub;
 DBGDBG("Before: %08X", *(uint32*)this->TProcAddr);
 int res = impl.sh_inst_hook(this->TProcAddr, this->HProcAddr);
 DBGDBG("Addr=%p, res=%i: %08X",this->TProcAddr,res, *(uint32*)this->TProcAddr);
 return 0;
}
//------------------------------------------------------------------------------------------------------------

public:
//------------------------------------------------------------------------------------------------------------
CUniHook(void): Flags(Flg), HProcAddr(HAddr) {}
//------------------------------------------------------------------------------------------------------------
static constexpr uint64 HashName(const achar* str)
{
 return 0;    // TODO: Put str len and first,last bytes in high half + CRC32
}
//------------------------------------------------------------------------------------------------------------
// TODO: Near allocation of tramopline stubs (static)
int Set(vptr TgtProcAddr, vptr HookProcAddr=HAddr, uint Flags=Flg)
{
 this->Flags     = Flags;
 this->TProcAddr = TgtProcAddr;
 this->HProcAddr = HookProcAddr;
 return this->InstallInternal();
}
//------------------------------------------------------------------------------------------------------------
int Install(vptr LibBase, const achar* ProcName, vptr HookProcAddr=HAddr, uint Flags=Flg)
{
 return 0;
}
//------------------------------------------------------------------------------------------------------------
int Install(vptr LibBase, uint64 ProcHash=PHash, vptr HookProcAddr=HAddr, uint Flags=Flg)
{
 return 0;
}
//------------------------------------------------------------------------------------------------------------
// Note: Several instances of same library may be present
int Install(const achar* LibPath, const achar* ProcName, vptr HookProcAddr=HAddr, uint Flags=Flg)  // Optionally may load the DLL
{
 return 0;
}
//------------------------------------------------------------------------------------------------------------
int Install(const achar* ProcName, uint64 LibHash=LHash, vptr HookProcAddr=HAddr, uint Flags=Flg)
{
 return 0;
}
//------------------------------------------------------------------------------------------------------------
int Install(uint64 LibHash=LHash, uint64 ProcHash=PHash, vptr HookProcAddr=HAddr, uint Flags=Flg)
{
 return 0;
}
//------------------------------------------------------------------------------------------------------------
int Remove(bool Any=true)
{
 return 0;
}
//------------------------------------------------------------------------------------------------------------
};
//============================================================================================================
#define PHOOK(Func) NFWK::NUHK::SProcHook<0,(uint64)0,(uint64)0,(vptr)&Func,decltype(Func)>
//#define PHOOKI(lh,ph,ha,Func) SC_STUB_DEF NSYSC::SProcHook<(uint64)lh,decltype(Func)>

template<uint32, uint64, uint64, vptr, class> struct SProcHook;
template<uint32 Flg, uint64 LibHash, uint64 ProcHash, vptr HAddr, class TRet, class... TPar> struct SProcHook<Flg, LibHash, ProcHash, HAddr, TRet(TPar...)>: public CUniHook<Flg, LibHash, ProcHash, HAddr, sizeof...(TPar)>
{
 using TFuncPtr  = TRet (*)(TPar...);
 SCVR int ArgNum = sizeof...(TPar);

//-------------------------
_finline TRet operator()(TPar... params) const {return ((const TFuncPtr)this->impl.enter_addr)(params...);}
//-------------------------
};
//------------------------------------------------------------------------------------------------------------
template<uint32, uint64, uint64, vptr, class> struct SProcHookVA;
template<uint32 Flg, uint64 LibHash, uint64 ProcHash, vptr HAddr, class TRet, class... TPar> struct SProcHookVA<Flg, LibHash, ProcHash, HAddr, TRet(TPar...)>: public CUniHook<Flg, LibHash, ProcHash, HAddr, sizeof...(TPar)>

{
 using TFuncPtr  = TRet (*)(TPar..., ...);
 SCVR int ArgNum = sizeof...(TPar);

//-------------------------
template<typename... VA> _finline TRet operator()(TPar... params, VA... vp) const noexcept {return ((const TFuncPtr)this->impl.enter_addr)(params..., vp...);}
//-------------------------
};
//------------------------------------------------------------------------------------------------------------

//============================================================================================================
// May be static or allocated nearby of a target DLL, or several DLLs
template<typename T> struct SHookBlk  // Should be in BSS(RW) if statically allocated
{
 uint32 OldProt;
// TODO: Array of module gaps
// TODO: Array of stub pages
//------------------------------------------------------------------------------------------------------------
static consteval uint64 HashName(const achar* str)
{
 return 0;//CUniHook::HashName(str);
}
//------------------------------------------------------------------------------------------------------------
bool Unprotect(bool NoExec=false)  // Required if the block is statically allocated in a ReadOnly section. NOTE: Not every system allows Writable and Executable memory type
{
 return NPTM::MemProtect(this, sizeof(T), PX::PROT_READ | PX::PROT_WRITE | (NoExec?0:PX::PROT_EXEC)) == 0;   // TODO: No exec
}
//------------------------------------------------------------------------------------------------------------
bool Protect(bool Writable=true)   // Exec+Read
{
 //return MemProtect(this, sizeof(T), PX::PROT_READ | PX::PROT_WRITE | PX::PROT_EXEC) == 0;
 return true;
}
//------------------------------------------------------------------------------------------------------------
int Install(uint32 FlgOverride=0)  // Goes through all hooks and tries to resolve them
{
 return 0;
}
//------------------------------------------------------------------------------------------------------------
int Remove(bool Force=false)
{
 return 0;
}
//------------------------------------------------------------------------------------------------------------
int Destroy(void)   // Frees any arrays and deallocates memory
{
 return 0;
}
//------------------------------------------------------------------------------------------------------------
};

};   // NUHK

/* Example
struct SMyLibHooks: NUHK::SHookBlk<SMyLibHooks>    // NOTE: May be placed in a separate section to avoid changing memory protection of other data
{
 ... hook declarations
} static MyLibHooks;

*/
//============================================================================================================
