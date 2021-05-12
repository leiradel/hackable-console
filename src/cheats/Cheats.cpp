#include "Cheats.h"
#include "Memory.h"
#include "Set.h"
#include "Filter.h"

extern "C" {
    #include <lauxlib.h>
}

#include "Cheats.lua.h"

static int s_onFrame = LUA_NOREF;

static int l_empty(lua_State* const L) {
    return hc::Set::empty()->push(L);
}

static int l_universal(lua_State* const L) {
    return hc::Set::universal()->push(L);
}

static int l_filter(lua_State* const L) {
    char const* const op_str = luaL_checkstring(L, 2);
    char const* const settings = luaL_checkstring(L, 4);

    bool is_signed = false;
    size_t value_size;
    hc::filter::Endianess endianess = hc::filter::Endianess::Little;

    switch (settings[0]) {
        case 's': is_signed = true; break;
        case 'u': is_signed = false; break;
        default: return luaL_error(L, "invalid signedness \'%c\'", settings[0]);
    }

    switch (settings[1]) {
        case 'b': value_size = 1; break;
        case 'w': value_size = 2; break;
        case 'd': value_size = 4; break;
        case 'q': value_size = 8; break;
        default: return luaL_error(L, "invalid operand size \'%c\'", settings[1]);
    }

    if (value_size != 1) {
        switch (settings[2]) {
            case 'l': endianess = hc::filter::Endianess::Little; break;
            case 'b': endianess = hc::filter::Endianess::Big; break;
            default: return luaL_error(L, "invalid endianess \'%c\'", settings[2]);
        }
    }

    if (settings[value_size == 1 ? 2 : 3] != 0) {
        return luaL_error(L, "invalid settings string \"%s\"", settings);
    }

    hc::filter::Operator op = hc::filter::Operator::NotEqual;
    uint8_t const op1 = op_str[0];
    uint8_t const op2 = op1 != 0 ? op_str[1] : 0;

    switch (op1 << 8 | op2) {
        case '<' << 8 | 0:   /* < */  op = hc::filter::Operator::LessThan; break;
        case '<' << 8 | '=': /* <= */ op = hc::filter::Operator::LessEqual; break;
        case '>' << 8 | 0:   /* > */  op = hc::filter::Operator::GreaterThan; break;
        case '>' << 8 | '=': /* >= */ op = hc::filter::Operator::GreaterEqual; break;
        case '=' << 8 | '=': /* == */ op = hc::filter::Operator::Equal; break;
        case '~' << 8 | '=': /* ~= */ op = hc::filter::Operator::NotEqual; break;
        default: return luaL_error(L, "unknown operator %s", op_str);
    }

    hc::Set* result = nullptr;

    if (lua_isnumber(L, 3)) {
        if (is_signed) {
            result = hc::filter::fsigned(*hc::Memory::check(L, 1), lua_tointeger(L, 3), op, endianess, value_size);
        }
        else {
            result = hc::filter::funsigned(*hc::Memory::check(L, 1), lua_tointeger(L, 3), op, endianess, value_size);
        }
    }
    else {
        if (is_signed) {
            result = hc::filter::fsigned(*hc::Memory::check(L, 1), *hc::Memory::check(L, 3), op, endianess, value_size);
        }
        else {
            result = hc::filter::funsigned(*hc::Memory::check(L, 1), *hc::Memory::check(L, 3), op, endianess, value_size);
        }
    }

    return result->push(L);
}

int hc::cheats::push(lua_State* const L) {
    static const luaL_Reg functions[] = {
        {"empty", l_empty},
        {"universal", l_universal},
        {"filter", l_filter},
        {nullptr, nullptr}
    };

    luaL_newlib(L, functions);

    int const res = luaL_loadbufferx(L, Cheats_lua, sizeof(Cheats_lua), "Cheats.lua", "t");

    if (res != LUA_OK) {
        //_desktop->error("%s", lua_tostring(L, -1));
        lua_pop(L, 2);
        return 0;
    }

    lua_call(L, 0, 1);
    lua_pushvalue(L, -2);
    lua_call(L, 1, 1);
    s_onFrame = luaL_ref(L, LUA_REGISTRYINDEX);

    return 1;
}

void hc::cheats::onFrame() {

}
