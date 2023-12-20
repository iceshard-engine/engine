/// Copyright 2023 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "task_native_thread.hxx"
#include <ice/task_queue.hxx>
#include <ice/os/windows.hxx>
#include <ice/os/unix.hxx>
#include <ice/mem_allocator_stack.hxx>
#include <ice/string_utils.hxx>
#include <ice/assert.hxx>
#include <ice/sort.hxx>

namespace ice
{

    NativeTaskThread::NativeTaskThread(
        ice::TaskQueue& queue,
        ice::TaskThreadInfo const& info
    ) noexcept
        : _info{ info }
        , _runtime{
            ._info = _info,
            ._queue = queue,
            ._state = ThreadState::Invalid,
            ._request = ThreadRequest::Create
        }
        , _native{ nullptr }
    {
        _native = thread_native::create_thread(*this);
    }

    NativeTaskThread::~NativeTaskThread() noexcept
    {
        // Wait for the thread to finish any pending request
        while (_runtime._request != ThreadRequest::None)
        {
            thread_native::sleep(1);
        }

        // Set destroy request
        _runtime._request = ThreadRequest::Destroy;
        while (_runtime._state != ThreadState::Destroyed)
        {
            thread_native::sleep(1);
        }

        thread_native::destroy_thread(_native);
    }

    bool NativeTaskThread::valid() noexcept
    {
        return _native.native != nullptr;
    }

    auto NativeTaskThread::runtime() noexcept -> ice::ThreadRuntime&
    {
        return _runtime;
    }

    auto NativeTaskThread::info() const noexcept -> ice::TaskThreadInfo const&
    {
        return _info;
    }

    bool NativeTaskThread::is_busy() const noexcept
    {
        return is_running() && estimated_task_count() > 0;
    }

    bool NativeTaskThread::is_running() const noexcept
    {
        return _runtime._state == ThreadState::Active;
    }

    auto NativeTaskThread::estimated_task_count() const noexcept -> ice::ucount
    {
        return 0;
    }

    auto NativeTaskThread::queue() noexcept -> ice::TaskQueue&
    {
        return _runtime._queue;
    }

    template<bool BusyWait>
    auto ThreadRuntime::thread_procedure(RoutineFn routine) noexcept -> ice::u32
    {
        ice::u32 result = 0;
        ice::u32 busy_loop = thread_native::Constant_BusyLoopCount;
        while (_request != ThreadRequest::Destroy)
        {
            if constexpr (BusyWait)
            {
                if (ice::linked_queue::empty(_queue._awaitables))
                {
                    if (busy_loop > 0)
                    {
                        busy_loop -= 1;
                    }
                    else
                    {
                        ice::thread_native::yield();
                    }
                    continue;
                }

                // Reset the busy loop value
                busy_loop = thread_native::Constant_BusyLoopCount;
            }

            // Run the selected routine
            result = (this->*routine)();
        }
        return result;
    }

    auto ThreadRuntime::custom_routine() noexcept -> ice::u32
    {
        return _info.custom_procedure(_info.custom_procedure_userdata, _queue);
    }

    auto ThreadRuntime::shared_routine() noexcept -> ice::u32
    {
        // Get the task nodes and ensure we are can access all of them.
        ice::TaskAwaitableBase* const task = ice::linked_queue::pop(_queue._awaitables);

        // Execute all tasks
        if (task != nullptr)
        {
            // Coroutines should never be destroyed from here.
            task->_coro.resume();
        }
        return 0;
    }

    auto ThreadRuntime::exclusive_fifo_routine() noexcept -> ice::u32
    {
        // Get the task nodes and ensure we are can access all of them.
        ice::LinkedQueueRange<ice::TaskAwaitableBase> tasks = ice::linked_queue::consume(_queue._awaitables);

        // Execute all tasks
        for (ice::TaskAwaitableBase* task_node : tasks)
        {
            // Coroutines should never be destroyed from here.
            task_node->_coro.resume();
        }
        return 0;
    }

