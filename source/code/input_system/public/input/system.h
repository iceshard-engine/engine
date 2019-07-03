#pragma once
#include <memsys/allocator.h>
#include <kernel/compiletime/stringid.h>

namespace mooned::io
{

enum class Backend
{
    // Uses the SDL2 library as the backed with the target machine (windows, linux, macos)
    SDL2
};

// IO system related functions
class IOSystem;

IOSystem* initialize(mem::allocator& alloc, Backend backend);

bool initialized(IOSystem* system);

void shutdown(IOSystem* system);

bool process_events(IOSystem* system);

void process_inputs(IOSystem* system);

// Clipboard API
const char* get_clipboard_text(IOSystem* system);

void set_clipboard_text(IOSystem* system, const char* text);

// Debug API
void send_debug_message(stringid_t name, int64_t value);

void send_tick_message();

}
