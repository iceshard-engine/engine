#include <iceshard/renderer/render_funcs.hxx>
#include <iceshard/renderer/render_commands.hxx>
#include <iceshard/renderer/render_buffers.hxx>

void iceshard::renderer::check() noexcept
{
    iceshard::renderer::api::render_module_api->check_func();
}

auto iceshard::renderer::create_buffer(
    iceshard::renderer::api::BufferType type,
    uint32_t size
) noexcept -> iceshard::renderer::api::Buffer
{
    return iceshard::renderer::api::render_module_api->create_data_buffer_func(type, size);
}

void iceshard::renderer::create_command_buffers(api::CommandBufferType type, core::pod::Array<api::CommandBuffer>& result) noexcept
{
    iceshard::renderer::api::render_module_api->create_command_buffers_func(
        type,
        core::pod::array::begin(result),
        core::pod::array::size(result)
    );
}

auto iceshard::renderer::create_texture(
    core::stringid_arg_type name,
    asset::AssetData texture_data
) noexcept -> iceshard::renderer::api::Texture
{
    using namespace core::math;

    int32_t width = resource::get_meta_int32(texture_data.metadata, "texture.extents.width"_sid);
    int32_t height = resource::get_meta_int32(texture_data.metadata, "texture.extents.height"_sid);

    return iceshard::renderer::api::render_module_api->create_texture_func(
        name,
        vec2<u32>(width, height)
    );
}

void iceshard::renderer::map_buffer(
    iceshard::renderer::api::Buffer buffer,
    iceshard::renderer::api::DataView& view
) noexcept
{
    iceshard::renderer::api::render_module_api->buffer_array_map_data_func(
        &buffer, &view, 1
    );
}

void iceshard::renderer::unmap_buffer(
    iceshard::renderer::api::Buffer buffer
) noexcept
{
    iceshard::renderer::api::render_module_api->buffer_array_unmap_data_func(
        &buffer, 1
    );
}

void iceshard::renderer::map_buffers(
    core::pod::Array<iceshard::renderer::api::Buffer>& buffers,
    core::pod::Array<iceshard::renderer::api::DataView>& views
) noexcept
{
    iceshard::renderer::api::render_module_api->buffer_array_map_data_func(
        core::pod::array::begin(buffers), core::pod::array::begin(views), core::pod::array::size(buffers)
    );
}

void iceshard::renderer::unmap_buffers(
    core::pod::Array<iceshard::renderer::api::Buffer>& buffers
) noexcept
{
    iceshard::renderer::api::render_module_api->buffer_array_unmap_data_func(
        core::pod::array::begin(buffers), core::pod::array::size(buffers)
    );
}

void iceshard::renderer::commands::begin(api::CommandBuffer cb, api::CommandBufferUsage usage) noexcept
{
    iceshard::renderer::api::render_module_api->cmd_begin_func(cb, usage);
}

void iceshard::renderer::commands::begin(api::CommandBuffer cb, api::RenderPass rp, uint32_t subpass) noexcept
{
    iceshard::renderer::api::render_module_api->cmd_begin_subpass_func(cb, rp, subpass);
}

void iceshard::renderer::commands::end(api::CommandBuffer cb) noexcept
{
    iceshard::renderer::api::render_module_api->cmd_end_func(cb);
}

void iceshard::renderer::commands::update_texture(
    api::CommandBuffer cb,
    api::Texture dst_texture,
    api::Buffer src_data,
    core::math::vec2u size
) noexcept
{
    iceshard::renderer::api::render_module_api->cmd_update_texture_func(
        cb,
        dst_texture,
        src_data,
        size
    );
}

void iceshard::renderer::commands::next_subpass(
    api::CommandBuffer cb,
    api::RenderSubPass subpass
) noexcept
{
    iceshard::renderer::api::render_module_api->cmd_next_subpass_func(
        cb, subpass
    );
}

void iceshard::renderer::commands::execute_commands(
    api::CommandBuffer cb,
    uint32_t count,
    api::CommandBuffer* command_buffers
) noexcept
{
    iceshard::renderer::api::render_module_api->cmd_execute_commands_func(
        cb, count, command_buffers
    );
}

