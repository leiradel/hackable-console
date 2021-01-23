#include "Audio.h"

#include <IconsFontAwesome4.h>

#include <float.h>

extern "C" {
    #include "lauxlib.h"
}

#define TAG "[AUD] "

hc::Audio::Audio(Desktop* desktop)
    : View(desktop)
    , _logger(nullptr)
    , _sampleRate(0.0)
    , _fifo(nullptr)
    , _min(FLT_MAX)
    , _max(-FLT_MAX)
    , _mute(false)
    , _wasMuted(false)
    , _rateControlDelta(0.0)
    , _currentRatio(0.0)
    , _originalRatio(0.0)
    , _resampler(nullptr)
{
    memset(&_timing, 0, sizeof(_timing));
}

void hc::Audio::init(Logger* const logger, double const sampleRate, Fifo* const fifo) {
    _logger = logger;
    _sampleRate = sampleRate;
    _fifo = fifo;
}

void hc::Audio::flush() {
    if (_resampler == nullptr) {
        return;
    }

    _mutex.lock();
    _previousSamples.swap(_samples);
    _mutex.unlock();
    _samples.clear();

    int16_t const* const data = _previousSamples.data();
    size_t const frames = _previousSamples.size() / 2;

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

char const* hc::Audio::getTitle() {
    return ICON_FA_VOLUME_UP " Audio";
}

void hc::Audio::onGameLoaded() {
    _min = FLT_MAX;
    _max = -FLT_MAX;

    // setSystemAvInfo has been called by now
    _currentRatio = _originalRatio = _sampleRate / _timing.sample_rate;
    _rateControlDelta = 0.005;

    int error;
    _resampler = speex_resampler_init(2, _timing.sample_rate, _sampleRate, SPEEX_RESAMPLER_QUALITY_DEFAULT, &error);

    if (_resampler == nullptr) {
        _logger->error(TAG "speex_resampler_init: %s", speex_resampler_strerror(error));
    }
    else {
        _logger->info(TAG "Resampler initialized to convert from %f to %f", _timing.sample_rate, _sampleRate);
    }
}

void hc::Audio::onGamePaused() {
    _wasMuted = _mute;
    _mute = true;
}

void hc::Audio::onGameResumed() {
    _mute = _wasMuted;
}

void hc::Audio::onGameReset() {
    _mutex.lock();
    _samples.clear();
    _previousSamples.clear();
    _drawSamples.clear();
    _mutex.unlock();
}

void hc::Audio::onDraw() {
    ImGui::Checkbox("Mute", &_mute);
    ImGui::SameLine();

    static auto const left = [](void* const data, int const idx) -> float {
        auto const self = static_cast<Audio*>(data);

        float sample = self->_drawSamples[idx * 2];
        self->_min = std::min(self->_min, sample);
        self->_max = std::max(self->_max, sample);

        return sample;
    };

    static auto const right = [](void* data, int idx) -> float {
        auto const self = static_cast<Audio*>(data);

        float sample = self->_drawSamples[idx * 2 + 1];
        self->_min = std::min(self->_min, sample);
        self->_max = std::max(self->_max, sample);

        return sample;
    };

    ImVec2 max = ImGui::GetContentRegionAvail();

    if (max.y > 0.0f) {
        max.x /= 2;

        _mutex.lock();
        _drawSamples = _previousSamples;
        _mutex.unlock();

        size_t const size = _drawSamples.size() / 2;

        ImGui::PlotLines("", left, this, size, 0, nullptr, _min, _max, max);
        ImGui::SameLine(0.0f, 0.0f);
        ImGui::PlotLines("", right, this, size, 0, nullptr, _min, _max, max);

        _drawSamples.clear();
    }
}

void hc::Audio::onGameUnloaded() {
    if (_resampler != nullptr) {
        speex_resampler_destroy(_resampler);
        _resampler = nullptr;
    }

    _mutex.lock();
    _samples.clear();
    _previousSamples.clear();
    _drawSamples.clear();
    _mutex.unlock();
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
    std::lock_guard<std::mutex> lock(_mutex);

    size_t const size = _samples.size();
    _samples.resize(size + frames * 2);
    memcpy(_samples.data() + size, data, frames * 4);

    return frames;
}

void hc::Audio::sample(int16_t left, int16_t right) {
    int16_t frame[2] = {left, right};
    sampleBatch(frame, 1);
}

int hc::Audio::push(lua_State* const L) {
    auto const self = static_cast<Audio**>(lua_newuserdata(L, sizeof(Audio*)));
    *self = this;

    if (luaL_newmetatable(L, "hc::Audio")) {
        static luaL_Reg const methods[] = {
            {nullptr, nullptr}
        };

        luaL_newlib(L, methods);
        lua_setfield(L, -2, "__index");
    }

    lua_setmetatable(L, -2);
    return 1;
}

hc::Audio* hc::Audio::check(lua_State* const L, int const index) {
    return *static_cast<Audio**>(luaL_checkudata(L, index, "hc::Audio"));
}
