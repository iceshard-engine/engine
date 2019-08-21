#include <renderlib/render_program.h>

#include <memsys/allocators/scratch_allocator.h>
#include <kernel/logger/logger.h>

#include <gl/glew.h>
#include <gl/GL.h>

#include <cassert>

//////////////////////////////////////////////////////////////////////////
// Shader Program
//////////////////////////////////////////////////////////////////////////

namespace mooned::render::opengl
{

GLenum shader_program_stage(ShaderProgram::Stage stage)
{
    GLenum result = GL_INVALID_ENUM;

    switch (stage)
    {
    case ShaderProgram::Stage::VERTEX:
        result = GL_VERTEX_SHADER;
        break;
    case ShaderProgram::Stage::FRAGMENT:
        result = GL_FRAGMENT_SHADER;
        break;
    case ShaderProgram::Stage::COMPUTE:
        result = GL_COMPUTE_SHADER;
        break;
    case ShaderProgram::Stage::GEOMETRY:
        result = GL_GEOMETRY_SHADER;
        break;
    }

    assert(result != GL_INVALID_ENUM);
    return result;
}

}

mooned::render::ShaderProgram::Handle mooned::render::ShaderProgram::get_handle()
{
    if (_handle == 0 && valid())
    {
        GLuint shader_handle = glCreateShader(opengl::shader_program_stage(_stage));

        const char* source_data = pod::buffer::data(_data);
        glShaderSource(shader_handle, 1, &source_data, nullptr);
        glCompileShader(shader_handle);

        GLint shader_compiled_successful = GL_FALSE;
        glGetShaderiv(shader_handle, GL_COMPILE_STATUS, &shader_compiled_successful);
        if (shader_compiled_successful != GL_TRUE)
        {
            GLint log_message_length = 0;
            glGetShaderiv(shader_handle, GL_INFO_LOG_LENGTH, &log_message_length);

            // This allocator is going to release it's memory when going out of bounds
            mem::scratch_allocator alloc{ _allocator, 1 KB };
            GLchar* buffer = reinterpret_cast<GLchar*>(alloc.allocate(log_message_length * sizeof(GLchar)));

            glGetShaderInfoLog(shader_handle, log_message_length, nullptr, buffer);
            MLogError("Shader compilation error: {}", buffer);
            alloc.deallocate(buffer);

            // Return an invalid handle
            return _handle;
        }

        // Create the opengl program handle and try to link it with the shader
        GLuint new_handle = glCreateProgram();

        glAttachShader(new_handle, shader_handle);

        glProgramParameteri(new_handle, GL_PROGRAM_SEPARABLE, GL_TRUE);
        glProgramParameteri(new_handle, GL_PROGRAM_BINARY_RETRIEVABLE_HINT, GL_TRUE);
        glLinkProgram(new_handle);

        glDetachShader(new_handle, shader_handle);
        glDeleteShader(shader_handle);

        GLint program_linked_successful = GL_FALSE;
        glGetProgramiv(new_handle, GL_LINK_STATUS, &program_linked_successful);
        if (program_linked_successful != GL_TRUE)
        {
            GLint log_message_length = 0;
            glGetProgramiv(new_handle, GL_INFO_LOG_LENGTH, &log_message_length);

            // This allocator is going to release it's memory when going out of bounds
            mem::scratch_allocator alloc{ _allocator, 1 KB };
            auto* buffer = reinterpret_cast<GLchar*>(alloc.allocate(log_message_length * sizeof(GLchar)));

            glGetProgramInfoLog(new_handle, log_message_length, nullptr, buffer);
            MLogError("Program linking error: {}", buffer);
            alloc.deallocate(buffer);

            // Release the handle
            glDeleteProgram(new_handle);
            return _handle;
        }

        // Set the handle
        _handle = new_handle;
        assert(_handle != 0);

        MLogDebug("Shader program creation successful.");
    }

    return _handle;
}

void mooned::render::ShaderProgram::release_handle()
{
    if (_handle != 0)
    {
        glDeleteProgram(_handle._value);
        _handle = 0;
    }
}

//////////////////////////////////////////////////////////////////////////
// ProgramPipeline
//////////////////////////////////////////////////////////////////////////

mooned::render::ProgramPipeline::Handle mooned::render::ProgramPipeline::get_handle()
{
    if (_handle == 0)
    {
        glGenProgramPipelines(1, std::addressof(_handle._value));
    }

    return _handle;
}

void mooned::render::ProgramPipeline::release_handle()
{
    if (_handle != 0)
    {
        glDeleteProgramPipelines(1, std::addressof(_handle._value));
    }
}
