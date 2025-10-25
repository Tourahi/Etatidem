#include <SDL2/SDL.h>
#include <stdbool.h>
#include <ctype.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include "api.h"
#include "../Cache.h"
#ifdef _WIN32
  #include <windows.h>
#endif

extern  SDL_Window *window;

static const char* buttonName(int btn) {
  switch (btn) {
    case 1: return "left";
    case 2: return "middle";
    case 3: return "right";
    default: return "?";
  }
}


static char* keyName(char *dst, int sym) {
  strcpy(dst, SDL_GetKeyName(sym));
  char *p = dst;
  while (*p) {
    *p = static_cast<char>(tolower(*p));
    p++;
  }
  return dst;
}

static int fPollEvent(lua_State *L) {
  char buf[16];
  int mx, my, wx, wy;
  SDL_Event event;

  while (SDL_PollEvent(&event)) {
    switch (event.type) {

      case SDL_QUIT:
        lua_pushstring(L, "quit");
        return 1;

      case SDL_WINDOWEVENT:
        if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
          lua_pushstring(L, "resized");
          lua_pushstring(L, reinterpret_cast<const char *>(event.window.data1));
          lua_pushstring(L, reinterpret_cast<const char *>(event.window.data2));
          return 3;
        }

        if (event.window.event == SDL_WINDOWEVENT_EXPOSED) {
          Renderer::Cache::invalidate();
          lua_pushstring(L, "exposed");
          return 1;
        }

        /* on some systems, when alt-tabbing to the window SDL will queue up
        ** several KEYDOWN events for the `tab` key; we flush all keydown
        ** events on focus so these are discarded */
        if (event.window.event == SDL_WINDOWEVENT_FOCUS_GAINED) {
          SDL_FlushEvent(SDL_KEYDOWN);
        }
        break;

      case SDL_DROPFILE:
        SDL_GetGlobalMouseState(&mx, &my);
        SDL_GetWindowPosition(window, &wx, &wy);
        lua_pushstring(L, "filedropped");
        lua_pushstring(L, event.drop.file);
        lua_pushnumber(L, mx - wx);
        lua_pushnumber(L, my - wy);
        SDL_free(event.drop.file);
        return 4;

      case SDL_KEYDOWN:
        lua_pushstring(L, "keypressed");
        lua_pushstring(L, keyName(buf, event.key.keysym.sym));
        return 2;

      case SDL_KEYUP:
        lua_pushstring(L, "keyreleased");
        lua_pushstring(L, keyName(buf, event.key.keysym.sym));
        return 2;

      case SDL_TEXTINPUT:
        lua_pushstring(L, "textinput");
        lua_pushstring(L, event.text.text);
        return 2;

      case SDL_MOUSEBUTTONDOWN:
        if (event.button.button == 1) { SDL_CaptureMouse(static_cast<SDL_bool>(1)); }
        lua_pushstring(L, "mousepressed");
        lua_pushstring(L, buttonName(event.button.button));
        lua_pushnumber(L, event.button.x);
        lua_pushnumber(L, event.button.y);
        lua_pushnumber(L, event.button.clicks);
        return 5;

      case SDL_MOUSEBUTTONUP:
        if (event.button.button == 1) { SDL_CaptureMouse(static_cast<SDL_bool>(0)); }
        lua_pushstring(L, "mousereleased");
        lua_pushstring(L, buttonName(event.button.button));
        lua_pushnumber(L, event.button.x);
        lua_pushnumber(L, event.button.y);
        return 4;

      case SDL_MOUSEMOTION:
        lua_pushstring(L, "mousemoved");
        lua_pushnumber(L, event.motion.x);
        lua_pushnumber(L, event.motion.y);
        lua_pushnumber(L, event.motion.xrel);
        lua_pushnumber(L, event.motion.yrel);
        return 5;

      case SDL_MOUSEWHEEL:
        lua_pushstring(L, "mousewheel");
        lua_pushnumber(L, event.wheel.y);
        return 2;

      default:
        break;
    }
  }
  return 0;
}




