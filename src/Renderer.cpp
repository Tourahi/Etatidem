#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <math.h>
#include "lib/stb/stb_truetype.h"
#include "Renderer.h"
#include "lib/aixlog.hpp"


namespace Renderer {
    // Window
    static SDL_Window *window;
    static struct { int left, top, right, bottom; } clip;

#define MAX_GLYPHSET 256

    struct Image {
        Color *pixels;
        int w, h;
    };

    typedef struct {
        Image *image;
        stbtt_bakedchar glyphs[256];
    } GlyphSet;

    struct Font {
        void *data;
        stbtt_fontinfo stbfont;
        GlyphSet *sets[MAX_GLYPHSET];
        float size;
        int height;
    };

    struct Font {

    };

    // Helpers ------------------------------------------------

    static void* checkAlloc(void* ptr) {
        if (!ptr) {
            LOG(WARNING) << AixLog::Color::red << "Fatal error: [checkAlloc] memory allocation failed\n";
            exit(EXIT_FAILURE);
        }
        return ptr;
    }

    const char* utf8toCodePoint(const char *c, unsigned *dst) {
        unsigned res, n;
        switch (*c & 0xf0) {
            case 0xf0 :  res = *c & 0x07;  n = 3;  break; // 4-byte char
            case 0xe0 :  res = *c & 0x0f;  n = 2;  break; // 3-byte char
            case 0xd0 :
            case 0xc0 :  res = *c & 0x1f;  n = 1;  break; // 2-byte char
            default   :  res = *c;         n = 0;  break;
        }
        while (n--) {
            res = (res << 6) | (*(++c) & 0x3f);
        }
        *dst = res;
        return c + 1; // return next char position
    }

    // Helpers ------------------------------------------------ End

    void setClipRect(Rect rect) {
        clip.left = rect.x;
        clip.top = rect.y;
        clip.right = rect.x + rect.w;
        clip.bottom = rect.y + rect.h;
    }

    void init(SDL_Window *win) {
        assert(win);
        window = win;
        SDL_Surface *surf = SDL_GetWindowSurface(window);
        setClipRect((Rect) {0, 0, surf->w, surf->h} );
    }

    void getClipRect(Rect *rect) {
        rect->x = clip.left;
        rect->y = clip.top;
        rect->w = clip.right - clip.left;
        rect->h = clip.bottom - clip.top;
    }




}