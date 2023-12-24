/// Copyright 2023 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <jni.h>
#include <EGL/egl.h>
#include <GLES/gl.h>

#include <android/sensor.h>
#include <android/log.h>

#include <android/native_activity.h>

#include <ice/string/string.hxx>
#include <ice/task.hxx>
#include <ice/task_queue.hxx>
#include <ice/task_scheduler.hxx>
#include <ice/task_thread.hxx>
#include <ice/task_thread_pool.hxx>
#include <ice/task_utils.hxx>
#include <ice/sync_manual_events.hxx>
#include <ice/mem_allocator_host.hxx>

#include <ice/log_module.hxx>
#include <ice/module_register.hxx>
#include <ice/resource_tracker.hxx>
#include <ice/resource_provider.hxx>
#include <ice/resource.hxx>
#include <ice/uri.hxx>

#include <ice/input/device_event_queue.hxx>
#include <ice/input/input_tracker.hxx>
#include <ice/input/input_touchscreen.hxx>

#include <ice/platform_core.hxx>
#include <ice/platform_storage.hxx>
#include <ice/platform_event.hxx>

#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <ice/app.hxx>

struct ice::app::Config { ice::Allocator& alloc; };
struct ice::app::State {
    State(ice::Allocator& alloc) noexcept
        : alloc{ alloc }
        , mreg{ ice::create_default_module_register(alloc) }
        , global_queue{ }
        , global_sched{ global_queue }
        , tpool{ }
        , res{ }
    {
        mreg->load_module(alloc, ice::load_log_module, ice::unload_log_module);
        ice::TaskThreadPoolCreateInfo const tpinfo {
            .thread_count = 4
        };
        tpool = ice::create_thread_pool(alloc, global_queue, tpinfo);
    }
    ice::Allocator& alloc;
    ice::UniquePtr<ice::ModuleRegister> mreg;
    ice::TaskQueue global_queue;
    ice::TaskScheduler global_sched;
    ice::UniquePtr<ice::TaskThreadPool> tpool;
    ice::UniquePtr<ice::ResourceTracker> res;
    ice::UniquePtr<ice::ResourceProvider> res_fs;

};
struct ice::app::Runtime {
    ice::Allocator& alloc;
    ice::SystemClock clock{ ice::clock::create_clock() };
    ice::UniquePtr<ice::input::InputTracker> inputs;
    ice::platform::Core* app_core;
};

template<typename T>
void destroy_object(T* obj) noexcept
{
    obj->alloc.destroy(obj);
}

void ice_init(
    ice::Allocator& alloc,
    ice::app::Factories& factories
) noexcept
{
    using namespace ice::app;
    ice::platform::initialize(ice::platform::available_features());

    factories.factory_config = [](ice::Allocator& alloc) -> ice::UniquePtr<ice::app::Config> { return ice::make_unique<Config>(&destroy_object<Config>, alloc.create<Config>(alloc)); };
    factories.factory_state = [](ice::Allocator& alloc) -> ice::UniquePtr<ice::app::State> { return ice::make_unique<State>(&destroy_object<State>, alloc.create<State>(alloc)); };
    factories.factory_runtime = [](ice::Allocator& alloc) -> ice::UniquePtr<ice::app::Runtime> { return ice::make_unique<Runtime>(&destroy_object<Runtime>, alloc.create<Runtime>(alloc)); };
}

auto ice_setup(
    ice::Allocator& alloc,
    ice::ParamList const& params,
    ice::app::Config& config,
    ice::app::State& state
) noexcept -> ice::Result
{
    ice::platform::StoragePaths* os_storage;
    ice::platform::query_api(os_storage);

    ice::ResourceTrackerCreateInfo const resinfo{
        .predicted_resource_count = 1000,
        .io_dedicated_threads = 1,
        .flags_io_complete = ice::TaskFlags{ },
        .flags_io_wait = ice::TaskFlags{ },
    };
    state.res = ice::create_resource_tracker(alloc, state.global_sched, resinfo);
    state.res_fs = ice::create_resource_provider(alloc, os_storage->data_locations());
    state.res->attach_provider(ice::move(state.res_fs));
    state.res->sync_resources();
    return ice::app::S_ApplicationResume;
}

auto ice_resume(
    ice::app::Config const& config,
    ice::app::State& state,
    ice::app::Runtime& runtime
) noexcept -> ice::Result
{
    runtime.inputs = ice::input::create_default_input_tracker(runtime.alloc, runtime.clock);
    runtime.inputs->register_device_type(ice::input::DeviceType::TouchScreen, ice::input::get_default_device_factory());
    ice::platform::query_api(runtime.app_core);
    return ice::app::S_ApplicationUpdate;
}

