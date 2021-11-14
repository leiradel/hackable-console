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
{}

void hc::Audio::init() {}

void hc::Audio::flush() {
    Data data;
    data.samples = std::move(_samples);
    data.rate = _rate;

    _queue.put(&data);
}

char const* hc::Audio::getTitle() {
    return ICON_FA_VOLUME_UP " Audio";
}

void hc::Audio::onGameReset() {
    _samples.clear();
}

void hc::Audio::onGameUnloaded() {
    _samples.clear();
}

bool hc::Audio::setSystemAvInfo(retro_system_av_info const* info) {
    _rate = info->timing.sample_rate;

    _desktop->info(TAG "Setting timing");
    _desktop->info(TAG "    sample_rate = %f", _rate);

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
