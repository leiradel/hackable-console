#include "Video.h"

bool hc::Video::init(Logger* logger) {
    _logger = logger;
    _logger->debug("%s:%u: %s()", __FILE__, __LINE__, __FUNCTION__);

    reset();
    return true;
}

void hc::Video::destroy() {
    _logger->debug("%s:%u: %s()", __FILE__, __LINE__, __FUNCTION__);

    if (_texture != 0) {
        glDeleteTextures(1, &_texture);
    }
}

void hc::Video::reset() {
    _logger->debug("%s:%u: %s()", __FILE__, __LINE__, __FUNCTION__);

    _rotation = 0;
    _pixelFormat = RETRO_PIXEL_FORMAT_UNKNOWN;
    memset(&_systemAvInfo, 0, sizeof(_systemAvInfo));

    if (_texture != 0) {
        glDeleteTextures(1, &_texture);
    }

    _maxWidth = 0;
    _maxHeight = 0;
    _width = 0;
    _height = 0;
}

void hc::Video::draw() {
    if (_texture != 0) {
        ImVec2 const min = ImGui::GetWindowContentRegionMin();
        ImVec2 const max = ImGui::GetWindowContentRegionMax();

        float height = max.y - min.y;
        float width = height * _systemAvInfo.geometry.aspect_ratio;

        if (width > max.x - min.x) {
            width = max.x - min.x;
            height = width / _systemAvInfo.geometry.aspect_ratio;
        }

        ImVec2 const size = ImVec2(width, height);
        ImVec2 const uv0 = ImVec2(0.0f, 0.0f);
        ImVec2 const uv1 = ImVec2((float)_width / _maxWidth, (float)_height / _maxHeight);

        ImGui::Image((ImTextureID)(uintptr_t)_texture, size, uv0, uv1);
    }
}

bool hc::Video::setRotation(unsigned rotation) {
    _logger->debug("%s:%u: %s(%u)", __FILE__, __LINE__, __FUNCTION__, rotation);
    _rotation = rotation;
    _logger->info("Set rotation to %u", rotation);
    return true;
}

bool hc::Video::getOverscan(bool* overscan) {
    _logger->debug("%s:%u: %s(%p)", __FILE__, __LINE__, __FUNCTION__, overscan);
    *overscan = true;
    _logger->warn("Returning fixed true for overscan");
    return true;
}

bool hc::Video::getCanDupe(bool* canDupe) {
    _logger->debug("%s:%u: %s(%p)", __FILE__, __LINE__, __FUNCTION__, canDupe);
    *canDupe = true;
    _logger->warn("Returning fixed true for can dupe");
    return true;
}

bool hc::Video::showMessage(retro_message const* message) {
    _logger->debug("%s:%u: %s(%p)", __FILE__, __LINE__, __FUNCTION__, message);
    _logger->warn("TODO: RETRO_ENVIRONMENT_SET_MESSAGE");
    _logger->info("Show message: %s", message->msg);
    return true;
}

bool hc::Video::setPixelFormat(retro_pixel_format format) {
    _logger->debug("%s:%u: %s(%d)", __FILE__, __LINE__, __FUNCTION__, format);
    _pixelFormat = format;

    switch (format) {
        case RETRO_PIXEL_FORMAT_0RGB1555: _logger->info("Set pixel format to RETRO_PIXEL_FORMAT_0RGB1555"); break;
        case RETRO_PIXEL_FORMAT_XRGB8888: _logger->info("Set pixel format to RETRO_PIXEL_FORMAT_XRGB8888"); break;
        case RETRO_PIXEL_FORMAT_RGB565:   _logger->info("Set pixel format to RETRO_PIXEL_FORMAT_RGB565"); break;

        default:
            _logger->info("Invalid pixel format %d", format);
            return false;
    }

    return true;
}

bool hc::Video::setHwRender(retro_hw_render_callback* callback) {
    _logger->debug("%s:%u: %s(%p)", __FILE__, __LINE__, __FUNCTION__, callback);
    _logger->warn("TODO: RETRO_ENVIRONMENT_SET_HW_RENDER");
    return false;
}

bool hc::Video::setFrameTimeCallback(retro_frame_time_callback const* callback) {
    _logger->debug("%s:%u: %s(%p)", __FILE__, __LINE__, __FUNCTION__, callback);
    _logger->warn("TODO: RETRO_ENVIRONMENT_SET_FRAME_TIME_CALLBACK");
    return false;
}

