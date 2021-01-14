#pragma once

#include "Plugin.h"
#include "Logger.h"

#include <Components.h>

#include <SDL_opengl.h>

#include <stdint.h>

namespace hc {
    class Video: public Plugin, public lrcpp::Video {
    public:
        Video();
        virtual ~Video() {}

        void init(Logger* logger);

        // hc::Plugin
        virtual char const* getName() override;
        virtual char const* getVersion() override;
        virtual char const* getLicense() override;
        virtual char const* getCopyright() override;
        virtual char const* getUrl() override;

        virtual void onStarted() override;
        virtual void onConsoleLoaded() override;
        virtual void onGameLoaded() override;
        virtual void onGamePaused() override;
        virtual void onGameResumed() override;
        virtual void onGameReset() override;
        virtual void onFrame() override;
        virtual void onDraw() override;
        virtual void onGameUnloaded() override;
        virtual void onConsoleUnloaded() override;
        virtual void onQuit() override;

        // lrcpp::Video
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
        void setupTexture(unsigned width, unsigned height);

        Logger* _logger;

        unsigned _rotation;
        retro_pixel_format _pixelFormat;

        GLuint _texture;
        unsigned _textureWidth;
        unsigned _textureHeight;
        float _aspectRatio;
        unsigned _width;
        unsigned _height;

        uint64_t _frameCount;
        int64_t _timeStarted;
        double _fps;
    };
}
