/// Copyright 2023 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/task_info.hxx>
#include <ice/task_promise.hxx>

namespace ice
{

    //! \brief Special handle accessing task information at runtime.
    //!
    //! \note Accessing handles can only be done on task creation, for coroutines that define `ice::TaskToken` as one of their arguments.
    //!   In allo ther cases the task will not bookkeep any information related it's lifetime.
    struct TaskHandle final
    {
        inline TaskHandle() noexcept;
        inline TaskHandle(TaskHandle&& other) noexcept;
        inline TaskHandle(TaskHandle const& other) noexcept;
        inline ~TaskHandle() noexcept;

        inline auto operator=(TaskHandle&& other) noexcept -> TaskHandle&;
        inline auto operator=(TaskHandle const& other) noexcept -> TaskHandle&;

        //! \return The current state of the assigned task or 'TaskState::Invalid' if not task was assigned to this handle.
        auto state() const noexcept
        {
            return _info == nullptr ? TaskState::Invalid : _info->state.load(std::memory_order_relaxed);
        }

        //! \return 'true' if the handle was assigned to a task object.
        //! \pre The handle needs to be assigned to a valid task.
        bool is_valid() const noexcept { return state() != TaskState::None; }

        //! \return 'true' if the state value contains the 'Running' flag.
        //! \pre The handle needs to be assigned to a valid task.
        bool is_running() const noexcept { return _info->has_any(TaskState::Running); }

        //! \return 'true' if the state value contains the 'Suspended' flag.
        //! \note This flag is currently not set.
        //! \pre The handle needs to be assigned to a valid task.
        bool is_suspended() const noexcept { return _info->has_any(TaskState::Suspended); }

        //! \return 'true' if the state value contains the 'Canceled' flag.
        //! \pre The handle needs to be assigned to a valid task.
        bool was_cancelled() const noexcept { return _info->has_any(TaskState::Canceled); }

        //! \return 'true' if the state value contains either 'Succeeded' or 'Failed' flags.
        //! \pre The handle needs to be assigned to a valid task.
        bool has_finished() const noexcept { return _info->has_any(TaskState::Succeeded | TaskState::Failed); }

        //! \return 'true' if the state value contains the 'Succeeded' flag.
        //! \pre The handle needs to be assigned to a valid task.
        bool has_succeded() const noexcept { return _info->has_any(TaskState::Succeeded); }

        //! \return 'true' if the state value contains the 'Failed' flag.
        //! \pre The handle needs to be assigned to a valid task.
        bool has_failed() const noexcept { return _info->has_any(TaskState::Failed); }

        //! \brief Sends a cancel request to the connected task.
        //! \returns 'true' if the handle was valid and the task was in the 'Running' state.
        inline bool cancel() noexcept;

        ice::TaskInfo* _info;
    };

    namespace detail
    {

        inline bool try_set_canceled_state(std::atomic<ice::TaskState>& state) noexcept
        {
            bool success = false;
            ice::TaskState expected = state.load(std::memory_order_relaxed);
            while(ice::has_any(expected, TaskState::Running | TaskState::Created) && success == false)
            {
                success = state.compare_exchange_weak(
                    expected,
                    expected | TaskState::Canceled,
                    std::memory_order_relaxed,
                    std::memory_order_relaxed
                );

                if (ice::has_any(expected, TaskState::Succeeded | TaskState::Failed))
                {
                    break;
                }
            }
            return success;
        }

    } // namespace detail


    inline TaskHandle::TaskHandle() noexcept
        : _info{ nullptr }
    { }

    inline TaskHandle::TaskHandle(TaskHandle&& other) noexcept
        : _info{ ice::exchange(other._info, nullptr) }
    { }

    inline TaskHandle::TaskHandle(TaskHandle const& other) noexcept
        : _info{ other._info->aquire() }
    { }

    inline TaskHandle::~TaskHandle() noexcept
    {
        if (_info != nullptr) _info->release();
    }

