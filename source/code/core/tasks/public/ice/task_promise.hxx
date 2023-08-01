/// Copyright 2023 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/assert.hxx>
#include <ice/task_promise_base.hxx>

namespace ice
{

    template<typename Value>
    class TaskPromise final : public ice::TaskPromiseBase
    {
    public:
        inline auto get_return_object() noexcept -> ice::Task<Value>;

        template<typename Other = Value>
            requires std::is_nothrow_move_assignable_v<Value> && std::is_nothrow_convertible_v<Other&&, Value>
        inline void return_value(Other&& value) noexcept
        {
            static_assert(std::is_nothrow_move_assignable_v<Value>, "We enforce noexcept everywhere.");
            _value = std::forward<Other>(value);
        }

        inline auto result() & noexcept -> Value&
        {
            return _value;
        }

        inline auto result() && noexcept -> Value&&
        {
            return std::move(_value);
        }

    private:
        Value _value;
    };

    template<typename T>
    class TaskPromise<T&> : public ice::TaskPromiseBase
    {
    public:
        inline auto get_return_object() noexcept -> ice::Task<T&>;

        void return_value(T& value) noexcept
        {
            _value = ice::addressof(value);
        }

        auto result() const noexcept -> T&
        {
            ICE_ASSERT(_value != nullptr, "Trying to access result from unfinished task!");
            return *_value;
        }

    private:
        T* _value = nullptr;
    };

    template<>
    class TaskPromise<void> : public ice::TaskPromiseBase
    {
    public:
        inline auto get_return_object() noexcept -> ice::Task<void>;

        void return_void() const noexcept { }

        void result() const noexcept { }
    };

} // namespace ice
