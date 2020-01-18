#pragma once
#include <memsys/allocator.h>

#include <collections/pod/array.h>
#include <collections/data/buffer.h>
#include <kernel/utils/strong_typedef.h>

#include <string>

namespace mooned::render
{

//! Defines a single shader program
class ShaderProgram
{
public:
    //! Defines a single stage a Shader program can be used for.
    enum class Stage
    {
        VERTEX,
        FRAGMENT,
        GEOMETRY,
        COMPUTE,
    };

    using Handle = mooned::strong_numeric_typedef<ShaderProgram, uint32_t>;

    //! A shader program takes always the stage it's going to be used for.
    ShaderProgram(mem::allocator& alloc, Stage stage);
    ~ShaderProgram();

    //! Loads the program data from a memory buffer
    void load(const pod::Buffer& buffer);

    //! Loads the program data from a memory buffer
    void load(const char* data, size_t size);

    //! Tells if the shader program is properly initialized and a valid handle can be returned.
    bool valid() const;

    //! Returns a handle to be used in a render command buffer.
    //! \note If a handle does not exist, it will create one.
    Handle get_handle();

    //! Releases the underlying handle, which makes it unusable in render command buffers.
    //! \note If the handle does not exist, nothing happens.
    void release_handle();

private:
    mem::allocator& _allocator;
    const Stage _stage;

    pod::Buffer _data;

    Handle _handle;
};

//! Defines a program pipeline
class ProgramPipeline
{
public:
    using Handle = mooned::strong_numeric_typedef<ProgramPipeline, uint32_t>;

    ProgramPipeline();
    ~ProgramPipeline() = default;

    //! Returns true if the program pipeline is valid.
    bool valid() const;

    //! Returns the handle, or creates one if it does not exist yet.
    Handle get_handle();

    //! Releases the underlying handle, or does nothing if it does not exist.
    void release_handle();

private:
    Handle _handle;
};

//! Creates a new shader program from the given file
ShaderProgram* program_from_file(mem::allocator& alloc, ShaderProgram::Stage stage, std::string path);

//! Creates a new shader program from the given buffer
ShaderProgram* program_from_buffer(mem::allocator& alloc, ShaderProgram::Stage stage, const pod::Buffer& buffer);

//! Creates a new shader program from the given raw data buffer
ShaderProgram* program_from_data(mem::allocator& alloc, ShaderProgram::Stage stage, const char* data, uint32_t size);

}
