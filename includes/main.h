#ifndef MAIN_H
#define MAIN_H

#include <stdbool.h>
#include "cpu.h"

#define MEM_REGION_BIOS       0x0000
#define MEM_REGION_ROM        0x0000
#define MEM_REGION_VRAM       0x8000

#define MEM_REGION_END_BIOS   0x0100
#define MEM_REGION_END_ROM    0x7fff // bank 0-n
#define MEM_REGION_END_VRAM   0x9fff

#define SIZE_BIOS     MEM_REGION_END_BIOS - MEM_REGION_BIOS
#define SIZE_ROM      MEM_REGION_END_ROM  - MEM_REGION_ROM
#define SIZE_VRAM     MEM_REGION_END_VRAM - MEM_REGION_VRAM

#define SIZE_OAM      0xFF
#define SIZE_WRAM     0x1FFF
#define SIZE_ERAM     0x1FFF
#define SIZE_ZRAM     0xFF

extern bool halt;

// Cpu
extern CPU mcpu;
extern CPU *cpu;

// memory holders
extern uint8_t *m_bios;     // bios
extern uint8_t *m_rom ;     // program rom
extern uint8_t *m_vram;     // gpu
extern uint8_t *g_oam ;     // gpu oam (160 bytes)
extern uint8_t *w_ram ;     // working ram (8k bytes)
extern uint8_t *e_ram ;     // external ram (8k bytes)
extern uint8_t *z_ram ;     // zero page ram (loc 0xffff - 0xff80)

#endif
