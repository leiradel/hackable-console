#pragma once

#include <stddef.h>
#include <mutex>
#include <queue>

namespace hc {
    template<typename T>
    class Queue {
    public:
        void put(T* const data) {
            std::lock_guard<std::mutex> lock(_mutex);
            _elements.emplace(std::move(*data));
        }

        void get(T* const data) {
            std::lock_guard<std::mutex> lock(_mutex);
            *data = std::move(_elements.front());
            _elements.pop();
        }

        size_t count() {
            std::lock_guard<std::mutex> lock(_mutex);
            return _elements.size();
        }

    protected:
        std::mutex _mutex;
        std::queue<T> _elements;
    };
}
