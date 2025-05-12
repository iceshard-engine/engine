/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/input/device_event_queue.hxx>
#include <SDL.h>
#undef main

namespace ice::platform::win32::sdl2
{

    void mouse_input_events(
        ice::input::DeviceEventQueue& input_queue,
        SDL_Event const& sdl_event
    ) noexcept;

    void keyboard_input_events(
        ice::input::DeviceEventQueue& input_queue,
        SDL_Event const& sdl_event
    ) noexcept;

} // namespace ice::platform::sdl2
