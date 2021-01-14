#include "Video.h"

#include <IconsFontAwesome4.h>

#include <chrono>

#define TAG "[VID] "

static int64_t getTimeUs() {
    auto const now_us = std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::system_clock::now());
    return static_cast<int64_t>(now_us.time_since_epoch().count());
}

hc::Video::Video() : _logger(nullptr) {}

void hc::Video::init(Logger* logger) {
    _logger = logger;

    _rotation = 0;
    _pixelFormat = RETRO_PIXEL_FORMAT_UNKNOWN;

    _texture = 0;
    _textureWidth = _textureHeight = 0;
    _width = _height = 0;

    _frameCount = 0;
    _timeStarted = 0;
    _fps = -1.0f;
}

char const* hc::Video::getName() {
    return "hc::Video built-in video plugin";
}

char const* hc::Video::getVersion() {
    return "0.0.0";
}

char const* hc::Video::getLicense() {
    return "MIT";
}

char const* hc::Video::getCopyright() {
    return "Copyright (c) Andre Leiradella";
}

char const* hc::Video::getUrl() {
    return "https://github.com/leiradel/hackable-console";
}

void hc::Video::onStarted() {
    _timeStarted = getTimeUs();
}

void hc::Video::onConsoleLoaded() {}

void hc::Video::onGameLoaded() {}

void hc::Video::onGamePaused() {}

void hc::Video::onGameResumed() {}

void hc::Video::onGameReset() {}

void hc::Video::onFrame() {}

void hc::Video::onDraw() {
    _frameCount++;

    int64_t const t1 = getTimeUs();
    int64_t const delta = t1 - _timeStarted;
    _fps = static_cast<double>(_frameCount) * 1000000.0f / static_cast<double>(delta);

    if (!ImGui::Begin(ICON_FA_DESKTOP " Video")) {
        return;
    }

    if (_texture != 0) {
        ImVec2 const min = ImGui::GetWindowContentRegionMin();
        ImVec2 const max = ImGui::GetWindowContentRegionMax();

        float height = max.y - min.y;
        float width = height * _aspectRatio;

        if (width > max.x - min.x) {
            width = max.x - min.x;
            height = width / _aspectRatio;
        }

        ImVec2 const size = ImVec2(width, height);
        ImVec2 const uv0 = ImVec2(0.0f, 0.0f);
        ImVec2 const uv1 = ImVec2((float)_width / _textureWidth, (float)_height / _textureHeight);

        ImGui::Image((ImTextureID)(uintptr_t)_texture, size, uv0, uv1);
    }

    ImGui::End();
}

void hc::Video::onGameUnloaded() {
    glDeleteTextures(1, &_texture);
    _texture = 0;
    _textureWidth = _textureHeight = 0;
    _width = _height = 0;
}

void hc::Video::onConsoleUnloaded() {
    _pixelFormat = RETRO_PIXEL_FORMAT_UNKNOWN;
}

void hc::Video::onQuit() {}

bool hc::Video::setRotation(unsigned rotation) {
    _rotation = rotation;
    _logger->info(TAG "Set rotation to %u", rotation);
    return true;
}

bool hc::Video::getOverscan(bool* overscan) {
    *overscan = true;
    _logger->warn(TAG "Returning fixed true for overscan");
    return true;
}

bool hc::Video::getCanDupe(bool* canDupe) {
    *canDupe = true;
    _logger->warn(TAG "Returning fixed true for can dupe");
    return true;
}

bool hc::Video::showMessage(retro_message const* message) {
    _logger->warn(TAG "showMessage not implemented: %s", message->msg);
    return true;
}

bool hc::Video::setPixelFormat(retro_pixel_format format) {
    _pixelFormat = format;

    switch (format) {
        case RETRO_PIXEL_FORMAT_0RGB1555: _logger->info(TAG "Set pixel format to RETRO_PIXEL_FORMAT_0RGB1555"); break;
        case RETRO_PIXEL_FORMAT_XRGB8888: _logger->info(TAG "Set pixel format to RETRO_PIXEL_FORMAT_XRGB8888"); break;
        case RETRO_PIXEL_FORMAT_RGB565:   _logger->info(TAG "Set pixel format to RETRO_PIXEL_FORMAT_RGB565"); break;

        default:
            _logger->info(TAG "Invalid pixel format %d", format);
            return false;
    }

    return true;
}

bool hc::Video::setHwRender(retro_hw_render_callback* callback) {
    _logger->warn(TAG "TODO: RETRO_ENVIRONMENT_SET_HW_RENDER");
    return false;
}

bool hc::Video::setFrameTimeCallback(retro_frame_time_callback const* callback) {
    _logger->warn(TAG "TODO: RETRO_ENVIRONMENT_SET_FRAME_TIME_CALLBACK");
    return false;
}

