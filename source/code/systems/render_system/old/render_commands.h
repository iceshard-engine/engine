#pragma once
#include <memsys/allocator.h>
#include <collections/pod/array.h>
#include <collections/data/buffer.h>

#include <kernel/compiletime/stringid.h>

#include <renderlib/render_target.h>
#include <renderlib/render_texture.h>
#include <renderlib/render_program.h>
#include <renderlib/render_vertex_array.h>
#include <renderlib/render_buffer.h>
#include <renderlib/render_enums.h>

namespace mooned::render
{

//! Describes a single command to the renderer.
struct Command
{
    //! The command ID.
    stringid_hash_t id;

    //! The command additional data size.
    uint32_t size;

    //! The command additional data.
    uint32_t data_offset;
};

static_assert(sizeof(Command) == 16llu, "The command POD structure size is not valid! Expected a sizeof 16 bytes!");

class CommandBuffer
{
public:
    CommandBuffer(mem::allocator& alloc);
    ~CommandBuffer();

    // Disable move and copy semantics.
    CommandBuffer(CommandBuffer&&) = delete;
    CommandBuffer(const CommandBuffer&) = delete;

    CommandBuffer& operator=(CommandBuffer&&) = delete;
    CommandBuffer& operator=(const CommandBuffer&) = delete;

    //! Returns true if the given command buffer is empty.
    bool empty() const;

    //! Removes all commands from the buffer.
    void clear();

    //! Appends another command to the buffer.
    //! \returns A pointer to a buffer big enough to store the requested data size.
    void* append(stringid_hash_t id, uint32_t size);

    //! Iterates over all commands and calls the given function for each command.
    template<class Func>
    void foreach(Func func) const
    {
        for (const auto& cmd : *this)
        {
            func(cmd, command_data(&cmd));
        }
    }

protected:
    //! Returns the first command of this buffer.
    const Command* begin() const;

    //! Returns past the last command of this buffer.
    const Command* end() const;

    //! Returns the given commands data pointer.
    const void* command_data(const Command* cmd) const;

private:
    mem::allocator& _allocator;
    pod::Array<Command> _commands;
    pod::Buffer _data;
};

//! Defines free functions which take a command buffer as the first argument, and additional arguments for command specific data.
namespace commands
{

//! Enables a particular render option.
void enable(CommandBuffer&, RenderOption option);

//! Disables a particular render option.
void disable(CommandBuffer&, RenderOption option);

//! Sets the clear color.
void clear_color(CommandBuffer&, float r, float g, float b, float a = 0.0f);

//! Clears the current render target values.
void clear(CommandBuffer&, ClearFlags flags);

//! Sets the blend equation.
//! \note This equation is used if BLEND mode is enabled.
void blend_equation(CommandBuffer&, BlendEquation equation);

//! Sets the blend equation sources.
void blend_factors(CommandBuffer&, BlendFactor sfactor, BlendFactor dfactor);

//! Runs a scissor test for the given bounding box.
void scissor_test(CommandBuffer&, int x, int y, int width, int height);

//! Changes the current render target.
void bind_render_target(CommandBuffer&, RenderTarget::Handle handle);

//! Changes the current render target for writing.
void copy_render_target_data(CommandBuffer&, RenderTarget::Handle read_target, RenderTarget::Handle write_target, int src_x, int src_y, int src_w, int src_h, int dest_x, int dest_y, int dest_w, int dest_h, int mask, int filter);

//! Changes the current vertex array object.
void bind_vertex_array(CommandBuffer&, VertexArray::Handle handle);

//! Changes the current program pipeline.
void use_program_pipeline(CommandBuffer&, ProgramPipeline::Handle handle);

//! Changes the current program stage at the given slot.
void use_program(CommandBuffer&, ProgramPipeline::Handle pipeline, ShaderProgram::Handle handle, ShaderProgram::Stage stage);

//! Activates the given texture index.
void active_texture(CommandBuffer&, TextureSlot slot);

//! Binds a texture to the current active texture slot.
void bind_texture(CommandBuffer&, TextureDetails::Type type, Texture::Handle handle);

//! Binds the given buffer to the current context.
void bind_buffer(CommandBuffer&, BufferTarget target, RenderBuffer::Handle buffer);

//! Copies the given data into the currently bound buffer.
void buffer_data(CommandBuffer&, BufferTarget target, const void* data, uint32_t size);

//! Copies the given data into the currently bound buffer.
//! \note The given 'data' pointer needs to be alive the whole frame!
void buffer_data_ptr(CommandBuffer&, BufferTarget target, const void* data, uint32_t size);

//! Binds the current buffer as the vertex buffer into the rendering pipeline.
void bind_vertex_buffer(CommandBuffer&, uint32_t binding, RenderBuffer::Handle buffer, uint32_t offset, uint32_t stride);

//! Binds the current buffer as a data range into a shader program uniform object.
void bind_buffer_range(CommandBuffer& cb, BufferTarget type, uint32_t index, RenderBuffer::Handle buffer, uint32_t offset, uint32_t size);

//! Draws all elements from the buffer object.
void draw_elements(CommandBuffer&, DrawFunction func, ElementType element, uint32_t count, uint32_t offset);

//! Draws all elements from the buffer object.
//! \note Sets the base vertex offset.
void draw_elements(CommandBuffer&, DrawFunction func, ElementType element, uint32_t count, uint32_t offset, uint32_t base_vertex);

//! Draws all elements from the buffer object, using instanced data from a second render buffer.
void draw_elements_instanced(CommandBuffer&, DrawFunction func, ElementType element, uint32_t count, uint32_t offset, int32_t primcount);

//! Draws all elements from the buffer object, using instanced data from a second render buffer.
void draw_elements_instanced_base_vertex(CommandBuffer&, DrawFunction func, ElementType element, uint32_t count, uint32_t offset, int32_t primcount, int32_t base_vertex);

//! Draws all elements from the buffer object, using instanced data from a second render buffer.
void draw_elements_instanced_base_vertex_base_instance(CommandBuffer&, DrawFunction func, ElementType element, uint32_t count, uint32_t offset, int32_t primcount, int32_t base_vertex, int32_t base_instance);

}

}
