/// Copyright 2023 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/task_handle.hxx>

namespace ice
{

    struct TaskCancelationToken : TaskTokenBase
    {
        using TaskTokenBase::TaskTokenBase;

        inline bool was_cancelled() const noexcept { _handle.was_cancelled(); }

        inline auto checkpoint() const noexcept;

        template<typename Fn>
        inline auto checkpoint(Fn&& on_cancel_cb) const noexcept;
    };

    inline auto TaskCancelationToken::checkpoint() const noexcept
    {
        struct Awaitable
        {
            ice::TaskState state;

            // Skip checkpoint if the task was not canceled.
            inline bool await_ready() const noexcept { return ice::has_none(state, TaskState::Canceled); }

            // If we where canceled destroy the coroutine to clean everything up and make the Task's coroutine handle object invalid.
            inline auto await_suspend(std::coroutine_handle<> coro) const noexcept -> ice::coroutine_handle<>
            {
                // Get the base promise type for all of our coroutines. (We assume the TaskPromiseBase is always the base type)
                auto const promise_coro = ice::coroutine_handle<ice::TaskPromiseBase>::from_address(coro.address());

                // If there is a continuation, resume into it
                if (ice::coroutine_handle<> const continuation = promise_coro.promise().continuation(); continuation)
                {
                    return continuation;
                }
                return std::noop_coroutine();
            }

            constexpr void await_resume() const noexcept
            {
            }

        } await{ _handle.state() };
        return await;
    }

    template<typename Fn>
    inline auto TaskCancelationToken::checkpoint(Fn&& on_cancel_cb) const noexcept
    {
        struct Awaitable
        {
            ice::TaskState state;
            Fn _fn;

            // Skip checkpoint if the task was not canceled.
            inline bool await_ready() const noexcept { return ice::has_none(state, TaskState::Canceled); }

            // If we where canceled destroy the coroutine to clean everything up and make the Task's coroutine handle object invalid.
            inline auto await_suspend(std::coroutine_handle<> coro) const noexcept -> ice::coroutine_handle<>
            {
                _fn(); // We already know we are cancelled, can be used to cleanup

                // Get the base promise type for all of our coroutines. (We assume the TaskPromiseBase is always the base type)
                auto const promise_coro = ice::coroutine_handle<ice::TaskPromiseBase>::from_address(coro.address());

                // If there is a continuation, resume into it
                if (ice::coroutine_handle<> const continuation = promise_coro.promise().continuation(); continuation)
                {
                    return continuation;
                }
                return std::noop_coroutine();
            }

            constexpr void await_resume() const noexcept
            {
            }

        } await{ _handle.state(), ice::forward<Fn>(on_cancel_cb) };
        return await;
    }

} // namespace ice

// Free function traits
template<typename Result, typename... Args>
struct std::coroutine_traits<ice::Task<Result>, ice::TaskCancelationToken, Args...>
{
    using promise_type = ice::TaskInfoPromise<Result, Args...>;
};

// Member function traits
template<typename Result, typename Class, typename... Args>
struct std::coroutine_traits<ice::Task<Result>, Class, ice::TaskCancelationToken, Args...>
{
    using promise_type = ice::TaskInfoPromise<Result, Args...>;
};
