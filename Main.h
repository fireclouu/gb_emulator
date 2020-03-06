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
	uint8_t reg[8];
    uint16_t pc, sp;  // 16-bit register address6
	bool ze : 1, ne : 1, hf : 1, cy : 1;	// flags, pos: znhc0000 (as reg. F / psw)
	bool sw_interrupt : 1;		// interrupt
} CPU;

// Display
bool initWin();
void closeWin();
void setDisplay(const uint8_t*);
