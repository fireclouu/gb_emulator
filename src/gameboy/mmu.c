#include "cpu.h"
#include "mmu.h"
#include "shared.h"

// MMU / Memory Bus
static inline uint8_t mmu_read_byte(const uint16_t addr) {
    tmp_cycle_cpu += 4;
    tmp_cycle_bytes++;
    return memory[addr];
}
static inline void mmu_write_byte(const uint16_t addr, const uint8_t data) {
    tmp_cycle_cpu += 4;
    memory[addr] = data;
}
static inline uint16_t mmu_read_word(const uint16_t addr) {
    return memory[addr] | (memory[addr + 1] << 8);
}
static inline void mmu_write_word(const uint16_t addr, const uint16_t data) {
    memory[addr] = data & 0xff;
    memory[addr + 1] = data >> 8;
}
