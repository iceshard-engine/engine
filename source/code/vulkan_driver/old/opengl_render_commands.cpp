#include <renderlib/render_commands.h>

#define RENDER_COMMAND(name) static constexpr stringid_hash_t name## _cmd_id = _stringid(#name)
#include "opengl_render_command_list.h"
#undef RENDER_COMMAND

void mooned::render::commands::enable(CommandBuffer& cb, RenderOption option)
{
    auto* data = reinterpret_cast<RenderOption*>(cb.append(enable_cmd_id, sizeof(RenderOption)));
    *data = option;
}

void mooned::render::commands::disable(CommandBuffer& cb, RenderOption option)
{
    auto* data = reinterpret_cast<RenderOption*>(cb.append(disable_cmd_id, sizeof(RenderOption)));
    *data = option;
}

void mooned::render::commands::clear_color(CommandBuffer& cb, float r, float g, float b, float a /*= 0.0f*/)
{
    auto* color = reinterpret_cast<float*>(cb.append(clear_color_cmd_id, sizeof(float) * 4));
    color[0] = r;
    color[1] = g;
    color[2] = b;
    color[3] = a;
}

void mooned::render::commands::clear(CommandBuffer& cb, ClearFlags flags)
{
    auto* data = reinterpret_cast<ClearFlags*>(cb.append(clear_cmd_id, sizeof(ClearFlags)));
    *data = flags;
}

void mooned::render::commands::blend_equation(CommandBuffer& cb, BlendEquation equation)
{
    auto* data = reinterpret_cast<BlendEquation*>(cb.append(blend_equation_cmd_id, sizeof(BlendEquation)));
    *data = equation;
}

void mooned::render::commands::blend_factors(CommandBuffer& cb, BlendFactor sfactor, BlendFactor dfactor)
{
    auto* data = reinterpret_cast<BlendFactor*>(cb.append(blend_factors_cmd_id, sizeof(BlendFactor) * 2));
    *data++ = sfactor;
    *data = dfactor;
}

void mooned::render::commands::scissor_test(CommandBuffer& cb, int x, int y, int width, int height)
{
    struct Data {
        int x;
        int y;
        int width;
        int height;
    };

    auto* data = reinterpret_cast<Data*>(cb.append(scissor_test_cmd_id, sizeof(Data)));
    data->x = x;
    data->y = y;
    data->width = width;
    data->height = height;
}

void mooned::render::commands::bind_render_target(CommandBuffer& cb, RenderTarget::Handle render_target)
{
    auto* data = reinterpret_cast<uint32_t*>(cb.append(bind_render_target_cmd_id, sizeof(uint32_t)));
    *data = static_cast<uint32_t>(render_target);
}

void mooned::render::commands::copy_render_target_data(CommandBuffer& cb, RenderTarget::Handle read_target, RenderTarget::Handle write_target, int src_x, int src_y, int src_w, int src_h, int dest_x, int dest_y, int dest_w, int dest_h, int mask, int filter)
{
    struct Data
    {
        struct Dimensions
        {
            int x;
            int y;
            int width;
            int height;
        };

        uint32_t read_target;
        uint32_t write_target;

        Dimensions source;
        Dimensions target;
        long mask;
        long filter;
    };

    auto* data = reinterpret_cast<Data*>(cb.append(copy_render_target_cmd_id, sizeof(Data)));
    data->read_target = static_cast<uint32_t>(read_target);
    data->write_target = static_cast<uint32_t>(write_target);
    data->source = { src_x, src_y, src_w, src_h };
    data->target = { dest_x, dest_y, dest_w, dest_h };
    data->mask = mask;
    data->filter = filter;
}

void mooned::render::commands::bind_vertex_array(CommandBuffer& cb, VertexArray::Handle vertex_array)
{
    auto* data = reinterpret_cast<uint32_t*>(cb.append(bind_vertex_array_cmd_id, sizeof(uint32_t)));
    *data = static_cast<uint32_t>(vertex_array);
}

void mooned::render::commands::use_program_pipeline(CommandBuffer& cb, ProgramPipeline::Handle pipeline)
{
    auto* data = reinterpret_cast<uint32_t*>(cb.append(use_program_pipeline_cmd_id, sizeof(uint32_t)));
    *data = static_cast<uint32_t>(pipeline);
}

void mooned::render::commands::use_program(CommandBuffer& cb, ProgramPipeline::Handle pipeline, ShaderProgram::Handle program, ShaderProgram::Stage stage)
{
    struct Data {
        uint32_t pipeline;
        uint32_t program;
        ShaderProgram::Stage stage;
    };

    void* raw = cb.append(use_program_cmd_id, sizeof(Data));
    Data* data = reinterpret_cast<Data*>(raw);
    data->pipeline = static_cast<uint32_t>(pipeline);
    data->program = static_cast<uint32_t>(program);
    data->stage = stage;
}

void mooned::render::commands::active_texture(CommandBuffer& cb, TextureSlot slot)
{
    auto* data = reinterpret_cast<TextureSlot*>(cb.append(active_texture_cmd_id, sizeof(TextureSlot)));
    *data = slot;
}

void mooned::render::commands::bind_texture(CommandBuffer& cb, TextureDetails::Type type, Texture::Handle handle)
{
    struct Data {
        TextureDetails::Type type;
        uint32_t texture;
    };

    auto* raw = cb.append(bind_texture_cmd_id, sizeof(Data));
    auto* data = reinterpret_cast<Data*>(raw);
    data->type = type;
    data->texture = static_cast<uint32_t>(handle);
}

