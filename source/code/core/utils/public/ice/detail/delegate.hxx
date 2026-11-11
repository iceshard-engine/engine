#pragma once
#include <ice/base.hxx>

namespace ice::detail
{

    struct MemberPointerStorage
    {
        struct Helper{ void undefined() noexcept; };
        union
        {
            void* _function_ptr;
            char _member_data[sizeof(&Helper::undefined)];
        };
    };

    template<typename R, typename... Args>
    using InvokerFn = R(*)(void*, MemberPointerStorage const&, Args...) noexcept;

    template<typename C, typename R, typename... Args>
    using MemberFnType = R(C::*)(Args...) noexcept;

    template<typename C, typename R, typename... Args>
    static auto store_member_ptr(MemberFnType<C, R, Args...> fn) noexcept -> MemberPointerStorage
    {
        return std::bit_cast<MemberPointerStorage>(fn);
    }

    template<typename C, typename R, typename... Args>
    static auto read_member_ptr(MemberPointerStorage const& storage) noexcept -> MemberFnType<C, R, Args...>
    {
        return std::bit_cast<MemberFnType<C, R, Args...>>(storage);
    }

    template<typename R, typename... Args>
    constexpr auto delegate_invoker_raw_func(
        void*,
        MemberPointerStorage const& storage,
        Args... args
    ) noexcept -> R
    {
        return reinterpret_cast<R(*)(Args...) noexcept>(storage._function_ptr)(args...);
    }

    template<typename C, typename R, typename... Args>
    constexpr auto delegate_invoker_member_func(
        void* ctx,
        MemberPointerStorage const& storage,
        Args... args
    ) noexcept -> R
    {
        C* const object = static_cast<C*>(ctx);
        MemberFnType<C, R, Args...> const member = read_member_ptr<C, R, Args...>(storage);
        return (object->*member)(args...);
    }

} // namespace ice::detail
