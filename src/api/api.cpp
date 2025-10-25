
#include "api.h"

int luaOpenSystem(lua_State *L);
int luaOpenRenderer(lua_State *L);

static const luaL_Reg libs[] = {
{ "system",    luaOpenSystem     },
{ "renderer",  luaOpenRenderer   },
{ nullptr, nullptr               }
};