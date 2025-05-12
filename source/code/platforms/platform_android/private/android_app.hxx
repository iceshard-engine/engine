/// Copyright 2023 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/string/static_string.hxx>
#include <ice/platform_core.hxx>
#include <ice/platform_storage.hxx>
#include <ice/input/device_event_queue.hxx>
#include <ice/module_register.hxx>
#include <ice/task_queue.hxx>
#include <ice/task_thread.hxx>
#include <ice/app.hxx>
#include <atomic>

#include <android/native_activity.h>
#include "android_render_surface.hxx"
#include "android_threads.hxx"
#include "android_app_core.hxx"

namespace ice::platform::android
{

    class AndroidApp final
        : public ice::platform::Core
        , public ice::platform::StoragePaths
        , public ice::platform::android::AndroidAppCore
    {
    public:
        static AndroidApp* global_instance;

        AndroidApp(
            ice::Allocator& alloc,
            ice::Data saved_state,
            ANativeActivity* activity
        ) noexcept;

        void initialize(ice::Span<ice::Shard const> params) noexcept;
        auto threads() noexcept -> ice::platform::Threads* { return _threads.get(); }
        auto render_surface() noexcept -> ice::platform::RenderSurface* { return &_app_surface; }

    public: // ice::platform::Core
        auto refresh_events() noexcept -> ice::Result override;
        auto system_events() noexcept -> ice::ShardContainer const& override { return _system_events; }
        auto input_events() noexcept -> ice::Span<ice::input::DeviceEvent const> override { return _input_events._events; }

    public: // ice::platform::StoragePaths
        auto data_locations() const noexcept -> ice::Span<ice::String const> override;
        auto save_location() const noexcept -> ice::String override { return _app_save_data; }
        auto cache_location() const noexcept -> ice::String override { return _app_internal_data; }
        auto dylibs_location() const noexcept -> ice::String override { return _app_modules; }

    public: // ice::platform::android::AndroidAppCore
        void on_init() noexcept override;
        void on_setup() noexcept override;
        void on_resume() noexcept override;
        void on_update() noexcept override;
        void on_suspend() noexcept override;
        void on_shutdown() noexcept override;

    public: // internal
        static void native_callback_on_start(ANativeActivity* activity);
        static void native_callback_on_resume(ANativeActivity* activity);
        static void native_callback_on_update(void* userdata);
        static void native_callback_on_pause(ANativeActivity* activity);
        static void native_callback_on_stop(ANativeActivity* activity);
        static void native_callback_on_destroy(ANativeActivity* activity);
        static void native_callback_on_focus_changed(ANativeActivity* activity, int has_focus);

        static void native_window_on_created(ANativeActivity* activity, ANativeWindow* window);
        static void native_window_on_destroyed(ANativeActivity* activity, ANativeWindow* window);
        static void native_window_on_redraw_needed(ANativeActivity* activity, ANativeWindow* window);
        static void native_window_on_resized(ANativeActivity* activity, ANativeWindow* window);

        static void native_inputqueue_on_created(ANativeActivity* activity, AInputQueue* inputs);
        static void native_inputqueue_on_destroyed(ANativeActivity* activity, AInputQueue* inputs);

        static void native_callback_on_low_memory(ANativeActivity* activity);
        static void native_callback_on_configuration_changed(ANativeActivity* activity);
        static void native_callback_on_rect_changed(ANativeActivity* activity, ARect const* rect);
        static auto native_callback_on_save_state(ANativeActivity* activity, size_t* out_size) -> void*;

    private:
        ice::app::Factories _factories;

        ice::Params _params;
        ice::UniquePtr<ice::app::Config> _config;
        ice::UniquePtr<ice::app::State> _state;
        ice::UniquePtr<ice::app::Runtime> _runtime;

        ice::ShardContainer _system_events;
        ice::input::DeviceEventQueue _input_events;
        ice::vec2f _new_screen_size;

        ice::UniquePtr<AndroidThreads> _threads;

        std::atomic_uint32_t _app_state;
        std::atomic<AInputQueue*> _app_queue;
        AndroidRenderSurface _app_surface;

        ice::StaticString<256> _app_modules;
        ice::StaticString<256> _app_internal_data;
        ice::StaticString<256> _app_external_data;
        ice::StaticString<256> _app_save_data;
    };

} // namespace ice
