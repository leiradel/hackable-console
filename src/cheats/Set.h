#pragma once

#include "Scriptable.h"

extern "C" {
    #include <lua.h>
}

#include <vector>
#include <algorithm>
#include <stdint.h>

namespace hc {
    class Set : public Scriptable {
    public:
        size_t size() const { return _elements.size(); }
        size_t size(size_t universal_size) const { return _complemented ? universal_size - _elements.size() : _elements.size(); }
        bool complemented() const { return _complemented; }

        void add(uint64_t element) { _elements.emplace_back(element); }
        bool contains(uint64_t element) const;

        Set* union_(Set const* other) const;
        Set* intersection(Set const* other) const;
        Set* difference(Set const* other) const;
        Set* complement() const;

        static Set* empty();
        static Set* universal();

        std::vector<uint64_t>::const_iterator begin() const { return _elements.begin(); }
        std::vector<uint64_t>::const_iterator end() const { return _elements.end(); }

        static Set* check(lua_State* L, int index);

        // hc::Scriptable
        int push(lua_State* L);

    protected:
        Set();

        static int l_size(lua_State* const L);
        static int l_contains(lua_State* const L);
        static int l_union(lua_State* const L);
        static int l_intersection(lua_State* const L);
        static int l_difference(lua_State* const L);
        static int l_complement(lua_State* const L);
        static int l_elements(lua_State* const L);
        static int l_asTable(lua_State* const L);
        static int l_collect(lua_State* const L);

        std::vector<uint64_t> _elements;
        bool _complemented;
    };
}
