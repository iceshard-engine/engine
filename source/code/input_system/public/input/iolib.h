#pragma once
#include <input/system.h>
#include <input/window.h>
#include <input/keyboard.h>
#include <input/mouse.h>

namespace mooned::io
{

// Stateless API
auto ticks() -> uint32_t;

void vsync(bool enable);

}
