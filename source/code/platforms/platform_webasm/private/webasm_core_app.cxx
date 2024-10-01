/// Copyright 2024 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "webasm_core_app.hxx"
#include "webasm_include.hxx"

#include <ice/platform_event.hxx>
#include <ice/log.hxx>
#include <ice/app.hxx>

namespace ice::platform::webasm
{

    WebAsmCoreApp::WebAsmCoreApp() noexcept
        : _allocator{ }
        , _threads{ }
        , _factories{ }
        , _params{ ice::create_params(_allocator, "iceshard", "0.0.1", "") }
        , _initstage{ 0 }
        , _config{ }
        , _state{ }
        , _runtime{ }
        , _system_events{ _allocator }
        , _input_events{ _allocator }
        , _last_windows_size{ }
        , _render_surface{ }
    {
    }

    WebAsmCoreApp::~WebAsmCoreApp() noexcept
    {
        _runtime.reset();
        _state.reset();
        _config.reset();
    }

    void WebAsmCoreApp::initialize(ice::Span<ice::Shard const> params) noexcept
    {
        _threads = ice::make_unique<WebASM_Threads>(_allocator, _allocator, params);

        // On browsers it's better to use the actual main thread as the gfx thread, and introduce a new 'main' thread instead.
        ice::UniquePtr<ice::TaskThread> logic_thread = ice::create_thread(
            _allocator,
            _threads->queue_main,
            TaskThreadInfo{
                .exclusive_queue = true,
                .wait_on_queue = false, // The main-thread needs to be called all the time
                .custom_procedure = WebAsmCoreApp::native_webapp_thread,
                .custom_procedure_userdata = this,
                .debug_name = "ice.main"
            }
        );

        // Attach the main logic thread to the threadpool so we don't need to manage it ourselfs.
        _threads->threadpool_object()->attach_thread("platform.main-thread"_sid, ice::move(logic_thread));
    }

    void WebAsmCoreApp::main_update() noexcept
    {
        if (_initstage == 0) [[unlikely]]
        {
            ice_init(_allocator, _factories);

            _config = _factories.factory_config(_allocator);
            ice_args(_allocator, _params, *_config);

            _state = _factories.factory_state(_allocator);
            _runtime = _factories.factory_runtime(_allocator);
            _initstage = 1;
        }
        else if (_initstage == 1) [[unlikely]]
        {
            ice::Result const res = ice_setup(_allocator, *_config, *_state);
            if (res == ice::app::S_ApplicationResume)
            {
                _initstage = 2;
            }
            else if (res == ice::E_Fail)
            {
                ICE_LOG(LogSeverity::Critical, LogTag::Engine, "Failed setup stage, exiting. {}", res.error());
                _initstage = 5;
            }

            ice::vec2u const new_size = _render_surface.get_dimensions();
            if (_last_windows_size.x != new_size.x || _last_windows_size.y != new_size.y)
            {
                _last_windows_size = ice::vec2i{ new_size };
                //ice::shards::push_back(_system_events, ice::shard(ice::platform::ShardID_WindowResized, _last_windows_size));
            }
        }
        else if (_initstage == 2) // OnResume
        {
            ice::Result const res = ice_resume(*_config, *_state, *_runtime);
            if (res == ice::app::S_ApplicationUpdate)
            {
                _initstage = 3;
            }
            else
            {
                ICE_LOG(LogSeverity::Critical, LogTag::Engine, "Failed engine resume stage, exiting.");
                ice_suspend(*_config, *_state, *_runtime);
                _initstage = 5;
            }
        }
        else if (_initstage == 3)
        {
            _threads->queue_gfx.process_all();
        }
        else if (_initstage == 4) // OnSuspend
        {
            ice_suspend(*_config, *_state, *_runtime);
            ice_shutdown(_allocator, *_config, *_state);
            emscripten_force_exit(-1);
            _initstage = 6;
        }
        else if (_initstage == 5)
        {
            ice_shutdown(_allocator, *_config, *_state);
            emscripten_force_exit(-1);
            _initstage = 6;
        }
    }

    void WebAsmCoreApp::thread_update() noexcept
    {
        // Process all tasks awaiting for the main thread regardless of the init stage.
        _threads->queue_main.process_all();

        if (_initstage == 3) // OnUpdate
        {
            ice::Result const res = ice_update(*_config, *_state, *_runtime);
            if (res == ice::app::S_ApplicationExit)
            {
                _initstage = 4;
            }
            else
            {
                ICE_ASSERT_CORE(res == ice::app::S_ApplicationUpdate);
            }
        }
    }

    auto WebAsmCoreApp::native_webapp_thread(void* userdata, ice::TaskQueue& queue) noexcept -> ice::u32
    {
        WebAsmCoreApp* app = reinterpret_cast<WebAsmCoreApp*>(userdata);
        app->thread_update();
        sched_yield();
        return 0;
    }

} // namespace ice::platform::webasm
