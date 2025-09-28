#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <math.h>
#include "lib/stb/stb_truetype.h"
#include "Renderer.h"

#include "config.h"
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

    void updateRects(Rect *rects, int count) {
        SDL_UpdateWindowSurfaceRects(window, (SDL_Rect*) rects, count);
        static bool initialFrame = true;
        if (initialFrame) {
            SDL_ShowWindow(window); // Show window on first frame
            initialFrame = false;
        }
    }

    void getSize(int *x, int *y) {
        SDL_Surface *surf = SDL_GetWindowSurface(window);
        *x = surf->w;
        *y = surf->h;
    }

    Image* newImage(int width, int height) {
        assert(width > 0 && height > 0);
        Image *image = static_cast<Image *>(malloc(sizeof(Image) + width * height * sizeof(Color)));
        checkAlloc(image);
        image->pixels = reinterpret_cast<Color *>(image + 1);
        image->w = width;
        image->h = height;
        return image;
    }

    void freeImage(Image *image) {
        free(image);
    }

    static GlyphSet* loadGlyphSet(Font *font, int idx) {
        GlyphSet *set = static_cast<GlyphSet *>(checkAlloc(calloc(1, sizeof(GlyphSet))));

        // Init image
        int width = 128;
        int height = 128;
    retry:
        set->image = newImage(width, height);

        // load glyphs
        float s =  stbtt_ScaleForMappingEmToPixels(&font->stbfont, 1) /
                stbtt_ScaleForPixelHeight(&font->stbfont, 1); // for constant scaling
        int res = stbtt_BakeFontBitmap(
            static_cast<const unsigned char *>(font->data), 0, font->size * s,
            reinterpret_cast<unsigned char *>(set->image->pixels),
            width, height, idx * 256, 256, set->glyphs);

        // retry each time with a larger image size
        if (res < 0) {
            width *= 2;
            height *= 2;
            freeImage(set->image);
            goto retry; // HERESY
        }

        // adjust glyph yoffsets and xadvance
        int ascent, descent, linegap;
        stbtt_GetFontVMetrics(&font->stbfont, &ascent, &descent, &linegap);
        float scale = stbtt_ScaleForMappingEmToPixels(&font->stbfont, font->size);
        int scaled_ascent = ascent * scale + EYEWITNESS_SCALED_ASCENT_OFFSET;
        for (int i = 0; i < 256; i++) {
            set->glyphs[i].yoff += scaled_ascent;
            set->glyphs[i].xadvance = floor(set->glyphs[i].xadvance);
        }

        // convert 8Bits data to 32Bits RGBA
        for (int i = width * height - 1; i >= 0; i--) {
            uint8_t n = *(reinterpret_cast<uint8_t *>(set->image->pixels) + i);
            set->image->pixels[i] = (Color) { .b = 255, .g = 255, .r = 255, .a = n };
        }

        return set;
    }

    static GlyphSet* getGlyphSet(Font *font, int codepoint) {
        int idx = (codepoint >> 8) % MAX_GLYPHSET; // divide by 256
        if (!font->sets[idx]) {
            font->sets[idx] = loadGlyphSet(font, idx);
        }
        return font->sets[idx];
    }

    Font* loadFont(const char *filename, float size) {
        Font *font = nullptr;
        FILE *fp = nullptr;

        // init font
        font = static_cast<Font*>(checkAlloc(calloc(1, sizeof(Font))));
        font->size = size;

        // load font into buffer
        fp = fopen(filename, "rb");
        if (!fp) { return nullptr; }

        // get size
        fseek(fp, 0, SEEK_END);
        int bufSize = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        // load
        font->data = checkAlloc(malloc(bufSize));
        int _ = fread(font->data, 1, bufSize, fp); (void) _;
        fclose(fp);
        fp = nullptr;

        // init stbfont
        int ok = stbtt_InitFont(&font->stbfont, static_cast<const unsigned char *>(font->data), 0);
        if (!ok) {
            if (fp) { fclose(fp); }
            if (font) { free(font->data); }
            free(font);
            return nullptr;
        }

        // get height and scale
        int ascent, descent, linegap;
        stbtt_GetFontVMetrics(&font->stbfont, &ascent, &descent, &linegap);
        float scale = stbtt_ScaleForMappingEmToPixels(&font->stbfont, font->size);
        font->height = (ascent - descent + linegap) * scale + EYEWITNESS_SCALED_ASCENT_OFFSET;

        // make tab and newline glyphs invisible
        stbtt_bakedchar *g = getGlyphSet(font, '\n')->glyphs;
        g['\t'].x1 = g['\t'].x0;
        g['\n'].x1 = g['\n'].x0;

        return font;
    }



}