#include <renderlib/render_commands.h>

#include <memsys/memsys.h>
#include <kernel/logger/logger.h>

#include "opengl_engine.h"
#include "opengl_enums.h"

#define RENDER_COMMAND(name) static constexpr stringid_hash_t name## _cmd_id = _stringid(#name)
#include "opengl_render_command_list.h"
#undef RENDER_COMMAND

//! Executes the given command
void execute_render_command(const mooned::render::Command& command, const void* data)
{
    switch (command.id)
    {
    case enable_cmd_id:
    {
        auto* option = reinterpret_cast<const mooned::render::RenderOption*>(data);
        glEnable(mooned::render::togl(*option));
        break;
    }
    case disable_cmd_id:
    {
        auto* option = reinterpret_cast<const mooned::render::RenderOption*>(data);
        glDisable(mooned::render::togl(*option));
        break;
    }
    case clear_color_cmd_id:
    {
        auto* color = reinterpret_cast<const float*>(data);
        glClearColor(color[0], color[1], color[2], color[3]);
        break;
    }
    case clear_cmd_id:
    {
        auto* option = reinterpret_cast<const mooned::render::ClearFlags*>(data);
        glClear(mooned::render::togl(*option));
        break;
    }
    case blend_equation_cmd_id:
    {
        auto* equation = reinterpret_cast<const mooned::render::BlendEquation*>(data);
        glBlendEquation(togl(*equation));
        break;
    }
    case blend_factors_cmd_id:
    {
        auto* factors = reinterpret_cast<const mooned::render::BlendFactor*>(data);
        glBlendFunc(togl(*factors), togl(*(factors + 1)));
        break;
    }
    case scissor_test_cmd_id:
    {
        struct Data {
            int x, y, width, height;
        };
        auto* bb = reinterpret_cast<const Data*>(data);
        glScissor(bb->x, bb->y, bb->width, bb->height);
        break;
    }
    case bind_render_target_cmd_id:
    {
        auto* render_target = reinterpret_cast<const uint32_t*>(data);
        glBindFramebuffer(GL_FRAMEBUFFER, *render_target);
        break;
    }
    case copy_render_target_cmd_id:
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
            int mask;
            int filter;
        };

        auto* render_target = reinterpret_cast<const Data*>(data);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, render_target->read_target);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, render_target->write_target);
        glBlitFramebuffer(
            render_target->source.x, render_target->source.y, render_target->source.width, render_target->source.height
            , render_target->target.x, render_target->target.y, render_target->target.width, render_target->target.height
            , render_target->mask + GL_COLOR_BUFFER_BIT
            , render_target->filter + GL_NEAREST
        );
        break;
    }
    case bind_vertex_array_cmd_id:
    {
        auto* vertex_array = reinterpret_cast<const uint32_t*>(data);
        glBindVertexArray(*vertex_array);
        break;
    }
    case use_program_pipeline_cmd_id:
    {
        auto* program_pipeline = reinterpret_cast<const uint32_t*>(data);
        glBindProgramPipeline(*program_pipeline);
        break;
    }
    case use_program_cmd_id:
    {
        auto* program_pipeline = reinterpret_cast<const uint32_t*>(data);
        auto* program = reinterpret_cast<const uint32_t*>(program_pipeline + 1);
        auto* stage = reinterpret_cast<const mooned::render::ShaderProgram::Stage*>(program + 1);
        glUseProgramStages(*program_pipeline, togl(*stage), *program);
        break;
    }
    case active_texture_cmd_id:
    {
        auto* slot = reinterpret_cast<const mooned::render::TextureSlot*>(data);
        glActiveTexture(togl(*slot));
        break;
    }
    case bind_texture_cmd_id:
    {
        auto* type = reinterpret_cast<const mooned::render::TextureDetails::Type*>(data);
        auto* texture = reinterpret_cast<const uint32_t*>(type + 1);
        glBindTexture(togl(*type), *texture);
        break;
    }
    case bind_buffer_cmd_id:
    {
        auto* target = reinterpret_cast<const mooned::render::BufferTarget*>(data);
        auto* buffer = reinterpret_cast<const uint32_t*>(target + 1);
        glBindBuffer(togl(*target), *buffer);
        break;
    }
    case buffer_data_cmd_id:
    {
        auto* target = reinterpret_cast<const mooned::render::BufferTarget*>(data);
        auto* size = reinterpret_cast<const uint32_t*>(target + 1);
        const void* dataptr = size + 1;
        glBufferData(togl(*target), static_cast<GLsizeiptr>(*size), dataptr, GL_STATIC_DRAW); // #todo Publish this in the api?
        break;
    }
    case buffer_data_ptr_cmd_id:
    {
        struct Header {
            mooned::render::BufferTarget target;
            uint32_t size;
            const void* data;
        };
        auto* src = reinterpret_cast<const Header*>(data);
        glBufferData(togl(src->target), static_cast<GLsizeiptr>(src->size), src->data, GL_STATIC_DRAW); // #todo Publish this in the api?
        break;
    }
    case bind_vertex_buffer_cmd_id:
    {
        struct Data {
            uint32_t binding;
            uint32_t buffer;
            uint32_t offset;
            uint32_t stride;
        };

        auto* src = reinterpret_cast<const Data*>(data);

        glBindVertexBuffer(src->binding, src->buffer, src->offset, src->stride);
        break;
    }
    case bind_buffer_range_cmd_id:
    {
        struct Data {
            mooned::render::BufferTarget target;
            uint32_t index;
            uint32_t buffer;
            uint32_t offset;
            uint32_t size;
        };

        auto* src = reinterpret_cast<const Data*>(data);
        glBindBufferRange(togl(src->target), src->index, src->buffer, src->offset, src->size);
        break;
    }
    case draw_elements_cmd_id:
    {
        struct Data {
            mooned::render::DrawFunction func;
            mooned::render::ElementType element;
            uint32_t count;
            uint32_t offset;
        };

        auto* src = reinterpret_cast<const Data*>(data);

        glDrawElements(togl(src->func), src->count, togl(src->element), reinterpret_cast<const void*>(static_cast<uintptr_t>(src->offset)));
        break;
    }
    case draw_elements_base_vertex_cmd_id:
    {
        struct Data {
            mooned::render::DrawFunction func;
            mooned::render::ElementType element;
            uint32_t count;
            uint32_t offset;
            uint32_t base_vertex;
        };

        auto* src = reinterpret_cast<const Data*>(data);
        glDrawElementsBaseVertex(togl(src->func), src->count, togl(src->element), reinterpret_cast<void*>(static_cast<uintptr_t>(src->offset)), src->base_vertex);
        break;
    }
    case draw_elements_instanced_cmd_id:
    {
        struct Data {
            mooned::render::DrawFunction func;
            mooned::render::ElementType element;
            uint32_t count;
            uint32_t offset;
            int32_t instance_count;
        };

        auto* src = reinterpret_cast<const Data*>(data);
        glDrawElementsInstanced(togl(src->func), src->count, togl(src->element), reinterpret_cast<void*>(static_cast<uintptr_t>(src->offset)), src->instance_count);
        break;
    }
    case draw_elements_instanced_base_vertex_cmd_id:
    {
        struct Data {
            mooned::render::DrawFunction func;
            mooned::render::ElementType element;
            uint32_t count;
            uint32_t offset;
            int32_t instance_count;
            int32_t base_vertex;
        };

        auto* src = reinterpret_cast<const Data*>(data);
        glDrawElementsInstancedBaseVertex(togl(src->func), src->count, togl(src->element), reinterpret_cast<void*>(static_cast<uintptr_t>(src->offset)), src->instance_count, src->base_vertex);
        break;
    }
    case draw_elements_instanced_base_vertex_base_instance_cmd_id:
    {
        struct Data {
            mooned::render::DrawFunction func;
            mooned::render::ElementType element;
            uint32_t count;
            uint32_t offset;
            int32_t instance_count;
            int32_t base_vertex;
            int32_t base_instance;
        };

        auto* src = reinterpret_cast<const Data*>(data);
        glDrawElementsInstancedBaseVertexBaseInstance(togl(src->func), src->count, togl(src->element), reinterpret_cast<void*>(static_cast<uintptr_t>(src->offset)), src->instance_count, src->base_vertex, src->base_instance);
        break;
    }
    default:
    {
        extern const char* tostring(const mooned::render::Command&);
        MLogWarning("Render command not handled! {}", tostring(command));
    }
    }

    // Check for errors
    auto gl_error = glGetError();
    if (GL_NO_ERROR != gl_error)
    {
        const GLubyte* gl_error_message = glewGetErrorString(gl_error);
        extern const char* tostring(const mooned::render::Command&);
        MLogError("OpenGl rendering error: {} ({:x} : {})", tostring(command), gl_error, gl_error_message);
    }

}

//! Executes all render command of the given buffer
void mooned::render::opengl::OpenGlRenderEngine::execute(const mooned::render::CommandBuffer& cb)
{
    cb.foreach(execute_render_command);
}
