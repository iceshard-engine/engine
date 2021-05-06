#pragma once
#include <ice/stringid.hxx>
#include <ice/render/render_command_buffer.hxx>
#include <ice/gfx/gfx_pass.hxx>

namespace ice::gfx
{

    template<typename T>
    class Task;

    class GfxQueue;

    class GfxFrame
    {
    protected:
        virtual ~GfxFrame() noexcept = default;

    public:
        virtual void execute_task(
            ice::Task<void> task
        ) noexcept = 0;

        virtual void enqueue_pass(
            ice::StringID_Arg queue_name,
            ice::gfx::GfxPass* pass
        ) noexcept = 0;
    };

} // namespace ice::gfx
