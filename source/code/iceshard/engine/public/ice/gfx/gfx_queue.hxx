#pragma once
#include <ice/span.hxx>
#include <ice/stringid.hxx>
#include <ice/render/render_declarations.hxx>

namespace ice::gfx
{

    class GfxPass;

    class GfxQueue
    {
    public:
        virtual ~GfxQueue() noexcept = default;

        virtual bool presenting() const noexcept = 0;
        virtual void set_presenting(bool is_presenting) noexcept = 0;

        virtual void alloc_command_buffers(
            ice::render::CommandBufferType type,
            ice::Span<ice::render::CommandBuffer> buffers
        ) noexcept = 0;

        virtual void submit_command_buffers(
            ice::Span<ice::render::CommandBuffer> buffers
        ) noexcept = 0;
    };

} // namespace ice::gfx
