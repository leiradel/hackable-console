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
typedef enum {
    HC_EVENT_TICK = 0,
    HC_EVENT_EXECUTION = 1,
    HC_EVENT_RETURN = 2,
    HC_EVENT_INTERRUPT = 3,
    HC_EVENT_MEMORY = 4,
    HC_EVENT_REG = 5,
    HC_EVENT_IO = 6,
    HC_EVENT_GENERIC = 7
}
hc_EventType;

/* Subscription ID. Helps identify subscriber, and also allows unsubscribing from an event. A negative ID indicates an error.
   IDs are not necessarily consecutive, and an ID may be re-used only after unsubscribing. Otherwise, IDs are unique, even
   between different event types. (The core might implement this by using some bits of the event ID to indicate the event type.) */
typedef int64_t hc_SubscriptionID;

typedef enum {
    /* Report all execution events */
    HC_STEP,
    
    /* As above, but if an interrupt occurs, temporarily disable until returned from interrupt */
    HC_STEP_SKIP_INTERRUPT,
    
    /* As above, but if a subroutine is invoked, temporarily disable until returned from subroutine */
    HC_STEP_CURRENT_SUBROUTINE,
}
hc_ExecutionType;

typedef struct hc_GenericBreakpoint {
    struct {
        /* Breakpoint info */
        char const* description;
    }
    v1;
}
hc_GenericBreakpoint;

typedef struct hc_Memory {
    struct {
        /* Memory info */
        char const* id;
        char const* description;
        unsigned alignment; /* in bytes */
        uint64_t base_address;
        uint64_t size;

        /* Supported breakpoints not covered by specific functions */
        hc_GenericBreakpoint const* const* break_points;
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
        hc_GenericBreakpoint const* const* break_points;
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
        hc_GenericBreakpoint const* const* break_points;
        unsigned num_break_points;
    }
    v1;
}
hc_System;

/* Informs the front-end that a CPU is about to execute an instruction at the given address */
typedef struct hc_ExecutionEvent {
    hc_Cpu const* cpu;
    uint64_t address;
}
hc_ExecutionEvent;

/* Informs the front-end that a CPU has just returned from a subroutine */
typedef struct hc_ExecutionReturnEvent {
    hc_Cpu const* cpu;
    
    /* address of the return instruction */
    uint64_t previous_address;
    
    /* address returned to */
    uint64_t return_address;
}
hc_ExecutionReturnEvent;

/* Informs the front-end that an interrupt was served */
typedef struct hc_InterruptEvent {
    hc_Cpu const* cpu;
    
    /* Identifies the type of interrupt. Meaning depends on CPU model */
    unsigned kind;
    
    /* Address of the next instruction to be executed when returning from interrupt */
    uint64_t return_address;
    
    /* New value of the program counter (in general, the start of the interrupt vector) */
    uint64_t vector_address;
}
hc_InterruptEvent;

/* Informs the front-end that a memory location is about to be read from or written to */
typedef struct hc_MemoryWatchpointEvent {
    hc_Memory const* memory;
    uint64_t address;
    uint8_t operation;
    uint8_t value;
}
hc_MemoryWatchpoint;

/* Informs the front-end that a register is about to have its value changed */
typedef struct hc_RegisterWatchpointEvent {
    hc_Cpu const* cpu;
    unsigned reg;
    uint64_t new_value;
}
hc_RegisterWatchpoint;

/* Informs the front-end that an IO port is about to be read from or written to */
typedef struct hc_IoWatchpointEvent {
    hc_Cpu const* cpu;
    uint64_t address;
    uint8_t operation;
    uint64_t value;
}
hc_IoWatchpoint;

/* Informs the front-end that a generic breakpoint was hit */
typedef struct hc_GenericBreakpointEvent {
    hc_GenericBreakpoint const* breakpoint;
    uint64_t args[4];
}
hc_Breakpoint;

