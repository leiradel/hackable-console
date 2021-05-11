#include "Snapshot.h"
#include "Memory.h"

#include <inttypes.h>
#include <sys/time.h>

extern "C" {
    #include <lauxlib.h>
}

static std::string createName(char const* memory_name) {
    struct timeval tv;
    gettimeofday(&tv, nullptr);

    struct tm tm;
    gmtime_r(&tv.tv_sec, &tm);

    char ts1[32];
    strftime(ts1, sizeof(ts1), "%Y-%m-%dT%H:%M:%S", &tm);

    char ts2[64];
    snprintf(ts2, sizeof(ts2), "%s.%06ldZ", ts1, tv.tv_usec);

    std::string name("Snapshot of ");
    name += std::string(memory_name);
    name += " created on ";
    name += ts2;
    return name;
}

static void* snapshot(hc::Memory* const memory) {
    uint8_t* const data = new uint8_t[memory->size()];
    uint64_t const end = memory->base() + memory->size();

    for (uint64_t i = 0, address = memory->base(); address < end; i++, address++) {
        data[i] = memory->peek(address);
    }

    return data;
}

hc::Snapshot::Snapshot(Memory* memory) : Snapshot(memory->base(), memory->size(), snapshot(memory), memory) {}

hc::Snapshot::Snapshot(uint64_t baseAddress, uint64_t size, void const* data, Memory* memory)
    : _name(createName(memory->name()))
    , _baseAddress(baseAddress)
    , _size(size)
    , _data(data)
    , _memory(memory)
{}

hc::Snapshot::~Snapshot() {
    free(const_cast<void*>(_data));
}

uint8_t hc::Snapshot::peek(uint64_t address) const {
    uint64_t addr = address - _baseAddress;

    if (addr < _size) {
        auto data = static_cast<uint8_t const*>(_data);
        uint8_t byte = data[addr];
        return byte;
    }

    return 0;
}

#define SNAPSHOT_MT "Snapshot"

int hc::Snapshot::push(lua_State* L) {
    Snapshot** const self = static_cast<Snapshot**>(lua_newuserdata(L, sizeof(*self)));
    *self = this;

    if (luaL_newmetatable(L, SNAPSHOT_MT)) {
        static const luaL_Reg methods[] = {
            {"name", l_name},
            {"base", l_base},
            {"size", l_size},
            {"readonly", l_readonly},
            {"peek", l_peek},
            {"poke", l_poke},
            {"memory", l_memory},
            {NULL, NULL}
        };

        luaL_newlib(L, methods);
        lua_setfield(L, -2, "__index");

        lua_pushcfunction(L, l_collect);
        lua_setfield(L, -2, "__gc");
    }

    lua_setmetatable(L, -2);
    return 1;
}

hc::Snapshot* hc::Snapshot::check(lua_State* L, int index) {
    return *static_cast<Snapshot**>(luaL_checkudata(L, index, SNAPSHOT_MT));
}

bool hc::Snapshot::is(lua_State* L, int index) {
    return luaL_testudata(L, index, SNAPSHOT_MT) != nullptr;
}

int hc::Snapshot::l_memory(lua_State* L) {
    auto self = check(L, 1);
    return self->_memory->push(L);
}

int hc::Snapshot::l_collect(lua_State* L) {
    auto self = *static_cast<Snapshot**>(lua_touserdata(L, 1));
    delete self;
    return 0;
}
