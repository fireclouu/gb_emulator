#include "Main.h"

static inline uint8_t get_display(const uint8_t* memory, const uint16_t addr) {
	// msb , lsb
}

static inline uint8_t proc_color(const uint8_t* hNib, const uint8_t* lNib, const uint8_t pos) {
	return ( ( (lNib >> pos) & 0x1) | (( (HNib >> pos) & 0x1   ) << 1));
}
