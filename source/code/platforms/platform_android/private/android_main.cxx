/// Copyright 2023 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT
#include "android_app.hxx"

#include <ice/mem_allocator_host.hxx>
#include <android/log.h>

extern "C"
{

void ANativeActivity_onCreate(
    ANativeActivity* activity,
    void* saved_data,
    size_t saved_size
)
{
    if constexpr (ice::build::is_release == false)
    {
        // System specific initializations
        if (__builtin_available(android 30, *))
        {
            // Because we handle priorities by ourselfs, just use verbose logging as default
            __android_log_set_minimum_priority(ANDROID_LOG_DEBUG);
        }
    }

    using ice::platform::android::AndroidApp;

    static ice::HostAllocator host_alloc;
    activity->instance = host_alloc.create<AndroidApp>(
        host_alloc,
        ice::Data{ saved_data, ice::usize{ saved_size } },
        activity
    );

    // Setup all callbacks
    activity->callbacks->onStart = AndroidApp::native_callback_on_start;
    activity->callbacks->onResume = AndroidApp::native_callback_on_resume;
    activity->callbacks->onPause = AndroidApp::native_callback_on_pause;
    activity->callbacks->onStop = AndroidApp::native_callback_on_stop;
    activity->callbacks->onDestroy = AndroidApp::native_callback_on_destroy;
    activity->callbacks->onWindowFocusChanged = AndroidApp::native_callback_on_focus_changed;
    activity->callbacks->onInputQueueCreated = AndroidApp::native_inputqueue_on_created;
    activity->callbacks->onInputQueueDestroyed = AndroidApp::native_inputqueue_on_destroyed;
    activity->callbacks->onNativeWindowCreated = AndroidApp::native_window_on_created;
    activity->callbacks->onNativeWindowDestroyed = AndroidApp::native_window_on_destroyed;
    activity->callbacks->onNativeWindowRedrawNeeded = AndroidApp::native_window_on_redraw_needed;
    activity->callbacks->onNativeWindowResized = AndroidApp::native_window_on_resized;
    activity->callbacks->onLowMemory = AndroidApp::native_callback_on_low_memory;
    activity->callbacks->onConfigurationChanged = AndroidApp::native_callback_on_configuration_changed;
    activity->callbacks->onContentRectChanged = AndroidApp::native_callback_on_rect_changed;
    activity->callbacks->onSaveInstanceState = AndroidApp::native_callback_on_save_state;
}

}
