#include "LuaUtil.h"

extern "C" {
    #include "lauxlib.h"
}

#define TAG "[LUA] "

int hc::GetField(lua_State* const L, int const tableIndex, char const* const fieldName, Logger* const logger) {
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
            logger->error(TAG "%s", lua_tostring(L, -1));
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
            logger->error(TAG "Field \"%s\" is not a function", fieldName);
        }

        lua_pop(L, nargs + 1);
        return false;
    }

    lua_insert(L, lua_gettop(L) - nargs);
    return ProtectedCall(L, nargs, nresults, logger);
}

static lrcpp::Frontend* checkFrontend(lua_State* const L, int index) {
    return *(lrcpp::Frontend**)luaL_checkudata(L, index, "lrcpp::Frontend");
}

static int l_run(lua_State* const L) {
    auto const self = checkFrontend(L, 1);

    if (!self->run()) {
        return luaL_error(L, "error getting the API version");
    }

    return 0;
}

static int l_apiVersion(lua_State* const L) {
    auto const self = checkFrontend(L, 1);
    unsigned version;

    if (!self->apiVersion(&version)) {
        return luaL_error(L, "error getting the API version");
    }

    lua_pushinteger(L, version);
    return 1;
}

static int l_getSystemInfo(lua_State* const L) {
    auto const self = checkFrontend(L, 1);
    retro_system_info info;

    if (!self->getSystemInfo(&info)) {
        return luaL_error(L, "error getting the system info");
    }

    lua_createtable(L, 0, 5);

    lua_pushstring(L, info.library_name);
    lua_setfield(L, -2, "libraryName");

    lua_pushstring(L, info.library_version);
    lua_setfield(L, -2, "libraryVersion");

    lua_pushstring(L, info.valid_extensions);
    lua_setfield(L, -2, "validExtensions");

    lua_pushboolean(L, info.need_fullpath);
    lua_setfield(L, -2, "needFullPath");

    lua_pushboolean(L, info.block_extract);
    lua_setfield(L, -2, "blockExtract");

    return 1;
}

static int l_getSystemAvInfo(lua_State* const L) {
    auto const self = checkFrontend(L, 1);
    retro_system_av_info info;

    if (!self->getSystemAvInfo(&info)) {
        return luaL_error(L, "error getting the system a/v info");
    }

    lua_createtable(L, 0, 2);

    lua_createtable(L, 0, 5);

    lua_pushinteger(L, info.geometry.base_width);
    lua_setfield(L, -2, "baseWidth");

    lua_pushinteger(L, info.geometry.base_height);
    lua_setfield(L, -2, "baseHeight");

    lua_pushinteger(L, info.geometry.max_width);
    lua_setfield(L, -2, "maxWidth");

    lua_pushinteger(L, info.geometry.max_height);
    lua_setfield(L, -2, "maxHeight");

    lua_pushnumber(L, info.geometry.aspect_ratio);
    lua_setfield(L, -2, "aspectRatio");

    lua_setfield(L, -2, "geometry");

    lua_createtable(L, 0, 2);

    lua_pushnumber(L, info.timing.fps);
    lua_setfield(L, -2, "fps");

    lua_pushnumber(L, info.timing.sample_rate);
    lua_setfield(L, -2, "sampleRate");

    lua_setfield(L, -2, "timing");

    return 1;
}

static int l_serializeSize(lua_State* const L) {
    auto const self = checkFrontend(L, 1);
    size_t size;

    if (!self->serializeSize(&size)) {
        return luaL_error(L, "error getting the size need to serialize the state");
    }

    lua_pushinteger(L, size);
    return 1;
}

static int l_serialize(lua_State* const L) {
    auto const self = checkFrontend(L, 1);
    size_t size;

    if (!self->serializeSize(&size)) {
        return luaL_error(L, "error serializing state");
    }

    void* const data = malloc(size);

    if (data == nullptr) {
        return luaL_error(L, "out of memory");
    }

    if (!self->serialize(data, size)) {
        free(data);
        return luaL_error(L, "error serializing state");
    }

    lua_pushlstring(L, static_cast<char*>(data), size);
    free(data);
    return 1;
}

static int l_unserialize(lua_State* const L) {
    auto const self = checkFrontend(L, 1);

    size_t size;
    char const* const data = luaL_checklstring(L, 2, &size);

    if (!self->unserialize(data, size)) {
        return luaL_error(L, "error unserializing state");
    }

    return 0;
}

