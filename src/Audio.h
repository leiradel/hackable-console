#pragma once

#include "Desktop.h"
#include "Logger.h"

#include <lrcpp/Components.h>
#include <Fifo.h>

#include <speex_resampler.h>

#include <vector>

namespace hc {
    class Audio: public View, public lrcpp::Audio {
    public:
        Audio(Desktop* desktop);
        virtual ~Audio() {}

        void init(Logger* const logger, double const sampleRate, Fifo* const fifo);
        void flush();

        static Audio* check(lua_State* const L, int const index);

        // hc::View
        virtual char const* getTitle() override;
        virtual void onStarted() override;
        virtual void onCoreLoaded() override;
        virtual void onGameLoaded() override;
        virtual void onGamePaused() override;
        virtual void onGameResumed() override;
        virtual void onGameReset() override;
        virtual void onFrame() override;
        virtual void onDraw() override;
        virtual void onGameUnloaded() override;
        virtual void onConsoleUnloaded() override;
        virtual void onQuit() override;

        virtual int push(lua_State* const L) override;

        // lrcpp::Audio
        virtual bool setSystemAvInfo(retro_system_av_info const* info) override;
        virtual bool setAudioCallback(retro_audio_callback const* callback) override;

        virtual size_t sampleBatch(int16_t const* data, size_t frames) override;
        virtual void sample(int16_t left, int16_t right) override;

    protected:
        Logger* _logger;
        double _sampleRate;
        Fifo* _fifo;

        std::vector<int16_t> _samples;
        SDL_mutex* _mutex;

        float _min;
        float _max;

        retro_system_timing _timing;
        bool _mute;
        bool _wasMuted;

        double _rateControlDelta;
        double _currentRatio;
        double _originalRatio;
        SpeexResamplerState* _resampler;
    };
}
