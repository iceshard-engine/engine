/// Copyright 2023 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/task_promise.hxx>
#include <ice/container_concepts.hxx>

namespace ice
{

    template<typename Result>
    class Task final
    {
    public:
        using ValueType = Result;
        using PromiseType = ice::TaskPromise<ValueType>;

    public:
        inline explicit Task(
            ice::coroutine_handle<PromiseType> coro = nullptr
        ) noexcept;
        inline ~Task() noexcept;

        inline Task(Task const&) noexcept = delete;
        inline auto operator=(Task const&) noexcept = delete;

        inline Task(Task&&) noexcept;
        inline auto operator=(Task&& other) noexcept -> Task&;

        inline auto operator co_await() & noexcept;
        inline auto operator co_await() && noexcept;

        bool valid() const noexcept { return _coroutine && _coroutine.done() == false; }

    private:
        struct AwaitableBase
        {
            ice::coroutine_handle<PromiseType> _coroutine;

            inline bool await_ready() const noexcept;
            inline auto await_suspend(
                ice::coroutine_handle<> awaiting_coroutine
            ) const noexcept -> ice::coroutine_handle<>;
        };

    private:
        ice::coroutine_handle<PromiseType> _coroutine;
    };

    template<typename Result>
    inline bool Task<Result>::AwaitableBase::await_ready() const noexcept
    {
        return !_coroutine || _coroutine.done();
    }

    template<typename Result>
    inline auto Task<Result>::AwaitableBase::await_suspend(
        ice::coroutine_handle<> awaiting_coroutine
    ) const noexcept -> ice::coroutine_handle<>
    {
        _coroutine.promise().set_continuation(awaiting_coroutine);
        return _coroutine;
    }

    template<typename Result>
    inline Task<Result>::Task(ice::coroutine_handle<PromiseType> coro) noexcept
        : _coroutine{ coro }
    { }

    template<typename Result>
    inline Task<Result>::~Task() noexcept
    {
        if (_coroutine)
        {
            _coroutine.destroy();
        }
    }

    template<typename Result>
    inline Task<Result>::Task(Task&& other) noexcept
        : _coroutine{ ice::exchange(other._coroutine, nullptr) }
    { }

    template<typename Result>
    inline auto Task<Result>::operator=(Task&& other) noexcept -> Task&
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

    template<typename Result>
    inline auto Task<Result>::operator co_await() & noexcept
    {
        struct TaskAwaitable : AwaitableBase
        {
            auto await_resume() const noexcept -> decltype(auto)
            {
                ICE_ASSERT(
                    this->_coroutine.operator bool(),
                    "Broken promise on coroutine Task!"
                );

                if constexpr (std::is_same_v<ValueType, void> == false)
                {
                    return this->_coroutine.promise().result();
                }
            }
        };

        return TaskAwaitable{ _coroutine };
    }

    template<typename Result>
    inline auto Task<Result>::operator co_await() && noexcept
    {
        struct TaskAwaitable : AwaitableBase
        {
            auto await_resume() const noexcept -> decltype(auto)
            {
                ICE_ASSERT(
                    this->_coroutine.operator bool(),
                    "Broken promise on coroutine Task!"
                );

                if constexpr (std::is_same_v<ValueType, void> == false)
                {
                    return ice::move(this->_coroutine.promise().result());
                }
            }
        };

        return TaskAwaitable{ ice::move(_coroutine) };
    }

    template<typename Value>
    inline auto ice::TaskPromise<Value>::get_return_object() noexcept -> ice::Task<Value>
    {
        return ice::Task<Value>{ ice::coroutine_handle<ice::TaskPromise<Value>>::from_promise(*this) };
    }

    template<typename Value>
    inline auto ice::TaskPromise<Value&>::get_return_object() noexcept -> ice::Task<Value&>
    {
        return ice::Task<Value&>{ ice::coroutine_handle<ice::TaskPromise<Value&>>::from_promise(*this) };
    }

    auto ice::TaskPromise<void>::get_return_object() noexcept -> ice::Task<void>
    {
        return ice::Task<void>{ ice::coroutine_handle<ice::TaskPromise<void>>::from_promise(*this) };
    }

} // namespace ice

template<typename Result, typename... Args>
struct std::coroutine_traits<ice::Task<Result>, Args...>
{
    using promise_type = typename ice::Task<Result>::PromiseType;
};
