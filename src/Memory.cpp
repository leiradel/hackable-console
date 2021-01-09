#include "Memory.h"

#include <imguial_button.h>
#include <IconsFontAwesome4.h>

bool hc::Memory::init(Logger* logger) {
    _logger = logger;
    return true;
}

void hc::Memory::destroy() {}

void hc::Memory::reset() {
    _selected = 0;
    _views.clear();
    _regions.clear();
}

void hc::Memory::draw() {
    static auto const getter = [](void* const data, int const idx, char const** const text) -> bool {
        auto const regions = (std::vector<Region>*)data;

        if (idx < static_cast<int>(regions->size())) {
            *text = (*regions)[idx].name.c_str();
            return true;
        }

        return false;
    };

    if (ImGui::Begin(ICON_FA_MICROCHIP " Memory")) {
        int const count = static_cast<int>(_regions.size());
        ImVec2 const size = ImVec2(120.0f, 0.0f);

        ImGui::Combo("##Regions", &_selected, getter, &_regions, count);
        ImGui::SameLine();

        if (ImGuiAl::Button(ICON_FA_EYE " View Region", _selected < count, size)) {
            _views.emplace_back(_regions[_selected]);
        }
    }

    ImGui::End();

    for (size_t i = 0; i < _views.size();) {
        auto& view = _views[i];

        char label[128];
        snprintf(label, sizeof(label), ICON_FA_EYE" %s##%zu", view.region.name.c_str(), i);

        bool open = true;

        if (ImGui::Begin(label, &open)) {
            view.editor.DrawContents(view.region.data, view.region.size, view.region.base);
        }

        if (open) {
            i++;
        }
        else {
            _views.erase(_views.begin() + i);
        }

        ImGui::End();
    }
}

void hc::Memory::addRegion(char const* const name, void* data, size_t const base, size_t const size, bool const readOnly) {
    _regions.emplace_back(name, data, base, size, readOnly);
}

hc::Memory::Region::Region(char const* name, void* data, size_t base, size_t size, bool readOnly)
    : name(name)
    , data(data)
    , base(base)
    , size(size)
    , readOnly(readOnly)
{}

hc::Memory::View::View(Region const& region) : region(region) {
    editor.OptUpperCaseHex = false;
    editor.ReadOnly = region.readOnly;
}
