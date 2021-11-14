#pragma once

#include "Desktop.h"

#include <lrcpp/Components.h>

#include <SDL.h>

#include <vector>
#include <mutex>

namespace hc {
    class Audio: public View, public lrcpp::Audio {
    public:
        Audio(Desktop* desktop);
        virtual ~Audio();

        void init();
        void flush();

        // hc::View
        virtual char const* getTitle() override;
        virtual void onGamePaused() override;
        virtual void onGameResumed() override;
        virtual void onGameReset() override;
        virtual void onDraw() override;
        virtual void onGameUnloaded() override;

        // lrcpp::Audio
        virtual bool setSystemAvInfo(retro_system_av_info const* info) override;
        virtual bool setAudioCallback(retro_audio_callback const* callback) override;

        virtual size_t sampleBatch(int16_t const* data, size_t frames) override;
        virtual void sample(int16_t left, int16_t right) override;

    protected:
        SDL_AudioDeviceID _device;

        double _rate;
        bool _mute;
        bool _wasMuted;

        std::vector<int16_t> _samples;
        std::vector<int16_t> _draw;
        std::mutex _mutex;
    };
}
