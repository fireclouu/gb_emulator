#include "mmu.h"
#include "main.h"

// MMU / Memory Bus
static inline uint8_t  mmu_read_byte(const uint16_t addr) {
    return memory[addr];
}
static inline void mmu_write_byte(const uint16_t addr, const uint8_t data) {
    memory[addr] = data;
}
