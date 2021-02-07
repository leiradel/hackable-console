#include "Cpu.h"

#define CHIPS_IMPL
#include <z80dasm.h>

unsigned hc::disasm_z80(uint64_t const address, Peek const peek, void* const userdata, char* const buffer, size_t size) {
    struct Userdata {
        Peek peek;
        uint64_t address;
        void* userdata;

        char* buffer;
        size_t position;
        size_t size;
    };

    static auto const inCallback = [](void* user_data) -> uint8_t {
        auto const ud = static_cast<Userdata const*>(user_data);
        return ud->peek(ud->address, ud->userdata);
    };

    static auto const outCallback = [](char c, void* user_data) -> void {
        auto const ud = static_cast<Userdata*>(user_data);

        if (ud->position < ud->size - 1) {
            ud->buffer[ud->position++] = c;
        }
    };

    Userdata ud = {peek, address, userdata, buffer, 0, size};
    unsigned const length = z80dasm_op(static_cast<uint16_t>(address), inCallback, outCallback, &ud);

    ud.buffer[ud.position] = 0;
    return length;
}
