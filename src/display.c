#include "display.h"

enum RENDERERS {
    TRANSPARENT, WGRAY, SGRAY, DARK, RENDERER_COUNT
};

SDL_Window *gWindow;
SDL_Renderer *gRenderer;

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
            printf("Cannot create renderer! %s",  SDL_GetError());
            return -1;
        }
    }

    // enlarge pixel area
    SDL_RenderSetScale(gRenderer, 2, 2);
    return 0;
}

void plotPoints(const int x, const int y, const uint8_t color) {
    switch (color)
    {
        // transparent can be skipped
        case TRANSPARENT:
            SDL_SetRenderDrawColor(gRenderer, 255, 255, 255, 255); // placeholder only
            break;
        case WGRAY:
            SDL_SetRenderDrawColor(gRenderer, 170, 170, 170, 255);
            break;
        case SGRAY:
            SDL_SetRenderDrawColor(gRenderer, 85, 85, 85, 255);
            break;
        case DARK:
            SDL_SetRenderDrawColor(gRenderer, 0, 0, 0, 255);
            break;
        default:
            printf("%d: cannot place this pixel", color);
            break;
    }

    SDL_RenderDrawPoint(gRenderer, x, y);
}

void closeWin() {
    SDL_DestroyWindow(gWindow);
    SDL_Quit();
}

static inline uint8_t proc_color(const uint8_t hNib, const uint8_t lNib, const uint8_t pos) {
	return ( ( (lNib >> pos) & 0x1) | (( (hNib >> pos) & 0x1   ) << 1));
}

// 0x8000 - 0x9ffff (Read)
void setDisplay(const uint8_t* memory) {
	// clear Window
    SDL_SetRenderDrawColor(gRenderer, 255, 255, 255, 255);
    SDL_RenderClear(gRenderer);

    int read = VRAM_START;

    for ( int y = 0; y <= GB_SCR_H; y++ ) 
    {
        for ( int x = 0; x <= GB_SCR_W; x+=8 ) 
        {
            for (int bitpos = 7; bitpos >= 0; bitpos--) 
            {
                int color = proc_color(memory[read], memory[read + 1], bitpos);
                plotPoints(x + bitpos, y, color);
            }

            read += 2;
        }
    }

    SDL_RenderPresent(gRenderer);
}


