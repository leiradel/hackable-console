#include "Control.h"

#include "LuaUtil.h"

#include <imguial_button.h>
#include <imguifilesystem.h>
#include <IconsFontAwesome4.h>

#include <algorithm>

extern "C" {
    #include "lauxlib.h"
}

#define TAG "[CTR] "

static int str2id(char const* const str) {
    if (strcmp(str, "save") == 0) {
        return RETRO_MEMORY_SAVE_RAM;
    }
    else if (strcmp(str, "rtc") == 0) {
        return RETRO_MEMORY_RTC;
    }
    else if (strcmp(str, "sram") == 0) {
        return RETRO_MEMORY_SYSTEM_RAM;
    }
    else if (strcmp(str, "vram") == 0) {
        return RETRO_MEMORY_VIDEO_RAM;
    }
    else {
        char* end;
        long const id = strtol(str, &end, 10);

        if (*str == 0 || *end != 0) {
            return -1;
        }

        return static_cast<int>(id);
    }
}

hc::Control::Control() : _logger(nullptr), _selected(0), _opened(-1) {}

void hc::Control::init(Logger* const logger, LifeCycle* const fsm) {
    _logger = logger;
    _fsm = fsm;
}

char const* hc::Control::getName() {
    return "hc::Control built-in control plugin";
}

char const* hc::Control::getVersion() {
    return "0.0.0";
}

char const* hc::Control::getLicense() {
    return "MIT";
}

char const* hc::Control::getCopyright() {
    return "Copyright (c) Andre Leiradella";
}

char const* hc::Control::getUrl() {
    return "https://github.com/leiradel/hackable-console";
}

void hc::Control::onStarted() {}

void hc::Control::onConsoleLoaded() {}

void hc::Control::onGameLoaded() {
    auto const& cb = _consoles[_opened];
    lua_rawgeti(cb.L, LUA_REGISTRYINDEX, cb.ref);
    protectedCallField(cb.L, -1, "onGameLoaded", 0, 0, _logger);
    lua_pop(cb.L, 1);
}

void hc::Control::onGamePaused() {}

void hc::Control::onGameResumed() {}

void hc::Control::onGameReset() {}

void hc::Control::onFrame() {}

void hc::Control::onDraw(bool* opened) {
    static auto const getter = [](void* const data, int idx, char const** const text) -> bool {
        auto const consoles = (std::vector<Console>*)data;
        *text = (*consoles)[idx].name.c_str();
        return true;
    };

    if (!*opened) {
        return;
    }

    if (ImGui::Begin(ICON_FA_COG " Control", opened)) {
        int const count = static_cast<int>(_consoles.size());
        ImVec2 const size = ImVec2(120.0f, 0.0f);

        ImGui::Combo("##Consoles", &_selected, getter, &_consoles, count);
        ImGui::SameLine();

        if (ImGuiAl::Button(ICON_FA_FOLDER_OPEN " Load Console", _fsm->currentState() == LifeCycle::State::Start && _selected < count, size)) {
            _opened = _selected;
            Console const& cb = _consoles[_selected];

            lua_rawgeti(cb.L, LUA_REGISTRYINDEX, cb.ref);
            protectedCallField(cb.L, -1, "onConsoleLoaded", 0, 0, _logger);
            lua_pop(cb.L, 1);
        }

        bool loadGamePressed = false;

        if (ImGuiAl::Button(ICON_FA_ROCKET " Load Game", _fsm->currentState() == LifeCycle::State::ConsoleLoaded, size)) {
            loadGamePressed = true;
        }

        static ImGuiFs::Dialog gameDialog;

        ImVec2 const gameDialogSize = ImVec2(
            ImGui::GetIO().DisplaySize.x / 2.0f,
            ImGui::GetIO().DisplaySize.y / 2.0f
        );

        ImVec2 const gameDialogPos = ImVec2(
            (ImGui::GetIO().DisplaySize.x - gameDialogSize.x) / 2.0f,
            (ImGui::GetIO().DisplaySize.y - gameDialogSize.y) / 2.0f
        );

        char const* const path = gameDialog.chooseFileDialog(
            loadGamePressed,
            _lastGameFolder.c_str(),
            _extensions.c_str(),
            ICON_FA_ROCKET" Load Game",
            gameDialogSize,
            gameDialogPos
        );

        if (path != nullptr && path[0] != 0) {
            if (_fsm->loadGame(path)) {
                char temp[ImGuiFs::MAX_PATH_BYTES];
                ImGuiFs::PathGetDirectoryName(path, temp);
                _lastGameFolder = temp;
            }
        }

        ImGui::SameLine();

        if (_fsm->currentState() == LifeCycle::State::GameRunning) {
            if (ImGuiAl::Button(ICON_FA_PAUSE " Pause", true, size)) {
                _fsm->pauseGame();
            }
        }
        else {
            if (ImGuiAl::Button(ICON_FA_PLAY " Run", _fsm->currentState() == LifeCycle::State::GamePaused, size)) {
                _fsm->resumeGame();
            }
        }

        ImGui::SameLine();

        if (ImGuiAl::Button(ICON_FA_STEP_FORWARD " Frame Step", _fsm->currentState() == LifeCycle::State::GamePaused, size)) {
            _fsm->step();
        }

        bool const gameLoaded = _fsm->currentState() == LifeCycle::State::GameRunning ||
                                _fsm->currentState() == LifeCycle::State::GamePaused;

        if (ImGuiAl::Button(ICON_FA_REFRESH " Reset Game", gameLoaded, size)) {
            _fsm->resetGame();
        }

        ImGui::SameLine();

        if (ImGuiAl::Button(ICON_FA_EJECT " Unload Game", gameLoaded, size)) {
            _fsm->unloadGame();
        }

        ImGui::SameLine();

        if (ImGuiAl::Button(ICON_FA_POWER_OFF " Unload Console", _fsm->currentState() == LifeCycle::State::ConsoleLoaded, size)) {
            _fsm->unloadCore();
        }
    }

    ImGui::End();
}

