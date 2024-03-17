/// Copyright 2024 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include "webgpu_utils.hxx"
#include <ice/render/render_command_buffer.hxx>

namespace ice::render::webgpu
{

    class WebGPUCommands : public ice::render::RenderCommands
    {
    public:
        void begin(
            ice::render::CommandBuffer cmds
        ) noexcept override;

        void begin_renderpass(
            ice::render::CommandBuffer cmds,
            ice::render::Renderpass renderpass,
            ice::render::Framebuffer framebuffer,
            ice::vec2u extent,
            ice::vec4f clear_color
        ) noexcept override
        {
            begin_renderpass(cmds, renderpass, framebuffer, { &clear_color, 1 }, extent);
        }

        void begin_renderpass(
            ice::render::CommandBuffer cmds,
            ice::render::Renderpass renderpass,
            ice::render::Framebuffer framebuffer,
            ice::Span<ice::vec4f const> clear_values,
            ice::vec2u extent
        ) noexcept override;

        void next_subpass(
            ice::render::CommandBuffer cmds,
            ice::render::SubPassContents contents
        ) noexcept override { }

        void set_viewport(
            ice::render::CommandBuffer cmds,
            ice::vec4u viewport_rect
        ) noexcept override;

        void set_scissor(
            ice::render::CommandBuffer cmds,
            ice::vec4u scissor_rect
        ) noexcept override;

        void bind_pipeline(
            ice::render::CommandBuffer cmds,
            ice::render::Pipeline pipeline
        ) noexcept override;

        void bind_resource_set(
            ice::render::CommandBuffer cmds,
            ice::render::PipelineLayout pipeline_layout,
            ice::render::ResourceSet resource_set,
            ice::u32 first_set
        ) noexcept override;

        void bind_index_buffer(
            ice::render::CommandBuffer cmds,
            ice::render::Buffer buffer
        ) noexcept override;

        void bind_vertex_buffer(
            ice::render::CommandBuffer cmds,
            ice::render::Buffer buffer,
            ice::u32 binding
        ) noexcept override;

        void draw(
            ice::render::CommandBuffer cmds,
            ice::u32 vertex_count,
            ice::u32 instance_count,
            ice::u32 vertex_offset,
            ice::u32 instance_offset
        ) noexcept override;

        void draw_indexed(
            ice::render::CommandBuffer cmds,
            ice::u32 vertex_count,
            ice::u32 instance_count
        ) noexcept override;

        void draw_indexed(
            ice::render::CommandBuffer cmds,
            ice::u32 vertex_count,
            ice::u32 instance_count,
            ice::u32 index_offset,
            ice::u32 vertex_offset,
            ice::u32 instance_offset
        ) noexcept override;

        void end_renderpass(
            ice::render::CommandBuffer cmds
        ) noexcept override;

        void end(
            ice::render::CommandBuffer cmds
        ) noexcept override;

        void pipeline_image_barrier(
            ice::render::CommandBuffer cmds,
            ice::render::PipelineStage source_stage,
            ice::render::PipelineStage destination_stage,
            ice::Span<ice::render::ImageBarrier const> image_barriers
        ) noexcept override { }

        [[deprecated]]
        void update_texture(
            ice::render::CommandBuffer cmds,
            ice::render::Image image,
            ice::render::Buffer image_contents,
            ice::vec2u extents
        ) noexcept override;

        void update_texture_v2(
            ice::render::CommandBuffer cmds,
            ice::render::Image image,
            ice::render::Buffer image_contents,
            ice::vec2u extents
        ) noexcept override { }

        void push_constant(
            ice::render::CommandBuffer cmds,
            ice::render::PipelineLayout pipeline,
            ice::render::ShaderStageFlags shader_stages,
            ice::Data data,
            ice::u32 offset
        ) noexcept override { }
    };

} // namespace ice::render::webgpu
