#pragma once
#include <ice/task.hxx>

namespace ice
{

    struct DetachedTask;

    struct DetachedTaskPromise
    {
        inline auto initial_suspend() const noexcept { return ice::suspend_never{ }; }
        inline auto final_suspend() const noexcept { return ice::suspend_never{ }; }
        inline auto return_void() const noexcept { }

        inline auto get_return_object() const noexcept -> ice::DetachedTask;
        inline void unhandled_exception() const noexcept
        {
            ICE_ASSERT(false, "Unexpected coroutine exception!");
        }
    };

    struct DetachedTask
    {
        using PromiseType = ice::DetachedTaskPromise;
    };

    inline auto DetachedTaskPromise::get_return_object() const noexcept -> ice::DetachedTask
    {
        return {};
    }

} // namespace ice

template<typename... Args>
struct std::coroutine_traits<ice::DetachedTask, Args...>
{
    using promise_type = ice::DetachedTask::PromiseType;
};