/* Tagged union over all hc Event types */
typedef struct hc_Event {
    hc_EventType type;
    hc_SubscriptionID subscribtion_id;

    union {
        hc_ExecutionEvent execution;
        hc_ExecutionReturnEvent execution_return;
        hc_InterruptEvent interrupt;
        hc_MemoryWatchpointEvent memory;
        hc_RegisterWatchpointEvent reg;
        hc_IoWatchpointEvent io;
        hc_GenericBreakpointEvent generic;
    }
    event;
}
hc_Event;

/* Tells the core to report certain execution events. Note that the core should implicitly include the
   current stack depth and/or subroutine being executed in the context of this subscription */
typedef struct hc_ExecutionSubscription {
    hc_Cpu const* cpu;
    hc_ExecutionType type;
    uint64_t address_range_begin;
    uint64_t address_range_end;
}
hc_ExecutionSubscription;

/* Tells the core to report after returning from the current subroutine.
   Note:
    - The core should implicitly include the current stack depth and/or subroutine
      being executed in the context of this subscription.
    - This event is only reported once per subscription, so the front-end should
      immediately unsubscribe any time this event is reported.
*/
typedef struct hc_ExecutionReturnSubscription {
    hc_Cpu const* cpu;
}
hc_ExecutionReturnSubscription;

/* Tells the core to report certain interrupt events */
typedef struct hc_InterruptSubscription {
    hc_Cpu const* cpu;
    unsigned kind;
}
hc_InterruptSubscription;

/* Tells the core to report certain memory access events */
typedef struct hc_MemoryWatchpointSubscription {
    hc_Memory const* memory;
    uint64_t address_range_begin;
    uint64_t address_range_end;
    uint8_t operation;
}
hc_MemoryWatchpointSubscription;

/* Tells the core to report certain register change events */
typedef struct hc_RegisterWatchpointSubscription {
    hc_Cpu const cpu;
    unsigned reg;
}
hc_RegisterWatchpointSubscription;

/* Tells the core to report when an IO port is accessed  */
typedef struct hc_IoWatchpointSubscription {
    hc_Cpu const* cpu;
    uint64_t address_range_begin;
    uint64_t address_range_end;
    uint8_t operation;
}
hc_IoWatchpointSubscription;

/* Tells the core to report a generic breakpoint event */
typedef struct hc_GenericBreakpointSubscription {
    hc_GenericBreakpoint const* breakpoint;
}
hc_GenericBreakpointSubscription;

/* Informs the core that a particular type of event should be reported (via handle_event()) */
typedef struct hc_Subscription {
    hc_EventType type;
    
    union {
        hc_ExecutionSubscription execution;
        hc_ExecutionReturnSubscription execution_return;
        hc_InterruptSubscription interrupt;
        hc_MemoryWatchpointSubscription memory;
        hc_RegisterWatchpointSubscription reg;
        hc_IoWatchpointSubscription io;
        hc_GenericBreakpointSubscription generic;
    }
    subscription;
}
hc_Subscription;

/* Debug interface. Shared between the core and frontend. Members which are const are initialized by the frontend */
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
        
        /* Tells the core to report certain events. Returns negative value if not supported or if an error occurred */
        hc_SubscriptionID (* subscribe)(hc_Subscription);
        void (* unsubscribe)(hc_SubscriptionID);
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

#define HC_CPU_65816 HC_MAKE_CPU_TYPE(2, 1)

#define HC_65816_A 0
#define HC_65816_X 1
#define HC_65816_Y 2
#define HC_65816_S 3
#define HC_65816_PC 4
#define HC_65816_P 5
#define HC_65816_DB 6
#define HC_65816_D 7
#define HC_65816_PB 8
#define HC_65816_EMU 9 /* 'hidden' 1-bit register, set to 1 in emulation mode, 0 in native mode */

#define HC_65816_NUM_REGISTERS 10

#endif /* HC_DEBUG__ */
