#include "Logger.h"
#include "Config.h"
#include "Audio.h"
#include "Video.h"
#include "Fifo.h"

#include "LuaBind.h"

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

extern "C" {
    #include <lua.h>
    #include <lauxlib.h>
    #include <lualib.h>
}

#include <stdlib.h>
#include <sys/stat.h>

static SDL_Window* window;
static SDL_GLContext glContext;
static SDL_AudioSpec audioSpec;
static SDL_AudioDeviceID audioDev;

static hc::Logger logger;
static hc::Config config;
static hc::Audio audio;
static hc::Video video;

static lrcpp::Frontend frontend;

static hc::Fifo fifo;

static lua_State* L;

static void audioCallback(void* const udata, Uint8* const stream, int const len) {
    (void)udata;
    size_t const avail = fifo.occupied();

    if (avail < (size_t)len) {
        fifo.read(static_cast<void*>(stream), avail);
        memset(static_cast<void*>(stream + avail), 0, len - avail);
    }
    else {
        fifo.read(static_cast<void*>(stream), len);
    }
}

static int luaopen_hc(lua_State* const L) {
    //static luaL_Reg const functions[] = {};

    static struct {char const* const name; std::function<void(lua_State* const L)> const pusher;} const constants[] = {
        {"logger", [](lua_State* const L) { logger.push(L); }},
        {"config", [](lua_State* const L) { config.push(L); }}
    };

    static struct {char const* const name; char const* const value;} const info[] = {
        {"_COPYRIGHT", "Copyright (c) 2020 Andre Leiradella"},
        {"_LICENSE", "MIT"},
        {"_VERSION", "1.0.0"},
        {"_NAME", "hc"},
        {"_URL", "https://github.com/leiradel/hackable-console"},
        {"_DESCRIPTION", "Hackable Console bindings"}
    };

    size_t const functions_count = 0; //sizeof(functions) / sizeof(functions[0]) - 1;
    size_t const constants_count = sizeof(constants) / sizeof(constants[0]);
    size_t const info_count = sizeof(info) / sizeof(info[0]);

    lua_createtable(L, 0, functions_count + constants_count + info_count);
    //luaL_setfuncs(L, functions, 0);

    for (size_t i = 0; i < constants_count; i++) {
        constants[i].pusher(L);
        lua_setfield(L, -2, constants[i].name);
    }

    for (size_t i = 0; i < info_count; i++) {
        lua_pushstring(L, info[i].value);
        lua_setfield(L, -2, info[i].name);
    }

    return 1;
}

