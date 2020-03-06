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
    // https://cturt.github.io/cinoop.html
    struct {

        struct {
            union {
                struct {
                    struct {
                        uint8_t filler : 4;
                        uint8_t cy : 1;
                        uint8_t hf : 1;
                        uint8_t ne : 1; 
                        uint8_t ze : 1; // flags, pos: znhc0000 (as reg. F / psw)	
                    } flags;
                    uint8_t a;
                };
                uint16_t af;
            };
        };

        struct {
            union {
                struct {
                    uint8_t c;
                    uint8_t b;
                };
                uint16_t bc;
            };
        };

        struct {
            union {
                struct {
                    uint8_t e;
                    uint8_t d;
                };
                uint16_t de;
            };
        };

        struct {
            union {
                struct {
                    uint8_t l;
                    uint8_t h;
                };
                uint16_t hl;
            };
        };

    } registers;

	uint8_t *reg[8];

    uint16_t pc, sp;  // 16-bit register address6
    bool sw_interrupt : 1;		// interrupt
} CPU;

// Display
bool initWin();
void closeWin();
void setDisplay(const uint8_t*);
