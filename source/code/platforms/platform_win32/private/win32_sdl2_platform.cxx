/// Copyright 2023 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "win32_sdl2_platform.hxx"
#include "win32_sdl2_utils.hxx"
#include "win32_storage.hxx"

#include <ice/platform_event.hxx>
#include <ice/mem_allocator_host.hxx>
#include <ice/profiler.hxx>
#include <ice/assert.hxx>

namespace ice::platform::win32::sdl2
{

    Platform_Win32SDL2::Platform_Win32SDL2(ice::Allocator& alloc) noexcept
        : _alloc{ alloc, "iceshard.platform-layer" }
        , _system_events{ _alloc }
        , _input_events{ _alloc }
        , _render_surface{ }
    {
        ice::shards::reserve(_system_events, 32);
        ice::array::reserve(_input_events._events, 512);

        SDL_InitSubSystem(SDL_INIT_EVENTS);

        using namespace ice::input;
    }

    Platform_Win32SDL2::~Platform_Win32SDL2() noexcept
    {
        SDL_QuitSubSystem(SDL_INIT_EVENTS);
        SDL_Quit();
    }

    auto Platform_Win32SDL2::refresh_events() noexcept -> ice::Result
    {
        IPT_ZONE_SCOPED;

        using namespace ice::input;

        _input_events.clear();
        ice::shards::clear(_system_events);

        static bool first_refresh = true;
        if (first_refresh)
        {
            first_refresh = false;

            _input_events.push(
                make_device_handle(DeviceType::Mouse, DeviceIndex{ 0 }),
                DeviceMessage::DeviceConnected
            );
            _input_events.push(
                make_device_handle(DeviceType::Keyboard, DeviceIndex{ 0 }),
                DeviceMessage::DeviceConnected
            );
        }

        static char text_buffer[32];
        static SDL_Event current_event{ };
        while (SDL_PollEvent(&current_event) != 0)
        {
            switch (current_event.type)
            {
            case SDL_QUIT:
                ice::shards::push_back(_system_events, ice::platform::Shard_AppQuit);

                _input_events.push(
                    make_device_handle(DeviceType::Keyboard, DeviceIndex(0)),
                    DeviceMessage::DeviceDisconnected
                );
                _input_events.push(
                    make_device_handle(DeviceType::Mouse, DeviceIndex(0)),
                    DeviceMessage::DeviceDisconnected
                );
                break;
            case SDL_WINDOWEVENT:
            {
                ice::vec2i const window_size{current_event.window.data1, current_event.window.data2};
                ICE_LOG(LogSeverity::Debug, LogTag::Core, "Window event ID: {}", current_event.window.event);

                switch (current_event.window.event)
                {
                case SDL_WINDOWEVENT_MINIMIZED:
                    ice::shards::push_back(_system_events, { ice::platform::ShardID_WindowMinimized });
                    break;
                case SDL_WINDOWEVENT_RESTORED:
                    ice::shards::push_back(_system_events, ice::platform::ShardID_WindowRestored | window_size);
                    break;
                case SDL_WINDOWEVENT_MAXIMIZED:
                    ice::shards::push_back(_system_events, ice::platform::ShardID_WindowMaximized | window_size);
                    break;
                case SDL_WINDOWEVENT_SIZE_CHANGED:
                case SDL_WINDOWEVENT_RESIZED:
                    ice::shards::push_back(_system_events, ice::platform::ShardID_WindowResized | window_size);
                    break;
                }
            }
            case SDL_MOUSEBUTTONUP:
            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEWHEEL:
            case SDL_MOUSEMOTION:
                mouse_input_events(_input_events, current_event);
                break;
            case SDL_KEYDOWN:
            case SDL_KEYUP:
                keyboard_input_events(_input_events, current_event);
                break;
                // [issue #33]
            case SDL_TEXTINPUT:
                ice::memcpy(text_buffer, current_event.text.text, 32);
                ice::shards::push_back(
                    _system_events,
                    ice::platform::ShardID_InputText | (char const*)text_buffer
                );
            }
        }

        return S_Success;
    }

} // namespace ice::platform::win32::sdl2

