#include "Memory.h"
#include "Logger.h"

#include <imguial_button.h>
#include <IconsFontAwesome4.h>

#include <inttypes.h>

#define TAG "[MEM] "

uint8_t hc::Memory::Region::peek(size_t address) const {
    address -= base;

    for (auto const& block : blocks) {
        if (address < block.size) {
            return static_cast<uint8_t const*>(block.data)[address];
        }

        address -= block.size;
    }

    return 0;
}

void hc::Memory::Region::poke(size_t address, uint8_t value) {
    address -= base;

    for (auto const& block : blocks) {
        if (address < block.size) {
            static_cast<uint8_t*>(block.data)[address] = value;
            return;
        }

        address -= block.size;
    }
}

void hc::Memory::init() {}

hc::Memory::Region* hc::Memory::translate(Handle<Region> const handle) {
    return _handles.translate(handle);
}

char const* hc::Memory::getTitle() {
    return ICON_FA_MICROCHIP " Memory";
}

void hc::Memory::onDraw() {
    static auto const getter = [](void* const data, int const idx, char const** const text) -> bool {
        auto const self = static_cast<Memory*>(data);

        if (idx < static_cast<int>(self->_regions.size())) {
            *text = self->translate(self->_regions[idx])->name.c_str();
            return true;
        }

        return false;
    };

    int const count = static_cast<int>(_regions.size());
    ImVec2 const size = ImVec2(120.0f, 0.0f);

    ImGui::Combo("##Regions", &_selected, getter, this, count);
    ImGui::SameLine();

    if (ImGuiAl::Button(ICON_FA_EYE " View Region", _selected < count, size)) {
        char title[128];
        snprintf(title, sizeof(title), ICON_FA_EYE" %s##%u", translate(_regions[_selected])->name.c_str(), _viewCount++);

        MemoryWatch* watch = new MemoryWatch(_desktop, title, this, _regions[_selected]);
        _desktop->addView(watch, false, true);
    }
}

void hc::Memory::onGameUnloaded() {
    _selected = 0;
    _viewCount = 0;

    for (auto const& handle : _regions) {
        _handles.free(handle);
    }

    _regions.clear();
    _handles.reset();
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
    int const readOnly = lua_toboolean(L, 3);

    int const top = lua_gettop(L);

    Handle<Region> const handle = self->_handles.allocate();
    Region* region = self->translate(handle);

    region->name = std::string(name, length);
    region->base = 0;
    region->size = 0;
    region->readOnly = readOnly != 0;

    int i = 4;

    do {
        luaL_argexpected(L, lua_type(L, i) == LUA_TTABLE, i, "table");
        int isnum = 0;

        // data
        lua_geti(L, i, 1);
        void* const data = lua_touserdata(L, -1);

        if (data == nullptr) {
            self->_handles.free(handle);
            return luaL_error(L, "data is null in block %d", i - 3);
        }

        // base
        lua_geti(L, i, 2);
        lua_Integer const base = lua_tointegerx(L, -1, &isnum);

        if (!isnum) {
            self->_handles.free(handle);
            return luaL_error(L, "base address is not a number in block %d", i - 3);
        }
        else if (base < 0) {
            self->_handles.free(handle);
            return luaL_error(L, "base address is negative in block %d", i - 3);
        }

        if (i == 4) {
            region->base = base;
        }
        else if (static_cast<size_t>(base) != (region->base + region->size)) {
            self->_handles.free(handle);
            return luaL_error(L, "base address for block %d is not contiguous to the previous block", i - 3);
        }

        // size
        lua_geti(L, i, 3);
        lua_Integer const size = lua_tointegerx(L, -1, &isnum);

        if (!isnum) {
            self->_handles.free(handle);
            return luaL_error(L, "size is not a number in block %d", i - 3);
        }
        else if (size <= 0) {
            self->_handles.free(handle);
            return luaL_error(L, "invalid size %I in block %d", size, i - 3);
        }

        // offset
        lua_geti(L, i, 4);
        lua_Integer offset = lua_tointegerx(L, -1, &isnum);

        if (isnum && offset < 0) {
            self->_handles.free(handle);
            return luaL_error(L, "invalid offset %I in block %d", offset, i - 3);
        }

        lua_pop(L, 4);

        region->blocks.emplace_back();
        Block& block = region->blocks[region->blocks.size() - 1];
        block.data = data;
        block.offset = offset;
        block.size = size;

        region->size += size;
    }
    while (++i <= top);

    self->_regions.emplace_back(handle);
    self->_desktop->info(TAG "Added memory region \"%s\"", name);
    return 0;
}

hc::MemoryWatch::MemoryWatch(Desktop* desktop, char const* title, Memory* memory, Handle<Memory::Region> handle)
    : View(desktop)
    , _title(title)
    , _memory(memory)
    , _handle(handle)
{
    _editor.OptUpperCaseHex = false;
    _editor.OptShowDataPreview = true;
    _editor.ReadOnly = memory->translate(handle)->readOnly;

    _editor.ReadFn = [](const ImU8* data, size_t off) -> ImU8 {
        auto const region = reinterpret_cast<Memory::Region const*>(data);
        return region->peek(off);
    };

    _editor.WriteFn = [](ImU8* data, size_t off, ImU8 d) -> void {
        auto region = reinterpret_cast<Memory::Region*>(data);
        region->poke(off, d);
    };

    _lastPreviewAddress = (size_t)-1;
    _lastEndianess = -1;
    _lastType = ImGuiDataType_COUNT;
}

char const* hc::MemoryWatch::getTitle() {
    return _title.c_str();
}

void hc::MemoryWatch::onFrame() {
    static uint8_t sizes[ImGuiDataType_COUNT] = {1, 1, 2, 2, 4, 4, 8, 8, 4, 8};

    Memory::Region const* const region = _memory->translate(_handle);

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

        uint64_t address = _editor.DataPreviewAddr;
        uint64_t const size = sizes[_editor.PreviewDataType];
        uint64_t value = 0;

        switch (size) {
            case 8: value = value << 8 | region->peek(address++);
            case 7: value = value << 8 | region->peek(address++);
            case 6: value = value << 8 | region->peek(address++);
            case 5: value = value << 8 | region->peek(address++);
            case 4: value = value << 8 | region->peek(address++);
            case 3: value = value << 8 | region->peek(address++);
            case 2: value = value << 8 | region->peek(address++);
            case 1: value = value << 8 | region->peek(address++);
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

    Memory::Region* const region = _memory->translate(_handle);

    if (region != nullptr) {
        _editor.DrawContents(region, region->size, region->base, custom, this, ImGui::GetTextLineHeight() * 5.0f);
    }
}
