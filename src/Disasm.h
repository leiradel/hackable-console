#pragma once

#include "Desktop.h"
#include "Handle.h"

namespace hc {
    class Disasm : public View {
    public:
        Disasm(Desktop* desktop, Handle<Memory*> memory, Handle<Register*> reg)
            : View(desktop)
            , _memory(memory)
            , _register(reg)
        {}

        // hc::View
        virtual char const* getTitle() override;
        virtual void onDraw() override;

    protected:
        Handle<Memory*> _memory;
        Handle<Register*> _register;
    };
}
