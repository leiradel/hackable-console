#include "Snapshot.h"
#include "Memory.h"

#include <inttypes.h>
#include <sys/time.h>
#include <atomic>

extern "C" {
    #include <lauxlib.h>
}

static std::string createId() {
    static std::atomic<unsigned> unique(1);

    char buffer[64];
    snprintf(buffer, sizeof(buffer), "snap%u", unique++);

    return buffer;
}

static std::string createName(char const* memory_name) {
    struct timeval tv;
    gettimeofday(&tv, nullptr);

    struct tm tm;
    gmtime_r(&tv.tv_sec, &tm);

    char ts1[32];
    strftime(ts1, sizeof(ts1), "%Y-%m-%dT%H:%M:%S", &tm);

    char ts2[64];
    snprintf(ts2, sizeof(ts2), "%s.%06ldZ", ts1, tv.tv_usec);

    std::string name("Snapshot of ");
    name += std::string(memory_name);
    name += " created on ";
    name += ts2;
    return name;
}

static void* snapshot(hc::Memory* const memory) {
    uint8_t* const data = new uint8_t[memory->size()];
    uint64_t const end = memory->base() + memory->size();

    for (uint64_t i = 0, address = memory->base(); address < end; i++, address++) {
        data[i] = memory->peek(address);
    }

    return data;
}

hc::Snapshot::Snapshot(Memory* memory) : Snapshot(memory->base(), memory->size(), snapshot(memory), memory) {}

hc::Snapshot::Snapshot(uint64_t baseAddress, uint64_t size, void const* data, Memory* memory)
    : _id(createId())
    , _name(createName(memory->name()))
    , _baseAddress(baseAddress)
    , _size(size)
    , _data(data)
    , _memory(memory)
{}

hc::Snapshot::~Snapshot() {
    free(const_cast<void*>(_data));
}

uint8_t hc::Snapshot::peek(uint64_t address) const {
    uint64_t addr = address - _baseAddress;

    if (addr < _size) {
        auto data = static_cast<uint8_t const*>(_data);
        uint8_t byte = data[addr];
        return byte;
    }

    return 0;
}