auto ice_update(
    ice::app::Config const& config,
    ice::app::State const& state,
    ice::app::Runtime& runtime
) noexcept -> ice::Result
{
    using ice::operator""_uri;
    IPT_ZONE_SCOPED;

    ice::clock::update(runtime.clock);

    ice::Array<ice::input::InputEvent> inputs{ runtime.alloc };
    runtime.app_core->refresh_events();
    runtime.inputs->process_device_events(runtime.app_core->input_events(), inputs);

    static ice::ucount pointer_count = 0;
    ice::vec2f positions[6];

    for (ice::input::InputEvent ev : inputs)
    {
        if (ev.identifier == ice::input::input_identifier(ice::input::DeviceType::TouchScreen, ice::input::TouchInput::TouchPointerCount))
        {
            pointer_count = ev.value.axis.value_i32;
            ICE_LOG(ice::LogSeverity::Retail, ice::LogTag::Game, "Touch Points ({})", pointer_count);
        }
        if (ev.identifier == ice::input::input_identifier(ice::input::DeviceType::TouchScreen, ice::input::TouchInput::TouchPosX))
        {
            positions[ice::u8(ice::input::make_device(ev.device).index)].x = ev.value.axis.value_f32;
        }
        if (ev.identifier == ice::input::input_identifier(ice::input::DeviceType::TouchScreen, ice::input::TouchInput::TouchPosY))
        {
            positions[ice::u8(ice::input::make_device(ev.device).index)].y = ev.value.axis.value_f32;
        }
        if (ev.identifier == ice::input::input_identifier(ice::input::DeviceType::TouchScreen, ice::input::TouchInput::VirtualButton))
        {
            if (ev.value.button.state.repeat ||
                ev.value.button.state.clicked ||
                ev.value.button.state.hold ||
                ev.value.button.state.released)
            ICE_LOG(
                ice::LogSeverity::Retail, ice::LogTag::Game,
                "Touch ({}) {}",
                ice::u8(ice::input::make_device(ev.device).index),
                ev.value.button.state.repeat ? "Repeat" :
                ev.value.button.state.clicked ? "Clicked" :
                ev.value.button.state.hold ? "Hold" :
                ev.value.button.state.released ? "Released" :
                "Pressed"
            );
        }
    }

    // for (ice::u32 idx = 0; idx < ice::min(pointer_count, 6u); ++idx)
    if (pointer_count > 0)
    {
        static ice::Timer tt = ice::timer::create_timer(runtime.clock, 1.f);
        if (ice::timer::update(tt))
        {
            ICE_LOG(
                ice::LogSeverity::Retail, ice::LogTag::Game,
                "Positions ({},{}) - ({},{}) - ({},{}) - ({},{}) - ({},{}) - ({},{})",
                positions[0].x, positions[0].y,
                positions[1].x, positions[1].y,
                positions[2].x, positions[2].y,
                positions[3].x, positions[3].y,
                positions[4].x, positions[4].y,
                positions[5].x, positions[5].y
            );
        }
    }

    static bool once = true;
    if (once)
    {
        once = false;
        ice::ResourceHandle* const text_res = state.res->find_resource("file:/config.json"_uri);
        ice::ResourceHandle* const text_res2 = state.res->find_resource("urn:config.json"_uri);
        if (text_res && text_res == text_res2)
        {
            ice::ResourceResult text = ice::wait_for(state.res->load_resource(text_res));
            if (text.resource_status == ice::ResourceStatus::Loaded)
            {
                ICE_LOG(ice::LogSeverity::Retail, ice::LogTag::Engine, "Loaded resource config.json");
            }
            else
            {
                ICE_LOG(ice::LogSeverity::Error, ice::LogTag::Engine, "Failed to load resource config.json");
            }
        }
    }

    return ice::app::S_ApplicationUpdate;
}

auto ice_suspend(
    ice::app::Config const& config,
    ice::app::State& state,
    ice::app::Runtime& runtime
) noexcept -> ice::Result
{
    runtime.inputs.reset();
    return ice::app::S_ApplicationResume;
}

auto ice_shutdown(
    ice::Allocator& alloc,
    ice::ParamList const& params,
    ice::app::Config const& config,
    ice::app::State& state
) noexcept -> ice::Result
{
    IPT_ZONE_SCOPED;
    state.res = nullptr;
    state.tpool = nullptr;
    state.mreg = nullptr;

    ice::platform::shutdown();
    return ice::app::S_ApplicationExit;
}