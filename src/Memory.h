#pragma once

#include "Logger.h"

#include <imgui_memory_editor.h>

#include <string>
#include <vector>

namespace hc {
    class Memory {
    public:
        Memory() : _logger(nullptr), _selected(0) {}

        bool init(Logger* logger);
        void destroy();
        void reset();
        void draw();

        void addRegion(char const* const name, void* data, size_t const base, size_t const size, bool const readOnly);

    protected:
        struct Region {
            Region(char const* name, void* data, size_t base, size_t size, bool readOnly);

            std::string name;
            void* data;
            size_t base;
            size_t size;
            bool readOnly;
        };

        struct View {
            View(Region const& region);

            Region region;
            MemoryEditor editor;
        };

        Logger* _logger;

        std::vector<Region> _regions;
        int _selected;

        std::vector<View> _views;
    };
}
