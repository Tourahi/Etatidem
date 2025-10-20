
#ifndef ETATIDEM_RENCACHE_H
#define ETATIDEM_RENCACHE_H

#include "Renderer.h"

namespace Renderer::Cache {

    void showDebug(bool enable);
    void freeFontCmd(Font *font);
    void setClipRectCmd(Rect rect);
    void drawRectCmd(Rect rect, Color color);
    int drawTextCmd(Font *font, const char *text, int x, int y, Color color);
    void invalidate();
    void beginFrame();
    void endFrame();

}


#endif //ETATIDEM_RENCACHE_H