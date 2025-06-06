/// Copyright 2025 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/input/device_event_queue.hxx>
#include "linux_sdl2.hxx"

namespace ice::platform::linux::sdl2
{

    void mouse_input_events(
        ice::input::DeviceEventQueue& input_queue,
        SDL_Event const& sdl_event
    ) noexcept;

    void keyboard_input_events(
        ice::input::DeviceEventQueue& input_queue,
        SDL_Event const& sdl_event
    ) noexcept;

} // namespace ice::platform::linux::sdl2
