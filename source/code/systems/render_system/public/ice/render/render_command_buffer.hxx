#pragma once
#include <ice/base.hxx>

namespace ice::render
{

    enum class CommandBufferType
    {
        Primary,
        Secondary,
    };

    enum class SubPassContents : ice::u32
    {
        Inline,
        SecondaryCommandBuffers
    };

    enum class CommandBuffer : ice::uptr
    {
        Invalid = 0x0
    };

    enum class RenderPass : ice::uptr;

    enum class Framebuffer : ice::uptr;

    class RenderCommands
    {
    protected:
        virtual ~RenderCommands() noexcept = default;

    public:
        virtual void begin(
            ice::render::CommandBuffer cmds
        ) noexcept = 0;

        virtual void begin_renderpass(
            ice::render::CommandBuffer cmds,
            ice::render::RenderPass renderpass,
            ice::render::Framebuffer framebuffer,
            ice::vec2u extent,
            ice::vec4f clear_color
        ) noexcept = 0;

        virtual void next_subpass(
            ice::render::CommandBuffer cmds,
            ice::render::SubPassContents contents
        ) noexcept = 0;

        virtual void end_renderpass(
            ice::render::CommandBuffer cmds
        ) noexcept = 0;

        virtual void end(
            ice::render::CommandBuffer cmds
        ) noexcept = 0;
    };

} // namespace ice::render
