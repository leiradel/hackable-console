#include "Control.h"

#include "LuaUtil.h"

#include <imguial_button.h>
#include <imguifilesystem.h>
#include <IconsFontAwesome4.h>

extern "C" {
    #include "lauxlib.h"
}

#define TAG "[CTR] "

hc::Control::Control() : _logger(nullptr), _selected(0) {}

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

void hc::Control::onGameLoaded() {}

void hc::Control::onGamePaused() {}

void hc::Control::onGameResumed() {}

void hc::Control::onGameReset() {}

void hc::Control::onFrame() {}

void hc::Control::onDraw() {
    static auto const getter = [](void* const data, int idx, char const** const text) -> bool {
        auto const consoles = (std::map<std::string, int>*)data;

        if (idx < static_cast<int>(consoles->size())) {
            for (auto const& pair : *consoles) {
                if (idx == 0) {
                    *text = pair.first.c_str();
                    return true;
                }

                idx--;
            }
        }

        return false;
    };

    if (ImGui::Begin(ICON_FA_COG " Control")) {
        int const count = static_cast<int>(_consoles.size());
        ImVec2 const size = ImVec2(120.0f, 0.0f);

        ImGui::Combo("##Consoles", &_selected, getter, &_consoles, count);
        ImGui::SameLine();

        if (ImGuiAl::Button(ICON_FA_FOLDER_OPEN " Load Console", _fsm->currentState() == LifeCycle::State::Start && _selected < count, size)) {
            char const* name = nullptr;

            if (getter(&_consoles, _selected, &name)) {
                auto const found = _consoles.find(name);
                Callback const& cb = found->second;

                lua_rawgeti(cb.L, LUA_REGISTRYINDEX, cb.ref);
                protectedCall(cb.L, 0, 0, _logger);
            }
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
            _fsm->unloadConsole();
        }
    }

    ImGui::End();
}

void hc::Control::onGameUnloaded() {}

void hc::Control::onConsoleUnloaded() {
    _extensions.clear();
}

void hc::Control::onQuit() {}

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
    luaL_argexpected(L, lua_type(L, 3) == LUA_TFUNCTION, 3, "function");

    lua_pushvalue(L, 3);
    int const ref = luaL_ref(L, LUA_REGISTRYINDEX);

    Callback cb = {L, ref};
    self->_consoles.emplace(std::string(name, length), cb);
    return 0;
}

int hc::Control::l_loadConsole(lua_State* const L) {
    auto const self = check(L, 1);
    char const* const name = luaL_checkstring(L, 2);

    if (!self->_fsm->loadConsole(name)) {
        return luaL_error(L, "could not load console \"%s\"", name);
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

int hc::Control::l_unloadConsole(lua_State* const L) {
    auto const self = check(L, 1);

    if (!self->_fsm->unloadConsole()) {
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
