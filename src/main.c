#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "cpu.h"
#include "disassembler.h"
#include "display.c"
#include "main.h"

#define USYSTIME tv.tv_sec*1000000+tv.tv_usec
#define PRINT_LESS 0
#define PRINT_FULL 1

struct timeval tv;
CPU mcpu;
CPU *cpu = &mcpu;

bool halt;
uint8_t *m_bios;
uint8_t *m_rom;

// ram
uint8_t *w_ram;
uint8_t *e_ram;
uint8_t *z_ram;

size_t timer;

void loadfile(const char *filename, uint8_t *memptr,
        int bufstart, int bufend)
{
    FILE *fp = fopen(filename, "rb");

    if (fp == NULL)
    {
        printf("HOST: File \"%s\" not found.\n", filename);
        exit(1);
    }

    fseek(fp, 0, SEEK_END);
    int fsize = ftell(fp);
    if ( (bufstart > fsize) || (bufend > fsize) )
    {
        printf("HOST: Requested buffer exceed file's size.\n");
        exit(1);
    }

    rewind(fp);
    fseek(fp, bufstart, SEEK_SET);
    fread(memptr, sizeof(uint8_t), bufend, fp);
}
void init_gb() {
    cpu_init(cpu);
}
void init() {
    halt = false;

    m_bios = malloc(SIZE_BIOS * sizeof(uint8_t));
    m_rom  = malloc(SIZE_ROM  * sizeof(uint8_t));
    m_vram = malloc(SIZE_VRAM * sizeof(uint8_t));

    g_oam  = malloc(SIZE_OAM  * sizeof(uint8_t));
    w_ram  = malloc(SIZE_WRAM * sizeof(uint8_t));
    e_ram  = malloc(SIZE_ERAM * sizeof(uint8_t));
    z_ram  = malloc(SIZE_ZRAM * sizeof(uint8_t));

    init_gb();
}
void printTrace(int type)
{
	// check if address is 0xCB
	int op = (mmu_rb(cpu->registers.pc, 0) == 0xCB) ?
		mmu_rb(cpu->registers.pc + 1, 0) + ADDR_CB  :
		mmu_rb(cpu->registers.pc, 0);

    if (type == PRINT_LESS)
    {
        printf("%04x %s\n", cpu->registers.pc,
                CPU_INST[mmu_rb(op, 0)]);
    } else if (type == PRINT_FULL) {
        printf("PC: %04x (%03x) | SP: %04x | AF: %04x | BC: %04x | DE: %04x | HL: %04x ( %02x %02x %02x %02x ) %s\n",
                // 16 bit registers
                cpu->registers.pc, op, cpu->registers.sp,
                // register pairs
                cpu->registers.af, cpu->registers.bc,
                cpu->registers.de, cpu->registers.hl,
                // peek
                mmu_rb(cpu->registers.pc,     0), mmu_rb(cpu->registers.pc + 1, 0),
                mmu_rb(cpu->registers.pc + 2, 0), mmu_rb(cpu->registers.pc + 3, 0),
                // instruction
                CPU_INST[op]);
    }
}
int main(/*int argc, char **args*/) {
    printf("PROGRAM STARTED\n");

    init();
    loadfile("roms/bios.bin", m_bios, MEM_REGION_BIOS, MEM_REGION_END_BIOS);
    loadfile("roms/tetris.gb", m_rom, MEM_REGION_ROM,  MEM_REGION_END_ROM);
    display_init();
    display_test();
    display_close();
    while(!halt)
    {
        // timing
        gettimeofday(&tv, NULL);
        timer = USYSTIME;
        // printtrace
        printTrace(PRINT_FULL);
        // cpu exec
        cpu_step(cpu, 0);
    }

    printf("PROGRAM ENDED\n");
    return 0;
}
