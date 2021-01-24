#include "Application.h"
#include "Control.h"
#include "LuaUtil.h"

#include "gamecontrollerdb.h"

#include <imgui.h>
#include <imgui_impl_sdl.h>
#include <imgui_impl_opengl2.h>

#include <ProggyTiny.inl>

#include <FontAwesome4.inl>
#include <IconsFontAwesome4.h>

#include <stdlib.h>
#include <sys/stat.h>

extern "C" {
    #include "lauxlib.h"
    #include "lualib.h"
}

#define TAG "[HC ] "

static void const* readAll(hc::Logger* logger, char const* const path, size_t* const size) {
    struct stat statbuf;

    if (stat(path, &statbuf) != 0) {
        logger->error(TAG "Error getting content info: %s", strerror(errno));
        return nullptr;
    }

    void* const data = malloc(statbuf.st_size);

    if (data == nullptr) {
        logger->error(TAG "Out of memory allocating %zu bytes", statbuf.st_size);
        return nullptr;
    }

    FILE* file = fopen(path, "rb");

    if (file == nullptr) {
        logger->error(TAG "Error opening content: %s", strerror(errno));
        free(data);
        return nullptr;
    }

    size_t numread = fread(data, 1, statbuf.st_size, file);

    if (numread != (size_t)statbuf.st_size) {
        logger->error(TAG "Error reading content: %s", strerror(errno));
        fclose(file);
        free(data);
        return nullptr;
    }

    fclose(file);

    logger->info(TAG "Loaded content from \"%s\", %zu bytes", path, numread);
    *size = numread;
    return data;
}

hc::Application::Application()
    : _fsm(*this, lifeCycleVprintf, this)
    , _logger(this)
    , _config(this)
    , _video(this)
    , _led(this)
    , _audio(this)
    , _input(this)
    , _perf(this)
    , _control(this)
    , _memory(this)
{}

