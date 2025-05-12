/// Copyright 2023 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/assert.hxx>
#include <ice/task_promise_base.hxx>

namespace ice
{

    template<typename Value>
    class GeneratorPromise : public ice::TaskPromiseBase
    {
    public:
        inline auto initial_suspend() const noexcept -> std::suspend_never { return {}; }

        inline auto get_return_object() noexcept -> ice::Generator<Value>;

        template<typename Other = Value>
            requires std::is_nothrow_move_assignable_v<Value> && std::is_nothrow_convertible_v<Other&&, Value>
        inline auto yield_value(Other&& value) noexcept -> ice::suspend_always
        {
            _value = std::forward<Other>(value);
            return {};
        }

        template<typename Other = Value>
            requires std::is_nothrow_copy_assignable_v<Value> && std::is_nothrow_convertible_v<Other&&, Value>
        inline auto yield_value(Other const& value) noexcept -> ice::suspend_always
        {
            _value = value;
            return {};
        }

        inline auto result() & noexcept -> Value&
        {
            return _value;
        }

        inline auto result() && noexcept -> Value&&
        {
            return std::move(_value);
        }

        void return_void() noexcept
        {
        }

    private:
        Value _value;
    };

    template<typename Result>
    class Generator final
    {
    public:
        using ValueType = Result;
        using PromiseType = ice::GeneratorPromise<ValueType>;

    public:
        inline explicit Generator(
            ice::coroutine_handle<PromiseType> coro = nullptr
        ) noexcept;
        inline ~Generator() noexcept;

        inline Generator(Generator const&) noexcept = delete;
        inline auto operator=(Generator const&) noexcept = delete;

        inline Generator(Generator&&) noexcept;
        inline auto operator=(Generator&& other) noexcept -> Generator&;

        inline auto operator co_await() & noexcept;
        inline auto operator co_await() && noexcept;

        struct GeneratorIterator
        {
            ice::coroutine_handle<PromiseType> _coro;

            auto operator*() & noexcept -> Result& { return _coro.promise().result(); }
            auto operator*() && noexcept -> Result&& { return ice::move(_coro.promise().result()); }

            auto operator++() noexcept -> GeneratorIterator&
            {
                if (_coro.done() == false)
                {
                    _coro.resume();
                }
                return *this;
            }

            bool operator==(GeneratorIterator const& other) const noexcept
            {
                return _coro == other._coro && _coro.done();
            }

            auto end() noexcept -> Result*
            {
                return nullptr;
            }
        };

        auto begin() noexcept { return GeneratorIterator{ _coroutine }; }
        auto end() noexcept { return GeneratorIterator{ _coroutine }; }

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
    inline bool Generator<Result>::AwaitableBase::await_ready() const noexcept
    {
        return !_coroutine || _coroutine.done();
    }

    template<typename Result>
    inline auto Generator<Result>::AwaitableBase::await_suspend(
        ice::coroutine_handle<> awaiting_coroutine
    ) const noexcept -> ice::coroutine_handle<>
    {
        _coroutine.promise().set_continuation(awaiting_coroutine);
        return _coroutine;
    }

    template<typename Result>
    inline Generator<Result>::Generator(ice::coroutine_handle<PromiseType> coro) noexcept
        : _coroutine{ coro }
    { }

    template<typename Result>
    inline Generator<Result>::~Generator() noexcept
    {
        if (_coroutine)
        {
            _coroutine.destroy();
        }
    }

    template<typename Result>
    inline Generator<Result>::Generator(Generator&& other) noexcept
        : _coroutine{ ice::exchange(other._coroutine, nullptr) }
    { }

    template<typename Result>
    inline auto Generator<Result>::operator=(Generator&& other) noexcept -> Generator&
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
    inline auto Generator<Result>::operator co_await() & noexcept
    {
        struct GeneratorAwaitable : AwaitableBase
        {
            auto await_resume() const noexcept -> decltype(auto)
            {
                ICE_ASSERT(
                    this->_coroutine.operator bool(),
                    "Broken promise on coroutine Generator!"
                );

                if constexpr (std::is_same_v<ValueType, void> == false)
                {
                    return this->_coroutine.promise().result();
                }
            }
        };

        return GeneratorAwaitable{ _coroutine };
    }

    template<typename Result>
    inline auto Generator<Result>::operator co_await() && noexcept
    {
        struct GeneratorAwaitable : AwaitableBase
        {
            auto await_resume() const noexcept -> decltype(auto)
            {
                ICE_ASSERT(
                    this->_coroutine.operator bool(),
                    "Broken promise on coroutine Generator!"
                );

                if constexpr (std::is_same_v<ValueType, void> == false)
                {
                    return ice::move(this->_coroutine.promise().result());
                }
            }
        };

        return GeneratorAwaitable{ ice::move(_coroutine) };
    }

    template<typename Value>
    inline auto ice::GeneratorPromise<Value>::get_return_object() noexcept -> ice::Generator<Value>
    {
        return ice::Generator<Value>{ ice::coroutine_handle<ice::GeneratorPromise<Value>>::from_promise(*this) };
    }

} // namespace ice

template<typename Result, typename... Args>
struct std::coroutine_traits<ice::Generator<Result>, Args...>
{
    using promise_type = typename ice::Generator<Result>::PromiseType;
};
