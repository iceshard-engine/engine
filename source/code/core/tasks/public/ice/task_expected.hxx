/// Copyright 2024 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/task_expected_promise.hxx>

namespace ice
{

    template<typename Result, typename ErrorType>
    struct TaskExpected final
    {
    public:
        using ValueType = Result;
        using PromiseType = ice::TaskExpectedPromise<Result, ErrorType>;
        using CoroutineType = ice::coroutine_handle<PromiseType>;

    public:
        TaskExpected(CoroutineType coro = nullptr) noexcept;
        ~TaskExpected() noexcept;

        inline TaskExpected(TaskExpected const&) noexcept = delete;
        inline auto operator=(TaskExpected const&) noexcept = delete;

        inline TaskExpected(TaskExpected&&) noexcept;
        inline auto operator=(TaskExpected&& other) noexcept -> TaskExpected&;

        //! \returns 'true' if the task was successful.
        //! \note Allows to handle errors explicitly and does not suspend the calling coroutine on error.
        inline auto valid() const noexcept;

        //! \returns The result value stored when task was successful.
        inline auto result() & noexcept -> Result&;

        //! \returns The result value stored when task was successful.
        inline auto result() && noexcept -> Result&&;

        //! \returns The error value stored when task has failed.
        inline auto error() const noexcept -> ErrorType;

        //! \brief Awaits the task and returns the result if successful.
        //!   On failure, resumes the top most coroutine that handles the error explicitly.
        //! \note If there is no explicit error handling the whole coroutine stack will be suspended and destroyed.
        //! \returns The Result value on success.
        inline auto operator co_await() & noexcept;

        //! \brief Awaits the task and returns the result if successful.
        //!   On failure, resumes the top most coroutine that handles the error explicitly.
        //! \note If there is no explicit error handling the whole coroutine stack will be suspended and destroyed.
        //! \returns The Result value on success.
        inline auto operator co_await() && noexcept;

    private:
        struct AwaitableBase
        {
            CoroutineType _coroutine;
            bool _continue_on_error;

            inline bool await_ready() const noexcept;
            inline auto await_suspend(
                CoroutineType awaiting_coroutine
            ) const noexcept -> ice::coroutine_handle<>;
            inline auto await_suspend(
                ice::coroutine_handle<> awaiting_coroutine
            ) const noexcept -> ice::coroutine_handle<>;
        };

    private:
        CoroutineType _coroutine;
    };

    template<typename Result, typename ErrorType>
    inline bool TaskExpected<Result, ErrorType>::AwaitableBase::await_ready() const noexcept
    {
        return !_coroutine || _coroutine.done();
    }

    template<typename Result, typename ErrorType>
    inline auto TaskExpected<Result, ErrorType>::AwaitableBase::await_suspend(
        CoroutineType awaiting_coroutine
    ) const noexcept -> ice::coroutine_handle<>
    {
        _coroutine.promise().set_continuation(awaiting_coroutine);
        _coroutine.promise()._error_continuation = awaiting_coroutine.promise()._error_continuation;
        _coroutine.promise()._error_pointer = awaiting_coroutine.promise()._error_pointer;
        return _coroutine;
    }

    template<typename Result, typename ErrorType>
    inline auto TaskExpected<Result, ErrorType>::AwaitableBase::await_suspend(
        ice::coroutine_handle<> awaiting_coroutine
    ) const noexcept -> ice::coroutine_handle<>
    {
        if (_continue_on_error)
        {
            _coroutine.promise()._error_continuation = awaiting_coroutine; // Allows us to control if the coroutine should continue after failure.
        }
        _coroutine.promise().set_continuation(awaiting_coroutine);
        return _coroutine;
    }

    template<typename Result, typename ErrorType>
    inline TaskExpected<Result, ErrorType>::TaskExpected(CoroutineType coro) noexcept
        : _coroutine{ coro }
    { }

    template<typename Result, typename ErrorType>
    inline TaskExpected<Result, ErrorType>::~TaskExpected() noexcept
    {
        if (_coroutine)
        {
            _coroutine.destroy();
        }
    }

    template<typename Result, typename ErrorType>
    inline TaskExpected<Result, ErrorType>::TaskExpected(TaskExpected&& other) noexcept
        : _coroutine{ ice::exchange(other._coroutine, nullptr) }
    { }

