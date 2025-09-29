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

    static void logInfo(const std::string msg) {
#ifdef ENABLE_DEBUG_INFO
        LOG(INFO) << AixLog::Color::blue  << msg << "\n";
#endif
    }

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
        logInfo("Font loaded: " + std::string(filename));

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

    void freeFont(Font *font) {
        for (int i = 0; i < MAX_GLYPHSET; i++) {
            GlyphSet *set = font->sets[i];
            if (set) {
                freeImage(set->image);
                free(set);
            }
        }
        free(font->data);
        free(font);
    }

    void setFontTabWidth(Font *font, int n) {
        GlyphSet *set = getGlyphSet(font, '\t');
        set->glyphs['\t'].xadvance = n;
    }

    int getFontTabWidth(Font *font) {
        GlyphSet *set = getGlyphSet(font, '\t');
        return set->glyphs['\t'].xadvance;
    }

    int getFontWidth(Font *font, const char *text) {
        int x = 0;
        const char *p = text;
        unsigned codepoint;
        while (*p) {
            p = utf8toCodePoint(p, &codepoint);
            GlyphSet *set = getGlyphSet(font, codepoint);
            stbtt_bakedchar *g = &set->glyphs[codepoint & 0xff]; // [codepoint & 0xff] insures index in [0,255]
            x += g->xadvance;
        }
        return x;
    }

    int getFontHeight(Font *font) {
        return font->height;
    }

    static Color blendPixel(Color dst, Color src) {
        int ia = 0xff - src.a;
        dst.r = ((src.r * src.a) + (dst.r * ia)) >> 8;
        dst.g = ((src.g * src.a) + (dst.g * ia)) >> 8;
        dst.b = ((src.b * src.a) + (dst.b * ia)) >> 8;
        return dst;
    }

    static Color blendPixel2(Color dst, Color src, Color color) {
        src.a = (src.a * color.a) >> 8;
        int ia = 0xff - src.a;
        dst.r = ((src.r * color.r * src.a) >> 16) + ((dst.r * ia) >> 8);
        dst.g = ((src.g * color.g * src.a) >> 16) + ((dst.g * ia) >> 8);
        dst.b = ((src.b * color.b * src.a) >> 16) + ((dst.b * ia) >> 8);
        return dst;
    }

#define rect_draw_loop(expr)            \
    for (int j = y1; j < y2; j++) {     \
        for (int i = x1; i < x2; i++) { \
            *d = expr;                  \
            d++;                        \
        }                               \
        d += dr;                        \
    }

    void drawRect(Rect rect, Color color) {
        if (color.a == 0) { return; }

        int x1 = rect.x < clip.left ? clip.left : rect.x; // clipped x
        int y1 = rect.y < clip.top  ? clip.top  : rect.y; // clipped y
        int x2 = rect.x + rect.w;
        int y2 = rect.y + rect.h;
        x2 = x2 > clip.right  ? clip.right  : x2;
        y2 = y2 > clip.bottom ? clip.bottom : y2;

        SDL_Surface *surf = SDL_GetWindowSurface(window);
        Color *d = static_cast<Color*>(surf->pixels);
        d += x1 + y1 * surf->w; // move to (x1, y1)
        int dr = surf->w - (x2 - x1); // delta row

        if (color.a == 0xff) {
            rect_draw_loop(color);
        } else {
            rect_draw_loop(blendPixel(*d, color));
        }
    }

    void drawImage(Image *image, Rect *sub, int x, int y, Color color) {
        if (color.a == 0) { return; }

        // clip
        int n;
        if ((n = clip.left - x) > 0) { sub->w  -= n; sub->x += n; x += n; }
        if ((n = clip.top  - y) > 0) { sub->h -= n; sub->y += n; y += n; }
        if ((n = x + sub->w  - clip.right ) > 0) { sub->w  -= n; }
        if ((n = y + sub->h - clip.bottom) > 0) { sub->h -= n; }

        if (sub->w <= 0 || sub->h <= 0) {
            return;
        }

        // Draw
        SDL_Surface *surf = SDL_GetWindowSurface(window);
        Color *s = image->pixels;
        Color *d = (Color*) surf->pixels;
        s += sub->x + sub->y * image->w;
        d += x + y * surf->w;
        int sr = image->w - sub->w;
        int dr = surf->w - sub->w;

        for (int j = 0; j < sub->h; j++) {
            for (int i = 0; i < sub->w; i++) {
                *d = blendPixel2(*d, *s, color);
                d++;
                s++;
            }
            d += dr;
            s += sr;
        }
    }

    int drawText(Font *font, const char *text, int x, int y, Color color) {
        Rect rect;
        const char *p = text;
        unsigned codepoint;
        while (*p) {
            p = utf8toCodePoint(p, &codepoint);
            GlyphSet *set = getGlyphSet(font, codepoint);
            stbtt_bakedchar *g = &set->glyphs[codepoint & 0xff];
            rect.x = g->x0;
            rect.y = g->y0;
            rect.w = g->x1 - g->x0;
            rect.h = g->y1 - g->y0;
            drawImage(set->image, &rect, x + g->xoff, y + g->yoff, color);
            x += g->xadvance;
        }
        return x;
    }


}