#pragma once
#include <ice/task_handle.hxx>

namespace ice
{

    struct TaskCancelationToken
    {
        TaskCancelationToken(ice::TaskHandle& handle) noexcept : handle{ handle } { }
        ~TaskCancelationToken() noexcept = default;

        inline auto checkpoint() const noexcept;

        ice::TaskHandle& handle;
    };

    inline auto TaskCancelationToken::checkpoint() const noexcept
    {
        struct Awaitable
        {
            ice::TaskState state;

            // Skip checkpoint if the task was not canceled.
            inline bool await_ready() const noexcept { return ice::has_none(state, TaskState::Canceled); }

            // If we where canceled destroy the coroutine to clean everything up and make the Task's coroutine handle object invalid.
            inline void await_suspend(std::coroutine_handle<> coro) const noexcept
            {
                // Destroy the coroutine here?
                coro.destroy();
            }

            constexpr void await_resume() const noexcept
            {
            }

        } await{ handle.state() };
        return await;
    }

} // namespace ice

template<typename Result, typename... Args>
struct std::coroutine_traits<ice::Task<Result>, ice::TaskCancelationToken, Args...>
{
    struct ExtendedPromise : public ice::Task<Result>::PromiseType
    {
        ice::TaskInfo* _info;

        ExtendedPromise() noexcept = default;
        ExtendedPromise(ice::TaskCancelationToken& token) noexcept
        {
            this->_info = new ice::TaskInfo{};
            if (token.handle._info)
            {
                token.handle._info->release();
            }
            token.handle._info = this->_info->aquire();
        }

        ~ExtendedPromise() noexcept
        {
            if (this->_info != nullptr)
            {
                if (this->_info->has_any(ice::TaskState::Canceled))
                {
                    this->_info->state.store(ice::TaskState::Canceled | ice::TaskState::Failed, std::memory_order_relaxed);
                }
                this->_info->release();
            }
        }

        struct ExtendedFinalAwaitable : ice::Task<Result>::PromiseType::FinalAwaitable
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

                return ice::Task<Result>::PromiseType::FinalAwaitable::await_suspend<Promise>(coro);
            }
        };

        struct InitialAwaitable
        {
            ice::TaskInfo* _info;

            constexpr bool await_ready() const noexcept { return false; }

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

    using promise_type = ExtendedPromise;
};
