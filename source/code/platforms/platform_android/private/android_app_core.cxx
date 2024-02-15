#include "android_app_core.hxx"
#include <android/native_window.h>

namespace ice::platform::android
{

    AndroidAppCore::AndroidAppCore(
        ice::Allocator& alloc,
        ANativeActivity* activity
    ) noexcept
        : _allocator{ alloc, "android-core" }
        , _current_activity{ activity }
        , _current_window{ nullptr }
        , _current_state{ AndroidState::Initializing }
        , _pending_messages{ }
    {
        pthread_mutex_init(&_message_processing_mutex, nullptr);
    }

    AndroidAppCore::~AndroidAppCore() noexcept
    {
        pthread_mutex_destroy(&_message_processing_mutex);
    }

    void AndroidAppCore::push_message(
            ice::platform::android::AndroidMessageType msg_type,
            ANativeWindow* window
    ) noexcept
    {
        AndroidMessage* message = _allocator.create<AndroidMessage>(msg_type, window, nullptr);
        ice::linked_queue::push(_pending_messages, message);
    }

    void AndroidAppCore::process_message(
            ice::platform::android::AndroidMessageType msg_type,
            ANativeWindow* window
    ) noexcept
    {
        AndroidMessage message{ msg_type, window, nullptr };
        pthread_mutex_lock(&_message_processing_mutex);
        process_message(message);
        pthread_mutex_unlock(&_message_processing_mutex);
    }

    void AndroidAppCore::process_pending_messages() noexcept
    {
        if (ice::linked_queue::any(_pending_messages))
        {
            pthread_mutex_lock(&_message_processing_mutex);
            for (AndroidMessage* message : ice::linked_queue::consume(_pending_messages))
            {
                process_message(*message);
                _allocator.destroy(message);
            }
            pthread_mutex_unlock(&_message_processing_mutex);
        }

        if (pthread_mutex_trylock(&_message_processing_mutex) == 0)
        {
            // If we are active, we can update the app
            if (_current_state == AndroidState::Active)
            {
                on_update();
            }

            pthread_mutex_unlock(&_message_processing_mutex);
        }
    }

    void AndroidAppCore::process_message(ice::platform::android::AndroidMessage const& message) noexcept
    {
        switch (message.type)
        {
        using enum AndroidMessageType;
        case OnInit:
        {
            on_init();
            break;
        }
        case OnSetup:
        {
            on_setup();
            break;
        }
        case OnCreateWindow:
        {
            ICE_ASSERT_CORE(_current_state == AndroidState::Initializing || _current_state == AndroidState::Resuming);
            if (_current_window)
            {
                ANativeWindow_release(_current_window);
            }
            _current_window = message.window;
            ICE_ASSERT_CORE(_current_window != nullptr);
            ANativeWindow_acquire(_current_window);

            // Special case for resuming the app
            if (_current_state == AndroidState::Resuming)
            {
                on_resume();
                _current_state = AndroidState::Active;
            }
            break;
        }
        case OnDestroyWindow:
        {
            ICE_ASSERT_CORE(_current_window == message.window);
            if (_current_window != nullptr)
            {
                ANativeWindow_release(ice::exchange(_current_window, nullptr));
            }
            break;
        }
        case OnResume:
        {
            ICE_ASSERT_CORE(_current_state == AndroidState::Initializing || _current_state == AndroidState::Suspended);
            if (_current_window != nullptr)
            {
                on_resume();
                _current_state = AndroidState::Active;
            }
            else
            {
                _current_state = AndroidState::Resuming;
            }
            break;
        }
        case OnSuspend:
        {
            ICE_ASSERT_CORE(_current_state == AndroidState::Active);
            on_suspend();
            _current_state = AndroidState::Suspended;
            break;
        }
        case OnShutdown:
        {
            ICE_ASSERT_CORE(_current_state == AndroidState::Suspended);
            on_shutdown();
            _current_state = AndroidState::Shutdown;
            break;
        }
        default: ICE_ASSERT_CORE(false); break;
        }
    }

} // ice::platform::android
