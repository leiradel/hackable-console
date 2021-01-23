#pragma once

#include <stdint.h>

namespace hc {
    // https://lazyfoo.net/tutorials/SDL/24_calculating_frame_rate/index.php
    class Timer {
    public:
        Timer();

        void start();
        void stop();
        void pause();
        void resume();
        void reset();

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
