#include "cheats/Set.h"

#include <inttypes.h>

#include <atomic>
#include <algorithm>
#include <iterator>

extern "C" {
    #include "lauxlib.h"
}

hc::Set::Set() : _complemented(false) {}

bool hc::Set::contains(uint64_t element) const {
    bool const contains = std::binary_search(_elements.begin(), _elements.end(), element);
    return _complemented ? !contains : contains;
}

hc::Set* hc::Set::union_(Set const* other) const {
    if (!_complemented && !other->_complemented) {
        // A + B
        Set* result = new Set;
        result->_elements.reserve(_elements.size() + other->_elements.size());

        std::set_union(
            _elements.begin(),
            _elements.end(),
            other->_elements.begin(),
            other->_elements.end(),
            std::inserter(result->_elements, result->_elements.begin())
        );

        result->_elements.shrink_to_fit();
        return result;
    }
    else if (!_complemented && other->_complemented) {
        // A + ~B = ~(B - A)
        // https://www.wolframalpha.com/input/?i=is+A+union+B%27+%3D+%28B+difference+A%29%27
        Set* result = new Set;
        result->_elements.reserve(_elements.size());

        std::set_difference(
            other->_elements.begin(),
            other->_elements.end(),
            _elements.begin(),
            _elements.end(),
            std::inserter(result->_elements, result->_elements.begin())
        );

        result->_elements.shrink_to_fit();
        result->_complemented = true;
        return result;
    }
    else if (_complemented && !other->_complemented) {
        // ~A + B = ~(A - B)
        // https://www.wolframalpha.com/input/?i=is+A%27+union+B+%3D+%28A+difference+B%29%27
        Set* result = new Set;
        result->_elements.reserve(_elements.size());

        std::set_difference(
            _elements.begin(),
            _elements.end(),
            other->_elements.begin(),
            other->_elements.end(),
            std::inserter(result->_elements, result->_elements.begin())
        );

        result->_elements.shrink_to_fit();
        result->_complemented = true;
        return result;
    }
    else {
        // ~A + ~B = ~(A * B)
        // https://www.wolframalpha.com/input/?i=is+A%27+union+B%27+%3D+%28A+intersects+B%29%27
        Set* result = new Set;
        result->_elements.reserve(std::min(_elements.size(), other->_elements.size()));

        std::set_intersection(
            _elements.begin(),
            _elements.end(),
            other->_elements.begin(),
            other->_elements.end(),
            std::inserter(result->_elements, result->_elements.begin())
        );

        result->_elements.shrink_to_fit();
        result->_complemented = true;
        return result;
    }

    return nullptr;
}

hc::Set* hc::Set::intersection(Set const* other) const {
    if (!_complemented && !other->_complemented) {
        // A * B
        Set* result = new Set;
        result->_elements.reserve(std::min(_elements.size(), other->_elements.size()));

        std::set_intersection(
            _elements.begin(),
            _elements.end(),
            other->_elements.begin(),
            other->_elements.end(),
            std::inserter(result->_elements, result->_elements.begin())
        );

        result->_elements.shrink_to_fit();
        return result;
    }
    else if (!_complemented && other->_complemented) {
        // A * ~B = A - B
        // https://www.wolframalpha.com/input/?i=is+A+intersect+B%27+%3D+A+difference+B
        Set* result = new Set;
        result->_elements.reserve(_elements.size());

        std::set_difference(
            _elements.begin(),
            _elements.end(),
            other->_elements.begin(),
            other->_elements.end(),
            std::inserter(result->_elements, result->_elements.begin())
        );

        result->_elements.shrink_to_fit();
        return result;
    }
    else if (_complemented && !other->_complemented) {
        // ~A * B = B - A
        // https://www.wolframalpha.com/input/?i=is+A%27+intersects+B+%3D+B+difference+A
        Set* result = new Set;
        result->_elements.reserve(_elements.size());

        std::set_difference(
            other->_elements.begin(),
            other->_elements.end(),
            _elements.begin(),
            _elements.end(),
            std::inserter(result->_elements, result->_elements.begin())
        );

        result->_elements.shrink_to_fit();
        return result;
    }
    else {
        // ~A * ~B = ~(A + B)
        // https://www.wolframalpha.com/input/?i=is+A%27+intersect+B%27+%3D+%28A+union+B%29%27
        Set* result = new Set;
        result->_elements.reserve(_elements.size() + other->_elements.size());

        std::set_union(
            _elements.begin(),
            _elements.end(),
            other->_elements.begin(),
            other->_elements.end(),
            std::inserter(result->_elements, result->_elements.begin())
        );

        result->_elements.shrink_to_fit();
        result->_complemented = true;
        return result;
    }

    return nullptr;
}

