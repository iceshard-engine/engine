#include <renderlib/render_buffer.h>

#include <GL/glew.h>
#include <GL/GL.h>

mooned::render::RenderBuffer::Handle mooned::render::RenderBuffer::get_handle()
{
    if (_handle == 0)
    {
        glGenBuffers(1, std::addressof(_handle._value));
    }
    return _handle;
}

void mooned::render::RenderBuffer::release_handle()
{
    if (_handle != 0)
    {
        glDeleteBuffers(1, std::addressof(_handle._value));
    }
}
