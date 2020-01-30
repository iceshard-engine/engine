#pragma once
#include <core/pod/array.hxx>
#include <iceshard/renderer/render_api.hxx>

namespace iceshard::renderer::commands
{
    void next_subpass(api::CommandBuffer command_buffer) noexcept;

    void bind_pipeline(api::CommandBuffer command_buffer, api::Pipeline pipeline) noexcept;
    void bind_resource_set(api::CommandBuffer command_buffer, api::ResourceSet resource_set) noexcept;
    void bind_vertex_buffers(api::CommandBuffer command_buffer, core::pod::Array<api::Buffer> const& buffer_handles) noexcept;
    void bind_index_buffer(api::CommandBuffer command_buffer, api::Buffer index_buffer) noexcept;

    void set_viewport(api::CommandBuffer command_buffer, uint32_t width, uint32_t height) noexcept;
    void set_scissor(api::CommandBuffer command_buffer, uint32_t width, uint32_t height) noexcept;
    void set_scissor(api::CommandBuffer command_buffer, uint32_t x, uint32_t y, uint32_t width, uint32_t height) noexcept;

    void draw(api::CommandBuffer command_buffer, uint32_t vertice_count, uint32_t instance_count) noexcept;
    void draw_indexed(
        api::CommandBuffer command_buffer,
        uint32_t indice_count,
        uint32_t instance_count,
        uint32_t base_index,
        uint32_t base_vertex,
        uint32_t base_instance
    ) noexcept;

} // namespace render
