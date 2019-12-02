#include <renderlib/render_vertex_array.h>

#include <gl/glew.h>
#include <gl/GL.h>

#include <cassert>

namespace mooned::render::opengl
{

GLenum vertexarray_attribute_type(VertexArrayAttribute::Type type)
{
    GLenum result = GL_INVALID_ENUM;

    switch (type)
    {
    case VertexArrayAttribute::Type::FLOAT:
        result = GL_FLOAT;
        break;
    case VertexArrayAttribute::Type::UNSIGNED_BYTE:
        result = GL_UNSIGNED_BYTE;
        break;
    }

    assert(result != GL_INVALID_ENUM);
    return result;
}

}

bool mooned::render::VertexArray::valid() const
{
    return _handle != 0;
}

mooned::render::VertexArray::Handle mooned::render::VertexArray::get_handle()
{
    if (!valid())
    {
        glCreateVertexArrays(1, std::addressof(_handle._value));
        glBindVertexArray(static_cast<uint32_t>(_handle));
        assert(glGetError() == GL_NO_ERROR);

        for (auto& binding : _bindings)
        {
            auto* attrib = pod::multi_hash::find_first(_binding_attributes, binding.index);
            while (nullptr != attrib)
            {
                const auto& attrib_format = attrib->value;
                glVertexAttribFormat(attrib_format.index, attrib_format.size, mooned::render::opengl::vertexarray_attribute_type(attrib_format.type), attrib_format.normalized, attrib_format.offset);
                glVertexAttribBinding(attrib_format.index, binding.index);
                glEnableVertexArrayAttrib(static_cast<uint32_t>(_handle), attrib_format.index);

                attrib = pod::multi_hash::find_next(_binding_attributes, attrib);
            }
            glVertexBindingDivisor(binding.index, binding.divisor);
        }

        assert(glGetError() == GL_NO_ERROR);

        glBindVertexArray(0);
    }

    return _handle;
}

void mooned::render::VertexArray::release_handle()
{
    if (valid())
    {
        glDeleteVertexArrays(1, std::addressof(_handle._value));
        _handle = 0;
    }
}

