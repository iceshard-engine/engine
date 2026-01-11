/// Copyright 2023 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "android_app.hxx"
#include "android_input_motion.hxx"

#include <ice/profiler.hxx>
#include <ice/app.hxx>
#include <ice/log.hxx>
#include <ice/assert.hxx>
#include <ice/path_utils.hxx>
#include <ice/heap_string.hxx>
#include <ice/os/android.hxx>
#include <ice/params.hxx>
#include <thread>


namespace ice::platform::android
{

    static constexpr auto Constant_TouchScreenDevice = ice::input::make_device_handle(
        ice::input::DeviceType::TouchScreen,
        ice::input::DeviceIndex{ 0 }
    );

    static constexpr auto Constant_KeyboardDevice = ice::input::make_device_handle(
        ice::input::DeviceType::Keyboard,
        ice::input::DeviceIndex{ 0 }
    );

    static auto get_cache_path(ice::Allocator& alloc, jobject const& activity, JNIEnv* env) noexcept
    {
        ice::HeapString<> res{ alloc };
        jmethodID mid_getCacheDir;
        jmethodID mid_getAbsPath;
        {
            ice::jni::JClass clazz{ env, env->GetObjectClass(activity) };
            mid_getCacheDir = env->GetMethodID(clazz.native(), "getCacheDir", "()Ljava/io/File;");
            clazz = jni::JClass{ env, env->FindClass("java/io/File") };
            mid_getAbsPath = env->GetMethodID(clazz.native(), "getAbsolutePath", "()Ljava/lang/String;");
        }

        ice::jni::JObject path{ env, env->CallObjectMethod(activity, mid_getCacheDir) };
        path = jni::JObject{ env, env->CallObjectMethod(path.native(), mid_getAbsPath) };
        jsize const len = env->GetStringUTFLength((jstring)path.native());
        ice::string::resize(res, static_cast<ice::ucount>(len));
        env->GetStringUTFRegion((jstring)path.native(), 0, len, ice::string::begin(res));
        return res;
    }

    static auto get_jni_path(ice::Allocator& alloc, jobject const& activity, JNIEnv* env) noexcept
    {
        ice::HeapString<> res{ alloc };
        jmethodID mid_getPackageName;
        jmethodID mid_getPackageManager;
        jmethodID mid_getApplicationInfo;
        jfieldID fid_nativeLibraryDir;
        {
            ice::jni::JClass clazz{ env, env->GetObjectClass(activity) };
            mid_getPackageName = env->GetMethodID(clazz.native(), "getPackageName", "()Ljava/lang/String;");
            mid_getPackageManager = env->GetMethodID(clazz.native(), "getPackageManager", "()Landroid/content/pm/PackageManager;");
            clazz = jni::JClass{ env, env->FindClass("android/content/pm/PackageManager") };
            mid_getApplicationInfo = env->GetMethodID(clazz.native(), "getApplicationInfo", "(Ljava/lang/String;I)Landroid/content/pm/ApplicationInfo;");
            clazz = jni::JClass{ env, env->FindClass("android/content/pm/ApplicationInfo") };
            fid_nativeLibraryDir = env->GetFieldID(clazz.native(), "nativeLibraryDir", "Ljava/lang/String;");
        }

        ice::jni::JObject obj{ env, env->CallObjectMethod(activity, mid_getPackageManager) };
        ice::jni::JObject pkgName{ env, env->CallObjectMethod(activity, mid_getPackageName) };
        obj = jni::JObject{ env, env->CallObjectMethod(obj.native(), mid_getApplicationInfo, pkgName.native(), jint{ 0 }) };
        obj = jni::JObject{ env, env->GetObjectField(obj.native(), fid_nativeLibraryDir) };
        jsize const len = env->GetStringUTFLength((jstring)obj.native());
        ice::string::resize(res, static_cast<ice::ucount>(len));
        env->GetStringUTFRegion((jstring)obj.native(), 0, len, ice::string::begin(res));
        return res;
    }

    static auto core_instance(ANativeActivity* activity) noexcept
    {
        return reinterpret_cast<AndroidApp*>(activity->instance);
    }

    static auto core_instance(void* userdata) noexcept
    {
        return reinterpret_cast<AndroidApp*>(userdata);
    }

