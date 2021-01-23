#pragma once

#include "Desktop.h"
#include "Logger.h"

#include <lrcpp/Components.h>

#include <stdint.h>
#include <string>
#include <unordered_map>

namespace hc
{
    class Perf : public View, public lrcpp::Perf {
    public:
        Perf(Desktop* desktop) : View(desktop), _logger(nullptr) {}
        virtual ~Perf() {}

        void init(Logger* const logger);

        static uint64_t getTimeUs();
        static uint64_t getTimeNs();

        static Perf* check(lua_State* const L, int const index);

        // hc::View
        virtual char const* getTitle() override;
        virtual void onDraw() override;
        virtual void onConsoleUnloaded() override;

        // hc::Scriptable
        virtual int push(lua_State* const L) override;

        // lrcpp::Perf
        virtual retro_time_t getTimeUsec() override;
        virtual uint64_t getCpuFeatures() override;
        virtual retro_perf_tick_t getCounter() override;
        virtual void register_(retro_perf_counter* counter) override;
        virtual void start(retro_perf_counter* counter) override;
        virtual void stop(retro_perf_counter* counter) override;
        virtual void log() override;

    protected:
        static int l_getTimeUsec(lua_State* const L);
        static int l_getCounter(lua_State* const L);
        static int l_register(lua_State* const L);
        static int l_start(lua_State* const L);
        static int l_stop(lua_State* const L);
        static int l_log(lua_State* const L);

        struct Counter {
            retro_perf_counter* const counter;
            bool const mustDelete;
        };

        lrcpp::Logger* _logger;

        std::unordered_map<std::string, Counter> _counters;
    };
}
