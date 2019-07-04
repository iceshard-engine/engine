#pragma once
#include <core/allocator.hxx>
#include <core/cexpr/stringid.hxx>

namespace input
{

enum class Backend
{
    // Uses the SDL2 library as the backed with the target machine (windows, linux, macos)
    SDL2
};

// IO system related functions
class IOSystem;

IOSystem* initialize(core::allocator& alloc, Backend backend);

bool initialized(IOSystem* system);

void shutdown(IOSystem* system);

bool process_events(IOSystem* system);

void process_inputs(IOSystem* system);

// Clipboard API
const char* get_clipboard_text(IOSystem* system);

void set_clipboard_text(IOSystem* system, const char* text);

// Debug API
void send_debug_message(core::cexpr::stringid_argument_type name, int64_t value);

void send_tick_message();

}
