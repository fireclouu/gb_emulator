#include <SDL2/SDL.h>
#include <stdbool.h>
#include <strings.h>
#include "display.h"

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#define TITLE "GBEMU2"

SDL_Renderer *gRenderer;
SDL_Window *gWindow;
SDL_Texture *gTexture;

static inline bool display_init(void) {
    bool success = true;

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("SDL could not be initialized! %s\n", SDL_GetError());
        success = false;
    } else {
        gWindow = SDL_CreateWindow(TITLE, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);

        if (gWindow == NULL) {
            printf("Window could not be created! %s\n", SDL_GetError());
            success = false;
        } else {
            // trying hardware rendering
            gRenderer = SDL_CreateRenderer(gWindow, -1, SDL_RENDERER_ACCELERATED);

            if (gRenderer == NULL) {
                gRenderer = SDL_CreateRenderer(gWindow, -1, SDL_RENDERER_SOFTWARE);
                printf("Trying software rendering...\n");
            }

            if (gRenderer == NULL) {
                printf("Failed to create renderer! %s\n", SDL_GetError());
                success = false;
            }
        }
    }

    return success;
}
static void display_test(void) {
    SDL_RenderPresent(gRenderer);
    SDL_Delay(10000);
}
static inline void display_close(void) {
    SDL_DestroyRenderer(gRenderer);
    SDL_DestroyWindow(gWindow);
    gRenderer = NULL;
    gWindow = NULL;

    SDL_Quit();
}
