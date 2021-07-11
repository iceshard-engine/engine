#pragma once
#include <ice/base.hxx>
#include <coroutine>

namespace ice
{

    struct EngineTaskOperationBaseData
    {
        std::coroutine_handle<> coroutine = nullptr;
        ice::EngineTaskOperationBaseData* next = nullptr;
    };

    template<typename Target, typename Data = ice::EngineTaskOperationBaseData>
    struct EngineTaskOperation
    {
        inline EngineTaskOperation(Target& target) noexcept;
        inline EngineTaskOperation(Target& target, Data data) noexcept;

        inline bool await_ready() const noexcept { return false; }
        inline void await_suspend(std::coroutine_handle<> coro) noexcept;
        inline void await_resume() const noexcept { }

    protected:
        Target& _target;
        Data _data;
    };

    template<typename Target, typename Data>
    inline EngineTaskOperation<Target, Data>::EngineTaskOperation(Target& target) noexcept
        : _target{ target }
        , _data{ }
    { }

    template<typename Target, typename Data>
    inline EngineTaskOperation<Target, Data>::EngineTaskOperation(Target& target, Data data) noexcept
        : _target{ target }
        , _data{ data }
    { }

    template<typename Target, typename Data>
    inline void EngineTaskOperation<Target, Data>::await_suspend(std::coroutine_handle<> coro) noexcept
    {
        _data.coroutine = coro;
        _target.schedule_internal(_data);
    }

} // namespace ice
