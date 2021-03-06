#include "Memory.h"
#include "Logger.h"
#include "cheats/Snapshot.h"

#include <imguial_button.h>
#include <IconsFontAwesome4.h>

#include <inttypes.h>
#include <string.h>
#include <algorithm>

#define TAG "[MEM] "

#ifdef HC_DEBUG_MEMORY_ENABLED
namespace {
    class DebugMemory : public hc::Memory {
    public:
        DebugMemory() {
            memset(&_memory, 0, sizeof(_memory));
            _memory.static_ = UINT64_C(0xdeadbeefbaadf00d);
        }

        // hc::Memory
        virtual char const* id() const override { return "debug"; }
        virtual char const* name() const override { return "Debug Memory"; }
        virtual uint64_t base() const override { return 0; }
        virtual uint64_t size() const override { return sizeof(_memory); }
        virtual bool readonly() const override { return false; }

        virtual uint8_t peek(uint64_t address) const override {
            return address <= sizeof(_memory) ? ((uint8_t*)&_memory)[address] : 0;
        }

        virtual void poke(uint64_t address, uint8_t value) override {
            if (address <= sizeof(_memory)) {
                ((uint8_t*)&_memory)[address] = value;
            }
        }

        void tick() {
            _memory.counter64++;
            _memory.counter32++;
            _memory.counter16++;
            _memory.counter8++;

            _memory.random = static_cast<uint64_t>(rand() % 0xff);
            _memory.random |= static_cast<uint64_t>(rand() % 0xff) << 8;
            _memory.random |= static_cast<uint64_t>(rand() % 0xff) << 16;
            _memory.random |= static_cast<uint64_t>(rand() % 0xff) << 24;
            _memory.random |= static_cast<uint64_t>(rand() % 0xff) << 32;
            _memory.random |= static_cast<uint64_t>(rand() % 0xff) << 40;
            _memory.random |= static_cast<uint64_t>(rand() % 0xff) << 48;
            _memory.random |= static_cast<uint64_t>(rand() % 0xff) << 56;
        }

    protected:
        struct {
            uint64_t counter64;
            uint32_t counter32;
            uint16_t counter16;
            uint8_t counter8;
            uint64_t random;
            uint64_t static_;
            uint64_t custom;
        }
        _memory;
    };
}
#endif

static void formatU64(char* const buffer, size_t const size, uint64_t const value) {
    if (value <= UINT64_C(0xffff)) {
        snprintf(buffer, size, "0x%04" PRIx64, value);
    }
    else if (value <= UINT64_C(0xffffffff)) {
        snprintf(buffer, size, "0x%08" PRIx64, value);
    }
    else {
        snprintf(buffer, size, "0x%016" PRIx64, value);
    }
}

static int pushU64(lua_State* const L, uint64_t const value) {
    auto const i = static_cast<lua_Integer>(value);

    if (static_cast<uint64_t>(i) == value) {
        lua_pushinteger(L, i);
    }
    else {
        lua_pushnumber(L, value);
    }

    return 1;
}

#define MEMORY_MT "hc::Memory"

namespace {
    class MemoryHandle : public hc::Memory {
    public:
        MemoryHandle(hc::Handle<hc::Memory*> handle, hc::MemorySelector* selector): _handle(handle), _selector(selector) {}
        virtual ~MemoryHandle() {}

        // hc::Memory
        virtual char const* id() const override {
            Memory* const* const memptr = _selector->translate(_handle);
            return memptr != nullptr ? (*memptr)->id() : "(invalid)";
        }

        virtual char const* name() const override {
            Memory* const* const memptr = _selector->translate(_handle);
            return memptr != nullptr ? (*memptr)->name() : "(invalid)";
        }

        virtual uint64_t base() const override {
            Memory* const* const memptr = _selector->translate(_handle);
            return memptr != nullptr ? (*memptr)->base() : 0;
        }

        virtual uint64_t size() const override {
            Memory* const* const memptr = _selector->translate(_handle);
            return memptr != nullptr ? (*memptr)->size() : 0;
        }

        virtual bool readonly() const override {
            Memory* const* const memptr = _selector->translate(_handle);
            return memptr != nullptr ? (*memptr)->readonly() : true;
        }

        virtual uint8_t peek(uint64_t address) const override {
            Memory* const* const memptr = _selector->translate(_handle);
            return memptr != nullptr ? (*memptr)->peek(address) : 0;
        }

        virtual void poke(uint64_t address, uint8_t value) override {
            Memory* const* const memptr = _selector->translate(_handle);
            
            if (memptr != nullptr) {
                (*memptr)->poke(address, value);
            }
        }

    protected:
        hc::Handle<hc::Memory*> const _handle;
        hc::MemorySelector* const _selector;
    };
}

unsigned hc::Memory::requiredDigits() {
    unsigned count = 0;

    for (uint64_t maximum = base() + size() - 1; maximum > 0; maximum >>= 4) {
        count++;
    }

    return count;
}

