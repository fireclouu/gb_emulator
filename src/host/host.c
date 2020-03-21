#include <stdio.h>
#include <stdlib.h>
#include "cpu.h"
#include "display.h"
#include "host.h"
#include "shared.h"
#include "utils.h"
#include "disassembler.c"

void allocateMemory()
{
	memory = malloc(MEMORY_SIZE * sizeof(uint8_t));
	memset(memory, 0, MEMORY_SIZE);
}

int loadFile(const char *fname, size_t addr)
{
	FILE *fp = fopen(fname, "rb");

	if (fp == NULL) {
		fprintf(stderr, "File \"%s\" not found!\n", fname);
		return -1;
	}

	fseek(fp, 0, SEEK_END);
	file_size = ftell(fp);
	rewind(fp);

	memory_size = fread(&memory[addr], sizeof(uint8_t), file_size, fp);
	fclose(fp);

	return 0;
}

void printTrace(int type)
{
    if (type == PRINT_LESS) {
		printf("%04x %s\n", cpu->registers.pc, CPU_INST[memory[cpu->registers.pc]]);
    } else if (type == PRINT_FULL) {
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
}