    auto ThreadRuntime::exclusive_sorted_routine() noexcept -> ice::u32
    {
        // Get the task nodes and ensure we are can access all of them.
        ice::LinkedQueueRange<ice::TaskAwaitableBase> tasks = ice::linked_queue::consume(_queue._awaitables);

        ice::u32 count = 0;
        ice::TaskAwaitableBase* head = tasks._head;
        while (head != tasks._tail)
        {
            // If we are not at 'tail' and encounter a nullptr, this means some thread did not write it's 'next' member yet.
            while (head->next == nullptr)
            {
                ice::thread_native::yield();
            }

            head = head->next;
            count += 1;
        }

        auto constexpr predicate = [](ice::TaskAwaitableBase& left, ice::TaskAwaitableBase& right) noexcept -> bool
        {
            ice::TaskFlags left_flags{ };
            ice::TaskFlags right_flags{ };

            // TODO: Improve logic for this!
            if (left._params.modifier == TaskAwaitableModifier::PriorityFlags)
            {
                left_flags.value = left._params.task_flags.value & 0xf;
            }
            if (right._params.modifier == TaskAwaitableModifier::PriorityFlags)
            {
                right_flags.value = right._params.task_flags.value & 0xf;
            }

            return left_flags.value > right_flags.value;
        };

        // Sort the list
        tasks._head = ice::sort_linked_list(tasks._head, count, predicate);

        // Update the head and tail pointers.
        tasks._tail = tasks._head;
        while (tasks._tail->next != nullptr)
        {
            tasks._tail = tasks._tail->next;
        }

        // Execute all tasks
        for (ice::TaskAwaitableBase* task_node : tasks)
        {
            // Coroutines should never be destroyed from here.
            task_node->_coro.resume();
        }
        return 0;
    }

#if ISP_WINDOWS

    namespace thread_native
    {

        DWORD native_thread_routine(void* userdata)
        {
            ice::NativeTaskThread* const thread_obj = reinterpret_cast<ice::NativeTaskThread*>(userdata);
            ice::TaskThreadInfo const& thread_info = thread_obj->info();
            ice::ThreadRuntime& runtime = thread_obj->runtime();

            ICE_ASSERT(
                runtime._request == ThreadRequest::Create && runtime._state == ThreadState::Invalid,
                "Entering thread routine from invalid state!"
            );
            runtime._state = ThreadState::Active;
            runtime._request = ThreadRequest::None;

            ice::u32 result;

            // Runs the custom user procedure.
            if (thread_info.custom_procedure)
            {
                result = runtime.thread_procedure<false>(&ThreadRuntime::custom_routine);
            }
            // Runs either of the two routines for the thread.
            else if (thread_info.exclusive_queue)
            {
                if (thread_info.sort_by_priority)
                {
                    result = runtime.thread_procedure<true>(&ThreadRuntime::exclusive_sorted_routine);
                }
                else
                {
                    result = runtime.thread_procedure<true>(&ThreadRuntime::exclusive_fifo_routine);
                }
            }
            else
            {
                result = runtime.thread_procedure<true>(&ThreadRuntime::shared_routine);
            }

            runtime._state = ThreadState::Destroyed;
            return result;
        }

        auto create_thread(
            ice::NativeTaskThread& native_thread
        ) noexcept -> Handle
        {
            ice::TaskThreadInfo const& info = native_thread.info();

            HANDLE thread_handle = CreateThread(
                nullptr,
                info.stack_size.value,
                &native_thread_routine,
                &native_thread,
                0,
                nullptr
            );

            if constexpr (ice::build::is_release == false)
            {
                if (ice::string::any(info.debug_name))
                {
                    ice::StackAllocator<256_B> stack_alloc;
                    ice::ucount const wide_count = ice::utf8_to_wide_size(info.debug_name);
                    ICE_ASSERT(
                        ice::size_of<ice::wchar> *(wide_count + 1) < stack_alloc.Constant_InternalCapacity,
                        "Thread debug name too long!"
                    );

                    ice::HeapString<ice::wchar> wide_name{ stack_alloc };
                    ice::utf8_to_wide_append(info.debug_name, wide_name);
                    SetThreadDescription(thread_handle, wide_name._data);
                }
            }

            return { thread_handle };
        }

