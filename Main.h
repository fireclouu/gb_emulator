#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#define NAME "gbemu"
#define MEMORY_SIZE 0x10000

// GameBoy
// CPU: 8-bit CPU @ 4 MHz
// Display: Monochrome 4-tone LCD 160x144
// Feats: expandable RAM/ROM module

// Custom Z80 cpu
enum REG_ID {
    REG_B, REG_C,
    REG_D, REG_E,
    REG_H, REG_L,
    REG_FILLER, REG_A
};

typedef struct lr35902 {
	// uint8_t a, b, c, d, e, h, l;	// 8-bit general purpose registers
	uint8_t reg[8];
	uint16_t pc, sp;		// 16-bit register address
	bool ze, ne, hf, cy;		// flags, pos: znhc0000 (as reg. F / psw)
	bool sw_interrupt;		// interrupt
} CPU;

// Display
bool initWin();
void closeWin();
void setDisplay(const uint8_t*);