static int l_cheatReset(lua_State* const L) {
    auto const self = checkFrontend(L, 1);

    if (!self->cheatReset()) {
        return luaL_error(L, "error resetting cheats");
    }

    return 0;
}

static int l_cheatSet(lua_State* const L) {
    auto const self = checkFrontend(L, 1);
    unsigned const index = luaL_checkinteger(L, 2);
    bool const enabled = lua_toboolean(L, 3);
    char const* const code = luaL_checkstring(L, 4);

    if (!self->cheatSet(index, enabled, code)) {
        return luaL_error(L, "error setting cheat (%u, %s, \"%s\")", index, enabled ? "true" : "false", code);
    }

    return 0;
}

static int l_getRegion(lua_State* const L) {
    auto const self = checkFrontend(L, 1);
    unsigned region;

    if (!self->getRegion(&region)) {
        return luaL_error(L, "error getting the region");
    }

    switch (region) {
        case RETRO_REGION_NTSC: lua_pushliteral(L, "ntsc"); break;
        case RETRO_REGION_PAL:  lua_pushliteral(L, "pal"); break;
        default: return luaL_error(L, "invalid region: %u", region);
    }

    return 1;
}

static int str2id(char const* const str) {
    if (strcmp(str, "save") == 0) {
        return RETRO_MEMORY_SAVE_RAM;
    }
    else if (strcmp(str, "rtc") == 0) {
        return RETRO_MEMORY_RTC;
    }
    else if (strcmp(str, "sram") == 0) {
        return RETRO_MEMORY_SYSTEM_RAM;
    }
    else if (strcmp(str, "vram") == 0) {
        return RETRO_MEMORY_VIDEO_RAM;
    }
    else {
        char* end;
        long const id = strtol(str, &end, 10);

        if (*str == 0 || *end != 0) {
            return -1;
        }

        return static_cast<int>(id);
    }
}

static int l_getMemoryData(lua_State* const L) {
    auto const self = checkFrontend(L, 1);
    char const* const idStr = luaL_checkstring(L, 2);

    int const id = str2id(idStr);

    if (id < 0) {
        return luaL_error(L, "invalid memory id: \"%s\"", idStr);
    }

    void* data;

    if (!self->getMemoryData(static_cast<unsigned>(id), &data)) {
        return luaL_error(L, "error getting memory data for \"%s\"", idStr);
    }

    lua_pushlightuserdata(L, data);
    return 1;
}

static int l_getMemorySize(lua_State* const L) {
    auto const self = checkFrontend(L, 1);
    char const* const idStr = luaL_checkstring(L, 2);

    int const id = str2id(idStr);

    if (id < 0) {
        return luaL_error(L, "invalid memory id: \"%s\"", idStr);
    }

    size_t size;

    if (!self->getMemorySize(static_cast<unsigned>(id), &size)) {
        return luaL_error(L, "error getting memory size for \"%s\"", idStr);
    }

    lua_pushinteger(L, size);
    return 1;
}

int hc::PushFrontend(lua_State* const L, lrcpp::Frontend* const frontend) {
    auto const self = static_cast<lrcpp::Frontend**>(lua_newuserdata(L, sizeof(lrcpp::Frontend*)));
    *self = frontend;

    if (luaL_newmetatable(L, "lrcpp::Frontend")) {
        static luaL_Reg const methods[] = {
            {"run", l_run},
            {"apiVersion", l_apiVersion},
            {"getSystemInfo", l_getSystemInfo},
            {"getSystemAvInfo", l_getSystemAvInfo},
            {"serializeSize", l_serializeSize},
            {"serialize", l_serialize},
            {"unserialize", l_unserialize},
            {"cheatReset", l_cheatReset},
            {"cheatSet", l_cheatSet},
            {"getRegion", l_getRegion},
            {"getMemoryData", l_getMemoryData},
            {"getMemorySize", l_getMemorySize},
            {nullptr, nullptr}
        };

        luaL_newlib(L, methods);

        int const load = luaL_loadstring(L,
            "local string = require 'string'\n"
            "return function(fmt, ...)\n"
            "    return string.format(fmt, ...)\n"
            "end\n"
        );

        if (load != LUA_OK) {
            return lua_error(L);
        }

        lua_call(L, 0, 1);
        lua_setfield(L, -2, "format");

        lua_setfield(L, -2, "__index");
    }

    lua_setmetatable(L, -2);
    return 1;
}
