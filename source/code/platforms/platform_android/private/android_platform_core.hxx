/// Copyright 2023 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/string/static_string.hxx>
#include <ice/platform_core.hxx>
#include <ice/platform_paths.hxx>
#include <ice/module_register.hxx>
#include <ice/task_queue.hxx>
#include <ice/task_thread.hxx>
#include <ice/app.hxx>
#include <atomic>

#include <android/native_activity.h>

namespace ice::platform::android
{

    class AndroidCore final : public ice::platform::Core, public ice::platform::Paths
    {
    public:
        static AndroidCore* global_instance;

        AndroidCore(
            ice::Allocator& alloc,
            ice::Data saved_state,
            ANativeActivity* activity
        ) noexcept;

    public: // ice::platform::Core
        auto refresh_events() noexcept -> ice::Result override { return ice::Res::E_NotImplemented; }
        auto system_events() noexcept -> ice::ShardContainer const& override { return _shards; }
        auto input_events() noexcept -> ice::Span<ice::input::DeviceEvent const> override { return {}; }

    public: // ice::platform::Paths
        auto internal_data() const noexcept -> ice::String override { return _app_internal_data; }
        auto external_data() const noexcept -> ice::String override { return _app_external_data; }
        auto save_data() const noexcept -> ice::String override { return _app_save_data; }

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
        static void* native_callback_on_save_state(ANativeActivity* activity, size_t* out_size);

    private:
        ice::Allocator& _allocator;
        ice::app::Factories _factories;

        ice::UniquePtr<ice::app::Config> _config;
        ice::UniquePtr<ice::app::State> _state;
        ice::UniquePtr<ice::app::Runtime> _runtime;

        ice::ShardContainer _shards;

        ice::TaskQueue _main_queue;
        ice::UniquePtr<ice::TaskThread> _main_thread;

        std::atomic_uint32_t _app_state;
        std::atomic<AInputQueue*> _app_queue;

        ice::StaticString<256> _app_internal_data;
        ice::StaticString<256> _app_external_data;
        ice::StaticString<256> _app_save_data;
    };

} // namespace ice