void hc::Control::onGameUnloaded() {}

void hc::Control::onConsoleUnloaded() {
    _extensions.clear();
    _opened = -1;
}

void hc::Control::onQuit() {
    for (auto const& cb : _consoles) {
        luaL_unref(cb.L, LUA_REGISTRYINDEX, cb.ref);
    }
}

void hc::Control::setSystemInfo(retro_system_info const* info) {
    char const* ext = info->valid_extensions;

    if (ext != nullptr) {
        for (;;) {
            char const* const begin = ext;

            while (*ext != 0 && *ext != '|') {
                ext++;
            }

            _extensions.append(".");
            _extensions.append(begin, ext - begin);

            if (*ext == 0) {
                break;
            }

            _extensions.append( ";" );
            ext++;
        }
    }
}

int hc::Control::push(lua_State* const L) {
    auto const self = static_cast<Control**>(lua_newuserdata(L, sizeof(Control*)));
    *self = this;

    if (luaL_newmetatable(L, "hc::Control")) {
        static luaL_Reg const methods[] = {
            {"addConsole", l_addConsole},
            {"loadCore", l_loadCore},
            {"quit", l_quit},
            {"unloadCore", l_unloadCore},
            {"loadGame", l_loadGame},
            {"resumeGame", l_resumeGame},
            {"resetGame", l_resetGame},
            {"step", l_step},
            {"unloadGame", l_unloadGame},
            {"pauseGame", l_pauseGame},
            {"apiVersion", l_apiVersion},
            {"getSystemInfo", l_getSystemInfo},
            {"getSystemAvInfo", l_getSystemAvInfo},
            {"serializeSize", l_serializeSize},
            {"serialize", l_serialize},
            {"unserialize", l_unserialize},
            {"cheatReset", l_cheatReset},
            {"cheatSet", l_cheatSet},
            {"getRegion", l_getRegion},
            {"getMemoryData", l_getMemoryData},
            {"getMemorySize", l_getMemorySize},
            {nullptr, nullptr}
        };

        luaL_newlib(L, methods);
        lua_setfield(L, -2, "__index");
    }

    lua_setmetatable(L, -2);
    return 1;
}

hc::Control* hc::Control::check(lua_State* const L, int const index) {
    return *static_cast<Control**>(luaL_checkudata(L, index, "hc::Control"));
}

int hc::Control::l_addConsole(lua_State* const L) {
    auto const self = check(L, 1);
    size_t length = 0;
    char const* const name = lua_tolstring(L, 2, &length);
    luaL_argexpected(L, lua_type(L, 3) == LUA_TTABLE, 3, "table");

    lua_pushvalue(L, 3);
    int const ref = luaL_ref(L, LUA_REGISTRYINDEX);

    Console cb = {std::string(name, length), L, ref};
    self->_consoles.emplace_back(cb);

    std::sort(self->_consoles.begin(), self->_consoles.end(), [](Console const& a, Console const& b) -> bool {
        return a.name < b.name;
    });

    return 0;
}

