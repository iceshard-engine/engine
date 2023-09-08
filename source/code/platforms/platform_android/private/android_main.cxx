/// Copyright 2023 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/mem_allocator_host.hxx>
#include "android_platform.hxx"

extern "C"
{

void ANativeActivity_onCreate(
    ANativeActivity* activity,
    void* saved_data,
    size_t saved_size
)
{
    using ice::platform::android::AndroidCore;

    static ice::HostAllocator host_alloc;
    activity->instance = host_alloc.create<AndroidCore>(host_alloc, ice::Data{ saved_data, ice::usize{ saved_size } });

    // Setup all callbacks
    activity->callbacks->onStart = AndroidCore::native_callback_on_start;
    activity->callbacks->onResume = AndroidCore::native_callback_on_resume;
    activity->callbacks->onPause = AndroidCore::native_callback_on_pause;
    activity->callbacks->onStop = AndroidCore::native_callback_on_stop;
    activity->callbacks->onDestroy = AndroidCore::native_callback_on_destroy;
    activity->callbacks->onWindowFocusChanged = AndroidCore::native_callback_on_focus_changed;
    activity->callbacks->onInputQueueCreated = AndroidCore::native_inputqueue_on_created;
    activity->callbacks->onInputQueueDestroyed = AndroidCore::native_inputqueue_on_destroyed;
    activity->callbacks->onNativeWindowCreated = AndroidCore::native_window_on_created;
    activity->callbacks->onNativeWindowDestroyed = AndroidCore::native_window_on_destroyed;
    activity->callbacks->onNativeWindowRedrawNeeded = AndroidCore::native_window_on_redraw_needed;
    activity->callbacks->onNativeWindowResized = AndroidCore::native_window_on_resized;
    activity->callbacks->onLowMemory = AndroidCore::native_callback_on_low_memory;
    activity->callbacks->onConfigurationChanged = AndroidCore::native_callback_on_configuration_changed;
    activity->callbacks->onContentRectChanged = AndroidCore::native_callback_on_rect_changed;
    activity->callbacks->onSaveInstanceState = AndroidCore::native_callback_on_save_state;
}

}
