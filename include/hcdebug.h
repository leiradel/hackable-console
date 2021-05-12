#ifndef HC_DEBUG__
#define HC_DEBUG__

#include <stdint.h>

#define HC_API_VERSION 1

typedef struct {
    struct {
        char const* description;
        unsigned (*enable)(void* ud, int yes);
    }
    v1;
}
hc_Breakpoint;

typedef struct {
    struct {
        char const* id;
        char const* description;
        unsigned alignment; /* in bytes */
        uint64_t base_address;
        uint64_t size;
        uint8_t (*peek)(void* ud, uint64_t address);

        /* poke can be null for read-only memory but all memory should be writeable to allow patching */
        /* poke can be non-null and still don't change the value, i.e. for the main memory region when the address is in rom */
        void (*poke)(void* ud, uint64_t address, uint8_t value);

        /* set_watch_point can be null when not supported */
        unsigned (*set_watchpoint)(void* ud, uint64_t address, uint64_t length, int read, int write);

        /* Supported breakpoints not covered by specific functions */
        hc_Breakpoint const* const* break_points;
        unsigned num_break_points;
    }
    v1;
}
hc_Memory;

typedef struct {
    struct {
        /* CPU info */
        char const* description;
        unsigned type;
        int is_main;

        /* Memory region that is CPU addressable */
        hc_Memory const* memory_region;

        /* Registers */
        uint64_t (*get_register)(void* ud, unsigned reg);
        void (*set_register)(void* ud, unsigned reg, uint64_t value);
        unsigned (*set_reg_breakpoint)(void* ud, unsigned reg);

        /* Any one of these can be null if the cpu doesn't support the functionality */
        void (*step_into)(void* ud); /* step_into is also used to step a single instruction */
        void (*step_over)(void* ud);
        void (*step_out)(void* ud);

        /* set_break_point can be null when not supported */
        unsigned (*set_exec_breakpoint)(void* ud, uint64_t address);

        /* Breaks on read and writes to the input/output address space */
        unsigned (*set_io_watchpoint)(void* ud, uint64_t address, uint64_t length, int read, int write);

        /* Breaks when an interrupt occurs */
        unsigned (*set_int_breakpoint)(void* ud, unsigned type);

        /* Supported breakpoints not covered by specific functions */
        hc_Breakpoint const* const* break_points;
        unsigned num_break_points;
    }
    v1;
}
hc_Cpu;

typedef struct {
    struct {
        char const* description;

        /* CPUs */
        hc_Cpu const* const* cpus;
        unsigned num_cpus;

        /* Memory regions that aren't addressable by any of the CPUs on the system */
        hc_Memory const* const* memory_regions;
        unsigned num_memory_regions;

        /* Supported breakpoints not covered by specific functions */
        hc_Breakpoint const* const* break_points;
        unsigned num_break_points;
    }
    v1;
}
hc_System;

typedef struct {
    unsigned const frontend_api_version;
    unsigned core_api_version;

    struct {
        /* Informs the front-end that a breakpoint occurred */
        void (* const breakpoint_cb)(unsigned id);

        /* The emulated system */
        hc_System const* system;
    }
    v1;
}
hc_DebuggerIf;

typedef void* (*hc_Set)(hc_DebuggerIf* const debugger_if);

#define HC_MAKE_CPU_TYPE(id, version) ((id) << 16 | (version))
#define HC_CPU_API_VERSION(type) ((type) & 0xffffU)

/* Supported CPUs in API version 1 */
#define HC_CPU_Z80 HC_MAKE_CPU_TYPE(0, 1)

#define HC_Z80_A 0
#define HC_Z80_F 1
#define HC_Z80_BC 2
#define HC_Z80_DE 3
#define HC_Z80_HL 4
#define HC_Z80_IX 5
#define HC_Z80_IY 6
#define HC_Z80_AF2 7
#define HC_Z80_BC2 8
#define HC_Z80_DE2 9
#define HC_Z80_HL2 10
#define HC_Z80_I 11
#define HC_Z80_R 12
#define HC_Z80_SP 13
#define HC_Z80_PC 14
#define HC_Z80_IFF 15
#define HC_Z80_IM 16
#define HC_Z80_WZ 17

#define HC_Z80_NUM_REGISTERS 18

#define HC_Z80_INT 0
#define HC_Z80_NMI 1

#define HC_CPU_6502 HC_MAKE_CPU_TYPE(1, 1)

#define HC_6502_A 0
#define HC_6502_X 1
#define HC_6502_Y 2
#define HC_6502_S 3
#define HC_6502_PC 4
#define HC_6502_P 5

#define HC_6502_NUM_REGISTERS 6

#endif /* HC_DEBUG__ */
