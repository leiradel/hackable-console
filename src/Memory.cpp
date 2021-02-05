#include "Memory.h"
#include "Logger.h"

#include <imguial_button.h>
#include <IconsFontAwesome4.h>

#include <inttypes.h>
#include <algorithm>

#define TAG "[MEM] "

hc::CoreMemory::CoreMemory(char const* name, bool readonly)
    : _name(name)
    , _base(0)
    , _size(0)
    , _readonly(readonly)
{}

bool hc::CoreMemory::addBlock(void* data, uint64_t offset, uint64_t base, uint64_t size) {
    if (_blocks.size() == 0) {
        _base = base;
        _size = 0;
    }
    else if (base != _base + _size) {
        return false;
    }

    _blocks.emplace_back(data, offset, size);
    _size += size;
    return true;
}

uint8_t hc::CoreMemory::peek(uint64_t address) const {
    address -= _base;

    for (auto const& block : _blocks) {
        if (address < block.size) {
            return static_cast<uint8_t const*>(block.data)[address];
        }

        address -= block.size;
    }

    return 0;
}

void hc::CoreMemory::poke(uint64_t address, uint8_t value) {
    address -= _base;

    for (auto const& block : _blocks) {
        if (address < block.size) {
            static_cast<uint8_t*>(block.data)[address] = value;
            return;
        }

        address -= block.size;
    }
}

void hc::MemorySelector::init() {}

void hc::MemorySelector::add(Memory* memory) {
    _regions.emplace_back(memory);

    std::sort(_regions.begin(), _regions.end(), [](Memory* const a, Memory* const b) -> bool {
        return strcmp(a->name(), b->name()) < 0;
    });
}

hc::Memory* hc::MemorySelector::translate(Handle<Memory*> const handle) {
    Memory** ref = _handles.translate(handle);
    return ref != nullptr ? *ref : nullptr;
}

char const* hc::MemorySelector::getTitle() {
    return ICON_FA_MICROCHIP " Memory";
}

void hc::MemorySelector::onDraw() {
    static auto const getter = [](void* const data, int const idx, char const** const text) -> bool {
        auto const self = static_cast<MemorySelector*>(data);

        *text = self->_regions[idx]->name();
        return true;
    };

    int const count = static_cast<int>(_regions.size());
    ImVec2 const size = ImVec2(120.0f, 0.0f);

    ImGui::Combo("##Regions", &_selected, getter, this, count);
    ImGui::SameLine();

    if (ImGuiAl::Button(ICON_FA_EYE " View", _selected < count, size)) {
        char title[128];
        snprintf(title, sizeof(title), ICON_FA_EYE" %s##%u", _regions[_selected]->name(), _viewCount++);

        MemoryWatch* watch = new MemoryWatch(_desktop, title, this, _handles.allocate(_regions[_selected]));
        _desktop->addView(watch, false, true);
    }
}

void hc::MemorySelector::onGameUnloaded() {
    _selected = 0;
    _viewCount = 0;

    for (auto const& memory : _regions) {
        delete memory;
    }

    _regions.clear();
    _handles.reset();
}

hc::MemoryWatch::MemoryWatch(Desktop* desktop, char const* title, MemorySelector* memorySelector, Handle<Memory*> handle)
    : View(desktop)
    , _title(title)
    , _memorySelector(memorySelector)
    , _handle(handle)
{
    _editor.OptUpperCaseHex = false;
    _editor.OptShowDataPreview = true;
    _editor.OptFooterExtraHeight = ImGui::GetTextLineHeight() * 5.0f;
    _editor.ReadOnly = memorySelector->translate(handle)->readonly();

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

    Memory const* const memory = _memorySelector->translate(_handle);

    if (memory != nullptr && _editor.DataPreviewAddr != (size_t)-1) {
        bool const clearSparline = _lastPreviewAddress != _editor.DataPreviewAddr ||
                                   _lastEndianess != _editor.PreviewEndianess ||
                                   _lastType != _editor.PreviewDataType;

        if (clearSparline) {
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
    Memory* const memory = _memorySelector->translate(_handle);

    if (memory != nullptr) {
        _editor.DrawContents(memory, memory->size(), memory->base());
        _sparkline.draw("#sparkline", ImGui::GetContentRegionAvail());
    }
}
