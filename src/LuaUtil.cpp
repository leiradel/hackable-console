#include "LuaUtil.h"

extern "C" {
    #include "lauxlib.h"
}

int hc::GetField(lua_State* const L, int const tableIndex, char const* const fieldName, Logger* const logger) {
    int const top = lua_gettop(L);
    lua_pushvalue(L, tableIndex);

    if (!lua_istable(L, -1)) {
        if (logger != nullptr) {
            logger->error("Value at index %d is not a table", tableIndex);
        }

        lua_settop(L, top);
        lua_pushnil(L);
        return LUA_TNONE;
    }

    int const type = lua_getfield(L, -1, fieldName);

    if (type == LUA_TNIL && logger != nullptr) {
        logger->warn("Field \"%s\" is nil", fieldName);
    }

    lua_remove(L, top + 1);
    return type;
}

static int Traceback(lua_State* const L) {
    luaL_traceback(L, L, lua_tostring(L, -1), 1);
    return 1;
}

bool hc::ProtectedCall(lua_State* const L, const int nargs, const int nresults, Logger* const logger) {
    lua_pushcfunction(L, Traceback);
    int const msgh = lua_gettop(L) - 1 - nargs;
    lua_insert(L, msgh);

    if (lua_pcall(L, nargs, nresults, msgh) != LUA_OK) {
        if (logger != nullptr) {
            logger->error("%s", lua_tostring(L, -1));
        }

        lua_settop(L, msgh - 1);
        return false;
    }

    lua_remove(L, msgh);
    return true;
}

bool hc::ProtectedCallField(
    lua_State* const L,
    int const tableIndex,
    char const* const fieldName,
    int const nargs,
    int const nresults,
    Logger* const logger
) {
    if (GetField(L, tableIndex, fieldName) != LUA_TFUNCTION) {
        if (logger != nullptr) {
            logger->error("Field \"%s\" is not a function", fieldName);
        }

        lua_pop(L, nargs + 1);
        return false;
    }

    lua_insert(L, lua_gettop(L) - nargs);
    return ProtectedCall(L, nargs, nresults, logger);
}
