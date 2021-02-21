#include "Memory.h"
#include "Logger.h"

#include <imguial_button.h>
#include <IconsFontAwesome4.h>

#include <inttypes.h>
#include <algorithm>

#define TAG "[MEM] "

unsigned hc::Memory::requiredDigits() {
    unsigned count = 0;

    for (uint64_t maximum = base() + size() - 1; maximum > 0; maximum >>= 4) {
        count++;
    }

    return count;
}

void hc::MemorySelector::init() {}

void hc::MemorySelector::add(Memory* memory) {
    _regions.emplace_back(memory);
}

char const* hc::MemorySelector::getTitle() {
    return ICON_FA_MICROCHIP " Memory";
}

void hc::MemorySelector::onDraw() {
    static auto const getter = [](void* const data, int const idx, char const** const text) -> bool {
        auto const regions = static_cast<std::vector<Memory*> const*>(data);
        *text = (*regions)[idx]->name();
        return true;
    };

    int const count = static_cast<int>(_regions.size());
    ImVec2 const size = ImVec2(120.0f, 0.0f);

    ImGui::Combo("##Regions", &_selected, getter, &_regions, count);
    ImGui::SameLine();

    if (ImGuiAl::Button(ICON_FA_EYE " View", _selected < count, size)) {
        char title[128];
        snprintf(title, sizeof(title), ICON_FA_EYE" %s##%u", _regions[_selected]->name(), _viewCount++);

        MemoryWatch* watch = new MemoryWatch(_desktop, title, _regions[_selected]);
        _desktop->addView(watch, false, true);
    }
}

void hc::MemorySelector::onGameUnloaded() {
    _selected = 0;
    _viewCount = 0;
    _regions.clear();
}

hc::MemoryWatch::MemoryWatch(Desktop* desktop, char const* title, Memory* memory)
    : View(desktop)
    , _title(title)
    , _memory(memory)
{
    _editor.OptUpperCaseHex = false;
    _editor.OptShowDataPreview = true;
    _editor.OptFooterExtraHeight = ImGui::GetTextLineHeight() * 5.0f;
    _editor.ReadOnly = _memory->readonly();

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

    if (_memory == nullptr) {
        return;
    }

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

        uint64_t address = _editor.DataPreviewAddr + _memory->base();
        uint64_t const size = sizes[_editor.PreviewDataType];
        uint64_t value = 0;

        switch (size) {
            case 8: value = value << 8 | _memory->peek(address++);
            case 7: value = value << 8 | _memory->peek(address++);
            case 6: value = value << 8 | _memory->peek(address++);
            case 5: value = value << 8 | _memory->peek(address++);
            case 4: value = value << 8 | _memory->peek(address++);
            case 3: value = value << 8 | _memory->peek(address++);
            case 2: value = value << 8 | _memory->peek(address++);
            case 1: value = value << 8 | _memory->peek(address++);
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
    if (_memory != nullptr) {
        _editor.DrawContents(_memory, _memory->size(), _memory->base());
        _sparkline.draw("#sparkline", ImGui::GetContentRegionAvail());
    }
}

void hc::MemoryWatch::onGameUnloaded() {
    _memory = nullptr;
}