int hc::Control::l_loadCore(lua_State* const L) {
    auto const self = check(L, 1);
    char const* const path = luaL_checkstring(L, 2);

    if (!self->_fsm->loadCore(path)) {
        return luaL_error(L, "could not load core \"%s\"", path);
    }

    return 0;
}

int hc::Control::l_quit(lua_State* const L) {
    auto const self = check(L, 1);

    if (!self->_fsm->quit()) {
        return luaL_error(L, "could not quit");
    }

    return 0;
}

int hc::Control::l_unloadCore(lua_State* const L) {
    auto const self = check(L, 1);

    if (!self->_fsm->unloadCore()) {
        return luaL_error(L, "could not unload console");
    }

    return 0;
}

int hc::Control::l_loadGame(lua_State* const L) {
    auto const self = check(L, 1);
    char const* const name = luaL_checkstring(L, 2);

    if (!self->_fsm->loadGame(name)) {
        return luaL_error(L, "could not load game \"%s\"", name);
    }

    return 0;
}

int hc::Control::l_resumeGame(lua_State* const L) {
    auto const self = check(L, 1);

    if (!self->_fsm->resumeGame()) {
        return luaL_error(L, "could not resume game");
    }

    return 0;
}

int hc::Control::l_resetGame(lua_State* const L) {
    auto const self = check(L, 1);

    if (!self->_fsm->resetGame()) {
        return luaL_error(L, "could not reset game");
    }

    return 0;
}

int hc::Control::l_step(lua_State* const L) {
    auto const self = check(L, 1);

    if (!self->_fsm->step()) {
        return luaL_error(L, "could not step one game frame");
    }

    return 0;
}

int hc::Control::l_unloadGame(lua_State* const L) {
    auto const self = check(L, 1);

    if (!self->_fsm->unloadGame()) {
        return luaL_error(L, "could not unload game");
    }

    return 0;
}

int hc::Control::l_pauseGame(lua_State* const L) {
    auto const self = check(L, 1);

    if (!self->_fsm->pauseGame()) {
        return luaL_error(L, "could not pause game");
    }

    return 0;
}

int hc::Control::l_apiVersion(lua_State* const L) {
    check(L, 1);

    auto& frontend = lrcpp::Frontend::getInstance();
    unsigned version;

    if (!frontend.apiVersion(&version)) {
        return luaL_error(L, "error getting the API version");
    }

    lua_pushinteger(L, version);
    return 1;
}

int hc::Control::l_getSystemInfo(lua_State* const L) {
    check(L, 1);

    auto& frontend = lrcpp::Frontend::getInstance();
    retro_system_info info;

    if (!frontend.getSystemInfo(&info)) {
        return luaL_error(L, "error getting the system info");
    }

    lua_createtable(L, 0, 5);

    lua_pushstring(L, info.library_name);
    lua_setfield(L, -2, "libraryName");

    lua_pushstring(L, info.library_version);
    lua_setfield(L, -2, "libraryVersion");

    lua_pushstring(L, info.valid_extensions);
    lua_setfield(L, -2, "validExtensions");

    lua_pushboolean(L, info.need_fullpath);
    lua_setfield(L, -2, "needFullPath");

    lua_pushboolean(L, info.block_extract);
    lua_setfield(L, -2, "blockExtract");

    return 1;
}

int hc::Control::l_getSystemAvInfo(lua_State* const L) {
    check(L, 1);

    auto& frontend = lrcpp::Frontend::getInstance();
    retro_system_av_info info;

    if (!frontend.getSystemAvInfo(&info)) {
        return luaL_error(L, "error getting the system a/v info");
    }

    lua_createtable(L, 0, 2);

    lua_createtable(L, 0, 5);

    lua_pushinteger(L, info.geometry.base_width);
    lua_setfield(L, -2, "baseWidth");

    lua_pushinteger(L, info.geometry.base_height);
    lua_setfield(L, -2, "baseHeight");

    lua_pushinteger(L, info.geometry.max_width);
    lua_setfield(L, -2, "maxWidth");

    lua_pushinteger(L, info.geometry.max_height);
    lua_setfield(L, -2, "maxHeight");

    lua_pushnumber(L, info.geometry.aspect_ratio);
    lua_setfield(L, -2, "aspectRatio");

    lua_setfield(L, -2, "geometry");

    lua_createtable(L, 0, 2);

    lua_pushnumber(L, info.timing.fps);
    lua_setfield(L, -2, "fps");

    lua_pushnumber(L, info.timing.sample_rate);
    lua_setfield(L, -2, "sampleRate");

    lua_setfield(L, -2, "timing");

    return 1;
}

