#include <stdio.h>
#include <algorithm>
#include <array>
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
    int tabWidth;
    char text[0];
} Command;

// cache state
struct State {

    static State& get() {
        static State instance;
        return instance;
    }

    // Buffers
    std::array<unsigned, CELLS_X * CELLS_Y> cellsBuffer1{};
    std::array<unsigned, CELLS_X * CELLS_Y> cellsBuffer2{};
    unsigned* cellsPrev = cellsBuffer1.data();
    unsigned* cells = cellsBuffer2.data();

    // Rectangles
    std::array<Renderer::Rect, CELLS_X * CELLS_Y / 2> rectBuffer{};
    Renderer::Rect screenRect{};

    // Command buffer
    char commandBuffer[COMMAND_BUF_SIZE]{};
    int commandBufferIdx = 0;

    // Debug flag
    bool showDebug = false;

    State(const State&) = delete;
    State& operator=(const State&) = delete;

private:
    State() = default;
};


static void hash(unsigned *h, const void *data, int size) {
    const unsigned char *p = static_cast<unsigned char*>(const_cast<void*>(data));
    while (size--)
        *h = (*h ^ *p++) * 16777619;
}

static inline int cellIdx(const int x, const int y) {
    return x + y * CELLS_X;
}

static inline bool rectsOverlap(const Renderer::Rect a, const Renderer::Rect b) {
    return b.x + b.w  >= a.x && b.x <= a.x + a.w
        && b.y + b.h >= a.y && b.y <= a.y + a.h;
}

static Renderer::Rect intersect(const Renderer::Rect a, const Renderer::Rect b) {
    int const x1 = std::max(a.x, b.x);
    int const y1 = std::max(a.y, b.y);
    int const x2 = std::min(a.x + a.w, b.x + b.w);
    int const y2 = std::min(a.y + a.h, b.y + b.h);
    return (Renderer::Rect) {x1, y1, x2 - x1, y2 - y1};
}

static Renderer::Rect mergeRects(const Renderer::Rect a, const Renderer::Rect b) {
    int const x1 = std::min(a.x, b.x);
    int const y1 = std::min(a.y, b.y);
    int const x2 = std::max(a.x + a.w, b.x + b.w);
    int const y2 = std::max(a.y + a.h, b.y + b.h);
    return (Renderer::Rect) {x1, y1, x2 - x1, y2 - y1};
}

static Command* pushCommand(const int type, const int size = sizeof(Command)) {
    auto* cmd = reinterpret_cast<Command *>(State::get().commandBuffer + State::get().commandBufferIdx);
    const int n = State::get().commandBufferIdx + size;
    if (n > COMMAND_BUF_SIZE) {
        fprintf(stderr, "Warning: (" __FILE__ "): exhausted command buffer\n");
        return nullptr;
    }
    State::get().commandBufferIdx = n;
    memset(cmd, 0, sizeof(Command));
    cmd->type = type;
    cmd->size = size;
    return cmd;
}

static bool nextCommand(Command **prev) {
    if (*prev == nullptr) {
        *prev = reinterpret_cast<Command *>(State::get().commandBuffer);
    } else {
        *prev = reinterpret_cast<Command *>(reinterpret_cast<char *>(*prev) + (*prev)->size);
    }
    return *prev != reinterpret_cast<Command *>(State::get().commandBuffer + State::get().commandBufferIdx);
}

void showDebug(const bool enable) {
    State::get().showDebug = enable;
}

void freeFont(Renderer::Font *font) {
    if (Command *cmd = pushCommand(FREE_FONT)) { cmd->font = font; }
}

void setClipRect(const Renderer::Rect rect) {
    if (Command *cmd = pushCommand(SET_CLIP)) { cmd->rect = intersect(rect, State::get().screenRect); }
}

void drawRect(const Renderer::Rect rect, const Renderer::Color color) {
    if (!rectsOverlap(State::get().screenRect, rect)) { return; }
    if (Command *cmd = pushCommand(DRAW_RECT)) {
        cmd->rect = rect;
        cmd->color = color;
    }
}

int drawText(Renderer::Font *font, const char *text, int x, int y, Renderer::Color color) {
    Renderer::Rect rect;
    rect.x = x;
    rect.y = y;
    rect.w = Renderer::getFontWidth(font, text);
    rect.h = Renderer::getFontHeight(font);

    if (rectsOverlap(State::get().screenRect, rect)) {
        const int sz = static_cast<int>(strlen(text) + 1);
        Command *cmd = pushCommand(DRAW_TEXT, static_cast<int>(sizeof(Command)) + sz);
        if (cmd) {
            memcpy(cmd->text, text, sz);
            cmd->color = color;
            cmd->font = font;
            cmd->rect = rect;
            cmd->tabWidth = Renderer::getFontTabWidth(font);
        }
    }

    return x + rect.w;
}


