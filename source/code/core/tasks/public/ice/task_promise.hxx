#pragma once
#include <ice/base.hxx>
#include <ice/string.hxx>
#include <ice/assert.hxx>
#include <coroutine>

namespace ice
{

    template<typename T>
    class Task;

    namespace detail
    {

        class TaskPromiseBase
        {
        public:
            friend struct FinalAwaitable;

            struct FinalAwaitable
            {
                bool await_ready() const noexcept { return false; }

                template<typename Promise>
                auto await_suspend(std::coroutine_handle<Promise> coro) noexcept -> std::coroutine_handle<>
                {
                    return coro.promise()._continuation;
                }

                void await_resume() noexcept { }
            };

            auto initial_suspend() noexcept
            {
                return std::suspend_always{};
            }

            auto final_suspend() noexcept
            {
                return FinalAwaitable{};
            }

            auto set_continuation(std::coroutine_handle<> coro) noexcept
            {
                _continuation = coro;
            }

            auto continuation() const noexcept -> std::coroutine_handle<>
            {
                return _continuation;
            }

        protected:
            TaskPromiseBase() noexcept = default;

        protected:
            std::coroutine_handle<> _continuation;
        };

        template<typename T>
        class TaskPromise final : public TaskPromiseBase
        {
        public:
            TaskPromise() noexcept = default;

            ~TaskPromise() noexcept
            {
                _value.~T();
            }

            inline auto get_return_object() noexcept -> Task<T>;

            void unhandled_exception() noexcept
            {
                ICE_ASSERT(false, "Unhandled exception in Task object!");
            }

            template<typename Value, typename = std::enable_if_t<std::is_convertible_v<Value&&, T>>>
            void return_value(Value&& value) noexcept(std::is_nothrow_constructible_v<T, Value&&>)
            {
                ::new (static_cast<void*>(ice::addressof(_value))) T{ ice::forward<Value>(value) };
            }

            auto result() & noexcept -> T&
            {
                return _value;
            }

            auto result() && noexcept -> T&&
            {
                return std::move(_value);
            }

        private:
            T _value;
        };

        template<>
        class TaskPromise<void> : public TaskPromiseBase
        {
        public:
            TaskPromise() noexcept = default;

            inline auto get_return_object() noexcept -> Task<void>;

            void return_void() noexcept { }

            void unhandled_exception() noexcept
            {
                ICE_ASSERT(false, "Unhandled exception in Task object!");
            }

            void result() { }
        };

        template<typename T>
        class TaskPromise<T&> : public TaskPromiseBase
        {
        public:
            TaskPromise() noexcept = default;

            inline auto get_return_object() noexcept -> Task<T&>;

            void unhandled_exception() noexcept
            {
                ICE_ASSERT(false, "Unhandled exception in Task object!");
            }

            void return_value(T& value) noexcept
            {
                _value = ice::addressof(value);
            }

            auto result() noexcept -> T&
            {
                return *_value;
            }

        private:
            T* _value = nullptr;
        };

    } // namespace detail

} // namespace ice
