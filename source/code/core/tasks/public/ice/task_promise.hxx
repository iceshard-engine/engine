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
            auto initial_suspend() noexcept
            {
                return std::suspend_always{};
            }

            auto final_suspend() noexcept
            {
                return std::suspend_always{};
            }

        protected:
            TaskPromiseBase() noexcept = default;
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
