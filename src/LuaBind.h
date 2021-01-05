#pragma once

extern "C" {
    #include <lua.h>
}

void RegisterSearcher(lua_State* const L);
