#pragma once

#include <SDL.h>
#include <stddef.h>

namespace hc {
    class Fifo final {
    public:
        bool init(size_t const size);
        void destroy();
        void reset();

        void read(void* const data, size_t const size);
        void write(void const* const data, size_t const size);

        size_t size() { return _size; }

        size_t occupied();
        size_t free();

    protected:
        SDL_mutex* _mutex;
        uint8_t*   _buffer;
        size_t     _size;
        size_t     _avail;
        size_t     _first;
        size_t     _last;
    };
}
