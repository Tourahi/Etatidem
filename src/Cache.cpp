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

void Renderer::Cache::showDebug(const bool enable) {
    State::get().showDebug = enable;
}

void Renderer::Cache::freeFontCmd(Font *font) {
    if (Command *cmd = pushCommand(FREE_FONT)) { cmd->font = font; }
}

void Renderer::Cache::setClipRectCmd(const Rect rect) {
    if (Command *cmd = pushCommand(SET_CLIP)) { cmd->rect = intersect(rect, State::get().screenRect); }
}

void Renderer::Cache::drawRectCmd(const Rect rect, const Color color) {
    if (!rectsOverlap(State::get().screenRect, rect)) { return; }
    if (Command *cmd = pushCommand(DRAW_RECT)) {
        cmd->rect = rect;
        cmd->color = color;
    }
}

int Renderer::Cache::drawTextCmd(Font *font, const char *text, const int x, const int y, Color color) {
    Rect rect;
    rect.x = x;
    rect.y = y;
    rect.w = getFontWidth(font, text);
    rect.h = getFontHeight(font);

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

void Renderer::Cache::invalidate() {
    memset(State::get().cellsPrev, 0xff, sizeof(State::get().cellsBuffer1));
}

void Renderer::Cache::beginFrame() {
    int w, h;
    getSize(&w, &h);
    if (State::get().screenRect.w != w || State::get().screenRect.h != h) {
        State::get().screenRect.w = w;
        State::get().screenRect.h = h;
        invalidate();
    }
}

static void pushRect(Renderer::Rect r, int *count) {
    for (int i = *count; i > 0; i--) {
        if (Renderer::Rect *rp = &State::get().rectBuffer[i]; rectsOverlap(*rp, r)) {
            *rp = mergeRects(*rp, r);
            return;
        }
    }
    State::get().rectBuffer[(*count)++] = r;
}

static void updateOverlappingCells(const Renderer::Rect r, const unsigned h) {
    const int x1 = r.x / CELL_SIZE;
    const int y1 = r.y / CELL_SIZE;
    const int x2 = (r.x + r.w) / CELL_SIZE;
    const int y2 = (r.y + r.h) / CELL_SIZE;

    for (int y = y1; y <= y2; y++) {
        for (int x = x1; x <= x2; x++) {
            const int idx = cellIdx(x, y);
            hash(&State::get().cells[idx], &h, sizeof(h));
        }
    }
}



void Renderer::Cache::endFrame() {
    // update cells from commands
    Command *cmd = nullptr;
    Rect cr = State::get().screenRect;
    while (nextCommand(&cmd)) {
        if (cmd->type == SET_CLIP) { cr = cmd->rect; }
        Rect r = intersect(cmd->rect, cr);
        if (r.w == 0 || r.h == 0) { continue; }
        unsigned h = HASH_INIT;
        hash(&h, cmd, cmd->size);
        updateOverlappingCells(r, h);
    }

    // push rects for all cells changed from last frame, reset cells
    int rectCount = 0;
    int maxX = State::get().screenRect.w / CELL_SIZE + 1;
    int maxY = State::get().screenRect.h / CELL_SIZE + 1;
    for (int y = 0; y < maxY; y++) {
        for (int x = 0; x < maxX; x++) {
            // compare previous and current cell
            const int idx = cellIdx(x, y);
            if (State::get().cells[idx] != State::get().cellsPrev[idx]) {
                pushRect({x, y, 1, 1}, &rectCount);
            }
            State::get().cellsPrev[idx] = HASH_INIT;
        }
    }

    // expand rects from cells to pixels
    for (int i = 0; i < rectCount; i++) {
        Rect *r = &State::get().rectBuffer[i];
        r->x *= CELL_SIZE;
        r->y *= CELL_SIZE;
        r->w *= CELL_SIZE;
        r->h *= CELL_SIZE;
        *r = intersect(*r, State::get().screenRect);
    }

    // redraw updated regions
    bool hasFreeCommands = false;
    for (int i = 0; i < rectCount; i++) {
        // draw
        Rect r = State::get().rectBuffer[i];
        setClipRectCmd(r);

        cmd = nullptr;
        while (nextCommand(&cmd)) {
            switch (cmd->type) {
                case FREE_FONT:
                    hasFreeCommands = true;
                    break;
                case SET_CLIP:
                    setClipRect(intersect(cmd->rect, r));
                    break;
                case DRAW_RECT:
                    drawRect(cmd->rect, cmd->color);
                    break;
                case DRAW_TEXT:
                    setFontTabWidth(cmd->font, cmd->tabWidth);
                    drawText(cmd->font, cmd->text, cmd->rect.x, cmd->rect.y, cmd->color);
                    break;
            }
        }
    }

    // update dirty rects
    if (rectCount > 0) {
        updateRects(State::get().rectBuffer.data(), rectCount);
    }

    // free fonts
    if (hasFreeCommands) {
        cmd = nullptr;
        while (nextCommand(&cmd)) {
            if (cmd->type == FREE_FONT) {
                freeFont(cmd->font);
            }
        }
    }

    // swap cell buffer and reset
    unsigned *tmp = State::get().cells;
    State::get().cells = State::get().cellsPrev;
    State::get().cellsPrev = tmp;
    State::get().commandBufferIdx = 0;
}







