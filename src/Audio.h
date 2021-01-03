#pragma once

#include "Logger.h"

#include <Components.h>
#include <Fifo.h>

#include <speex_resampler.h>

#include <vector>

namespace hc {
    class Audio: public lrcpp::Audio {
    public:
        Audio() : _logger(nullptr), _fifo(nullptr) {}
        virtual ~Audio() {}

        bool init(Logger* logger, double sampleRate, Fifo* fifo);
        void destroy();
        void reset();
        void draw();

        virtual bool setSystemAvInfo(retro_system_av_info const* info) override;
        virtual bool setAudioCallback(retro_audio_callback const* callback) override;

        virtual size_t sampleBatch(int16_t const* data, size_t frames) override;
        virtual void sample(int16_t left, int16_t right) override;

        void flush();

    protected:
        void setupResampler(double const rate);

        Logger* _logger;

        std::vector<int16_t> _samples;
        SDL_mutex* _mutex;

        float _min;
        float _max;

        retro_system_timing _timing;
        bool _mute;

        Fifo* _fifo;
        double _sampleRate;
        double _coreRate;

        double _rateControlDelta;
        double _currentRatio;
        double _originalRatio;
        SpeexResamplerState* _resampler;
    };
}
