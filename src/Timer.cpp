#include "Timer.h"

#include "Perf.h"

hc::Timer::Timer() {
    stop();
}

void hc::Timer::start() {
    _started = true;
    _paused = false;

    _startTicks = Perf::getTimeUs();
    _pausedTicks = 0;
}

void hc::Timer::stop() {
    _started = false;
    _paused = false;

    _startTicks = 0;
    _pausedTicks = 0;
}

void hc::Timer::pause() {
    if (_started && !_paused) {
        _paused = true;

        _pausedTicks = Perf::getTimeUs() - _startTicks;
        _startTicks = 0;
    }
}

void hc::Timer::resume() {
    if (_started && _paused) {
        _paused = false;

        _startTicks = Perf::getTimeUs() - _pausedTicks;
        _pausedTicks = 0;
    }
}

uint64_t hc::Timer::getTimeUs() const {
    if (_started) {
        if (_paused) {
            return _pausedTicks;
        }
        else {
            return Perf::getTimeUs() - _startTicks;
        }
    }

    return 0;
}
