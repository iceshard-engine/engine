#pragma once
#include <iolib/system.h>
#include <iolib/window.h>
#include <iolib/keyboard.h>
#include <iolib/mouse.h>

namespace mooned::io
{

// Stateless API
auto ticks() -> uint32_t;

void vsync(bool enable);

}
