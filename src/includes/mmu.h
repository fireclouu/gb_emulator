#define MODE_R 0
#define MODE_W 1

static inline uint8_t mmu_read_byte(uint16_t addr);
static inline void mmu_write_byte(uint16_t addr, uint8_t data);