    static auto core_procedure(void* userdata, ice::TaskQueue& queue) noexcept -> ice::u32
    {
        AndroidApp::native_callback_on_update(userdata);
        return 0;
    }

    AndroidApp* AndroidApp::global_instance = nullptr;

    AndroidApp::AndroidApp(
        ice::Allocator& alloc,
        ice::Data saved_state,
        ANativeActivity* activity
    ) noexcept
        : AndroidAppCore{ alloc, activity }
        , _factories{ }
        , _params{ ice::create_params(_allocator, "iceshard", "0.0.1", "") }
        , _system_events{ _allocator }
        , _input_events{ _allocator }
        , _new_screen_size{ 1.f, 1.f }
        , _app_surface{ }
    {
        ice::HeapString<> cache_path = get_cache_path(alloc, activity->clazz, activity->env);

        _app_modules = get_jni_path(alloc, activity->clazz, activity->env);
        _app_internal_data = activity->internalDataPath;
        _app_external_data = activity->externalDataPath;
        _app_save_data = activity->externalDataPath;
        ice::string::push_back(_app_modules, '/');
        ice::string::push_back(_app_internal_data, '/');
        ice::string::push_back(_app_external_data, '/');
        ice::string::push_back(_app_save_data, '/');

        ICE_ASSERT(global_instance == nullptr, "Only one instance of AndroidApp should ever be created!");
        global_instance = this;
    }

    void AndroidApp::initialize(ice::Span<ice::Shard const> params) noexcept
    {
        _threads = ice::make_unique<AndroidThreads>(_allocator, _allocator, params);

        // On browsers it's better to use the actual main thread as the gfx thread, and introduce a new 'main' thread instead.
        ice::UniquePtr<ice::TaskThread> logic_thread = ice::create_thread(
            _allocator,
            _threads->queue_main,
            TaskThreadInfo{
                .exclusive_queue = true,
                .wait_on_queue = false, // The main-thread needs to be called all the time
                .custom_procedure = core_procedure,
                .custom_procedure_userdata = this,
                .debug_name = "ice.main"
            }
        );

        _threads->threadpool_object()->attach_thread("platform.main-thread"_sid, ice::move(logic_thread));
    }

    auto AndroidApp::refresh_events() noexcept -> ice::Result
    {
        using namespace ice::input;
        ice::shards::clear(_system_events);
        _input_events.clear();

        static bool first_refresh = true;
        if (first_refresh)
        {
            first_refresh = false;
            _input_events.push(Constant_KeyboardDevice, DeviceMessage::DeviceConnected);
            _input_events.push(Constant_TouchScreenDevice, DeviceMessage::DeviceConnected);
        }

        if (_new_screen_size.x > 0.f)
        {
            _input_events.push(Constant_TouchScreenDevice, DeviceMessage::TouchScreenSizeX, _new_screen_size.x);
            _input_events.push(Constant_TouchScreenDevice, DeviceMessage::TouchScreenSizeY, _new_screen_size.y);
            _new_screen_size = { 0.f, 0.f };
        }

        // Query inputs if we have a queue available.
        AInputQueue* const queue = _app_queue.exchange(nullptr, std::memory_order_relaxed);
        if (queue != nullptr)
        {
            AInputEvent* event_ptr;
            while(AInputQueue_getEvent(queue, &event_ptr) >= 0)
            {
                if (AInputQueue_preDispatchEvent(queue, event_ptr) != 0)
                {
                    continue;
                }

                ice::Result result = ice::E_Error;
                ice::i32 const event_type = AInputEvent_getType(event_ptr);
                switch (event_type)
                {
                case AINPUT_EVENT_TYPE_MOTION:
                {
                    result = ice::platform::android::handle_android_motion_event(event_ptr, _input_events);
                    break;
                }
                case AINPUT_EVENT_TYPE_KEY:
                    IPT_MESSAGE("AndroidEvent::Key");
                    ICE_LOG(ice::LogSeverity::Retail, ice::LogTag::Core, "AndroidEvent::Key");
                    break;
                case AINPUT_EVENT_TYPE_DRAG:
                    IPT_MESSAGE("AndroidEvent::Drag");
                    ICE_LOG(ice::LogSeverity::Retail, ice::LogTag::Core, "AndroidEvent::Drag");
                    break;
                case AINPUT_EVENT_TYPE_FOCUS:
                    IPT_MESSAGE("AndroidEvent::Focus");
                    ICE_LOG(ice::LogSeverity::Retail, ice::LogTag::Core, "AndroidEvent::Focus");
                    break;
                case AINPUT_EVENT_TYPE_TOUCH_MODE:
                    IPT_MESSAGE("AndroidEvent::TouchMode");
                    ICE_LOG(ice::LogSeverity::Retail, ice::LogTag::Core, "AndroidEvent::TouchMode");
                    break;
                default:
                    break;
                }

                AInputQueue_finishEvent(queue, event_ptr, result == ice::S_Success);
                ICE_LOG_IF(
                    result != ice::S_Success,
                    LogSeverity::Retail, LogTag::Core,
                    "Unhandled input event: {}", result.error()
                );
            }

            _app_queue.store(queue, std::memory_order_relaxed);
        }

        return ice::S_Success;
    }

