#pragma once
#include <ice/task_promise.hxx>

namespace ice
{

    template<typename T = void>
    class Task
    {
        using ValueType = T;
        using PromiseType = detail::TaskPromise<T>;

    private:
        struct AwaitableBase
        {
            std::coroutine_handle<PromiseType> _coroutine;

            explicit AwaitableBase(
                std::coroutine_handle<PromiseType> coro
            ) noexcept
                : _coroutine{ coro }
            { }

            bool await_ready() const noexcept
            {
                return !_coroutine || _coroutine.done();
            }

            auto await_suspend(
                std::coroutine_handle<> awaiting_coroutine
            ) noexcept -> std::coroutine_handle<>
            {
                _coroutine.promise().set_continuation(awaiting_coroutine);
                return _coroutine;
            }
        };

    public:
        Task() noexcept
            : _coroutine{ nullptr }
        { }

        explicit Task(std::coroutine_handle<PromiseType> coro) noexcept
            : _coroutine{ coro }
        { }

        Task(Task&& other) noexcept
            : _coroutine{ ice::exchange(other._coroutine, nullptr) }
        { }

        Task(Task const&) noexcept = delete;
        auto operator=(Task const&) noexcept = delete;

        ~Task() noexcept
        {
            if (_coroutine)
            {
                _coroutine.destroy();
            }
        }

        auto operator=(Task&& other) noexcept -> Task&
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

        auto is_ready() const noexcept
        {
            return !_coroutine || _coroutine.done();
        }

        auto operator co_await() const& noexcept
        {
            struct TaskAwaitable : AwaitableBase
            {
                using AwaitableBase::AwaitableBase;

                auto await_resume() noexcept -> decltype(auto)
                {
                    ICE_ASSERT(
                        this->_coroutine.operator bool(),
                        "Broken promise on coroutine Task!"
                    );

                    if constexpr (std::is_same_v<T, void> == false)
                    {
                        return this->_coroutine.promise().result();
                    }
                }
            };

            return TaskAwaitable{ _coroutine };
        }

        auto operator co_await() const && noexcept
        {
            struct TaskAwaitable : AwaitableBase
            {
                using AwaitableBase::AwaitableBase;

                auto await_resume() noexcept -> decltype(auto)
                {
                    ICE_ASSERT(
                        this->_coroutine.operator bool(),
                        "Broken promise on coroutine Task!"
                    );

                    if constexpr (std::is_same_v<T, void> == false)
                    {
                        return std::move(this->_coroutine.promise().result());
                    }
                }
            };

            return TaskAwaitable{ _coroutine };
        }

        auto when_ready() const noexcept
        {
            struct TaskAwaitable : AwaitableBase
            {
                using AwaitableBase::AwaitableBase;

                void await_resume() const noexcept {}
            };

            return TaskAwaitable{ _coroutine };
        }

    private:
        std::coroutine_handle<PromiseType> _coroutine;

    public:
        // Required by the C++ standard
        using promise_type = PromiseType;
    };

    namespace detail
    {

        template<typename T>
        auto TaskPromise<T>::get_return_object() noexcept -> Task<T>
        {
            return Task<T>{ std::coroutine_handle<TaskPromise>::from_promise(*this) };
        }

        auto TaskPromise<void>::get_return_object() noexcept -> Task<void>
        {
            return Task<void>{ std::coroutine_handle<TaskPromise>::from_promise(*this) };
        }

        template<typename T>
        auto TaskPromise<T&>::get_return_object() noexcept -> Task<T&>
        {
            return Task<T&>{ std::coroutine_handle<TaskPromise>::from_promise(*this) };
        }

    } // namespace detail

} // namespace ice
