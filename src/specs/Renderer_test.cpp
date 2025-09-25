#include "../lib/utest.h"
#include "../Renderer.h"

UTEST(Renderer, InitSetsClipRect) {
    SDL_Window *win = SDL_CreateWindow("Test", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, 0);
    Renderer::init(win);

    SDL_DestroyWindow(win);
}
