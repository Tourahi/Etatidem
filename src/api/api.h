
#ifndef ETATIDEM_API_H
#define ETATIDEM_API_H

#include "../lib/lua52/lapi.h"
#include "../lib/lua52/lauxlib.h"
#include "../lib/lua52/lualib.h"

#define API_TYPE_FONT "font"

void apiLoadLibs(lua_State *L);

#endif //ETATIDEM_API_H