bool hc::Video::setSystemAvInfo(retro_system_av_info const* info) {
    _logger->info(TAG "Setting timing");

    _logger->info(TAG "    fps         = %f", info->timing.fps);
    _logger->info(TAG "    sample_rate = %f", info->timing.sample_rate);

    return setGeometry(&info->geometry);
}

bool hc::Video::setGeometry(retro_game_geometry const* geometry) {
    _aspectRatio = geometry->aspect_ratio;

    if (_aspectRatio <= 0) {
        _aspectRatio = (float)geometry->base_width / (float)geometry->base_height;
    }

    _logger->info(TAG "Setting geometry");

    _logger->info(TAG "    base_width   = %u", geometry->base_width);
    _logger->info(TAG "    base_height  = %u", geometry->base_height);
    _logger->info(TAG "    max_width    = %u", geometry->max_width);
    _logger->info(TAG "    max_height   = %u", geometry->max_height);
    _logger->info(TAG "    aspect_ratio = %f", geometry->aspect_ratio);

    setupTexture(geometry->max_width, geometry->max_height);
    return true;
}

bool hc::Video::getCurrentSoftwareFramebuffer(retro_framebuffer* framebuffer) {
    _logger->warn(TAG "TODO: RETRO_ENVIRONMENT_GET_CURRENT_SOFTWARE_FRAMEBUFFER");
    return false;
}

bool hc::Video::getHwRenderInterface(retro_hw_render_interface const** interface) {
    _logger->warn(TAG "TODO: RETRO_ENVIRONMENT_GET_HW_RENDER_INTERFACE");
    return false;
}

bool hc::Video::setHwRenderContextNegotiationInterface(retro_hw_render_context_negotiation_interface const* interface) {
    _logger->warn(TAG "TODO: RETRO_ENVIRONMENT_SET_HW_RENDER_CONTEXT_NEGOTIATION_INTERFACE");
    return false;
}

bool hc::Video::setHwSharedContext() {
    _logger->warn(TAG "TODO: RETRO_ENVIRONMENT_SET_HW_SHARED_CONTEXT");
    return false;
}

bool hc::Video::getTargetRefreshRate(float* rate) {
    *rate = static_cast<float>(_fps);
    _logger->info(TAG "Returning %f for target refresh rate", _fps);
    return true;
}

bool hc::Video::getPreferredHwRender(unsigned* preferred) {
    *preferred = RETRO_HW_CONTEXT_NONE;
    _logger->warn(TAG "Returning fixed RETRO_HW_CONTEXT_NONE for preferred hardware renderer");
    return true;
}

void hc::Video::refresh(void const* data, unsigned width, unsigned height, size_t pitch) {
    if (data == nullptr || data == RETRO_HW_FRAME_BUFFER_VALID) {
        return;
    }

    GLint previous_texture;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &previous_texture);
    glBindTexture(GL_TEXTURE_2D, _texture);

    switch (_pixelFormat) {
        case RETRO_PIXEL_FORMAT_XRGB8888:
            glPixelStorei(GL_UNPACK_ROW_LENGTH, pitch / 4);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, data);
            break;

        case RETRO_PIXEL_FORMAT_RGB565:
            glPixelStorei(GL_UNPACK_ROW_LENGTH, pitch / 2);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, data);
            break;

        case RETRO_PIXEL_FORMAT_0RGB1555:
            glPixelStorei(GL_UNPACK_ROW_LENGTH, pitch / 2);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_BGRA, GL_UNSIGNED_SHORT_1_5_5_5_REV, data);
            break;

        case RETRO_PIXEL_FORMAT_UNKNOWN:
            break;
    }

    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glBindTexture(GL_TEXTURE_2D, previous_texture);

    _width = width;
    _height = height;
}

uintptr_t hc::Video::getCurrentFramebuffer() {
    return 0;
}

retro_proc_address_t hc::Video::getProcAddress(char const* symbol) {
    return nullptr;
}

void hc::Video::setupTexture(unsigned width, unsigned height) {
    if (width <= _textureWidth && height <= _textureHeight) {
        return;
    }

    _textureWidth = width;
    _textureHeight = height;

    if (_texture != 0) {
        glDeleteTextures(1, &_texture);
    }

    GLint previous_texture;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &previous_texture);

    glGenTextures(1, &_texture);
    glBindTexture(GL_TEXTURE_2D, _texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    switch (_pixelFormat) {
        case RETRO_PIXEL_FORMAT_XRGB8888:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
            break;

        case RETRO_PIXEL_FORMAT_RGB565:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, NULL);
            break;

        case RETRO_PIXEL_FORMAT_0RGB1555:
        default:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_BGRA, GL_UNSIGNED_SHORT_1_5_5_5_REV, NULL);
            break;
    }

    glBindTexture(GL_TEXTURE_2D, previous_texture);
    _logger->info(TAG "Texture set to %u x %u", width, height);
}
