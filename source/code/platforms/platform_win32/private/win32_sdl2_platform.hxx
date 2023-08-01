#pragma once
#include <ice/platform.hxx>
#include <ice/platform_core.hxx>

#include <ice/shard_container.hxx>
#include <ice/input/device_event_queue.hxx>
#include <ice/mem_allocator_proxy.hxx>

#include "win32_sdl2_platform_render_surface.hxx"

namespace ice::platform::win32::sdl2
{

    struct Platform_Win32SDL2 final : Core
    {
        Platform_Win32SDL2(ice::Allocator& alloc) noexcept;
        ~Platform_Win32SDL2() noexcept;

        auto refresh_events() noexcept -> ice::Result override;

        auto system_events() noexcept -> ice::ShardContainer const& override { return _system_events; }
        auto input_events() noexcept -> ice::Span<ice::input::DeviceEvent const> override { return ice::array::slice(_input_events._events); }

        ice::ProxyAllocator _alloc;
        ice::ShardContainer _system_events;
        ice::input::DeviceEventQueue _input_events;

        RenderSurface_Win32SDL2 _render_surface;
    };

} // namespace ice::platform::win32::sdl2
