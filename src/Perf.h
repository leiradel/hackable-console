#pragma once

#include "Desktop.h"
#include "Logger.h"

#include <Components.h>

#include <string>
#include <unordered_map>

namespace hc
{
    class Perf : public View, public lrcpp::Perf {
    public:
        Perf() : _logger(nullptr) {}
        virtual ~Perf() {}

        void init(Logger* const logger);

        static Perf* check(lua_State* const L, int const index);

        // hc::View
        virtual Type getType() override { return Type::Perf; }
        virtual char const* getName() override;
        virtual char const* getVersion() override;
        virtual char const* getLicense() override;
        virtual char const* getCopyright() override;
        virtual char const* getUrl() override;

        virtual void onStarted() override;
        virtual void onConsoleLoaded() override;
        virtual void onGameLoaded() override;
        virtual void onGamePaused() override;
        virtual void onGameResumed() override;
        virtual void onGameReset() override;
        virtual void onFrame() override;
        virtual void onDraw(bool* opened) override;
        virtual void onGameUnloaded() override;
        virtual void onConsoleUnloaded() override;
        virtual void onQuit() override;

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
