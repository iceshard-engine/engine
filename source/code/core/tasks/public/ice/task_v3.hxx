#pragma once
#include <ice/task_promise_v3.hxx>
#include <ice/container_concepts.hxx>

namespace ice
{

    template<typename Result>
    class Task_v3 final
    {
    public:
        using ValueType = Result;
        using PromiseType = ice::TaskPromise<ValueType>;

    private:
        struct AwaitableBase
        {
            ice::coroutine_handle<PromiseType> _coroutine;

            inline bool await_ready() const noexcept;
            inline auto await_suspend(
                ice::coroutine_handle<> awaiting_coroutine
            ) const noexcept -> ice::coroutine_handle<>;
        };

    public:
        inline explicit Task_v3(ice::coroutine_handle<PromiseType> coro) noexcept;
        inline Task_v3(Task_v3&& other) noexcept;
        inline ~Task_v3() noexcept;

        inline Task_v3(Task_v3 const&) noexcept = delete;
        inline auto operator=(Task_v3 const&) noexcept = delete;

        inline Task_v3(Task_v3&&) noexcept = default;
        inline auto operator=(Task_v3&& other) noexcept -> Task_v3& = default;

        inline auto is_ready() const noexcept;
        inline auto when_ready() noexcept;

        inline auto operator co_await() & noexcept;
        inline auto operator co_await() && noexcept;

    private:
        ice::coroutine_handle<PromiseType> _coroutine;
    };

    //explicit AwaitableBase(
    //    ice::coroutine_handle<PromiseType> coro
    //) noexcept
    //    : _coroutine{ coro }
    //{ }

    template<typename Result>
    inline bool Task_v3<Result>::AwaitableBase::await_ready() const noexcept
    {
        return !_coroutine || _coroutine.done();
    }

    template<typename Result>
    inline auto Task_v3<Result>::AwaitableBase::await_suspend(
        ice::coroutine_handle<> awaiting_coroutine
    ) const noexcept -> ice::coroutine_handle<>
    {
        _coroutine.promise().set_continuation(awaiting_coroutine);
        return _coroutine;
    }

    template<typename Result>
    inline Task_v3<Result>::Task_v3(ice::coroutine_handle<PromiseType> coro) noexcept
        : _coroutine{ coro }
    { }

    template<typename Result>
    inline Task_v3<Result>::Task_v3(Task_v3&& other) noexcept
        : _coroutine{ ice::exchange(other._coroutine, nullptr) }
    { }

    template<typename Result>
    inline Task_v3<Result>::~Task_v3() noexcept
    {
        if (_coroutine)
        {
            _coroutine.destroy();
        }
    }

    template<typename Result>
    inline auto Task_v3<Result>::operator=(Task_v3&& other) noexcept -> Task_v3&
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
    inline auto Task_v3<Result>::is_ready() const noexcept
    {
        return !_coroutine || _coroutine.done();
    }

    template<typename Result>
    inline auto Task_v3<Result>::when_ready() noexcept
    {
        struct TaskAwaitable : AwaitableBase
        {
            void await_resume() const noexcept {}
        };

        return TaskAwaitable{ _coroutine };
    }

    template<typename Result>
    inline auto Task_v3<Result>::operator co_await() & noexcept
    {
        struct TaskAwaitable : AwaitableBase
        {
            auto await_resume() const noexcept -> decltype(auto)
            {
                ICE_ASSERT(
                    _coroutine.operator bool(),
                    "Broken promise on coroutine Task_v3!"
                );

                if constexpr (std::is_same_v<ValueType, void> == false)
                {
                    return _coroutine.promise().result();
                }
            }
        };

        return TaskAwaitable{ _coroutine };
    }

    template<typename Result>
    inline auto Task_v3<Result>::operator co_await() && noexcept
    {
        struct TaskAwaitable : AwaitableBase
        {
            auto await_resume() const noexcept -> decltype(auto)
            {
                ICE_ASSERT(
                    _coroutine.operator bool(),
                    "Broken promise on coroutine Task!"
                );

                if constexpr (std::is_same_v<ValueType, void> == false)
                {
                    return ice::move(_coroutine.promise().result());
                }
            }
        };

        return TaskAwaitable{ ice::move(_coroutine) };
    }

    template<typename Value>
    inline auto ice::TaskPromise<Value>::get_return_object() noexcept -> ice::Task_v3<Value>
    {
        return ice::Task_v3<Value>{ ice::coroutine_handle<ice::TaskPromise<Value>>::from_promise(*this) };
    }

    template<typename Value>
    inline auto ice::TaskPromise<Value&>::get_return_object() noexcept -> ice::Task_v3<Value&>
    {
        return ice::Task_v3<Value&>{ ice::coroutine_handle<ice::TaskPromise<Value&>>::from_promise(this) };
    }

    auto ice::TaskPromise<void>::get_return_object() noexcept -> ice::Task_v3<void>
    {
        return ice::Task_v3<void>{ ice::coroutine_handle<ice::TaskPromise<void>>::from_address(this) };
    }

} // namespace ice
