

//============================================================================================================
// https://github.com/bytedance/android-inline-hook
//
class SHookImpl
{
//============================================================================================================
// https://developer.arm.com/documentation/ddi0487/latest
// https://developer.arm.com/documentation/ddi0602/latest
//
struct SA64RW
{
//------------------------------------------------------------------------------------------------------------
 uintptr_t start_addr;
 uintptr_t end_addr;
 uint32 *buf;
 size_t buf_offset;
 size_t inst_lens[4];
 size_t inst_lens_cnt;

//------------------------------------------------------------------------------------------------------------
enum sh_a64_type_t
{
  IGNORED = 0,
  B,
  B_COND,
  BL,
  ADR,
  ADRP,
  LDR_LIT_32,
  LDR_LIT_64,
  LDRSW_LIT,
  PRFM_LIT,
  LDR_SIMD_LIT_32,
  LDR_SIMD_LIT_64,
  LDR_SIMD_LIT_128,
  CBZ,
  CBNZ,
  TBZ,
  TBNZ
};
//------------------------------------------------------------------------------------------------------------
static size_t sh_a64_get_rewrite_inst_len(uint32 inst)
{
  static uint8 map[] = {
      4,   // IGNORED
      20,  // B
      28,  // B_COND
      20,  // BL
      16,  // ADR
      16,  // ADRP
      20,  // LDR_LIT_32
      20,  // LDR_LIT_64
      20,  // LDRSW_LIT
      28,  // PRFM_LIT
      28,  // LDR_SIMD_LIT_32
      28,  // LDR_SIMD_LIT_64
      28,  // LDR_SIMD_LIT_128
      24,  // CBZ
      24,  // CBNZ
      24,  // TBZ
      24   // TBNZ
  };

  return (size_t)(map[sh_a64_get_type(inst)]);
}
//------------------------------------------------------------------------------------------------------------
static sh_a64_type_t sh_a64_get_type(uint32 inst)
{
       if ((inst & 0xFC000000u) == 0x14000000u)return B;
  else if ((inst & 0xFF000010u) == 0x54000000u)return B_COND;
  else if ((inst & 0xFC000000u) == 0x94000000u)return BL;
  else if ((inst & 0x9F000000u) == 0x10000000u)return ADR;
  else if ((inst & 0x9F000000u) == 0x90000000u)return ADRP;
  else if ((inst & 0xFF000000u) == 0x18000000u)return LDR_LIT_32;
  else if ((inst & 0xFF000000u) == 0x58000000u)return LDR_LIT_64;
  else if ((inst & 0xFF000000u) == 0x98000000u)return LDRSW_LIT;
  else if ((inst & 0xFF000000u) == 0xD8000000u)return PRFM_LIT;
  else if ((inst & 0xFF000000u) == 0x1C000000u)return LDR_SIMD_LIT_32;
  else if ((inst & 0xFF000000u) == 0x5C000000u)return LDR_SIMD_LIT_64;
  else if ((inst & 0xFF000000u) == 0x9C000000u)return LDR_SIMD_LIT_128;
  else if ((inst & 0x7F000000u) == 0x34000000u)return CBZ;
  else if ((inst & 0x7F000000u) == 0x35000000u)return CBNZ;
  else if ((inst & 0x7F000000u) == 0x36000000u)return TBZ;
  else if ((inst & 0x7F000000u) == 0x37000000u)return TBNZ;
  else return IGNORED;
}
//------------------------------------------------------------------------------------------------------------
bool sh_a64_is_addr_need_fix(uintptr_t addr)
{
 return (this->start_addr <= addr && addr < this->end_addr);
}
//------------------------------------------------------------------------------------------------------------
uintptr_t sh_a64_fix_addr(uintptr_t addr)
{
 if(this->start_addr <= addr && addr < this->end_addr)
  {
   uintptr_t cursor_addr = this->start_addr;
   size_t offset = 0;
   for(size_t i = 0; i < this->inst_lens_cnt; i++)
    {
     if (cursor_addr >= addr) break;
     cursor_addr += 4;
     offset += this->inst_lens[i];
    }
   uintptr_t fixed_addr = (uintptr_t)this->buf + offset;
 //  LOGINF("a64 rewrite: fix addr %" PRIxPTR " -> %" PRIxPTR, addr, fixed_addr);
   return fixed_addr;
  }

  return addr;
}
//------------------------------------------------------------------------------------------------------------
 size_t sh_a64_rewrite_b(uint32 *buf, uint32 inst, uintptr_t pc, sh_a64_type_t type)
{
 uint64 imm64;
 if(type == B_COND)
  {
   uint64 imm19 = SH_UTIL_GET_BITS_32(inst, 23, 5);
   imm64 = SH_UTIL_SIGN_EXTEND_64(imm19 << 2u, 21u);
  }
  else
   {
    uint64 imm26 = SH_UTIL_GET_BITS_32(inst, 25, 0);
    imm64 = SH_UTIL_SIGN_EXTEND_64(imm26 << 2u, 28u);
   }
  uint64 addr = pc + imm64;
  addr = this->sh_a64_fix_addr(addr);

 size_t idx = 0;
 if (type == B_COND)
  {
   buf[idx++] = (inst & 0xFF00001F) | 0x40u;  // B.<cond> #8
   buf[idx++] = 0x14000006;                   // B #24
  }
  buf[idx++] = 0x58000051;  // LDR X17, #8
  buf[idx++] = 0x14000003;  // B #12
  buf[idx++] = addr & 0xFFFFFFFF;
  buf[idx++] = addr >> 32u;
  if (type == BL)buf[idx++] = 0xD63F0220;  // BLR X17
   else buf[idx++] = 0xD61F0220;  // BR X17
  return idx * 4;             // 20 or 28
}
//------------------------------------------------------------------------------------------------------------
size_t sh_a64_rewrite_adr(uint32 *buf, uint32 inst, uintptr_t pc, sh_a64_type_t type)
{
  uint32 xd    = SH_UTIL_GET_BITS_32(inst, 4,   0);
  uint64 immlo = SH_UTIL_GET_BITS_32(inst, 30, 29);
  uint64 immhi = SH_UTIL_GET_BITS_32(inst, 23,  5);
  uint64 addr;
  if (type == ADR)addr = pc + SH_UTIL_SIGN_EXTEND_64((immhi << 2u) | immlo, 21u);
    else addr = (pc & 0xFFFFFFFFFFFFF000) + SH_UTIL_SIGN_EXTEND_64((immhi << 14u) | (immlo << 12u), 33u);  // ADRP
  if (this->sh_a64_is_addr_need_fix(addr)) return 0;  // rewrite failed

  buf[0] = 0x58000040u | xd;  // LDR Xd, #8
  buf[1] = 0x14000003;        // B #12
  buf[2] = addr & 0xFFFFFFFF;
  buf[3] = addr >> 32u;
  return 16;
}
//------------------------------------------------------------------------------------------------------------
size_t sh_a64_rewrite_ldr(uint32 *buf, uint32 inst, uintptr_t pc, sh_a64_type_t type)
{
 uint32 rt     = SH_UTIL_GET_BITS_32(inst, 4, 0);
 uint64 imm19  = SH_UTIL_GET_BITS_32(inst, 23, 5);
 uint64 offset = SH_UTIL_SIGN_EXTEND_64((imm19 << 2u), 21u);
 uint64 addr   = pc + offset;

 if (this->sh_a64_is_addr_need_fix(addr))
  {
   if (type != PRFM_LIT) return 0;  // rewrite failed
   addr = this->sh_a64_fix_addr(addr);
  }

 if (type == LDR_LIT_32 || type == LDR_LIT_64 || type == LDRSW_LIT)
  {
    buf[0] = 0x58000060u | rt;  // LDR Xt, #12
    if (type == LDR_LIT_32)buf[1] = 0xB9400000 | rt | (rt << 5u);  // LDR Wt, [Xt]
    else if (type == LDR_LIT_64)buf[1] = 0xF9400000 | rt | (rt << 5u);  // LDR Xt, [Xt]
    else buf[1] = 0xB9800000 | rt | (rt << 5u);  // LDRSW Xt, [Xt]   // LDRSW_LIT
    buf[2] = 0x14000003;          // B #12
    buf[3] = addr & 0xFFFFFFFF;
    buf[4] = addr >> 32u;
    return 20;
  }
   else
    {
     buf[0] = 0xA93F47F0;  // STP X16, X17, [SP, -0x10]
     buf[1] = 0x58000091;  // LDR X17, #16
     if (type == PRFM_LIT)buf[2] = 0xF9800220 | rt;  // PRFM Rt, [X17]
     else if (type == LDR_SIMD_LIT_32)buf[2] = 0xBD400220 | rt;  // LDR St, [X17]
     else if (type == LDR_SIMD_LIT_64)buf[2] = 0xFD400220 | rt;  // LDR Dt, [X17]
     else buf[2] = 0x3DC00220u | rt;  // LDR Qt, [X17]      // LDR_SIMD_LIT_128
     buf[3] = 0xF85F83F1;          // LDR X17, [SP, -0x8]
     buf[4] = 0x14000003;          // B #12
     buf[5] = addr & 0xFFFFFFFF;
     buf[6] = addr >> 32u;
     return 28;
    }
}
//------------------------------------------------------------------------------------------------------------
size_t sh_a64_rewrite_cb(uint32 *buf, uint32 inst, uintptr_t pc)
{
 uint64 imm19  = SH_UTIL_GET_BITS_32(inst, 23, 5);
 uint64 offset = SH_UTIL_SIGN_EXTEND_64((imm19 << 2u), 21u);
 uint64 addr   = pc + offset;
 addr = this->sh_a64_fix_addr(addr);

 buf[0] = (inst & 0xFF00001F) | 0x40u;  // CB(N)Z Rt, #8
 buf[1] = 0x14000005;                   // B #20
 buf[2] = 0x58000051;                   // LDR X17, #8
 buf[3] = 0xd61f0220;                   // BR X17
 buf[4] = addr & 0xFFFFFFFF;
 buf[5] = addr >> 32u;
 return 24;
}
//------------------------------------------------------------------------------------------------------------
size_t sh_a64_rewrite_tb(uint32 *buf, uint32 inst, uintptr_t pc)
{
 uint64 imm14  = SH_UTIL_GET_BITS_32(inst, 18, 5);
 uint64 offset = SH_UTIL_SIGN_EXTEND_64((imm14 << 2u), 16u);
 uint64 addr   = pc + offset;
 addr = this->sh_a64_fix_addr(addr);

 buf[0] = (inst & 0xFFF8001F) | 0x40u;  // TB(N)Z Rt, #<imm>, #8
 buf[1] = 0x14000005;                   // B #20
 buf[2] = 0x58000051;                   // LDR X17, #8
 buf[3] = 0xd61f0220;                   // BR X17
 buf[4] = addr & 0xFFFFFFFF;
 buf[5] = addr >> 32u;
 return 24;
}
//------------------------------------------------------------------------------------------------------------
size_t sh_a64_rewrite(uint32 *buf, uint32 inst, uintptr_t pc)
{
 sh_a64_type_t type = sh_a64_get_type(inst);
//  LOGINF("a64 rewrite: type %d, inst %08X", type, inst);
 if (type == B || type == B_COND || type == BL)return this->sh_a64_rewrite_b(buf, inst, pc, type);
 else if (type == ADR || type == ADRP)return this->sh_a64_rewrite_adr(buf, inst, pc, type);
 else if (type == LDR_LIT_32 || type == LDR_LIT_64 || type == LDRSW_LIT || type == PRFM_LIT || type == LDR_SIMD_LIT_32 || type == LDR_SIMD_LIT_64 || type == LDR_SIMD_LIT_128)return this->sh_a64_rewrite_ldr(buf, inst, pc, type);
 else if (type == CBZ || type == CBNZ)return this->sh_a64_rewrite_cb(buf, inst, pc);
 else if (type == TBZ || type == TBNZ)return this->sh_a64_rewrite_tb(buf, inst, pc);

 buf[0] = inst;    // IGNORED
 return 4;
}
//------------------------------------------------------------------------------------------------------------
static size_t sh_a64_absolute_jump_with_br(uint32 *buf, uintptr_t addr)
{
  buf[0] = 0x58000051;  // LDR X17, #8
  buf[1] = 0xd61f0220;  // BR X17
  buf[2] = addr & 0xFFFFFFFF;
  buf[3] = addr >> 32u;
  return 16;
}
//------------------------------------------------------------------------------------------------------------
// Use RET instead of BR to bypass arm64 BTI.
//
// ref:
// https://developer.arm.com/documentation/102433/0100/Jump-oriented-programming
// https://developer.arm.com/documentation/ddi0602/2023-06/Base-Instructions/BTI--Branch-Target-Identification-?lang=en
// https://github.com/torvalds/linux/commit/8ef8f360cf30be12382f89ff48a57fbbd9b31c14
// https://android-review.googlesource.com/c/platform/bionic/+/1242754
// https://developer.android.com/ndk/guides/abis#armv9_enabling_pac_and_bti_for_cc
// https://developer.arm.com/documentation/100067/0612/armclang-Command-line-Options/-mbranch-protection
//
static size_t sh_a64_absolute_jump_with_ret(uint32 *buf, uintptr_t addr)
{
  buf[0] = 0x58000051;  // LDR X17, #8
  buf[1] = 0xd65f0220;  // RET X17
  buf[2] = addr & 0xFFFFFFFF;
  buf[3] = addr >> 32u;
  return 16;
}
//------------------------------------------------------------------------------------------------------------
static size_t sh_a64_relative_jump(uint32 *buf, uintptr_t addr, uintptr_t pc)
{
  buf[0] = 0x14000000u | (((addr - pc) & 0x0FFFFFFFu) >> 2u);  // B <label>
  return 4;
}
//------------------------------------------------------------------------------------------------------------
};
//============================================================================================================
// Impl:
//------------------------------------------------------------------------------------------------------------
// EntryStub:    // You call it if you want to call an original proc
//   OrigInstructions  // Those that has been overwritten by jump to a HookProc  // Possibly recompiled to fix relative addressing
//   JumpToRestOfTheProcInstr  // If any instructions left after hooking
//
int BuildEntryStub(uintptr_t target_addr)  // Builds the entry stub from original instructions and preserves them as a backup
{
  // Backup original instructions (length: 4 or 16) to be able to restore them later
  memcpy((void *)(this->backup), (void *)target_addr, this->backup_len);   // TODO: Do one by one. Error if end of function is detected

  // Package the information passed to rewrite
  SA64RW rinfo;
  rinfo.start_addr    = target_addr;
  rinfo.end_addr      = target_addr + this->backup_len;
  rinfo.buf           = (uint32 *)this->enter_addr;
  rinfo.buf_offset    = 0;
  rinfo.inst_lens_cnt = this->backup_len / 4;
  for (uint32 i = 0; i < this->backup_len; i += 4)rinfo.inst_lens[i / 4] = SA64RW::sh_a64_get_rewrite_inst_len(*((uint32 *)(target_addr + i)));

  // Rewrite original instructions(if necessary) for the entry stub. Recompiles branches and relative addressing
  uintptr_t pc = target_addr;
  for (uint32 i = 0; i < this->backup_len; i += 4, pc += 4)
   {
    size_t offset = rinfo.sh_a64_rewrite((uint32 *)((uintptr_t)this->enter_addr + rinfo.buf_offset), *((uint32 *)(target_addr + i)), pc);
    if (0 == offset) return -1;
    rinfo.buf_offset += offset;
   }

  // absolute jump back to remaining original instructions (fill in enter)
  rinfo.buf_offset += SA64RW::sh_a64_absolute_jump_with_ret((uint32 *)((uintptr_t)this->enter_addr + rinfo.buf_offset), target_addr + this->backup_len);
  sh_util_clear_cache(this->enter_addr, rinfo.buf_offset);
  return 0;
}
//------------------------------------------------------------------------------------------------------------
// B: [-128M, +128M - 4]
//#define SH_INST_A64_B_RANGE_LOW  (134217728)
//#define SH_INST_A64_B_RANGE_HIGH (134217724)

// Short hook using relative jump (only 4 bytes to overwrite). Needs an absolute jump stub nearby if a hook proc is too far
// NOTE: We should have a gap map for an dll or a page allocated nearby if the hook proc is outside of reljmp range
//
/*int sh_inst_hook_rel(uintptr_t target_addr, uintptr_t new_addr)
{
  uintptr_t pc = target_addr;
  this->backup_len = 4;   // Short distance relative jump size

 // if (dlinfo->dli_ssize < this->backup_len) return SHADOWHOOK_ERRNO_HOOK_SYMSZ;

  // alloc an exit for absolute jump
  SA64RW::sh_a64_absolute_jump_with_br(this->exit, new_addr);    // This stub will redirect to an actual hook proc
  if (0 != (r = sh_exit_alloc(&this->exit_addr, (uint16_t *)&this->exit_type, pc, dlinfo, (uint8_t *)(this->exit), sizeof(this->exit), SH_INST_A64_B_RANGE_LOW, SH_INST_A64_B_RANGE_HIGH)))return r;

  // rewrite
  if (0 != sh_util_mprotect(target_addr, this->backup_len, PROT_READ | PROT_WRITE | PROT_EXEC)) {
    r = SHADOWHOOK_ERRNO_MPROT;
    goto err;
  }

  int  r = this->BuildEntryStub(self, target_addr);
  if (0 != r) goto err;

  // relative jump to the exit by overwriting the head of original function
  sh_a64_relative_jump(this->trampo, this->exit_addr, pc);
  __atomic_thread_fence(__ATOMIC_SEQ_CST);
  if (0 != (r = sh_util_write_inst(target_addr, this->trampo, this->backup_len))) goto err;   // Set the hook

 // SH_LOG_INFO("a64: hook (WITH EXIT) OK. target %" PRIxPTR " -> exit %" PRIxPTR " -> new %" PRIxPTR " -> enter %" PRIxPTR " -> remaining %" PRIxPTR, target_addr, this->exit_addr, new_addr, this->enter_addr, target_addr + this->backup_len);
  return 0;

err:
  sh_exit_free(this->exit_addr, (uint16_t)this->exit_type, (uint8_t *)(this->exit), sizeof(this->exit));
  this->exit_addr = 0;  // this is a flag for with-exit or without-exit
  return r;
} */
//------------------------------------------------------------------------------------------------------------
int sh_inst_hook_abs(uintptr_t target_addr, uintptr_t new_addr)
{
  this->backup_len = 16;

//  if (dlinfo->dli_ssize < this->backup_len) return SHADOWHOOK_ERRNO_HOOK_SYMSZ;

  // rewrite
  if (0 != NPTM::MemProtect((vptr)target_addr, this->backup_len, PX::PROT_READ | PX::PROT_WRITE | PX::PROT_EXEC))return -1;
  if (int r = this->BuildEntryStub(target_addr);r < 0) return r;

  // absolute jump to new function address by overwriting the head of original function
  SA64RW::sh_a64_absolute_jump_with_br(this->trampo, new_addr);
  __atomic_thread_fence(__ATOMIC_SEQ_CST);
  if(int r = sh_util_write_inst(target_addr, this->trampo, this->backup_len);r < 0) return r;   // Set the hook

//  LOGINF("a64: hook (WITHOUT EXIT) OK. target %p -> new %p -> enter %p -> remaining %p", target_addr, new_addr, this->enter_addr, target_addr + this->backup_len);
  return 0;
}
//------------------------------------------------------------------------------------------------------------
public:

int sh_inst_hook(vptr target_addr, vptr new_addr)
{
//  this->enter_addr = sh_enter_alloc();
  if (0 == this->enter_addr) return -1;   // Should be provided

  int r;
#ifdef SH_CONFIG_TRY_WITH_EXIT
  if (0 == (r = sh_inst_hook_rel(target_addr, new_addr))) return r;
#endif
  if (0 == (r = sh_inst_hook_abs((uintptr_t)target_addr, (uintptr_t)new_addr))) return r;

  // hook failed
 // sh_enter_free(this->enter_addr);  // TODO: !!!
  return r;
}
//------------------------------------------------------------------------------------------------------------
int sh_inst_unhook(vptr target_addr)
{
  int r = memcmp((void *)target_addr, this->trampo, this->backup_len);    // TODO: Try-catch or validate the addr somehow
  if (0 != r) return -1;
  if (0 != (r = sh_util_write_inst((uintptr_t)target_addr, this->backup, this->backup_len))) return r;     // restore the instructions at the target address
  __atomic_thread_fence(__ATOMIC_SEQ_CST);

  // free memory space for exit     // TODO: !!!
//  if (0 != this->exit_addr)  // Fill it with what was there before
//    if (0 != (r = sh_exit_free(this->exit_addr, (uint16)this->exit_type, (uint8 *)(this->exit), sizeof(this->exit))))
//      return r;

  // free memory space for enter
//  sh_enter_free(this->enter_addr);   // TODO: !!!

  LOGINF("a64: unhook OK. target %p", (vptr)target_addr);
  return 0;
}
//------------------------------------------------------------------------------------------------------------

  uint32  exit[4];     // Far stub, actually  // Stored here to check if it is hooked?
  uint32  trampo[4];   // align 16 // length == backup_len    // Used on unhook to check that we still own this hook
  uint32  backup[4];   // align 16     // uint8_t   backup[16];
  uint32  backup_len;  // == 4 or 16
  uint32  exit_type;   // Stores prev fill byte of the gap and its allocation type (TODO)
  vptr    exit_addr;   // Probably points to some hole nearby or into allocated page  (Must be in an Executable memory, nearby to target proc start addr)
  vptr    enter_addr;  // Points to a stub with instructions taken(and possibly recompiled) from a target proc start addr (Must be in an Executable memory)
//------------------------------------------------------------------------------------------------------------
};
