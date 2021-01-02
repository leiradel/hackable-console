#include "Fifo.h"

bool hc::Fifo::init(size_t const size) {
    _mutex = SDL_CreateMutex();

    if (!_mutex) {
        return false;
    }

    _buffer = (uint8_t*)malloc(size);

    if (_buffer == NULL) {
        SDL_DestroyMutex(_mutex);
        return false;
    }

    _size = _avail = size;
    _first = _last = 0;
    return true;
}

void hc::Fifo::destroy() {
    ::free(_buffer);
    SDL_DestroyMutex(_mutex);
}

void hc::Fifo::reset() {
    _avail = _size;
    _first = _last = 0;
}

void hc::Fifo::read(void* const data, size_t const size) {
    SDL_LockMutex(_mutex);

    size_t first = size;
    size_t second = 0;

    if (first > _size - _first) {
        first = _size - _first;
        second = size - first;
    }

    uint8_t* src = _buffer + _first;
    memcpy(data, src, first);
    memcpy((uint8_t*)data + first, _buffer, second);

    _first = (_first + size) % _size;
    _avail += size;

    SDL_UnlockMutex(_mutex);
}

void hc::Fifo::write(void const* const data, size_t const size) {
    SDL_LockMutex(_mutex);

    size_t first = size;
    size_t second = 0;

    if (first > _size - _last) {
        first = _size - _last;
        second = size - first;
    }

    uint8_t* dest = _buffer + _last;
    memcpy(dest, data, first);
    memcpy(_buffer, (uint8_t*)data + first, second);

    _last = (_last + size) % _size;
    _avail -= size;

    SDL_UnlockMutex(_mutex);
}

size_t hc::Fifo::occupied() {
    SDL_LockMutex(_mutex);
    size_t const occupied = _size - _avail;
    SDL_UnlockMutex(_mutex);

    return occupied;
}

size_t hc::Fifo::free() {
    SDL_LockMutex(_mutex);
    size_t const free = _avail;
    SDL_UnlockMutex(_mutex);

    return free;
}