void mooned::render::commands::bind_buffer(CommandBuffer& cb, BufferTarget target, RenderBuffer::Handle buffer)
{
    struct Data {
        BufferTarget target;
        uint32_t buffer;
    };

    auto* raw = cb.append(bind_buffer_cmd_id, sizeof(Data));
    auto* dest = reinterpret_cast<Data*>(raw);
    dest->target = target;
    dest->buffer = static_cast<uint32_t>(buffer);
}

void mooned::render::commands::buffer_data(CommandBuffer& cb, BufferTarget target, const void* data, uint32_t size)
{
    struct Header {
        BufferTarget target;
        uint32_t size;
    };

    void* raw = cb.append(buffer_data_cmd_id, sizeof(Header) + size);
    Header* header = reinterpret_cast<Header*>(raw);
    header->target = target;
    header->size = size;

    void* dest = header + 1; // The data destination is just after the header!
    memcpy(dest, data, size);
}

void mooned::render::commands::buffer_data_ptr(CommandBuffer& cb, BufferTarget target, const void* data, uint32_t size)
{
    struct Header {
        BufferTarget target;
        uint32_t size;
        const void* data;
    };

    void* raw = cb.append(buffer_data_ptr_cmd_id, sizeof(Header));
    Header* header = reinterpret_cast<Header*>(raw);
    header->target = target;
    header->size = size;
    header->data = data;
}

void mooned::render::commands::bind_vertex_buffer(CommandBuffer& cb, uint32_t binding, RenderBuffer::Handle buffer, uint32_t offset, uint32_t stride)
{
    struct Data {
        uint32_t binding;
        uint32_t buffer;
        uint32_t offset;
        uint32_t stride;
    };

    void* raw = cb.append(bind_vertex_buffer_cmd_id, sizeof(Data));
    Data* data = reinterpret_cast<Data*>(raw);
    data->binding = binding;
    data->buffer = static_cast<uint32_t>(buffer);
    data->offset = offset;
    data->stride = stride;
}

void mooned::render::commands::bind_buffer_range(CommandBuffer& cb, BufferTarget type, uint32_t index, RenderBuffer::Handle buffer, uint32_t offset, uint32_t size)
{
    struct Data {
        mooned::render::BufferTarget type;
        uint32_t index;
        uint32_t buffer;
        uint32_t offset;
        uint32_t size;
    };

    void* raw = cb.append(bind_buffer_range_cmd_id, sizeof(Data));
    Data* data = reinterpret_cast<Data*>(raw);
    data->type = type;
    data->index = index;
    data->buffer = static_cast<uint32_t>(buffer);
    data->offset = offset;
    data->size = size;
}

void mooned::render::commands::draw_elements(CommandBuffer& cb, DrawFunction func, ElementType element, uint32_t count, uint32_t offset)
{
    struct Data {
        DrawFunction func;
        ElementType element;
        uint32_t count;
        uint32_t offset;
    };

    void* raw = cb.append(draw_elements_cmd_id, sizeof(Data));
    Data* data = reinterpret_cast<Data*>(raw);
    data->func = func;
    data->element = element;
    data->count = count;
    data->offset = offset;
}

void mooned::render::commands::draw_elements(CommandBuffer& cb, DrawFunction func, ElementType element, uint32_t count, uint32_t offset, uint32_t base_vertex)
{
    struct Data {
        DrawFunction func;
        ElementType element;
        uint32_t count;
        uint32_t offset;
        uint32_t base_vertex;
    };

    void* raw = cb.append(draw_elements_base_vertex_cmd_id, sizeof(Data));
    Data* data = reinterpret_cast<Data*>(raw);
    data->func = func;
    data->element = element;
    data->count = count;
    data->offset = offset;
    data->base_vertex = base_vertex;
}

void mooned::render::commands::draw_elements_instanced(CommandBuffer& cb, DrawFunction func, ElementType element, uint32_t count, uint32_t offset, int32_t primcount)
{
    struct Data {
        DrawFunction func;
        ElementType element;
        uint32_t count;
        uint32_t offset;
        int32_t instance_count;
    };

    void* raw = cb.append(draw_elements_instanced_cmd_id, sizeof(Data));
    Data* data = reinterpret_cast<Data*>(raw);
    data->func = func;
    data->element = element;
    data->count = count;
    data->offset = offset;
    data->instance_count = primcount;
}

void mooned::render::commands::draw_elements_instanced_base_vertex(CommandBuffer& cb, DrawFunction func, ElementType element, uint32_t count, uint32_t offset, int32_t primcount, int32_t base_vertex)
{
    struct Data {
        DrawFunction func;
        ElementType element;
        uint32_t count;
        uint32_t offset;
        int32_t instance_count;
        int32_t base_vertex;
    };

    void* raw = cb.append(draw_elements_instanced_base_vertex_cmd_id, sizeof(Data));
    Data* data = reinterpret_cast<Data*>(raw);
    data->func = func;
    data->element = element;
    data->count = count;
    data->offset = offset;
    data->instance_count = primcount;
    data->base_vertex = base_vertex;
}


void mooned::render::commands::draw_elements_instanced_base_vertex_base_instance(CommandBuffer& cb, DrawFunction func, ElementType element, uint32_t count, uint32_t offset, int32_t primcount, int32_t base_vertex, int32_t base_instance)
{
    struct Data {
        DrawFunction func;
        ElementType element;
        uint32_t count;
        uint32_t offset;
        int32_t instance_count;
        int32_t base_vertex;
        int32_t base_instance;
    };

    void* raw = cb.append(draw_elements_instanced_base_vertex_base_instance_cmd_id, sizeof(Data));
    Data* data = reinterpret_cast<Data*>(raw);
    data->func = func;
    data->element = element;
    data->count = count;
    data->offset = offset;
    data->instance_count = primcount;
    data->base_vertex = base_vertex;
    data->base_instance = base_instance;
}
