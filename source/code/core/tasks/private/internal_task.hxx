#pragma once
#include <ice/base.hxx>
#include <ice/assert.hxx>
#include <coroutine>

namespace ice::detail
{

    class InternalTask;

    struct InternalPromiseType
    {
        auto initial_suspend() noexcept
        {
            return std::suspend_always{};
        }

        auto final_suspend() noexcept
        {
            return std::suspend_always{};
        }

        auto get_return_object() noexcept -> InternalTask;

        void return_void() noexcept { }

        void unhandled_exception() noexcept
        {
            ICE_ASSERT(false, "Unhandled exception in Task object!");
        }

        void result() { }
    };

    class InternalTask
    {
        using ValueType = void;
        using PromiseType = InternalPromiseType;

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
                _coroutine.resume();
                return awaiting_coroutine;
            }
        };

    public:
        InternalTask() noexcept
            : _coroutine{ nullptr }
        { }

        explicit InternalTask(std::coroutine_handle<PromiseType> coro) noexcept
            : _coroutine{ coro }
        { }

        InternalTask(InternalTask&& other) noexcept
            : _coroutine{ ice::exchange(other._coroutine, nullptr) }
        { }

        InternalTask(InternalTask const&) noexcept = delete;
        auto operator=(InternalTask const&) noexcept = delete;

        ~InternalTask() noexcept
        {
            if (_coroutine)
            {
                _coroutine.destroy();
            }
        }

        auto operator=(InternalTask&& other) noexcept -> InternalTask&
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

        void resume() noexcept
        {
            if (_coroutine)
            {
                _coroutine.resume();
            }
        }

        auto move_and_reset() noexcept -> std::coroutine_handle<PromiseType>
        {
            return ice::exchange(_coroutine, nullptr);
        }

    private:
        std::coroutine_handle<PromiseType> _coroutine;

    public:
        // Required by the C++ standard
        using promise_type = PromiseType;
    };

    auto InternalPromiseType::get_return_object() noexcept -> InternalTask
    {
        return InternalTask{ std::coroutine_handle<InternalPromiseType>::from_promise(*this) };
    }

} // namespace ice
