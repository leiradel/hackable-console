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
    , _rate(0.0)
    , _mute(false)
    , _wasMuted(false)
{}

void hc::Audio::init() {
    _samples.clear();
    _draw.clear();
}

void hc::Audio::flush() {
    Data data;
    data.samples = _samples;
    data.rate = _rate;

    _queue.put(&data);

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

    _rate = info->timing.sample_rate;
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