static bool init(std::string const& title, int const width, int const height) {
    class Undo {
    public:
        ~Undo() {
            for (size_t i = _list.size(); i != 0; i--) {
                _list[i - 1]();
            }
        }

        void add(void (*undo)()) {
            _list.emplace_back(undo);
        }

        void clear() {
            _list.clear();
        }

    protected:
        std::vector<void (*)()> _list;
    }
    undo;

    if (!logger.init()) {
        return false;
    }

    undo.add([]() { logger.destroy(); });

    {
        // Setup SDL
        if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
            logger.error("Error in SDL_Init: %s", SDL_GetError());
            return false;
        }

        undo.add([]() { SDL_Quit(); });

        // Setup window
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

        Uint32 const windowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED;

        window = SDL_CreateWindow(
            title.c_str(),
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            width, height,
            windowFlags
        );

        if (window == nullptr) {
            logger.error("Error in SDL_CreateWindow: %s", SDL_GetError());
            return false;
        }

        undo.add([]() { SDL_DestroyWindow(window); });

        glContext = SDL_GL_CreateContext(window);

        if (glContext == nullptr) {
            logger.error("Error in SDL_GL_CreateContext: %s", SDL_GetError());
            return false;
        }

        undo.add([]() { SDL_GL_DeleteContext(glContext); });

        SDL_GL_MakeCurrent(window, glContext);
        SDL_GL_SetSwapInterval(1);

        // Init audio
        SDL_AudioSpec want;
        memset(&want, 0, sizeof(want));

        want.freq = 44100;
        want.format = AUDIO_S16SYS;
        want.channels = 2;
        want.samples = 1024;
        want.callback = audioCallback;
        want.userdata = nullptr;

        audioDev = SDL_OpenAudioDevice(
            nullptr, 0,
            &want, &audioSpec,
            SDL_AUDIO_ALLOW_FREQUENCY_CHANGE | SDL_AUDIO_ALLOW_CHANNELS_CHANGE
        );

        if (audioDev == 0) {
            logger.error("Error in SDL_OpenAudioDevice: %s", SDL_GetError());
            return false;
        }

        SDL_PauseAudioDevice(audioDev, 0);

        undo.add([]() { SDL_CloseAudioDevice(audioDev); });

        if (!fifo.init(audioSpec.size * 2)) {
            logger.error("Error in audio FIFO init");
            return false;
        }

        undo.add([]() { fifo.destroy(); });

        // Add controller mappings
        SDL_RWops* const ctrldb = SDL_RWFromMem(
            const_cast<void*>(static_cast<void const*>(gamecontrollerdb)),
            static_cast<int>(gamecontrollerdb_len)
        );

        if (SDL_GameControllerAddMappingsFromRW(ctrldb, 1) < 0) {
            logger.error("Error in SDL_GameControllerAddMappingsFromRW: %s", SDL_GetError());
            return false;
        }
    }

    {
        // Setup ImGui
        IMGUI_CHECKVERSION();
        if (ImGui::CreateContext() == nullptr) {
            logger.error("Error creating ImGui context");
            return false;
        }

        undo.add([]() { ImGui::DestroyContext(); });

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        ImGui::StyleColorsDark();

        if (!ImGui_ImplSDL2_InitForOpenGL(window, glContext)) {
            logger.error("Error initializing ImGui for OpenGL");
            return false;
        }

        undo.add([]() { ImGui_ImplSDL2_Shutdown(); });

        if (!ImGui_ImplOpenGL2_Init()) {
            logger.error("Error initializing ImGui OpenGL implementation");
            return false;
        }

        undo.add([]() { ImGui_ImplOpenGL2_Shutdown(); });

        // Set Proggy Tiny as the default font
        io = ImGui::GetIO();

        ImFont* const proggyTiny = io.Fonts->AddFontFromMemoryCompressedTTF(
            ProggyTiny_compressed_data,
            ProggyTiny_compressed_size,
            10.0f
        );

        if (proggyTiny == nullptr) {
            logger.error("Error adding Proggy Tiny font");
            return false;
        }

        // Add icons from Font Awesome
        ImFontConfig config;
        config.MergeMode = true;
        config.PixelSnapH = true;

        static ImWchar const ranges1[] = {ICON_MIN_FA, ICON_MAX_FA, 0};

        ImFont* const fontAwesome = io.Fonts->AddFontFromMemoryCompressedTTF(
            FontAwesome4_compressed_data,
            FontAwesome4_compressed_size,
            12.0f, &config, ranges1
        );

        if (fontAwesome == nullptr) {
            logger.error("Error adding Font Awesome 4 font");
            return false;
        }

        // Add Icons from Material Design
        static ImWchar const ranges2[] = {ICON_MIN_MD, ICON_MAX_MD, 0};

        ImFont* const materialDesign = io.Fonts->AddFontFromMemoryCompressedTTF(
            MaterialDesign_compressed_data,
            MaterialDesign_compressed_size,
            24.0f, &config, ranges2
        );

        if (materialDesign == nullptr) {
            logger.error("Error adding Material Design font");
            return false;
        }
    }

    {
        // Initialize components
        if (!config.init(&logger)) {
            return false;
        }

        undo.add([]() { config.destroy(); });

        if (!video.init(&logger)) {
            return false;
        }

        undo.add([]() { video.destroy(); });

        if (!audio.init(&logger, audioSpec.freq, &fifo)) {
            return false;
        }

        undo.add([]() { audio.destroy(); });

        frontend.setLogger(&logger);
        frontend.setConfig(&config);
        frontend.setAudio(&audio);
        frontend.setVideo(&video);
    }

    {
        // Initialize Lua
        L = luaL_newstate();

        if (L == nullptr) {
            return false;
        }

        undo.add([]() { lua_close(L); });

        luaL_openlibs(L);
        RegisterSearcher(L);

        static auto const traceback = [](lua_State* const L) -> int {
            luaL_traceback(L, L, lua_tostring(L, -1), 1);
            return 1;
        };

        static auto const main = [](lua_State* const L) -> int {
            char const* const path = luaL_checkstring(L, 1);

            if (luaL_loadfilex(L, path, "t") != LUA_OK) {
                return lua_error(L);
            }

            lua_call(L, 0, 0);
            return 0;
        };

        std::string const& autorun = config.getAutorunPath();

        lua_pushcfunction(L, traceback);
        lua_pushcfunction(L, main);
        lua_pushlstring(L, autorun.c_str(), autorun.length());
        
        logger.info("Running \"%s\"", autorun.c_str());

        if (lua_pcall(L, 1, 0, -3) != LUA_OK) {
            logger.error("%s", lua_tostring(L, -1));
            lua_pop(L, 1);
        }

        lua_pop(L, 1);
    }

    undo.clear();
    return true;
}

static void destroy() {
    lua_close(L);

    video.destroy();
    audio.destroy();
    config.destroy();
    logger.destroy();

    ImGui_ImplOpenGL2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_CloseAudioDevice(audioDev);
    fifo.destroy();

    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

static void draw() {
    ImGui::DockSpaceOverViewport();

    if (ImGui::Begin(ICON_FA_COMMENT " Log")) {
        logger.draw();
    }

    ImGui::End();

    if (ImGui::Begin(ICON_FA_WRENCH " Configuration")) {
        config.draw();
    }

    ImGui::End();

    if (ImGui::Begin(ICON_FA_VOLUME_UP " Audio")) {
        audio.draw();
    }

    ImGui::End();

    if (ImGui::Begin(ICON_FA_DESKTOP " Video")) {
        video.draw();
    }

    ImGui::End();
}

static void run() {
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

        frontend.run();

        ImGui_ImplOpenGL2_NewFrame();
        ImGui_ImplSDL2_NewFrame(window);
        ImGui::NewFrame();

        draw();
        audio.flush();

        ImGui::Render();

        glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y);
        glClearColor(0.05f, 0.05f, 0.05f, 0);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());

        SDL_GL_SwapWindow(window);
        SDL_Delay(1);
    }
    while (!done);
}

int main(int, char**) {
    if (!init("Hackable Console", 1024, 640)) {
        return EXIT_FAILURE;
    }

    run();
    destroy();
    return EXIT_SUCCESS;
}
