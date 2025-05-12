/// Copyright 2023 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/task_queue.hxx>
#include <ice/task_awaitable.hxx>
#include <ice/container/linked_queue.hxx>

namespace ice
{

    template<typename StageObject = void>
    struct TaskStageAwaitable
    {
        inline explicit TaskStageAwaitable(ice::TaskQueue& queue) noexcept
            : _awaitable{ ._params{.modifier = TaskAwaitableModifier::Unused} }
            , _queue{ queue }
        { }

        inline bool await_ready() const noexcept
        {
            return false;
        }

        inline auto await_suspend(ice::coroutine_handle<> coroutine) noexcept
        {
            _awaitable._coro = coroutine;
            _queue.push_back(&_awaitable);
        }

        inline auto await_resume() const noexcept -> StageObject&
        {
            return *reinterpret_cast<StageObject*>(_awaitable.result.ptr);
        }

        ice::TaskAwaitableBase _awaitable;
        ice::TaskQueue& _queue;
    };

    template<>
    struct TaskStageAwaitable<void>
    {
        inline explicit TaskStageAwaitable(ice::TaskQueue& queue) noexcept
            : _awaitable{ ._params{.modifier = TaskAwaitableModifier::Unused} }
            , _queue{ queue }
        { }

        inline bool await_ready() const noexcept
        {
            return false;
        }

        inline auto await_suspend(ice::coroutine_handle<> coroutine) noexcept
        {
            _awaitable._coro = coroutine;
            _queue.push_back(&_awaitable);
        }

        inline void await_resume() const noexcept
        {
        }

        ice::TaskAwaitableBase _awaitable;
        ice::TaskQueue& _queue;
    };

    template<typename StageResult = void>
    struct TaskStage
    {
        ice::TaskQueue& _queue;

        inline auto operator co_await() const noexcept -> ice::TaskStageAwaitable<StageResult>
        {
            return ice::TaskStageAwaitable<StageResult>{ _queue };
        }
    };

} // namespace ice
