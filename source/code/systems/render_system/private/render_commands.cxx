#include <render_system/render_commands.hxx>

void render::cmd::begin(render::api::CommandBuffer command_buffer) noexcept
{
    render::api::render_api_instance->cmd_begin_func(command_buffer);
}

void render::cmd::end(render::api::CommandBuffer command_buffer) noexcept
{
    render::api::render_api_instance->cmd_end_func(command_buffer);
}

void render::cmd::begin_renderpass(render::api::CommandBuffer command_buffer) noexcept
{
    render::api::render_api_instance->cmd_begin_renderpass_func(command_buffer);
}

void render::cmd::end_renderpass(render::api::CommandBuffer command_buffer) noexcept
{
    render::api::render_api_instance->cmd_end_renderpass_func(command_buffer);
}

void render::cmd::bind_render_pipeline(render::api::CommandBuffer command_buffer, render::api::RenderPipeline pipeline) noexcept
{
    render::api::render_api_instance->cmd_bind_render_pipeline_func(command_buffer, pipeline);
}

void render::cmd::bind_descriptor_sets(render::api::CommandBuffer command_buffer, render::api::DescriptorSets descriptor_sets) noexcept
{
    render::api::render_api_instance->cmd_bind_descriptor_sets_func(command_buffer, descriptor_sets);
}

void render::cmd::bind_vertex_buffers(render::api::CommandBuffer command_buffer, render::api::VertexBuffer vertice_buffer, render::api::VertexBuffer instance_buffer) noexcept
{
    render::api::render_api_instance->cmd_bind_vertex_buffers_func(command_buffer, vertice_buffer, instance_buffer);
}

void render::cmd::bind_index_buffers(render::api::CommandBuffer command_buffer, render::api::VertexBuffer index_buffer) noexcept
{
    render::api::render_api_instance->cmd_bind_index_buffers_func(command_buffer, index_buffer);
}

void render::cmd::set_viewport(render::api::CommandBuffer command_buffer, uint32_t width, uint32_t height) noexcept
{
    render::api::render_api_instance->cmd_set_viewport_func(command_buffer, width, height);
}

void render::cmd::set_scissor(render::api::CommandBuffer command_buffer, uint32_t width, uint32_t height) noexcept
{
    render::api::render_api_instance->cmd_set_scissor_func(command_buffer, width, height);
}

void render::cmd::set_scissor(render::api::CommandBuffer command_buffer, uint32_t x, uint32_t y, uint32_t width, uint32_t height) noexcept
{
    render::api::render_api_instance->cmd_set_scissor2_func(command_buffer, x, y, width, height);
}

void render::cmd::draw(render::api::CommandBuffer command_buffer, uint32_t vertice_count, uint32_t instance_count) noexcept
{
    render::api::render_api_instance->cmd_draw_func(command_buffer, vertice_count, instance_count);
}

void render::cmd::draw_indexed(render::api::CommandBuffer command_buffer, uint32_t indice_count, uint32_t instance_count, uint32_t base_index, uint32_t base_vertex, uint32_t base_instance) noexcept
{
    render::api::render_api_instance->cmd_draw_indexed_func(command_buffer, indice_count, instance_count, base_index, base_vertex, base_instance);
}
