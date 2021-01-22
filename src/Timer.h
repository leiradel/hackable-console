#pragma once

#include <stdint.h>

namespace hc {
    class Timer {
    public:
        Timer();

        void start();
        void stop();
        void pause();
        void resume();

        uint64_t getTimeUs() const;

        bool started() const { return _started; }
        bool paused() const { return _paused && _started; }

    private:
        uint64_t _startTicks;
        uint64_t _pausedTicks;

        bool _started;
        bool _paused;
    };
}
