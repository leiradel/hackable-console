#pragma once

#include <stdint.h>
#include <vector>

namespace hc {
    template<typename T>
    class Handle {
    public:
        Handle() : offset(0), counter(0) {}
        Handle(uint32_t offset, uint32_t counter) : offset(offset), counter(counter) {}

        Handle& operator=(Handle const& other) { offset = other.offset; counter = other.counter; return *this; }
        bool null() const { return offset == 0; }

    protected:
        template<typename U> friend class HandleAllocator;

        uint32_t offset;
        uint32_t counter;
    };

    template<typename T>
    class HandleAllocator;

    template<typename T>
    class FullHandle {
    public:
        T* translate() const { return _translator->translate(_handle); }

    protected:
        template<typename U> friend class HandleAllocator;

        Handle<T> _handle;
        HandleAllocator<T> const* _translator;
    };

    template<typename T>
    class HandleAllocator {
    public:
        HandleAllocator() : _freeList(0) {}

        template<typename... A>
        Handle<T> allocate(A&&... args) {
            uint32_t offset = 0;

            if (_freeList != 0) {
                offset = _freeList - 1;
                _freeList = _elements[offset].next;
            }
            else {
                offset = static_cast<uint32_t>(_elements.size());
                _elements.resize(offset + 1);
                _counters.resize(offset + 1);
                _counters[offset] = 0;
            }

            new (_elements[offset].element) T(args...);
            return Handle<T>(offset + 1, _counters[offset]);
        }

        void free(Handle<T> const handle) {
            T* const element = translate(handle);

            if (element != nullptr) {
                element->~T();

                _counters[handle.offset - 1]++;
                _elements[handle.offset - 1].next = _freeList;
                _freeList = handle.offset;
            }
        }

        // Translated pointers can become invalid after a call to allocate
        T* translate(Handle<T> const handle) const {
            uint32_t const offset = handle.offset - 1;

            if (offset < _elements.size() && handle.counter == _counters[offset]) {
                return reinterpret_cast<T*>(const_cast<uint8_t*>(_elements[offset].element));
            }

            return nullptr;
        }

        FullHandle<T> full(Handle<T> const handle) const {
            FullHandle<T> fullHandle;
            fullHandle._handle = handle;
            fullHandle._translator = this;
            return fullHandle;
        }

        void reset() {
            for (auto& counter : _counters) {
                counter++;
            }
        }

    protected:
        union alignas(T) Element {
            uint8_t element[sizeof(T)];
            uint32_t next;
        };

        std::vector<Element> _elements;
        std::vector<uint32_t> _counters;
        uint32_t _freeList;
    };
}
