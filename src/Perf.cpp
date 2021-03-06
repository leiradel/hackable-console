#include "Perf.h"
#include "Logger.h"

#include <IconsFontAwesome4.h>

#include <inttypes.h>
#include <chrono>

extern "C" {
    #include "lauxlib.h"
}

#define TAG "[PRF] "

void hc::Perf::init() {}

uint64_t hc::Perf::getTimeUs() {
    auto const now_us = std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::system_clock::now());
    return static_cast<int64_t>(now_us.time_since_epoch().count());
}

uint64_t hc::Perf::getTimeNs() {
    auto const now_ns = std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now());
    return static_cast<uint64_t>(now_ns.time_since_epoch().count());
}

char const* hc::Perf::getTitle() {
    return ICON_FA_TASKS " Perf";
}

void hc::Perf::onDraw() {
    ImGui::Text("       %7.3f (fps) application", _desktop->drawFps());
    ImGui::Text("       %7.3f (fps) game", _desktop->frameFps());

    for (const auto& pair : _counters) {
        Counter const& cnt = pair.second;

        uint64_t const nsPerCall = cnt.counter->call_cnt != 0 ? cnt.counter->total / cnt.counter->call_cnt : 0;
        uint64_t const usPerCall = nsPerCall / 1000;
        unsigned const ms = usPerCall / 1000;
        unsigned const us = usPerCall - ms * 1000;

        ImGui::Text("%6" PRIu64 " %3u.%03u (ms)  %s", cnt.counter->call_cnt, ms, us, cnt.counter->ident);
    }
}

void hc::Perf::onCoreUnloaded() {
    for (auto const& pair : _counters) {
        Counter const& cnt = pair.second;

        if (cnt.mustDelete) {
            free((void*)cnt.counter->ident);
            free(cnt.counter);
        }
    }

    _counters.clear();
}

retro_time_t hc::Perf::getTimeUsec() {
    return static_cast<retro_time_t>(getTimeUs());
}

uint64_t hc::Perf::getCpuFeatures() {
    // TODO actually probe the CPU for features
    return 0;
}

retro_perf_tick_t hc::Perf::getCounter() {
    return static_cast<retro_perf_tick_t>(getTimeNs());
}

void hc::Perf::register_(retro_perf_counter* counter) {
    auto found = _counters.find(counter->ident);

    if (found == _counters.end()) {
        counter->start = 0;
        counter->total = 0;
        counter->call_cnt = 0;
        counter->registered = true;

        Counter cnt = {counter, false};
        _counters.insert(std::make_pair(counter->ident, cnt));
    }
}

void hc::Perf::start(retro_perf_counter* counter) {
    const retro_perf_tick_t tick = getCounter();
    counter->start = tick;
}

void hc::Perf::stop(retro_perf_counter* counter) {
    const retro_perf_tick_t tick = getCounter();
    counter->total += tick - counter->start;
    counter->call_cnt++;
}

void hc::Perf::log() {
    for (const auto& pair : _counters) {
        Counter const& cnt = pair.second;

        uint64_t const nsPerCall = cnt.counter->call_cnt != 0 ? cnt.counter->total / cnt.counter->call_cnt : 0;
        uint64_t const usPerCall = nsPerCall / 1000;
        unsigned const ms = usPerCall / 1000;
        unsigned const us = usPerCall - ms * 1000;

        _desktop->info(TAG "%6" PRIu64 " %2u.%03u %s", cnt.counter->call_cnt, ms, us, cnt.counter->ident);
    }
}

int hc::Perf::push(lua_State* const L) {
    auto const self = static_cast<Perf**>(lua_newuserdata(L, sizeof(Perf*)));
    *self = this;

    if (luaL_newmetatable(L, "hc::Perf")) {
        static luaL_Reg const methods[] = {
            {"getTimeUsec", l_getTimeUsec},
            {"getCounter", l_getCounter},
            {"register", l_register},
            {"start", l_start},
            {"stop", l_stop},
            {"log", l_log},
            {nullptr, nullptr}
        };

        luaL_newlib(L, methods);
        lua_setfield(L, -2, "__index");
    }

    lua_setmetatable(L, -2);
    return 1;
}

hc::Perf* hc::Perf::check(lua_State* const L, int const index) {
    return *static_cast<Perf**>(luaL_checkudata(L, index, "hc::Perf"));
}

int hc::Perf::l_getTimeUsec(lua_State* const L) {
    auto const self = check(L, 1);
    lua_pushinteger(L, self->getTimeUsec());
    return 1;
}

int hc::Perf::l_getCounter(lua_State* const L) {
    auto const self = check(L, 1);
    lua_pushinteger(L, self->getCounter());
    return 1;
}

int hc::Perf::l_register(lua_State* const L) {
    auto const self = check(L, 1);
    size_t length = 0;
    char const* const ident = luaL_checklstring(L, 2, &length);

    auto const found = self->_counters.find(std::string(ident, length));

    if (found != self->_counters.end()) {
        return luaL_error(L, "counter \"%s\" already exists", ident);
    }

    retro_perf_counter* const counter = static_cast<retro_perf_counter*>(malloc(sizeof(*counter)));
    char const* const identCopy = strdup(ident);

    if (counter == nullptr || identCopy == nullptr) {
        free(counter);
        free((void*)identCopy);
        return luaL_error(L, "out of memory");
    }

    counter->ident = identCopy;
    counter->start = 0;
    counter->total = 0;
    counter->call_cnt = 0;
    counter->registered = true;

    Counter cnt = {counter, true};
    self->_counters.insert(std::make_pair(std::string(ident, length), cnt));
    return 0;
}

int hc::Perf::l_start(lua_State* const L) {
    auto const self = check(L, 1);
    size_t length = 0;
    char const* const ident = luaL_checklstring(L, 2, &length);

    auto const found = self->_counters.find(std::string(ident, length));

    if (found == self->_counters.end()) {
        return luaL_error(L, "counter \"%s\" does not exist", ident);
    }

    self->start(found->second.counter);
    return 0;
}

int hc::Perf::l_stop(lua_State* const L) {
    auto const self = check(L, 1);
    size_t length = 0;
    char const* const ident = luaL_checklstring(L, 2, &length);

    auto const found = self->_counters.find(std::string(ident, length));

    if (found == self->_counters.end()) {
        return luaL_error(L, "counter \"%s\" does not exist", ident);
    }

    self->stop(found->second.counter);
    return 0;
}

int hc::Perf::l_log(lua_State* const L) {
    auto const self = check(L, 1);
    self->log();
    return 0;
}