bool hc::Application::init(std::string const& title, int const width, int const height) {
    class Undo {
    public:
        ~Undo() {
            for (size_t i = _list.size(); i != 0; i--) {
                _list[i - 1]();
            }
        }

        void add(std::function<void()> const& undo) {
            _list.emplace_back(undo);
        }

        void clear() {
            _list.clear();
        }

    protected:
        std::vector<std::function<void()>> _list;
    }
    undo;

    Desktop::init();

    addView(&_logger, true, false);

    if (!_logger.init()) {
        return false;
    }

    {
        // Redirect SDL logs
        SDL_LogSetOutputFunction(sdlPrint, this);
        SDL_LogSetAllPriority(SDL_LOG_PRIORITY_VERBOSE);

        // Setup SDL
        if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
            error(TAG "Error in SDL_Init: %s", SDL_GetError());
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

        _window = SDL_CreateWindow(
            title.c_str(),
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            width, height,
            windowFlags
        );

        if (_window == nullptr) {
            error(TAG "Error in SDL_CreateWindow: %s", SDL_GetError());
            return false;
        }

        undo.add([this]() { SDL_DestroyWindow(_window); });

        _glContext = SDL_GL_CreateContext(_window);

        if (_glContext == nullptr) {
            error(TAG "Error in SDL_GL_CreateContext: %s", SDL_GetError());
            return false;
        }

        undo.add([this]() { SDL_GL_DeleteContext(_glContext); });

        SDL_GL_MakeCurrent(_window, _glContext);
        SDL_GL_SetSwapInterval(0);

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
            error(TAG "Error in SDL_OpenAudioDevice: %s", SDL_GetError());
            return false;
        }

        undo.add([this]() { SDL_CloseAudioDevice(_audioDev); });

        if (!_fifo.init(_audioSpec.size * 2)) {
            error(TAG "Error in audio FIFO init");
            return false;
        }

        undo.add([this]() { _fifo.destroy(); });

        SDL_PauseAudioDevice(_audioDev, 0);

        // Add controller mappings
        SDL_RWops* const ctrldb = SDL_RWFromMem(
            const_cast<void*>(static_cast<void const*>(gamecontrollerdb)),
            static_cast<int>(gamecontrollerdb_len)
        );

        if (SDL_GameControllerAddMappingsFromRW(ctrldb, 1) < 0) {
            error(TAG "Error in SDL_GameControllerAddMappingsFromRW: %s", SDL_GetError());
            return false;
        }
    }

    {
        // Setup ImGui
        IMGUI_CHECKVERSION();

        if (ImGui::CreateContext() == nullptr) {
            error(TAG "Error creating ImGui context");
            return false;
        }

        undo.add([]() { ImGui::DestroyContext(); });

        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowRounding = 0.0f;
        style.FrameRounding = 0.0f;
        style.ScrollbarRounding = 0.0f;
        style.GrabRounding = 0.0f;
        style.TabRounding = 0.0f;

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        ImGui::StyleColorsDark();

        if (!ImGui_ImplSDL2_InitForOpenGL(_window, _glContext)) {
            error(TAG "Error initializing ImGui for OpenGL");
            return false;
        }

        undo.add([]() { ImGui_ImplSDL2_Shutdown(); });

        if (!ImGui_ImplOpenGL2_Init()) {
            error(TAG "Error initializing ImGui OpenGL implementation");
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
            error(TAG "Error adding Proggy Tiny font");
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
            error(TAG "Error adding Font Awesome 4 font");
            return false;
        }
    }

    {
        // Initialize components (logger has already been initialized)
        lrcpp::Frontend& frontend = lrcpp::Frontend::getInstance();

        addView(&_config, true, false);
        addView(&_video, true, false);
        addView(&_led, true, false);
        addView(&_audio, true, false);
        addView(&_input, true, false);
        addView(&_perf, true, false);

        addView(&_control, true, false);
        addView(&_memory, true, false);

        if (!_config.init()) {
            return false;
        }

        _video.init();
        _led.init();
        _audio.init(_audioSpec.freq, &_fifo);
        _input.init(&frontend);
        _perf.init();

        _control.init(&_fsm);
        _memory.init();

        frontend.setLogger(&_logger);
        frontend.setConfig(&_config);
        frontend.setVideo(&_video);
        frontend.setLed(&_led);
        frontend.setAudio(&_audio);
        frontend.setInput(&_input);
        frontend.setPerf(&_perf);
    }

    {
        // Initialize Lua
        _L = luaL_newstate();

        if (_L == nullptr) {
            return false;
        }

        undo.add([this]() { lua_close(_L); });

        luaL_openlibs(_L);
        push(_L);
        registerSearcher(_L);

        static auto const main = [](lua_State* const L) -> int {
            char const* const path = luaL_checkstring(L, 1);

            if (luaL_loadfilex(L, path, "t") != LUA_OK) {
                return lua_error(L);
            }

            lua_call(L, 0, 0);
            return 0;
        };

        std::string const& autorun = _config.getAutorunPath();

        lua_pushcfunction(_L, main);
        lua_pushlstring(_L, autorun.c_str(), autorun.length());
        
        info(TAG "Running \"%s\"", autorun.c_str());

        if (!protectedCall(_L, 1, 0, &_logger)) {
            return false;
        }
    }

    undo.clear();
    onStarted();
    return true;
}

void hc::Application::destroy() {
    Desktop::onQuit();
    lua_close(_L);

    ImGui_ImplOpenGL2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_CloseAudioDevice(_audioDev);
    _fifo.destroy();

    SDL_GL_DeleteContext(_glContext);
    SDL_DestroyWindow(_window);
    SDL_Quit();
}

void hc::Application::run() {
    bool done = false;
    lrcpp::Frontend& frontend = lrcpp::Frontend::getInstance();

    do {
        SDL_Event event;

        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);

            switch (event.type) {
                case SDL_QUIT:
                    done = _fsm.quit();
                    break;

                case SDL_CONTROLLERDEVICEADDED:
                case SDL_CONTROLLERDEVICEREMOVED:
                case SDL_CONTROLLERBUTTONUP:
                case SDL_CONTROLLERBUTTONDOWN:
                case SDL_CONTROLLERAXISMOTION:
                case SDL_KEYUP:
                case SDL_KEYDOWN:
                case SDL_JOYDEVICEADDED:
                    _input.processEvent(&event);
                    break;
            }
        }

        if (_fsm.currentState() == LifeCycle::State::GameRunning) {
            if (!_syncExact || _runningTime.getTimeUs() >= _nextFrameTime) {
                _nextFrameTime += _coreUsPerFrame;

                _perf.start(&_runPerf);
                frontend.run();
                _perf.stop(&_runPerf);

                _audio.flush();
                onFrame();
            }
        }

        ImGui_ImplOpenGL2_NewFrame();
        ImGui_ImplSDL2_NewFrame(_window);
        ImGui::NewFrame();

        ImGui::End();

        onDraw();

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

