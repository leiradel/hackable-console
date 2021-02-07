#include "Z80.h"

void hc::z80_info(uint64_t address, Memory const* memory, uint8_t* length, uint8_t* cycles, FlagState flags[8]) {
    // 64 is for DJNZ: 13 when it jumps, 8 when it doesn't.
    // 65 is for JR cc: 12 when it jumps, 7 when it doesn't.
    // 66 is for RET cc: 11 when it returns, 5 when it doesn't.
    // 67 is for CALL cc: 17 when it jumps, 10 when it doesn't.
    // 68 is for block transfers: 21 when it repeats, 16 when it doesn't.
    static uint8_t const cycles_main[256] = {
         4, 10,  7,  6,  4,  4,  7,  4,  4, 11,  7,  6,  4,  4,  7,  4,
        64, 10,  7,  6,  4,  4,  7,  4, 12, 11,  7,  6,  4,  4,  7,  4,
        65, 10, 16,  6,  4,  4,  7,  4, 65, 11, 16,  6,  4,  4,  7,  4,
        65, 10, 13,  6, 11, 11, 10,  4, 65, 11, 13,  6,  4,  4,  7,  4,
         4,  4,  4,  4,  4,  4,  7,  4,  4,  4,  4,  4,  4,  4,  7,  4,
         4,  4,  4,  4,  4,  4,  7,  4,  4,  4,  4,  4,  4,  4,  7,  4,
         4,  4,  4,  4,  4,  4,  7,  4,  4,  4,  4,  4,  4,  4,  7,  4,
         7,  7,  7,  7,  7,  7,  4,  7,  4,  4,  4,  4,  4,  4,  7,  4,
         4,  4,  4,  4,  4,  4,  7,  4,  4,  4,  4,  4,  4,  4,  7,  4,
         4,  4,  4,  4,  4,  4,  7,  4,  4,  4,  4,  4,  4,  4,  7,  4,
         4,  4,  4,  4,  4,  4,  7,  4,  4,  4,  4,  4,  4,  4,  7,  4,
         4,  4,  4,  4,  4,  4,  7,  4,  4,  4,  4,  4,  4,  4,  7,  4,
        66, 10, 10, 10, 67, 11,  7, 11, 66, 10, 10,  0, 67, 17,  7, 11,
        66, 10, 10, 11, 67, 11,  7, 11, 66,  4, 10, 11, 67,  0,  7, 11,
        66, 10, 10, 19, 67, 11,  7, 11, 66,  4, 10,  4, 67,  0,  7, 11,
        66, 10, 10,  4, 67, 11,  7, 11, 66,  6, 10,  4, 67,  0,  7, 11,
    };

    static uint8_t const cycles_ed[128] = {
        12, 12, 15, 20,  8, 14,  8,  9, 12,  8, 15,  8,  8, 14,  8,  9,
        12,  8, 15,  8,  8, 14,  8,  9, 12,  8, 15,  8,  8, 14,  8,  9,
        12,  8, 15,  8,  8, 14,  8, 18, 12,  8, 15,  8,  8, 14,  8, 18,
        12,  8, 15,  8,  8, 14,  8,  8, 12,  8, 15,  8,  8, 14,  8,  8,
         8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,
         8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,
        16, 16, 16, 16,  8,  8,  8,  8, 16, 16, 16, 16,  8,  8,  8,  8,
        68, 68, 68, 68,  8,  8,  8,  8, 68, 68, 68, 68,  8,  8,  8,  8,
    };

    static uint8_t const cycles_ddfd[256] = {
         0,  0,  0,  0,  0,  0,  0,  0,  0, 15,  0,  0,  0,  0,  0,  0,
         0,  0,  0,  0,  0,  0,  0,  0,  0, 15,  0,  0,  0,  0,  0,  0,
         0, 14, 20, 10,  8,  8, 11,  0,  0, 15, 20, 10,  8,  8, 11,  0,
         0,  0,  0,  0, 23, 23, 19,  0,  0, 15,  0,  0,  0,  0,  0,  0,
         0,  0,  0,  0,  8,  8, 19,  0,  0,  0,  0,  0,  8,  8, 19,  0,
         0,  0,  0,  0,  8,  8, 19,  0,  0,  0,  0,  0,  8,  8, 19,  0,
         0,  0,  0,  0,  8,  8, 19,  0,  0,  0,  0,  0,  8,  8, 19,  0,
        19, 19, 19, 19, 19, 19,  0, 19,  0,  0,  0,  0,  8,  8, 19,  0,
         0,  0,  0,  0,  8,  8, 19,  0,  0,  0,  0,  0,  8,  8, 19,  0,
         0,  0,  0,  0,  8,  8, 19,  0,  0,  0,  0,  0,  8,  8, 19,  0,
         0,  0,  0,  0,  8,  8, 19,  0,  0,  0,  0,  0,  8,  8, 19,  0,
         0,  0,  0,  0,  8,  8, 19,  0,  0,  0,  0,  0,  8,  8, 19,  0,
         0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
         0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
         0, 14,  0, 23,  0, 15,  0,  0,  0,  8,  0,  0,  0,  0,  0,  0,
         0,  0,  0,  0,  0,  0,  0,  0,  0, 10,  0,  0,  0,  0,  0,  0,
    };

    // Two bits per flag, in the following order (MSB to LSB): SZ5H3PNV
    // 0b00: flag is unchanged
    // 0b01: flag is set
    // 0b10: flag is reset
    // 0b11: flag is changed depending on the CPU state
    static uint16_t const flags_main[256] = {
        0x0000, 0x0000, 0x0000, 0x0000, 0xfffc, 0xfffc, 0x0000, 0x0ecb,
        0xffff, 0x0fcb, 0x0000, 0x0000, 0xfffc, 0xfffc, 0x0000, 0x0ecb,
        0x0000, 0x0000, 0x0000, 0x0000, 0xfffc, 0xfffc, 0x0000, 0x0ecb,
        0x0000, 0x0fcb, 0x0000, 0x0000, 0xfffc, 0xfffc, 0x0000, 0x0ecb,
        0x0000, 0x0000, 0x0000, 0x0000, 0xfffc, 0xfffc, 0x0000, 0xfff3,
        0x0000, 0x0fcb, 0x0000, 0x0000, 0xfffc, 0xfffc, 0x0000, 0x0dc4,
        0x0000, 0x0000, 0x0000, 0x0000, 0xfffc, 0xfffc, 0x0000, 0x0ec9,
        0x0000, 0x0fcb, 0x0000, 0x0000, 0xfffc, 0xfffc, 0x0000, 0x0fcb,
        0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
        0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
        0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
        0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
        0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
        0xfdfa, 0xfdfa, 0xfdfa, 0xfdfa, 0xfdfa, 0xfdfa, 0xfdfa, 0xfdfa,
        0xfefa, 0xfefa, 0xfefa, 0xfefa, 0xfefa, 0xfefa, 0xfefa, 0xfefa,
        0xfefa, 0xfefa, 0xfefa, 0xfefa, 0xfefa, 0xfefa, 0xfefa, 0xfefa,
        0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
        0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xffff, 0x0000,
        0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xffff, 0x0000,
        0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xffff, 0x0000,
        0x0000, 0x0000, 0x0000, 0xfef8, 0x0000, 0x0000, 0xffff, 0x0000,
        0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xfdfa, 0x0000,
        0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xfefa, 0x0000,
        0x0000, 0xffff, 0x0000, 0x0000, 0x0000, 0x0000, 0xfefa, 0x0000,
        0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xffff, 0x0000,
    };

    static uint16_t const flags_ed[128] = {
        0xfef8, 0x0000, 0xffff, 0x0000, 0xfff7, 0x0000, 0x0000, 0x0000,
        0xfef8, 0x0000, 0xffff, 0x0000, 0xfff7, 0x0000, 0x0000, 0x0000,
        0xfef8, 0x0000, 0xffff, 0x0000, 0xfff7, 0x0000, 0x0000, 0xfef8,
        0xfef8, 0x0000, 0xffff, 0x0000, 0xfff7, 0x0000, 0x0000, 0xfef8,
        0xfef8, 0x0000, 0xffff, 0x0000, 0xfff7, 0x0000, 0x0000, 0xfef8,
        0xfef8, 0x0000, 0xffff, 0x0000, 0xfff7, 0x0000, 0x0000, 0xfef8,
        0xfef8, 0x0000, 0xffff, 0x0000, 0xfff7, 0x0000, 0x0000, 0x0000,
        0xfef8, 0x0000, 0xffff, 0x0000, 0xfff7, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
        0x0ef8, 0xfff4, 0xffff, 0xffff, 0x0000, 0x0000, 0x0000, 0x0000,
        0x0ef8, 0xfff4, 0xffff, 0xffff, 0x0000, 0x0000, 0x0000, 0x0000,
        0x0ef8, 0xfff4, 0xffff, 0xffff, 0x0000, 0x0000, 0x0000, 0x0000,
        0x0ef8, 0xfff4, 0xffff, 0xffff, 0x0000, 0x0000, 0x0000, 0x0000,
    };

    uint16_t f = 0;

    uint8_t const op0 = memory->peek(address);

    if (op0 == 0xcb) {
        // Bit instructions, all shift and rotate instructions affect the flags
        // in the same way. Same thing for all bit test instructions. Set and
        // reset instructions don't affect the flags.
        uint8_t const op1 = memory->peek(address + 1);
        f = op1 < 0x40 ? 0xfefb : op1 < 0x80 ? 0xfdf8 : 0x0000;
    }
    else if (op0 == 0xed) {
        // Valid extended instructions change the flags according to the
        // flags_ed table. Invalid ones are NOPs and don't change the flags.
        uint8_t const op1 = memory->peek(address + 1) - 0x40;
        f = (op1 < 0x80) ? flags_ed[op1] : 0x0000;
    }
    else if (op0 == 0xdd || op0 == 0xfd) {
        // IX or IY.
        uint8_t const op1 = memory->peek(address + 1);

        if (op1 == 0xcb) {
            // The index prefix don't change how flags are affected by the bit
            // instructions.
            uint8_t const op3 = memory->peek(address + 3);
            f = op3 < 0x40 ? 0xfefb : op3 < 0x80 ? 0xfdf8 : 0x0000;
        }
        else if (op1 == 0xed || op1 == 0xdd || op1 == 0xfd) {
            // A prefix followed by ED or by another prefix executes as a NOP,
            // which doesn't affect the flags.
            f = 0x0000;
        }
        else if (cycles_ddfd[op1] == 0) {
            // A prefix followed by an instruction that doesn't use IX or IY
            // is also executed as a NOP.
            f = 0x0000;
        }
        else {
            // Use the flags_main table since the prefix will affect the flags
            // in the same way as the unprefixed instructions.
            f = flags_main[op1];
        }
    }
    else {
        // Use the flags_main table directly.
        f = flags_main[op0];
    }

    flags[0] = static_cast<FlagState>((f >>  0) & 3);
    flags[1] = static_cast<FlagState>((f >>  2) & 3);
    flags[2] = static_cast<FlagState>((f >>  4) & 3);
    flags[3] = static_cast<FlagState>((f >>  6) & 3);
    flags[4] = static_cast<FlagState>((f >>  8) & 3);
    flags[5] = static_cast<FlagState>((f >> 10) & 3);
    flags[6] = static_cast<FlagState>((f >> 12) & 3);
    flags[7] = static_cast<FlagState>((f >> 14) & 3);

    if (op0 == 0xcb) {
        // Bit instructions.
        uint8_t const op1 = memory->peek(address + 1) & 0xc7;

        if (op1 == 0x06 || op1 == 0x86 || op1 == 0xc6) {
            // Shift, rotate, set, and reset instructions that use (HL) take
            // 15 cycles.
            *cycles = 15;
        } else if (op1 == 0x46) {
            // Bit test instructions that use (HL) take 12 cycles.
            *cycles = 12;
        } else {
            // All other bit instructions take 8 cycles.
            *cycles = 8;
        }
    }
    else if (op0 == 0xed) {
        // Valid extended instructions consume cycles according to the
        // cycles_ed table. Invalid ones are NOPs that take 8 cycles.
        uint8_t const op1 = memory->peek(address + 1) - 0x40;
        *cycles = (op1 < 0x80) ? cycles_ed[op1] : 8;
    }
    else if (op0 == 0xdd || op0 == 0xfd) {
        // IX or IY.
        uint8_t const op1 = memory->peek(address + 1);

        if (op1 == 0xcb) {
            // Bit instructions.
            uint8_t const op3 = memory->peek(address + 3) & 0xc0;

            if (op3 == 0x40) {
                // Bit test instructions take 20 cycles.
                *cycles = 20;
            } else {
                // All other bit instructions take 23 cycles.
                *cycles = 23;
            }
        }
        else if (op1 == 0xed || op1 == 0xdd || op1 == 0xfd) {
            // A prefix followed by ED or by another prefix executes as a NOP,
            // which takes 4 cycles.
            *cycles = 4;
        }
        else if (cycles_ddfd[op1] == 0) {
            // A prefix followed by an instruction that doesn't use IX or IY
            // is also executed as a NOP.
            *cycles = 4;
        }
        else {
            // Use the cycles_ddfd table for valid uses of the prefix.
            *cycles = cycles_ddfd[op1];
        }
    }
    else {
        // Use the cycles_main table directly.
        *cycles = cycles_main[op0];
    }
}

void hc::z80_disasm(uint64_t const address, Handle<Memory*> const memory, char* const buffer, size_t size);
