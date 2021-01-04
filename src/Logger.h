#pragma once

#include <Components.h>
#include <imguial_term.h>

#include <SDL.h>

namespace hc {
    class Logger: public lrcpp::Logger {
    public:
        Logger() {}
        virtual ~Logger() {}

        bool init();
        void destroy();
        void reset();
        void draw();

        virtual void vprintf(retro_log_level level, char const* format, va_list args) override;

    protected:
        ImGuiAl::BufferedLog<1024 * 1024> _logger;
        SDL_mutex* _mutex;
    };
}
