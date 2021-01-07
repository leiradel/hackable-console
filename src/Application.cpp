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

#include <MaterialDesign.inl>
#include <IconsMaterialDesign.h>

#include <stdlib.h>
#include <sys/stat.h>

extern "C" {
    #include "lauxlib.h"
    #include "lualib.h"
}

static void const* readAll(hc::Logger* logger, char const* const path, size_t* const size) {
    struct stat statbuf;

    if (stat(path, &statbuf) != 0) {
        logger->error("Error getting content info: %s", strerror(errno));
        return nullptr;
    }

    void* const data = malloc(statbuf.st_size);

    if (data == nullptr) {
        logger->error("Out of memory allocating %zu bytes", statbuf.st_size);
        return nullptr;
    }

    FILE* file = fopen(path, "rb");

    if (file == nullptr) {
        logger->error("Error opening content: %s", strerror(errno));
        free(data);
        return nullptr;
    }

    size_t numread = fread(data, 1, statbuf.st_size, file);

    if (numread != (size_t)statbuf.st_size) {
        logger->error("Error reading content: %s", strerror(errno));
        fclose(file);
        free(data);
        return nullptr;
    }

    fclose(file);

    logger->info("Loaded content from \"%s\", %zu bytes", path, numread);
    *size = numread;
    return data;
}

hc::Application::Application() : _fsm(*this) {}

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

    if (!_logger.init()) {
        return false;
    }

    undo.add([this]() { _logger.destroy(); });

    {
        // Setup SDL
        if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
            _logger.error("Error in SDL_Init: %s", SDL_GetError());
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
            _logger.error("Error in SDL_CreateWindow: %s", SDL_GetError());
            return false;
        }

        undo.add([this]() { SDL_DestroyWindow(_window); });

        _glContext = SDL_GL_CreateContext(_window);

        if (_glContext == nullptr) {
            _logger.error("Error in SDL_GL_CreateContext: %s", SDL_GetError());
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
            _logger.error("Error in SDL_OpenAudioDevice: %s", SDL_GetError());
            return false;
        }

        undo.add([this]() { SDL_CloseAudioDevice(_audioDev); });

        if (!_fifo.init(_audioSpec.size * 2)) {
            _logger.error("Error in audio FIFO init");
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
            _logger.error("Error in SDL_GameControllerAddMappingsFromRW: %s", SDL_GetError());
            return false;
        }
    }

    {
        // Setup ImGui
        IMGUI_CHECKVERSION();

        if (ImGui::CreateContext() == nullptr) {
            _logger.error("Error creating ImGui context");
            return false;
        }

        undo.add([]() { ImGui::DestroyContext(); });

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        ImGui::StyleColorsDark();

        if (!ImGui_ImplSDL2_InitForOpenGL(_window, _glContext)) {
            _logger.error("Error initializing ImGui for OpenGL");
            return false;
        }

        undo.add([]() { ImGui_ImplSDL2_Shutdown(); });

        if (!ImGui_ImplOpenGL2_Init()) {
            _logger.error("Error initializing ImGui OpenGL implementation");
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
            _logger.error("Error adding Proggy Tiny font");
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
            _logger.error("Error adding Font Awesome 4 font");
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
            _logger.error("Error adding Material Design font");
            return false;
        }
    }

    {
        // Initialize components (logger has already been initialized)
        if (!_control.init(&_fsm, &_logger)) {
            return false;
        }

        if (!_config.init(&_logger)) {
            return false;
        }

        undo.add([this]() { _config.destroy(); });

        if (!_video.init(&_logger)) {
            return false;
        }

        undo.add([this]() { _video.destroy(); });

        if (!_audio.init(&_logger, _audioSpec.freq, &_fifo)) {
            return false;
        }

        undo.add([this]() { _audio.destroy(); });

        _frontend.setLogger(&_logger);
        _frontend.setConfig(&_config);
        _frontend.setAudio(&_audio);
        _frontend.setVideo(&_video);
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

        std::string const& autorun = _config.getAutorunPath();

        lua_pushcfunction(_L, main);
        lua_pushlstring(_L, autorun.c_str(), autorun.length());
        
        _logger.info("Running \"%s\"", autorun.c_str());

        if (!ProtectedCall(_L, 1, 0, &_logger)) {
            return false;
        }
    }

    undo.clear();
    return true;
}

