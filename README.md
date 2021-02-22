# Hackable Console

Hackable Console is a pretentious project to add debugging capabilities to [Libretro](https://www.libretro.com/index.php/api/) cores, and to create a front-end capable of debugging such cores.

![Hackable Console screenshot](https://raw.githubusercontent.com/leiradel/hackable-console/hc.png)

## Why Libretro

The Libretro API provides a common interface that can be used to write multi-media applications in a multi-platform way. Programs implementing the API as a producer of media are called Libretro cores, and programs that can run those cores are called front-ends.

By creating a Libretro front-end, it's possible to load and run most of the [50+ cores](https://docs.libretro.com/meta/core-list/) available. The API already supports extensions, so the debug API could be added to the cores, and queried by the front-end.

I believe that this will be much easier than trying to integrate debugging capabilities into stand-alone emulators.

## Goals

Hackable Console aims to:

1. Specify an debug API able to
    * Execute of instructions: step-by-step, step into, step out, step over
    * Stop execution at certain addresses or when memory is read or written
    * Visualize and change the internal CPU state: registers, flags, interruptions
    * Disassemble instructions
    * Visualize and change memory
    * Be versioned so it's easy to add new functionalities
1. Implement a Libretro front-end that can load and run Libretro cores, and that implements the debug API. In addition to the regular debugging capabilities provided by the debug API, the front-end also aims to:
    1. Show symbols instead of addresses where applicable
        * Pre-defined symbols available at startup, i.e. ROM routines
        * User-defined symbols
    1. Automatically show important memory regions, such as those pointed to by selected registers, i.e. BC, DE, HL, IX, and IY in the case of the Z80
    1. Automatically show the system stack, with symbol support
    1. A cheat engine capable of filtering memory and combining filter results
    1. Allow external plugins for specific consoles to be loaded and managed by the front-end
    1. Support scripts
    1. Snapshots
    1. Saving and loading of the entire front-end state to support long running reverse-engineering projects
1. Add the debug API to existing Libretro cores
    * To help the development of the debug API and the front-end, [zx48k-libretro](https://github.com/leiradel/zx48k-libretro), a core capable of running ZX Spectrum 48K programs and games, was developed using [chips](https://github.com/floooh/chips/)

However, as the project is just starting, the current goal is to implement something that works and that can be used to test the code design both for the debug API and the front-end.

## The API

The API consists of just one function that cores must support via `retro_get_proc_address_interface`: `hc_set_debuggger`. This function will receive a pointer to a `hc_Debugger` structure, which the core must then fill with the information necessary for the front-end to implement the debugging functionality.

```c
typedef void* (*hc_Set)(hc_DebuggerIf* const debugger_if);
```

> I'm a stronger supporter of the "east const" style, for reasons that are better explained elsewhere.

The API is written in C to allow its implementation in as many cores as possible, given the increased inter-operability provided by the language.

### hc_DebuggerIf

```c
typedef struct {
    unsigned const frontend_version;
    unsigned core_version;

    struct {
        hc_System const* system;
    }
    v1;
}
hc_DebuggerIf;
```

* `frontend_version` comes in filled by the front-end with the debug API version it supports when the `hc_set_debugger` implementation is called. Only data and functions available inside the structures which versions are less than or equal to `frontend_version` must be used.
* `core_version` must be filled by the core with the version that it supports. The front-end will never try to access data or functions that the core doesn't support.
* `v1.system`: a pointer to a `hc_System` structure that describes the system being emulated.

> It's likely that `hc_DebuggerIf` will be extended to allow more than one system per core, as it's common for emulators to support an entire family of consoles or computers.

### hc_System

`hc_System` and all other structures from now on must be filled by the core.

```c
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
```

* `v1.description`: The system name that the core emulates.
* `v1.cpus`: A list of CPUs present in the system. It's not mandatory that all CPUs that compose the system are available in the debug API.
* `v1.num_cpus`: The number of CPUs in the `cpus` list.
* `v1.registers`: System-wide registers deemed important for debugging, such as timers.
* `v1.num_registers`: The number of registers in the `registers` list.
* `v1.memory_regions`: A list of memory regions that are not attached to a specific CPU.
* `v1.num_memory_regions`: The number of memory regions in the `memory_regions` list.

> `memory_regions` here are used for blocks of memory that are not directly accessible by any of the CPUs, at list not in their entirety, like memory that is banked or only accessible via I/O.

### hc_Cpu

```c
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
        void (*step_into)(void* user_data);
        void (*step_over)(void* user_data);
        void (*step_out)(void* user_data);

        /* set_break_point can be null when not supported */
        unsigned (*set_break_point)(void* user_data, uint64_t address);
    }
    v1;
}
hc_Cpu;
```

* `v1.type`: The particular CPU model, so that the front-end knows how to disassemble instructions etc.
* `v1.description`: The CPU description, i.e. Zilog Z80.
* `v1.flags`: Flags for the CPU. Add `HC_CPU_MAIN` for the main system CPU.
* `v1.registers`: The list of registers present in the CPU. While this could be inferred from the CPU type, some CPU emulators may be able to emulate more registers than others. In the case of the Z80, some emulators can correctly emulate the [WZ](https://retrocomputing.stackexchange.com/questions/6671/what-are-the-registers-w-and-z-inside-a-z80) register.
* `v1.num_registers`: The number of registers in the `registers` list.
* `v1.memory_regions`: The memory regions accessible by the CPU.
* `v1.num_memory_regions`: The number of memory regions in the `memory_regions` list.
* `v1.step_into`: Executes a single instruction for the CPU. If the instruction is a **call**, execution continues in the called address.
* `v1.step_over`: Executes a single instruction for the CPU. If the instruction is a **call**, execution will be resumed until the callee returns.
* `v1.step_out`: Resumes execution until control is returned to the caller, e.g. via a `RET` instruction in the Z80 case.

> `memory_regions` here reflect exact what the CPU sees when it reads bytes. As an example, a banked cartridge won't appear in its entirety here, only the banks that are currently selected to be visible via the CPU address bus.

> Many games written in assembly employ tricks to save either memory space, CPU cycles, or both. Sometimes when an address is called, the callee won't return to the instruction following the `CALL` instruction. The core must be careful to implement `step_over` and `step_out` to support the required functionality as best as possible. In any case, the core must never lock into an execution loop that prevents the user from interacting with the front-end.

### hc_Memory

```c
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
```

* `v1.description`: The description of the memory region.
* `v1.flags`: The flags for the region. Specifically, `HC_CPU_ADDRESSABLE` must be set for regions that which contents are exactly what the CPU would see via its address bus.
* `v1.base_address`: Where in the CPU address space the region begins. As an example, a MSX cartridge has a base address of `0x4000`.
* `v1.size`: The size of the region.
* `v1.peek`: A function that can read bytes from the memory region. The address will be inside the `base_address`-`base_address + size - 1` range.
* `v1.poke`: A function that can write bytes to the region. ROMs should support writes to allow patches.
* `v1.set_watch_point`: Sets a watchpoint at the given block of region, starting at `address` and going for `length` bytes. The watchpoint can be triggered for reads, writes, or both.

> A 256 KiB [MegaROM](https://www.msx.org/wiki/MegaROM_Mappers) MSX cartridge (a cartridge that has banking) can be exposed via two different memory regions, one that begins at `0x4000` and has a size of 32 KiB, and that always reflect what the CPU can see via its address bus and has the `HC_CPU_ADDRESSABLE` bit set, and another that begins at address 0 and has a size of 256 KiB, which is the entire contents of the cartridge, and that does **not** have the `HC_CPU_ADDRESSABLE` bit set as the CPU is not capable of accessing the contents of the entire 256 KiB range via its address bus.

### hc_Register

```c
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
```

* `v1.name`: The register name.
* `v1.flags`: The flags for the register. `HC_SIZE_*` specifies the size of the register in bytes. If the register is the program counter, the register will have the `HC_PROGRAM_COUNTER` bit set. Same thing for the stack pointers. If the register is commonly used to index memory, as e.g. `HL`, `IX`, and `IY` in the Z80, it can set the `HC_MEMORY_POINTER` bit so the front-end can provide a special visualization if possible.
* `v1.get`: Gets the value of the register.
* `v1.set`: Sets the value of the register. Read-only registers can have this set to `NULL`.
* `v1.bits`: An array of C strings for each bit of the register, from most to less significant, I.e. the Z80 `F` register should set this to `{"S", "Z", "Y", "H", "X", "PV", "N", "C"}`. This makes it possible for the front-end to provide a bit-by-bit view of the register, where each bit can be changed in isolation.

## The Front-end

The front-end works with the concept of **Consoles**. Consoles are specific Libretro cores that implement the debug API, and that have an companion Lua script that is responsible for its loading and required configuration and/or automation.

The Lua script is required because some cores have specific needs to be considered ready to be debbuged, i.e.:

* Concatenate different memory regions or descriptors into one single memory view
* Configure some aspects of the core so that it works best with the front-end, i.e. configure the core for software rendering

Some technology that is used:

* [SDL](https://www.libsdl.org/) to abstract a bunch of stuff
* [SDL_GameControllerDB](https://github.com/gabomdq/SDL_GameControllerDB) to support as many as possible game controllers in the front-end
* [Dear ImGui](https://github.com/ocornut/imgui) for the GUI
    * A file system widget from [here](https://github.com/Flix01/imgui)
    * [This patched version](https://github.com/leiradel/imgui_club/tree/CustomPreview3) of `imgui_memory_editor`
    * [ImGuiAl](https://github.com/leiradel/ImGuiAl) for some widgets
    * [IconFontCppHeaders](https://github.com/juliettef/IconFontCppHeaders) for macros that make it easy to use the icons provided by [Font Awesome](https://fontawesome.com/)
* [lrcpp](https://github.com/leiradel/lrcpp) to make it easier to write a fully functional Libretro front-end
* [Speex](https://www.speex.org/) to resample the audio output from the cores to the sample rate required by the host system
* [fnkdat](https://savannah.nongnu.org/projects/fnkdat) to provide a platform-independent way of computing and creating some folders needed by the front-end
    * This library is old so I'm open to ideas for replacements
* `dynlib` for a platform-independent way of loading shared objects and getting their symbols
    * I've been using this for a long time and I can't seem to track down the original source, if you're the author and the license is not compatible with the one used here (MIT), please contact me
* [Lua](https://www.lua.org/) 5.4.2 as the scripting language
    * [LuaFileSystem](https://github.com/keplerproject/luafilesystem) to provide the scripts the ability to discover and run other scripts
* [ddlt](https://github.com/leiradel/luamods) for its Finite State Machine compiler that is used to generate a class from `LifeCycle.fsm` that guarantees the application is always in a consistent status.
* I'm currently using [chips](https://github.com/floooh/chips) for its Z80 disassembler, but I'll likely have to write a custom one that can use symbols instead of only addresses

The language used for the front-end is C++, and I'm employing multiple-inheritance as a way to allow classes to implement multiple interfaces, where an interface is an abstract C++ class. For all the criticism that multiple-inheritance gets, for this project I believe that it's working quite well. Everything is declared in the `hc` namespace.

The Front-end has the concept of a desktop, where multiple views can be added and laid out.

Some notes about the code:

* `Desktop.h`
    * Declares the `View` interface.
        * Views must have a title, and have methods that follow the application life-cycle.
        * Classes that implement this interface can be added to the desktop.
    * Declares the `Desktop` class, which is itself a `View`.
        * `Desktop` has a method to add views for it to manage.
        * It has helpers to access the logging system because it makes it easier for views to log things as they all have a parent desktop property.
        * It also has methods to compute the draw FPS, which is the FPS of the front-end so to speak, and the frame FPS, which is the FPS of the core.
        * The only class that inherits from `Desktop` is `Application`.
* `Scriptable.h`
    * Declares the `Scriptable` interface.
        * This interface has only one method, `push`, which must pushe an instance of a class that implements it onto the Lua stack as a full userdata object.
        * Classes that implement this interface should also have a method that can check if an instance of it is present at a particular index of the Lua stack, i.e. `static Config* check(lua_State* const L, int const index);`
        * I'm not convinced of the necessity of this interface, but it'll stay there for now.
* `Application.h`
    * Declares the `Application` class which is the main class of the front-end.
        * `Application` implements both `Desktop` and `Scriptable`
        * As the aggregator of all the front-end functionality, its `push` method will push all other scriptable objects present in the system: `logger`, `config`, `led`, `perf`, `control`, as well as some additional information commonly found in Lua modules.
        * `Application` is not a singleton, but it should be. I'm not very keen of singletons, but it made sense to write the main application code in a class, and there should be only one instance of it at a time.
        * This class is responsible for initializing everything: SDL, ImGui, and instantiate the views that are always available.
        * It's also responsible for the event loop and system tear down.
        * Application is the context used with the Finite State Machine that manages the application life-cycle, and thus implements all the methods that the FSM needs to change its state.
* `Timer.h`
    * Implements a timer that can be paused and resumed, used to compute FPS metrics in the desktop. Shamelessly copied from the [Lazy Foo](https://lazyfoo.net/tutorials/SDL/24_calculating_frame_rate/index.php) implementation.
* `Fifo.h`
    * Implements a byte ring buffer used by the audio subsystem to help with audio resampling and sending to the hardware audio device
* `Devices.h`
    * `Device` is a `View` that abstracts input devices for the rest of the system
        * Has a keyboard device, usable by both the physical keyboard and via a On-Screen Keyboard widget
        * Has a mouse device, which is the physical mouse but constrained by the video texture so it's usable by the Libretro core
        * Has a virtual game controller usable via the physical keyboard
        * Manages all connected game controllers, including hot-plugging
    * Classes wanting to know when devices are added to and removed from the system can implement the `DeviceListener` interface and add themselves to the `Device` instance
* `lrcpp` components
    * `Logger.h`: Declares the `Logger` implementation. The logger is also a `View` and `Scriptable`.
    * `Config.h`: Declares the `Config` implementation. `Config` also implements `View` and `Scriptable`.
        * `Config` is also responsible for declaring memory views using the concatenation of different memory regions or descriptors. This can be done in a Lua script.
    * `Video.h`: Declares the `Video` implementation. `Video` is a view, and uses OpenGL to keep a texture updated in respect to the emulated framebuffer and blit it via ImGui.
    * `Led.h`: Declares the `Led` implementation. Led is also a `View` and `Scriptable` so Lua can use leds to signal state if they want. This `lrcpp` component is a minor one, but the Vice Libretro core crashes if there's not one available.
    * `Audio.h`: Declares the `Audio` implementation, which is also a `View` that renders the audio frames as a wave form. Not particularly useful but interesting to watch.
    * `Input.h`: Declares the `Input` implementation, which is also a `View` and a `DeviceListener`.
    * `Perf.h`: Declares the `Perf` implementation. `Perf` also implements `View` (so it's possible to see the registered counters), and `Scriptable` (so it's possible to perf Lua code)
        * `Application` automatically creates a counter around the Libretro `retro_run` function call
    * Other components are not implemented for now
* Other views
    * `Control.h`: Has a GUI to allow the control of the application lifecycle: open a core, open a game, run, pause, and resume the game, unload it, and unload the core. It's also scriptable, and provides Lua methods to call into the Libretro API implemented by the core.
    * `Debugger.h`
        * Has the `Debugger` view, which can be used to open a view for one of the CPUs present in the emulated system.
        * Has the `Cpu` view, which shows the registers, can step the code instruction by instruction, and can open views for a disassembly window.
        * Has the `Disasm` view, which presents the disassembly of the code surrounding the CPU's program counter. Must be extended to support scrolling to arbitrary addresses.
    * `Memory.h`
        * Declares the `Memory` interface, which must be implemented by anyone wanting to present an edit control for a block of memory as a hexadecimal view.
        * Has the `MemoryWatch` view, which shows an edit control for a `Memory`.
        * Has the `MemorySelector` view, which centralizes all `Memory` instances and allows them to be opened in a `MemoryView`.
* The rest
    * `LifeCycle.fsm`: May appear small and unnecessary, but is central in making sure the application is always in a consistent state. One of the biggest challenges and source of bugs in any medium-sized application IMO.
    * `LuaUtil.h`: Some utility stuff to use with Lua
