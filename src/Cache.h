
#ifndef ETATIDEM_RENCACHE_H
#define ETATIDEM_RENCACHE_H

#include "Renderer.h"

namespace Renderer::Cache {

    void showDebug(bool enable);
    void freeFont(Font *font);
    void setClipRect(Rect rect);
    void drawRect(Rect rect, Color color);
    int drawText(Font *font, const char *text, int x, int y, Color color);
    void invalidate();
    void beginFrame();
    void endFrame();

}


#endif //ETATIDEM_RENCACHE_H