    auto AndroidApp::data_locations() const noexcept -> ice::Span<ice::String const>
    {
        static ice::String const paths[]{
            _app_external_data,
            _app_internal_data,
        };
        return paths;
    }

    void AndroidApp::on_init() noexcept
    {
        IPT_ZONE_SCOPED;
        IPT_MESSAGE("Android::OnInit");
        ICE_LOG(ice::LogSeverity::Retail, ice::LogTag::Core, "Android::OnInit");

        ice_init(_allocator, _factories);

        _config = _factories.factory_config(_allocator);
        ice_args(_allocator, _params, *_config);

        _state = _factories.factory_state(_allocator);
        _runtime = _factories.factory_runtime(_allocator);
    }

    void AndroidApp::on_setup() noexcept
    {
        IPT_ZONE_SCOPED;
        IPT_MESSAGE("Android::OnSetup");
        ICE_LOG(ice::LogSeverity::Retail, ice::LogTag::Core, "Android::OnSetup");

        _new_screen_size = ice::vec2f{ _app_surface.get_dimensions() };

        ICE_ASSERT_CORE(native_window() != nullptr);
        _app_surface.set_native_window(native_window());

        ice::Result result = ice_setup(_allocator, *_config, *_state);
        if(result == ice::app::S_ApplicationSetupPending)
        {
            result = ice_setup(_allocator, *_config, *_state);
            ICE_ASSERT_CORE(result == app::S_ApplicationResume);
        }

        ICE_LOG_IF(result == false, ice::LogSeverity::Error, ice::LogTag::Core, "{}", result.error());
        ICE_ASSERT_CORE(result == true);
    }

    void AndroidApp::on_resume() noexcept
    {
        IPT_ZONE_SCOPED;
        IPT_MESSAGE("Android::OnResume");
        ICE_LOG(ice::LogSeverity::Retail, ice::LogTag::Core, "Android::OnResume");

        ice_resume(*_config, *_state, *_runtime);
    }

    void AndroidApp::on_update() noexcept
    {
        IPT_ZONE_SCOPED;
        ice_update(*_config, *_state, *_runtime);
    }

    void AndroidApp::on_suspend() noexcept
    {
        IPT_ZONE_SCOPED;
        IPT_MESSAGE("Android::OnSuspend");
        ICE_LOG(ice::LogSeverity::Retail, ice::LogTag::Core, "Android::OnSuspend");

        ice_suspend(*_config, *_state, *_runtime);
    }

    void AndroidApp::on_shutdown() noexcept
    {
        IPT_ZONE_SCOPED;
        IPT_MESSAGE("Android::OnShutdown");
        ICE_LOG(ice::LogSeverity::Retail, ice::LogTag::Core, "Android::OnShutdown");

        ice_shutdown(_allocator, *_config, *_state);

        _runtime.reset();
        _state.reset();
        _config.reset();
    }

    void AndroidApp::native_callback_on_start(ANativeActivity* activity)
    {
        IPT_ZONE_SCOPED;
        ice::platform::android::AndroidAppCore* app = core_instance(activity);
        app->process_message(AndroidMessageType::OnInit);
    }

    void AndroidApp::native_callback_on_resume(ANativeActivity* activity)
    {
        IPT_ZONE_SCOPED;
        ice::platform::android::AndroidAppCore* app = core_instance(activity);
        app->push_message(AndroidMessageType::OnResume);
    }

