#include <stdbool.h>
#include <stdio.h>
#include "cpu.h"
#include "cycle.h"
#include "main.h"

static inline void cpu_inst_push(CPU*, const uint16_t);

// d8/d16 reads
static inline uint16_t read_next_word(CPU *cpu) {
    return mmu_rb(cpu->registers.pc++, 1) | (mmu_rb(cpu->registers.pc++, 1) << 8);
}
static inline uint8_t read_next_byte(CPU *cpu) {
	return mmu_rb(cpu->registers.pc++, 1);
}

// Instructions
static inline void cpu_inst_add(CPU* cpu, const uint8_t val, const bool cy) {
    uint16_t hold_word = cpu->registers.a + val + cy;
    cpu->registers.flags.ze = !(hold_word);
    cpu->registers.flags.ne = 0;
    cpu->registers.flags.hf = (uint8_t) ((val & 0xf) + (cpu->registers.a & 0xf) + cy) > 0xf;
    cpu->registers.flags.cy = hold_word > 0xff;
    cpu->registers.a = hold_word;
}
static inline void cpu_inst_add_hl(CPU* cpu, const uint16_t val) {
    // flag ze not affected
    cpu->registers.flags.ne = 0;
    cpu->registers.flags.hf = (uint16_t) ((cpu->registers.hl & 0xfff) + (val & 0xfff)) > 0xfff;
    cpu->registers.flags.cy = (uint32_t) (cpu->registers.hl + val) > 0xffff;
    cpu->registers.hl += val;
}
static inline void cpu_inst_and(CPU* cpu, const uint8_t val) {
    cpu->registers.a &= val;
    cpu->registers.flags.ze = !(cpu->registers.a);
    cpu->registers.flags.hf = 1;
    cpu->registers.flags.ne = cpu->registers.flags.cy = 0;
}
static inline void cpu_inst_call(CPU *cpu, const uint16_t addr) {
	cpu_inst_push(cpu, cpu->registers.pc);
	cpu->registers.pc = addr;
}
static inline void cpu_inst_cp(CPU *cpu, const uint8_t val) {
	cpu->registers.flags.ze = (*cpu->regAddr[REG_A] == val); // same value subtraction = 0
	cpu->registers.flags.ne = 1;
	cpu->registers.flags.hf = (*cpu->regAddr[REG_A] & 0xf) > (val & 0xf);
    cpu->registers.flags.cy = cpu->registers.a < val;
}
static inline void cpu_inst_dec(CPU* cpu, uint8_t* reg) {
	uint8_t hold_byte = *reg - 1;
	cpu->registers.flags.ze = !(hold_byte);
	cpu->registers.flags.ne = 1;
	cpu->registers.flags.hf = (uint8_t) ((*reg & 0xf) - 1) > 0xf;

    (*reg)--;
}
static inline uint8_t cpu_inst_inc(CPU* cpu, const uint8_t val) {
    uint8_t hold_byte = val + 1;
    cpu->registers.flags.ze = !(hold_byte);
    cpu->registers.flags.ne = 0;
    cpu->registers.flags.hf = (uint8_t) ((val & 0xf) + 1) > 0xf;
    return hold_byte;
}
static inline void cpu_inst_or(CPU* cpu, const uint8_t val) {
    cpu->registers.a |= val;
    cpu->registers.flags.ze = !(cpu->registers.a);
    cpu->registers.flags.ne = cpu->registers.flags.hf = cpu->registers.flags.cy = 0;
}
static inline uint16_t cpu_inst_pop(CPU *cpu) {
    return mmu_rb(cpu->registers.sp++, 1) | (mmu_rb(cpu->registers.sp++, 1) << 8);
}
static inline void cpu_inst_push(CPU *cpu, const uint16_t val) {
    mmu_wb(--cpu->registers.sp, val >> 8);
    mmu_wb(--cpu->registers.sp, val & 0xff);

   //cpu->registers.sp--; // decrement sp location
}
// diff between rotates
// through carry: flip bit (pos 0 or 7) THROUGH old carry, gets carry on old state
// THROUGH being the path of rotate includes carry flag
// normal: gets carry on old state pos (0 or 7) then rotates (basically wrap content)
static inline uint8_t cpu_inst_rl(CPU* cpu, uint8_t val) {
    uint8_t hold_byte = cpu->registers.flags.cy;
    cpu->registers.flags.ne = cpu->registers.flags.hf = 0;
    cpu->registers.flags.cy = val >> 7;
    val = (val << 1) | hold_byte;                        // !
    cpu->registers.flags.ze = !(val);
    return val;
}
static inline uint8_t cpu_inst_rlc(CPU* cpu, uint8_t val) {
    cpu->registers.flags.ne = cpu->registers.flags.hf = 0;
    cpu->registers.flags.cy = val >> 7;
    val = (val << 1) | cpu->registers.flags.cy;         // !
    cpu->registers.flags.ze = !(val);
    return val;
}
static inline uint8_t cpu_inst_rr(CPU* cpu, uint8_t val) {
    uint8_t hold_byte = cpu->registers.flags.cy;
    cpu->registers.flags.ne = cpu->registers.flags.hf = 0;
    cpu->registers.flags.cy = val & 0x1;
    val = (val >> 1) | (hold_byte << 7);                // !
    cpu->registers.flags.ze = !(val);
    return val;
}
static inline uint8_t cpu_inst_rrc(CPU* cpu, uint8_t val) {
    cpu->registers.flags.ne = cpu->registers.flags.hf = 0;
    cpu->registers.flags.cy = val & 0x1;
    val = (val >> 1) | (cpu->registers.flags.cy << 7); // !
    cpu->registers.flags.ze = !(val);
    return val;
}
static inline void cpu_inst_rst(CPU* cpu, const int val) {
	cpu_inst_push(cpu, cpu->registers.pc);
	cpu->registers.pc = val;
}
// book says set if "no borrow"
static inline void cpu_inst_sub(CPU* cpu, const uint8_t val, const bool cy) {
    uint16_t hold_word = cpu->registers.a - (val + cy);
    cpu->registers.flags.ze = !(hold_word);
    cpu->registers.flags.ne = 1;
    cpu->registers.flags.hf = !((uint8_t) ((cpu->registers.a & 0xf) - ((val & 0xf) + cy)) > 0xf);
    cpu->registers.flags.cy = !(hold_word > 0xff);
    cpu->registers.a = hold_word;
}
static inline void cpu_inst_xor(CPU* cpu, const uint8_t val) {
	*cpu->regAddr[REG_A] ^= val;
	cpu->registers.flags.ze = !(*cpu->regAddr[REG_A]);
	cpu->registers.flags.ne = cpu->registers.flags.hf = cpu->registers.flags.cy = 0;
}
// conditional jumps
static inline void cpu_inst_cond_call(CPU* cpu, const bool cond, const uint16_t addr) {
    if (cond) cpu_inst_call(cpu, addr);
}
static inline void cpu_inst_cond_jp_sign(CPU* cpu, const bool cond, const int8_t data) {
    cpu->clock.cur_cyc = 8;

	if (cond)
    {
        cpu->registers.pc +=  data;
        cpu->clock.cur_cyc = 12;
    }
}
static inline void cpu_inst_cond_jp_word(CPU *cpu, const bool cond, const uint16_t addr) {
    cpu->clock.cur_cyc = 12;

    if (cond)
    {
        cpu->registers.pc = addr;
        cpu->clock.cur_cyc = 16;
    }
}
static inline void cpu_inst_cond_ret(CPU* cpu, const bool cond) {
    cpu->clock.cur_cyc = 8;

    if (cond)
    {
        cpu->registers.pc = cpu_inst_pop(cpu);
        cpu->clock.cur_cyc = 20;
    }
}