bool hc::Application::loadCore(char const* path) {
    info(TAG "Loading core \"%s\"", path);

    lrcpp::Frontend& frontend = lrcpp::Frontend::getInstance();

    if (!frontend.load(path)) {
        return false;
    }

    retro_system_info sysinfo;

    if (frontend.getSystemInfo(&sysinfo)) {
        _control.setSystemInfo(&sysinfo);
    }

    info(TAG "System Info");
    info(TAG "    library_name     = %s", sysinfo.library_name);
    info(TAG "    library_version  = %s", sysinfo.library_version);
    info(TAG "    valid_extensions = %s", sysinfo.valid_extensions);
    info(TAG "    need_fullpath    = %s", sysinfo.need_fullpath ? "true" : "false");
    info(TAG "    block_extract    = %s", sysinfo.block_extract ? "true" : "false");

    onCoreLoaded();
    return true;
}

bool hc::Application::loadGame(char const* path) {
    info(TAG "Loading game from \"%s\"", path);

    lrcpp::Frontend& frontend = lrcpp::Frontend::getInstance();
    retro_system_info sysinfo;

    if (!frontend.getSystemInfo(&sysinfo)) {
        return false;
    }

    bool ok = false;

    if (sysinfo.need_fullpath) {
        ok = frontend.loadGame(path);
    }
    else {
        size_t size = 0;
        void const* data = readAll(&_logger, path, &size);

        if (data == nullptr) {
            return false;
        }

        ok = frontend.loadGame(path, data, size);
        free(const_cast<void*>(data));
    }

    if (!ok) {
        return false;
    }

    static struct {char const* const name; unsigned const id;} memory[] = {
        {"save", RETRO_MEMORY_SAVE_RAM},
        {"rtc", RETRO_MEMORY_RTC},
        {"sram", RETRO_MEMORY_SYSTEM_RAM},
        {"vram", RETRO_MEMORY_VIDEO_RAM}
    };

    info(TAG "Core memory");
    bool any = false;

    for (size_t i = 0; i < sizeof(memory) / sizeof(memory[0]); i++) {
        void* data = nullptr;
        size_t size = 0;

        if (frontend.getMemoryData(memory[i].id, &data) && frontend.getMemorySize(memory[i].id, &size) && size != 0) {
            info(TAG "    %-4s %p %zu bytes", memory[i].name, data, size);
            any = true;
        }
    }

    if (!any) {
        info(TAG "    No core memory exposed via the get_memory interface");
    }

    onGameLoaded();
    return true;
}

bool hc::Application::pauseGame() {
    onGamePaused();
    return true;
}

bool hc::Application::quit() {
    onQuit();
    return true;
}

bool hc::Application::resetGame() {
    onGameReset();
    return lrcpp::Frontend::getInstance().reset();
}

bool hc::Application::resumeGame() {
    onGameResumed();
    return true;
}

bool hc::Application::startGame() {
    onGameStarted();
    return true;
}

bool hc::Application::step() {
    _perf.start(&_runPerf);
    bool const ok = lrcpp::Frontend::getInstance().run();
    _perf.stop(&_runPerf);

    onFrame();
    return ok;
}

bool hc::Application::unloadCore() {
    if (lrcpp::Frontend::getInstance().unload()) {
        onCoreUnloaded();
        return true;
    }

    return false;
}

bool hc::Application::unloadGame() {
    if (lrcpp::Frontend::getInstance().unloadGame()) {
        onGameUnloaded();
        _runPerf.start = _runPerf.total = _runPerf.call_cnt = 0;
        return true;
    }

    return false;
}

char const* hc::Application::getTitle() {
    return ICON_FA_PLUG " Desktop";
}

void hc::Application::onCoreLoaded() {
    // Perf has to unregister all counters when a core is unloaded so we
    // register this here.
    _runPerf.ident = "hc::retro_run";
    _perf.register_(&_runPerf);

    Desktop::onCoreLoaded();
}

void hc::Application::onGameLoaded() {
    Desktop::onGameLoaded();
    _coreUsPerFrame = 1000000.0 / _video.getCoreFps();
}

void hc::Application::onGameStarted() {
    Desktop::onGameStarted();
    _runningTime.start();
}

void hc::Application::onGamePaused() {
    Desktop::onGamePaused();
    _runningTime.pause();
}

void hc::Application::onGameResumed() {
    Desktop::onGameResumed();

    _runningTime.resume();
    _nextFrameTime = _runningTime.getTimeUs() + _coreUsPerFrame;
}

