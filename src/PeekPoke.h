#pragma once

#include "Bitcast.h"

#include <stdint.h>

namespace hc {
    template<typename T>
    class MemoryPeek {
    public:
        uint8_t peekU8(uint64_t address) const {
            return static_cast<const T*>(this)->peek(address);
        }

        int8_t peekI8(uint64_t address) const {
            return bitcast<int8_t>(peekU8(address));
        }

        uint16_t peekU16LE(uint64_t address) const {
            uint16_t low = peekU8(address);
            uint16_t high = peekU8(address + 1);
            return high << 8 | low;
        }

        int16_t peekI16LE(uint64_t address) const {
            return bitcast<int16_t>(peekU16LE(address));
        }

        uint16_t peekU16BE(uint64_t address) const {
            uint16_t high = peekU8(address);
            uint16_t low = peekU8(address + 1);
            return high << 8 | low;
        }

        int16_t peekI16BE(uint64_t address) const {
            return bitcast<int16_t>(peekU16BE(address));
        }

        uint32_t peekU32LE(uint64_t address) const {
            uint32_t low = peekU16LE(address);
            uint32_t high = peekU16LE(address + 2);
            return high << 16 | low;
        }

        int32_t peekI32LE(uint64_t address) const {
            return bitcast<int32_t>(peekU32LE(address));
        }

        uint32_t peekU32BE(uint64_t address) const {
            const uint32_t high = peekU16BE(address);
            const uint32_t low = peekU16BE(address + 2);
            return high << 16 | low;
        }

        int32_t peekI32BE(uint64_t address) const {
            return bitcast<int32_t>(peekU32BE(address));
        }

        uint64_t peekU64LE(uint64_t address) const {
            uint64_t low = peekU32LE(address);
            uint64_t high = peekU32LE(address + 4);
            return high << 32 | low;
        }

        int64_t peekI64LE(uint64_t address) const {
            return bitcast<int64_t>(peekU64LE(address));
        }

        uint64_t peekU64BE(uint64_t address) const {
            uint64_t high = peekU32BE(address);
            uint64_t low = peekU32BE(address + 4);
            return high << 32 | low;
        }

        int64_t peekI64BE(uint64_t address) const {
            return bitcast<int64_t>(peekU64BE(address));
        }

    private:
        MemoryPeek() {}
        friend T;
    };

    template<typename T>
    class MemoryPoke {
    public:
        void pokeU8(uint64_t address, uint8_t value) {
            static_cast<T*>(this)->poke(address, value);
        }

        void pokeI8(uint64_t address, int8_t value) {
            pokeU8(address, bitcast<uint8_t>(value));
        }

        void pokeU16LE(uint64_t address, uint16_t value) {
            pokeU8(address, value);
            pokeU8(address + 1, value >> 8);
        }

        void pokeI16LE(uint64_t address, int16_t value) {
            pokeU16LE(address, bitcast<uint16_t>(value));
        }

        void pokeU16BE(uint64_t address, uint16_t value) {
            pokeU8(address, value >> 8);
            pokeU8(address + 1, value);
        }

        void pokeI16BE(uint64_t address, int16_t value) {
            pokeU16BE(address, bitcast<uint16_t>(value));
        }

        void pokeU32LE(uint64_t address, uint32_t value) {
            pokeU16LE(address, value);
            pokeU16LE(address + 2, value >> 16);
        }

        void pokeI32LE(uint64_t address, int32_t value) {
            pokeU32LE(address, bitcast<uint32_t>(value));
        }

        void pokeU32BE(uint64_t address, uint32_t value) {
            pokeU16BE(address, value >> 16);
            pokeU16BE(address + 2, value);
        }

        void pokeI32BE(uint64_t address, int32_t value) {
            pokeU32BE(address, bitcast<uint32_t>(value));
        }

        void pokeU64LE(uint64_t address, uint64_t value) {
            pokeU32LE(address, value);
            pokeU32LE(address + 4, value >> 32);
        }

        void pokeI64LE(uint64_t address, int64_t value) {
            pokeU64LE(address, bitcast<uint64_t>(value));
        }

        void pokeU64BE(uint64_t address, uint64_t value) {
            pokeU32BE(address, value >> 32);
            pokeU32BE(address + 4, value);
        }

        void pokeI64BE(uint64_t address, int64_t value) {
            pokeU64BE(address, bitcast<uint64_t>(value));
        }

    private:
        MemoryPoke() {}
        friend T;
    };
}
