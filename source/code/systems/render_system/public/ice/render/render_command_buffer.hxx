#pragma once
#include <ice/span.hxx>
#include <ice/render/render_declarations.hxx>

namespace ice::render
{

    enum class CommandBuffer : ice::uptr
    {
        Invalid = 0x0
    };

    enum class CommandBufferType : ice::u32
    {
        Primary,
        Secondary,
    };

    enum class SubPassContents : ice::u32
    {
        Inline,
        SecondaryCommandBuffers
    };

    class RenderCommands
    {
    public:
        virtual void begin(
            ice::render::CommandBuffer cmds
        ) noexcept = 0;

        virtual void begin_renderpass(
            ice::render::CommandBuffer cmds,
            ice::render::Renderpass renderpass,
            ice::render::Framebuffer framebuffer,
            ice::vec2u extent,
            ice::vec4f clear_color
        ) noexcept = 0;

        virtual void begin_renderpass(
            ice::render::CommandBuffer cmds,
            ice::render::Renderpass renderpass,
            ice::render::Framebuffer framebuffer,
            ice::Span<ice::vec4f> clear_values,
            ice::vec2u extent
        ) noexcept = 0;

        virtual void next_subpass(
            ice::render::CommandBuffer cmds,
            ice::render::SubPassContents contents
        ) noexcept = 0;

        virtual void set_viewport(
            ice::render::CommandBuffer cmds,
            ice::vec4u viewport_rect
        ) noexcept = 0;

        virtual void set_scissor(
            ice::render::CommandBuffer cmds,
            ice::vec4u scissor_rect
        ) noexcept = 0;

        virtual void bind_pipeline(
            ice::render::CommandBuffer cmds,
            ice::render::Pipeline pipeline
        ) noexcept = 0;

        virtual void bind_resource_set(
            ice::render::CommandBuffer cmds,
            ice::render::PipelineLayout pipeline_layout,
            ice::render::ResourceSet resource_set,
            ice::u32 bind_point
        ) noexcept = 0;

        virtual void bind_index_buffer(
            ice::render::CommandBuffer cmds,
            ice::render::Buffer buffer
        ) noexcept = 0;

        virtual void bind_vertex_buffer(
            ice::render::CommandBuffer cmds,
            ice::render::Buffer buffer,
            ice::u32 binding
        ) noexcept = 0;

        virtual void draw_indexed(
            ice::render::CommandBuffer cmds,
            ice::u32 vertex_count,
            ice::u32 instance_count
        ) noexcept = 0;

        virtual void end_renderpass(
            ice::render::CommandBuffer cmds
        ) noexcept = 0;

        virtual void end(
            ice::render::CommandBuffer cmds
        ) noexcept = 0;

    protected:
        virtual ~RenderCommands() noexcept = default;
    };

} // namespace ice::render
