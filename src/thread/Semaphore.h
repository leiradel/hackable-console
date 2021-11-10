#pragma once

#include <stddef.h>

#include <mutex>
#include <condition_variable>

namespace hc {
    class Semaphore {
    public:
        Semaphore(ptrdiff_t count): _count(count) {}

        void release() {
            std::lock_guard<std::mutex> lock(_mutex);
            _count++;
            _condition.notify_one();
        }

        void acquire() {
            std::unique_lock<std::mutex> lock(_mutex);

            while (_count == 0) {
                _condition.wait(lock);
            }

            _count--;
        }

    protected:
        std::mutex _mutex;
        std::condition_variable _condition;
        ptrdiff_t _count;
    };
}
