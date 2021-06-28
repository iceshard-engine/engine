#pragma once
#include <ice/task_thread.hxx>
#include <ice/task_operations.hxx>

namespace ice::gfx
{

    class GfxFrame;

    using GfxScheduleFrameOperation = ice::ScheduleContextOperation<ice::TaskThread, ice::gfx::GfxFrame>;
    using GfxScheduleFrameEndOperation = ice::ScheduleOperation<ice::TaskThread>;

} // namespace ice::gfx
