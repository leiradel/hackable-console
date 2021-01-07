#pragma once

#include "Logger.h"
#include "LifeCycle.h"

#include <string>
#include <set>

namespace hc {
    class Control {
    public:
        Control() : _logger(nullptr), _selected(0) {}

        bool init(LifeCycle* fsm, Logger* logger);
        void destroy();
        void reset();
        void draw();

        void setSystemInfo(retro_system_info const* info);
        void addConsole(char const* const name);

    protected:
        LifeCycle* _fsm;
        Logger* _logger;

        std::set<std::string> _consoles;
        int _selected;
        std::string _extensions;
        std::string _lastGameFolder;
    };
}