bool hc::Video::setSystemAvInfo(retro_system_av_info const* info) {
    _logger->debug("%s:%u: %s(%p)", __FILE__, __LINE__, __FUNCTION__, info);
    _systemAvInfo = *info;

    _logger->info("Setting system av info");

    _logger->info("    geometry.base_width   = %u", _systemAvInfo.geometry.base_width);
    _logger->info("    geometry.base_height  = %u", _systemAvInfo.geometry.base_height);
    _logger->info("    geometry.max_width    = %u", _systemAvInfo.geometry.max_width);
    _logger->info("    geometry.max_height   = %u", _systemAvInfo.geometry.max_height);
    _logger->info("    geometry.aspect_ratio = %f", _systemAvInfo.geometry.aspect_ratio);
    _logger->info("    timing.fps            = %f", _systemAvInfo.timing.fps);
    _logger->info("    timing.sample_rate    = %f", _systemAvInfo.timing.sample_rate);

    setupTexture(info->geometry.max_width, info->geometry.max_height);
    return true;
}

bool hc::Video::setGeometry(retro_game_geometry const* geometry) {
    _logger->debug("%s:%u: %s(%p)", __FILE__, __LINE__, __FUNCTION__, geometry);
    _systemAvInfo.geometry = *geometry;

    _logger->info("Setting geometry");

    _logger->info("    base_width   = %u", _systemAvInfo.geometry.base_width);
    _logger->info("    base_height  = %u", _systemAvInfo.geometry.base_height);
    _logger->info("    max_width    = %u", _systemAvInfo.geometry.max_width);
    _logger->info("    max_height   = %u", _systemAvInfo.geometry.max_height);
    _logger->info("    aspect_ratio = %f", _systemAvInfo.geometry.aspect_ratio);

    setupTexture(geometry->max_width, geometry->max_height);
    return true;
}

bool hc::Video::getCurrentSoftwareFramebuffer(retro_framebuffer* framebuffer) {
    _logger->debug("%s:%u: %s(%p)", __FILE__, __LINE__, __FUNCTION__, framebuffer);
    _logger->warn("TODO: RETRO_ENVIRONMENT_GET_CURRENT_SOFTWARE_FRAMEBUFFER");
    return false;
}

bool hc::Video::getHwRenderInterface(retro_hw_render_interface const** interface) {
    _logger->debug("%s:%u: %s(%p)", __FILE__, __LINE__, __FUNCTION__, interface);
    _logger->warn("TODO: RETRO_ENVIRONMENT_GET_HW_RENDER_INTERFACE");
    return false;
}

bool hc::Video::setHwRenderContextNegotiationInterface(retro_hw_render_context_negotiation_interface const* interface) {
    _logger->debug("%s:%u: %s(%p)", __FILE__, __LINE__, __FUNCTION__, interface);
    _logger->warn("TODO: RETRO_ENVIRONMENT_SET_HW_RENDER_CONTEXT_NEGOTIATION_INTERFACE");
    return false;
}

bool hc::Video::setHwSharedContext() {
    _logger->debug("%s:%u: %s()", __FILE__, __LINE__, __FUNCTION__);
    _logger->warn("TODO: RETRO_ENVIRONMENT_SET_HW_SHARED_CONTEXT");
    return false;
}

bool hc::Video::getTargetRefreshRate(float* rate) {
    _logger->debug("%s:%u: %s(%p)", __FILE__, __LINE__, __FUNCTION__, rate);
    *rate = 60;
    _logger->warn("Returning fixed 60 for target refresh rate");
    return true;
}

bool hc::Video::getPreferredHwRender(unsigned* preferred) {
    _logger->debug("%s:%u: %s(%p)", __FILE__, __LINE__, __FUNCTION__, preferred);
    *preferred = RETRO_HW_CONTEXT_NONE;
    _logger->warn("Returning fixed RETRO_HW_CONTEXT_NONE for preferred hardware renderer");
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
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_SHORT_1_5_5_5_REV, data);
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
    if (width <= _maxWidth && height <= _maxHeight) {
        return;
    }

    _maxWidth = width;
    _maxHeight = height;

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
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
            break;

        case RETRO_PIXEL_FORMAT_RGB565:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, NULL);
            break;

        case RETRO_PIXEL_FORMAT_0RGB1555:
        default:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_SHORT_1_5_5_5_REV, NULL);
            break;
    }

    glBindTexture(GL_TEXTURE_2D, previous_texture);

    _logger->info("Texture set to %u x %u", width, height);
}
