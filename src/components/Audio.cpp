#include "Audio.h"
#include "Logger.h"

#include <IconsFontAwesome4.h>

#include <float.h>

extern "C" {
    #include "lauxlib.h"
}

#define TAG "[AUD] "

hc::Audio::Audio(Desktop* desktop)
    : View(desktop)
    , _device(0)
    , _rate(0.0)
    , _mute(false)
    , _wasMuted(false)
{}

hc::Audio::~Audio() {
    if (_device != 0) {
        SDL_CloseAudioDevice(_device);
        _device = 0;
    }
}

void hc::Audio::init() {
    _samples.clear();
    _draw.clear();
}

void hc::Audio::flush() {
    if (_device != 0 && !_mute && SDL_QueueAudio(_device, _samples.data(), _samples.size() * 2) != 0) {
        _desktop->error("SDL_QueueAudio() failed: %s", SDL_GetError());
    }

    {
        std::lock_guard<std::mutex> lock(_mutex);
        _draw = std::move(_samples);
    }
}

char const* hc::Audio::getTitle() {
    return ICON_FA_VOLUME_UP " Audio";
}

void hc::Audio::onGamePaused() {
    _wasMuted = _mute;
    _mute = true;
}

void hc::Audio::onGameResumed() {
    _mute = _wasMuted;
}

void hc::Audio::onGameReset() {
    _samples.clear();

    {
        std::lock_guard<std::mutex> lock(_mutex);
        _draw.clear();
    }
}

void hc::Audio::onDraw() {
    std::lock_guard<std::mutex> lock(_mutex);

    ImGui::Checkbox("Mute", &_mute);
    ImGui::SameLine();

    static auto const left = [](void* const data, int const idx) -> float {
        auto const self = static_cast<Audio*>(data);
        return self->_draw[idx * 2];
    };

    static auto const right = [](void* data, int idx) -> float {
        auto const self = static_cast<Audio*>(data);
        return self->_draw[idx * 2 + 1];
    };

    ImVec2 avail = ImGui::GetContentRegionAvail();

    if (avail.y > 0.0f) {
        avail.x /= 2;

        int16_t min = INT16_MAX;
        int16_t max = INT16_MIN;

        size_t const count = _draw.size();

        for (size_t i = 0; i < count; i++) {
            int16_t const sample = _draw[i];
            min = std::min(min, sample);
            max = std::max(max, sample);
        }

        size_t const size = count / 2;

        ImGui::PlotLines("", left, this, size, 0, nullptr, min, max, avail);
        ImGui::SameLine(0.0f, 0.0f);
        ImGui::PlotLines("", right, this, size, 0, nullptr, min, max, avail);
    }
}

void hc::Audio::onGameUnloaded() {
    _samples.clear();

    {
        std::lock_guard<std::mutex> lock(_mutex);
        _draw.clear();
    }
}

bool hc::Audio::setSystemAvInfo(retro_system_av_info const* info) {
    _desktop->info(TAG "Setting timing");
    _desktop->info(TAG "    fps         = %f", info->timing.fps);
    _desktop->info(TAG "    sample_rate = %f", info->timing.sample_rate);

    if (info->timing.sample_rate != _rate) {
        _rate = info->timing.sample_rate;

        if (_device != 0) {
            SDL_CloseAudioDevice(_device);
        }

        SDL_AudioSpec desired, obtained;
        memset(&desired, 0, sizeof(desired));
        memset(&obtained, 0, sizeof(obtained));

        desired.freq = _rate;
        desired.format = AUDIO_S16SYS;
        desired.channels = 2;
        desired.samples = 1024;
        desired.callback = nullptr;
        desired.userdata = nullptr;

        _device = SDL_OpenAudioDevice(
            nullptr, 0,
            &desired, &obtained,
            SDL_AUDIO_ALLOW_FREQUENCY_CHANGE | SDL_AUDIO_ALLOW_CHANNELS_CHANGE
        );

        if (_device != 0) {
            SDL_PauseAudioDevice(_device, 0);

            _desktop->info(TAG "Opened audio driver %s", SDL_GetCurrentAudioDriver());
            _desktop->info(TAG "    %d Hz", obtained.freq);
            _desktop->info(TAG "    %u channels", obtained.channels);
            _desktop->info(TAG "    %u bits per sample", SDL_AUDIO_BITSIZE(obtained.format));

            _desktop->info(
                TAG "    %s %s",
                SDL_AUDIO_ISSIGNED(obtained.format) ? "signed" : "unsigned",
                SDL_AUDIO_ISFLOAT(obtained.format) ? "float" : "integer"
            );

            _desktop->info(TAG "    %s endian", SDL_AUDIO_ISBIGENDIAN(obtained.format) ? "big" : "little");
        }
        else {
            _desktop->error(TAG "Error in SDL_OpenAudioDevice: %s", SDL_GetError());
        }
    }

    return true;
}

bool hc::Audio::setAudioCallback(retro_audio_callback const* callback) {
    _desktop->warn(TAG "TODO: RETRO_ENVIRONMENT_SET_AUDIO_CALLBACK");
    return false;
}

size_t hc::Audio::sampleBatch(int16_t const* data, size_t frames) {
    size_t const size = _samples.size();
    _samples.resize(size + frames * 2);
    memcpy(_samples.data() + size, data, frames * 4);

    return frames;
}

void hc::Audio::sample(int16_t left, int16_t right) {
    int16_t frame[2] = {left, right};
    sampleBatch(frame, 1);
}
