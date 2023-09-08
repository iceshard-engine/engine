#include "android_platform.hxx"
#include <ice/profiler.hxx>
#include <ice/app.hxx>
#include <ice/log.hxx>
#include <ice/assert.hxx>
#include <thread>

namespace ice::platform::android
{

    static auto core_instance(ANativeActivity* activity) noexcept
    {
        return reinterpret_cast<AndroidCore*>(activity->instance);
    }

    static auto core_instance(void* userdata) noexcept
    {
        return reinterpret_cast<AndroidCore*>(userdata);
    }

    static auto core_procedure(void* userdata, ice::TaskQueue& queue) noexcept -> ice::u32
    {
        AndroidCore::native_callback_on_update(userdata);
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(1us);
        return 0;
    }

    AndroidCore::AndroidCore(ice::Allocator& alloc, ice::Data saved_state) noexcept
        : _allocator{ alloc }
        , _shards{ _allocator }
        , _main_queue{ }
        , _main_thread{ }
    {
        _app_state.store(2);
        ice::TaskThreadInfo const tinfo {
            .exclusive_queue = true,
            .custom_procedure = core_procedure,
            .custom_procedure_userdata = this,
            .debug_name = "ice.main",
        };
        _main_thread = ice::create_thread(_allocator, _main_queue, tinfo);
    }

    void AndroidCore::native_callback_on_start(ANativeActivity* activity)
    {
        IPT_ZONE_SCOPED;

        ice::platform::android::AndroidCore* core = core_instance(activity);
        ice::app::Factories& app_factories = core->_factories;
        ice::Allocator& alloc = core->_allocator;

        ice_init(alloc, app_factories);

        ice::app::ArgumentsConfig app_arguments_config{ };
        ice_args(alloc, app_arguments_config);

        ice::app::Arguments const app_arguments{ app_arguments_config, ice::Span<char const*>{ nullptr, (ice::ucount) 0 } };

        core->_config = app_factories.factory_config(alloc);
        core->_state = app_factories.factory_state(alloc);

        IPT_MESSAGE("Android::OnSetup");
        ICE_LOG(ice::LogSeverity::Retail, ice::LogTag::Core, "Android::OnSetup");

        ice::Result result = ice_setup(alloc, app_arguments, *core->_config, *core->_state);
        ICE_LOG_IF(result == false, ice::LogSeverity::Error, ice::LogTag::Core, "{}", ice::result_hint(result));
        ICE_ASSERT_CORE(result == true);
    }

    void AndroidCore::native_callback_on_resume(ANativeActivity* activity)
    {
        IPT_ZONE_SCOPED;
        IPT_MESSAGE("Android::OnResume");
        ICE_LOG(ice::LogSeverity::Retail, ice::LogTag::Core, "Android::OnResume");

        ice::platform::android::AndroidCore* core = core_instance(activity);
        ice::app::Factories& app_factories = core->_factories;

        core->_runtime = app_factories.factory_runtime(core->_allocator);
        ice_resume(*core->_config, *core->_state, *core->_runtime);

        [[maybe_unused]]
        ice::u32 const last_state = core->_app_state.exchange(0);
        ICE_ASSERT_CORE(last_state == 2);
    }

    void AndroidCore::native_callback_on_update(void* userdata)
    {
        IPT_FRAME_MARK;
        IPT_ZONE_SCOPED;

        ice::platform::android::AndroidCore* core = core_instance(userdata);

        // Query inputs if we have a queue available.
        AInputQueue* const queue = core->_app_queue.exchange(nullptr, std::memory_order_relaxed);
        if (queue != nullptr)
        {
            AInputEvent* event_ptr;
            while(AInputQueue_getEvent(queue, &event_ptr) >= 0)
            {
                if (AInputQueue_preDispatchEvent(queue, event_ptr) != 0)
                {
                    continue;
                }

                ice::i32 const event_type = AInputEvent_getType(event_ptr);
                switch (event_type)
                {
                case AINPUT_EVENT_TYPE_MOTION:
                    IPT_MESSAGE("AndroidEvent::Motion");
                    break;
                case AINPUT_EVENT_TYPE_KEY:
                    IPT_MESSAGE("AndroidEvent::Key");
                    break;
                case AINPUT_EVENT_TYPE_DRAG:
                    IPT_MESSAGE("AndroidEvent::Drag");
                    break;
                case AINPUT_EVENT_TYPE_FOCUS:
                    IPT_MESSAGE("AndroidEvent::Focus");
                    break;
                case AINPUT_EVENT_TYPE_TOUCH_MODE:
                    IPT_MESSAGE("AndroidEvent::TouchMode");
                    break;
                default:
                    break;
                }

                AInputQueue_finishEvent(queue, event_ptr, 0);
            }

            core->_app_queue.store(queue, std::memory_order_relaxed);
        }

        ice::u32 allowed_state = 0;
        if (core->_app_state.compare_exchange_weak(allowed_state, 1) == false)
        {
            return;
        }

        ice::app::Factories& app_factories = core->_factories;

        core->_runtime = app_factories.factory_runtime(core->_allocator);
        ice_update(*core->_config, *core->_state, *core->_runtime);

        core->_app_state.store(0);
    }

