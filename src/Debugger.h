#pragma once

#include "Desktop.h"
#include "Config.h"

extern "C" {
    #include "hcdebug.h"
}

namespace hc {
    class Debugger : public View {
    public:
        Debugger(Desktop* desktop) : View(desktop), _debuggerIf(nullptr), _selectedCpu(0) {}
        virtual ~Debugger() {}

        void init(Config* config);

        virtual char const* getTitle() override;
        virtual void onGameLoaded() override;
        virtual void onGameUnloaded() override;
        virtual void onDraw() override;

    protected:
        Config* _config;
        hc_DebuggerIf* _debuggerIf;
        int _selectedCpu;
    };
}
