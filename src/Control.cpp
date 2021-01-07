#include "Control.h"

#include <IconsFontAwesome4.h>

bool hc::Control::init(Logger* logger) {
    _logger = logger;
    return true;
}

void hc::Control::destroy() {}

void hc::Control::reset() {
    _selected = 0;
}

void hc::Control::draw() {
    if (ImGui::Begin(ICON_FA_COG " Control")) {
        int current = _selected;

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

        ImGui::Combo("Consoles", &current, getter, (void*)&_consoles, (int)_consoles.size());
    }

    ImGui::End();

}

void hc::Control::addConsole(char const* const name) {
    _consoles.emplace(name);
}
