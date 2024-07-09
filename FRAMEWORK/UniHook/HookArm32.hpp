
//============================================================================================================
// https://github.com/bytedance/android-inline-hook
//
class SHookImpl
{
//============================================================================================================
// https://developer.arm.com/documentation/ddi0406/latest
// https://developer.arm.com/documentation/ddi0597/latest
//
// ARM32
//
struct SA32RW
{
//------------------------------------------------------------------------------------------------------------
//struct sh_a32_rewrite_info_t
//{
  uintptr_t overwrite_start_addr;
  uintptr_t overwrite_end_addr;
  uint32 *rewrite_buf;
  size_t rewrite_buf_offset;
  size_t rewrite_inst_lens[2];
  size_t rewrite_inst_lens_cnt;
//};
//------------------------------------------------------------------------------------------------------------
enum sh_a32_type_t
{
  IGNORED = 0,
  B_A1,
  BX_A1,
  BL_IMM_A1,
  BLX_IMM_A2,
  ADD_REG_A1,
  ADD_REG_PC_A1,
  SUB_REG_A1,
  SUB_REG_PC_A1,
  ADR_A1,
  ADR_A2,
  MOV_REG_A1,
  MOV_REG_PC_A1,
  LDR_LIT_A1,
  LDR_LIT_PC_A1,
  LDRB_LIT_A1,
  LDRD_LIT_A1,
  LDRH_LIT_A1,
  LDRSB_LIT_A1,
  LDRSH_LIT_A1,
  LDR_REG_A1,
  LDR_REG_PC_A1,
  LDRB_REG_A1,
  LDRD_REG_A1,
  LDRH_REG_A1,
  LDRSB_REG_A1,
  LDRSH_REG_A1
};
//------------------------------------------------------------------------------------------------------------
size_t sh_a32_get_rewrite_inst_len(uint32 inst)
{
  static uint8 map[] = {
      4,   // IGNORED
      12,  // B_A1
      12,  // BX_A1
      16,  // BL_IMM_A1
      16,  // BLX_IMM_A2
      32,  // ADD_REG_A1
      32,  // ADD_REG_PC_A1
      32,  // SUB_REG_A1
      32,  // SUB_REG_PC_A1
      12,  // ADR_A1
      12,  // ADR_A2
      32,  // MOV_REG_A1
      12,  // MOV_REG_PC_A1
      24,  // LDR_LIT_A1
      36,  // LDR_LIT_PC_A1
      24,  // LDRB_LIT_A1
      24,  // LDRD_LIT_A1
      24,  // LDRH_LIT_A1
      24,  // LDRSB_LIT_A1
      24,  // LDRSH_LIT_A1
      32,  // LDR_REG_A1
      36,  // LDR_REG_PC_A1
      32,  // LDRB_REG_A1
      32,  // LDRD_REG_A1
      32,  // LDRH_REG_A1
      32,  // LDRSB_REG_A1
      32   // LDRSH_REG_A1
  };

  return (size_t)(map[sh_a32_get_type(inst)]);
}
//------------------------------------------------------------------------------------------------------------
static sh_a32_type_t sh_a32_get_type(uint32 inst)
{
       if (((inst & 0x0F000000u) == 0x0A000000u) && ((inst & 0xF0000000u) != 0xF0000000u))return B_A1;
  else if (((inst & 0x0FFFFFFFu) == 0x012FFF1Fu) && ((inst & 0xF0000000u) != 0xF0000000u))return BX_A1;
  else if (((inst & 0x0F000000u) == 0x0B000000u) && ((inst & 0xF0000000u) != 0xF0000000u))return BL_IMM_A1;
  else if ((inst  & 0xFE000000u) == 0xFA000000u)return BLX_IMM_A2;
  else if (((inst & 0x0FE00010u) == 0x00800000u) && ((inst & 0xF0000000u) != 0xF0000000u) && ((inst & 0x0010F000u) != 0x0010F000u) && ((inst & 0x000F0000u) != 0x000D0000u) && (((inst & 0x000F0000u) == 0x000F0000u) || ((inst & 0x0000000Fu) == 0x0000000Fu)))return ((inst & 0x0000F000u) == 0x0000F000u) ? ADD_REG_PC_A1 : ADD_REG_A1;
  else if (((inst & 0x0FE00010u) == 0x00400000u) && ((inst & 0xF0000000u) != 0xF0000000u) && ((inst & 0x0010F000u) != 0x0010F000u) && ((inst & 0x000F0000u) != 0x000D0000u) && (((inst & 0x000F0000u) == 0x000F0000u) || ((inst & 0x0000000Fu) == 0x0000000Fu)))return ((inst & 0x0000F000u) == 0x0000F000u) ? SUB_REG_PC_A1 : SUB_REG_A1;
  else if (((inst & 0x0FFF0000u) == 0x028F0000u) && ((inst & 0xF0000000u) != 0xF0000000u))return ADR_A1;
  else if (((inst & 0x0FFF0000u) == 0x024F0000u) && ((inst & 0xF0000000u) != 0xF0000000u))return ADR_A2;
  else if (((inst & 0x0FEF001Fu) == 0x01A0000Fu) && ((inst & 0xF0000000u) != 0xF0000000u) && ((inst & 0x0010F000u) != 0x0010F000u) && (!(((inst & 0x0000F000u) == 0x0000F000u) && ((inst & 0x00000FF0u) != 0x00000000u))))return ((inst & 0x0000F000u) == 0x0000F000u) ? MOV_REG_PC_A1 : MOV_REG_A1;
  else if (((inst & 0x0F7F0000u) == 0x051F0000u) && ((inst & 0xF0000000u) != 0xF0000000u))return ((inst & 0x0000F000u) == 0x0000F000u) ? LDR_LIT_PC_A1 : LDR_LIT_A1;
  else if (((inst & 0x0F7F0000u) == 0x055F0000u) && ((inst & 0xF0000000u) != 0xF0000000u))return LDRB_LIT_A1;
  else if (((inst & 0x0F7F00F0u) == 0x014F00D0u) && ((inst & 0xF0000000u) != 0xF0000000u))return LDRD_LIT_A1;
  else if (((inst & 0x0F7F00F0u) == 0x015F00B0u) && ((inst & 0xF0000000u) != 0xF0000000u))return LDRH_LIT_A1;
  else if (((inst & 0x0F7F00F0u) == 0x015F00D0u) && ((inst & 0xF0000000u) != 0xF0000000u))return LDRSB_LIT_A1;
  else if (((inst & 0x0F7F00F0u) == 0x015F00F0u) && ((inst & 0xF0000000u) != 0xF0000000u))return LDRSH_LIT_A1;
  else if (((inst & 0x0E5F0010u) == 0x061F0000u) && ((inst & 0xF0000000u) != 0xF0000000u) && ((inst & 0x01200000u) != 0x00200000u))return ((inst & 0x0000F000u) == 0x0000F000u) ? LDR_REG_PC_A1 : LDR_REG_A1;
  else if (((inst & 0x0E5F0010u) == 0x065F0000u) && ((inst & 0xF0000000u) != 0xF0000000u) && ((inst & 0x01200000u) != 0x00200000u))return LDRB_REG_A1;
  else if (((inst & 0x0E5F0FF0u) == 0x000F00D0u) && ((inst & 0xF0000000u) != 0xF0000000u) && ((inst & 0x01200000u) != 0x00200000u))return LDRD_REG_A1;
  else if (((inst & 0x0E5F0FF0u) == 0x001F00B0u) && ((inst & 0xF0000000u) != 0xF0000000u) && ((inst & 0x01200000u) != 0x00200000u))return LDRH_REG_A1;
  else if (((inst & 0x0E5F0FF0u) == 0x001F00D0u) && ((inst & 0xF0000000u) != 0xF0000000u) && ((inst & 0x01200000u) != 0x00200000u))return LDRSB_REG_A1;
  else if (((inst & 0x0E5F0FF0u) == 0x001F00F0u) && ((inst & 0xF0000000u) != 0xF0000000u) && ((inst & 0x01200000u) != 0x00200000u))return LDRSH_REG_A1;
  else return IGNORED;
}
//------------------------------------------------------------------------------------------------------------
bool sh_a32_is_addr_need_fix(uintptr_t addr)
{
 return (this->overwrite_start_addr <= addr && addr < this->overwrite_end_addr);
}
//------------------------------------------------------------------------------------------------------------
uintptr_t sh_a32_fix_addr(uintptr_t addr)
{
 if (this->overwrite_start_addr <= addr && addr < this->overwrite_end_addr)
  {
   uintptr_t cursor_addr = this->overwrite_start_addr;
   size_t offset = 0;
   for (size_t i = 0; i < this->rewrite_inst_lens_cnt; i++)
    {
      if (cursor_addr >= addr) break;
      cursor_addr += 4;
      offset += this->rewrite_inst_lens[i];
    }
   uintptr_t fixed_addr = (uintptr_t)this->rewrite_buf + offset;
//   SH_LOG_INFO("a32 rewrite: fix addr %" PRIxPTR " -> %" PRIxPTR, addr, fixed_addr);
   return fixed_addr;
  }
 return addr;
}
//------------------------------------------------------------------------------------------------------------
size_t sh_a32_rewrite_b(uint32 *buf, uint32 inst, uintptr_t pc, sh_a32_type_t type)
{
 uint32 cond;
 if (type == B_A1 || type == BL_IMM_A1 || type == BX_A1)cond = SH_UTIL_GET_BITS_32(inst, 31, 28);
  else cond = 0xE;  // 1110 None (AL)  // type == BLX_IMM_A2

 uint32 addr;
 if (type == B_A1 || type == BL_IMM_A1)
  {
    uint32 imm24 = SH_UTIL_GET_BITS_32(inst, 23, 0);
    uint32 imm32 = SH_UTIL_SIGN_EXTEND_32(imm24 << 2u, 26u);
    addr = pc + imm32;  // arm -> arm
  }
  else if (type == BLX_IMM_A2)
   {
    uint32 h = SH_UTIL_GET_BIT_32(inst, 24);
    uint32 imm24 = SH_UTIL_GET_BITS_32(inst, 23, 0);
    uint32 imm32 = SH_UTIL_SIGN_EXTEND_32((imm24 << 2u) | (h << 1u), 26u);
    addr = SH_UTIL_SET_BIT0(pc + imm32);  // arm -> thumb
   }
  else
   {
    // type == BX_A1
    // BX PC
    // PC must be even, and the "arm" instruction must be at a 4-byte aligned address,
    // so the instruction set must keep "arm" unchanged.
    addr = pc;  // arm -> arm
   }
  addr = this->sh_a32_fix_addr(addr);

  size_t idx = 0;
  if (type == BL_IMM_A1 || type == BLX_IMM_A2)buf[idx++] = 0x028FE008u | (cond << 28u);  // ADD<c> LR, PC, #8
  buf[idx++] = 0x059FF000u | (cond << 28u);  // LDR<c> PC, [PC, #0]
  buf[idx++] = 0xEA000000;                   // B #0
  buf[idx++] = addr;
  return idx * 4;  // 12 or 16
}
//------------------------------------------------------------------------------------------------------------
static size_t sh_a32_rewrite_add_or_sub(uint32 *buf, uint32 inst, uintptr_t pc)
{
 // ADD{S}<c> <Rd>, <Rn>, PC{, <shift>}  or  ADD{S}<c> <Rd>, PC, <Rm>{, <shift>}
 // SUB{S}<c> <Rd>, <Rn>, PC{, <shift>}  or  SUB{S}<c> <Rd>, PC, <Rm>{, <shift>}
 uint32 cond = SH_UTIL_GET_BITS_32(inst, 31, 28);
 uint32 rn   = SH_UTIL_GET_BITS_32(inst, 19, 16);
 uint32 rm   = SH_UTIL_GET_BITS_32(inst, 3, 0);
 uint32 rd   = SH_UTIL_GET_BITS_32(inst, 15, 12);

 uint32 rx;  // r0 - r3
 for (rx = 3;; --rx)
   if (rx != rn && rx != rm && rx != rd) break;

 if (rd == 0xF)  // Rd == PC
  {
   uint32 ry;  // r0 - r4
   for (ry = 4;; --ry)
     if (ry != rn && ry != rm && ry != rd && ry != rx) break;

   buf[0] = 0x0A000000u | (cond << 28u);           // B<c> #0
   buf[1] = 0xEA000005;                            // B #20
   buf[2] = 0xE92D8000 | (1u << rx) | (1u << ry);  // PUSH {Rx, Ry, PC}
   buf[3] = 0xE59F0008 | (rx << 12u);              // LDR Rx, [PC, #8]
   if (rn == 0xF)buf[4] = (inst & 0x0FF00FFFu) | 0xE0000000 | (ry << 12u) | (rx << 16u);  // ADD/SUB Ry, Rx, Rm{, <shift>}     // Rn == PC
     else buf[4] = (inst & 0x0FFF0FF0u) | 0xE0000000 | (ry << 12u) | rx;    // ADD/SUB Ry, Rn, Rx{, <shift>}    // Rm == PC
   buf[5] = 0xE58D0008 | (ry << 12u);                                // STR Ry, [SP, #8]
   buf[6] = 0xE8BD8000 | (1u << rx) | (1u << ry);                    // POP {Rx, Ry, PC}
   buf[7] = pc;
   return 32;
  }
  else
   {
    buf[0] = 0x0A000000u | (cond << 28u);  // B<c> #0
    buf[1] = 0xEA000005;                   // B #20
    buf[2] = 0xE52D0004 | (rx << 12u);     // PUSH {Rx}
    buf[3] = 0xE59F0008 | (rx << 12u);     // LDR Rx, [PC, #8]
    if (rn == 0xF)buf[4] = (inst & 0x0FF0FFFFu) | 0xE0000000 | (rx << 16u);  // ADD/SUB{S} Rd, Rx, Rm{, <shift>}    // Rn == PC
      else buf[4] = (inst & 0x0FFFFFF0u) | 0xE0000000 | rx;    // ADD/SUB{S} Rd, Rn, Rx{, <shift>}    // Rm == PC
    buf[5] = 0xE49D0004 | (rx << 12u);                  // POP {Rx}
    buf[6] = 0xEA000000;                                // B #0
    buf[7] = pc;
    return 32;
   }
}
//------------------------------------------------------------------------------------------------------------
size_t sh_a32_rewrite_adr(uint32 *buf, uint32 inst, uintptr_t pc, sh_a32_type_t type)
{
  uint32 cond  = SH_UTIL_GET_BITS_32(inst, 31, 28);
  uint32 rd    = SH_UTIL_GET_BITS_32(inst, 15, 12);  // r0 - r15
  uint32 imm12 = SH_UTIL_GET_BITS_32(inst, 11, 0);
  uint32 imm32 = sh_util_arm_expand_imm(imm12);
  uint32 addr  = (type == ADR_A1 ? (SH_UTIL_ALIGN_4(pc) + imm32) : (SH_UTIL_ALIGN_4(pc) - imm32));
  if (this->sh_a32_is_addr_need_fix(addr)) return 0;  // rewrite failed

  buf[0] = 0x059F0000u | (cond << 28u) | (rd << 12u);  // LDR<c> Rd, [PC, #0]
  buf[1] = 0xEA000000;                                 // B #0
  buf[2] = addr;
  return 12;
}
//------------------------------------------------------------------------------------------------------------
static size_t sh_a32_rewrite_mov(uint32 *buf, uint32 inst, uintptr_t pc)
{
 // MOV{S}<c> <Rd>, PC
 uint32 cond = SH_UTIL_GET_BITS_32(inst, 31, 28);
 uint32 rd   = SH_UTIL_GET_BITS_32(inst, 15, 12);
 uint32 rx   = (rd == 0) ? 1 : 0;
 if(rd == 0xF)  // Rd == PC (MOV PC, PC)
  {
   buf[0] = 0x059FF000u | (cond << 28u);  // LDR<c> PC, [PC, #0]
   buf[1] = 0xEA000000;                   // B #0
   buf[2] = pc;
   return 12;
  }
  else
   {
    buf[0] = 0x0A000000u | (cond << 28u);             // B<c> #0
    buf[1] = 0xEA000005;                              // B #20
    buf[2] = 0xE52D0004 | (rx << 12u);                // PUSH {Rx}
    buf[3] = 0xE59F0008 | (rx << 12u);                // LDR Rx, [PC, #8]
    buf[4] = (inst & 0x0FFFFFF0u) | 0xE0000000 | rx;  // MOV{S} Rd, Rx{, <shift> #<amount>/RRX}
    buf[5] = 0xE49D0004 | (rx << 12u);                // POP {Rx}
    buf[6] = 0xEA000000;                              // B #0
    buf[7] = pc;
    return 32;
   }
}
//------------------------------------------------------------------------------------------------------------
size_t sh_a32_rewrite_ldr_lit(uint32 *buf, uint32 inst, uintptr_t pc, sh_a32_type_t type)
{
 uint32 cond = SH_UTIL_GET_BITS_32(inst, 31, 28);
 uint32 u    = SH_UTIL_GET_BIT_32(inst, 23);
 uint32 rt   = SH_UTIL_GET_BITS_16(inst, 15, 12);

 uint32 imm32;
 if (type == LDR_LIT_A1 || type == LDR_LIT_PC_A1 || type == LDRB_LIT_A1)imm32 = SH_UTIL_GET_BITS_32(inst, 11, 0);
   else imm32 = (SH_UTIL_GET_BITS_32(inst, 11, 8) << 4u) + SH_UTIL_GET_BITS_32(inst, 3, 0);
 uint32 addr = (u ? (SH_UTIL_ALIGN_4(pc) + imm32) : (SH_UTIL_ALIGN_4(pc) - imm32));
 if (this->sh_a32_is_addr_need_fix(addr)) return 0;  // rewrite failed

 if (type == LDR_LIT_PC_A1 && rt == 0xF)   // Rt == PC
  {
    buf[0] = 0x0A000000u | (cond << 28u);  // B<c> #0
    buf[1] = 0xEA000006;                   // B #24
    buf[2] = 0xE92D0003;                   // PUSH {R0, R1}
    buf[3] = 0xE59F0000;                   // LDR R0, [PC, #0]
    buf[4] = 0xEA000000;                   // B #0
    buf[5] = addr;                         //
    buf[6] = 0xE5900000;                   // LDR R0, [R0]
    buf[7] = 0xE58D0004;                   // STR R0, [SP, #4]
    buf[8] = 0xE8BD8001;                   // POP {R0, PC}
    return 36;
  }
  else
   {
    buf[0] = 0x0A000000u | (cond << 28u);  // B<c> #0
    buf[1] = 0xEA000003;                   // B #12
    buf[2] = 0xE59F0000 | (rt << 12u);     // LDR Rt, [PC, #0]
    buf[3] = 0xEA000000;                   // B #0
    buf[4] = addr;                         //
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch"
    switch (type)
     {
      case LDR_LIT_A1:
        buf[5] = 0xE5900000 | (rt << 16u) | (rt << 12u);  // LDR Rt, [Rt]
        break;
      case LDRB_LIT_A1:
        buf[5] = 0xE5D00000 | (rt << 16u) | (rt << 12u);  // LDRB Rt, [Rt]
        break;
      case LDRD_LIT_A1:
        buf[5] = 0xE1C000D0 | (rt << 16u) | (rt << 12u);  // LDRD Rt, [Rt]
        break;
      case LDRH_LIT_A1:
        buf[5] = 0xE1D000B0 | (rt << 16u) | (rt << 12u);  // LDRH Rt, [Rt]
        break;
      case LDRSB_LIT_A1:
        buf[5] = 0xE1D000D0 | (rt << 16u) | (rt << 12u);  // LDRSB Rt, [Rt]
        break;
      case LDRSH_LIT_A1:
        buf[5] = 0xE1D000F0 | (rt << 16u) | (rt << 12u);  // LDRSH Rt, [Rt]
        break;
     }
#pragma clang diagnostic pop
    return 24;
   }
}
//------------------------------------------------------------------------------------------------------------
static size_t sh_a32_rewrite_ldr_reg(uint32 *buf, uint32 inst, uintptr_t pc, sh_a32_type_t type)
{
 // LDR<c> <Rt>, [PC,+/-<Rm>{, <shift>}]{!}
 // ......
 uint32 cond = SH_UTIL_GET_BITS_32(inst, 31, 28);
 uint32 rt   = SH_UTIL_GET_BITS_16(inst, 15, 12);
 uint32 rt2  = rt + 1;
 uint32 rm   = SH_UTIL_GET_BITS_16(inst, 3, 0);
 uint32 rx;  // r0 - r3
 for (rx = 3;; --rx)
   if (rx != rt && rx != rt2 && rx != rm) break;

 if (type == LDR_REG_PC_A1 && rt == 0xF)
  {
   // Rt == PC
   uint32 ry;  // r0 - r4
   for (ry = 4;; --ry)
     if (ry != rt && ry != rt2 && ry != rm && ry != rx) break;
   buf[0] = 0x0A000000u | (cond << 28u);           // B<c> #0
   buf[1] = 0xEA000006;                            // B #24
   buf[2] = 0xE92D8000 | (1u << rx) | (1u << ry);  // PUSH {Rx, Ry, PC}
   buf[3] = 0xE59F0000 | (rx << 12u);              // LDR Rx, [PC, #8]
   buf[4] = 0xEA000000;                            // B #0
   buf[5] = pc;
   buf[6] = (inst & 0x0FF00FFFu) | 0xE0000000 | (rx << 16u) | (ry << 12u);  // LDRxx Ry, [Rx],+/-Rm{, <shift>}
   buf[7] = 0xE58D0008 | (ry << 12u);                                       // STR Ry, [SP, #8]
   buf[8] = 0xE8BD8000 | (1u << rx) | (1u << ry);                           // POP {Rx, Ry, PC}
   return 36;
  }
  else
   {
    buf[0] = 0x0A000000u | (cond << 28u);  // B<c> #0
    buf[1] = 0xEA000005;                   // B #20
    buf[2] = 0xE52D0004 | (rx << 12u);     // PUSH {Rx}
    buf[3] = 0xE59F0000 | (rx << 12u);     // LDR Rx, [PC, #0]
    buf[4] = 0xEA000000;                   // B #0
    buf[5] = pc;
    buf[6] = (inst & 0x0FF0FFFFu) | 0xE0000000 | (rx << 16u);  // LDRxx Rt, [Rx],+/-Rm{, <shift>}
    buf[7] = 0xE49D0004 | (rx << 12u);                         // POP {Rx}
    return 32;
   }
}
//------------------------------------------------------------------------------------------------------------
size_t sh_a32_rewrite(uint32 *buf, uint32 inst, uintptr_t pc)
{
 sh_a32_type_t type = sh_a32_get_type(inst);
// SH_LOG_INFO("a32 rewrite: type %d, inst %" PRIx32, type, inst);

 // We will only overwrite 4 to 8 bytes on A32, so PC cannot be in the coverage.
 // In this case, the add/sub/mov/ldr_reg instruction does not need to consider the problem of PC in the coverage area when rewriting.

 if (type == B_A1 || type == BX_A1 || type == BL_IMM_A1 || type == BLX_IMM_A2)return this->sh_a32_rewrite_b(buf, inst, pc, type);
 else if (type == ADD_REG_A1 || type == ADD_REG_PC_A1 || type == SUB_REG_A1 || type == SUB_REG_PC_A1)return SA32RW::sh_a32_rewrite_add_or_sub(buf, inst, pc);
 else if (type == ADR_A1 || type == ADR_A2)return this->sh_a32_rewrite_adr(buf, inst, pc, type);
 else if (type == MOV_REG_A1 || type == MOV_REG_PC_A1)return SA32RW::sh_a32_rewrite_mov(buf, inst, pc);
 else if (type == LDR_LIT_A1 || type == LDR_LIT_PC_A1 || type == LDRB_LIT_A1 || type == LDRD_LIT_A1 || type == LDRH_LIT_A1 || type == LDRSB_LIT_A1 || type == LDRSH_LIT_A1)return this->sh_a32_rewrite_ldr_lit(buf, inst, pc, type);
 else if (type == LDR_REG_A1 || type == LDR_REG_PC_A1 || type == LDRB_REG_A1 || type == LDRD_REG_A1 || type == LDRH_REG_A1 || type == LDRSB_REG_A1 || type == LDRSH_REG_A1)return SA32RW::sh_a32_rewrite_ldr_reg(buf, inst, pc, type);

 buf[0] = inst;  // IGNORED
 return 4;
}
//------------------------------------------------------------------------------------------------------------
size_t sh_a32_absolute_jump(uint32 *buf, uintptr_t addr)
{
 buf[0] = 0xE51FF004;  // LDR PC, [PC, #-4]
 buf[1] = addr;
 return 8;
}
//------------------------------------------------------------------------------------------------------------
size_t sh_a32_relative_jump(uint32 *buf, uintptr_t addr, uintptr_t pc)
{
 buf[0] = 0xEA000000 | (((addr - pc) & 0x03FFFFFFu) >> 2u);  // B <label>
 return 4;
}
//------------------------------------------------------------------------------------------------------------

};
//============================================================================================================
//
// ARM THUMB (16+32)
//
struct STXX
{
//struct sh_txx_rewrite_info_t
//{
  uintptr_t start_addr;
  uintptr_t end_addr;
  uint16 *buf;
  size_t buf_offset;
  size_t inst_lens[13];  // 26 / 2 = 13
  size_t inst_lens_cnt;
//};

//------------------------------------------------------------------------------------------------------------
bool sh_txx_is_addr_need_fix(uintptr_t addr)
{
 return (this->start_addr <= addr && addr < this->end_addr);
}
//------------------------------------------------------------------------------------------------------------
uintptr_t sh_txx_fix_addr(uintptr_t addr)
{
 bool is_thumb = SH_UTIL_IS_THUMB(addr);
 if (is_thumb) addr = SH_UTIL_CLEAR_BIT0(addr);
 if (this->start_addr <= addr && addr < this->end_addr)
  {
   uintptr_t cursor_addr = this->start_addr;
   size_t offset = 0;
   for (size_t i = 0; i < this->inst_lens_cnt; i++)
    {
      if (cursor_addr >= addr) break;
      cursor_addr += 2;
      offset += this->inst_lens[i];
    }
   uintptr_t fixed_addr = (uintptr_t)this->buf + offset;
   if (is_thumb) fixed_addr = SH_UTIL_SET_BIT0(fixed_addr);

//   SH_LOG_INFO("txx rewrite: fix addr %" PRIxPTR " -> %" PRIxPTR, addr, fixed_addr);
   return fixed_addr;
  }
 if (is_thumb) addr = SH_UTIL_SET_BIT0(addr);
 return addr;
}
//------------------------------------------------------------------------------------------------------------
//                                            ARM THUMB (16)
//------------------------------------------------------------------------------------------------------------
// https://developer.arm.com/documentation/ddi0406/latest
// https://developer.arm.com/documentation/ddi0597/latest
//
struct sh_t16_it_t
{
  uint16 insts[8];
  size_t insts_len;       // 2 - 16 (bytes)
  size_t insts_cnt;       // 1 - 4
  size_t insts_else_cnt;  // 0 - 3
  uintptr_t pcs[4];
  uint8 firstcond;
  uint8 padding[3];
};
//------------------------------------------------------------------------------------------------------------
enum sh_t16_type_t
{
  IGNORED = 0,
  IT_T1,
  B_T1,
  B_T2,
  BX_T1,
  ADD_REG_T2,
  MOV_REG_T1,
  ADR_T1,
  LDR_LIT_T1,
  CBZ_T1,
  CBNZ_T1
};
//------------------------------------------------------------------------------------------------------------
size_t sh_t16_get_rewrite_inst_len(uint16 inst)
{
  static uint8 map[] = {
      4,   // IGNORED
      0,   // IT_T1
      12,  // B_T1
      8,   // B_T2
      8,   // BX_T1
      16,  // ADD_REG_T2
      12,  // MOV_REG_T1
      8,   // ADR_T1
      12,  // LDR_LIT_T1
      12,  // CBZ_T1
      12   // CBNZ_T1
  };

  return (size_t)(map[sh_t16_get_type(inst)]);
}
//------------------------------------------------------------------------------------------------------------
static sh_t16_type_t sh_t16_get_type(uint16 inst)
{
       if (((inst & 0xFF00u) == 0xBF00u) && ((inst & 0x000Fu) != 0x0000u) && ((inst & 0x00F0u) != 0x00F0u))return IT_T1;
  else if (((inst & 0xF000u) == 0xD000u) && ((inst & 0x0F00u) != 0x0F00u) && ((inst & 0x0F00u) != 0x0E00u))return B_T1;
  else if ((inst  & 0xF800u) == 0xE000u)return B_T2;
  else if ((inst  & 0xFFF8u) == 0x4778u)return BX_T1;
  else if (((inst & 0xFF78u) == 0x4478u) && ((inst & 0x0087u) != 0x0085u))return ADD_REG_T2;
  else if ((inst  & 0xFF78u) == 0x4678u)return MOV_REG_T1;
  else if ((inst  & 0xF800u) == 0xA000u)return ADR_T1;
  else if ((inst  & 0xF800u) == 0x4800u)return LDR_LIT_T1;
  else if ((inst  & 0xFD00u) == 0xB100u)return CBZ_T1;
  else if ((inst  & 0xFD00u) == 0xB900u)return CBNZ_T1;
  else return IGNORED;
}
//------------------------------------------------------------------------------------------------------------
size_t sh_t16_rewrite_b(uint16 *buf, uint16 inst, uintptr_t pc, sh_t16_type_t type)
{
 uint32 addr;
 if (type == B_T1)
  {
   uint32 imm8 = SH_UTIL_GET_BITS_16(inst, 7, 0);
   addr = pc + SH_UTIL_SIGN_EXTEND_32(imm8 << 1u, 9u);
   addr = SH_UTIL_SET_BIT0(addr);  // thumb -> thumb
  }
 else if (type == B_T2)
  {
   uint32 imm11 = SH_UTIL_GET_BITS_16(inst, 10, 0);
   addr = pc + SH_UTIL_SIGN_EXTEND_32(imm11 << 1u, 12u);
   addr = SH_UTIL_SET_BIT0(addr);  // thumb -> thumb
  }
 else
  {
   // type == BX_T1
   // BX PC
   // PC must be even, and the "BX PC" instruction must be at a 4-byte aligned address,
   // so the instruction set must be exchanged from "thumb" to "arm".
   addr = pc;  // thumb -> arm
  }
 addr = this->sh_txx_fix_addr(addr);

 size_t idx = 0;
 if (type == B_T1)
  {
   buf[idx++] = inst & 0xFF00u;  // B<c> #0
   buf[idx++] = 0xE003;          // B PC, #6
  }
 buf[idx++] = 0xF8DF;  // LDR.W PC, [PC]
 buf[idx++] = 0xF000;  // ...
 buf[idx++] = addr & 0xFFFFu;
 buf[idx++] = addr >> 16u;
 return idx * 2;       // 8 or 12
}
//------------------------------------------------------------------------------------------------------------
static size_t sh_t16_rewrite_add(uint16 *buf, uint16 inst, uintptr_t pc)
{
 // ADD<c> <Rdn>, PC
 uint16 dn  = SH_UTIL_GET_BIT_16(inst, 7);
 uint16 rdn = SH_UTIL_GET_BITS_16(inst, 2, 0);
 uint16 rd  = (uint16)(dn << 3u) | rdn;
 uint16 rx  = (rd == 0) ? 1 : 0;  // r0 - r1

 buf[0] = (uint16)(0xB400u | (1u << rx));         // PUSH {Rx}
 buf[1] = 0x4802u | (uint16)(rx << 8u);           // LDR Rx, [PC, #8]
 buf[2] = (inst & 0xFF87u) | (uint16)(rx << 3u);  // ADD Rd, Rx
 buf[3] = (uint16)(0xBC00u | (1u << rx));         // POP {Rx}
 buf[4] = 0xE002;                                   // B #4
 buf[5] = 0xBF00;
 buf[6] = pc & 0xFFFFu;
 buf[7] = pc >> 16u;
 return 16;
}
//------------------------------------------------------------------------------------------------------------
static size_t sh_t16_rewrite_mov(uint16 *buf, uint16 inst, uintptr_t pc)
{
 // MOV<c> <Rd>, PC
 uint16 D  = SH_UTIL_GET_BIT_16(inst, 7);
 uint16 rd = SH_UTIL_GET_BITS_16(inst, 2, 0);
 uint16 d  = (uint16)(D << 3u) | rd;  // r0 - r15

 buf[0] = 0xF8DF;                     // LDR.W Rd, [PC, #4]
 buf[1] = (uint16)(d << 12u) + 4u;  // ...
 buf[2] = 0xE002;                     // B #4
 buf[3] = 0xBF00;                     // NOP
 buf[4] = pc & 0xFFFFu;
 buf[5] = pc >> 16u;
 return 12;
}
//------------------------------------------------------------------------------------------------------------
size_t sh_t16_rewrite_adr(uint16 *buf, uint16 inst, uintptr_t pc)
{
 // ADR<c> <Rd>, <label>
 uint16 rd   = SH_UTIL_GET_BITS_16(inst, 10, 8);  // r0 - r7
 uint16 imm8 = SH_UTIL_GET_BITS_16(inst, 7, 0);
 uint32 addr = SH_UTIL_ALIGN_4(pc) + (uint32)(imm8 << 2u);
 if (this->sh_txx_is_addr_need_fix(addr)) return 0;  // rewrite failed

 buf[0] = 0x4800u | (uint16)(rd << 8u);  // LDR Rd, [PC]
 buf[1] = 0xE001;                          // B #2
 buf[2] = addr & 0xFFFFu;
 buf[3] = addr >> 16u;
 return 8;
}
//------------------------------------------------------------------------------------------------------------
size_t sh_t16_rewrite_ldr(uint16 *buf, uint16 inst, uintptr_t pc)
{
 // LDR<c> <Rt>, <label>
 uint16 rt   = SH_UTIL_GET_BITS_16(inst, 10, 8);  // r0 - r7
 uint16 imm8 = SH_UTIL_GET_BITS_16(inst, 7, 0);
 uint32 addr = SH_UTIL_ALIGN_4(pc) + (uint32)(imm8 << 2u);
 if (this->sh_txx_is_addr_need_fix(addr)) return 0;  // rewrite failed

 buf[0] = 0x4800u | (uint16)(rt << 8u);    // LDR Rt, [PC]
 buf[1] = 0xE001;                          // B #2
 buf[2] = addr & 0xFFFFu;
 buf[3] = addr >> 16u;
 buf[4] = 0x6800u | (uint16)(rt << 3u) | rt;    // LDR Rt, [Rt]
 buf[5] = 0xBF00;                               // NOP
 return 12;
}
//------------------------------------------------------------------------------------------------------------
size_t sh_t16_rewrite_cb(uint16 *buf, uint16 inst, uintptr_t pc)
{
 // CB{N}Z <Rn>, <label>
 uint16 i     = SH_UTIL_GET_BIT_16(inst, 9);
 uint16 imm5  = SH_UTIL_GET_BITS_16(inst, 7, 3);
 uint32 imm32 = (uint32)(i << 6u) | (uint32)(imm5 << 1u);
 uint32 addr  = SH_UTIL_SET_BIT0(pc + imm32);  // thumb -> thumb
 addr   = this->sh_txx_fix_addr(addr);

 buf[0] = inst & 0xFD07u;  // CB(N)Z Rn, #0
 buf[1] = 0xE003;          // B PC, #6
 buf[2] = 0xF8DF;          // LDR.W PC, [PC]
 buf[3] = 0xF000;          // ...
 buf[4] = addr & 0xFFFFu;
 buf[5] = addr >> 16u;
 return 12;
}
//------------------------------------------------------------------------------------------------------------
size_t sh_t16_rewrite(uint16 *buf, uint16 inst, uintptr_t pc)
{
 sh_t16_type_t type = sh_t16_get_type(inst);
// SH_LOG_INFO("t16 rewrite: type %d, inst %" PRIx16, type, inst);

 if (type == B_T1 || type == B_T2 || type == BX_T1)return this->sh_t16_rewrite_b(buf, inst, pc, type);
 else if (type == ADD_REG_T2)return sh_t16_rewrite_add(buf, inst, pc);
 else if (type == MOV_REG_T1)return sh_t16_rewrite_mov(buf, inst, pc);
 else if (type == ADR_T1)return this->sh_t16_rewrite_adr(buf, inst, pc);
 else if (type == LDR_LIT_T1)return this->sh_t16_rewrite_ldr(buf, inst, pc);
 else if (type == CBZ_T1 || type == CBNZ_T1)return this->sh_t16_rewrite_cb(buf, inst, pc);

 buf[0] = inst;
 buf[1] = 0xBF00;  // NOP
 return 4;   // IGNORED
}
//------------------------------------------------------------------------------------------------------------
static size_t sh_t16_get_it_insts_count(uint16 inst)
{
 if ((inst & 0x1u) != 0) return 4;
 if ((inst & 0x2u) != 0) return 3;
 if ((inst & 0x4u) != 0) return 2;
 return 1;
}
//------------------------------------------------------------------------------------------------------------
static bool sh_t16_parse_it(sh_t16_it_t *it, uint16 inst, uintptr_t pc)
{
 if (IT_T1 != sh_t16_get_type(inst)) return false;
// SH_LOG_INFO("t16 rewrite: type IT, inst %" PRIx16, inst);

 // address of the first inst in the IT block, skip the IT inst itself (2 bytes)
 uintptr_t target_addr = pc - 4 + 2;

 it->firstcond = (uint8)(inst >> 4u);
 uint8 firstcond_0 = it->firstcond & 1u;

 memset(it, 0, sizeof(sh_t16_it_t));
 it->insts_cnt = sh_t16_get_it_insts_count(inst);

 size_t insts_idx = 0, pcs_idx = 0;
 for (int parse_else = 1; parse_else >= 0; parse_else--)  // round 0: parse ELSE, round 1: THEN
  {
   uintptr_t target_offset = 0;
   for (size_t i = 0; i < it->insts_cnt; i++)
    {
      bool is_thumb32 = sh_util_is_thumb32(target_addr + target_offset);
      uint8 mask_x = (uint8)(inst >> (uint16)(4 - i)) & 1u;

      if ((parse_else && mask_x != firstcond_0) ||  // parse ELSE or
          (!parse_else && mask_x == firstcond_0))   // parse THEN
      {
        it->insts[insts_idx++] = *((uint16 *)(target_addr + target_offset));
        if (is_thumb32) it->insts[insts_idx++] = *((uint16 *)(target_addr + target_offset + 2));

        it->pcs[pcs_idx++] = target_addr + target_offset + 4;
        if (parse_else) it->insts_else_cnt++;
      }
      target_offset += (is_thumb32 ? 4 : 2);
    }
  }
 it->insts_len = insts_idx * 2;
 return true;
}
//------------------------------------------------------------------------------------------------------------
static void sh_t16_rewrite_it_else(uint16 *buf, uint16 imm9, sh_t16_it_t *it)
{
 buf[0] = 0xD000u | (uint16)(it->firstcond << 8u) | (uint16)(imm9 >> 1u);  // B<c> <label>
 buf[1] = 0xBF00;                                                              // NOP
}
//------------------------------------------------------------------------------------------------------------
static void sh_t16_rewrite_it_then(uint16 *buf, uint16 imm12)
{
 buf[0] = 0xE000u | (uint16)(imm12 >> 1u);  // B <label>
 buf[1] = 0xBF00;                             // NOP
}

//------------------------------------------------------------------------------------------------------------
void sh_inst_get_thumb_rewrite_info(uintptr_t target_addr)
{
 memset(this, 0, sizeof(*this));

 size_t idx = 0;
 uintptr_t target_addr_offset = 0;
 uintptr_t pc = target_addr + 4;
 size_t rewrite_len = 0;

 while (rewrite_len < this->backup_len)
  {
   // IT block
   sh_t16_it_t it;
   if (sh_t16_parse_it(&it, *((uint16 *)(target_addr + target_addr_offset)), pc))
    {
      rewrite_len += (2 + it.insts_len);

      size_t it_block_idx = idx++;
      size_t it_block_len = 4 + 4;  // IT-else + IT-then
      for (size_t i = 0, j = 0; i < it.insts_cnt; i++)
       {
        bool is_thumb32 = sh_util_is_thumb32((uintptr_t)(&(it.insts[j])));
        if (is_thumb32)
         {
          it_block_len += sh_t32_get_rewrite_inst_len(it.insts[j], it.insts[j + 1]);
          this->inst_lens[idx++] = 0;
          this->inst_lens[idx++] = 0;
          j += 2;
         }
         else
          {
           it_block_len += sh_t16_get_rewrite_inst_len(it.insts[j]);
           this->inst_lens[idx++] = 0;
           j += 1;
          }
       }
      this->inst_lens[it_block_idx] = it_block_len;
      target_addr_offset += (2 + it.insts_len);
      pc += (2 + it.insts_len);
    }
    // not IT block
    else
     {
      bool is_thumb32 = sh_util_is_thumb32(target_addr + target_addr_offset);
      size_t inst_len = (is_thumb32 ? 4 : 2);
      rewrite_len += inst_len;
      if (is_thumb32)
       {
        this->inst_lens[idx++] = sh_t32_get_rewrite_inst_len(*((uint16 *)(target_addr + target_addr_offset)), *((uint16 *)(target_addr + target_addr_offset + 2)));
        this->inst_lens[idx++] = 0;
       }
        else this->inst_lens[idx++] = sh_t16_get_rewrite_inst_len(*((uint16 *)(target_addr + target_addr_offset)));
      target_addr_offset += inst_len;
      pc += inst_len;
     }
  }

 this->start_addr    = target_addr;
 this->end_addr      = target_addr + rewrite_len;
 this->buf           = (uint16 *)this->enter_addr;
 this->buf_offset    = 0;
 this->inst_lens_cnt = idx;
}
//------------------------------------------------------------------------------------------------------------
//                                           ARM THUMB-2 (32)
//------------------------------------------------------------------------------------------------------------
// https://developer.arm.com/documentation/ddi0406/latest
// https://developer.arm.com/documentation/ddi0597/latest
//
enum sh_t32_type_t
{
//  IGNORED = 0,
  B_T3,
  B_T4,
  BL_IMM_T1,
  BLX_IMM_T2,
  ADR_T2,
  ADR_T3,
  LDR_LIT_T2,
  LDR_LIT_PC_T2,
  LDRB_LIT_T1,
  LDRD_LIT_T1,
  LDRH_LIT_T1,
  LDRSB_LIT_T1,
  LDRSH_LIT_T1,
  PLD_LIT_T1,
  PLI_LIT_T3,
  TBB_T1,
  TBH_T1,
  VLDR_LIT_T1
};
//------------------------------------------------------------------------------------------------------------
size_t sh_t32_get_rewrite_inst_len(uint16 high_inst, uint16 low_inst)
{
  static uint8 map[] = {
      4,   // IGNORED
      12,  // B_T3
      8,   // B_T4
      12,  // BL_IMM_T1
      12,  // BLX_IMM_T2
      12,  // ADR_T2
      12,  // ADR_T3
      16,  // LDR_LIT_T2
      24,  // LDR_LIT_PC_T2
      16,  // LDRB_LIT_T1
      16,  // LDRD_LIT_T1
      16,  // LDRH_LIT_T1
      16,  // LDRSB_LIT_T1
      16,  // LDRSH_LIT_T1
      20,  // PLD_LIT_T1
      20,  // PLI_LIT_T3
      32,  // TBB_T1
      32,  // TBH_T1
      24   // VLDR_LIT_T1
  };

  uint32 inst = (uint32)(high_inst << 16u) | low_inst;
  return (size_t)(map[sh_t32_get_type(inst)]);
}
//------------------------------------------------------------------------------------------------------------
static sh_t32_type_t sh_t32_get_type(uint32 inst)
{
      if (((inst  & 0xF800D000u) == 0xF0008000u) && ((inst & 0x03800000u) != 0x03800000u))return B_T3;
  else if ((inst  & 0xF800D000u) == 0xF0009000u)return B_T4;
  else if ((inst  & 0xF800D000u) == 0xF000D000u)return BL_IMM_T1;
  else if ((inst  & 0xF800D000u) == 0xF000C000u)return BLX_IMM_T2;
  else if ((inst  & 0xFBFF8000u) == 0xF2AF0000u)return ADR_T2;
  else if ((inst  & 0xFBFF8000u) == 0xF20F0000u)return ADR_T3;
  else if ((inst  & 0xFF7F0000u) == 0xF85F0000u)return ((inst & 0x0000F000u) == 0x0000F000u) ? LDR_LIT_PC_T2 : LDR_LIT_T2;
  else if (((inst & 0xFF7F0000u) == 0xF81F0000u) && ((inst & 0xF000u) != 0xF000u))return LDRB_LIT_T1;
  else if ((inst  & 0xFF7F0000u) == 0xE95F0000u)return LDRD_LIT_T1;
  else if (((inst & 0xFF7F0000u) == 0xF83F0000u) && ((inst & 0xF000u) != 0xF000u))return LDRH_LIT_T1;
  else if (((inst & 0xFF7F0000u) == 0xF91F0000u) && ((inst & 0xF000u) != 0xF000u))return LDRSB_LIT_T1;
  else if (((inst & 0xFF7F0000u) == 0xF93F0000u) && ((inst & 0xF000u) != 0xF000u))return LDRSH_LIT_T1;
  else if ((inst  & 0xFF7FF000u) == 0xF81FF000u)return PLD_LIT_T1;
  else if ((inst  & 0xFF7FF000u) == 0xF91FF000u)return PLI_LIT_T3;
  else if ((inst  & 0xFFF0FFF0u) == 0xE8D0F000u)return TBB_T1;
  else if ((inst  & 0xFFF0FFF0u) == 0xE8D0F010u)return TBH_T1;
  else if ((inst  & 0xFF3F0C00u) == 0xED1F0800u)return VLDR_LIT_T1;
  else return IGNORED;
}
//------------------------------------------------------------------------------------------------------------
size_t sh_t32_rewrite_b(uint16 *buf, uint16 high_inst, uint16 low_inst, uintptr_t pc, sh_t32_type_t type)
{
 uint32 j1 = SH_UTIL_GET_BIT_16(low_inst, 13);
 uint32 j2 = SH_UTIL_GET_BIT_16(low_inst, 11);
 uint32 s  = SH_UTIL_GET_BIT_16(high_inst, 10);
 uint32 i1 = !(j1 ^ s);
 uint32 i2 = !(j2 ^ s);

 uint32 addr;
 if (type == B_T3)
  {
    uint32 x = (s << 20u) | (j2 << 19u) | (j1 << 18u) | ((high_inst & 0x3Fu) << 12u) | ((low_inst & 0x7FFu) << 1u);
    uint32 imm32 = SH_UTIL_SIGN_EXTEND_32(x, 21u);
    addr = SH_UTIL_SET_BIT0(pc + imm32);  // thumb -> thumb
  }
 else if (type == B_T4)
  {
    uint32 x = (s << 24u) | (i1 << 23u) | (i2 << 22u) | ((high_inst & 0x3FFu) << 12u) | ((low_inst & 0x7FFu) << 1u);
    uint32 imm32 = SH_UTIL_SIGN_EXTEND_32(x, 25u);
    addr = SH_UTIL_SET_BIT0(pc + imm32);  // thumb -> thumb
  }
 else if (type == BL_IMM_T1)
  {
    uint32 x = (s << 24u) | (i1 << 23u) | (i2 << 22u) | ((high_inst & 0x3FFu) << 12u) | ((low_inst & 0x7FFu) << 1u);
    uint32 imm32 = SH_UTIL_SIGN_EXTEND_32(x, 25u);
    addr = SH_UTIL_SET_BIT0(pc + imm32);  // thumb -> thumb
  }
 else             // type == BLX_IMM_T2
  {
    uint32 x = (s << 24u) | (i1 << 23u) | (i2 << 22u) | ((high_inst & 0x3FFu) << 12u) | ((low_inst & 0x7FEu) << 1u);
    uint32 imm32 = SH_UTIL_SIGN_EXTEND_32(x, 25u);
    // In BL and BLX instructions, only when the target instruction set is "arm",
    // you need to do 4-byte alignment for PC.
    addr = SH_UTIL_ALIGN_4(pc) + imm32;  // thumb -> arm, align4
  }
 addr = this->sh_txx_fix_addr(addr);

 size_t idx = 0;
 if (type == B_T3)
  {
    uint32 cond = SH_UTIL_GET_BITS_16(high_inst, 9, 6);
    buf[idx++]  = 0xD000u | (uint16)(cond << 8u);  // B<c> #0
    buf[idx++]  = 0xE003;                            // B #6
  }
 else if (type == BL_IMM_T1 || type == BLX_IMM_T2)
  {
    buf[idx++] = 0xF20F;  // ADD LR, PC, #9
    buf[idx++] = 0x0E09;  // ...
  }
 buf[idx++] = 0xF8DF;  // LDR.W PC, [PC]
 buf[idx++] = 0xF000;  // ...
 buf[idx++] = addr & 0xFFFFu;
 buf[idx++] = addr >> 16u;
 return idx * 2;  // 8 or 12
}
//------------------------------------------------------------------------------------------------------------
size_t sh_t32_rewrite_adr(uint16 *buf, uint16 high_inst, uint16 low_inst, uintptr_t pc, sh_t32_type_t type)
{
 uint32 rt    = SH_UTIL_GET_BITS_16(low_inst, 11, 8);  // r0 - r14
 uint32 i     = SH_UTIL_GET_BIT_16(high_inst, 10);
 uint32 imm3  = SH_UTIL_GET_BITS_16(low_inst, 14, 12);
 uint32 imm8  = SH_UTIL_GET_BITS_16(low_inst, 7, 0);
 uint32 imm32 = (i << 11u) | (imm3 << 8u) | imm8;
 uint32 addr  = (type == ADR_T2 ? (SH_UTIL_ALIGN_4(pc) - imm32) : (SH_UTIL_ALIGN_4(pc) + imm32));
 if (this->sh_txx_is_addr_need_fix(addr)) return 0;  // rewrite failed

 buf[0] = 0xF8DF;                      // LDR.W Rt, [PC, #4]
 buf[1] = (uint16)(rt << 12u) + 4u;  // ...
 buf[2] = 0xE002;                      // B #4
 buf[3] = 0xBF00;                      // NOP
 buf[4] = addr & 0xFFFFu;
 buf[5] = addr >> 16u;
 return 12;
}
//------------------------------------------------------------------------------------------------------------
size_t sh_t32_rewrite_ldr(uint16 *buf, uint16 high_inst, uint16 low_inst, uintptr_t pc, sh_t32_type_t type)
{
 uint32 u   = SH_UTIL_GET_BIT_16(high_inst, 7);
 uint32 rt  = SH_UTIL_GET_BITS_16(low_inst, 15, 12);  // r0 - r15
 uint32 rt2 = 0;                                     // r0 - r15
 uint32 addr;

 if (type == LDRD_LIT_T1)
  {
   rt2  = SH_UTIL_GET_BITS_16(low_inst, 11, 8);
   uint32 imm8 = SH_UTIL_GET_BITS_16(low_inst, 7, 0);
   addr = (u ? SH_UTIL_ALIGN_4(pc) + (imm8 << 2u) : SH_UTIL_ALIGN_4(pc) - (imm8 << 2u));
  }
  else
   {
    uint32 imm12 = (uint32)SH_UTIL_GET_BITS_16(low_inst, 11, 0);
    addr = (u ? SH_UTIL_ALIGN_4(pc) + imm12 : SH_UTIL_ALIGN_4(pc) - imm12);
   }
 if (this->sh_txx_is_addr_need_fix(addr)) return 0;  // rewrite failed

 if (type == LDR_LIT_PC_T2 && rt == 0xF)  // Rt == PC
  {
    buf[0] = 0xB403;          // PUSH {R0, R1}
    buf[1] = 0xBF00;          // NOP
    buf[2] = 0xF8DF;          // LDR.W R0, [PC, #4]
    buf[3] = 0x0004;          // ...
    buf[4] = 0xE002;          // B #4
    buf[5] = 0xBF00;          // NOP
    buf[6] = addr & 0xFFFFu;  //
    buf[7] = addr >> 16u;     //
    buf[8] = 0xF8D0;          // LDR.W R0, [R0]
    buf[9] = 0x0000;          // ...
    buf[10] = 0x9001;         // STR R0, [SP, #4]
    buf[11] = 0xBD01;         // POP {R0, PC}
    return 24;
  }
  else
   {
    buf[0] = 0xF8DF;                      // LDR.W Rt, [PC, #4]
    buf[1] = (uint16)(rt << 12u) | 4u;  // ...
    buf[2] = 0xE002;                      // B #4
    buf[3] = 0xBF00;                      // NOP
    buf[4] = addr & 0xFFFFu;
    buf[5] = addr >> 16u;
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch"
    switch (type)
     {
      case LDR_LIT_T2:
        buf[6] = (uint16)(0xF8D0 + rt);  // LDR.W Rt, [Rt]
        buf[7] = (uint16)(rt << 12u);    // ...
        break;
      case LDRB_LIT_T1:
        buf[6] = (uint16)(0xF890 + rt);  // LDRB.W Rt, [Rt]
        buf[7] = (uint16)(rt << 12u);    // ...
        break;
      case LDRD_LIT_T1:
        buf[6] = (uint16)(0xE9D0 + rt);                        // LDRD Rt, Rt2, [Rt]
        buf[7] = (uint16)(rt << 12u) + (uint16)(rt2 << 8u);  // ...
        break;
      case LDRH_LIT_T1:
        buf[6] = (uint16)(0xF8B0 + rt);  // LDRH.W Rt, [Rt]
        buf[7] = (uint16)(rt << 12u);    // ...
        break;
      case LDRSB_LIT_T1:
        buf[6] = (uint16)(0xF990 + rt);  // LDRSB.W Rt, [Rt]
        buf[7] = (uint16)(rt << 12u);    // ...
        break;
      case LDRSH_LIT_T1:
        buf[6] = (uint16)(0xF9B0 + rt);  // LDRSH.W Rt, [Rt]
        buf[7] = (uint16)(rt << 12u);    // ...
        break;
     }
#pragma clang diagnostic pop
    return 16;
  }
}
//------------------------------------------------------------------------------------------------------------
size_t sh_t32_rewrite_pl(uint16 *buf, uint16 high_inst, uint16 low_inst, uintptr_t pc, sh_t32_type_t type)
{
 uint32 u     = SH_UTIL_GET_BIT_16(high_inst, 7);
 uint32 imm12 = SH_UTIL_GET_BITS_16(low_inst, 11, 0);
 uint32 addr  = (u ? SH_UTIL_ALIGN_4(pc) + imm12 : SH_UTIL_ALIGN_4(pc) - imm12);
 addr = this->sh_txx_fix_addr(addr);

 buf[0] = 0xB401;  // PUSH {R0}
 buf[1] = 0xBF00;  // NOP
 buf[2] = 0xF8DF;  // LDR.W R0, [PC, #8]
 buf[3] = 0x0008;  // ...
 if (type == PLD_LIT_T1)
  {
    buf[4] = 0xF890;  // PLD [R0]
    buf[5] = 0xF000;  // ...
  }
  else
   {
    buf[4] = 0xF990;  // PLI [R0]
    buf[5] = 0xF000;  // ...
   }
 buf[6] = 0xBC01;  // POP {R0}
 buf[7] = 0xE001;  // B #2
 buf[8] = addr & 0xFFFFu;
 buf[9] = addr >> 16u;
 return 20;
}
//------------------------------------------------------------------------------------------------------------
size_t sh_t32_rewrite_tb(uint16 *buf, uint16 high_inst, uint16 low_inst, uintptr_t pc, sh_t32_type_t type)
{
 // If TBB/TBH is not the last instruction that needs to be rewritten,
 // the rewriting can NOT be completed.
 uintptr_t target_addr = SH_UTIL_CLEAR_BIT0(pc - 4);
 if (target_addr + 4 != this->end_addr) return 0;  // rewrite failed

 uint32 rn = SH_UTIL_GET_BITS_16(high_inst, 3, 0);
 uint32 rm = SH_UTIL_GET_BITS_16(low_inst, 3, 0);
 uint32 rx, ry;  // r0 - r7
 for (rx = 7;; --rx)
   if (rx != rn && rx != rm) break;
 for (ry = 7;; --ry)
   if (ry != rn && ry != rm && ry != rx) break;

 buf[0] = (uint16)(0xB500u | (1u << rx) | (1u << ry));  // PUSH {Rx, Ry, LR}
 buf[1] = 0xBF00;                                         // NOP
 buf[2] = 0xF8DF;                                         // LDR.W Rx, [PC, #20]
 buf[3] = (uint16)(rx << 12u) | 20u;                    // ...
 if (type == TBB_T1)
  {
   buf[4] = (uint16)(0xEB00u | (rn == 0xF ? rx : rn));  // ADD.W Ry, Rx|Rn, Rm
   buf[5] = (uint16)(0x0000u | (ry << 8u) | rm);        // ...
   buf[6] = (uint16)(0x7800u | (ry << 3u) | ry);        // LDRB Ry, [Ry]
   buf[7] = 0xBF00;                                       // NOP
  }
  else
   {
    buf[4] = (uint16)(0xEB00u | (rn == 0xF ? rx : rn));  // ADD.W Ry, Rx|Rn, Rm, LSL #1
    buf[5] = (uint16)(0x0040u | (ry << 8u) | rm);        // ...
    buf[6] = (uint16)(0x8800u | (ry << 3u) | ry);        // LDRH Ry, [Ry]
    buf[7] = 0xBF00;                                       // NOP
   }
 buf[8]  = (uint16)(0xEB00u | rx);                        // ADD Rx, Rx, Ry, LSL #1
 buf[9]  = (uint16)(0x0040u | (rx << 8u) | ry);           // ...
 buf[10] = (uint16)(0x3001u | (rx << 8u));               // ADD Rx, #1
 buf[11] = (uint16)(0x9002u | (rx << 8u));               // STR Rx, [SP, #8]
 buf[12] = (uint16)(0xBD00u | (1u << rx) | (1u << ry));  // POP {Rx, Ry, PC}
 buf[13] = 0xBF00;                                         // NOP
 buf[14] = pc & 0xFFFFu;
 buf[15] = pc >> 16u;
 return 32;
}
//------------------------------------------------------------------------------------------------------------
size_t sh_t32_rewrite_vldr(uint16 *buf, uint16 high_inst, uint16 low_inst, uintptr_t pc)
{
 uint32 u     = SH_UTIL_GET_BIT_16(high_inst, 7);
 uint32 D     = SH_UTIL_GET_BIT_16(high_inst, 6);
 uint32 vd    = SH_UTIL_GET_BITS_16(low_inst, 15, 12);
 uint32 size  = SH_UTIL_GET_BITS_16(low_inst, 9, 8);
 uint32 imm8  = SH_UTIL_GET_BITS_16(low_inst, 7, 0);
 uint32 esize = (8u << size);
 uint32 imm32 = (esize == 16 ? imm8 << 1u : imm8 << 2u);
 uint32 addr  = (u ? SH_UTIL_ALIGN_4(pc) + imm32 : SH_UTIL_ALIGN_4(pc) - imm32);
 if (this->sh_txx_is_addr_need_fix(addr)) return 0;  // rewrite failed

 buf[0]  = 0xB401;                                       // PUSH {R0}
 buf[1]  = 0xBF00;                                       // NOP
 buf[2]  = 0xF8DF;                                       // LDR.W R0, [PC, #4]
 buf[3]  = 0x0004;                                       // ...
 buf[4]  = 0xE002;                                       // B #4
 buf[5]  = 0xBF00;                                       // NOP
 buf[6]  = addr & 0xFFFFu;                               //
 buf[7]  = addr >> 16u;                                  //
 buf[8]  = (uint16)(0xED90u | D << 6u);                // VLDR Sd|Dd, [R0]
 buf[9]  = (uint16)(0x800u | vd << 12u | size << 8u);  // ...
 buf[10] = 0xBC01;                                       // POP {R0}
 buf[11] = 0xBF00;                                       // NOP
 return 24;
}
//------------------------------------------------------------------------------------------------------------
size_t sh_t32_rewrite(uint16 *buf, uint16 high_inst, uint16 low_inst, uintptr_t pc)
{
 uint32 inst = (uint32)(high_inst << 16u) | low_inst;
 sh_t32_type_t type = sh_t32_get_type(inst);
// SH_LOG_INFO("t32 rewrite: type %d, high inst %" PRIx16 ", low inst %" PRIx16, type, high_inst, low_inst);

 if (type == B_T3 || type == B_T4 || type == BL_IMM_T1 || type == BLX_IMM_T2)return this->sh_t32_rewrite_b(buf, high_inst, low_inst, pc, type);
 else if (type == ADR_T2 || type == ADR_T3)return this->sh_t32_rewrite_adr(buf, high_inst, low_inst, pc, type);
 else if (type == LDR_LIT_T2 || type == LDR_LIT_PC_T2 || type == LDRB_LIT_T1 || type == LDRD_LIT_T1 || type == LDRH_LIT_T1 || type == LDRSB_LIT_T1 || type == LDRSH_LIT_T1)return this->sh_t32_rewrite_ldr(buf, high_inst, low_inst, pc, type);
 else if (type == PLD_LIT_T1 || type == PLI_LIT_T3)return this->sh_t32_rewrite_pl(buf, high_inst, low_inst, pc, type);
 else if (type == TBB_T1 || type == TBH_T1)return this->sh_t32_rewrite_tb(buf, high_inst, low_inst, pc, type);
 else if (type == VLDR_LIT_T1)return this->sh_t32_rewrite_vldr(buf, high_inst, low_inst, pc);

 buf[0] = high_inst;
 buf[1] = low_inst;
 return 4;  // IGNORED
}
//------------------------------------------------------------------------------------------------------------
size_t sh_t32_absolute_jump(uint16 *buf, bool is_align4, uintptr_t addr)
{
 size_t i = 0;
 if (!is_align4) buf[i++] = 0xBF00;  // NOP
 buf[i++] = 0xF8DF;                  // LDR.W PC, [PC]
 buf[i++] = 0xF000;                  // ...
 buf[i++] = addr & 0xFFFFu;
 buf[i++] = addr >> 16u;
 return i * 2;
}
//------------------------------------------------------------------------------------------------------------
size_t sh_t32_relative_jump(uint16 *buf, uintptr_t addr, uintptr_t pc)
{
 uint32 imm32 = addr - pc;
 uint32 s     = SH_UTIL_GET_BIT_32(imm32, 24);
 uint32 i1    = SH_UTIL_GET_BIT_32(imm32, 23);
 uint32 i2    = SH_UTIL_GET_BIT_32(imm32, 22);
 uint32 imm10 = SH_UTIL_GET_BITS_32(imm32, 21, 12);
 uint32 imm11 = SH_UTIL_GET_BITS_32(imm32, 11, 1);
 uint32 j1    = (!i1) ^ s;
 uint32 j2    = (!i2) ^ s;

 buf[0] = (uint16)(0xF000u | (s << 10u) | imm10);
 buf[1] = (uint16)(0x9000u | (j1 << 13u) | (j2 << 11u) | imm11);
 return 4;
}
//------------------------------------------------------------------------------------------------------------
};
//============================================================================================================
// Impl:
//------------------------------------------------------------------------------------------------------------
int sh_inst_hook_thumb_rewrite(uintptr_t target_addr, uintptr_t *orig_addr, size_t *rewrite_len)
{
 // backup original instructions (length: 4 or 8 or 10)
 memcpy((void *)(this->backup), (void *)target_addr, this->backup_len);

 // package the information passed to rewrite
 STXX rinfo;
 rinfo.sh_inst_get_thumb_rewrite_info(target_addr);

 // backup and rewrite original instructions
 uintptr_t target_addr_offset = 0;
 uintptr_t pc = target_addr + 4;
 *rewrite_len = 0;
 while (*rewrite_len < this->backup_len)
  {
   // IT block
   typename STXX::sh_t16_it_t it;
   if (sh_t16_parse_it(&it, *((uint16 *)(target_addr + target_addr_offset)), pc))
    {
     *rewrite_len += (2 + it.insts_len);

     // save space holder point of IT-else B instruction
     uintptr_t enter_inst_else_p = this->enter_addr + rinfo.buf_offset;
     rinfo.buf_offset += 2;  // B<c> <label>
     rinfo.buf_offset += 2;  // NOP

     // rewrite IT block
     size_t enter_inst_else_len = 4;  // B<c> + NOP + B + NOP
     size_t enter_inst_then_len = 0;  // B + NOP
     uintptr_t enter_inst_then_p = 0;
     for (size_t i = 0, j = 0; i < it.insts_cnt; i++)
      {
       if (i == it.insts_else_cnt)
        {
          // save space holder point of IT-then (for B instruction)
          enter_inst_then_p = this->enter_addr + rinfo.buf_offset;
          rinfo.buf_offset += 2;  // B <label>
          rinfo.buf_offset += 2;  // NOP

          // fill IT-else B instruction
          sh_t16_rewrite_it_else((uint16 *)enter_inst_else_p, (uint16)enter_inst_else_len, &it);
        }
        // rewrite instructions in IT block
        bool is_thumb32 = sh_util_is_thumb32((uintptr_t)(&(it.insts[j])));
        size_t len;
        if (is_thumb32)len = rinfo.sh_t32_rewrite((uint16 *)(this->enter_addr + rinfo.buf_offset), it.insts[j], it.insts[j + 1], it.pcs[i]);
          else len = rinfo.sh_t16_rewrite((uint16 *)(this->enter_addr + rinfo.buf_offset), it.insts[j], it.pcs[i]);
        if (0 == len) return -1;
        rinfo.buf_offset += len;
        j += (is_thumb32 ? 2 : 1);

        // save the total offset for ELSE/THEN in enter
        if (i < it.insts_else_cnt)enter_inst_else_len += len;
          else enter_inst_then_len += len;
        if (i == it.insts_cnt - 1)STXX::sh_t16_rewrite_it_then((uint16 *)enter_inst_then_p, (uint16)enter_inst_then_len);    // fill IT-then B instruction
      }

      target_addr_offset += (2 + it.insts_len);
      pc += (2 + it.insts_len);
    }
    // not IT block
    else
     {
      bool is_thumb32 = sh_util_is_thumb32(target_addr + target_addr_offset);
      size_t inst_len = (is_thumb32 ? 4 : 2);
      *rewrite_len += inst_len;

      // rewrite original instructions (fill in enter)
//      SH_LOG_INFO("thumb rewrite: offset %zu, pc %" PRIxPTR, rinfo.buf_offset, pc);
      size_t len;
      if (is_thumb32)len = rinfo.sh_t32_rewrite((uint16 *)(this->enter_addr + rinfo.buf_offset), *((uint16 *)(target_addr + target_addr_offset)), *((uint16 *)(target_addr + target_addr_offset + 2)), pc);
        else len = rinfo.sh_t16_rewrite((uint16 *)(this->enter_addr + rinfo.buf_offset), *((uint16 *)(target_addr + target_addr_offset)), pc);
      if (0 == len) return -2;
      rinfo.buf_offset += len;

      target_addr_offset += inst_len;
      pc += inst_len;
     }
  }
//  SH_LOG_INFO("thumb rewrite: len %zu to %zu", *rewrite_len, rinfo.buf_offset);

 // absolute jump back to remaining original instructions (fill in enter)
 rinfo.buf_offset += STXX::sh_t32_absolute_jump((uint16 *)(this->enter_addr + rinfo.buf_offset), true, SH_UTIL_SET_BIT0(target_addr + *rewrite_len));
 sh_util_clear_cache(this->enter_addr, rinfo.buf_offset);
 return 0;
}
//------------------------------------------------------------------------------------------------------------
#ifdef SH_CONFIG_DETECT_THUMB_TAIL_ALIGNED
static bool sh_inst_thumb_is_long_enough(uintptr_t target_addr, size_t overwrite_len)
{
  uint dli_ssize = 999; // ???
  if (overwrite_len <= dli_ssize) return true;

  // check align-4 in the end of symbol
  if ((overwrite_len == dli_ssize + 2) && ((target_addr + dli_ssize) % 4 == 2)) {
    uintptr_t sym_end = target_addr + dli_ssize;
    if (0 != sh_util_mprotect(sym_end, 2, PROT_READ | PROT_WRITE | PROT_EXEC)) return false;

    // should be zero-ed
    if (0 != *((uint16 *)sym_end)) return false;

    // should not belong to any symbol
    void *dlcache = NULL;
    xdl_info_t dlinfo2;
    if (sh_util_get_api_level() >= __ANDROID_API_L__) {
      xdl_addr((void *)SH_UTIL_SET_BIT0(sym_end), &dlinfo2, &dlcache);
    } else {
      SH_SIG_TRY(SIGSEGV, SIGBUS) {
        xdl_addr((void *)SH_UTIL_SET_BIT0(sym_end), &dlinfo2, &dlcache);
      }
      SH_SIG_CATCH() {
        memset(&dlinfo2, 0, sizeof(dlinfo2));
        SH_LOG_WARN("thumb detect tail aligned: crashed");
      }
      SH_SIG_EXIT
    }
    xdl_addr_clean(&dlcache);
    if (NULL != dlinfo2.dli_sname) return false;

    // trust here is useless alignment data
    return true;
  }

  return false;
}
#endif
//------------------------------------------------------------------------------------------------------------
#ifdef SH_CONFIG_TRY_WITH_EXIT

// B T4: [-16M, +16M - 2]
#define SH_INST_T32_B_RANGE_LOW  (16777216)
#define SH_INST_T32_B_RANGE_HIGH (16777214)

static int sh_inst_hook_thumb_with_exit(uintptr_t target_addr, uintptr_t new_addr, uintptr_t *orig_addr, uintptr_t *orig_addr2)
{
  int r;
  target_addr = SH_UTIL_CLEAR_BIT0(target_addr);
  uintptr_t pc = target_addr + 4;
  this->backup_len = 4;
  uint dli_ssize = 999; // ???
#ifdef SH_CONFIG_DETECT_THUMB_TAIL_ALIGNED
  if (!sh_inst_thumb_is_long_enough(target_addr, this->backup_len, dlinfo))
    return SHADOWHOOK_ERRNO_HOOK_SYMSZ;
#else
  if (dli_ssize < this->backup_len) return SHADOWHOOK_ERRNO_HOOK_SYMSZ;
#endif

  // alloc an exit for absolute jump
  sh_t32_absolute_jump((uint16 *)this->exit, true, new_addr);
  if (0 != (r = sh_exit_alloc(&this->exit_addr, &this->exit_type, pc, dlinfo, (uint8_t *)(this->exit), sizeof(this->exit), SH_INST_T32_B_RANGE_LOW, SH_INST_T32_B_RANGE_HIGH)))
    return r;

  // rewrite
  if (0 != sh_util_mprotect(target_addr, dli_ssize, PROT_READ | PROT_WRITE | PROT_EXEC)) {
    r = SHADOWHOOK_ERRNO_MPROT;
    goto err;
  }
  size_t rewrite_len = 0;
  SH_SIG_TRY(SIGSEGV, SIGBUS) {
    r = this->sh_inst_hook_thumb_rewrite(target_addr, &rewrite_len);
  }
  SH_SIG_CATCH() {
    r = SHADOWHOOK_ERRNO_HOOK_REWRITE_CRASH;
    goto err;
  }
  SH_SIG_EXIT
  if (0 != r) goto err;

  // relative jump to the exit by overwriting the head of original function
  sh_t32_relative_jump((uint16 *)this->trampo, this->exit_addr, pc);
  __atomic_thread_fence(__ATOMIC_SEQ_CST);
  if (0 != (r = sh_util_write_inst(target_addr, this->trampo, this->backup_len))) goto err;

  SH_LOG_INFO("thumb: hook (WITH EXIT) OK. target %" PRIxPTR " -> exit %" PRIxPTR " -> new %" PRIxPTR " -> enter %" PRIxPTR " -> remaining %" PRIxPTR, target_addr, this->exit_addr, new_addr, this->enter_addr, SH_UTIL_SET_BIT0(target_addr + rewrite_len));
  return 0;

err:
  sh_exit_free(this->exit_addr, this->exit_type, (uint8_t *)(this->exit), sizeof(this->exit));
  this->exit_addr = 0;  // this is a flag for with-exit or without-exit
  return r;
}
#endif
//------------------------------------------------------------------------------------------------------------
int sh_inst_hook_thumb_without_exit(uintptr_t target_addr, uintptr_t new_addr, uintptr_t *orig_addr, uintptr_t *orig_addr2)
{
  int r;
  target_addr = SH_UTIL_CLEAR_BIT0(target_addr);
  bool is_align4 = (0 == (target_addr % 4));
  this->backup_len = (is_align4 ? 8 : 10);
  uint dli_ssize = 999; // ???
#ifdef SH_CONFIG_DETECT_THUMB_TAIL_ALIGNED
  if (!sh_inst_thumb_is_long_enough(target_addr, this->backup_len, dlinfo))
    return -1;
#else
  if (dli_ssize < this->backup_len) return -2;
#endif

  // rewrite
  if (0 != NPTM::MemProtect((vptr)target_addr, dli_ssize, PX::PROT_READ | PX::PROT_WRITE | PX::PROT_EXEC))return -1;
  size_t rewrite_len = 0;

  r = this->sh_inst_hook_thumb_rewrite(target_addr, orig_addr, orig_addr2, &rewrite_len);

  if (0 != r) return r;

  // absolute jump to new function address by overwriting the head of original function
  STXX::sh_t32_absolute_jump((uint16 *)this->trampo, is_align4, new_addr);
  __atomic_thread_fence(__ATOMIC_SEQ_CST);
  if (0 != (r = sh_util_write_inst(target_addr, this->trampo, this->backup_len))) return r;

//  SH_LOG_INFO("thumb: hook (WITHOUT EXIT) OK. target %" PRIxPTR " -> new %" PRIxPTR " -> enter %" PRIxPTR " -> remaining %" PRIxPTR, target_addr, new_addr, this->enter_addr, SH_UTIL_SET_BIT0(target_addr + rewrite_len));
  return 0;
}
//------------------------------------------------------------------------------------------------------------
int sh_inst_hook_arm_rewrite(uintptr_t target_addr)
{
 // backup original instructions (length: 4 or 8)
 memcpy((void *)(this->backup), (void *)target_addr, this->backup_len);

 // package the information passed to rewrite
 SA32RW rinfo;
 rinfo.overwrite_start_addr  = target_addr;
 rinfo.overwrite_end_addr    = target_addr + this->backup_len;
 rinfo.rewrite_buf           = (uint32 *)this->enter_addr;
 rinfo.rewrite_buf_offset    = 0;
 rinfo.rewrite_inst_lens_cnt = this->backup_len / 4;
 for (uintptr_t i = 0; i < this->backup_len; i += 4)rinfo.rewrite_inst_lens[i / 4] = STXX::sh_a32_get_rewrite_inst_len(*((uint32 *)(target_addr + i)));

 // rewrite original instructions (fill in enter)
 uintptr_t pc = target_addr + 8;
 for (uintptr_t i = 0; i < this->backup_len; i += 4, pc += 4)
  {
    size_t offset = rinfo.sh_a32_rewrite((uint32 *)(this->enter_addr + rinfo.rewrite_buf_offset), *((uint32 *)(target_addr + i)), pc);
    if (0 == offset) return -1;
    rinfo.rewrite_buf_offset += offset;
  }

 // absolute jump back to remaining original instructions (fill in enter)
 rinfo.rewrite_buf_offset += sh_a32_absolute_jump((uint32 *)(this->enter_addr + rinfo.rewrite_buf_offset), target_addr + this->backup_len);
 sh_util_clear_cache(this->enter_addr, rinfo.rewrite_buf_offset);
 return 0;
}
//------------------------------------------------------------------------------------------------------------
#ifdef SH_CONFIG_TRY_WITH_EXIT

// B A1: [-32M, +32M - 4]
#define SH_INST_A32_B_RANGE_LOW  (33554432)
#define SH_INST_A32_B_RANGE_HIGH (33554428)

static int sh_inst_hook_arm_with_exit(sh_inst_t *self, uintptr_t target_addr, uintptr_t new_addr, uintptr_t *orig_addr, uintptr_t *orig_addr2)
{
  int r;
  uintptr_t pc = target_addr + 8;
  this->backup_len = 4;
   uint dli_ssize = 999; // ???
  if (dli_ssize < this->backup_len) return SHADOWHOOK_ERRNO_HOOK_SYMSZ;

  // alloc an exit for absolute jump
  sh_a32_absolute_jump(this->exit, new_addr);
  if (0 != (r = sh_exit_alloc(&this->exit_addr, &this->exit_type, pc, dlinfo, (uint8_t *)(this->exit), sizeof(this->exit), SH_INST_A32_B_RANGE_LOW, SH_INST_A32_B_RANGE_HIGH)))return r;

  // rewrite
  if (0 != sh_util_mprotect(target_addr, this->backup_len, PROT_READ | PROT_WRITE | PROT_EXEC)) {
    r = SHADOWHOOK_ERRNO_MPROT;
    goto err;
  }
  SH_SIG_TRY(SIGSEGV, SIGBUS) {
    r = sh_inst_hook_arm_rewrite(self, target_addr, orig_addr, orig_addr2);
  }
  SH_SIG_CATCH() {
    r = SHADOWHOOK_ERRNO_HOOK_REWRITE_CRASH;
    goto err;
  }
  SH_SIG_EXIT
  if (0 != r) goto err;

  // relative jump to the exit by overwriting the head of original function
  sh_a32_relative_jump(this->trampo, this->exit_addr, pc);
  __atomic_thread_fence(__ATOMIC_SEQ_CST);
  if (0 != (r = sh_util_write_inst(target_addr, this->trampo, this->backup_len))) goto err;

  SH_LOG_INFO("a32: hook (WITH EXIT) OK. target %" PRIxPTR " -> exit %" PRIxPTR " -> new %" PRIxPTR " -> enter %" PRIxPTR " -> remaining %" PRIxPTR, target_addr, this->exit_addr, new_addr, this->enter_addr, target_addr + this->backup_len);
  return 0;

err:
  sh_exit_free(this->exit_addr, this->exit_type, (uint8_t *)(this->exit), sizeof(this->exit));
  this->exit_addr = 0;  // this is a flag for with-exit or without-exit
  return r;
}

#endif
//------------------------------------------------------------------------------------------------------------
int sh_inst_hook_arm_without_exit(uintptr_t target_addr, uintptr_t new_addr, uintptr_t *orig_addr, uintptr_t *orig_addr2)
{
  int r;
  this->backup_len = 8;
  uint dli_ssize = 999; // ???
  if (dli_ssize < this->backup_len) return -1;

  // rewrite
  if (0 != sh_util_mprotect(target_addr, this->backup_len, PX::PROT_READ | PX::PROT_WRITE | PX::PROT_EXEC))return -2;
  r = this->sh_inst_hook_arm_rewrite(target_addr, orig_addr, orig_addr2);

  if (0 != r) return r;

  // absolute jump to new function address by overwriting the head of original function
  sh_a32_absolute_jump(this->trampo, new_addr);
  __atomic_thread_fence(__ATOMIC_SEQ_CST);
  if (0 != (r = sh_util_write_inst(target_addr, this->trampo, this->backup_len))) return r;

//  SH_LOG_INFO("a32: hook (WITHOUT EXIT) OK. target %" PRIxPTR " -> new %" PRIxPTR " -> enter %" PRIxPTR " -> remaining %" PRIxPTR, target_addr, new_addr, this->enter_addr, target_addr + this->backup_len);
  return 0;
}
//------------------------------------------------------------------------------------------------------------
public:

int sh_inst_hook(uintptr_t target_addr, uintptr_t new_addr, uintptr_t *orig_addr, uintptr_t *orig_addr2)
{
//  this->enter_addr = sh_enter_alloc();
  if (0 == this->enter_addr) return -1;

  int r;
  if (SH_UTIL_IS_THUMB(target_addr)) {
#ifdef SH_CONFIG_TRY_WITH_EXIT
    if (0 == (r = this->sh_inst_hook_thumb_with_exit(target_addr, new_addr, orig_addr, orig_addr2)))
      return r;
#endif
    if (0 ==
        (r = this->sh_inst_hook_thumb_without_exit(target_addr, new_addr, orig_addr, orig_addr2)))
      return r;
  } else {
#ifdef SH_CONFIG_TRY_WITH_EXIT
    if (0 == (r = this->sh_inst_hook_arm_with_exit(target_addr, new_addr, orig_addr, orig_addr2)))
      return r;
#endif
    if (0 == (r = this->sh_inst_hook_arm_without_exit(target_addr, new_addr, orig_addr, orig_addr2)))
      return r;
  }

  // hook failed
  sh_enter_free(this->enter_addr);
  return r;
}
//------------------------------------------------------------------------------------------------------------
int sh_inst_unhook(uintptr_t target_addr)
{
  int r;
  bool is_thumb = SH_UTIL_IS_THUMB(target_addr);

  if (is_thumb) target_addr = SH_UTIL_CLEAR_BIT0(target_addr);

  // restore the instructions at the target address
    r = memcmp((void *)target_addr, this->trampo, this->backup_len);
  if (0 != r) return -1;
  if (0 != (r = sh_util_write_inst(target_addr, this->backup, this->backup_len))) return r;
  __atomic_thread_fence(__ATOMIC_SEQ_CST);

  // free memory space for exit
  if (0 != this->exit_addr)
    if (0 != (r = sh_exit_free(this->exit_addr, this->exit_type, (uint8 *)(this->exit), sizeof(this->exit))))
      return r;

  // free memory space for enter
 // sh_enter_free(this->enter_addr);

//  SH_LOG_INFO("%s: unhook OK. target %" PRIxPTR, is_thumb ? "thumb" : "a32", target_addr);
  return 0;
}
//------------------------------------------------------------------------------------------------------------
//struct sh_inst_t
//{
/*  uint32  trampo[4];    // align 16 // length == backup_len
  uint8_t   backup[16];   // align 16
  uint16  backup_len;   // == 4 or 8 or 10
  uint16  exit_type;
  uintptr_t exit_addr;
  uint32  exit[2];
  uintptr_t enter_addr; */

  uint32  exit[4];     // Far stub, actually  // Stored here to check if it is hooked?
  uint32  trampo[4];   // align 16 // length == backup_len    // Used on unhook to check that we still own this hook
  uint32  backup[4];   // align 16     // uint8_t   backup[16];
  uint32  backup_len;  // == 4 or 16
  uint32  exit_type;   // Stores prev fill byte of the gap and its allocation type (TODO)
  vptr    exit_addr;   // Probably points to some hole nearby or into allocated page  (Must be in an Executable memory, nearby to target proc start addr)
  vptr    enter_addr;  // Points to a stub with instructions taken(and possibly recompiled) from a target proc start addr (Must be in an Executable memory)
//};
//------------------------------------------------------------------------------------------------------------
};
//============================================================================================================
