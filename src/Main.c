#include "includes/Main.h"
#include "Disassembler.c"

CPU z80;
CPU *cpu = &z80;

bool PRINT_LESS = 1;
static inline void cpu_inst_push(CPU *a1, const uint16_t a2);

uint8_t *memory;
size_t file_size;
size_t memory_size; // store memory size
uint8_t holdByte;
uint16_t holdWord;

bool active;

static inline uint16_t read_next_word(CPU *cpu) {
	return memory[cpu->registers.pc++] | (memory[cpu->registers.pc++] << 8);
}
static inline uint8_t read_next_byte(CPU *cpu) {
	return memory[cpu->registers.pc++];
}

// Flags

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
	cpu->registers.flags.ze = (*cpu->reg[REG_A] == val); // same value subtraction = 0
	cpu->registers.flags.ne = 1;
	cpu->registers.flags.hf = (*cpu->reg[REG_A] & 0xf) >= (val & 0xf);
    // cy not affected
}
static inline void cpu_inst_dec(CPU* cpu, uint8_t* reg) {
	holdByte = *reg - 1;
	cpu->registers.flags.ze = !(holdByte);
	cpu->registers.flags.ne = 1;
	cpu->registers.flags.hf = (uint8_t) ((*reg & 0xf) - 1) > 0xf;

    (*reg)--;
}
static inline void cpu_inst_inc(CPU* cpu, uint8_t* reg) {
    holdByte = *reg + 1;
    cpu->registers.flags.ze = !(holdByte);
    cpu->registers.flags.ne = 0;
    cpu->registers.flags.hf = (uint8_t) ((*reg & 0xf) + 1) > 0xf;
    (*reg)++;
}
static inline void cpu_inst_or(CPU* cpu, const uint8_t val) {
    cpu->registers.a |= val;
    cpu->registers.flags.ze = !(cpu->registers.a);
    cpu->registers.flags.ne = cpu->registers.flags.hf = cpu->registers.flags.cy = 0;
}
static inline uint16_t cpu_inst_pop(CPU *cpu) {
    return (memory[cpu->registers.sp++] | (memory[cpu->registers.sp++] << 8));
}
static inline void cpu_inst_push(CPU *cpu, const uint16_t val) {
	memory[--cpu->registers.sp] = val >> 8;
	memory[--cpu->registers.sp] = val;
}
static inline void cpu_inst_rlca(CPU* cpu) {
    cpu->registers.flags.ne = cpu->registers.flags.hf = 0;
    cpu->registers.flags.cy = (cpu->registers.a >> 7);
    cpu->registers.a = (cpu->registers.a << 1) | cpu->registers.flags.cy;
    cpu->registers.flags.ze = !(cpu->registers.a);
}
static inline void cpu_inst_rst(CPU* cpu, const int val) {
	cpu_inst_push(cpu, cpu->registers.pc);
	cpu->registers.pc = val;
}
static inline void cpu_inst_xor(CPU* cpu, uint8_t val) {
	*cpu->reg[REG_A] ^= val;
	cpu->registers.flags.ze = !(*cpu->reg[REG_A]);
	cpu->registers.flags.ne = cpu->registers.flags.hf = cpu->registers.flags.cy = 0; 
}
// conditional jumps
static inline void cpu_inst_cond_call(CPU* cpu, const bool cond, const uint16_t addr) {
    if (cond) cpu_inst_call(cpu, addr);
}
static inline void cpu_inst_cond_jp(CPU* cpu, const bool cond, const int8_t data) {
	if (cond) cpu->registers.pc = (cpu->registers.pc + data);
}
static inline void cpu_inst_cond_ret(CPU* cpu, const bool cond) {
    if (cond) cpu->registers.pc = cpu_inst_pop(cpu);
}
// interrupts
int pass = 0;
bool interrupt, swd, swe;
static inline void service_interrupt(CPU* cpu, bool *sw, const bool val) {
	if (!sw) return;
	if (++pass >= 2) {
		interrupt = val;
		pass = 0;
		*sw = 0;
	}
}
// return its cycle value
static inline int cpu_exec(CPU *cpu) {
	uint8_t op = memory[cpu->registers.pc];
	cpu->registers.pc++;

	switch(op) 
	{
        // UNTESTED INSTS.
		// RST
		case 0xc7: cpu_inst_rst(cpu, 0x00); break;
		case 0xcf: cpu_inst_rst(cpu, 0x08); break;
		case 0xd7: cpu_inst_rst(cpu, 0x10); break;
		case 0xdf: cpu_inst_rst(cpu, 0x18); break;
		case 0xe7: cpu_inst_rst(cpu, 0x20); break;
		case 0xef: cpu_inst_rst(cpu, 0x28); break;
		case 0xf7: cpu_inst_rst(cpu, 0x30); break;
		case 0xff: cpu_inst_rst(cpu, 0x38); break;

		// NOP
		case 0x00:
			break;
		// Disabled — should not modify anything in cpu
		case 0xd3: case 0xdb: case 0xdd: case 0xe3: case 0xe4: case 0xeb: case 0xec:
		case 0xed: case 0xf4: case 0xfc: case 0xfd:
			return 0;
		// Interrupts
		case 0xf3: swd = 1; break; // DI
		case 0xfb: swe = 1; break; // EI

        // INC regs
        case 0x04: case 0x0c: case 0x14: case 0x1c: case 0x24: case 0x2c: case 0x3c:
            cpu_inst_inc(cpu, cpu->reg[(op & 0x38) >> 3]);
            break;
		// DEC regs
		case 0x05: case 0x0d: case 0x15: case 0x1d: case 0x25: case 0x2d: case 0x3d:
			cpu_inst_dec(cpu, cpu->reg[(op & 0x38) >> 3]);
			break;
		// LD regs, d8
		case 0x06: case 0x0e: case 0x16: case 0x1e: case 0x26: case 0x2e: case 0x3e:
			(*cpu->reg[(op & 0x38) >> 3]) = read_next_byte(cpu);
			break;
		// LD reg, reg
		case 0x40: case 0x41: case 0x42: case 0x43: case 0x44: case 0x45: case 0x47:
		case 0x48: case 0x49: case 0x4a: case 0x4b: case 0x4c: case 0x4d: case 0x4f:
		case 0x50: case 0x51: case 0x52: case 0x53: case 0x54: case 0x55: case 0x57:
		case 0x58: case 0x59: case 0x5a: case 0x5b: case 0x5c: case 0x5d: case 0x5f:
		case 0x60: case 0x61: case 0x62: case 0x63: case 0x64: case 0x65: case 0x67:
		case 0x68: case 0x69: case 0x6a: case 0x6b: case 0x6c: case 0x6d: case 0x6f:
		case 0x78: case 0x79: case 0x7a: case 0x7b: case 0x7c: case 0x7d: case 0x7f:
			*cpu->reg[(op & 0x38) >> 3] = *cpu->reg[op & 0x7];
			break;
        // ADD regs
        case 0x80: case 0x81: case 0x82: case 0x83: case 0x84: case 0x85: case 0x87:
            cpu_inst_add(cpu, *cpu->reg[op & 0x7], 0);
            break;
        // ADD (HL)/d8
        case 0x86:
            cpu_inst_add(cpu, memory[cpu->registers.hl], 0);
            break;
        case 0xc6:
            cpu_inst_add(cpu, read_next_byte(cpu), 0);
            break;
        // ADC regs
        case 0x88: case 0x89: case 0x8a: case 0x8b: case 0x8c: case 0x8d: case 0x8f:
            cpu_inst_add(cpu, *cpu->reg[op & 0x7], cpu->registers.flags.cy);
            break;
        // ADC (HL)
        case 0x8e:
            cpu_inst_add(cpu, memory[cpu->registers.hl], cpu->registers.flags.cy);
            break;
        // AND regs
        case 0xa0: case 0xa1: case 0xa2: case 0xa3: case 0xa4: case 0xa5: case 0xa7:
            cpu_inst_and(cpu, *cpu->reg[op & 0x7]);
            break;
        // AND (HL)/d16
        case 0xa6:
            cpu_inst_and(cpu, memory[cpu->registers.hl]);
            break;
        case 0xe6:
            cpu_inst_and(cpu, read_next_byte(cpu));
            break;
		// XOR
		case 0xa8: case 0xa9: case 0xaa: case 0xab: case 0xac: case 0xad: case 0xaf:
			cpu_inst_xor(cpu, *cpu->reg[op & 0x7]);
			break;
        // XOR (HL)/d8
        case 0xae: cpu_inst_xor(cpu, memory[cpu->registers.hl]); break;
        case 0xee: cpu_inst_xor(cpu, read_next_byte(cpu)); break;
        // OR regs
        case 0xb0: case 0xb1: case 0xb2: case 0xb3: case 0xb4: case 0xb5: case 0xb7:
            cpu_inst_or(cpu, *cpu->reg[op & 0x7]);
            break;
        // OR (HL)/d8
        case 0xb6: cpu_inst_or(cpu, memory[cpu->registers.hl]); break;
        case 0xf6: cpu_inst_or(cpu, read_next_byte(cpu)); break;
        // CP regs
		case 0xb8: case 0xb9: case 0xba: case 0xbb: case 0xbc: case 0xbd: case 0xbf:
			cpu_inst_cp(cpu, *cpu->reg[op & 0x7]);
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
            cpu_inst_cond_jp(cpu, !cpu->registers.flags.ze, (int8_t) read_next_byte(cpu));
            break; // NZ
        case 0x28:
            cpu_inst_cond_jp(cpu, cpu->registers.flags.ze,  (int8_t) read_next_byte(cpu));
            break; // Z
        case 0x30:
            cpu_inst_cond_jp(cpu, !cpu->registers.flags.cy, (int8_t) read_next_byte(cpu));
            break; // NC
        case 0x38:
            cpu_inst_cond_jp(cpu, cpu->registers.flags.cy,  (int8_t) read_next_byte(cpu));
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
            cpu_inst_cond_ret(cpu, !(cpu->registers.flags.ze));
            break;
        case 0xc8:
            cpu_inst_cond_ret(cpu, (cpu->registers.flags.ze));
            break;
        case 0xd0:
            cpu_inst_cond_ret(cpu, !(cpu->registers.flags.cy));
            break;
        case 0xd8:
            cpu_inst_cond_ret(cpu, (cpu->registers.flags.cy));
            break;

        case 0x18:
            cpu_inst_cond_jp(cpu, true, read_next_byte(cpu));
            break; // JR r8
		case 0xc3: cpu->registers.pc = read_next_word(cpu); break; // JP d16

        // Pairs
		// LD RP, d16
		case 0x01: cpu->registers.bc = read_next_word(cpu); break;
		case 0x11: cpu->registers.de = read_next_word(cpu); break;
		case 0x21: cpu->registers.hl = read_next_word(cpu); break;
        // LD rp/d16, A (except hl)
        case 0x02: memory[cpu->registers.bc]   = cpu->registers.a; break;
        case 0x12: memory[cpu->registers.de]   = cpu->registers.a; break;
        case 0xea: memory[read_next_word(cpu)] = cpu->registers.a; break;
		// LD r, hl
		case 0x46: case 0x4e: case 0x56: case 0x5e: case 0x66: case 0x6e: case 0x7e:
			*cpu->reg[(op & 0x38) >> 3] = memory[cpu->registers.hl];
			break;	
		// LD hl, r
		case 0x70: case 0x71: case 0x72: case 0x73: case 0x74: case 0x75: case 0x77:
			memory[cpu->registers.hl] = *cpu->reg[op & 0x7];
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

        // 
        case 0x07: cpu_inst_rlca(cpu); break; // RLCA

        // not ls first?
        // ld?
        // ??
        case 0x08: cpu->registers.sp = read_next_word(cpu); break; // LD (nn), SP

        case 0x0a: cpu->registers.a = memory[cpu->registers.bc]; break; // LD A, (BC)
        case 0x1a: cpu->registers.a = memory[cpu->registers.de]; break; // LD A, (DE)
        case 0x22: memory[cpu->registers.hl++] = cpu->registers.a; break; // LD (HL+), A
        case 0x2a:
            cpu->registers.a = memory[cpu->registers.hl++];
            break; // LD A, (HL+)
        case 0x31:
            cpu->registers.sp = read_next_word(cpu);
            break; // LD SP, d16
		case 0x32:
			memory[cpu->registers.hl] = *cpu->reg[REG_A];
            cpu->registers.hl--;
			break; // LD (HL-), A
        case 0x34:
            cpu_inst_inc(cpu, &memory[cpu->registers.hl]);
            break; // INC (HL)
        case 0x3a:
            cpu->registers.a = memory[cpu->registers.hl--];
            break; // LD A, (HL-)

        // HUGE
        case 0xcb:
            break; // PREFIX CB

        case 0xcd:
            cpu_inst_push(cpu, cpu->registers.pc);
            cpu->registers.pc = read_next_word(cpu);
            break; // CALL d16
        case 0xc9:
            cpu->registers.pc = cpu_inst_pop(cpu);
            break; // RET
		case 0xe0:
			memory[0xff << 8 | read_next_byte(cpu)] = *cpu->reg[REG_A];
			break; // LDH (a8), A
		case 0xf0:
			*cpu->reg[REG_A] = memory[0xff << 8 | read_next_byte(cpu)];
			break; // LDH A, (a8)
        case 0xf9:
            cpu->registers.sp = cpu->registers.hl;
            break; // LD SP, HL
        case 0xfa:
            cpu->registers.a = memory[read_next_word(cpu)];
            break; // LD A, d16
		default:
			printf("Opcode %02x not implemented!\n", op);
			active = 0;
			return -1;
	}
    // e6 , 10, c4 8d
	// service
	service_interrupt(cpu, &swd, 0);
	service_interrupt(cpu, &swe, 1);
	return 0; // stub: cycle
}

