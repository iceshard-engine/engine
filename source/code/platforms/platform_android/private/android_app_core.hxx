/// Copyright 2024 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/mem_allocator_proxy.hxx>
#include <ice/container/linked_queue.hxx>
#include <ice/os.hxx>

#include <android/native_activity.h>
#include <android/native_window.h>

#include "android_render_surface.hxx"

namespace ice::platform::android
{

    enum class AndroidMessageType : uint8_t
    {
        OnInit,
        OnCreateWindow,
        OnDestroyWindow,
        OnResume,
        OnSuspend,
        OnShutdown
    };

    struct AndroidMessage
    {
        AndroidMessageType type;
        ANativeWindow* window;

        AndroidMessage* next;
    };

    enum class AndroidState : uint8_t
    {
        Initializing,
        PendingResume,
        Active,
        Suspended,
        Shutdown
    };

    class AndroidAppCore
    {
    public:
        AndroidAppCore(
            ice::Allocator& alloc,
            ANativeActivity* activity
        ) noexcept;
        virtual ~AndroidAppCore() noexcept;

        void push_message(
            ice::platform::android::AndroidMessageType msg_type,
            ANativeWindow* window = nullptr
        ) noexcept;

        void process_message(
            ice::platform::android::AndroidMessageType msg_type,
            ANativeWindow* window = nullptr
        ) noexcept;

        void process_pending_messages() noexcept;

    protected:
        virtual void on_init() noexcept = 0;
        virtual void on_setup() noexcept = 0;
        virtual void on_resume() noexcept = 0;
        virtual void on_update() noexcept = 0;
        virtual void on_suspend() noexcept = 0;
        virtual void on_shutdown() noexcept = 0;

        virtual auto native_window() noexcept -> ANativeWindow* { return _current_window; }

    private:
        void process_message(ice::platform::android::AndroidMessage const& message) noexcept;

    protected:
        ice::ProxyAllocator _allocator;

    private:
        ANativeActivity* const _current_activity;
        ANativeWindow* _current_window;
        AndroidState _current_state;

        ice::AtomicLinkedQueue<AndroidMessage> _pending_messages;
        pthread_mutex_t _message_processing_mutex;
    };

} // ice::platform::android
