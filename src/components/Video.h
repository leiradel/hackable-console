#pragma once

#include "Desktop.h"

#include <lrcpp/Components.h>

#include <imgui.h>
#include <SDL_opengl.h>

#include <stdint.h>
#include <vector>
#include <mutex>

namespace hc {
    class Video: public View, public lrcpp::Video {
    public:
        Video(Desktop* desktop);
        virtual ~Video() {}

        void init();
        void flush();

        double getCoreFps();
        bool getMousePos(int* const x, int* const y) const;

        // hc::View
        virtual char const* getTitle() override;
        virtual void onDraw() override;
        virtual void onGameUnloaded() override;
        virtual void onCoreUnloaded() override;

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
        void setupTexture(unsigned const width, unsigned const height, retro_pixel_format const format);

        unsigned _width;
        unsigned _height;
        unsigned _maxWidth;
        unsigned _maxHeight;
        float _aspect;
        retro_pixel_format _format;
        size_t _pitch;
        double _coreFps;
        std::vector<uint8_t> _pixels;
        std::mutex _mutex;

        unsigned _rotation;

        GLuint _texture;
        unsigned _textureWidth;
        unsigned _textureHeight;

        ImVec2 _texturePos;
        ImVec2 _mousePos;
        bool _mouseOnTexture;
    };
}
