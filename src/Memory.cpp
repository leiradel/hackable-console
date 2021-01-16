#include "Memory.h"

#include <imguial_button.h>
#include <IconsFontAwesome4.h>

#include <inttypes.h>

#define TAG "[MEM] "

hc::Memory::Region::Region(std::string&& name, void* data, size_t offset, size_t size, size_t base, bool readOnly)
    : name(std::move(name))
    , data(data)
    , offset(offset)
    , size(size)
    , base(base)
    , readOnly(readOnly)
{}

hc::Memory::View::View(Region const& region) : region(region) {
    editor.OptUpperCaseHex = false;
    editor.ReadOnly = region.readOnly;
}

void hc::Memory::init(Logger* const logger) {
    _logger = logger;
}

char const* hc::Memory::getName() {
    return "hc::Memory built-in memory plugin";
}

char const* hc::Memory::getVersion() {
    return "0.0.0";
}

char const* hc::Memory::getLicense() {
    return "MIT";
}

char const* hc::Memory::getCopyright() {
    return "Copyright (c) Andre Leiradella";
}

char const* hc::Memory::getUrl() {
    return "https://github.com/leiradel/hackable-console";
}

void hc::Memory::onStarted() {}

void hc::Memory::onConsoleLoaded() {}

void hc::Memory::onGameLoaded() {}

void hc::Memory::onGamePaused() {}

void hc::Memory::onGameResumed() {}

void hc::Memory::onGameReset() {}

void hc::Memory::onFrame() {}

void hc::Memory::onDraw(bool* opened) {
    static auto const getter = [](void* const data, int const idx, char const** const text) -> bool {
        auto const regions = (std::vector<Region>*)data;

        if (idx < static_cast<int>(regions->size())) {
            *text = (*regions)[idx].name.c_str();
            return true;
        }

        return false;
    };

    if (*opened) {
        if (ImGui::Begin(ICON_FA_MICROCHIP " Memory", opened)) {
            int const count = static_cast<int>(_regions.size());
            ImVec2 const size = ImVec2(120.0f, 0.0f);

            ImGui::Combo("##Regions", &_selected, getter, &_regions, count);
            ImGui::SameLine();

            if (ImGuiAl::Button(ICON_FA_EYE " View Region", _selected < count, size)) {
                _views.emplace_back(_regions[_selected]);
            }
        }

        ImGui::End();
    }

    for (size_t i = 0; i < _views.size();) {
        auto& view = _views[i];

        char label[128];
        snprintf(label, sizeof(label), ICON_FA_EYE" %s##%zu", view.region.name.c_str(), i);

        bool open = true;

        if (ImGui::Begin(label, &open)) {
            auto const data = static_cast<uint8_t*>(view.region.data) + view.region.offset;
            view.editor.DrawContents(data, view.region.size, view.region.base);
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

void hc::Memory::onGameUnloaded() {
    _selected = 0;
    _views.clear();
    _regions.clear();
}

void hc::Memory::onConsoleUnloaded() {}

void hc::Memory::onQuit() {}

int hc::Memory::push(lua_State* const L) {
    auto const self = static_cast<Memory**>(lua_newuserdata(L, sizeof(Memory*)));
    *self = this;

    if (luaL_newmetatable(L, "hc::Memory")) {
        static luaL_Reg const methods[] = {
            {"addRegion", l_addRegion},
            {nullptr, nullptr}
        };

        luaL_newlib(L, methods);
        lua_setfield(L, -2, "__index");
    }

    lua_setmetatable(L, -2);
    return 1;
}

hc::Memory* hc::Memory::check(lua_State* const L, int const index) {
    return *static_cast<Memory**>(luaL_checkudata(L, index, "hc::Memory"));
}

int hc::Memory::l_addRegion(lua_State* const L) {
    auto const self = check(L, 1);
    size_t length = 0;
    char const* const name = luaL_checklstring(L, 2, &length);
    lua_Integer const base = luaL_checkinteger(L, 3);
    void* const data = lua_touserdata(L, 4);
    lua_Integer const size = luaL_checkinteger(L, 5);
    lua_Integer const offset = luaL_optinteger(L, 6, 0);
    int const readOnly = lua_toboolean(L, 7);

    if (data == nullptr) {
        return luaL_error(L, "data is null");
    }

    if (size <= 0) {
        return luaL_error(L, "invalid size: %I", size);
    }

    if (offset < 0) {
        return luaL_error(L, "invalid offset: %I", size);
    }

    self->_regions.emplace_back(std::string(name, length), data, offset, size, base, readOnly);

    self->_logger->info(
        TAG "Added memory region {\"%s\", 0x%016" PRIxPTR ", 0x%016" PRIxPTR", %zu, %zu, %s}",
        name, static_cast<uintptr_t>(base), reinterpret_cast<uintptr_t>(data), size, offset, readOnly ? "true" : "false"
    );

    return 0;
}
