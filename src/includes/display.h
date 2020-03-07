#include <stdio.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include "utils.h"

#define WIDTH 640
#define HEIGHT 480
#define GB_SCR_W 160
#define GB_SCR_H 144
#define VRAM_START 0x8000

// Display
bool initWin();
void closeWin();
void setDisplay(const uint8_t*);
