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

hc::Application::Application() : _fsm(*this, vprintf, this) {}

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

    _logger = new Logger;

    if (!_logger->init()) {
        delete _logger;
        return false;
    }

    _plugins.emplace(_logger);
    undo.add([this]() { delete _logger; });

    {
        // Setup SDL
        if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
            _logger->error(TAG "Error in SDL_Init: %s", SDL_GetError());
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
            _logger->error(TAG "Error in SDL_CreateWindow: %s", SDL_GetError());
            return false;
        }

        undo.add([this]() { SDL_DestroyWindow(_window); });

        _glContext = SDL_GL_CreateContext(_window);

        if (_glContext == nullptr) {
            _logger->error(TAG "Error in SDL_GL_CreateContext: %s", SDL_GetError());
            return false;
        }

        undo.add([this]() { SDL_GL_DeleteContext(_glContext); });

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
            _logger->error(TAG "Error in SDL_OpenAudioDevice: %s", SDL_GetError());
            return false;
        }

        undo.add([this]() { SDL_CloseAudioDevice(_audioDev); });

        if (!_fifo.init(_audioSpec.size * 2)) {
            _logger->error(TAG "Error in audio FIFO init");
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
            _logger->error(TAG "Error in SDL_GameControllerAddMappingsFromRW: %s", SDL_GetError());
            return false;
        }
    }

    {
        // Setup ImGui
        IMGUI_CHECKVERSION();

        if (ImGui::CreateContext() == nullptr) {
            _logger->error(TAG "Error creating ImGui context");
            return false;
        }

        undo.add([]() { ImGui::DestroyContext(); });

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        ImGui::StyleColorsDark();

        if (!ImGui_ImplSDL2_InitForOpenGL(_window, _glContext)) {
            _logger->error(TAG "Error initializing ImGui for OpenGL");
            return false;
        }

        undo.add([]() { ImGui_ImplSDL2_Shutdown(); });

        if (!ImGui_ImplOpenGL2_Init()) {
            _logger->error(TAG "Error initializing ImGui OpenGL implementation");
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
            _logger->error(TAG "Error adding Proggy Tiny font");
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
            _logger->error(TAG "Error adding Font Awesome 4 font");
            return false;
        }
    }

    {
        // Initialize components (logger has already been initialized)
        lrcpp::Frontend& frontend = lrcpp::Frontend::getInstance();

        _control = new Control;
        undo.add([this]() { delete _control; });
        _control->init(_logger, &_fsm);
        _plugins.emplace(_control);

        _config = new Config;
        undo.add([this]() { delete _config; });
        
        if (!_config->init(_logger)) {
            return false;
        }

        _plugins.emplace(_config);

        _video = new Video;
        undo.add([this]() { delete _video; });
        _video->init(_logger);
        _plugins.emplace(_video);

        _audio = new Audio;
        undo.add([this]() { delete _audio; });
        _audio->init(_logger, _audioSpec.freq, &_fifo);
        _plugins.emplace(_audio);

        _led = new Led;
        undo.add([this]() { delete _led; });
        _led->init(_logger);
        _plugins.emplace(_led);

        _input = new Input;
        undo.add([this]() { delete _input; });
        _input->init(_logger, &frontend);
        _plugins.emplace(_input);

        _perf = new Perf;
        undo.add([this]() { delete _perf; });
        _perf->init(_logger);
        _plugins.emplace(_perf);

        if (!_memory.init(_logger)) {
            return false;
        }

        undo.add([this]() { _memory.destroy(); });

        frontend.setLogger(_logger);
        frontend.setConfig(_config);
        frontend.setAudio(_audio);
        frontend.setVideo(_video);
        frontend.setLed(_led);
        frontend.setInput(_input);
        frontend.setPerf(_perf);
    }

    {
        // Initialize Lua
        _L = luaL_newstate();

        if (_L == nullptr) {
            return false;
        }

        undo.add([this]() { lua_close(_L); });

        luaL_openlibs(_L);
        luaopen_hc(_L);
        RegisterSearcher(_L);

        static auto const main = [](lua_State* const L) -> int {
            char const* const path = luaL_checkstring(L, 1);

            if (luaL_loadfilex(L, path, "t") != LUA_OK) {
                return lua_error(L);
            }

            lua_call(L, 0, 0);
            return 0;
        };

        std::string const& autorun = _config->getAutorunPath();

        lua_pushcfunction(_L, main);
        lua_pushlstring(_L, autorun.c_str(), autorun.length());
        
        _logger->info(TAG "Running \"%s\"", autorun.c_str());

        if (!ProtectedCall(_L, 1, 0, _logger)) {
            return false;
        }
    }

    undo.clear();
    onStarted();
    return true;
}

