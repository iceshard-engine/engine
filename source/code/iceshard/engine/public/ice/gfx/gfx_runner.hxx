#pragma once
#include <ice/task.hxx>
#include <ice/task_scheduler.hxx>

namespace ice::gfx
{

    class GfxRunner
    {
    public:
        virtual ~GfxRunner() noexcept = default;

        virtual void push_task(ice::Task<void> task) noexcept = 0;
    };

} // namespace ice::gfx