    void AndroidCore::native_callback_on_pause(ANativeActivity* activity)
    {
        IPT_ZONE_SCOPED;
        IPT_MESSAGE("Android::OnSuspend");
        ICE_LOG(ice::LogSeverity::Retail, ice::LogTag::Core, "Android::OnSuspend");

        ice::platform::android::AndroidCore* core = core_instance(activity);
        ice::u32 expected_state = 0;
        while(core->_app_state.compare_exchange_weak(expected_state, 2))
        {
            continue;
        }

        ice_suspend(*core->_config, *core->_state, *core->_runtime);
    }

    void AndroidCore::native_callback_on_stop(ANativeActivity* activity)
    {
        IPT_ZONE_SCOPED;
        IPT_MESSAGE("Android::OnShutdown");
        ICE_LOG(ice::LogSeverity::Retail, ice::LogTag::Core, "Android::OnShutdown");

        ice::platform::android::AndroidCore* core = core_instance(activity);

        ice::u32 expected_state = 0;
        while(core->_app_state.compare_exchange_weak(expected_state, 2))
        {
            continue;
        }

        core->_runtime.reset();

        ice::app::ArgumentsConfig args_config { };
        ice::app::Arguments args{ args_config, {} };
        ice_shutdown(core->_allocator, args, *core->_config, *core->_state);

        core->_state.reset();
        core->_config.reset();
    }

    void AndroidCore::native_callback_on_destroy(ANativeActivity* activity)
    {
        IPT_ZONE_SCOPED;
        IPT_MESSAGE("Android::OnDestroy");
        ICE_LOG(ice::LogSeverity::Retail, ice::LogTag::Core, "Android::OnDestroy");

        AndroidCore* core = reinterpret_cast<AndroidCore*>(activity->instance);
        ice::Allocator& alloc = core->_allocator;
        alloc.destroy(core);
    }

    void AndroidCore::native_callback_on_focus_changed(ANativeActivity* activity, int has_focus) { }
    void AndroidCore::native_window_on_created(ANativeActivity* activity, ANativeWindow* window) { }
    void AndroidCore::native_window_on_destroyed(ANativeActivity* activity, ANativeWindow* window) { }
    void AndroidCore::native_window_on_redraw_needed(ANativeActivity* activity, ANativeWindow* window) { }
    void AndroidCore::native_window_on_resized(ANativeActivity* activity, ANativeWindow* window) { }

    void AndroidCore::native_inputqueue_on_created(ANativeActivity* activity, AInputQueue* inputs)
    {
        IPT_MESSAGE("Android::InputQueue::Created");
        ice::platform::android::AndroidCore* core = core_instance(activity);
        ICE_ASSERT(
            core->_app_queue == nullptr,
            "The 'AndroidCore::_app_queue' has unexpected value! [expected:{}, found:{}]",
            fmt::ptr((void*)0), fmt::ptr(core->_app_queue.load(std::memory_order_relaxed))
        );
        core->_app_queue.store(inputs, std::memory_order_relaxed);
    }

    void AndroidCore::native_inputqueue_on_destroyed(ANativeActivity* activity, AInputQueue* inputs)
    {
        IPT_MESSAGE("Android::InputQueue::Destroyed");
        ice::platform::android::AndroidCore* core = core_instance(activity);

        AInputQueue* expected_queue = inputs;
        while(core->_app_queue.compare_exchange_weak(expected_queue, nullptr, std::memory_order_relaxed) == false)
        {
            // Re-set the queue value
            expected_queue = inputs;
        }

        // ICE_ASSERT(
        //     core->_app_queue == inputs,
        //     "The 'AndroidCore::_app_queue' has unexpected value! [expected:{}, found:{}]",
        //     fmt::ptr(inputs), fmt::ptr(core->_app_queue.load(std::memory_order_relaxed))
        // );
        // core->_app_queue.store(nullptr, std::memory_order_relaxed);
    }

    void AndroidCore::native_callback_on_low_memory(ANativeActivity* activity) { }
    void AndroidCore::native_callback_on_configuration_changed(ANativeActivity* activity) { }
    void AndroidCore::native_callback_on_rect_changed(ANativeActivity* activity, ARect const* rect) { }
    void* AndroidCore::native_callback_on_save_state(ANativeActivity* activity, size_t* out_size) { *out_size = 0; return nullptr; }

} // namespace ice::platform::android