void hc::Application::destroy() {
    lua_close(_L);

    _memory.destroy();

    ImGui_ImplOpenGL2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_CloseAudioDevice(_audioDev);
    _fifo.destroy();

    SDL_GL_DeleteContext(_glContext);
    SDL_DestroyWindow(_window);
    SDL_Quit();
}

void hc::Application::draw() {
    ImGui::DockSpaceOverViewport();

    for (auto const plugin : _plugins) {
        plugin->onDraw();
    }

    _memory.draw();
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
                    _input->processEvent(&event);
                    break;
            }
        }

        if (_fsm.currentState() == LifeCycle::State::GameRunning) {
            frontend.run();
            onFrame();
        }

        ImGui_ImplOpenGL2_NewFrame();
        ImGui_ImplSDL2_NewFrame(_window);
        ImGui::NewFrame();

        draw();
        _audio->flush();

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

bool hc::Application::loadConsole(char const* name) {
    LuaRewinder rewinder(_L);

    _logger->info(TAG "Loading console \"%s\"", name);

    auto const found = _consoleRefs.find(name);

    if (found == _consoleRefs.end()) {
        _logger->error(TAG "Unknown console \"%s\"", name);
        return false;
    }

    lua_rawgeti(_L, LUA_REGISTRYINDEX, found->second);
    int const tableIndex = lua_gettop(_L);

    if (!ProtectedCallField(_L, tableIndex, "onLoadCore", 0, 1, _logger)) {
        _logger->warn(TAG "onLoadCore crashed, will continue loading!");
        lua_pushboolean(_L, 1);
    }

    if (!lua_toboolean(_L, -1)) {
        _logger->warn(TAG "onLoadCore prevented loading the console");
        return false;
    }

    if (GetField(_L, tableIndex, "path", _logger) != LUA_TSTRING) {
        return false;
    }

    char const* const path = lua_tostring(_L, -1);
    _logger->info(TAG "Loading core from \"%s\"", path);

    lrcpp::Frontend& frontend = lrcpp::Frontend::getInstance();

    if (!frontend.load(path)) {
        return false;
    }

    hc::PushFrontend(_L, &frontend);

    if (!ProtectedCallField(_L, tableIndex, "onCoreLoaded", 1, 1, _logger)) {
        _logger->warn(TAG "onCoreLoaded crashed, will continue loading!");
        lua_pushboolean(_L, 1);
    }

    if (!lua_toboolean(_L, -1)) {
        _logger->warn(TAG "onCoreLoaded prevented loading the console");

        if (frontend.unload()) {
            return false;
        }

        _logger->error(TAG "Couldn't unload the core, will continue loading!");
    }
    
    _currentConsole = name;

    retro_system_info info;

    if (frontend.getSystemInfo(&info)) {
        _control->setSystemInfo(&info);
    }

    _logger->info(TAG "System Info");
    _logger->info(TAG "    library_name     = %s", info.library_name);
    _logger->info(TAG "    library_version  = %s", info.library_version);
    _logger->info(TAG "    valid_extensions = %s", info.valid_extensions);
    _logger->info(TAG "    need_fullpath    = %s", info.need_fullpath ? "true" : "false");
    _logger->info(TAG "    block_extract    = %s", info.block_extract ? "true" : "false");

    onConsoleLoaded();
    return true;
}