    void AndroidApp::native_callback_on_update(void* userdata)
    {
        IPT_ZONE_SCOPED;
        ice::platform::android::AndroidAppCore* app = core_instance(userdata);
        app->process_pending_messages();
    }

    void AndroidApp::native_callback_on_pause(ANativeActivity* activity)
    {
        IPT_ZONE_SCOPED;
        ice::platform::android::AndroidAppCore* app = core_instance(activity);
        app->push_message(AndroidMessageType::OnSuspend);
    }

    void AndroidApp::native_callback_on_stop(ANativeActivity* activity)
    {
        IPT_ZONE_SCOPED;
        ice::platform::android::AndroidAppCore* app = core_instance(activity);
        app->process_message(AndroidMessageType::OnShutdown);
    }

    void AndroidApp::native_callback_on_destroy(ANativeActivity* activity)
    {
        IPT_ZONE_SCOPED;
        IPT_MESSAGE("Android::OnDestroy");
        ICE_LOG(ice::LogSeverity::Retail, ice::LogTag::Core, "Android::OnDestroy");

        AndroidApp* core = core_instance(activity);
        ice::Allocator& alloc = core->_allocator;
        alloc.destroy(core);
    }

    void AndroidApp::native_callback_on_focus_changed(ANativeActivity* activity, int has_focus) { }

    void AndroidApp::native_window_on_created(ANativeActivity* activity, ANativeWindow* window)
    {
        IPT_ZONE_SCOPED;
        ice::platform::android::AndroidAppCore* app = core_instance(activity);
        app->process_message(AndroidMessageType::OnCreateWindow, window);
    }

    void AndroidApp::native_window_on_destroyed(ANativeActivity* activity, ANativeWindow* window)
    {
        IPT_ZONE_SCOPED;
        ice::platform::android::AndroidAppCore* app = core_instance(activity);
        app->process_message(AndroidMessageType::OnDestroyWindow, window);
    }

    void AndroidApp::native_window_on_redraw_needed(ANativeActivity* activity, ANativeWindow* window) { }
    void AndroidApp::native_window_on_resized(ANativeActivity* activity, ANativeWindow* window) { }

    void AndroidApp::native_inputqueue_on_created(ANativeActivity* activity, AInputQueue* inputs)
    {
        IPT_MESSAGE("Android::InputQueue::Created");
        ice::platform::android::AndroidApp* core = core_instance(activity);
        ICE_ASSERT(
            core->_app_queue == nullptr,
            "The 'AndroidApp::_app_queue' has unexpected value! [expected:{}, found:{}]",
            fmt::ptr((void*)0), fmt::ptr(core->_app_queue.load(std::memory_order_relaxed))
        );
        core->_app_queue.store(inputs, std::memory_order_relaxed);
    }

    void AndroidApp::native_inputqueue_on_destroyed(ANativeActivity* activity, AInputQueue* inputs)
    {
        IPT_MESSAGE("Android::InputQueue::Destroyed");
        ice::platform::android::AndroidApp* core = core_instance(activity);

        AInputQueue* expected_queue = inputs;
        while(core->_app_queue.compare_exchange_weak(expected_queue, nullptr, std::memory_order_relaxed) == false)
        {
            // Re-set the queue value
            expected_queue = inputs;
        }

        // ICE_ASSERT(
        //     core->_app_queue == inputs,
        //     "The 'AndroidApp::_app_queue' has unexpected value! [expected:{}, found:{}]",
        //     fmt::ptr(inputs), fmt::ptr(core->_app_queue.load(std::memory_order_relaxed))
        // );
        // core->_app_queue.store(nullptr, std::memory_order_relaxed);
    }

    void AndroidApp::native_callback_on_low_memory(ANativeActivity* activity) { }
    void AndroidApp::native_callback_on_configuration_changed(ANativeActivity* activity) { }
    void AndroidApp::native_callback_on_rect_changed(ANativeActivity* activity, ARect const* rect) { }
    auto AndroidApp::native_callback_on_save_state(ANativeActivity* activity, size_t* out_size) -> void* { *out_size = 0; return nullptr; }

} // namespace ice::platform::android
