/// Copyright 2024 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/platform_core.hxx>
#include <ice/platform_storage.hxx>
#include <ice/mem_allocator_host.hxx>
#include <ice/array.hxx>
#include <ice/shard_container.hxx>
#include <ice/task_queue.hxx>
#include <ice/task_thread.hxx>
#include <ice/sync_manual_events.hxx>
#include <ice/app.hxx>

#include <ice/input/device_event_queue.hxx>
#include "webasm_render_surface.hxx"
#include "webasm_threads.hxx"

namespace ice::platform::webasm
{

    class WebAsmCoreApp
        : public ice::platform::Core
        , public ice::platform::StoragePaths
    {
    public:
        WebAsmCoreApp() noexcept;
        virtual ~WebAsmCoreApp() noexcept;

        virtual void initialize(ice::Span<ice::Shard const> params) noexcept;

        void main_update() noexcept;
        void thread_update() noexcept;

        auto threads() noexcept -> WebASM_Threads* { return _threads.get(); }

    public: // ice::platform::Core
        auto system_events() noexcept -> ice::ShardContainer const& override { return _system_events; }
        auto input_events() noexcept -> ice::Span<ice::input::DeviceEvent const> override { return _input_events._events; }

    public:
        auto data_locations() const noexcept -> ice::Span<ice::String const> override { static ice::String paths[]{ "/" }; return paths; }
        auto save_location() const noexcept -> ice::String override { return "/saves"; }
        auto cache_location() const noexcept -> ice::String override { return "/cache"; }
        auto dylibs_location() const noexcept -> ice::String override { return {}; }

    private:
        static auto native_webapp_thread(void* userdata, ice::TaskQueue& queue) noexcept -> ice::u32;

    protected:
        ice::HostAllocator _allocator;
        ice::UniquePtr<WebASM_Threads> _threads;
        ice::ManualResetEvent _wait;

    private:
        ice::app::Factories _factories;
        ice::Params _params;

        ice::u32 _initstage;

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
