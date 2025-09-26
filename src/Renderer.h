#ifndef RENDERER_H
#define RENDERER_H

#include <SDL2/SDL.h>
#include <stdint.h>

namespace Renderer {

    typedef struct Image;
    typedef struct Font;

    typedef struct { uint8_t b, g, r, a; } Color;
    typedef struct {int x, y, w, h; } Rect;

    void init(SDL_Window* win);
    void setClipRect(Rect rect);
    void getClipRect(Rect *rect);


    // Helpers
    const char* utf8toCodePoint(const char *c, unsigned *dst);

}

#endif //RENDERER_H