int hc::Control::l_serializeSize(lua_State* const L) {
    check(L, 1);

    auto& frontend = lrcpp::Frontend::getInstance();
    size_t size;

    if (!frontend.serializeSize(&size)) {
        return luaL_error(L, "error getting the size need to serialize the state");
    }

    lua_pushinteger(L, size);
    return 1;
}

int hc::Control::l_serialize(lua_State* const L) {
    check(L, 1);

    auto& frontend = lrcpp::Frontend::getInstance();
    size_t size;

    if (!frontend.serializeSize(&size)) {
        return luaL_error(L, "error serializing state");
    }

    void* const data = malloc(size);

    if (data == nullptr) {
        return luaL_error(L, "out of memory");
    }

    if (!frontend.serialize(data, size)) {
        free(data);
        return luaL_error(L, "error serializing state");
    }

    lua_pushlstring(L, static_cast<char*>(data), size);
    free(data);
    return 1;
}

int hc::Control::l_unserialize(lua_State* const L) {
    check(L, 1);

    size_t size;
    char const* const data = luaL_checklstring(L, 2, &size);

    auto& frontend = lrcpp::Frontend::getInstance();

    if (!frontend.unserialize(data, size)) {
        return luaL_error(L, "error unserializing state");
    }

    return 0;
}

int hc::Control::l_cheatReset(lua_State* const L) {
    check(L, 1);

    auto& frontend = lrcpp::Frontend::getInstance();

    if (!frontend.cheatReset()) {
        return luaL_error(L, "error resetting cheats");
    }

    return 0;
}

int hc::Control::l_cheatSet(lua_State* const L) {
    check(L, 1);
    unsigned const index = luaL_checkinteger(L, 2);
    bool const enabled = lua_toboolean(L, 3);
    char const* const code = luaL_checkstring(L, 4);

    auto& frontend = lrcpp::Frontend::getInstance();

    if (!frontend.cheatSet(index, enabled, code)) {
        return luaL_error(L, "error setting cheat (%u, %s, \"%s\")", index, enabled ? "true" : "false", code);
    }

    return 0;
}

int hc::Control::l_getRegion(lua_State* const L) {
    check(L, 1);

    auto& frontend = lrcpp::Frontend::getInstance();
    unsigned region;

    if (!frontend.getRegion(&region)) {
        return luaL_error(L, "error getting the region");
    }

    switch (region) {
        case RETRO_REGION_NTSC: lua_pushliteral(L, "ntsc"); break;
        case RETRO_REGION_PAL:  lua_pushliteral(L, "pal"); break;
        default: return luaL_error(L, "invalid region: %u", region);
    }

    return 1;
}

int hc::Control::l_getMemoryData(lua_State* const L) {
    check(L, 1);
    char const* const idStr = luaL_checkstring(L, 2);

    int const id = str2id(idStr);

    if (id < 0) {
        return luaL_error(L, "invalid memory id: \"%s\"", idStr);
    }

    auto& frontend = lrcpp::Frontend::getInstance();
    void* data;

    if (!frontend.getMemoryData(static_cast<unsigned>(id), &data)) {
        return luaL_error(L, "error getting memory data for \"%s\"", idStr);
    }

    lua_pushlightuserdata(L, data);
    return 1;
}

int hc::Control::l_getMemorySize(lua_State* const L) {
    check(L, 1);
    char const* const idStr = luaL_checkstring(L, 2);

    int const id = str2id(idStr);

    if (id < 0) {
        return luaL_error(L, "invalid memory id: \"%s\"", idStr);
    }

    auto& frontend = lrcpp::Frontend::getInstance();
    size_t size;

    if (!frontend.getMemorySize(static_cast<unsigned>(id), &size)) {
        return luaL_error(L, "error getting memory size for \"%s\"", idStr);
    }

    lua_pushinteger(L, size);
    return 1;
}