    template<typename Result, typename ErrorType>
    inline auto TaskExpected<Result, ErrorType>::operator=(TaskExpected&& other) noexcept -> TaskExpected&
    {
        if (this != &other)
        {
            if (_coroutine != nullptr)
            {
                _coroutine.destroy();
            }

            _coroutine = ice::exchange(other._coroutine, nullptr);
        }

        return *this;
    }

    template<typename Result, typename ErrorType>
    inline auto TaskExpected<Result, ErrorType>::valid() const noexcept
    {
        struct TaskAwaitable : AwaitableBase
        {
            auto await_ready() const noexcept
            {
                // Never continue if it was cancelled or failed already
                return this->_coroutine.promise()._info->has_any(TaskState::Failed | TaskState::Canceled);
            }

            auto await_resume() const noexcept -> decltype(auto)
            {
                ICE_ASSERT(
                    this->_coroutine.operator bool(),
                    "Broken promise on coroutine TaskExpected!"
                );

                return this->_coroutine.promise()._info->has_any(TaskState::Succeeded);
            }
        };

        return TaskAwaitable{ _coroutine, true };
    }

    template<typename Result, typename ErrorType>
    inline auto TaskExpected<Result, ErrorType>::result() & noexcept -> Result&
    {
        ICE_ASSERT_CORE(this->_coroutine && this->_coroutine.promise()._info->has_any(TaskState::Succeeded));
        return this->_coroutine.promise().result();
    }

    template<typename Result, typename ErrorType>
    inline auto TaskExpected<Result, ErrorType>::result() && noexcept -> Result&&
    {
        ICE_ASSERT_CORE(this->_coroutine && this->_coroutine.promise()._info->has_any(TaskState::Succeeded));
        return ice::move(this->_coroutine.promise().result());
    }

    template<typename Result, typename ErrorType>
    inline auto TaskExpected<Result, ErrorType>::error() const noexcept -> ErrorType
    {
        ICE_ASSERT_CORE(this->_coroutine != nullptr);
        ice::TaskState const state = this->_coroutine.promise()._info->state;

        ICE_ASSERT_CORE(ice::has_any(state, TaskState::Failed | TaskState::Canceled));
        if constexpr (std::is_same_v<ErrorType, ice::ErrorCode>)
        {
            if (ice::has_any(state, TaskState::Canceled))
            {
                return E_TaskCanceled;
            }
        }
        else
        {
            ICE_ASSERT_CORE(ice::has_none(state, TaskState::Canceled));
        }

        return this->_coroutine.promise()._error_value;
    }

    template<typename Result, typename ErrorType>
    inline auto TaskExpected<Result, ErrorType>::operator co_await() & noexcept
    {
        struct TaskAwaitable : AwaitableBase
        {
            auto await_resume() const noexcept -> decltype(auto)
            {
                ICE_ASSERT(
                    this->_coroutine.operator bool(),
                    "Broken promise on coroutine TaskExpected!"
                );

                if constexpr (std::is_same_v<ValueType, void> == false)
                {
                    return this->_coroutine.promise().result();
                }
            }
        };

        return TaskAwaitable{ _coroutine };
    }

    template<typename Result, typename ErrorType>
    inline auto TaskExpected<Result, ErrorType>::operator co_await() && noexcept
    {
        struct TaskAwaitable : AwaitableBase
        {
            auto await_resume() const noexcept -> decltype(auto)
            {
                ICE_ASSERT(
                    this->_coroutine.operator bool(),
                    "Broken promise on coroutine TaskExpected!"
                );

                if constexpr (std::is_same_v<ValueType, void> == false)
                {
                    return ice::move(this->_coroutine.promise().result());
                }
            }
        };

        return TaskAwaitable{ ice::move(_coroutine) };
    }

    template<typename Result, typename ErrorType>
    inline auto TaskExpectedPromise<Result, ErrorType>::get_return_object() noexcept -> ice::TaskExpected<Result, ErrorType>
    {
        return ice::TaskExpected<Result, ErrorType>{ ice::coroutine_handle<ice::TaskExpectedPromise<Result, ErrorType>>::from_promise(*this) };
    }

} // namespace ice

template<typename Result, typename ErrorType, typename... Args>
struct std::coroutine_traits<ice::TaskExpected<Result, ErrorType>, Args...>
{
    using promise_type = typename ice::TaskExpected<Result, ErrorType>::PromiseType;
};