static void cpu_regs_init(CPU *cpu) {

	cpu->reg[REG_B] = &cpu->registers.b;
	cpu->reg[REG_C] = &cpu->registers.c;	
	cpu->reg[REG_D] = &cpu->registers.d;
	cpu->reg[REG_E] = &cpu->registers.e;
	cpu->reg[REG_H] = &cpu->registers.h;
	cpu->reg[REG_L] = &cpu->registers.l;
	cpu->reg[REG_A] = &cpu->registers.a;

	cpu->registers.pc = 0;
	cpu->registers.sp = 0;

	cpu->registers.flags.ze = 0;
	cpu->registers.flags.ne = 0;
	cpu->registers.flags.hf = 0;
	cpu->registers.flags.cy = 0;
	
}

// manual allocation of array
// TODO: watch this as some gb games will exceed 0x8000
void allocateMemory() {
	// allocate how much space pointer array need
	// 65536 bytes
	memory = malloc(MEMORY_SIZE * sizeof(uint8_t));
	// reset array
	memset(memory, 0, MEMORY_SIZE);
}

int loadFile(const char *fname, size_t addr) {
	// open only in read-binary mode
	FILE *fp = fopen(fname, "rb");

	if (fp == NULL) {
		fprintf(stderr, "File %s not found!", fname);
		return 1;
	}

	fseek(fp, 0, SEEK_END);
	file_size = ftell(fp);
	rewind(fp);

	// read file
	// offset memory to given address before loading 1byte integer data of a file
	// returns value of last cursor of succeeding reads of file
	
	// args explantion
	// arg 1: where to store buffer
	// arg 2: how much bytes to read in buffer (usually 1 byte or 0xff)
	// arg 3: how much memory to read and load (1 byte * SEEK_END)
	// arg 4: what file to stream (read)
	memory_size = fread(&memory[addr], sizeof(uint8_t), file_size, fp);

	fclose(fp);

	return 0;
}

