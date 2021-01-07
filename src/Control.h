#pragma once

#include "Logger.h"

#include <string>
#include <set>

namespace hc {
    class Control {
    public:
        Control() : _logger(nullptr), _selected(0) {}

        bool init(Logger* logger);
        void destroy();
        void reset();
        void draw();

        void addConsole(char const* const name);

    protected:
        Logger* _logger;

        std::set<std::string> _consoles;
        int _selected;
    };
}