    inline auto TaskHandle::operator=(TaskHandle&& other) noexcept -> TaskHandle&
    {
        if (this != ice::addressof(other))
        {
            _info->release();
            _info = ice::exchange(other._info, nullptr);
        }
        return *this;
    }

    inline auto TaskHandle::operator=(TaskHandle const& other) noexcept -> TaskHandle&
    {
        if (this != ice::addressof(other))
        {
            _info->release();
            _info = other._info->aquire();
        }
        return *this;
    }

    inline bool TaskHandle::cancel() noexcept
    {
        bool success = false;
        if (_info != nullptr)
        {
            success = ice::detail::try_set_canceled_state(_info->state);
        }
        return success;
    }

    template<typename Result>
    struct TaskInfoPromise : public ice::TaskPromise<Result>
    {
        ice::TaskInfo* _info;

        TaskInfoPromise() noexcept = default;

        template<typename... Args>
        TaskInfoPromise(ice::TaskHandle& handle, Args const&...) noexcept
            : _info{ new ice::TaskInfo{} }
        {
            if (handle._info)
            {
                handle._info->release();
            }
            handle._info = _info->aquire();
        }

        template<typename Class, typename... Args>
        TaskInfoPromise(Class const&, ice::TaskHandle& handle, Args const&...) noexcept
            : _info{ new ice::TaskInfo{} }
        {
            if (handle._info)
            {
                handle._info->release();
            }
            handle._info = _info->aquire();
        }

        ~TaskInfoPromise() noexcept
        {
            if (_info != nullptr)
            {
                if (_info->has_any(ice::TaskState::Canceled))
                {
                    _info->state.store(ice::TaskState::Canceled | ice::TaskState::Failed, std::memory_order_relaxed);
                }
                _info->release();
            }
        }

        struct ExtendedFinalAwaitable : ice::TaskPromise<Result>::FinalAwaitable
        {
            template<typename Promise>
            inline auto await_suspend(ice::coroutine_handle<Promise> coro) noexcept
            {
                ice::TaskInfo* const info = coro.promise()._info;
                if (info != nullptr)
                {
                    if (info->has_any(ice::TaskState::Canceled))
                    {
                        info->state.store(ice::TaskState::Succeeded | ice::TaskState::Canceled);
                    }
                    else
                    {
                        info->state.store(ice::TaskState::Succeeded);
                    }
                }

                return ice::TaskPromise<Result>::FinalAwaitable::template await_suspend<Promise>(coro);
            }
        };

        struct InitialAwaitable
        {
            ice::TaskInfo* _info;

            constexpr bool await_ready() const noexcept { return this->_info->has_any(ice::TaskState::Canceled); }

            constexpr void await_suspend(ice::coroutine_handle<>) const noexcept { }

            inline void await_resume() const noexcept
            {
                if (this->_info != nullptr)
                {
                    ICE_ASSERT_CORE(this->_info->has_any(ice::TaskState::Canceled) == false);
                    this->_info->state.store(ice::TaskState::Running);
                }
            }
        };

        inline auto initial_suspend() const noexcept
        {
            return InitialAwaitable{ this->_info };
        }

        inline auto final_suspend() const noexcept
        {
            return ExtendedFinalAwaitable{ };
        }
    };

    struct TaskTokenBase
    {
        TaskTokenBase(ice::TaskHandle& handle) noexcept : _handle{ handle } { }
        ~TaskTokenBase() noexcept = default;

        inline operator ice::TaskHandle&() noexcept { return _handle; }

        ice::TaskHandle& _handle;
    };


} // namespace ice

// Free function traits
template<typename Result, typename... Args>
struct std::coroutine_traits<ice::Task<Result>, ice::TaskHandle&, Args...>
{
    using promise_type = ice::TaskInfoPromise<Result>;
};

// Member function traits
template<typename Result, typename Class, typename... Args>
struct std::coroutine_traits<ice::Task<Result>, Class, ice::TaskHandle&, Args...>
{
    using promise_type = ice::TaskInfoPromise<Result>;
};
