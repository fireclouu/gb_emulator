#ifndef CPU_H
#define CPU_H

#include <stdbool.h>
#include <stdio.h>

// Custom Z80 cpu
enum REG_ID {
    REG_B, REG_C,
    REG_D, REG_E,
    REG_H, REG_L,
    REG_FILLER, REG_A
};

typedef struct gb_cpu {
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

// holder
extern uint8_t tmp_cycle_bytes;
extern uint8_t tmp_cycle_cpu;

// shared
void cpu_regs_init(CPU*);
int cpu_exec(CPU*);
uint8_t mmu_rb(const uint16_t);
void mmu_wb(const uint16_t, const uint8_t);
#endif