hc::Set* hc::Set::difference(Set const* other) const {
    if (!_complemented && !other->_complemented) {
        // A - B
        Set* result = new Set;
        result->_elements.reserve(_elements.size());

        std::set_difference(
            _elements.begin(),
            _elements.end(),
            other->_elements.begin(),
            other->_elements.end(),
            std::inserter(result->_elements, result->_elements.begin())
        );

        result->_elements.shrink_to_fit();
        return result;
    }
    else if (!_complemented && other->_complemented) {
        // A - ~B = A * B
        // https://www.wolframalpha.com/input/?i=is+A+difference+B%27+%3D+A+intersect+B
        Set* result = new Set;
        result->_elements.reserve(std::min(_elements.size(), other->_elements.size()));

        std::set_intersection(
            _elements.begin(),
            _elements.end(),
            other->_elements.begin(),
            other->_elements.end(),
            std::inserter(result->_elements, result->_elements.begin())
        );

        result->_elements.shrink_to_fit();
        return result;
    }
    else if (_complemented && !other->_complemented) {
        // ~A - B = ~(A + B)
        // https://www.wolframalpha.com/input/?i=is+A%27+difference+B+%3D+%28A+union+B%29%27
        Set* result = new Set;
        result->_elements.reserve(_elements.size() + other->_elements.size());

        std::set_union(
            _elements.begin(),
            _elements.end(),
            other->_elements.begin(),
            other->_elements.end(),
            std::inserter(result->_elements, result->_elements.begin())
        );

        result->_elements.shrink_to_fit();
        result->_complemented = true;
        return result;
    }
    else {
        // ~A - ~B = B - A
        // https://www.wolframalpha.com/input/?i=is+A%27+difference+B%27+%3D+B+difference+A
        Set* result = new Set;
        result->_elements.reserve(_elements.size());

        std::set_difference(
            other->_elements.begin(),
            other->_elements.end(),
            _elements.begin(),
            _elements.end(),
            std::inserter(result->_elements, result->_elements.begin())
        );

        result->_elements.shrink_to_fit();
        return result;
    }

    return nullptr;
}

hc::Set* hc::Set::complement() const {
    Set* result = new Set;
    result->_elements = _elements;
    result->_complemented = !_complemented;
    return result;
}

hc::Set* hc::Set::empty() {
    Set* result = new Set;
    result->_complemented = false;
    return result;
}

hc::Set* hc::Set::universal() {
    Set* result = new Set;
    result->_complemented = true;
    return result;
}

#define SET_MT "Set"

hc::Set* hc::Set::check(lua_State* L, int index) {
    return *static_cast<Set**>(luaL_checkudata(L, index, SET_MT));
}

int hc::Set::push(lua_State* const L) {
    Set** const self = static_cast<Set**>(lua_newuserdata(L, sizeof(*self)));
    *self = this;

    if (luaL_newmetatable(L, SET_MT)) {
        static const luaL_Reg methods[] = {
            {"size", l_size},
            {"contains", l_contains},
            {"union", l_union},
            {"intersection", l_intersection},
            {"difference", l_difference},
            {"complement", l_complement},
            {"elements", l_elements},
            {"asTable", l_asTable},
            {NULL, NULL}
        };

        luaL_newlib(L, methods);
        lua_setfield(L, -2, "__index");

        lua_pushcfunction(L, l_union);
        lua_setfield(L, -2, "__add");

        lua_pushcfunction(L, l_intersection);
        lua_setfield(L, -2, "__mul");

        lua_pushcfunction(L, l_difference);
        lua_setfield(L, -2, "__sub");

        lua_pushcfunction(L, l_complement);
        lua_setfield(L, -2, "__unm");

        lua_pushcfunction(L, l_collect);
        lua_setfield(L, -2, "__gc");
    }

    lua_setmetatable(L, -2);
    return 1;
}

int hc::Set::l_size(lua_State* L) {
    auto self = check(L, 1);
    lua_pushinteger(L, self->size());
    return 1;
}

int hc::Set::l_contains(lua_State* L) {
    auto self = check(L, 1);
    lua_Integer element = luaL_checkinteger(L, 2);
    lua_pushboolean(L, self->contains(element));
    return 1;
}

int hc::Set::l_union(lua_State* L) {
    auto self = check(L, 1);
    auto other = check(L, 2);
    auto result = self->union_(other);
    return result->push(L);
}

int hc::Set::l_intersection(lua_State* L) {
    auto self = check(L, 1);
    auto other = check(L, 2);
    auto result = self->intersection(other);
    return result->push(L);
}

int hc::Set::l_difference(lua_State* L) {
    auto self = check(L, 1);
    auto other = check(L, 2);
    auto result = self->difference(other);
    return result->push(L);
}

int hc::Set::l_complement(lua_State* L) {
    auto self = check(L, 1);
    auto result = self->complement();
    return result->push(L);
}

int hc::Set::l_elements(lua_State* L) {
    static auto const next = [](lua_State* L) -> int {
        auto self = *static_cast<Set**>(lua_touserdata(L, 1));
        size_t index = lua_tointeger(L, 2);

        if (index < self->_elements.size()) {
            lua_pushinteger(L, index + 1);
            lua_pushinteger(L, self->_elements[index]);
            return 2;
        }

        lua_pushnil(L);
        return 1;
    };

    check(L, 1);

    lua_pushcfunction(L, next);
    lua_pushvalue(L, 1);
    lua_pushinteger(L, 0);
    return 3;
}

int hc::Set::l_asTable(lua_State* L) {
    auto self = check(L, 1);

    size_t size = self->_elements.size();
    lua_createtable(L, size, 0);

    for (size_t i = 0; i < size; i++) {
        lua_pushinteger(L, self->_elements[i]);
        lua_rawseti(L, -2, i + 1);
    }

    return 1;
}

int hc::Set::l_collect(lua_State* L) {
    auto self = *static_cast<Set**>(lua_touserdata(L, 1));
    delete self;
    return 0;
}