namespace ice::platform
{

    static win32::sdl2::Platform_Win32SDL2* core_feature;
    static ice::UniquePtr<win32::Win32Storage> storage_feature;
    static ice::UniquePtr<win32::Win32Threads> threads_feature;

    auto available_features() noexcept -> ice::platform::FeatureFlags
    {
        return FeatureFlags::Core
            | FeatureFlags::RenderSurface
            | FeatureFlags::StoragePaths
            | FeatureFlags::Threads;
    }

    auto initialize(
        ice::platform::FeatureFlags flags,
        ice::Span<ice::Shard const> params
    ) noexcept -> ice::Result
    {
        static ice::HostAllocator host_alloc;
        return initialize_with_allocator(host_alloc, flags, params);
    }

    auto initialize_with_allocator(
        ice::Allocator& alloc,
        ice::platform::FeatureFlags flags,
        ice::Span<ice::Shard const> params
    ) noexcept -> ice::Result
    {
        IPT_ZONE_SCOPED;

        static alignas(alignof(win32::sdl2::Platform_Win32SDL2)) char instance_buffer[sizeof(win32::sdl2::Platform_Win32SDL2)];

        if (core_feature != nullptr)
        {
            return E_PlatformAlreadyInitialized;
        }

        if (flags == FeatureFlags::None || !ice::has_all(available_features(), flags))
        {
            return E_InvalidArgument;
        }

        if (ice::has_any(flags, FeatureFlags::StoragePaths))
        {
            storage_feature = ice::make_unique<win32::Win32Storage>(alloc, alloc, params);
        }

        if (ice::has_any(flags, FeatureFlags::StoragePaths))
        {
            threads_feature = ice::make_unique<win32::Win32Threads>(alloc, alloc, params);
        }

        // Initialize the global platform instance. We don't use the allocator so we don't leak this pointer.
        core_feature = new (instance_buffer) win32::sdl2::Platform_Win32SDL2{ alloc };
        return S_Success;
    }

    auto query_api(ice::platform::FeatureFlags flag, void*& out_api_ptr) noexcept -> ice::Result
    {
        win32::sdl2::Platform_Win32SDL2* instance_ptr = core_feature;
        ICE_ASSERT(instance_ptr != nullptr, "Platform not initialized!");

        if (ice::has_all(available_features(), flag) == false)
        {
            return E_PlatformFeatureNotAvailable;
        }

        switch (flag)
        {
        case FeatureFlags::Core:
            out_api_ptr = static_cast<ice::platform::Core*>(instance_ptr);
            break;
        case FeatureFlags::StoragePaths:
            out_api_ptr = storage_feature.get();
            break;
        case FeatureFlags::Threads:
            out_api_ptr = threads_feature.get();
            break;
        case FeatureFlags::RenderSurface:
            out_api_ptr = static_cast<ice::platform::RenderSurface*>(ice::addressof(instance_ptr->_render_surface));
            break;
        default:
            return E_InvalidArgument;
        }

        return S_Success;
    }

    auto query_apis(ice::platform::FeatureFlags flags, void** out_api_ptrs) noexcept -> ice::Result
    {
        ice::Result result = S_Success;
        for (FeatureFlags flag : { FeatureFlags::Core, FeatureFlags::RenderSurface })
        {
            if (ice::has_all(flags, flag))
            {
                result = query_api(flag, *out_api_ptrs);
                if (result == false)
                {
                    break;
                }
                out_api_ptrs += 1;
            }
            else
            {
                result = E_InvalidArgument;
                break;
            }
        }

        return result;
    }

    void shutdown() noexcept
    {
        IPT_ZONE_SCOPED;

        win32::sdl2::Platform_Win32SDL2*& instance_ptr = core_feature;
        if (instance_ptr != nullptr)
        {
            storage_feature.reset();
            threads_feature.reset();
            instance_ptr->~Platform_Win32SDL2();
            instance_ptr = nullptr;
        }
    }

} // namespace ice::platform
