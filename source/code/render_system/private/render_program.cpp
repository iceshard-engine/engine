#include <renderlib/render_program.h>
#include <filesystem/filesystem.h>

//////////////////////////////////////////////////////////////////////////
// Shader Program
//////////////////////////////////////////////////////////////////////////

mooned::render::ShaderProgram::ShaderProgram(mem::allocator& alloc, Stage stage)
    : _allocator{ alloc }
    , _stage{ stage }
    , _data{ alloc }
    , _handle{ 0 }
{
}

mooned::render::ShaderProgram::~ShaderProgram()
{
    pod::buffer::clear(_data);

    release_handle();
}

void mooned::render::ShaderProgram::load(const pod::Buffer& buffer)
{
    _data = buffer;
}

void mooned::render::ShaderProgram::load(const char* data, size_t size)
{
    pod::buffer::clear(_data);
    pod::buffer::append(_data, data, size);
}

bool mooned::render::ShaderProgram::valid() const
{
    return !pod::buffer::empty(_data);
}

mooned::render::ShaderProgram* mooned::render::program_from_file(mem::allocator& alloc, ShaderProgram::Stage stage, std::string path)
{
    auto* result = alloc.make<mooned::render::ShaderProgram>(alloc, stage);
    fs::file_ptr file = fs::find_file(path);
    if (file && file->good())
    {
        auto file_size = file->size();
        char* buffer = reinterpret_cast<char*>(alloc.allocate(file_size + 1));
        memset(buffer, 0, file_size + 1);

        // Read the file
        file->read(buffer, 1, file_size);

        // Load it to the shader
        result->load(buffer, file_size + 1);

        alloc.deallocate(buffer);
    }
    return result;
}

mooned::render::ShaderProgram* mooned::render::program_from_buffer(mem::allocator& alloc, ShaderProgram::Stage stage, const pod::Buffer& buffer)
{
    auto* result = alloc.make<mooned::render::ShaderProgram>(alloc, stage);
    result->load(buffer);
    return result;
}

mooned::render::ShaderProgram* mooned::render::program_from_data(mem::allocator& alloc, ShaderProgram::Stage stage, const char* data, uint32_t size)
{
    auto* result = alloc.make<mooned::render::ShaderProgram>(alloc, stage);
    result->load(data, size);
    return result;
}

//////////////////////////////////////////////////////////////////////////
// ProgramPipeline
//////////////////////////////////////////////////////////////////////////

mooned::render::ProgramPipeline::ProgramPipeline()
    : _handle{ 0 }
{
}

bool mooned::render::ProgramPipeline::valid() const
{
    return true;
}
