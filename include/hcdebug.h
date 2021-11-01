#ifndef HC_DEBUG__
#define HC_DEBUG__

#include <stdint.h>

#define HC_API_VERSION 1

/* Watchpoint operations */
#define HC_MEMORY_READ (1 << 0)
#define HC_MEMORY_WRITE (1 << 1)

/* IO watchpoint operations */
#define HC_IO_READ (1 << 0)
#define HC_IO_WRITE (1 << 1)

/* Event types */
#define HC_EVENT_TICK 0
#define HC_EVENT_EXECUTION 1
#define HC_EVENT_INTERRUPT 2
#define HC_EVENT_MEMORY 3
#define HC_EVENT_REG 4
#define HC_EVENT_IO 5
#define HC_EVENT_GENERIC 6

typedef struct hc_Breakpoint {
    struct {
        /* Breakpoint info */
        char const* description;
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

        /* Supported breakpoints not covered by specific functions */
        hc_Breakpoint const* const* break_points;
        unsigned num_break_points;

        /* Reads a byte from an address */
        uint8_t (*peek)(uint64_t address);

        /*
        poke can be null for read-only memory but all memory should be writeable to allow patching. poke can be non-null and still
        don't change the value, i.e. for the main memory region when the address is in ROM. If poke succeeds to write to the given
        address, it returns a value different from 0 (true).
        */
        int (*poke)(uint64_t address, uint8_t value);
    }
    v1;
}
hc_Memory;

typedef struct hc_Cpu {
    struct {
        /* CPU info */
        char const* id;
        char const* description;
        unsigned type;
        int is_main; /* only one CPU can be the main CPU */

        /* Memory region that is CPU addressable */
        hc_Memory const* memory_region;

        /* Supported breakpoints not covered by specific functions */
        hc_Breakpoint const* const* break_points;
        unsigned num_break_points;

        /* Registers, return true on set_register to signal a successful write */
        uint64_t (*get_register)(unsigned reg);
        int (*set_register)(unsigned reg, uint64_t value);
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
    }
    v1;
}
hc_System;

/* Informs the front-end that a CPU is about to execute an instruction at the given address */
typedef struct hc_Execution {
    hc_Cpu const* cpu;
    uint64_t address;
}
hc_ExecutionBreakpoint;

/* Informs the front-end that an interrupt was served */
typedef struct hc_Interrupt {
    hc_Cpu const* cpu;
    unsigned kind;
    uint64_t address;
}
hc_InterruptBreakpoint;

/* Informs the front-end that a memory location is about to be read from or written to */
typedef struct hc_MemoryWatchpoint {
    hc_Memory const* memory;
    uint64_t address;
    unsigned operation;
    uint8_t new_value;
}
hc_MemoryWatchpoint;

/* Informs the front-end that a register is about to have its value changed */
typedef struct hc_RegisterWatchpoint {
    hc_Cpu const* cpu;
    unsigned reg;
    uint64_t new_value;
}
hc_RegisterWatchpoint;

/* Informs the front-end that an IO port is about to be read from or written to */
typedef struct hc_IoWatchpoint {
    hc_Cpu const* cpu;
    uint64_t address;
    unsigned operation;
    uint64_t value;
}
hc_IoWatchpoint;

/* Informs the front-end that a generic breakpoint was hit */
typedef struct hc_GenericBreakpoint {
    hc_Breakpoint const* breakpoint;
    uint64_t args[4];
}
hc_GenericBreakpoint;

typedef struct hc_Event {
    unsigned type;
    void* user_data;

    union {
        hc_ExecutionBreakpoint execution;
        hc_InterruptBreakpoint interrupt;
        hc_MemoryWatchpoint memory;
        hc_RegisterWatchpoint reg;
        hc_IoWatchpoint io;
        hc_GenericBreakpoint generic;
    }
    event;
}
hc_Event;

typedef struct hc_DebuggerIf {
    unsigned const frontend_api_version;
    unsigned core_api_version;

    struct {
        /* The emulated system */
        hc_System const* system;

        /* A front-end user-defined data */
        void* const user_data;

        /* Handles an event from the core */
        void (* const handle_event)(void* frontend_user_data, hc_Event const* event);
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