void hc::Application::destroy() {
    lua_close(_L);

    _video.destroy();
    _audio.destroy();
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

void hc::Application::draw() {
    ImGui::DockSpaceOverViewport();

    _logger.draw();
    _control.draw();
    _config.draw();
    _audio.draw();
    _video.draw();
}

void hc::Application::run() {
    bool done = false;

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
                    // _input.processEvent(&event);
                    break;
            }
        }

        if (_fsm.currentState() == LifeCycle::State::GameRunning) {
            _frontend.run();
        }

        ImGui_ImplOpenGL2_NewFrame();
        ImGui_ImplSDL2_NewFrame(_window);
        ImGui::NewFrame();

        draw();
        _audio.flush();

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
    _logger.info("Loading console \"%s\"", name);

    auto const found = _consoleRefs.find(name);

    if (found == _consoleRefs.end()) {
        _logger.error("Unknown console \"%s\"", name);
        return false;
    }

    lua_rawgeti(_L, LUA_REGISTRYINDEX, found->second);

    if (GetField(_L, -1, "onLoadCore", &_logger) != LUA_TFUNCTION) {
        lua_pop(_L, 1);
        lua_pushboolean(_L, 1);
    }
    else if (!ProtectedCall(_L, 0, 1, &_logger)) {
        lua_pop(_L, 2);
        return false;
    }

    if (!lua_toboolean(_L, -1)) {
        _logger.warn("onLoadCore prevented loading the console");
        lua_pop(_L, 2);
        return false;
    }

    lua_pop(_L, 1);
    // TODO call onCoreLoaded with the loaded core (or the frontend, more likely)
    
    if (GetField(_L, -1, "path", &_logger) != LUA_TSTRING) {
        lua_pop(_L, 2);
        return false;
    }

    char const* const path = lua_tostring(_L, -1);
    _logger.info("Loading core from \"%s\"", path);

    _currentConsole = name;
    bool const ok = _frontend.load(path);
    lua_pop(_L, 2);
    return ok;
}

bool hc::Application::loadGame(char const* path) {
    _logger.info("Loading game from \"%s\"", path);

    auto const found = _consoleRefs.find(_currentConsole);

    if (found == _consoleRefs.end()) {
        _logger.error("Unknown console \"%s\"", _currentConsole.c_str());
        return false;
    }

    retro_system_info info;

    if (!_frontend.getSystemInfo(&info)) {
        return false;
    }

    lua_rawgeti(_L, LUA_REGISTRYINDEX, found->second);
    
    if (GetField(_L, -1, "onLoadGame", &_logger) != LUA_TFUNCTION) {
        lua_pop(_L, 1);
        lua_pushboolean(_L, 1);
    }
    else {
        lua_pushnil(_L); // TODO push frontend
        lua_pushstring(_L, path);

        if (!ProtectedCall(_L, 2, 1, &_logger)) {
            lua_pop(_L, 2);
            return false;
        }
    }

    if (!lua_toboolean(_L, -1)) {
        _logger.warn("onLoadGame prevented loading the console");
        lua_pop(_L, 2);
        return false;
    }

    lua_pop(_L, 2);

    if (info.need_fullpath) {
        return _frontend.loadGame(path);
    }

    size_t size = 0;
    void const* data = readAll(&_logger, path, &size);

    if (data == nullptr) {
        return false;
    }

    return _frontend.loadGame(path, data, size);
}

bool hc::Application::pauseGame() {
    return true;
}

bool hc::Application::quit() {
    return true;
}

bool hc::Application::resetGame() {
    return _frontend.reset();
}

bool hc::Application::resumeGame() {
    return true;
}

bool hc::Application::step() {
    return _frontend.run();
}

bool hc::Application::unloadConsole() {
    if (_frontend.unload()) {
        _config.reset();
        return true;
    }

    return false;
}

bool hc::Application::unloadGame() {
    if (_frontend.unloadGame()) {
        _audio.reset();
        _video.reset();
        return true;
    }

    return false;
}

void hc::Application::printf(char const* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    _logger.vprintf(RETRO_LOG_DEBUG, fmt, args);
    va_end(args);
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

    _logger.push(L);
    lua_setfield(L, -2, "logger");

    _config.push(L);
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

    self->_control.addConsole(name);
    self->_consoleRefs.emplace(name, ref);
    return 0;
}
