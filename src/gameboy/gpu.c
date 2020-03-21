#include <stdio.h>
#include "gpu.h"
#include "shared.h"

size_t timer_ly;

enum INT 
{
    INT_LY = 0xff44,
};

static inline uint8_t gpu_read_vram() 
{
    return 0;// stub
}
static inline void gpu_write_vram() 
{

}
// writing 0xff44 will reset its value
// set vblank interrupt at 144 to  153
static inline void gpu_int_ly(const uint8_t val)
{
    // increment memory address 0xff44, every 109ms
    // pseudo for lcd driver scanline transfer
    if (timer >= timer_ly + 116)
    {
        if (!(++memory[INT_LY] < 153) || (val != 0)) memory[INT_LY] = 0;
        timer_ly = timer;
    }

    // check for interrupt
    if (memory[INT_LY] > 144) 
    {
        memory[ADDR_IF] |= 1;
    }
}
