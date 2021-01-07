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
        char const* const path = gameDialog.chooseFileDialog(loadGamePressed, nullptr, nullptr, ICON_FA_FOLDER_OPEN" Open Game");

        if (path != nullptr && path[0] != 0) {
            _fsm->loadGame(path);
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

void hc::Control::addConsole(char const* const name) {
    _consoles.emplace(name);
}
