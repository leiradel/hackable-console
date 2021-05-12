#pragma once

#include "Memory.h"
#include "Scriptable.h"

extern "C" {
    #include <lua.h>
}

#include <stddef.h>
#include <stdint.h>
#include <string>

namespace hc {
    class Snapshot : public Memory {
    public:
        Snapshot(Memory* memory);
        Snapshot(uint64_t baseAddress, uint64_t size, void const* data, Memory* memory);
        virtual ~Snapshot();

        Memory const* memory() const { return _memory; }

        static Snapshot* check(lua_State* L, int index);
        static bool is(lua_State* L, int index);

        // hc::Memory
        virtual char const* id() const override { return _id.c_str(); }
        virtual char const* name() const override { return _name.c_str(); }
        virtual uint64_t base() const override { return _baseAddress; }
        virtual uint64_t size() const override { return _size; }
        virtual bool readonly() const override { return true; }
        virtual uint8_t peek(uint64_t address) const override;
        virtual void poke(uint64_t address, uint8_t value) override { (void)address; (void)value; }

        // Scriptable
        virtual int push(lua_State* L) override;

    protected:
        static int l_memory(lua_State* L);
        static int l_collect(lua_State* L);

        std::string const _id;
        std::string const _name;
        uint64_t const _baseAddress;
        uint64_t const _size;
        void const* const _data;
        Memory* const _memory;
    };
}
