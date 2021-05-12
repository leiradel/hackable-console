#include "cheats/Filter.h"

#include "Bitcast.h"
#include "Memory.h"
#include "cheats/Snapshot.h"
#include "cheats/Set.h"

extern "C" {
    #include <lauxlib.h>
}

template<typename A, typename T, hc::filter::Endianess E>
static T first(A const a, uint64_t address) {
    if (sizeof(T) == 1) {
        uint8_t value = a.peek(address);
        return std::is_signed<T>() ? hc::bitcast<int8_t>(value) : value;
    }
    else if (sizeof(T) == 2) {
        uint16_t const b0 = a.peek(address);
        uint16_t const b1 = a.peek(address + 1);
        uint16_t value = 0;

        if (E == hc::filter::Endianess::Little) {
            value = b0 | b1 << 8;
        }
        else {
            value = b0 << 8 | b1;
        }

        return std::is_signed<T>() ? hc::bitcast<int16_t>(value) : value;
    }
    else if (sizeof(T) == 4) {
        uint32_t const b0 = a.peek(address);
        uint32_t const b1 = a.peek(address + 1);
        uint32_t const b2 = a.peek(address + 2);
        uint32_t const b3 = a.peek(address + 3);
        uint32_t value = 0;

        if (E == hc::filter::Endianess::Little) {
            value = b0 | b1 << 8 | b2 << 16 | b3 << 24;
        }
        else {
            value = b0 << 24 | b1 << 16 | b2 << 8 | b3;
        }

        return std::is_signed<T>() ? hc::bitcast<int32_t>(value) : value;
    }
    else if (sizeof(T) == 8) {
        uint64_t const b0 = a.peek(address);
        uint64_t const b1 = a.peek(address + 1);
        uint64_t const b2 = a.peek(address + 2);
        uint64_t const b3 = a.peek(address + 3);
        uint64_t const b4 = a.peek(address + 4);
        uint64_t const b5 = a.peek(address + 5);
        uint64_t const b6 = a.peek(address + 6);
        uint64_t const b7 = a.peek(address + 7);
        uint64_t value = 0;

        if (E == hc::filter::Endianess::Little) {
            value = b0 | b1 << 8 | b2 << 16 | b3 << 24 | b4 << 32 | b5 << 40 | b6 << 48 | b7 << 56;
        }
        else {
            value = b0 << 56 | b1 << 48 | b2 << 40 | b3 << 32 | b4 << 24 | b5 << 16 | b6 << 8 | b7;
        }

        return std::is_signed<T>() ? hc::bitcast<int64_t>(value) : value;
    }

    return static_cast<T>(0);
}

template<typename A, typename T, hc::filter::Endianess E>
static T first(int64_t a, uint64_t address) {
    (void)address;
    return static_cast<T>(a);
}

template<typename A, typename T, hc::filter::Endianess E>
static T first(uint64_t a, uint64_t address) {
    (void)address;
    return static_cast<T>(a);
}

template<typename A, typename T, hc::filter::Endianess E>
static T next(T const current, A const a, uint64_t address) {
    if (sizeof(T) == 1) {
        uint8_t const next = a.peek(address);
        return std::is_signed<T>() ? hc::bitcast<int8_t>(next) : next;
    }
    else if (sizeof(T) == 2) {
        uint16_t const value = std::is_signed<T>() ? hc::bitcast<uint16_t, int16_t>(current) : current;
        uint16_t const byte = a.peek(address + 1);
        uint16_t next = 0;

        if (E == hc::filter::Endianess::Little) {
            next = value >> 8 | byte << 8;
        }
        else {
            next = value << 8 | byte;
        }

        return std::is_signed<T>() ? hc::bitcast<int16_t>(next) : next;
    }
    else if (sizeof(T) == 4) {
        uint32_t const value = std::is_signed<T>() ? hc::bitcast<uint32_t, int32_t>(current) : current;
        uint32_t const byte = a.peek(address + 3);
        uint32_t next = 0;

        if (E == hc::filter::Endianess::Little) {
            next = value >> 8 | byte << 24;
        }
        else {
            next = value << 8 | byte;
        }

        return std::is_signed<T>() ? hc::bitcast<int32_t>(next) : next;
    }
    else if (sizeof(T) == 8) {
        uint64_t const value = std::is_signed<T>() ? hc::bitcast<uint64_t, int64_t>(current) : current;
        uint64_t const byte = a.peek(address + 7);
        uint64_t next = 0;

        if (E == hc::filter::Endianess::Little) {
            next = value >> 8 | byte << 56;
        }
        else {
            next = value << 8 | byte;
        }

        return std::is_signed<T>() ? hc::bitcast<int64_t>(next) : next;
    }

    return static_cast<T>(0);
}

template<typename A, typename T, hc::filter::Endianess E>
static T next(T const current, int64_t a, uint64_t address) {
    (void)a;
    (void)address;
    return current;
}

template<typename A, typename T, hc::filter::Endianess E>
static T next(T const current, uint64_t a, uint64_t address) {
    (void)a;
    (void)address;
    return current;
}

