/// Copyright 2024 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/platform_core.hxx>
#include <ice/platform_storage.hxx>
#include <ice/mem_allocator_host.hxx>
#include <ice/container/array.hxx>
#include <ice/shard_container.hxx>
#include <ice/task_queue.hxx>
#include <ice/task_thread.hxx>
#include <ice/app.hxx>

#include <ice/input/device_event_queue.hxx>
#include "webasm_render_surface.hxx"

namespace ice::platform::webasm
{

    class WebAsmCoreApp
        : public ice::platform::Core
        , public ice::platform::StoragePaths
    {
    public:
        static WebAsmCoreApp* global_instance;

        WebAsmCoreApp() noexcept;
        virtual ~WebAsmCoreApp() noexcept;

        void main_update() noexcept;
        void thread_update() noexcept;

    public: // ice::platform::Core
        auto system_events() noexcept -> ice::ShardContainer const& override { return _system_events; }
        auto input_events() noexcept -> ice::Span<ice::input::DeviceEvent const> override { return _input_events._events; }

        auto graphics_thread() noexcept -> ice::TaskScheduler& override { return _graphics_thread; }

    public:
        auto data_locations() const noexcept -> ice::Span<ice::String const> override { static ice::String paths[]{ "/" }; return paths; }
        auto save_location() const noexcept -> ice::String override { return "/saves"; }
        auto cache_location() const noexcept -> ice::String override { return "/cache"; }
        auto dylibs_location() const noexcept -> ice::String override { return {}; }

    private:
        static auto native_webapp_thread(void* userdata, ice::TaskQueue& queue) noexcept -> ice::u32;

    protected:
        ice::HostAllocator _allocator;

    private:
        ice::app::Factories _factories;
        ice::ParamList _params;

        ice::u32 _initstage;
        ice::TaskQueue _update_queue;
        ice::UniquePtr<ice::TaskThread> _update_thread;
        ice::TaskQueue _graphics_queue;
        ice::TaskScheduler _graphics_thread;

        ice::UniquePtr<ice::app::Config> _config;
        ice::UniquePtr<ice::app::State> _state;
        ice::UniquePtr<ice::app::Runtime> _runtime;

    protected:
        ice::ShardContainer _system_events;
        ice::input::DeviceEventQueue _input_events;

        ice::vec2i _last_windows_size;

    public:
        ice::platform::webasm::WebASM_RenderSurface _render_surface;
    };

} // namespace ice::platform::webasm