int main(int argc, char** argv) {
	allocateMemory();
    cpu_regs_init(cpu);
	loadFile("roms/cpu_instrs.gb", 0);

	// File check, can be removed after
	// TODO: to.check it, get the length of memory array and print it
	printf("Program size: %lu (%lu bits)\n", memory_size, 8 * memory_size);
	printf("PROGRAM START\n\n");

	active = 1;
	// 0x100 starting address	
	cpu->registers.pc = 0x100;

    // DISPLAY
    if (initWin() != 0) {
        return -1;
    }
    
	while(active) {
		if (PRINT_LESS) {
			printf("%04x %s\n", cpu->registers.pc, CPU_INST[memory[cpu->registers.pc]]);
		} else {
			printf("PC: %04x (%02x) | SP: %04x | AF: %04x | BC: %04x | DE: %04x | HL: %04x ( %02x %02x %02x %02x ) %s\n",
                // 16 bit registers
				cpu->registers.pc, memory[cpu->registers.pc], cpu->registers.sp,
                // register pairs
                cpu->registers.af, cpu->registers.bc,
                cpu->registers.de, cpu->registers.hl, 
                // peek
				memory[cpu->registers.pc],     memory[cpu->registers.pc + 1],
                memory[cpu->registers.pc + 2], memory[cpu->registers.pc + 3],
                // instruction
                CPU_INST[memory[cpu->registers.pc]]);
		}

		cpu_exec(cpu);
        setDisplay(memory);
	}

	printf("\nPROGRAM END\n\n");
}

// Project started: 16/02/2020
