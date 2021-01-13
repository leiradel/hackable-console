#include "Perf.h"

#include <inttypes.h>
#include <chrono>

#define TAG "[PER] "

void hc::Perf::init(Logger* logger) {
    _logger = logger;
}

char const* hc::Perf::getName() {
    return "hc::Perf built-in perf plugin";
}

char const* hc::Perf::getVersion() {
    return "0.0.0";
}

char const* hc::Perf::getLicense() {
    return "MIT";
}

char const* hc::Perf::getCopyright() {
    return "Copyright (c) Andre Leiradella";
}

char const* hc::Perf::getUrl() {
    return "https://github.com/leiradel/hackable-console";
}

void hc::Perf::onStarted() {}

void hc::Perf::onConsoleLoaded() {}

void hc::Perf::onGameLoaded() {}

void hc::Perf::onGamePaused() {}

void hc::Perf::onGameResumed() {}

void hc::Perf::onGameReset() {}

void hc::Perf::onFrame() {}

void hc::Perf::onDraw() {}

void hc::Perf::onGameUnloaded() {}

void hc::Perf::onConsoleUnloaded() {
    _counters.clear();
}

void hc::Perf::onQuit() {}

retro_time_t hc::Perf::getTimeUsec() {
    auto const now_us = std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::system_clock::now());
    return static_cast<retro_time_t>(now_us.time_since_epoch().count());
}

uint64_t hc::Perf::getCpuFeatures() {
    // TODO actually probe the CPU for features
    return 0;
}

retro_perf_tick_t hc::Perf::getCounter() {
    auto const now_ns = std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now());
    return static_cast<retro_perf_tick_t>(now_ns.time_since_epoch().count());
}

void hc::Perf::register_(retro_perf_counter* counter) {
    auto found = _counters.find(counter->ident);

    if (found != _counters.end()) {
        counter->start = 0;
        counter->total = 0;
        counter->call_cnt = 0;
        counter->registered = true;

        _counters.insert(std::make_pair(counter->ident, counter));
    }
}

void hc::Perf::start(retro_perf_counter* counter) {
    auto found = _counters.find(counter->ident);

    if (found != _counters.end())     {
        const retro_perf_tick_t tick = getCounter();
        counter->start = tick;
    }
}

void hc::Perf::stop(retro_perf_counter* counter) {
    auto found = _counters.find(counter->ident);

    if (found != _counters.end()) {
        const retro_perf_tick_t tick = getCounter();
        counter->total += tick - counter->start;
        counter->call_cnt++;
    }
}

void hc::Perf::log() {
    for (const auto& pair : _counters) {
        const retro_perf_counter* counter = pair.second;
        _logger->debug(TAG " %20" PRIu64 " %20" PRIu64 " %s", counter->total, counter->call_cnt, counter->ident);
    }
}
