#include "Audio.h"

#include <IconsFontAwesome4.h>

#define TAG "[AUD] "

hc::Audio::Audio()
    : _logger(nullptr)
    , _sampleRate(0.0)
    , _fifo(nullptr)
    , _mutex(nullptr)
    , _min(0.0)
    , _max(0.0)
    , _mute(false)
    , _wasMuted(false)
    , _rateControlDelta(0.0)
    , _currentRatio(0.0)
    , _originalRatio(0.0)
    , _resampler(nullptr)
{
    memset(&_timing, 0, sizeof(_timing));
}

void hc::Audio::init(Logger* logger, double sampleRate, Fifo* fifo) {
    _logger = logger;
    _sampleRate = sampleRate;
    _fifo = fifo;
}

void hc::Audio::flush() {
    if (_resampler == nullptr) {
        return;
    }

    std::vector<int16_t> samples;
    SDL_LockMutex(_mutex);
    samples.swap(_samples);
    SDL_UnlockMutex(_mutex);

    int16_t const* const data = samples.data();
    size_t const frames = samples.size() / 2;

    size_t const avail = _fifo->free();

    // Readjust the audio input rate
    int const halfSize = (int)_fifo->size() / 2;
    int const deltaMid = (int)avail - halfSize;
    double const direction = (double)deltaMid / (double)halfSize;
    double const adjust = 1.0 + _rateControlDelta * direction;

    _currentRatio = _originalRatio * adjust;

    spx_uint32_t inLen = frames * 2;
    spx_uint32_t outLen = (spx_uint32_t)(inLen * _currentRatio);
    outLen += outLen & 1; // request an even number of samples (stereo)
    int16_t* const output = (int16_t*)alloca(outLen * 2);

    if (output == NULL) {
        _logger->error(TAG "Error allocating temporary output buffer");
        return;
    }

    if (_mute) {
        memset(output, 0, outLen * 2);
    }
    else {
        int const error = speex_resampler_process_int(_resampler, 0, data, &inLen, output, &outLen);

        if (error != RESAMPLER_ERR_SUCCESS) {
            memset(output, 0, outLen * 2);
            _logger->error(TAG "Error resampling: %s", speex_resampler_strerror(error));
        }
    }

    outLen &= ~1; // don't send incomplete audio frames
    size_t const size = outLen * 2;
    _fifo->write(output, size <= avail ? size : avail);
}

char const* hc::Audio::getName() {
    return "hc::Audio built-in audio plugin";
}

char const* hc::Audio::getVersion() {
    return "0.0.0";
}

char const* hc::Audio::getLicense() {
    return "MIT";
}

char const* hc::Audio::getCopyright() {
    return "Copyright (c) Andre Leiradella";
}

char const* hc::Audio::getUrl() {
    return "https://github.com/leiradel/hackable-console";
}

void hc::Audio::onStarted() {
    _mutex = SDL_CreateMutex();

    if (_mutex == nullptr) {
        _logger->error(TAG "SDL_CreateMutex: %s", SDL_GetError());
    }
}

void hc::Audio::onConsoleLoaded() {}

void hc::Audio::onGameLoaded() {
    // setSystemAvInfo has been called by now
    _currentRatio = _originalRatio = _sampleRate / _timing.sample_rate;
    _rateControlDelta = 0.005;

    int error;
    _resampler = speex_resampler_init(2, _timing.sample_rate, _sampleRate, SPEEX_RESAMPLER_QUALITY_DEFAULT, &error);

    if (_resampler == nullptr) {
        _logger->error(TAG "speex_resampler_init: %s", speex_resampler_strerror(error));
        return;
    }

    _logger->info(TAG "Resampler initialized to convert from %f to %f", _timing.sample_rate, _sampleRate);
}

void hc::Audio::onGamePaused() {
    _wasMuted = _mute;
    _mute = true;
}

void hc::Audio::onGameResumed() {
    _mute = _wasMuted;
}

void hc::Audio::onGameReset() {
    SDL_LockMutex(_mutex);
    _samples.clear();
    SDL_UnlockMutex(_mutex);
}

void hc::Audio::onFrame() {}

void hc::Audio::onDraw() {
    if (!ImGui::Begin(ICON_FA_VOLUME_UP " Audio")) {
        return;
    }

    ImGui::Checkbox("Mute", &_mute);
    ImGui::SameLine();

    static auto const left = [](void* const data, int const idx) -> float {
        auto const self = static_cast<Audio*>(data);

        float sample = self->_samples[idx * 2];
        self->_min = std::min(self->_min, sample);
        self->_max = std::max(self->_max, sample);

        return sample;
    };

    static auto const right = [](void* data, int idx) -> float {
        auto const self = static_cast<Audio*>(data);

        float sample = self->_samples[idx * 2 + 1];
        self->_min = std::min(self->_min, sample);
        self->_max = std::max(self->_max, sample);

        return sample;
    };

    ImVec2 max = ImGui::GetContentRegionAvail();

    if (max.y > 0.0f) {
        max.x /= 2;

        size_t const size = _samples.size() / 2;

        ImGui::PlotLines("", left, this, size, 0, nullptr, _min, _max, max);
        ImGui::SameLine(0.0f, 0.0f);
        ImGui::PlotLines("", right, this, size, 0, nullptr, _min, _max, max);
    }

    ImGui::End();
}

void hc::Audio::onGameUnloaded() {
    speex_resampler_destroy(_resampler);
    _resampler = nullptr;

    SDL_LockMutex(_mutex);
    _samples.clear();
    SDL_UnlockMutex(_mutex);
}

void hc::Audio::onConsoleUnloaded() {}

void hc::Audio::onQuit() {
    SDL_DestroyMutex(_mutex);
    _mutex = nullptr;
}

bool hc::Audio::setSystemAvInfo(retro_system_av_info const* info) {
    _timing = info->timing;

    _logger->info(TAG "Setting timing");

    _logger->info(TAG "    fps         = %f", _timing.fps);
    _logger->info(TAG "    sample_rate = %f", _timing.sample_rate);

    return true;
}

bool hc::Audio::setAudioCallback(retro_audio_callback const* callback) {
    _logger->warn(TAG "TODO: RETRO_ENVIRONMENT_SET_AUDIO_CALLBACK");
    return false;
}

size_t hc::Audio::sampleBatch(int16_t const* data, size_t frames) {
    SDL_LockMutex(_mutex);

    size_t const size = _samples.size();
    _samples.resize(size + frames * 2);
    memcpy(_samples.data() + size, data, frames * 4);

    SDL_UnlockMutex(_mutex);
    return frames;
}

void hc::Audio::sample(int16_t left, int16_t right) {
    int16_t frame[2] = {left, right};
    sampleBatch(frame, 1);
}
