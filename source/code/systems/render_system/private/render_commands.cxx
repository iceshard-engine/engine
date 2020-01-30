#include <render_system/render_commands.hxx>

void iceshard::renderer::commands::next_subpass(iceshard::renderer::api::CommandBuffer command_buffer) noexcept
{
    iceshard::renderer::api::render_api_instance->cmd_next_subpass_func(command_buffer);
}

void iceshard::renderer::commands::bind_pipeline(iceshard::renderer::api::CommandBuffer command_buffer, iceshard::renderer::api::Pipeline pipeline) noexcept
{
    iceshard::renderer::api::render_api_instance->cmd_bind_render_pipeline_func(command_buffer, pipeline);
}

void iceshard::renderer::commands::bind_resource_set(iceshard::renderer::api::CommandBuffer command_buffer, iceshard::renderer::api::ResourceSet resource_set) noexcept
{
    iceshard::renderer::api::render_api_instance->cmd_bind_resource_set_func(command_buffer, resource_set);
}

void iceshard::renderer::commands::bind_vertex_buffers(iceshard::renderer::api::CommandBuffer command_buffer, core::pod::Array<iceshard::renderer::api::Buffer> const& buffer_handles) noexcept
{
    iceshard::renderer::api::render_api_instance->cmd_bind_vertex_buffers_array_func(command_buffer, core::pod::array::begin(buffer_handles), core::pod::array::size(buffer_handles));
}

void iceshard::renderer::commands::bind_index_buffer(iceshard::renderer::api::CommandBuffer command_buffer, iceshard::renderer::api::Buffer index_buffer) noexcept
{
    iceshard::renderer::api::render_api_instance->cmd_bind_index_buffer_func(command_buffer, index_buffer);
}

void iceshard::renderer::commands::set_viewport(iceshard::renderer::api::CommandBuffer command_buffer, uint32_t width, uint32_t height) noexcept
{
    iceshard::renderer::api::render_api_instance->cmd_set_viewport_func(command_buffer, width, height);
}

void iceshard::renderer::commands::set_scissor(iceshard::renderer::api::CommandBuffer command_buffer, uint32_t width, uint32_t height) noexcept
{
    iceshard::renderer::api::render_api_instance->cmd_set_scissor_func(command_buffer, width, height);
}

void iceshard::renderer::commands::set_scissor(iceshard::renderer::api::CommandBuffer command_buffer, uint32_t x, uint32_t y, uint32_t width, uint32_t height) noexcept
{
    iceshard::renderer::api::render_api_instance->cmd_set_scissor2_func(command_buffer, x, y, width, height);
}

void iceshard::renderer::commands::draw(iceshard::renderer::api::CommandBuffer command_buffer, uint32_t vertice_count, uint32_t instance_count) noexcept
{
    iceshard::renderer::api::render_api_instance->cmd_draw_func(command_buffer, vertice_count, instance_count);
}

void iceshard::renderer::commands::draw_indexed(iceshard::renderer::api::CommandBuffer command_buffer, uint32_t indice_count, uint32_t instance_count, uint32_t base_index, uint32_t base_vertex, uint32_t base_instance) noexcept
{
    iceshard::renderer::api::render_api_instance->cmd_draw_indexed_func(command_buffer, indice_count, instance_count, base_index, base_vertex, base_instance);
}