void hc::Application::onDraw() {
    bool on = false;

    if (_config.vsync(&on)) {
        resetDrawFps();
        resetFrameFps();
        SDL_GL_SetSwapInterval(on ? 1 : 0);
    }

    if (_config.syncExact(&_syncExact)) {
        resetFrameFps();

        if (_syncExact) {
            _nextFrameTime = _runningTime.getTimeUs();
        }
    }

    ImGui::DockSpaceOverViewport();
    Desktop::onDraw();
}

void hc::Application::onGameUnloaded() {
    Desktop::onGameUnloaded();
    _runningTime.stop();
}

int hc::Application::push(lua_State* const L) {
    static struct {char const* const name; char const* const value;} const stringConsts[] = {
        {"_COPYRIGHT", "Copyright (c) 2020 Andre Leiradella"},
        {"_LICENSE", "MIT"},
        {"_VERSION", "0.0.1"},
        {"_NAME", "hc"},
        {"_URL", "https://github.com/leiradel/hackable-console"},
        {"_DESCRIPTION", "Hackable Console bindings"},

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
        {"soExtension", "dll"}
#elif __linux__
        {"soExtension", "so"}
#else
        #error Unsupported platform
#endif
    };

    size_t const stringCount = sizeof(stringConsts) / sizeof(stringConsts[0]);

    lua_createtable(L, 0, stringCount + 6);

    _logger.push(L);
    lua_setfield(L, -2, "logger");

    _config.push(L);
    lua_setfield(L, -2, "config");

    _led.push(L);
    lua_setfield(L, -2, "led");

    _perf.push(L);
    lua_setfield(L, -2, "perf");

    _control.push(L);
    lua_setfield(L, -2, "control");

    _memory.push(L);
    lua_setfield(L, -2, "memory");

    for (size_t i = 0; i < stringCount; i++) {
        lua_pushstring(L, stringConsts[i].value);
        lua_setfield(L, -2, stringConsts[i].name);
    }

    return 1;
}

void hc::Application::sdlPrint(void* userdata, int category, SDL_LogPriority priority, char const* message) {
    auto const self = static_cast<Application*>(userdata);

    char const* categoryStr = "?";

    switch (category) {
        case SDL_LOG_CATEGORY_APPLICATION: categoryStr = "application"; break;
        case SDL_LOG_CATEGORY_ERROR: categoryStr = "error"; break;
        case SDL_LOG_CATEGORY_ASSERT: categoryStr = "assert"; break;
        case SDL_LOG_CATEGORY_SYSTEM: categoryStr = "system"; break;
        case SDL_LOG_CATEGORY_AUDIO: categoryStr = "audio"; break;
        case SDL_LOG_CATEGORY_VIDEO: categoryStr = "video"; break;
        case SDL_LOG_CATEGORY_RENDER: categoryStr = "render"; break;
        case SDL_LOG_CATEGORY_INPUT: categoryStr = "input"; break;
        case SDL_LOG_CATEGORY_TEST: categoryStr = "test"; break;
        case SDL_LOG_CATEGORY_CUSTOM: categoryStr = "custom"; break;
    }

    switch (priority) {
        case SDL_LOG_PRIORITY_VERBOSE:
        case SDL_LOG_PRIORITY_DEBUG: self->debug("[SDL] (%s): %s", categoryStr, message); break;
        case SDL_LOG_PRIORITY_INFO: self->info("[SDL] (%s): %s", categoryStr, message); break;
        case SDL_LOG_PRIORITY_WARN: self->warn("[SDL] (%s): %s", categoryStr, message); break;
        case SDL_LOG_PRIORITY_ERROR:
        case SDL_LOG_PRIORITY_CRITICAL: self->error("[SDL] (%s): %s", categoryStr, message); break;
        case SDL_NUM_LOG_PRIORITIES: self->error("[SDL] (%s): Invalid priority %d", categoryStr, priority); break;
    }
}

void hc::Application::lifeCycleVprintf(void* ud, char const* fmt, va_list args) {
    auto const self = static_cast<Application*>(ud);
    self->vprintf(RETRO_LOG_DEBUG, fmt, args);
}

void hc::Application::audioCallback(void* const udata, Uint8* const stream, int const len) {
    auto const self = static_cast<Application*>(udata);
    size_t const avail = self->_fifo.occupied();

    if (avail < (size_t)len) {
        self->_fifo.read(static_cast<void*>(stream), avail);
        memset(static_cast<void*>(stream + avail), 0, len - avail);
    }
    else {
        self->_fifo.read(static_cast<void*>(stream), len);
    }
}
