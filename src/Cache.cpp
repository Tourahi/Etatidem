#include <stdio.h>
#include <algorithm>
#include "Cache.h"

/* 32bit fnv-1a hash */
#define HASH_INIT 2166136261
#define CELLS_X 80
#define CELLS_Y 50
#define CELL_SIZE 96
#define COMMAND_BUF_SIZE (1024 * 512)


enum { FREE_FONT, SET_CLIP, DRAW_TEXT, DRAW_RECT };

typedef struct {
    int type, size;
    Renderer::Rect rect;
    Renderer::Color color;
    Renderer::Font *font;
    int tadWidth;
    char text[0];
} Command;


static void hash(unsigned *h, const void *data, int size) {
    const unsigned char *p = static_cast<unsigned char*>(const_cast<void*>(data));
    while (size--)
        *h = (*h ^ *p++) * 16777619;
}

static inline int cellIdx(int x, int y) {
    return x + y * CELLS_X;
}

static inline bool rectsOverlap(Renderer::Rect const a, Renderer::Rect const b) {
    return b.x + b.w  >= a.x && b.x <= a.x + a.w
        && b.y + b.h >= a.y && b.y <= a.y + a.h;
}

static Renderer::Rect intersect(Renderer::Rect const a, Renderer::Rect const b) {
    int x1 = std::max(a.x, b.x);
    int y1 = std::max(a.y, b.y);
    int x2 = std::min(a.x + a.w, b.x + b.w);
    int y2 = std::min(a.y + a.h, b.y + b.h);
    return (Renderer::Rect) {x1, y1, x2 - x1, y2 - y1};
}


