#include "Handle.h"

static hc::HandleAllocator<hc::Memory*> _memoryHandles;
static hc::HandleAllocator<hc::Register*> _registerHandles;

hc::Handle<hc::Memory*> hc::handle::allocate(Memory* const memory) {
    return _memoryHandles.allocate(memory);
}

void hc::handle::free(Handle<Memory*> const handle) {
    _memoryHandles.free(handle);
}

hc::Memory* hc::handle::translate(Handle<Memory*> const handle) {
    auto const ref = _memoryHandles.translate(handle);
    return ref != nullptr ? *ref : nullptr;
}

hc::Handle<hc::Register*> hc::handle::allocate(Register* const reg) {
    return _registerHandles.allocate(reg);
}

void hc::handle::free(Handle<Register*> const handle) {
    _registerHandles.free(handle);
}

hc::Register* hc::handle::translate(Handle<Register*> const handle) {
    auto const ref = _registerHandles.translate(handle);
    return ref != nullptr ? *ref : nullptr;
}
