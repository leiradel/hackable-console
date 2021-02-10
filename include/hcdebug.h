#ifndef HC_DEBUG__
#define HC_DEBUG__

#include <stdarg.h>
#include <stdint.h>

typedef struct hc_DebuggerIf hc_DebuggerIf;

typedef enum {
    HC_SIZE_1 = 0,
    HC_SIZE_2 = 1,
    HC_SIZE_4 = 2,
    HC_SIZE_8 = 3,
    HC_SIZE_MASK = 0xff,
    HC_PROGRAM_COUNTER = 1 << 8,
    HC_STACK_POINTER = 1 << 9,
    HC_MEMORY_POINTER = 1 << 10
}
hc_RegisterFlags;

typedef struct {
    struct {
        char const* name;
        uint64_t flags;
        uint64_t (*get)(void* user_data);

        /* set can be null if the register can't be changed or if it doesn't make sense to do so */ 
        void (*set)(void* user_data, uint64_t value);

        char const* const* bits;
    }
    v1;
}
hc_Register;

typedef enum {
    HC_ALIGNMENT_1 = 0,
    HC_ALIGNMENT_2 = 1,
    HC_ALIGNMENT_4 = 2,
    HC_ALIGNMENT_8 = 3,
    HC_ALIGNMENT_MASK = 0xff,
    HC_CPU_ADDRESSABLE = 1 << 8
}
hc_MemoryFlags;

typedef struct {
    struct {
        char const* description;
        uint64_t flags;
        uint64_t base_address;
        uint64_t size;
        uint8_t (*peek)(void* user_data, uint64_t address);

        /* poke can be null for read-only memory but all memory should be writeable to allow patching */
        /* poke can be non-null and still don't change the value, i.e. for the main memory region when the address is in rom */
        void (*poke)(void* user_data, uint64_t address, uint8_t value);

        /* set_watch_point can be null when not supported */
        unsigned (*set_watch_point)(void* user_data, uint64_t address, uint64_t length, int read, int write);
    }
    v1;
}
hc_Memory;

typedef enum {
    HC_Z80
}
hc_CpuType;

typedef enum {
    HC_CPU_MAIN = 1 << 0
}
hc_CpuFlags;

typedef struct {
    struct {
        hc_CpuType type;
        char const* description;
        uint64_t flags;
        hc_Register const* const* registers;
        unsigned num_registers;
        hc_Memory const* const* memory_regions;
        unsigned num_memory_regions;

        /* any one of these can be null if the cpu doesn't support the functionality */
        void (*pause)(void* user_data);
        void (*resume)(void* user_data);
        void (*step_into)(void* user_data);
        void (*step_over)(void* user_data);
        void (*step_out)(void* user_data);

        /* set_break_point can be null when not supported */
        unsigned (*set_break_point)(void* user_data, uint64_t address);
    }
    v1;
}
hc_Cpu;

typedef struct {
    struct {
        char const* description;
        hc_Cpu const* const* cpus;
        unsigned num_cpus;
        hc_Register const* const* registers;
        unsigned num_registers;
        hc_Memory const* const* memory_regions;
        unsigned num_memory_regions;
    }
    v1;
}
hc_System;

struct hc_DebuggerIf {
    unsigned const version;

    struct {
        hc_System const* system;
    }
    v1;
};

typedef void* (*hc_Set)(hc_DebuggerIf* const debugger_if);

#endif /* HC_DEBUG__ */
