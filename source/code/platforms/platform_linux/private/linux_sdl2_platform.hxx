/// Copyright 2023 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/platform.hxx>
#include <ice/platform_core.hxx>
#include <ice/platform_threads.hxx>

#include <ice/shard_container.hxx>
#include <ice/input/device_event_queue.hxx>
#include <ice/mem_allocator_proxy.hxx>
#include <ice/task_thread.hxx>
#include <ice/task_queue.hxx>

#include "linux_sdl2_platform_render_surface.hxx"

namespace ice::platform::linux::sdl2
{

    struct Platform_LinuxSDL2 final : Core
    {
        Platform_LinuxSDL2(ice::Allocator& alloc) noexcept;
        ~Platform_LinuxSDL2() noexcept;

        auto refresh_events() noexcept -> ice::Result override;

        auto system_events() noexcept -> ice::ShardContainer const& override { return _system_events; }
        auto input_events() noexcept -> ice::Span<ice::input::DeviceEvent const> override { return ice::array::slice(_input_events._events); }

        auto allocator() noexcept -> ice::Allocator& { return _alloc.backing_allocator(); }

        ice::ProxyAllocator _alloc;
        ice::ShardContainer _system_events;
        ice::input::DeviceEventQueue _input_events;

        RenderSurface_WaylandSDL2 _render_surface;
    };

} // namespace ice::platform::win32::sdl2
