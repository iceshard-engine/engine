#pragma once
#include <core/pod/array.hxx>
#include <render_system/render_api.hxx>

namespace render::cmd
{

    void begin(render::api::CommandBuffer command_buffer) noexcept;
    void end(render::api::CommandBuffer command_buffer) noexcept;

    void begin_renderpass(render::api::CommandBuffer command_buffer) noexcept;
    void end_renderpass(render::api::CommandBuffer command_buffer) noexcept;

    void bind_render_pipeline(render::api::CommandBuffer command_buffer, render::api::RenderPipeline pipeline) noexcept;
    void bind_descriptor_sets(render::api::CommandBuffer command_buffer, render::api::DescriptorSets descriptor_sets) noexcept;
    void bind_vertex_buffers(render::api::CommandBuffer command_buffer, core::pod::Array<render::api::VertexBuffer> const& buffer_handles) noexcept;
    void bind_vertex_buffers(render::api::CommandBuffer command_buffer, render::api::VertexBuffer vertice_buffer, render::api::VertexBuffer instance_buffer) noexcept;
    void bind_index_buffers(render::api::CommandBuffer command_buffer, render::api::VertexBuffer index_buffer) noexcept;

    void set_viewport(render::api::CommandBuffer command_buffer, uint32_t width, uint32_t height) noexcept;
    void set_scissor(render::api::CommandBuffer command_buffer, uint32_t width, uint32_t height) noexcept;
    void set_scissor(render::api::CommandBuffer command_buffer, uint32_t x, uint32_t y, uint32_t width, uint32_t height) noexcept;

    void draw(render::api::CommandBuffer command_buffer, uint32_t vertice_count, uint32_t instance_count) noexcept;
    void draw_indexed(
        render::api::CommandBuffer command_buffer,
        uint32_t indice_count,
        uint32_t instance_count,
        uint32_t base_index,
        uint32_t base_vertex,
        uint32_t base_instance
    ) noexcept;

} // namespace render
