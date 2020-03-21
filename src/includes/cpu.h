#ifndef _CPU_H
#define _CPU_H
#define ADDR_CB 0x100
#include <stdio.h>
#include <stdbool.h>

// cycles holder
uint8_t tmp_cycle_cpu, tmp_cycle_bytes;

// cb address holder
uint16_t addr_cb;

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

        uint16_t pc, sp;  // 16-bit register address
    } registers;

	uint8_t *regAddr[8];
    bool sw_interrupt : 1;		// interrupt
} CPU;

CPU sharp;
CPU *cpu;

#endif  // header guard
