#ifndef HC_DEBUG__
#define HC_DEBUG__

#include <stdint.h>

#define HC_API_VERSION 1

/* Watchpoint events */
#define HC_MEMORY_READ (1 << 0)
#define HC_MEMORY_WRITE (1 << 1)

/* IO watchpoint events */
#define HC_IO_READ (1 << 0)
#define HC_IO_WRITE (1 << 1)

typedef struct hc_Breakpoint {
    struct {
        /* Breakpoint info */
        char const* description;

        /* Enable this breakpoint */
        unsigned (*enable)(void);
    }
    v1;
}
hc_Breakpoint;

typedef struct hc_Memory {
    struct {
        /* Memory info */
        char const* id;
        char const* description;
        unsigned alignment; /* in bytes */
        uint64_t base_address;
        uint64_t size;
        uint8_t (*peek)(uint64_t address);

        /*
        poke can be null for read-only memory but all memory should be writeable to allow patching. poke can be non-null and still
        don't change the value, i.e. for the main memory region when the address is in ROM. If poke succeeds to write to the given
        address, it returns a value different from 0 (true).
        */
        int (*poke)(uint64_t address, uint8_t value);

        /* set_watch_point can be null when not supported; event is HC_MEMORY_READ or HC_MEMORY_WRITE or both or'ed together */
        unsigned (*set_watchpoint)(uint64_t address, uint64_t length, unsigned event);

        /* Supported breakpoints not covered by specific functions */
        hc_Breakpoint const* const* break_points;
        unsigned num_break_points;
    }
    v1;
}
hc_Memory;

typedef struct hc_Cpu {
    struct {
        /* CPU info */
        char const* description;
        unsigned type;
        int is_main; /* only one CPU can be the main CPU */

        /* Memory region that is CPU addressable */
        hc_Memory const* memory_region;

        /* Registers */
        uint64_t (*get_register)(unsigned reg);
        void (*set_register)(unsigned reg, uint64_t value);
        int (*set_reg_watchpoint)(unsigned reg);

        /* Any one of these can be null if the cpu doesn't support the functionality */
        void (*step_into)(void); /* step_into is also used to step a single instruction */
        void (*step_over)(void);
        void (*step_out)(void);

        /* set_break_point can be null when not supported */
        unsigned (*set_exec_breakpoint)(uint64_t address);

        /*
        Breaks on read and writes to the input/output address space. event is HC_IO_READ or HC_IO_WRITE or both or'ed together.
        set_io_watchpoint can be null when not supported.
        */
        unsigned (*set_io_watchpoint)(uint64_t address, uint64_t length, unsigned event);

        /* Breaks when an interrupt occurs; type is the particular interrupt type i.e. HC_Z80_NMI */
        unsigned (*set_int_breakpoint)(unsigned type);

        /* Supported breakpoints not covered by specific functions */
        hc_Breakpoint const* const* break_points;
        unsigned num_break_points;
    }
    v1;
}
hc_Cpu;

typedef struct hc_System {
    struct {
        /* System info */
        char const* description;

        /* CPUs available in the system */
        hc_Cpu const* const* cpus;
        unsigned num_cpus;

        /* Memory regions that aren't addressable by any of the CPUs on the system */
        hc_Memory const* const* memory_regions;
        unsigned num_memory_regions;

        /* Supported breakpoints not covered by specific functions */
        hc_Breakpoint const* const* break_points;
        unsigned num_break_points;

        /* Removes a breakpoint or watchpoint */
        void (*remove_breakpoint)(unsigned id);
    }
    v1;
}
hc_System;

typedef struct hc_DebuggerIf {
    unsigned const frontend_api_version;
    unsigned core_api_version;

    /* The emulated system */
    hc_System const* system;

    /* A front-end user-defined data */
    void* const user_data;

    struct {
        /*
        This callback should be called from all threads in the core that are related to the emulation, from their innermost
        loops like the CPU core emulator loop. This function will block the thread, as guarantees that the entire emulation enters
        a pause state when the front-end needs to pause it.
        */
        void (* const tick_cb)(void* ud, uint64_t thread_id);

        /* Informs the front-end that a watchpoint was triggered */
        void (* const mem_watchpoint_cb)(void* ud, unsigned id, hc_Memory const* memory, uint64_t address, unsigned event);

        /* Informs the front-end that a register had its value changed */
        void (* const reg_watchpoint_cb)(void* ud, unsigned id, hc_Cpu const* cpu, unsigned reg, uint64_t old_value);

        /* Informs the front-end that a breakpoint occurred */
        void (* const exec_breakpoint_cb)(void* ud, unsigned id, hc_Cpu const* cpu, uint64_t address);

        /* Informs the front-end that an IO port was accessed */
        void (* const io_watchpoint_cb)(void* ud, unsigned id, hc_Cpu const* cpu, uint64_t address, unsigned event, uint64_t value);

        /* Informs the front-end that an interrupt was served */
        void (* const int_breakpoint_cb)(void* ud, unsigned id, hc_Cpu const* cpu, unsigned type, uint64_t address);

        /* Informs the front-end that a generic breakpoint was hit */
        void (* const gen_breakpoint_cb)(void* ud, unsigned id, hc_Breakpoint const* break_point, uint64_t arg1, uint64_t arg2);
    }
    v1;
}
hc_DebuggerIf;

typedef void (*hc_Set)(hc_DebuggerIf* const debugger_if);

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
