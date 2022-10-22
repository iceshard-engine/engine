/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/base.hxx>
#include <coroutine>

namespace ice
{

    template<typename T>
    class ScheduleOperation;

    template<typename T>
    class ScheduleDelayedOperation;

    namespace detail
    {

        struct ScheduleOperationData
        {
            std::coroutine_handle<> _coroutine = nullptr;
            ice::detail::ScheduleOperationData* _next = nullptr;
        };

        struct ScheduleDelayedOperationData
        {
            ice::u32 const _target_time;
            std::coroutine_handle<> _coroutine = nullptr;
            ice::detail::ScheduleDelayedOperationData* _next = nullptr;
        };

    } // namespace detail

    template<typename T>
    class ScheduleOperation
    {
    public:
        using DataMemberType = ice::detail::ScheduleOperationData ScheduleOperation<T>::*;

        inline ScheduleOperation(T& target) noexcept;

        inline bool await_ready() const noexcept { return false; }
        inline void await_suspend(std::coroutine_handle<void> coro) noexcept;
        inline void await_resume() const noexcept { }

    protected:
        T& _target;
        ice::detail::ScheduleOperationData _data;
    };

    template<typename T>
    concept TaskScheduler = requires(T t) {
        { &T::schedule_internal } -> std::convertible_to<void (T::*)(ice::ScheduleOperation<T>*, typename ice::ScheduleOperation<T>::DataMemberType)noexcept>;
    };

    template<typename T> requires ice::TaskScheduler<T>
    inline auto operator co_await(T& target) noexcept
    {
        return ice::ScheduleOperation<T>{ target };
    }

    template<typename T>
    inline ScheduleOperation<T>::ScheduleOperation(T& target) noexcept
        : _target{ target }
        , _data{ }
    {
    }

    template<typename T>
    inline void ScheduleOperation<T>::await_suspend(std::coroutine_handle<void> coro) noexcept
    {
        _data._coroutine = coro;
        _target.schedule_internal(this, &ScheduleOperation::_data);
    }


    template<typename T>
    class ScheduleDelayedOperation final
    {
    public:
        inline ScheduleDelayedOperation(T& target, ice::u32 target_time) noexcept;

        inline bool await_ready() const noexcept { return false; }
        inline void await_suspend(std::coroutine_handle<void> coro) noexcept;
        inline void await_resume() const noexcept { }

    private:
        T& _target;
        ice::detail::ScheduleOperationData _data;
    };

    template<typename T>
    inline ScheduleDelayedOperation<T>::ScheduleDelayedOperation(T& target, ice::u32 target_time) noexcept
        : _target{ target }
        , _data{ target_time }
    {
    }

    template<typename T>
    inline void ScheduleDelayedOperation<T>::await_suspend(std::coroutine_handle<void> coro) noexcept
    {
        _data._coroutine = coro;
        _target.schedule_internal(this, &ScheduleDelayedOperation::_data);
    }


    template<typename T, typename K>
    class ScheduleContextOperation : public ice::ScheduleOperation<T>
    {
    public:
        inline ScheduleContextOperation(T& target, K& context) noexcept;

        inline auto await_resume() const noexcept -> K&;

    private:
        K& _context;
    };

    template<typename T, typename K>
    inline ScheduleContextOperation<T, K>::ScheduleContextOperation(T& target, K& context) noexcept
        : ice::ScheduleOperation<T>{ target }
        , _context{ context }
    {
    }

    template<typename T, typename K>
    inline auto ScheduleContextOperation<T, K>::await_resume() const noexcept -> K&
    {
        return _context;
    }

} // namespace ice
