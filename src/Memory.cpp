#include "Memory.h"
#include "Logger.h"

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

void hc::Memory::init() {
    _logger = _desktop->getView<Logger>();
}

hc::Memory::Region* hc::Memory::lock(Handle const handle) {
    if (handle != 0 && handle <= _regions.size()) {
        return &_regions[handle - 1];
    }

    return nullptr;
}

char const* hc::Memory::getTitle() {
    return ICON_FA_MICROCHIP " Memory";
}

void hc::Memory::onDraw() {
    static auto const getter = [](void* const data, int const idx, char const** const text) -> bool {
        auto const regions = (std::vector<Region>*)data;

        if (idx < static_cast<int>(regions->size())) {
            *text = (*regions)[idx].name.c_str();
            return true;
        }

        return false;
    };

    int const count = static_cast<int>(_regions.size());
    ImVec2 const size = ImVec2(120.0f, 0.0f);

    ImGui::Combo("##Regions", &_selected, getter, &_regions, count);
    ImGui::SameLine();

    if (ImGuiAl::Button(ICON_FA_EYE " View Region", _selected < count, size)) {
        char title[128];
        snprintf(title, sizeof(title), ICON_FA_EYE" %s##%u", _regions[_selected].name.c_str(), _viewCount++);

        MemoryWatch* watch = new MemoryWatch(_desktop);
        watch->init(_logger, title, this, _selected + 1);
        _desktop->addView(watch, false, true, nullptr);
    }
}

void hc::Memory::onGameUnloaded() {
    _selected = 0;
    _viewCount = 0;
    _regions.clear();
}

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

void hc::MemoryWatch::init(Logger* const logger, char const* title, Memory* const memory, Memory::Handle const handle) {
    _logger = logger;
    _title = title;
    _memory = memory;
    _handle = handle;

    _editor.OptUpperCaseHex = false;
    _editor.OptShowDataPreview = true;
    _editor.ReadOnly = memory->lock(handle)->readOnly;

    _lastPreviewAddress = (size_t)-1;
    _lastEndianess = -1;
    _lastType = ImGuiDataType_COUNT;
}

char const* hc::MemoryWatch::getTitle() {
    return _title.c_str();
}

void hc::MemoryWatch::onFrame() {
    static uint8_t sizes[ImGuiDataType_COUNT] = {1, 1, 2, 2, 4, 4, 8, 8, 4, 8};

    Memory::Region* const region = _memory->lock(_handle);

    if (region != nullptr && _editor.DataPreviewAddr != (size_t)-1) {
        bool const clearSparline = _lastPreviewAddress != _editor.DataPreviewAddr ||
                                   _lastEndianess != _editor.PreviewEndianess ||
                                   _lastType != _editor.PreviewDataType;

        if (clearSparline) {
            _sparkline.clear();
            _lastPreviewAddress = _editor.DataPreviewAddr;
            _lastEndianess = _editor.PreviewEndianess;
            _lastType = _editor.PreviewDataType;
        }

        auto const data = static_cast<uint8_t*>(region->data) + region->offset;
        uint64_t address = _editor.DataPreviewAddr - region->offset;
        uint64_t const left = region->size - address;
        uint64_t const size = sizes[_editor.PreviewDataType];

        uint64_t value = 0;

        switch (std::min(left, size)) {
            case 8: value = value << 8 | data[address++];
            case 7: value = value << 8 | data[address++];
            case 6: value = value << 8 | data[address++];
            case 5: value = value << 8 | data[address++];
            case 4: value = value << 8 | data[address++];
            case 3: value = value << 8 | data[address++];
            case 2: value = value << 8 | data[address++];
            case 1: value = value << 8 | data[address++];
        }

        if (_editor.PreviewEndianess == 0) {
            uint64_t le = 0;
            uint8_t* dest = (uint8_t*)&le;
            uint8_t* source = (uint8_t*)&value + size;

            switch (size) {
                case 8: *dest++ = *--source;
                case 7: *dest++ = *--source;
                case 6: *dest++ = *--source;
                case 5: *dest++ = *--source;
                case 4: *dest++ = *--source;
                case 3: *dest++ = *--source;
                case 2: *dest++ = *--source;
                case 1: *dest++ = *--source;
            }

            value = le;
        }

        if (_editor.PreviewDataType == ImGuiDataType_Float) {
            float val;
            memcpy(&val, &value, sizeof(val));
            _sparkline.add(val);
        }
        else if (_editor.PreviewDataType == ImGuiDataType_Double) {
            double val;
            memcpy(&val, &value, sizeof(val));
            _sparkline.add(static_cast<float>(val));
        }
        else {
            _sparkline.add(static_cast<float>(value));
        }
    }
}

void hc::MemoryWatch::onDraw() {
    static auto const custom = [](MemoryEditor* editor, void* userdata) {
        auto* const self = static_cast<MemoryWatch*>(userdata);
        ImVec2 const max = ImGui::GetContentRegionAvail();
        self->_sparkline.draw("#sparkline", max);
    };

    Memory::Region* const region = _memory->lock(_handle);

    if (region != nullptr) {
        auto const data = static_cast<uint8_t*>(region->data) + region->offset;
        _editor.DrawContents(data, region->size, region->base, custom, this, ImGui::GetTextLineHeight() * 5.0f);
    }
}
