#include "Main.h"
#include "Disassembler.c"

CPU z80;
CPU *cpu = &z80;

bool PRINT_LESS = true;

static inline void PUSH(CPU *a1, const uint16_t a2);

uint8_t *memory;
size_t file_size;
size_t memory_size; // store memory size

uint16_t rp; // make as pair pointer

bool active;

// separate this block of code if you learn how to make them link together
int loadFile(const char *fname, size_t addr) {
	// open only in read-binary mode
	FILE *fp = fopen(fname, "rb");

	if (fp == NULL) {
		fprintf(stderr, "File %s not found!", fname);
		return 1;
	}

	// get its length
	// arg explanation
	// arg 1: what file to seek
	// arg 2: move the cursor
	// arg 3: where to start
	fseek(fp, 0, SEEK_END);

	// size_t is convenience for largest integer data type available
	file_size = ftell(fp);

	// place cursor at the beginning
	// convenient than fseek(fp, 0, SEEK_SET)
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

	// close file buffer to save memory
	fclose(fp);

	// no errors within this point
	return 0;
}

// needs optimization
static inline uint16_t read_next_word(CPU *cpu) {
	cpu->pc += 2;
	return memory[cpu->pc - 1] << 8 | memory[cpu->pc - 2];
}

static inline uint8_t read_next_byte(CPU *cpu) {
	cpu->pc++;
	return memory[cpu->pc - 1];
}

// register pairs and psw
static inline uint8_t get_psw(const CPU *cpu) {
	return cpu->ze << 7 | cpu->ne << 6 |
		cpu-> hf << 5 | cpu->cy << 4;
}
static inline uint16_t get_pair_bc(const CPU *cpu) {
	return cpu->reg[REG_B] << 8 | cpu->reg[REG_C];
}
static inline uint16_t get_pair_de(const CPU *cpu) {
	return cpu->reg[REG_D] << 8 | cpu->reg[REG_E];
}

static inline uint16_t get_pair_hl(const CPU *cpu) {
	return cpu->reg[REG_H] << 8 | cpu->reg[REG_L];
}
static inline uint16_t get_pair_af(const CPU *cpu) {
	return cpu->reg[REG_A] << 8  | get_psw(cpu);
}
static inline void set_pair_bc(CPU *cpu, uint16_t val) {
	cpu->reg[REG_B] = (val >> 8);
	cpu->reg[REG_C] = val;
}
static inline void set_pair_de(CPU *cpu, uint16_t val) {
	cpu->reg[REG_D] = (val >> 8);
	cpu->reg[REG_E] = val;
}
static inline void set_pair_hl(CPU *cpu, uint16_t val) {
	cpu->reg[REG_H] = (val >> 8);
	cpu->reg[REG_L] = val;
}
// Flags

// Instructions
static inline uint16_t get_mask(const size_t size, const int bitNum) {
	return (1 << ((size * 8) - bitNum)) - 1;
}
static inline void CALL(CPU *cpu, const uint16_t addr) {
	PUSH(cpu, addr + 3);
	cpu->pc = addr;
}
static inline void CP(CPU *cpu, const uint8_t val) {
	cpu->ze = (cpu->reg[REG_A] == val);
	cpu->ne = 1;
	cpu->hf = (cpu->reg[REG_A] & 0xf) >= (val & 0xf);
}
// 8 bit
static inline void DEC(uint8_t* reg) {
	*reg = *reg - 1;
	cpu->ze = !(*reg);
	cpu->ne = 1;
	cpu->hf = (*reg & 0xf) >= 1;
}

