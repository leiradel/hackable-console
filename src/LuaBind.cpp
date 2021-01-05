#include "LuaBind.h"

#include <stddef.h>
#include <string.h>

extern "C" {
    #include <lua.h>
    #include <lauxlib.h>
}

extern "C" {
    int luaopen_lfs(lua_State*);
}

int luaopen_hc(lua_State*);

typedef struct {
    const char* name;

    union {
        const char* source;
        lua_CFunction openf;
    };

    size_t length;
}
module_t;

// Adds a Lua module to the modules array (length = length of the Lua source
// code).
#define MODL(name, array) {name, {array}, sizeof(array)}

// Adds a native module to the modules array (length = 0).
#define MODC(name, openf) {name, {(char*)openf}, 0}

static const module_t modules[] = {
    MODC("lfs", luaopen_lfs),
    MODC("hc", luaopen_hc)
};

#undef MODL
#undef MODC

static int searcher(lua_State* const L) {
    // Get the module name.
    char const* const modname = lua_tostring(L, 1);

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
void RegisterSearcher(lua_State* const L) {
    // Get the package global table.
    lua_getglobal(L, "package");
    // Get the list of searchers in the package table.
    lua_getfield(L, -1, "searchers");
    // Get the number of existing searchers in the table.
    size_t const length = lua_rawlen(L, -1);

    // Add our own searcher to the list.
    lua_pushcfunction(L, searcher);
    lua_rawseti(L, -2, length + 1);

    // Remove the seachers and the package tables from the stack.
    lua_pop(L, 2);
}
