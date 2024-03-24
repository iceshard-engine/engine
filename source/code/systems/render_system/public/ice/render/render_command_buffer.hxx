/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/span.hxx>
#include <ice/mem_data.hxx>
#include <ice/render/render_declarations.hxx>
#include <ice/render/render_profiler.hxx>

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
            ice::Span<ice::vec4f const> clear_values,
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
            ice::u32 first_set
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

        virtual void draw(
            ice::render::CommandBuffer cmds,
            ice::u32 vertex_count,
            ice::u32 instance_count,
            ice::u32 vertex_offset,
            ice::u32 instance_offset
        ) noexcept = 0;

        virtual void draw_indexed(
            ice::render::CommandBuffer cmds,
            ice::u32 vertex_count,
            ice::u32 instance_count
        ) noexcept = 0;

        virtual void draw_indexed(
            ice::render::CommandBuffer cmds,
            ice::u32 vertex_count,
            ice::u32 instance_count,
            ice::u32 index_offset,
            ice::u32 vertex_offset,
            ice::u32 instance_offset
        ) noexcept = 0;

        virtual void end_renderpass(
            ice::render::CommandBuffer cmds
        ) noexcept = 0;

        virtual void end(
            ice::render::CommandBuffer cmds
        ) noexcept = 0;

        virtual void pipeline_image_barrier(
            ice::render::CommandBuffer cmds,
            ice::render::PipelineStage source_stage,
            ice::render::PipelineStage destination_stage,
            ice::Span<ice::render::ImageBarrier const> image_barriers
        ) noexcept = 0;

        [[deprecated]]
        virtual void update_texture(
            ice::render::CommandBuffer cmds,
            ice::render::Image image,
            ice::render::Buffer image_contents,
            ice::vec2u extents
        ) noexcept = 0;

        virtual void update_texture_v2(
            ice::render::CommandBuffer cmds,
            ice::render::Image image,
            ice::render::Buffer image_contents,
            ice::vec2u extents
        ) noexcept = 0;

        virtual void push_constant(
            ice::render::CommandBuffer cmds,
            ice::render::PipelineLayout pipeline,
            ice::render::ShaderStageFlags shader_stages,
            ice::Data data,
            ice::u32 offset
        ) noexcept = 0;

#if IPT_ENABLED
        //! \brief Creates a new profiling zone for all commands recorded after this call.
        virtual auto profiling_zone(
            ice::render::CommandBuffer cmds,
            const tracy::SourceLocationData* srcloc,
            ice::String name
        ) noexcept -> ice::render::detail::ProfilingZone = 0;

        //! \brief Collects GPU zone timings, should not be used directly by the user.
        virtual void profiling_collect_zones(
            ice::render::CommandBuffer cmds
        ) noexcept = 0;
#endif

    protected:
        virtual ~RenderCommands() noexcept = default;
    };

} // namespace ice::render
