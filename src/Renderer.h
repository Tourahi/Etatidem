#ifndef RENDERER_H
#define RENDERER_H

#include <SDL2/SDL.h>
#include <stdint.h>

namespace Renderer {

    typedef struct Image Image;
    typedef struct Font Font;

    typedef struct { uint8_t b, g, r, a; } Color;
    typedef struct {int x, y, w, h; } Rect;

    void init(SDL_Window* win);
    void setClipRect(Rect rect);
    void getClipRect(Rect *rect);

    void updateRects(Rect *rects, int count);
    void getSize(int *x, int *y);
    Image* newImage(int width, int height);
    void freeImage(Image *image);

    // font
    Font* loadFont(const char *filename, float size);
    void freeFont(Font *font);
    void setFontTabWidth(Font *font, int n);
    int getFontTabWidth(Font *font);
    int getFontWidth(Font *font, const char *text);
    int getFontHeight(Font *font);



    // Helpers
    const char* utf8toCodePoint(const char *c, unsigned *dst);

}

#endif //RENDERER_H
