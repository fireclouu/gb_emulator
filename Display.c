#include "Main.h"
#include <SDL2/SDL.h>
#define WIDTH 640
#define HEIGHT 480

SDL_Window* gWindow;
SDL_Renderer* gRenderer;

bool initWin() {
    if ( SDL_Init(SDL_INIT_VIDEO) > 0 ) 
    {
        printf("SDL Initialization failed! %s", SDL_GetError());
        return -1;
    }

    gWindow = SDL_CreateWindow(NAME, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
    
    if (gWindow == NULL)
    {
        printf("Cannot create window! %s", SDL_GetError());
        return -1;
    }

    gRenderer = SDL_CreateRenderer(gWindow, -1, SDL_RENDERER_ACCELERATED);
    if (gRenderer == NULL) 
    {
        gRenderer = SDL_CreateRenderer(gWindow, -1, SDL_RENDERER_SOFTWARE);
        if (gRenderer == NULL) 
        {
            printf("Cannot create renderer! %s", SDL_GetError());
            return -1;
        }
    }

    SDL_SetRenderDrawColor(gRenderer, 0, 0, 0, 255);
    SDL_RenderClear(gRenderer);
    SDL_RenderPresent(gRenderer);
    return 0;
}

void closeWin() {
    SDL_DestroyWindow(gWindow);
    SDL_Quit();
}

static inline void get_display(const uint8_t* memory, const uint16_t addr) {
	// msb , lsb
}

static inline uint8_t proc_color(const uint8_t hNib, const uint8_t lNib, const uint8_t pos) {
	return ( ( (lNib >> pos) & 0x1) | (( (hNib >> pos) & 0x1   ) << 1));
}
