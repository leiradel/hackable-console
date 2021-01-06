#include "Audio.h"

#include <IconsFontAwesome4.h>

bool hc::Audio::init(Logger* logger, double sampleRate, Fifo* fifo) {
    _logger = logger;
    _logger->debug("%s:%u: %s()", __FILE__, __LINE__, __FUNCTION__);

    reset();

    _mutex = SDL_CreateMutex();

    if (_mutex == nullptr) {
        return false;
    }

    _sampleRate = sampleRate;
    _fifo = fifo;
    return true;
}

void hc::Audio::destroy() {
    _logger->debug("%s:%u: %s()", __FILE__, __LINE__, __FUNCTION__);

    if (_resampler != NULL) {
        speex_resampler_destroy(_resampler);
    }

    SDL_DestroyMutex(_mutex);
}

void hc::Audio::reset() {
    _logger->debug("%s:%u: %s()", __FILE__, __LINE__, __FUNCTION__);

    memset(&_timing, 0, sizeof(_timing));

    _min = 0.0f;
    _max = 0.0f;

    _mute = false;
    _sampleRate = 0.0;
    _coreRate = 0.0;

    _rateControlDelta = 0.005;
    _currentRatio = 0.0;
    _originalRatio = 0.0;
    _resampler = nullptr;
}

void hc::Audio::draw() {
    if (!ImGui::Begin(ICON_FA_VOLUME_UP " Audio")) {
        return;
    }

    ImGui::Checkbox("Mute", &_mute);

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

bool hc::Audio::setSystemAvInfo(retro_system_av_info const* info) {
    _logger->debug("%s:%u: %s()", __FILE__, __LINE__, __FUNCTION__);
    _timing = info->timing;

    _logger->info("Setting timing");

    _logger->info("    fps         = %f", _timing.fps);
    _logger->info("    sample_rate = %f", _timing.sample_rate);

    setupResampler(_timing.sample_rate);
    return true;
}

bool hc::Audio::setAudioCallback(retro_audio_callback const* callback) {
    _logger->debug("%s:%u: %s(%p)", __FILE__, __LINE__, __FUNCTION__, callback);
    _logger->warn("TODO: RETRO_ENVIRONMENT_SET_AUDIO_CALLBACK");
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
        _logger->error("Error allocating temporary output buffer");
        return;
    }

    if (_mute) {
        memset(output, 0, outLen * 2);
    }
    else {
        int const error = speex_resampler_process_int(_resampler, 0, data, &inLen, output, &outLen);

        if (error != RESAMPLER_ERR_SUCCESS) {
            memset(output, 0, outLen * 2);
            _logger->error("Error resampling: %s", speex_resampler_strerror(error));
        }
    }

    outLen &= ~1; // don't send incomplete audio frames
    size_t const size = outLen * 2;

    if (size > avail) {
        for (;;) {
            SDL_Delay(1);

            if (size <= _fifo->free()) {
                break;
            }
        }
    }

    _fifo->write(output, size);
}

void hc::Audio::setupResampler(double const rate) {
    if (_coreRate == rate) {
        return;
    }

    if (_resampler != nullptr) {
        speex_resampler_destroy(_resampler);
        _resampler = nullptr;
    }

    _coreRate = rate;
    _currentRatio = _originalRatio = _sampleRate / _coreRate;
    _rateControlDelta = 0.005;

    int error;
    _resampler = speex_resampler_init(2, _coreRate, _sampleRate, SPEEX_RESAMPLER_QUALITY_DEFAULT, &error);

    if (_resampler == nullptr) {
        _logger->error("speex_resampler_init: %s", speex_resampler_strerror(error));
        return;
    }

    _logger->info("Resampler initialized to convert from %f to %f", _coreRate, _sampleRate);
}
