#include "Control.h"

#include <imguial_button.h>
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

    if (ImGui::Begin(ICON_FA_COG " Control")) {
        int const count = static_cast<int>(_consoles.size());
        ImVec2 const size = ImVec2(100.0f, 0.0f);

        ImGui::Combo("Consoles", &_selected, getter, &_consoles, count);

        if (ImGuiAl::Button("Load Console", _fsm->currentState() == LifeCycle::State::Start && _selected < count, size)) {
            char const* name = nullptr;

            if (getter(&_consoles, _selected, &name)) {
                _fsm->loadConsole(name);
            }
        }
    }

    ImGui::End();
}

void hc::Control::addConsole(char const* const name) {
    _consoles.emplace(name);
}
