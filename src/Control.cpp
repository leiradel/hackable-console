#include "Control.h"

#include <imguial_button.h>
#include <imguifilesystem.h>
#include <IconsFontAwesome4.h>

bool hc::Control::init(LifeCycle* fsm, Logger* logger) {
    _fsm = fsm;
    _logger = logger;

    return true;
}

void hc::Control::destroy() {}

void hc::Control::reset() {
    _selected = 0;
    _extensions.clear();
}

void hc::Control::draw() {
    static auto const getter = [](void* const data, int idx, char const** const text) -> bool {
        auto const consoles = (std::set<std::string>*)data;

        if (idx < (int)consoles->size()) {
            for (auto const& name : *consoles) {
                if (idx == 0) {
                    *text = name.c_str();
                    return true;
                }

                idx--;
            }
        }

        return false;
    };

    if (ImGui::Begin(ICON_FA_COGS " Control")) {
        int const count = static_cast<int>(_consoles.size());
        ImVec2 const size = ImVec2(120.0f, 0.0f);

        ImGui::Combo("Consoles", &_selected, getter, &_consoles, count);

        if (ImGuiAl::Button(ICON_FA_COG " Load Console", _fsm->currentState() == LifeCycle::State::Start && _selected < count, size)) {
            char const* name = nullptr;

            if (getter(&_consoles, _selected, &name)) {
                _fsm->loadConsole(name);
            }
        }

        ImGui::SameLine();
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
            ICON_FA_FOLDER_OPEN" Open Game",
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

        ImGui::SameLine();

        bool const gameLoaded = _fsm->currentState() == LifeCycle::State::GameRunning ||
                                _fsm->currentState() == LifeCycle::State::GamePaused;

        if (ImGuiAl::Button(ICON_FA_REFRESH " Reset Game", gameLoaded, size)) {
            _fsm->resetGame();
        }

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

void hc::Control::addConsole(char const* const name) {
    _consoles.emplace(name);
}