bool hc::Memory::find(uint64_t* start, uint8_t const* bytes, size_t length) {
    if (length < 1 || length > size()) {
        return false;
    }

    uint64_t const end = size() - length + 1;
    uint8_t const first = bytes[0];

    for (uint64_t i = *start; i < end; i++) {
        if (peek(i) == first) {
            bool found = true;

            for (uint64_t j = 1; j < length; j++) {
                if (bytes[j] != peek(i + j)) {
                    found = false;
                    break;
                }
            }

            if (found) {
                *start = i;
                return true;
            }
        }
    }

    return false;
}

hc::Memory* hc::Memory::check(lua_State* L, int index) {
    return *static_cast<Memory**>(luaL_checkudata(L, index, MEMORY_MT));
}

bool hc::Memory::is(lua_State* L, int index) {
    return luaL_testudata(L, index, MEMORY_MT) != nullptr;
}

int hc::Memory::push(lua_State* L) {
    Memory** const self = static_cast<Memory**>(lua_newuserdata(L, sizeof(*self)));
    *self = this;

    if (luaL_newmetatable(L, MEMORY_MT)) {
        static const luaL_Reg methods[] = {
            {"id", l_id},
            {"name", l_name},
            {"base", l_base},
            {"size", l_size},
            {"readonly", l_readonly},
            {"peek", l_peek},
            {"poke", l_poke},
            {"find", l_find},
            {"snapshot", l_snapshot},
            {NULL, NULL}
        };

        luaL_newlib(L, methods);
        lua_setfield(L, -2, "__index");
    }

    lua_setmetatable(L, -2);
    return 1;
}

int hc::Memory::l_id(lua_State* L) {
    auto const self = check(L, 1);
    lua_pushstring(L, self->id());
    return 1;
}

int hc::Memory::l_name(lua_State* L) {
    auto const self = check(L, 1);
    lua_pushstring(L, self->name());
    return 1;
}

int hc::Memory::l_base(lua_State* L) {
    auto const self = check(L, 1);
    return pushU64(L, self->base());
}

int hc::Memory::l_size(lua_State* L) {
    auto const self = check(L, 1);
    return pushU64(L, self->size());
}

int hc::Memory::l_readonly(lua_State* L) {
    auto const self = check(L, 1);
    lua_pushboolean(L, self->readonly());
    return 1;
}

int hc::Memory::l_peek(lua_State* L) {
    auto const self = check(L, 1);
    size_t const address = luaL_checkinteger(L, 2);

    if (address - self->base() >= self->size()) {
        char buffer[64];
        formatU64(buffer, sizeof(buffer), address);
        return luaL_error(L, "address out of bounds: %s", buffer);
    }

    lua_pushinteger(L, self->peek(address));
    return 1;
}

int hc::Memory::l_poke(lua_State* L) {
    auto const self = check(L, 1);
    size_t const address = luaL_checkinteger(L, 2);
    uint8_t const value = luaL_checkinteger(L, 3);

    if (address - self->base() >= self->size()) {
        char buffer[64];
        formatU64(buffer, sizeof(buffer), address);
        return luaL_error(L, "address out of bounds: %s", buffer);
    }

    self->poke(address, value);
    return 0;
}

int hc::Memory::l_find(lua_State* L) {
    auto const self = check(L, 1);
    uint64_t address = luaL_checkinteger(L, 2);

    if (lua_type(L, 3) == LUA_TTABLE) {
        lua_len(L, 3);
        lua_Integer const length = lua_tointeger(L, -1);
        lua_pop(L, 1);

        uint8_t* const bytes = new uint8_t[length];

        for (lua_Integer i = 0; i < length; i++) {
            lua_geti(L, 3, i + 1);
            bytes[i] = static_cast<lua_Integer>(lua_tonumber(L, -1));
            lua_pop(L, 1);
        }

        if (self->find(&address, bytes, length)) {
            delete[] bytes;
            lua_pushinteger(L, address);
            return 1;
        }
    }
    else {
        size_t length;
        char const* string = luaL_checklstring(L, 3, &length);

        if (self->find(&address, reinterpret_cast<uint8_t const*>(string), length)) {
            lua_pushinteger(L, address);
            return 1;
        }
    }

    return 0;
}

int hc::Memory::l_snapshot(lua_State* L) {
    auto const self = check(L, 1);
    auto const snapshot = new Snapshot(self);
    return snapshot->push(L);
}

void hc::MemorySelector::init() {
#ifdef HC_DEBUG_MEMORY_ENABLED
    add(new DebugMemory());
#endif
}

void hc::MemorySelector::add(Memory* memory) {
    _regions.emplace_back(memory);
}

bool hc::MemorySelector::select(char const* const label, int* const selected, Handle<Memory*>* const handle) {
    static auto const getter = [](void* const data, int const idx, char const** const text) -> bool {
        auto const regions = static_cast<std::vector<Memory*> const*>(data);
        *text = (*regions)[idx]->name();
        return true;
    };

    int const count = static_cast<int>(_regions.size());

    if (*selected >= count) {
        *selected = 0;
    }

    ImGui::PushID(selected);
    ImGui::Combo("##memory_selector", selected, getter, &_regions, count);
    ImGui::SameLine();

    Memory* memory = nullptr;

    if (ImGuiAl::Button(label, *selected < count, ImVec2(120.0f, 0.0f))) {
        memory = _regions[*selected];
    }

    ImGui::PopID();

    if (memory != nullptr) {
        *handle = _handleAllocator.allocate(memory);
        return true;
    }

    return false;
}

