#pragma once

#include "Logger.h"
#include "Config.h"
#include "Audio.h"
#include "Video.h"
#include "Fifo.h"

#include "LuaBind.h"

#include <Frontend.h>

#include <SDL.h>
#include <SDL_opengl.h>

extern "C" {
    #include <lua.h>
}

namespace hc {
    class Application {
    public:
        bool init(std::string const& title, int const width, int const height);
        void destroy();
        void draw();
        void run();

        int luaopen_hc(lua_State* const L);

    public:
        static void audioCallback(void* const udata, Uint8* const stream, int const len);

        static int l_registerSystem(lua_State* const L);

        SDL_Window* _window;
        SDL_GLContext _glContext;
        SDL_AudioSpec _audioSpec;
        SDL_AudioDeviceID _audioDev;

        Logger _logger;
        Config _config;
        Audio _audio;
        Video _video;

        lrcpp::Frontend _frontend;

        Fifo _fifo;
        lua_State* _L;
    };
}