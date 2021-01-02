#include "Video.h"

bool hc::Video::init(Logger* logger) {
    _logger = logger;
    _logger->debug("%s:%u: %s()", __FILE__, __LINE__, __FUNCTION__);
    return true;
}

void hc::Video::destroy() {
    _logger->debug("%s:%u: %s()", __FILE__, __LINE__, __FUNCTION__);
}

void hc::Video::reset() {
    _logger->debug("%s:%u: %s()", __FILE__, __LINE__, __FUNCTION__);
}

void hc::Video::draw() {
    _logger->debug("%s:%u: %s()", __FILE__, __LINE__, __FUNCTION__);
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
    // TODO
}

uintptr_t hc::Video::getCurrentFramebuffer() {
    return 0;
}

retro_proc_address_t hc::Video::getProcAddress(char const* symbol) {
    return nullptr;
}
