#pragma once

#include "Logger.h"

#include <Components.h>

namespace hc {
    class Video: public lrcpp::Video {
    public:
        Video() : _logger(nullptr) {}
        virtual ~Video() {}

        bool init(Logger* logger);
        void destroy();
        void reset();
        void draw();

        virtual bool setRotation(unsigned rotation) override;
        virtual bool getOverscan(bool* overscan) override;
        virtual bool getCanDupe(bool* canDupe) override;
        virtual bool showMessage(retro_message const* message) override;
        virtual bool setPixelFormat(retro_pixel_format format) override;
        virtual bool setHwRender(retro_hw_render_callback* callback) override;
        virtual bool setFrameTimeCallback(retro_frame_time_callback const* callback) override;
        virtual bool setSystemAvInfo(retro_system_av_info const* info) override;
        virtual bool setGeometry(retro_game_geometry const* geometry) override;
        virtual bool getCurrentSoftwareFramebuffer(retro_framebuffer* framebuffer) override;
        virtual bool getHwRenderInterface(retro_hw_render_interface const** interface) override;
        virtual bool setHwRenderContextNegotiationInterface(retro_hw_render_context_negotiation_interface const* interface) override;
        virtual bool setHwSharedContext() override;
        virtual bool getTargetRefreshRate(float* rate) override;
        virtual bool getPreferredHwRender(unsigned* preferred) override;

        virtual void refresh(void const* data, unsigned width, unsigned height, size_t pitch) override;

        virtual uintptr_t getCurrentFramebuffer() override;
        virtual retro_proc_address_t getProcAddress(char const* symbol) override;

    protected:
        Logger* _logger;

        unsigned _rotation;
        retro_pixel_format _pixelFormat;
        retro_system_av_info _systemAvInfo;
    };
}
