#include "main.h"
#include "cpu.h"
#include "mmu.c"

static inline void cpu_inst_push(CPU *a1, const uint16_t a2);

uint8_t holdByte;
uint16_t holdWord;
uint16_t holdSrc;

// bool halt;

// d8/d16 reads
static inline uint16_t read_next_word(CPU *cpu) {
    return mmu_read_byte(cpu->registers.pc++) | (mmu_read_byte(cpu->registers.pc++) << 8);
}
static inline uint8_t read_next_byte(CPU *cpu) {
	return mmu_read_byte(cpu->registers.pc++);
}

// Instructions
static inline uint16_t get_mask(const size_t size, const int bitNum) {
	return (1 << ((size * 8) - bitNum)) - 1;
}
static inline void cpu_inst_add(CPU* cpu, const uint8_t val, const bool cy) {
    holdWord = cpu->registers.a + val + cy;
    cpu->registers.flags.ze = !(holdWord);
    cpu->registers.flags.ne = 0;
    cpu->registers.flags.hf = (uint8_t) ((val & 0xf) + (cpu->registers.a & 0xf) + cy) > 0xf;
    cpu->registers.flags.cy = holdWord > 0xff;

    cpu->registers.a = holdWord;
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
	cpu_inst_push(cpu, addr + 3);
	cpu->registers.pc = addr;
}
static inline void cpu_inst_cp(CPU *cpu, const uint8_t val) {
	cpu->registers.flags.ze = (*cpu->regAddr[REG_A] == val); // same value subtraction = 0
	cpu->registers.flags.ne = 1;
	cpu->registers.flags.hf = (*cpu->regAddr[REG_A] & 0xf) > (val & 0xf);
    cpu->registers.flags.cy = cpu->registers.a < val;
}
static inline void cpu_inst_dec(CPU* cpu, uint8_t* reg) {
	holdByte = *reg - 1;
	cpu->registers.flags.ze = !(holdByte);
	cpu->registers.flags.ne = 1;
	cpu->registers.flags.hf = (uint8_t) ((*reg & 0xf) - 1) > 0xf;

    (*reg)--;
}
static inline uint8_t cpu_inst_inc(CPU* cpu, const uint8_t val) {
    holdByte = val + 1;
    cpu->registers.flags.ze = !(holdByte);
    cpu->registers.flags.ne = 0;
    cpu->registers.flags.hf = (uint8_t) ((val & 0xf) + 1) > 0xf;
    return holdByte;
}
static inline void cpu_inst_or(CPU* cpu, const uint8_t val) {
    cpu->registers.a |= val;
    cpu->registers.flags.ze = !(cpu->registers.a);
    cpu->registers.flags.ne = cpu->registers.flags.hf = cpu->registers.flags.cy = 0;
}
static inline uint16_t cpu_inst_pop(CPU *cpu) {
    return mmu_read_byte(cpu->registers.sp++) | (mmu_read_byte(cpu->registers.sp++) << 8);
}
static inline void cpu_inst_push(CPU *cpu, const uint16_t val) {
    mmu_write_byte(--cpu->registers.sp, val >> 8);
    mmu_write_byte(--cpu->registers.sp, val);

   // cpu->registers.sp--; // decrement sp location
}
// diff between rotates
// through carry: flip bit (pos 0 or 7) THROUGH old carry, gets carry on old state
// THROUGH being the path of rotate includes carry flag
// normal: gets carry on old state pos (0 or 7) then rotates (basically wrap content)
static inline uint8_t cpu_inst_rl(CPU* cpu, uint8_t val) {
    holdByte = cpu->registers.flags.cy;
    cpu->registers.flags.ne = cpu->registers.flags.hf = 0;
    cpu->registers.flags.cy = val >> 7;
    val = (val << 1) | holdByte;                        // !
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
    holdByte = cpu->registers.flags.cy;
    cpu->registers.flags.ne = cpu->registers.flags.hf = 0;
    cpu->registers.flags.cy = val & 0x1;
    val = (val >> 1) | (holdByte << 7);                // !
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
static inline void cpu_inst_rst(CPU* cpu, const uint16_t pc, const int val) {
	cpu_inst_push(cpu, pc);
	cpu->registers.pc = val;
}
// book says set if "no borrow"
static inline void cpu_inst_sub(CPU* cpu, const uint8_t val, const bool cy) {
    holdWord = cpu->registers.a - (val + cy);
    cpu->registers.flags.ze = !(holdWord);
    cpu->registers.flags.ne = 1;
    cpu->registers.flags.hf = !((uint8_t) ((cpu->registers.a & 0xf) - ((val & 0xf) + cy)) > 0xf);
    cpu->registers.flags.cy = !(holdWord > 0xff);
    cpu->registers.a = holdWord;
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
static inline void cpu_inst_cond_jp(CPU* cpu, const bool cond, const uint16_t pc, const int8_t data) {
	if (cond) cpu->registers.pc = (pc + data);
}
static inline void cpu_inst_cond_ret(CPU* cpu, const bool cond, const uint16_t pc) {
    if (cond) cpu->registers.pc = cpu_inst_pop(cpu);
}

// prefix cb
static inline void cpu_inst_bit(CPU* cpu, const uint8_t regval, const uint8_t pos) {
    cpu->registers.flags.ze = ((regval & (1 << pos)) == 0);
    cpu->registers.flags.ne = 0;
    cpu->registers.flags.hf = 1;
    // cy not affected
}
static inline void cpu_inst_res(CPU* cpu, uint8_t* reg, const uint8_t pos) {
    *reg = *reg & (uint8_t) ~(1 << pos);
}
static inline void cpu_inst_set(CPU* cpu, uint8_t* reg, const uint8_t pos) {
    *reg = *reg | (1 << pos);
}
static inline int cpu_prefix_cb(CPU *cpu, const uint8_t op_cb) {
    switch(op_cb) 
    {
        // ROTATES
        // RLC r
        case 0x00: case 0x01: case 0x02: case 0x03: case 0x04: case 0x05: case 0x07:
            holdSrc = op_cb;
            *cpu->regAddr[holdSrc] = cpu_inst_rlc(cpu, *cpu->regAddr[holdSrc]);
            break;
        // RL r
        case 0x10: case 0x11: case 0x12: case 0x13: case 0x14: case 0x15: case 0x17:
            holdSrc = op_cb & 0x7;
            *cpu->regAddr[holdSrc] = cpu_inst_rl(cpu, *cpu->regAddr[holdSrc]);
            break;
        // RRC r
        case 0x08: case 0x09: case 0x0a: case 0x0b: case 0x0c: case 0x0d: case 0x0f:
            holdSrc = op_cb & 0x7;
            *cpu->regAddr[holdSrc] = cpu_inst_rrc(cpu, *cpu->regAddr[holdSrc]);
            break;
        // RR r
        case 0x18: case 0x19: case 0x1a: case 0x1b: case 0x1c: case 0x1d: case 0x1f:
            holdSrc = op_cb & 0x7;
            *cpu->regAddr[holdSrc] = cpu_inst_rr(cpu, *cpu->regAddr[holdSrc]);
            break;
        // BIT b, r 
		case 0x40: case 0x41: case 0x42: case 0x43: case 0x44: case 0x45: case 0x47:
		case 0x48: case 0x49: case 0x4a: case 0x4b: case 0x4c: case 0x4d: case 0x4f:
		case 0x50: case 0x51: case 0x52: case 0x53: case 0x54: case 0x55: case 0x57:
		case 0x58: case 0x59: case 0x5a: case 0x5b: case 0x5c: case 0x5d: case 0x5f:
		case 0x60: case 0x61: case 0x62: case 0x63: case 0x64: case 0x65: case 0x67:
		case 0x68: case 0x69: case 0x6a: case 0x6b: case 0x6c: case 0x6d: case 0x6f:
        case 0x70: case 0x71: case 0x72: case 0x73: case 0x74: case 0x75: case 0x77:
		case 0x78: case 0x79: case 0x7a: case 0x7b: case 0x7c: case 0x7d: case 0x7f:
            cpu_inst_bit(cpu, *cpu->regAddr[op_cb & 0x7], (op_cb & 0x38) >> 3);
            break;
        // RES b, r 
		case 0x80: case 0x81: case 0x82: case 0x83: case 0x84: case 0x85: case 0x87:
		case 0x88: case 0x89: case 0x8a: case 0x8b: case 0x8c: case 0x8d: case 0x8f:
		case 0x90: case 0x91: case 0x92: case 0x93: case 0x94: case 0x95: case 0x97:
		case 0x98: case 0x99: case 0x9a: case 0x9b: case 0x9c: case 0x9d: case 0x9f:
		case 0xa0: case 0xa1: case 0xa2: case 0xa3: case 0xa4: case 0xa5: case 0xa7:
		case 0xa8: case 0xa9: case 0xaa: case 0xab: case 0xac: case 0xad: case 0xaf:
        case 0xb0: case 0xb1: case 0xb2: case 0xb3: case 0xb4: case 0xb5: case 0xb7:
		case 0xb8: case 0xb9: case 0xba: case 0xbb: case 0xbc: case 0xbd: case 0xbf:
            cpu_inst_res(cpu, cpu->regAddr[op_cb & 0x7], (op_cb & 0x38) >> 3);
            break;
        // SET b, r
		case 0xc0: case 0xc1: case 0xc2: case 0xc3: case 0xc4: case 0xc5: case 0xc7:
		case 0xc8: case 0xc9: case 0xca: case 0xcb: case 0xcc: case 0xcd: case 0xcf:
		case 0xd0: case 0xd1: case 0xd2: case 0xd3: case 0xd4: case 0xd5: case 0xd7:
		case 0xd8: case 0xd9: case 0xda: case 0xdb: case 0xdc: case 0xdd: case 0xdf:
		case 0xe0: case 0xe1: case 0xe2: case 0xe3: case 0xe4: case 0xe5: case 0xe7:
		case 0xe8: case 0xe9: case 0xea: case 0xeb: case 0xec: case 0xed: case 0xef:
        case 0xf0: case 0xf1: case 0xf2: case 0xf3: case 0xf4: case 0xf5: case 0xf7:
		case 0xf8: case 0xf9: case 0xfa: case 0xfb: case 0xfc: case 0xfd: case 0xff:
            cpu_inst_set(cpu, cpu->regAddr[op_cb & 0x7], (op_cb & 0x38) >> 3);
            break;
        default:
			printf("PREFIX CB: Opcode %02x not implemented!\n", op_cb);
            halt = 1;
            return -1;

        // (HL) MMU
        case 0x06:
            holdSrc = cpu->registers.hl;
            mmu_write_byte(holdSrc, cpu_inst_rlc(cpu, mmu_read_byte(holdSrc)));
            break;// rlc
        case 0x16:
            holdSrc = cpu->registers.hl;
            mmu_write_byte(holdSrc, cpu_inst_rl(cpu, mmu_read_byte(holdSrc)));
            break;// rl
        case 0x0e:
            holdSrc = cpu->registers.hl;
            mmu_write_byte(holdSrc, cpu_inst_rrc(cpu, mmu_read_byte(holdSrc)));
            break;// rrc
        case 0x1e:
            holdSrc = cpu->registers.hl;
            mmu_write_byte(holdSrc, cpu_inst_rr(cpu, mmu_read_byte(holdSrc)));
            break;// rr
    }

    return 0; // stub
}

// return its cycle value
static inline int cpu_exec(CPU *cpu) {
	uint8_t op = memory[cpu->registers.pc]; // is it need to pass through mmu?
    uint16_t current_pc = cpu->registers.pc; // for jumps
	cpu->registers.pc++;

	switch(op) 
	{
        // PIPE
        case 0xcb:
            cpu_prefix_cb(cpu, read_next_byte(cpu));
            break; // PREFIX CB

		// RST
		case 0xc7: cpu_inst_rst(cpu, current_pc, 0x00); break;
		case 0xcf: cpu_inst_rst(cpu, current_pc, 0x08); break;
		case 0xd7: cpu_inst_rst(cpu, current_pc, 0x10); break;
		case 0xdf: cpu_inst_rst(cpu, current_pc, 0x18); break;
		case 0xe7: cpu_inst_rst(cpu, current_pc, 0x20); break;
		case 0xef: cpu_inst_rst(cpu, current_pc, 0x28); break;
		case 0xf7: cpu_inst_rst(cpu, current_pc, 0x30); break;
		case 0xff: cpu_inst_rst(cpu, current_pc, 0x38); break;

		// NOP
		case 0x00:
			break;
		// Disabled — should not modify anything in cpu
		case 0xd3: case 0xdb: case 0xdd: case 0xe3: case 0xe4: case 0xeb: case 0xec:
		case 0xed: case 0xf4: case 0xfc: case 0xfd:
			return 0;
		// Interrupts
		//case 0xf3: swd = 1; break; // DI
		//case 0xfb: swe = 1; break; // EI

        // INC regs
        case 0x04: case 0x0c: case 0x14: case 0x1c: case 0x24: case 0x2c: case 0x3c:
            holdSrc = (op & 0x38) >> 3;
            *cpu->regAddr[holdSrc] = cpu_inst_inc(cpu, *cpu->regAddr[holdSrc]);
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
            cpu_inst_add(cpu, mmu_read_byte(cpu->registers.hl), 0);
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
            cpu_inst_add(cpu, mmu_read_byte(cpu->registers.hl), cpu->registers.flags.cy);
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
            cpu_inst_sub(cpu, mmu_read_byte(cpu->registers.hl), 0);
            break; // SUB (HL)
        case 0x9e:
            cpu_inst_sub(cpu, mmu_read_byte(cpu->registers.hl), cpu->registers.flags.cy);
            break; // SBC (HL)
        // AND regs
        case 0xa0: case 0xa1: case 0xa2: case 0xa3: case 0xa4: case 0xa5: case 0xa7:
            cpu_inst_and(cpu, *cpu->regAddr[op & 0x7]);
            break;
        // AND (HL)/d16
        case 0xa6:
            cpu_inst_and(cpu, mmu_read_byte(cpu->registers.hl));
            break;
        case 0xe6:
            cpu_inst_and(cpu, read_next_byte(cpu));
            break;
		// XOR
		case 0xa8: case 0xa9: case 0xaa: case 0xab: case 0xac: case 0xad: case 0xaf:
			cpu_inst_xor(cpu, *cpu->regAddr[op & 0x7]);
			break;
        // XOR (HL)/d8
        case 0xae: cpu_inst_xor(cpu, mmu_read_byte(cpu->registers.hl)); break;
        case 0xee: cpu_inst_xor(cpu, read_next_byte(cpu)); break;
        // OR regs
        case 0xb0: case 0xb1: case 0xb2: case 0xb3: case 0xb4: case 0xb5: case 0xb7:
            cpu_inst_or(cpu, *cpu->regAddr[op & 0x7]);
            break;
        // OR (HL)/d8
        case 0xb6: cpu_inst_or(cpu, mmu_read_byte(cpu->registers.hl)); break;
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
        // Conditional JP
		case 0x20:
            cpu_inst_cond_jp(cpu, !cpu->registers.flags.ze, current_pc, (int8_t) read_next_byte(cpu));
            break; // NZ
        case 0x28:
            cpu_inst_cond_jp(cpu, cpu->registers.flags.ze,  current_pc, (int8_t) read_next_byte(cpu));
            break; // Z
        case 0x30:
            cpu_inst_cond_jp(cpu, !cpu->registers.flags.cy, current_pc, (int8_t) read_next_byte(cpu));
            break; // NC
        case 0x38:
            cpu_inst_cond_jp(cpu, cpu->registers.flags.cy,  current_pc, (int8_t) read_next_byte(cpu));
            break; // C

        // Conditional CALL
        case 0xc4:
            cpu_inst_cond_call(cpu, !(cpu->registers.flags.ze), read_next_word(cpu));
            break;
        case 0xcc:
            cpu_inst_cond_call(cpu, (cpu->registers.flags.ze), read_next_word(cpu));
            break;
        case 0xd4:
            cpu_inst_cond_call(cpu, !(cpu->registers.flags.cy), read_next_word(cpu));
            break;
        case 0xdc:
            cpu_inst_cond_call(cpu, (cpu->registers.flags.cy), read_next_word(cpu));
            break;

        // Conditional RET
        case 0xc0:
            cpu_inst_cond_ret(cpu, !(cpu->registers.flags.ze), current_pc);
            break;
        case 0xc8:
            cpu_inst_cond_ret(cpu, (cpu->registers.flags.ze), current_pc);
            break;
        case 0xd0:
            cpu_inst_cond_ret(cpu, !(cpu->registers.flags.cy), current_pc);
            break;
        case 0xd8:
            cpu_inst_cond_ret(cpu, (cpu->registers.flags.cy), current_pc);
            break;

        case 0x18:
            cpu_inst_cond_jp(cpu, true, read_next_byte(cpu), current_pc);
            break; // JR r8
		case 0xc3: cpu->registers.pc = read_next_word(cpu); break; // JP d16

        // Pairs
		// LD RP, d16
		case 0x01: cpu->registers.bc = read_next_word(cpu); break;
		case 0x11: cpu->registers.de = read_next_word(cpu); break;
		case 0x21: cpu->registers.hl = read_next_word(cpu); break;
        // LD rp/d16, A (except hl)
        case 0x02: mmu_write_byte(cpu->registers.bc, cpu->registers.a); break;
        case 0x12: mmu_write_byte(cpu->registers.de, cpu->registers.a);  break;
        case 0xea: mmu_write_byte(read_next_word(cpu), cpu->registers.a);  break;
		// LD r, hl
		case 0x46: case 0x4e: case 0x56: case 0x5e: case 0x66: case 0x6e: case 0x7e:
			*cpu->regAddr[(op & 0x38) >> 3] = mmu_read_byte(cpu->registers.hl);
			break;	
		// LD hl, r
		case 0x70: case 0x71: case 0x72: case 0x73: case 0x74: case 0x75: case 0x77:
            mmu_write_byte(cpu->registers.hl, *cpu->regAddr[op & 0x7]);
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
            holdSrc = read_next_word(cpu);
            mmu_write_byte(holdSrc, cpu->registers.sp & 0xff);
            mmu_write_byte(holdSrc + 1, cpu->registers.sp >> 8);
            break; // LD (nn), SP
        case 0x0a: cpu->registers.a = mmu_read_byte(cpu->registers.bc); break; // LD A, (BC)
        case 0x1a: cpu->registers.a = mmu_read_byte(cpu->registers.de); break; // LD A, (DE)
        case 0x22: mmu_write_byte(cpu->registers.hl++, cpu->registers.a); break; // LD (HL+), A
        case 0x2a:
            cpu->registers.a = mmu_read_byte(cpu->registers.hl++);
            break; // LD A, (HL+)
        case 0x31:
            cpu->registers.sp = read_next_word(cpu);
            break; // LD SP, d16
		case 0x32:
            mmu_write_byte(cpu->registers.hl--, cpu->registers.a);
			break; // LD (HL-), A
        case 0x34:
            holdSrc = cpu->registers.hl;
            mmu_write_byte(holdSrc, cpu_inst_inc(cpu, mmu_read_byte(holdSrc)));
            break; // INC (HL)
        case 0x36:
            mmu_write_byte(cpu->registers.hl, read_next_byte(cpu));
            break; // LD (HL), d8
        case 0x3a:
            cpu->registers.a = mmu_read_byte(cpu->registers.hl--);
            break; // LD A, (HL-)

        case 0xcd:
            cpu_inst_push(cpu, cpu->registers.pc);
            cpu->registers.pc = read_next_word(cpu);
            break; // CALL d16
        case 0xc9:
            cpu->registers.pc = cpu_inst_pop(cpu);
            break; // RET
		case 0xe0:
            mmu_write_byte((0xff00 | read_next_byte(cpu)), cpu->registers.a);
			break; // LDH (a8), A
        case 0xe2:
            mmu_write_byte(0xff00 | cpu->registers.c, cpu->registers.a);
            break; // LD (C), A
        case 0xe9:
            cpu->registers.pc = cpu->registers.hl;
            break; // JP HL
		case 0xf0:
            cpu->registers.a = mmu_read_byte(0xff00 | read_next_byte(cpu));
			break; // LDH A, (a8)
        case 0xf2:
            cpu->registers.a = mmu_read_byte(0xff00 | cpu->registers.c);
            break; // LD A, (C)
        case 0xf9:
            cpu->registers.sp = cpu->registers.hl;
            break; // LD SP, HL
        case 0xfa:
            cpu->registers.a = mmu_read_byte(read_next_word(cpu));
            break; // LD A, d16
		default:
			printf("Opcode %02x not implemented!\n", op);
			halt = 1;
			return -1;
	}
	
    return 0; // stub: cycle
}

static void cpu_regs_init(CPU *cpu) {

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
