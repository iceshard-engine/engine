/// Copyright 2024 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/task_types.hxx>
#include <ice/task_handle.hxx>
#include <ice/expected.hxx>

namespace ice
{

    template<typename Result, typename ErrorType>
    struct TaskExpected;

    template<typename Result, typename ErrorType>
    struct TaskExpectedPromise final : public ice::TaskInfoPromise<ice::TaskExpected<Result, ErrorType>>
    {
        TaskExpectedPromise() noexcept
        {
            this->_info = new TaskInfo{ };
        }

        ~TaskExpectedPromise() noexcept
        {
            if (this->_info->has_any(TaskState::Succeeded))
            {
                // Destroy the object if created
                result().~Result();
            }
        }

        // Support all other TaskInfo tag extensions. (ex.: TaskCancelationToken)
        using TaskInfoPromise<ice::TaskExpected<Result, ErrorType>>::TaskInfoPromise;

        inline auto get_return_object() noexcept -> ice::TaskExpected<Result, ErrorType>;

        struct ExpectedFinalAwaiter : public ice::TaskPromiseBase::FinalAwaitable
        {
            constexpr bool await_ready() const noexcept { return false; }

            template<typename Promise>
            inline auto await_suspend(std::coroutine_handle<Promise> coro) noexcept -> std::coroutine_handle<>
            {
                TaskInfo* const info = coro.promise()._info;

                if (info->has_any(TaskState::Canceled))
                {
                    info->state.store(TaskState::Failed | TaskState::Canceled, std::memory_order_relaxed);
                }

                // Don't resume anything when we got canceled or failed
                if (info->has_any(TaskState::Failed))
                {
                    return coro.promise()._error_continuation;
                }

                // Mark as successful
                info->state.store(TaskState::Succeeded, std::memory_order_release);

                // Call the parent suspend
                return TaskPromiseBase::FinalAwaitable::await_suspend(coro);
            }

            constexpr void await_resume() const noexcept { }
        };

        constexpr auto final_suspend() noexcept
        {
            return ExpectedFinalAwaiter{ };
        }

        template<typename TypeExpected> requires (std::is_same_v<ice::clean_type<TypeExpected>, ice::Expected<Result, ErrorType>>)
        inline void return_value(TypeExpected&& expected) noexcept(std::is_nothrow_move_constructible_v<Result>)
        {
            ICE_ASSERT_CORE(expected.valid());
            if (expected.succeeded())
            {
                new (&_value) Result{ ice::forward<TypeExpected>(expected).value() };
            }
            else
            {
                this->_info->state.store(TaskState::Failed);

                ICE_ASSERT_CORE(_error_pointer != nullptr);
                *_error_pointer = ice::forward<TypeExpected>(expected).error();
            }
        }

        inline void return_value(ErrorType error) noexcept
        {
            this->_info->state.store(TaskState::Failed);

            ICE_ASSERT_CORE(_error_pointer != nullptr);
            *_error_pointer = error;
        }

        inline void return_value(Result const& value) noexcept(std::is_nothrow_copy_constructible_v<Result>)
        {
            new (&_value) Result{ value };
        }

        inline void return_value(Result&& value) noexcept(std::is_nothrow_move_constructible_v<Result>)
        {
            new (&_value) Result{ std::move(value) };
        }

        inline auto expected() noexcept -> ice::Expected<Result, ErrorType>
        {
            if (this->_info->has_any(TaskState::Succeeded))
            {
                return *reinterpret_cast<Result*>(_value);
            }
            else
            {
                return _error_value;
            }
        }

        // TODO: Cleanup-this-mess
        inline auto expected_moved() noexcept -> ice::Expected<Result, ErrorType>
        {
            if (this->_info->has_any(TaskState::Succeeded))
            {
                return ice::move(*reinterpret_cast<Result*>(_value));
            }
            else
            {
                return _error_value;
            }
        }

        inline auto result() noexcept -> Result&
        {
            ICE_ASSERT_CORE(this->_info->has_any(TaskState::Succeeded));
            return *reinterpret_cast<Result*>(_value);
        }

        union
        {
            alignas(Result) char _value[sizeof(Result)];
            ErrorType _error_value;
        };

        ErrorType* _error_pointer{ &_error_value };
        std::coroutine_handle<> _error_continuation{ std::noop_coroutine() };
    };

} // namespace ice