char const* hc::MemorySelector::getTitle() {
    return ICON_FA_MICROCHIP " Memory";
}

void hc::MemorySelector::onFrame() {
#ifdef HC_DEBUG_MEMORY_ENABLED
    static_cast<DebugMemory*>(_regions[0])->tick();
#endif
}

void hc::MemorySelector::onDraw() {
    Handle<Memory*> handle;

    if (select(ICON_FA_EYE " View", &_selected, &handle)) {
        MemoryWatch* watch = new MemoryWatch(_desktop, handle, this);
        _desktop->addView(watch, false, true);
    }
}

void hc::MemorySelector::onGameUnloaded() {
    _selected = 0;
    _handleAllocator.reset();

#ifdef HC_DEBUG_MEMORY_ENABLED
    _regions.erase(_regions.begin() + 1, _regions.end());
#else
    _regions.clear();
#endif
}

int hc::MemorySelector::push(lua_State* const L) {
    auto const self = static_cast<MemorySelector**>(lua_newuserdata(L, sizeof(MemorySelector*)));
    *self = this;

    if (luaL_newmetatable(L, "hc::MemorySelector")) {
        lua_pushcfunction(L, l_index);
        lua_setfield(L, -2, "__index");
    }

    lua_setmetatable(L, -2);
    return 1;
}

hc::MemorySelector* hc::MemorySelector::check(lua_State* const L, int const index) {
    return *static_cast<MemorySelector**>(luaL_checkudata(L, index, "hc::MemorySelector"));
}

int hc::MemorySelector::l_index(lua_State* const L) {
    auto const self = check(L, 1);
    char const* const id = luaL_checkstring(L, 2);

    for (auto const& region : self->_regions) {
        if (!strcmp(id, region->id())) {
            Handle<Memory*> handle = self->_handleAllocator.allocate(region);
            auto memory = new MemoryHandle(handle, self);
            return memory->push(L);
        }
    }

    return luaL_error(L, "unknown memory id \"%s\"", id);
}

hc::MemoryWatch::MemoryWatch(Desktop* desktop, Handle<Memory*> handle, MemorySelector* selector)
    : View(desktop)
    , _handle(handle)
    , _selector(selector)
{
    Memory* const* const memptr = selector->translate(handle);
    Memory* const memory = *memptr;

    char title[128];
    snprintf(title, sizeof(title), ICON_FA_EYE" %s##%p", memory->name(), static_cast<void*>(memory));
    _title = title;

    _editor.OptUpperCaseHex = false;
    _editor.OptShowDataPreview = true;
    _editor.OptFooterExtraHeight = ImGui::GetTextLineHeight() * 5.0f;
    _editor.ReadOnly = memory->readonly();

    _editor.ReadFn = [](const ImU8* data, size_t off) -> ImU8 {
        auto const region = reinterpret_cast<Memory const*>(data);
        return region->peek(region->base() + off);
    };

    _editor.WriteFn = [](ImU8* data, size_t off, ImU8 d) -> void {
        auto region = reinterpret_cast<Memory*>(data);
        region->poke(region->base() + off, d);
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

    Memory* const* const memptr = _selector->translate(_handle);

    if (memptr == nullptr) {
        return;
    }

    Memory* const memory = *memptr;

    if (_editor.DataPreviewAddr != (size_t)-1) {
        bool const clearSparkline = _lastPreviewAddress != _editor.DataPreviewAddr ||
                                    _lastEndianess != _editor.PreviewEndianess ||
                                    _lastType != _editor.PreviewDataType;

        if (clearSparkline) {
            _sparkline.clear();
            _lastPreviewAddress = _editor.DataPreviewAddr;
            _lastEndianess = _editor.PreviewEndianess;
            _lastType = _editor.PreviewDataType;
        }

        uint64_t address = _editor.DataPreviewAddr + memory->base();
        uint64_t const size = sizes[_editor.PreviewDataType];
        uint64_t value = 0;

        switch (size) {
            case 8: value = value << 8 | memory->peek(address++);
            case 7: value = value << 8 | memory->peek(address++);
            case 6: value = value << 8 | memory->peek(address++);
            case 5: value = value << 8 | memory->peek(address++);
            case 4: value = value << 8 | memory->peek(address++);
            case 3: value = value << 8 | memory->peek(address++);
            case 2: value = value << 8 | memory->peek(address++);
            case 1: value = value << 8 | memory->peek(address++);
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
    Memory* const* const memptr = _selector->translate(_handle);

    if (memptr == nullptr) {
        _desktop->removeView(this);
        return;
    }

    Memory* const memory = *memptr;

    _editor.DrawContents(memory, memory->size(), memory->base());
    _sparkline.draw("#sparkline", ImGui::GetContentRegionAvail());
}
