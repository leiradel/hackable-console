#pragma once

#include "Plugin.h"
#include "Logger.h"

#include <Components.h>

#include <string>
#include <unordered_map>

namespace hc
{
    class Perf : public Plugin, public lrcpp::Perf {
    public:
        Perf() : _logger(nullptr) {}
        virtual ~Perf() {}

        void init(Logger* logger);

        // hc::Plugin
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
        virtual void onDraw() override;
        virtual void onGameUnloaded() override;
        virtual void onConsoleUnloaded() override;
        virtual void onQuit() override;

        // lrcpp::Perf
        virtual retro_time_t getTimeUsec() override;
        virtual uint64_t getCpuFeatures() override;
        virtual retro_perf_tick_t getCounter() override;
        virtual void register_(retro_perf_counter* counter) override;
        virtual void start(retro_perf_counter* counter) override;
        virtual void stop(retro_perf_counter* counter) override;
        virtual void log() override;

    protected:
        lrcpp::Logger* _logger;

        std::unordered_map<std::string, retro_perf_counter*> _counters;
    };
}
