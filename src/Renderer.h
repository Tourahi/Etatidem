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

}

#endif //RENDERER_H
