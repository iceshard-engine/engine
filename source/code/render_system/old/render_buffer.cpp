#include <renderlib/render_buffer.h>

mooned::render::RenderBuffer::RenderBuffer()
    : _handle{ 0 }
{
}

bool mooned::render::RenderBuffer::valid() const
{
    return true;
}