// verified
static inline void PUSH(CPU *cpu, const uint16_t val) {
	memory[--cpu->sp] = val & 0xff00 >> 8;
	memory[--cpu->sp] = val & 0x00ff;
}
static inline void RST(CPU *cpu, const int val) {
	PUSH(cpu, cpu->pc);
	cpu->pc = val;
}
static inline void XOR(CPU *cpu, uint8_t val) {
	cpu->reg[REG_A] ^= val;
	cpu->ze = !cpu->reg[REG_A];
	cpu->ne = cpu->hf = cpu->cy = 0; 
}
// conditional jumps
static inline void cond_JP(CPU *cpu, const bool cond, int16_t data) {
	if (cond) cpu->pc = (cpu->pc + data);
}
// interrupts
int pass = 0;
bool interrupt, swd, swe;
static inline void service_interrupt(CPU *cpu, bool *sw, const bool val) {
	if (!sw) return;
	if (++pass >= 2) {
		interrupt = val;
		pass = 0;
		*sw = 0;
	}
}
// return its cycle value
static inline int cpu_exec(CPU *cpu) {
	uint8_t op = memory[cpu->pc];
	cpu->pc++;

	switch(op) 
	{
		// DEC regs
		case 0x05: case 0x0d: case 0x15: case 0x1d: case 0x25: case 0x2d: case 0x3d:
			DEC(&cpu->reg[(op & 0x38) >> 3]);
			break;
		// LD regs, d8
		case 0x06: case 0x0e: case 0x16: case 0x1e: case 0x26: case 0x2e: case 0x3e:
			cpu->reg[(op & 0x38) >> 3] = read_next_byte(cpu);
			break;
		// LD (HL-), A
		case 0x32:
			memory[get_pair_hl(cpu)] = cpu->reg[REG_A];
			set_pair_hl(cpu, get_pair_hl(cpu) - 1);
			break;
		// LDH (a8), A
		case 0xe0:
			memory[0xff << 8 | read_next_byte(cpu)] = cpu->reg[REG_A];
			break;
		// LDH A, (a8)
		case 0xf0:
			cpu->reg[REG_A] = memory[0xff << 8 | read_next_byte(cpu)];
			break;
		// CP regs
		case 0xb8: case 0xb9: case 0xba: case 0xbb: case 0xbc: case 0xbd: case 0xbf:
			CP(cpu, cpu->reg[op & 0x7]);
			break;
		case 0xfe:
			CP(cpu, read_next_byte(cpu));
			break;
			/*
		case 0x39:
			cpu->hf = ((get_pair_hl(cpu) & 0xfff) + (cpu->sp & 0xfff)) & 0xf000;
			cpu->cy = (get_pair_hl(cpu) + cpu->sp) >> 16;
			set_pair_hl(cpu, get_pair_hl(cpu) + cpu->sp);
			cpu->ne = 0;
			break; // ADD HL, SP
		case 0x3b: cpu->sp--; break; // DEC SP
		// PUSH
		case 0xc5: PUSH(cpu, get_pair_bc(cpu)); break;
		case 0xd5: PUSH(cpu, get_pair_de(cpu)); break;
		case 0xe5: PUSH(cpu, get_pair_hl(cpu)); break;
		case 0xf5: PUSH(cpu, get_pair_af(cpu)); break;*/
		// Address Jumps
		case 0x20: cond_JP(cpu, !cpu->ze, (int8_t) read_next_byte(cpu)); break;
		case 0xc3: cpu->pc = read_next_word(cpu); break; // JP d16
		// Interrupts
		case 0xf3: swd = 1; break; // DI
		case 0xfb: swe = 1; break; // EI
		/*
		// RST
		case 0xc7: RST(cpu, 0x00); break;
		case 0xcf: RST(cpu, 0x08); break;
		case 0xd7: RST(cpu, 0x10); break;
		case 0xdf: RST(cpu, 0x18); break;
		case 0xe7: RST(cpu, 0x20); break;
		case 0xef: RST(cpu, 0x28); break;
		case 0xf7: RST(cpu, 0x30); break;
		case 0xff: RST(cpu, 0x38); break;*/
		// LD RP, d16
		case 0x01: set_pair_bc(cpu, read_next_word(cpu)); break;
		case 0x11: set_pair_de(cpu, read_next_word(cpu)); break;
		case 0x21: set_pair_hl(cpu, read_next_word(cpu)); break;
		// XOR
		case 0xa8: case 0xa9: case 0xaa: case 0xab: case 0xac: case 0xad: case 0xaf:
			   XOR(cpu, cpu->reg[op & 0x7]);
			   break;
		// LD (no hl)
		case 0x40: case 0x41: case 0x42: case 0x43: case 0x44: case 0x45: case 0x47:
		case 0x48: case 0x49: case 0x4a: case 0x4b: case 0x4c: case 0x4d: case 0x4f:
		case 0x50: case 0x51: case 0x52: case 0x53: case 0x54: case 0x55: case 0x57:
		case 0x58: case 0x59: case 0x5a: case 0x5b: case 0x5c: case 0x5d: case 0x5f:
		case 0x60: case 0x61: case 0x62: case 0x63: case 0x64: case 0x65: case 0x67:
		case 0x68: case 0x69: case 0x6a: case 0x6b: case 0x6c: case 0x6d: case 0x6f:
		case 0x78: case 0x79: case 0x7a: case 0x7b: case 0x7c: case 0x7d: case 0x7f:
			cpu->reg[(op & 0x38) >> 3] = cpu->reg[op & 0x7];
			break;

		// LD r, hl
		case 0x46: case 0x4e: case 0x56: case 0x5e: case 0x66: case 0x6e: case 0x7e:
			rp = get_pair_hl(cpu);
			cpu->reg[op & 0x38] = memory[rp];
			break;
	
		// LD hl, r
		case 0x70: case 0x71: case 0x72: case 0x73: case 0x74: case 0x75: case 0x77:
			rp = get_pair_hl(cpu);
			memory[rp] = cpu->reg[op & 0x7];
			break;

		// NOP
		case 0x00:
			break;

		// Disabled — should not modify anything in cpu
		case 0xd3: case 0xdb: case 0xdd: case 0xe3: case 0xe4: case 0xeb: case 0xec:
		case 0xed: case 0xf4: case 0xfc: case 0xfd:
			return 0;
		default:
			printf("Opcode %02x not implemented!\n", op);
			active = 0;
			return -1;
	}

	// service
	service_interrupt(cpu, &swd, 0);
	service_interrupt(cpu, &swe, 1);
	return 0; // stub: cycle
}