// prefix cb
static inline void cpu_inst_bit(CPU* cpu, const uint8_t regval, const uint8_t pos) {
    cpu->registers.flags.ze = ((regval & (1 << pos)) == 0);
    cpu->registers.flags.ne = 0;
    cpu->registers.flags.hf = 1;
    // cy not affected
}
static inline void cpu_inst_res(uint8_t* reg, const uint8_t pos) {
    *reg = *reg & (uint8_t) ~(1 << pos);
}
static inline void cpu_inst_set(uint8_t* reg, const uint8_t pos) {
    *reg = *reg | (1 << pos);
}

void cpu_step(CPU *cpu, const int addr_cb) {
    // reset last clock
    cpu->clock.cur_cyc = 0;
    cpu->clock.cur_mem = 0;

	uint16_t op = mmu_rb(cpu->registers.pc++, 1) + addr_cb;

	switch(op)
	{
        // PREFIX CB
        case 0xcb:
            cpu->clock.cyc += CPU_CYCLE[op] + cpu->clock.cur_cyc;
            cpu->clock.mem += cpu->clock.cur_mem;
            cpu_step(cpu, ADDR_CB); // Immediate execute
            return;

		// NOP
		case 0x00:
			break;
		// Disabled — should not modify anything in cpu
		case 0xd3: case 0xdb: case 0xdd: case 0xe3: case 0xe4: case 0xeb: case 0xec:
		case 0xed: case 0xf4: case 0xfc: case 0xfd:
			return;

		// RST
		case 0xc7: cpu_inst_rst(cpu, 0x00); break;
		case 0xcf: cpu_inst_rst(cpu, 0x08); break;
		case 0xd7: cpu_inst_rst(cpu, 0x10); break;
		case 0xdf: cpu_inst_rst(cpu, 0x18); break;
		case 0xe7: cpu_inst_rst(cpu, 0x20); break;
		case 0xef: cpu_inst_rst(cpu, 0x28); break;
		case 0xf7: cpu_inst_rst(cpu, 0x30); break;
		case 0xff: cpu_inst_rst(cpu, 0x38); break;

        // INC regs
        case 0x04: case 0x0c: case 0x14: case 0x1c: case 0x24: case 0x2c: case 0x3c:
        	{
            	uint16_t hold_src = (op & 0x38) >> 3;
            	*cpu->regAddr[hold_src] = cpu_inst_inc(cpu, *cpu->regAddr[hold_src]);
            }
            break;
		// DEC regs
		case 0x05: case 0x0d: case 0x15: case 0x1d: case 0x25: case 0x2d: case 0x3d:
			cpu_inst_dec(cpu, cpu->regAddr[(op & 0x38) >> 3]);
			break;
		// LD regs, d8
		case 0x06: case 0x0e: case 0x16: case 0x1e: case 0x26: case 0x2e: case 0x3e:
			(*cpu->regAddr[(op & 0x38) >> 3]) = read_next_byte(cpu);
			break;
		// LD reg, reg
		case 0x40: case 0x41: case 0x42: case 0x43: case 0x44: case 0x45: case 0x47:
		case 0x48: case 0x49: case 0x4a: case 0x4b: case 0x4c: case 0x4d: case 0x4f:
		case 0x50: case 0x51: case 0x52: case 0x53: case 0x54: case 0x55: case 0x57:
		case 0x58: case 0x59: case 0x5a: case 0x5b: case 0x5c: case 0x5d: case 0x5f:
		case 0x60: case 0x61: case 0x62: case 0x63: case 0x64: case 0x65: case 0x67:
		case 0x68: case 0x69: case 0x6a: case 0x6b: case 0x6c: case 0x6d: case 0x6f:
		case 0x78: case 0x79: case 0x7a: case 0x7b: case 0x7c: case 0x7d: case 0x7f:
			*cpu->regAddr[(op & 0x38) >> 3] = *cpu->regAddr[op & 0x7];
			break;
        // ADD regs
        case 0x80: case 0x81: case 0x82: case 0x83: case 0x84: case 0x85: case 0x87:
            cpu_inst_add(cpu, *cpu->regAddr[op & 0x7], 0);
            break;
        // ADD (HL)/d8
        case 0x86:
            cpu_inst_add(cpu, mmu_rb(cpu->registers.hl, 1), 0);
            break;
        case 0xc6:
            cpu_inst_add(cpu, read_next_byte(cpu), 0);
            break;
        // ADC regs
        case 0x88: case 0x89: case 0x8a: case 0x8b: case 0x8c: case 0x8d: case 0x8f:
            cpu_inst_add(cpu, *cpu->regAddr[op & 0x7], cpu->registers.flags.cy);
            break;
        // ADC (HL)
        case 0x8e:
            cpu_inst_add(cpu, mmu_rb(cpu->registers.hl, 1), cpu->registers.flags.cy);
            break;
        // SUB regs
        case 0x90: case 0x91: case 0x92: case 0x93: case 0x94: case 0x95: case 0x97:
            cpu_inst_sub(cpu, *cpu->regAddr[op & 0x7], 0);
            break;
        // SBC regs
        case 0x98: case 0x99: case 0x9a: case 0x9b: case 0x9c: case 0x9d: case 0x9f:
            cpu_inst_sub(cpu, *cpu->regAddr[op & 0x7], cpu->registers.flags.cy);
            break;
        // SUB/SBC (HL)
        case 0x96:
            cpu_inst_sub(cpu, mmu_rb(cpu->registers.hl, 1), 0);
            break; // SUB (HL)
        case 0x9e:
            cpu_inst_sub(cpu, mmu_rb(cpu->registers.hl, 1), cpu->registers.flags.cy);
            break; // SBC (HL)
        // AND regs
        case 0xa0: case 0xa1: case 0xa2: case 0xa3: case 0xa4: case 0xa5: case 0xa7:
            cpu_inst_and(cpu, *cpu->regAddr[op & 0x7]);
            break;
        // AND (HL)/d16
        case 0xa6:
            cpu_inst_and(cpu, mmu_rb(cpu->registers.hl, 1));
            break;
        case 0xe6:
            cpu_inst_and(cpu, read_next_byte(cpu));
            break;
		// XOR
		case 0xa8: case 0xa9: case 0xaa: case 0xab: case 0xac: case 0xad: case 0xaf:
			cpu_inst_xor(cpu, *cpu->regAddr[op & 0x7]);
			break;
        // XOR (HL)/d8
        case 0xae: cpu_inst_xor(cpu, mmu_rb(cpu->registers.hl, 1)); break;
        case 0xee: cpu_inst_xor(cpu, read_next_byte(cpu)); break;
        // OR regs
        case 0xb0: case 0xb1: case 0xb2: case 0xb3: case 0xb4: case 0xb5: case 0xb7:
            cpu_inst_or(cpu, *cpu->regAddr[op & 0x7]);
            break;
        // OR (HL)/d8
        case 0xb6: cpu_inst_or(cpu, mmu_rb(cpu->registers.hl, 1)); break;
        case 0xf6: cpu_inst_or(cpu, read_next_byte(cpu)); break;
        // CP regs
		case 0xb8: case 0xb9: case 0xba: case 0xbb: case 0xbc: case 0xbd: case 0xbf:
			cpu_inst_cp(cpu, *cpu->regAddr[op & 0x7]);
			break;
		case 0xfe:
			cpu_inst_cp(cpu, read_next_byte(cpu));
			break; // CP d8

		// PUSH
		case 0xc5: cpu_inst_push(cpu, cpu->registers.bc); break;
		case 0xd5: cpu_inst_push(cpu, cpu->registers.de); break;
		case 0xe5: cpu_inst_push(cpu, cpu->registers.hl); break;
		case 0xf5: cpu_inst_push(cpu, cpu->registers.af); break;

		// JUMPS
        // Conditional JP a8
		case 0x20:
            cpu_inst_cond_jp_sign
                (cpu, !cpu->registers.flags.ze,
                (int8_t) read_next_byte(cpu));
            break; // NZ
        case 0x28:
            cpu_inst_cond_jp_sign
                (cpu, cpu->registers.flags.ze,
                (int8_t) read_next_byte(cpu));
            break; // Z
        case 0x30:
            cpu_inst_cond_jp_sign
                (cpu, !cpu->registers.flags.cy,
                (int8_t) read_next_byte(cpu));
            break; // NC
        case 0x38:
            cpu_inst_cond_jp_sign
                (cpu, cpu->registers.flags.cy,
                (int8_t) read_next_byte(cpu));
            break; // C

        // Conditional JP d16
        case 0xc2:
            cpu_inst_cond_jp_word
                (cpu, !cpu->registers.flags.ze,
                read_next_word(cpu));
            break;
        case 0xca:
            cpu_inst_cond_jp_word
                (cpu, cpu->registers.flags.ze,
                read_next_word(cpu));
            break;
        case 0xd2:
            cpu_inst_cond_jp_word
                (cpu, !cpu->registers.flags.cy,
                read_next_word(cpu));
            break;
        case 0xda:
            cpu_inst_cond_jp_word
                (cpu, cpu->registers.flags.cy,
                read_next_word(cpu));
            break;

        // Conditional CALL
        case 0xc4:
            cpu_inst_cond_call
                (cpu, !(cpu->registers.flags.ze),
                read_next_word(cpu));
            break;
        case 0xcc:
            cpu_inst_cond_call
                (cpu, (cpu->registers.flags.ze),
                read_next_word(cpu));
            break;
        case 0xd4:
            cpu_inst_cond_call
                (cpu, !(cpu->registers.flags.cy),
                read_next_word(cpu));
            break;
        case 0xdc:
            cpu_inst_cond_call
                (cpu, (cpu->registers.flags.cy),
                read_next_word(cpu));
            break;

        // Conditional RET
        case 0xc0:
            cpu_inst_cond_ret
                (cpu, !(cpu->registers.flags.ze));
            break;
        case 0xc8:
            cpu_inst_cond_ret
                (cpu, (cpu->registers.flags.ze));
            break;
        case 0xd0:
            cpu_inst_cond_ret
                (cpu, !(cpu->registers.flags.cy));
            break;
        case 0xd8:
            cpu_inst_cond_ret
                (cpu, (cpu->registers.flags.cy));
            break;

        case 0x18:
            cpu_inst_cond_jp_sign
                (cpu, true, read_next_byte(cpu));
            break; // JR r8
		case 0xc3:
            cpu->registers.pc = read_next_word(cpu);
            break; // JP d16

        // Pairs
        // DEC rp
        case 0x0b: cpu->registers.bc--; break;
        case 0x1b: cpu->registers.de--; break;
        case 0x2b: cpu->registers.hl--; break;
        case 0x3b: cpu->registers.sp--; break;
		// LD RP, d16
		case 0x01: cpu->registers.bc = read_next_word(cpu); break;
		case 0x11: cpu->registers.de = read_next_word(cpu); break;
		case 0x21: cpu->registers.hl = read_next_word(cpu); break;
        // LD rp/d16, A (except hl)
        case 0x02: mmu_wb(cpu->registers.bc, cpu->registers.a); break;
        case 0x12: mmu_wb(cpu->registers.de, cpu->registers.a);  break;
        case 0xea: mmu_wb(read_next_word(cpu), cpu->registers.a);  break;
		// LD r, hl
		case 0x46: case 0x4e: case 0x56: case 0x5e: case 0x66: case 0x6e: case 0x7e:
			*cpu->regAddr[(op & 0x38) >> 3] = mmu_rb(cpu->registers.hl, 1);
			break;
		// LD hl, r
		case 0x70: case 0x71: case 0x72: case 0x73: case 0x74: case 0x75: case 0x77:
            mmu_wb(cpu->registers.hl, *cpu->regAddr[op & 0x7]);
			break;

        // INC rp
        case 0x03: cpu->registers.bc++; break;
        case 0x13: cpu->registers.de++; break;
        case 0x23: cpu->registers.hl++; break;
        case 0x33: cpu->registers.sp++; break;
        // ADD HL, n
        case 0x09: cpu_inst_add_hl(cpu, cpu->registers.bc); break;
        case 0x19: cpu_inst_add_hl(cpu, cpu->registers.de); break;
        case 0x29: cpu_inst_add_hl(cpu, cpu->registers.hl); break;
        case 0x39: cpu_inst_add_hl(cpu, cpu->registers.sp); break;
        // POP rp
        case 0xc1: cpu->registers.bc = cpu_inst_pop(cpu); break;
        case 0xd1: cpu->registers.de = cpu_inst_pop(cpu); break;
        case 0xe1: cpu->registers.hl = cpu_inst_pop(cpu); break;
        case 0xf1: cpu->registers.af = cpu_inst_pop(cpu); break;

        // Rotates
        case 0x07:
            cpu->registers.a = cpu_inst_rlc(cpu, cpu->registers.a);
            break; // RLCA
        case 0x0f:
            cpu->registers.a = cpu_inst_rrc(cpu, cpu->registers.a);
            break; // RRCA
        case 0x17:
            cpu->registers.a = cpu_inst_rl(cpu, cpu->registers.a);
            break; // RLA
        case 0x1f:
            cpu->registers.a = cpu_inst_rr(cpu, cpu->registers.a);
            break; // RRA

        case 0x08:
        	{
            	uint16_t hold_src = read_next_word(cpu);
            	mmu_wb(hold_src, cpu->registers.sp & 0xff);
            	mmu_wb(hold_src + 1, cpu->registers.sp >> 8);
            }
            break; // LD (nn), SP
        case 0x0a: cpu->registers.a = mmu_rb(cpu->registers.bc, 1); break; // LD A, (BC)
        case 0x1a: cpu->registers.a = mmu_rb(cpu->registers.de, 1); break; // LD A, (DE)
        case 0x22:
            mmu_wb(cpu->registers.hl++, cpu->registers.a);
            break; // LD (HL+), A
        case 0x2a:
            cpu->registers.a = mmu_rb(cpu->registers.hl++, 1);
            break; // LD A, (HL+)
        case 0x31:
            cpu->registers.sp = read_next_word(cpu);
            break; // LD SP, d16
		case 0x32:
            mmu_wb(cpu->registers.hl--, cpu->registers.a);
			break; // LD (HL-), A
        case 0x34:
        	{
            	uint16_t hold_src = cpu->registers.hl;
            	mmu_wb(hold_src, cpu_inst_inc(cpu, mmu_rb(hold_src, 1)));
            }
            break; // INC (HL)
        case 0x36:
            mmu_wb(cpu->registers.hl, read_next_byte(cpu));
            break; // LD (HL), d8
        case 0x3a:
            cpu->registers.a = mmu_rb(cpu->registers.hl--, 1);
            break; // LD A, (HL-)
        case 0xcd:
            cpu_inst_call(cpu, read_next_word(cpu));;
            break; // CALL d16
        case 0xc9:
            cpu->registers.pc = cpu_inst_pop(cpu);
            break; // RET
		case 0xe0:
            mmu_wb((0xff00 | read_next_byte(cpu)), cpu->registers.a);
			break; // LDH (a8), A
        case 0xe2:
            mmu_wb(0xff00 | cpu->registers.c, cpu->registers.a);
            break; // LD (C), A
        case 0xe9:
            cpu->registers.pc = cpu->registers.hl;
            break; // JP HL
		case 0xf0:
            cpu->registers.a = mmu_rb(0xff00 | read_next_byte(cpu), 1);
			break; // LDH A, (a8)
        case 0xf2:
            cpu->registers.a = mmu_rb(0xff00 | cpu->registers.c, 1);
            break; // LD A, (C)
        case 0xf9:
            cpu->registers.sp = cpu->registers.hl;
            break; // LD SP, HL
        case 0xfa:
            cpu->registers.a = mmu_rb(read_next_word(cpu), 1);
            break; // LD A, a16

        // INTERRUPTS
        // case 0xfb: break; // EI

        /////   PREFIX CB   /////

        // PREFIX CB
        // RLC r
        case 0x100: case 0x101: case 0x102: case 0x103: case 0x104: case 0x105: case 0x107:
			{
				uint16_t hold_src = op;
            	*cpu->regAddr[hold_src] = cpu_inst_rlc(cpu, *cpu->regAddr[hold_src]);
            }
            break;
        // RL r
        case 0x110: case 0x111: case 0x112: case 0x113: case 0x114: case 0x115: case 0x117:
        	{
            	uint16_t hold_src = op & 0x7;
            	*cpu->regAddr[hold_src] = cpu_inst_rl(cpu, *cpu->regAddr[hold_src]);
            }
            break;
        // RRC r
        case 0x108: case 0x109: case 0x10a: case 0x10b: case 0x10c: case 0x10d: case 0x10f:
        	{
            	uint16_t hold_src = op & 0x7;
            	*cpu->regAddr[hold_src] = cpu_inst_rrc(cpu, *cpu->regAddr[hold_src]);
            }
            break;
        // RR r
        case 0x118: case 0x119: case 0x11a: case 0x11b: case 0x11c: case 0x11d: case 0x11f:
        	{
            	uint16_t hold_src = op & 0x7;
            	*cpu->regAddr[hold_src] = cpu_inst_rr(cpu, *cpu->regAddr[hold_src]);
            }
            break;
        // BIT b, r
		case 0x140: case 0x141: case 0x142: case 0x143: case 0x144: case 0x145: case 0x147:
		case 0x148: case 0x149: case 0x14a: case 0x14b: case 0x14c: case 0x14d: case 0x14f:
		case 0x150: case 0x151: case 0x152: case 0x153: case 0x154: case 0x155: case 0x157:
		case 0x158: case 0x159: case 0x15a: case 0x15b: case 0x15c: case 0x15d: case 0x15f:
		case 0x160: case 0x161: case 0x162: case 0x163: case 0x164: case 0x165: case 0x167:
		case 0x168: case 0x169: case 0x16a: case 0x16b: case 0x16c: case 0x16d: case 0x16f:
        case 0x170: case 0x171: case 0x172: case 0x173: case 0x174: case 0x175: case 0x177:
		case 0x178: case 0x179: case 0x17a: case 0x17b: case 0x17c: case 0x17d: case 0x17f:
            cpu_inst_bit(cpu, *cpu->regAddr[op & 0x7], (op & 0x38) >> 3);
            break;
        // RES b, r
		case 0x180: case 0x181: case 0x182: case 0x183: case 0x184: case 0x185: case 0x187:
		case 0x188: case 0x189: case 0x18a: case 0x18b: case 0x18c: case 0x18d: case 0x18f:
		case 0x190: case 0x191: case 0x192: case 0x193: case 0x194: case 0x195: case 0x197:
		case 0x198: case 0x199: case 0x19a: case 0x19b: case 0x19c: case 0x19d: case 0x19f:
		case 0x1a0: case 0x1a1: case 0x1a2: case 0x1a3: case 0x1a4: case 0x1a5: case 0x1a7:
		case 0x1a8: case 0x1a9: case 0x1aa: case 0x1ab: case 0x1ac: case 0x1ad: case 0x1af:
        case 0x1b0: case 0x1b1: case 0x1b2: case 0x1b3: case 0x1b4: case 0x1b5: case 0x1b7:
		case 0x1b8: case 0x1b9: case 0x1ba: case 0x1bb: case 0x1bc: case 0x1bd: case 0x1bf:
            cpu_inst_res(cpu->regAddr[op & 0x7], (op & 0x38) >> 3);
            break;
        // SET b, r
		case 0x1c0: case 0x1c1: case 0x1c2: case 0x1c3: case 0x1c4: case 0x1c5: case 0x1c7:
		case 0x1c8: case 0x1c9: case 0x1ca: case 0x1cb: case 0x1cc: case 0x1cd: case 0x1cf:
		case 0x1d0: case 0x1d1: case 0x1d2: case 0x1d3: case 0x1d4: case 0x1d5: case 0x1d7:
		case 0x1d8: case 0x1d9: case 0x1da: case 0x1db: case 0x1dc: case 0x1dd: case 0x1df:
		case 0x1e0: case 0x1e1: case 0x1e2: case 0x1e3: case 0x1e4: case 0x1e5: case 0x1e7:
		case 0x1e8: case 0x1e9: case 0x1ea: case 0x1eb: case 0x1ec: case 0x1ed: case 0x1ef:
        case 0x1f0: case 0x1f1: case 0x1f2: case 0x1f3: case 0x1f4: case 0x1f5: case 0x1f7:
		case 0x1f8: case 0x1f9: case 0x1fa: case 0x1fb: case 0x1fc: case 0x1fd: case 0x1ff:
            cpu_inst_set(cpu->regAddr[op & 0x7], (op & 0x38) >> 3);
            break;
        // (HL) MMU
        case 0x106:
        	{
            	uint16_t hold_src = cpu->registers.hl;
            	mmu_wb(hold_src, cpu_inst_rlc(cpu, mmu_rb(hold_src, 1)));
            }
            break; // rlc
        case 0x116:
        	{
            	uint16_t hold_src = cpu->registers.hl;
            	mmu_wb(hold_src, cpu_inst_rl(cpu, mmu_rb(hold_src, 1)));
            }
            break; // rl
        case 0x10e:
        	{
            	uint16_t hold_src = cpu->registers.hl;
            	mmu_wb(hold_src, cpu_inst_rrc(cpu, mmu_rb(hold_src, 1)));
            }
            break; // rrc
        case 0x11e:
        	{
            	uint16_t hold_src = cpu->registers.hl;
            	mmu_wb(hold_src, cpu_inst_rr(cpu, mmu_rb(hold_src, 1)));
            }
            break; // rr

		default:
			printf("CPU: Opcode %02x not implemented!\n", op);
			halt = 1;
			return;
	}

    // calculate cycles
    cpu->clock.cyc += CPU_CYCLE[op] + cpu->clock.cur_cyc;
    cpu->clock.mem += cpu->clock.cur_mem;
}

void cpu_init(CPU *cpu) {

	cpu->clock.cur_cyc = 0;
	cpu->clock.cur_mem = 0;
	cpu->clock.cyc     = 0;
	cpu->clock.mem     = 0;

	cpu->registers.b = 0;
	cpu->registers.c = 0;
	cpu->registers.d = 0;
	cpu->registers.e = 0;
	cpu->registers.h = 0;
	cpu->registers.l = 0;
	cpu->registers.a = 0;

	// use advantage to per-register based instruction memory
	cpu->regAddr[REG_B] = &cpu->registers.b;
	cpu->regAddr[REG_C] = &cpu->registers.c;
	cpu->regAddr[REG_D] = &cpu->registers.d;
	cpu->regAddr[REG_E] = &cpu->registers.e;
	cpu->regAddr[REG_H] = &cpu->registers.h;
	cpu->regAddr[REG_L] = &cpu->registers.l;
	cpu->regAddr[REG_A] = &cpu->registers.a;

	cpu->registers.pc = 0;
	cpu->registers.sp = 0;

	cpu->registers.flags.ze = 0;
	cpu->registers.flags.ne = 0;
	cpu->registers.flags.hf = 0;
	cpu->registers.flags.cy = 0;
}

// Project started: 16/02/2020
