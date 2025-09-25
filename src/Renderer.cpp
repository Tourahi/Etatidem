#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <math.h>
#include "lib/stb/stb_truetype.h"
#include "Renderer.h"


namespace Renderer {
    // Window
    static SDL_Window *window;
    static struct { int left, top, right, bottom; } clip;

    struct Image {
        Renderer::Color *pixels;
        int w, h;
    };

    struct Font {

    };

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


}