template<typename T, hc::filter::Operator O>
static bool compare(const T v1, const T v2) {
    switch (O) {
        case hc::filter::Operator::LessThan: return v1 < v2;
        case hc::filter::Operator::LessEqual: return v1 <= v2;
        case hc::filter::Operator::GreaterThan: return v1 > v2;
        case hc::filter::Operator::GreaterEqual: return v1 >= v2;
        case hc::filter::Operator::Equal: return v1 == v2;
        case hc::filter::Operator::NotEqual: return v1 != v2;
    }
}

template<typename A, typename B, typename T, hc::filter::Endianess E, hc::filter::Operator O>
static hc::Set* doFilter(A const a, B const b) {
    hc::Set* result = hc::Set::empty();

    uint64_t address = a.base();
    uint64_t const size = a.size();

    if (size < sizeof(T)) {
        return result;
    }

    uint64_t const end = address + size - sizeof(T) + 1;

    T current1 = first<A, T, E>(a, address);
    T current2 = first<B, T, E>(b, address);

    for (;;) {
        if (compare<T, O>(current1, current2)) {
            result->add(address);
        }

        address++;

        if (address >= end) {
            break;
        }

        current1 = next<A, T, E>(current1, a, address);
        current2 = next<B, T, E>(current2, b, address);
    }

    return result;
}

template<typename A, typename B, typename T, hc::filter::Endianess E>
static hc::Set* doFilter(A const a, B const b, hc::filter::Operator op) {
    switch (op) {
        case hc::filter::Operator::LessThan:
            return doFilter<A, B, T, E, hc::filter::Operator::LessThan>(a, b);

        case hc::filter::Operator::LessEqual:
            return doFilter<A, B, T, E, hc::filter::Operator::LessEqual>(a, b);

        case hc::filter::Operator::GreaterThan:
            return doFilter<A, B, T, E, hc::filter::Operator::GreaterThan>(a, b);

        case hc::filter::Operator::GreaterEqual:
            return doFilter<A, B, T, E, hc::filter::Operator::GreaterEqual>(a, b);

        case hc::filter::Operator::Equal:
            return doFilter<A, B, T, E, hc::filter::Operator::Equal>(a, b);

        case hc::filter::Operator::NotEqual:
            return doFilter<A, B, T, E, hc::filter::Operator::NotEqual>(a, b);
    }

    return nullptr;
}

template<typename A, typename B, typename T>
static hc::Set* doFilter(A const a, B const b, hc::filter::Operator op, hc::filter::Endianess endianess) {
    switch (endianess) {
        case hc::filter::Endianess::Little:
            return doFilter<A, B, T, hc::filter::Endianess::Little>(a, b, op);

        case hc::filter::Endianess::Big:
            return doFilter<A, B, T, hc::filter::Endianess::Big>(a, b, op);
    }

    return nullptr;
}

template<typename A, typename B>
static hc::Set* doFilterSigned(A const a, B const b, hc::filter::Operator op, hc::filter::Endianess endianess, size_t value_size) {
    switch (value_size) {
        case 1: return doFilter<A, B, int8_t>(a, b, op, endianess);
        case 2: return doFilter<A, B, int16_t>(a, b, op, endianess);
        case 4: return doFilter<A, B, int32_t>(a, b, op, endianess);
        case 8: return doFilter<A, B, int64_t>(a, b, op, endianess);
    }

    return nullptr;
}

template<typename A, typename B>
static hc::Set* doFilterUnsigned(A const a, B const b, hc::filter::Operator op, hc::filter::Endianess endianess, size_t value_size) {
    switch (value_size) {
        case 1: return doFilter<A, B, uint8_t>(a, b, op, endianess);
        case 2: return doFilter<A, B, uint16_t>(a, b, op, endianess);
        case 4: return doFilter<A, B, uint32_t>(a, b, op, endianess);
        case 8: return doFilter<A, B, uint64_t>(a, b, op, endianess);
    }

    return nullptr;
}

hc::Set* hc::filter::fsigned(Memory const& memory, int64_t value, Operator op, Endianess endianess, size_t value_size) {
    return doFilterSigned<Memory const&, int64_t>(memory, value, op, endianess, value_size);
}

hc::Set* hc::filter::fsigned(Memory const& memory1, Memory const& memory2, Operator op, Endianess endianess, size_t value_size) {
    if (memory1.base() != memory2.base() || memory1.size() != memory2.size()) {
        return nullptr;
    }

    return doFilterSigned<Memory const&, Memory const&>(memory1, memory2, op, endianess, value_size);
}

hc::Set* hc::filter::funsigned(Memory const& memory, uint64_t value, Operator op, Endianess endianess, size_t value_size) {
    return doFilterUnsigned<Memory const&, uint64_t>(memory, value, op, endianess, value_size);
}

hc::Set* hc::filter::funsigned(Memory const& memory1, Memory const& memory2, Operator op, Endianess endianess, size_t value_size) {
    if (memory1.base() != memory2.base() || memory1.size() != memory2.size()) {
        return nullptr;
    }

    return doFilterUnsigned<Memory const&, Memory const&>(memory1, memory2, op, endianess, value_size);
}