static void cpu_regs_init(CPU *cpu) {

	cpu->reg[REG_A] = 0;
	cpu->reg[REG_B] = 0;
	cpu->reg[REG_C] = 0;	
	cpu->reg[REG_D] = 0;
	cpu->reg[REG_E] = 0;
	cpu->reg[REG_H] = 0;
	cpu->reg[REG_L] = 0;
	
	cpu->pc = 0;
	cpu->sp = 0;

	cpu->ze = 0;
	cpu->ne = 0;
	cpu->hf = 0;
	cpu->cy = 0;
	
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

int main(int argc, char** argv) {
	allocateMemory();
	loadFile("tetris.gb", 0);

	// File check, can be removed after
	// TODO: to.check it, get the length of memory array and print it
	printf("Program size: %lu (%lu bits)\n", memory_size, 8 * memory_size);
	printf("PROGRAM START\n\n");

	active = 1;
	// 0x100 starting address	
	cpu->pc = 0x0100;

    // DISPLAY
    if (initWin() != 0) {
        return -1;
    }

	while(active) {
		if (PRINT_LESS) {
			printf("%04x %s\n", cpu->pc, CPU_INST[memory[cpu->pc]]);
		} else {
			printf("PC: %04x (%02x) | SP: %04x | AF: %04x | BC: %04x | DE: %04x | HL: %04x ( %02x %02x %02x %02x ) (%s)\n",
				cpu->pc, memory[cpu->pc], cpu->sp, get_pair_af(cpu), 
				get_pair_bc(cpu), get_pair_de(cpu), get_pair_hl(cpu), 
				memory[cpu->pc], memory[cpu->pc + 1], memory[cpu->pc + 2], 
				memory[cpu->pc + 3], CPU_INST[memory[cpu->pc]]);
		}
		
		cpu_exec(cpu);
	}

	printf("\nPROGRAM END\n\n");
}

// Project started: 16/02/2020