        void destroy_thread(Handle native_handle) noexcept
        {
            CloseHandle(native_handle.native);
        }

        void sleep(ice::u32 ms) noexcept
        {
            SleepEx(ms, FALSE);
        }

        void yield() noexcept
        {
            SwitchToThread();
        }

    } // namespace native

#elif ISP_UNIX

    namespace thread_native
    {

        void* native_thread_routine(void* userdata)
        {
            ice::NativeTaskThread* const thread_obj = reinterpret_cast<ice::NativeTaskThread*>(userdata);
            ice::TaskThreadInfo const& thread_info = thread_obj->info();
            ice::ThreadRuntime& runtime = thread_obj->runtime();

            ICE_ASSERT(
                runtime._request == ThreadRequest::Create && runtime._state == ThreadState::Invalid,
                "Entering thread routine from invalid state!"
            );
            runtime._state = ThreadState::Active;
            runtime._request = ThreadRequest::None;

            ice::uptr result;

            // Runs the custom user procedure.
            if (thread_info.custom_procedure)
            {
                result = runtime.thread_procedure<false>(&ThreadRuntime::custom_routine);
            }
            // Runs either of the two routines for the thread.
            else if (thread_info.exclusive_queue)
            {
                if (thread_info.sort_by_priority)
                {
                    result = runtime.thread_procedure<true>(&ThreadRuntime::exclusive_sorted_routine);
                }
                else
                {
                    result = runtime.thread_procedure<true>(&ThreadRuntime::exclusive_fifo_routine);
                }
            }
            else
            {
                result = runtime.thread_procedure<true>(&ThreadRuntime::shared_routine);
            }

            runtime._state = ThreadState::Destroyed;
            return ice::bit_cast<void*>(result);
        }

        auto create_thread(
            ice::NativeTaskThread& native_thread
        ) noexcept -> Handle
        {
            ice::TaskThreadInfo const& info = native_thread.info();

            pthread_attr_t thread_attribs;
            int error = pthread_attr_init(&thread_attribs);
            ICE_ASSERT(error == 0, "Failed to initialize thread attributes with error: {}!", error);

            error = pthread_attr_setstacksize(
                &thread_attribs,
                ice::max(ice::usize::base_type{PTHREAD_STACK_MIN}, info.stack_size.value)
            );
            ICE_ASSERT(
                error == 0,
                "Failed to set native-thread stack size ({}) with error: {}!",
                info.stack_size.value, error
            );

            pthread_t thread_handle{};
            error = pthread_create(&thread_handle, &thread_attribs, &native_thread_routine, &native_thread);
            ICE_ASSERT(error == 0, "Failed to create native thread with error: {}!", error);

            error = pthread_attr_destroy(&thread_attribs);
            ICE_ASSERT(error == 0, "Failed to destroy thread attributes with error: {}!", error);

            if constexpr (ice::build::is_release == false)
            {
                if (ice::string::any(info.debug_name))
                {
                    error = pthread_setname_np(thread_handle, ice::string::begin(info.debug_name));
                    ICE_ASSERT(error == 0, "Failed to set name for native thread with error: {}!", error);
                }
            }

            return { ice::bit_cast<void*>(thread_handle) };
        }

        void destroy_thread(Handle native_handle) noexcept
        {
            void* result_ptrval = nullptr;
            int const error_join = pthread_join(std::bit_cast<pthread_t>(native_handle.native), &result_ptrval);
            ICE_ASSERT(error_join == 0, "Failed to join pthread with error: {}!", error_join);

            ice::uptr const result = ice::bit_cast<ice::uptr>(result_ptrval);
            ICE_ASSERT(result == 0, "Native thread exited with unexpected result: {}!", result);
        }

        void sleep(ice::u32 ms) noexcept
        {
            // Sleep in milliseconds
            int const error = usleep(ms * 1000);
            ICE_ASSERT(error == 0, "Failed to sleep thread for {}ms!", ms);
        }

        void yield() noexcept
        {
            int const error = sched_yield();
            ICE_ASSERT(error == 0, "Failed to yield thread to the system scheduler!");
        }

    } // namespace native

#else
#   error "Unknown platform!"
#endif

} // namespace ice
