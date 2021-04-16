#pragma once
#include <ice/stringid.hxx>
#include <ice/render/render_command_buffer.hxx>
#include <ice/gfx/gfx_pass.hxx>

namespace ice::gfx
{

    class GfxQueue;

    class GfxFrame
    {
    protected:
        virtual ~GfxFrame() noexcept = default;

    public:
        virtual void enqueue_pass(
            ice::StringID_Arg queue_name,
            ice::gfx::GfxPass* pass
        ) noexcept = 0;

        [[deprecated("This function will be removed at a later stage.")]]
        virtual auto get_queue(
            ice::StringID_Arg name
        ) noexcept -> GfxQueue* = 0;
    };

} // namespace ice::gfx
