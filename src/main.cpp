#include "Logger.h"
#include "Config.h"
#include "Video.h"
#include "Fifo.h"

#include "gamecontrollerdb.h"

#include <imgui.h>
#include <imgui_impl_sdl.h>
#include <imgui_impl_opengl2.h>
#include <SDL.h>
#include <SDL_opengl.h>

#include <Frontend.h>

#include <ProggyTiny.inl>

#include <FontAwesome4.inl>
#include <IconsFontAwesome4.h>

#include <MaterialDesign.inl>
#include <IconsMaterialDesign.h>

#include <stdlib.h>
#include <sys/stat.h>

class Application final {
protected:
    SDL_Window* _window;
    SDL_GLContext _glContext;
    SDL_AudioSpec _audioSpec;
    SDL_AudioDeviceID _audioDev;

    hc::Logger _logger;
public:
    hc::Config _config;
protected:
    hc::Video _video;

    hc::Fifo _fifo;

public:
    lrcpp::Frontend _frontend;
protected:

    static void audioCallback(void* const udata, Uint8* const stream, int const len) {
        auto const app = static_cast<Application*>(udata);
        size_t const avail = app->_fifo.occupied();

        if (avail < (size_t)len) {
            app->_fifo.read(static_cast<void*>(stream), avail);
            memset(static_cast<void*>(stream + avail), 0, len - avail);
        }
        else {
            app->_fifo.read(static_cast<void*>(stream), len);
        }
    }

public:
    Application() {}

    bool init(std::string const& title, int const width, int const height) {
        if (!_logger.init()) {
            return false;
        }

        {
            // Setup SDL
            if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
                _logger.error("Error in SDL_Init: %s", SDL_GetError());
                return false;
            }

            // Setup window
            SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
            SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
            SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

            Uint32 const windowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED;

            _window = SDL_CreateWindow(
                title.c_str(),
                SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                width, height,
                windowFlags
            );

            if (_window == nullptr) {
                _logger.error("Error in SDL_CreateWindow: %s", SDL_GetError());
                return false;
            }

            _glContext = SDL_GL_CreateContext(_window);

            if (_glContext == nullptr) {
                _logger.error("Error in SDL_GL_CreateContext: %s", SDL_GetError());
                SDL_DestroyWindow(_window);
                return false;
            }

            SDL_GL_MakeCurrent(_window, _glContext);
            SDL_GL_SetSwapInterval(1);

            // Init audio
            SDL_AudioSpec want;
            memset(&want, 0, sizeof(want));

            want.freq = 44100;
            want.format = AUDIO_S16SYS;
            want.channels = 2;
            want.samples = 1024;
            want.callback = audioCallback;
            want.userdata = this;

            _audioDev = SDL_OpenAudioDevice(
                nullptr, 0,
                &want, &_audioSpec,
                SDL_AUDIO_ALLOW_FREQUENCY_CHANGE | SDL_AUDIO_ALLOW_CHANNELS_CHANGE
            );

            if (_audioDev == 0) {
                _logger.error("Error in SDL_OpenAudioDevice: %s", SDL_GetError());
                SDL_GL_DeleteContext(_glContext);
                SDL_DestroyWindow(_window);
                return false;
            }

            if (!_fifo.init(_audioSpec.size * 2)) {
                _logger.error("Error in audio FIFO init");
                SDL_CloseAudioDevice(_audioDev);
                SDL_GL_DeleteContext(_glContext);
                SDL_DestroyWindow(_window);
                return false;
            }

            SDL_PauseAudioDevice(_audioDev, 0);

            // Add controller mappings
            SDL_RWops* const ctrldb = SDL_RWFromMem(
                const_cast<void*>(static_cast<void const*>(gamecontrollerdb)),
                static_cast<int>(gamecontrollerdb_len)
            );

            if (SDL_GameControllerAddMappingsFromRW(ctrldb, 1) < 0) {
                _logger.error("Error in SDL_GameControllerAddMappingsFromRW: %s", SDL_GetError());
                SDL_CloseAudioDevice(_audioDev);
                _fifo.destroy();
                SDL_GL_DeleteContext(_glContext);
                SDL_DestroyWindow(_window);
                return false;
            }
        }

        {
            // Setup ImGui
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO();
            io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

            ImGui::StyleColorsDark();

            ImGui_ImplSDL2_InitForOpenGL(_window, _glContext);
            ImGui_ImplOpenGL2_Init();

            // Set Proggy Tiny as the default font
            io = ImGui::GetIO();

            io.Fonts->AddFontFromMemoryCompressedTTF(
                ProggyTiny_compressed_data,
                ProggyTiny_compressed_size,
                10.0f
            );

            // Add icons from Font Awesome
            ImFontConfig config;
            config.MergeMode = true;
            config.PixelSnapH = true;

            static ImWchar const ranges1[] = {ICON_MIN_FA, ICON_MAX_FA, 0};

            io.Fonts->AddFontFromMemoryCompressedTTF(
                FontAwesome4_compressed_data,
                FontAwesome4_compressed_size,
                12.0f, &config, ranges1
            );

            // Add Icons from Material Design
            static ImWchar const ranges2[] = {ICON_MIN_MD, ICON_MAX_MD, 0};

            io.Fonts->AddFontFromMemoryCompressedTTF(
                MaterialDesign_compressed_data,
                MaterialDesign_compressed_size,
                24.0f, &config, ranges2
            );
        }

