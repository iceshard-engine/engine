/// Copyright 2026 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/detail/delegate.hxx>
#include <ice/assert_core.hxx>

namespace ice
{

    template<typename Func>
    struct Delegate;

    template<typename R, typename... Args>
    struct Delegate<R(Args...)>
    {
        using InvokerFn = ice::detail::InvokerFn<R, Args...>;

        constexpr Delegate() noexcept = default;

        constexpr Delegate(
            InvokerFn invoker,
            void* function_ptr
        ) noexcept
            : _invoker{ invoker }
            , _object_ptr{ nullptr }
            , _storage{ ._function_ptr = function_ptr }
        { }

        constexpr Delegate(
            InvokerFn invoker,
            void* object_ptr,
            ice::detail::MemberPointerStorage stored_member
        ) noexcept
            : _invoker{ invoker }
            , _object_ptr{ object_ptr }
            , _storage{ stored_member }
        { }

        constexpr Delegate(Delegate&& other) noexcept
            : _invoker{ ice::exchange(other._invoker, nullptr) }
            , _object_ptr{ ice::exchange(other._object_ptr, nullptr) }
            , _storage{ ice::exchange(other._storage, {}) }
        { }
        constexpr auto operator=(Delegate&& other) noexcept -> Delegate&
        {
            if (ice::addressof(other) != this)
            {
                _invoker = ice::exchange(other._invoker, nullptr);
                _object_ptr = ice::exchange(other._object_ptr, nullptr);
                _storage = ice::exchange(other._storage, {});
            }
            return *this;
        }

        constexpr bool valid() const noexcept
        {
            return _invoker != nullptr;
        }

        template<typename Self, typename... InArgs>
        constexpr auto operator()(this Self&& self, InArgs&&... args) noexcept -> R
        {
            ICE_ASSERT_CORE(self._invoker != nullptr);
            return self._invoker(self._object_ptr, self._storage, ice::forward<Args>(args)...);
        }

        InvokerFn _invoker;
        void* _object_ptr;
        ice::detail::MemberPointerStorage _storage;
    };

    template<typename R, typename... Args>
    constexpr auto make_delegate(R(*raw_fn)(Args...) noexcept) noexcept -> ice::Delegate<R(Args...)>
    {
        return ice::Delegate<R(Args...)>{
            ice::detail::delegate_invoker_raw_func<R, Args...>,
            reinterpret_cast<void*>(raw_fn)
        };
    }

    template<typename C, typename R, typename... Args>
    constexpr auto make_delegate(C* object, R(C::*member_fn)(Args...) noexcept) noexcept -> ice::Delegate<R(Args...)>
    {
        return ice::Delegate<R(Args...)>{
            ice::detail::delegate_invoker_member_func<C, R, Args...>,
            object,
            ice::detail::store_member_ptr(member_fn)
        };
    }

} // namespace ice