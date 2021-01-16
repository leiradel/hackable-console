#include "LuaUtil.h"

extern "C" {
    #include "lauxlib.h"
}

#define TAG "[LUA] "

extern "C" {
    int luaopen_lfs(lua_State*);
}

typedef struct {
    const char* name;

    union {
        const char* source;
        lua_CFunction openf;
    };

    size_t length;
}
Module;

// Adds a Lua module to the modules array (length = length of the Lua source
// code).
#define MODL(name, array) {name, {array}, sizeof(array)}

// Adds a native module to the modules array (length = 0).
#define MODC(name, openf) {name, {(char*)openf}, 0}

static const Module modules[] = {
    MODC("lfs", luaopen_lfs)
};

#undef MODL
#undef MODC

static int searcher(lua_State* const L) {
    // Get the module name.
    char const* const modname = lua_tostring(L, 1);

    if (strcmp(modname, "hc") == 0) {
        // It's the hc instance
        lua_Integer const hcRef = lua_tointeger(L, lua_upvalueindex(1));
        lua_pushcfunction(L, [](lua_State* const L) { lua_pushvalue(L, 2); return 1; });
        lua_rawgeti(L, LUA_REGISTRYINDEX, hcRef);
        return 2;
    }

    // Iterates over all modules we know.
    for (size_t i = 0; i < sizeof(modules) / sizeof(modules[0]); i++) {
        if (strcmp(modname, modules[i].name) == 0) {
            // Found the module!
            if (modules[i].length != 0) {
                // It's a Lua module, return the chunk that defines the module.
                const int res = luaL_loadbufferx(L,
                                                 modules[i].source,
                                                 modules[i].length,
                                                 modname,
                                                 "t");

                if (res != LUA_OK) {
                    // Compilation error.
                    return lua_error(L);
                }
            }
            else {
                // It's a native module, return the native function that
                // defines the module.
                lua_pushcfunction(L, modules[i].openf);
            }

            return 1;
        }
    }

    // Oops...
    lua_pushfstring(L, "unknown module \"%s\"", modname);
    return 1;
}

// Registers the searcher function.
void hc::registerSearcher(lua_State* const L) {
    int const top = lua_gettop(L);

    // Get the package global table.
    lua_getglobal(L, "package");
    // Get the list of searchers in the package table.
    lua_getfield(L, -1, "searchers");
    // Get the number of existing searchers in the table.
    size_t const length = lua_rawlen(L, -1);

    // Add our own searcher to the list.
    lua_pushvalue(L, top);
    int const hcRef = luaL_ref(L, LUA_REGISTRYINDEX);
    lua_pushinteger(L, hcRef);
    lua_pushcclosure(L, searcher, 1);
    lua_rawseti(L, -2, length + 1);

    // Remove everything from the stack, including the hc value.
    lua_settop(L, top - 1);
}

int hc::getField(lua_State* const L, int const tableIndex, char const* const fieldName, Logger* const logger) {
    int const top = lua_gettop(L);
    lua_pushvalue(L, tableIndex);

    if (!lua_istable(L, -1)) {
        if (logger != nullptr) {
            logger->error(TAG "Value at index %d is not a table", tableIndex);
        }

        lua_settop(L, top);
        lua_pushnil(L);
        return LUA_TNONE;
    }

    int const type = lua_getfield(L, -1, fieldName);

    if (type == LUA_TNIL && logger != nullptr) {
        logger->warn(TAG "Field \"%s\" is nil", fieldName);
    }

    lua_remove(L, top + 1);
    return type;
}

static int traceback(lua_State* const L) {
    luaL_traceback(L, L, lua_tostring(L, -1), 1);
    return 1;
}

bool hc::protectedCall(lua_State* const L, const int nargs, const int nresults, Logger* const logger) {
    lua_pushcfunction(L, traceback);
    int const msgh = lua_gettop(L) - 1 - nargs;
    lua_insert(L, msgh);

    if (lua_pcall(L, nargs, nresults, msgh) != LUA_OK) {
        if (logger != nullptr) {
            logger->error(TAG "%s", lua_tostring(L, -1));
        }

        lua_settop(L, msgh - 1);
        return false;
    }

    lua_remove(L, msgh);
    return true;
}

bool hc::protectedCallField(
    lua_State* const L,
    int const tableIndex,
    char const* const fieldName,
    int const nargs,
    int const nresults,
    Logger* const logger
) {
    if (getField(L, tableIndex, fieldName) != LUA_TFUNCTION) {
        if (logger != nullptr) {
            logger->error(TAG "Field \"%s\" is not a function", fieldName);
        }

        lua_pop(L, nargs + 1);
        return false;
    }

    lua_insert(L, lua_gettop(L) - nargs);
    return protectedCall(L, nargs, nresults, logger);
}