bool hc::Application::loadGame(char const* path) {
    LuaRewinder rewinder(_L);

    _logger->info(TAG "Loading game from \"%s\"", path);

    auto const found = _consoleRefs.find(_currentConsole);

    if (found == _consoleRefs.end()) {
        _logger->error(TAG "Unknown console \"%s\"", _currentConsole.c_str());
        return false;
    }

    lrcpp::Frontend& frontend = lrcpp::Frontend::getInstance();
    retro_system_info info;

    if (!frontend.getSystemInfo(&info)) {
        return false;
    }

    lua_rawgeti(_L, LUA_REGISTRYINDEX, found->second);
    int const tableIndex = lua_gettop(_L);
    
    hc::PushFrontend(_L, &frontend);
    lua_pushstring(_L, path);

    if (!ProtectedCallField(_L, tableIndex, "onLoadGame", 2, 1, _logger)) {
        _logger->warn(TAG "onLoadGame crashed, will continue loading!");
        lua_pushboolean(_L, 1);
    }

    if (!lua_toboolean(_L, -1)) {
        _logger->warn(TAG "onLoadGame prevented loading the game");
        return false;
    }

    bool ok = false;

    if (info.need_fullpath) {
        ok = frontend.loadGame(path);
    }
    else {
        size_t size = 0;
        void const* data = readAll(_logger, path, &size);

        if (data == nullptr) {
            return false;
        }

        ok = frontend.loadGame(path, data, size);
        free(const_cast<void*>(data));
    }

    if (!ok) {
        return false;
    }

    hc::PushFrontend(_L, &frontend);

    if (!ProtectedCallField(_L, tableIndex, "onGameLoaded", 1, 1, _logger)) {
        _logger->warn(TAG "onGameLoaded crashed, will continue loading!");
        lua_pushboolean(_L, 1);
    }

    if (!lua_toboolean(_L, -1)) {
        _logger->warn(TAG "onGameLoaded prevented loading the game");

        if (frontend.unloadGame()) {
            return false;
        }

        _logger->error(TAG "Couldn't unload the game, will continue loading!");
    }

    static struct {char const* const name; unsigned const id;} memory[] = {
        {"save", RETRO_MEMORY_SAVE_RAM},
        {"rtc", RETRO_MEMORY_RTC},
        {"sram", RETRO_MEMORY_SYSTEM_RAM},
        {"vram", RETRO_MEMORY_VIDEO_RAM}
    };

    _logger->info(TAG "Core memory");
    bool any = false;

    for (size_t i = 0; i < sizeof(memory) / sizeof(memory[0]); i++) {
        void* data = nullptr;
        size_t size = 0;

        if (frontend.getMemoryData(memory[i].id, &data) && frontend.getMemorySize(memory[i].id, &size) && size != 0) {
            _logger->info(TAG "    %-4s %p %zu bytes", memory[i].name, data, size);
            any = true;
        }
    }

    if (!any) {
        _logger->info(TAG "    No core memory exposed via the get_memory interface");
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

bool hc::Application::step() {
    onFrame();
    return lrcpp::Frontend::getInstance().run();
}

bool hc::Application::unloadConsole() {
    if (lrcpp::Frontend::getInstance().unload()) {
        onConsoleUnloaded();
        return true;
    }

    return false;
}

bool hc::Application::unloadGame() {
    if (lrcpp::Frontend::getInstance().unloadGame()) {
        _memory.reset();
        onGameUnloaded();
        return true;
    }

    return false;
}

void hc::Application::onStarted() {
    for (auto const plugin : _plugins) {
        _logger->debug(TAG "onStarted plugin %s (%s): %s", plugin->getName(), plugin->getVersion(), plugin->getCopyright());
        plugin->onStarted();
    }
}

void hc::Application::onConsoleLoaded() {
    for (auto const plugin : _plugins) {
        _logger->debug(TAG "onConsoleLoaded plugin %s (%s): %s", plugin->getName(), plugin->getVersion(), plugin->getCopyright());
        plugin->onConsoleLoaded();
    }
}

void hc::Application::onGameLoaded() {
    for (auto const plugin : _plugins) {
        _logger->debug(TAG "onGameLoaded plugin %s (%s): %s", plugin->getName(), plugin->getVersion(), plugin->getCopyright());
        plugin->onGameLoaded();
    }
}

void hc::Application::onGamePaused() {
    for (auto const plugin : _plugins) {
        _logger->debug(TAG "onGamePaused plugin %s (%s): %s", plugin->getName(), plugin->getVersion(), plugin->getCopyright());
        plugin->onGamePaused();
    }
}

void hc::Application::onGameResumed() {
    for (auto const plugin : _plugins) {
        _logger->debug(TAG "onGameResumed plugin %s (%s): %s", plugin->getName(), plugin->getVersion(), plugin->getCopyright());
        plugin->onGameResumed();
    }
}

void hc::Application::onGameReset() {
    for (auto const plugin : _plugins) {
        _logger->debug(TAG "onGameReset plugin %s (%s): %s", plugin->getName(), plugin->getVersion(), plugin->getCopyright());
        plugin->onGameReset();
    }
}

void hc::Application::onFrame() {
    for (auto const plugin : _plugins) {
        // Don't log stuff per frame
        plugin->onFrame();
    }
}

void hc::Application::onGameUnloaded() {
    for (auto const plugin : _plugins) {
        _logger->debug(TAG "onGameUnloaded plugin %s (%s): %s", plugin->getName(), plugin->getVersion(), plugin->getCopyright());
        plugin->onGameUnloaded();
    }
}

void hc::Application::onConsoleUnloaded() {
    for (auto const plugin : _plugins) {
        _logger->debug(TAG "onConsoleUnloaded plugin %s (%s): %s", plugin->getName(), plugin->getVersion(), plugin->getCopyright());
        plugin->onConsoleUnloaded();
    }
}

void hc::Application::onQuit() {
    for (auto const plugin : _plugins) {
        _logger->debug(TAG "onQuit plugin %s (%s): %s", plugin->getName(), plugin->getVersion(), plugin->getCopyright());
        plugin->onQuit();
        delete plugin;
    }

    _plugins.clear();
}

void hc::Application::vprintf(void* ud, char const* fmt, va_list args) {
    auto const self = static_cast<Application*>(ud);
    self->_logger->debug(fmt, args);
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

int hc::Application::luaopen_hc(lua_State* const L) {
    static luaL_Reg const functions[] = {
        {"addConsole", l_addConsole},
        {"addMemoryRegion", l_addMemoryRegion},
        {nullptr, nullptr}
    };

    static struct {char const* const name; char const* const value;} const stringConsts[] = {
        {"_COPYRIGHT", "Copyright (c) 2020 Andre Leiradella"},
        {"_LICENSE", "MIT"},
        {"_VERSION", "1.0.0"},
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

    size_t const functionsCount = sizeof(functions) / sizeof(functions[0]) - 1;
    size_t const stringCount = sizeof(stringConsts) / sizeof(stringConsts[0]);

    lua_createtable(L, 0, functionsCount + stringCount + 2);

    lua_pushlightuserdata(L, this);
    luaL_setfuncs(L, functions, 1);

    for (size_t i = 0; i < stringCount; i++) {
        lua_pushstring(L, stringConsts[i].value);
        lua_setfield(L, -2, stringConsts[i].name);
    }

    _logger->push(L);
    lua_setfield(L, -2, "logger");

    _config->push(L);
    lua_setfield(L, -2, "config");

    return 1;
}

int hc::Application::l_addConsole(lua_State* const L) {
    auto const self = static_cast<Application*>(lua_touserdata(L, lua_upvalueindex(1)));
    luaL_argexpected(L, lua_type(L, 1) == LUA_TTABLE, 1, "table");

    lua_getfield(L, 1, "name");

    if (!lua_isstring(L, -1)) {
        return luaL_error(L, "table for registerConsole must have a field \"name\"");
    }

    char const* const name = lua_tostring(L, -1);

    lua_pushvalue(L, 1);
    int const ref = luaL_ref(L, LUA_REGISTRYINDEX);

    self->_control->addConsole(name);
    self->_consoleRefs.emplace(name, ref);
    return 0;
}

int hc::Application::l_addMemoryRegion(lua_State* const L) {
    auto const self = static_cast<Application*>(lua_touserdata(L, lua_upvalueindex(1)));
    char const* const name = luaL_checkstring(L, 1);
    void* const data = lua_touserdata(L, 2);
    lua_Integer const base = luaL_checkinteger(L, 3);
    lua_Integer const size = luaL_checkinteger(L, 4);
    int const readOnly = lua_toboolean(L, 5);

    if (data == nullptr || size <= 0) {
        return luaL_error(L, "invalid data or size: %p, %d", data, size);
    }

    self->_memory.addRegion(name, data, base, size, readOnly != 0);
    return 0;
}
