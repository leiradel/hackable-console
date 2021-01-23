#include "Fifo.h"

#include <string.h>

bool hc::Fifo::init(size_t const size) {
    _buffer = (uint8_t*)malloc(size);

    if (_buffer == NULL) {
        return false;
    }

    _size = _avail = size;
    _first = _last = 0;
    return true;
}

void hc::Fifo::destroy() {
    ::free(_buffer);
}

void hc::Fifo::reset() {
    _avail = _size;
    _first = _last = 0;
}

void hc::Fifo::read(void* const data, size_t const size) {
    std::lock_guard<std::mutex> lock(_mutex);

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
}

void hc::Fifo::write(void const* const data, size_t const size) {
    std::lock_guard<std::mutex> lock(_mutex);

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
}

size_t hc::Fifo::occupied() {
    std::lock_guard<std::mutex> lock(_mutex);
    return _size - _avail;
}

size_t hc::Fifo::free() {
    std::lock_guard<std::mutex> lock(_mutex);
    return _avail;
}