void iceshard::renderer::commands::push_constants(api::CommandBuffer command_buffer, api::Pipeline pipeline, core::data_view data) noexcept
{
    iceshard::renderer::api::render_module_api->cmd_push_constants_func(command_buffer, pipeline, 0, data.size(), data.data());
}

void iceshard::renderer::commands::push_constants(api::CommandBuffer command_buffer, api::Pipeline pipeline, core::data_view data, uint32_t offset) noexcept
{
    iceshard::renderer::api::render_module_api->cmd_push_constants_func(command_buffer, pipeline, offset, data.size(), data.data());
}

void iceshard::renderer::commands::bind_pipeline(iceshard::renderer::api::CommandBuffer command_buffer, iceshard::renderer::api::Pipeline pipeline) noexcept
{
    iceshard::renderer::api::render_module_api->cmd_bind_render_pipeline_func(command_buffer, pipeline);
}

void iceshard::renderer::commands::bind_resource_set(iceshard::renderer::api::CommandBuffer command_buffer, iceshard::renderer::api::ResourceSet resource_set) noexcept
{
    iceshard::renderer::api::render_module_api->cmd_bind_resource_set_func(command_buffer, resource_set);
}

void iceshard::renderer::commands::bind_vertex_buffers(iceshard::renderer::api::CommandBuffer command_buffer, core::pod::Array<iceshard::renderer::api::Buffer> const& buffer_handles) noexcept
{
    iceshard::renderer::api::render_module_api->cmd_bind_vertex_buffers_array_func(command_buffer, core::pod::array::begin(buffer_handles), core::pod::array::size(buffer_handles));
}

void iceshard::renderer::commands::bind_vertex_buffers(iceshard::renderer::api::CommandBuffer command_buffer, core::pod::Array<iceshard::renderer::api::Buffer const> const& buffer_handles) noexcept
{
    iceshard::renderer::api::render_module_api->cmd_bind_vertex_buffers_array_func(command_buffer, core::pod::array::begin(buffer_handles), core::pod::array::size(buffer_handles));
}

void iceshard::renderer::commands::bind_index_buffer(iceshard::renderer::api::CommandBuffer command_buffer, iceshard::renderer::api::Buffer index_buffer) noexcept
{
    iceshard::renderer::api::render_module_api->cmd_bind_index_buffer_func(command_buffer, index_buffer);
}

void iceshard::renderer::commands::set_viewport(iceshard::renderer::api::CommandBuffer command_buffer, uint32_t width, uint32_t height) noexcept
{
    iceshard::renderer::api::render_module_api->cmd_set_viewport_func(command_buffer, width, height);
}

void iceshard::renderer::commands::set_scissor(iceshard::renderer::api::CommandBuffer command_buffer, uint32_t width, uint32_t height) noexcept
{
    iceshard::renderer::api::render_module_api->cmd_set_scissor_func(command_buffer, width, height);
}

void iceshard::renderer::commands::set_scissor(iceshard::renderer::api::CommandBuffer command_buffer, uint32_t x, uint32_t y, uint32_t width, uint32_t height) noexcept
{
    iceshard::renderer::api::render_module_api->cmd_set_scissor2_func(command_buffer, x, y, width, height);
}

void iceshard::renderer::commands::draw(iceshard::renderer::api::CommandBuffer command_buffer, uint32_t vertice_count, uint32_t instance_count) noexcept
{
    iceshard::renderer::api::render_module_api->cmd_draw_func(command_buffer, vertice_count, instance_count);
}

void iceshard::renderer::commands::draw_indexed(iceshard::renderer::api::CommandBuffer command_buffer, uint32_t indice_count, uint32_t instance_count, uint32_t base_index, uint32_t base_vertex, uint32_t base_instance) noexcept
{
    iceshard::renderer::api::render_module_api->cmd_draw_indexed_func(command_buffer, indice_count, instance_count, base_index, base_vertex, base_instance);
}