        {
            // Initialize components
            if (!_config.init(&_logger)) {
error:
                SDL_CloseAudioDevice(_audioDev);
                _fifo.destroy();
                SDL_GL_DeleteContext(_glContext);
                SDL_DestroyWindow(_window);
                return false;
            }

            if (!_video.init(&_logger)) {
                goto error;
            }

            _frontend.setLogger(&_logger);
            _frontend.setConfig(&_config);
            _frontend.setVideo(&_video);
        }

        return true;
    }

    void destroy() {
        _config.destroy();
        _logger.destroy();

        ImGui_ImplOpenGL2_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();

        SDL_CloseAudioDevice(_audioDev);
        _fifo.destroy();

        SDL_GL_DeleteContext(_glContext);
        SDL_DestroyWindow(_window);
        SDL_Quit();
    }

    void run() {
        bool done = false;

        do {
            SDL_Event event;

            while (SDL_PollEvent(&event)) {
                ImGui_ImplSDL2_ProcessEvent(&event);

                switch (event.type) {
                    case SDL_QUIT:
                        done = true;
                        break;

                    case SDL_CONTROLLERDEVICEADDED:
                    case SDL_CONTROLLERDEVICEREMOVED:
                    case SDL_CONTROLLERBUTTONUP:
                    case SDL_CONTROLLERBUTTONDOWN:
                    case SDL_CONTROLLERAXISMOTION:
                    case SDL_KEYUP:
                    case SDL_KEYDOWN:
                        // _input.processEvent(&event);
                        break;
                }
            }

            _frontend.run();

            ImGui_ImplOpenGL2_NewFrame();
            ImGui_ImplSDL2_NewFrame(_window);
            ImGui::NewFrame();

            draw();

            ImGui::Render();

            glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y);
            glClearColor(0.05f, 0.05f, 0.05f, 0);
            glClear(GL_COLOR_BUFFER_BIT);

            ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());

            SDL_GL_SwapWindow(_window);
            SDL_Delay(1);
        }
        while (!done);
    }

    void draw() {
        ImGui::DockSpaceOverViewport();

        if (ImGui::Begin(ICON_FA_COMMENT " Log")) {
            _logger.draw();
        }

        ImGui::End();

        if (ImGui::Begin(ICON_FA_WRENCH " Configuration")) {
            _config.draw();
        }

        ImGui::End();

        if (ImGui::Begin(ICON_FA_DESKTOP " Video")) {
          _video.draw();
        }

        ImGui::End();
    }
};

static void const* readAll(char const* const path, size_t* const size) {
    struct stat statbuf;

    if (stat(path, &statbuf) != 0) {
        //logger->printf(RETRO_LOG_ERROR, "Error getting content info: %s", strerror(errno));
        return nullptr;
    }

    void* const data = malloc(statbuf.st_size);

    if (data == nullptr) {
        //logger->printf(RETRO_LOG_ERROR, "Out of memory allocating %u bytes", statbuf.st_size);
        return nullptr;
    }

    FILE* file = fopen(path, "rb");

    if (file == nullptr) {
        //logger->printf(RETRO_LOG_ERROR, "Error opening content: %s", strerror(errno));
        free(data);
        return nullptr;
    }

    size_t numread = fread(data, 1, statbuf.st_size, file);

    if (numread != (size_t)statbuf.st_size) {
        //logger->printf(RETRO_LOG_ERROR, "Error reading content: %s", strerror(errno));
        fclose(file);
        free(data);
        return nullptr;
    }

    fclose(file);

    *size = numread;
    return data;
}

int main(int, char**) {
    Application app;

    if (!app.init("Hackable Console", 1024, 640)) {
        return EXIT_FAILURE;
    }

    // TEST load a core and a game
    char const* coresPath;
    app._config.getLibretroPath(&coresPath);
    std::string pico = coresPath;
    pico += "/picodrive_libretro.so";

    if (!app._frontend.load(pico.c_str())) {
        fprintf(stderr, "Error loading core from %s\n", pico.c_str());
        app.destroy();
        return EXIT_FAILURE;
    }

    size_t size = 0;
    void const* const data = readAll("/home/leiradel/ZombiesAteMyNeighbors/genesis_cartridge.bin", &size);

    if (data == nullptr) {
        fprintf(stderr, "Error loading game\n");
        app.destroy();
        return EXIT_FAILURE;
    }

    if (!app._frontend.loadGame("/home/leiradel/ZombiesAteMyNeighbors/genesis_cartridge.bin", data, size)) {
        fprintf(stderr, "Error loading game 2\n");
        app.destroy();
        return EXIT_FAILURE;
    }

    app.run();
    app.destroy();
    return EXIT_SUCCESS;
}
