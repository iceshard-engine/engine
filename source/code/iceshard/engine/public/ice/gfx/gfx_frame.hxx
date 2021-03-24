#pragma once
#include <ice/stringid.hxx>
#include <ice/render/render_command_buffer.hxx>

namespace ice::gfx
{

    class GfxQueue;

    class GfxFrame
    {
    protected:
        virtual ~GfxFrame() noexcept = default;

    public:
        virtual auto get_pass(
            ice::StringID_Arg name
        ) noexcept -> GfxQueue* = 0;
    };

} // namespace ice::gfx
