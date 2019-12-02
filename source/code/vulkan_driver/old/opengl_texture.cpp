#include <renderlib/render_texture.h>

#include <gl/glew.h>
#include <gl/GL.h>

#include <cassert>

namespace mooned::render::opengl
{

GLenum texture_type(TextureDetails::Type type)
{
    GLenum result = GL_INVALID_ENUM;

    switch (type)
    {
    case TextureDetails::Type::TEXTURE_1D:
        result = GL_TEXTURE_1D;
        break;
    case TextureDetails::Type::TEXTURE_2D:
        result = GL_TEXTURE_2D;
        break;
    case TextureDetails::Type::TEXTURE_2D_MULTISAMPLE:
        result = GL_TEXTURE_2D_MULTISAMPLE;
        break;
    case TextureDetails::Type::TEXTURE_3D:
        result = GL_TEXTURE_3D;
        break;
    default:
        break;
    }

    assert(result != GL_INVALID_ENUM);
    return result;
}

GLenum texture_color_format(TextureDetails::ColorFormat color_format)
{
    GLenum result = GL_INVALID_ENUM;

    switch (color_format)
    {
    case TextureDetails::ColorFormat::RGB:
        result = GL_RGB;
        break;
    case TextureDetails::ColorFormat::RGBA:
        result = GL_RGBA;
        break;
    }

    assert(result != GL_INVALID_ENUM);
    return result;
}

GLenum texure_data_type(TextureDetails::DataType data_type)
{
    GLenum result = GL_INVALID_ENUM;

    switch (data_type)
    {
    case TextureDetails::DataType::UNSIGNED_BYTE:
        result = GL_UNSIGNED_BYTE;
        break;
    }

    assert(result != GL_INVALID_ENUM);
    return result;
}

GLenum texture_property_name(TextureProperty prop)
{
    GLenum result = GL_INVALID_ENUM;

    switch (prop)
    {
    case TextureProperty::MAG_FILTER_NONE:
        [[fallthrough]];
    case TextureProperty::MAG_FILTER_LINEAR:
        result = GL_TEXTURE_MAG_FILTER;
        break;
    case TextureProperty::MIN_FILTER_NONE:
        [[fallthrough]];
    case TextureProperty::MIN_FILTER_LINEAR:
        result = GL_TEXTURE_MIN_FILTER;
        break;
    }

    assert(result != GL_INVALID_ENUM);
    return result;
}

GLint texture_property_value(TextureProperty prop)
{
    GLenum result = GL_INVALID_ENUM;

    switch (prop)
    {
    case TextureProperty::MAG_FILTER_LINEAR:
    case TextureProperty::MIN_FILTER_LINEAR:
        result = GL_LINEAR;
        break;
    case TextureProperty::MAG_FILTER_NONE:
    case TextureProperty::MIN_FILTER_NONE:
        result = GL_NONE;
        break;
    }

    assert(result != GL_INVALID_ENUM);
    return result;
}

GLenum pixel_store_property_name(TexturePixelStoreProperty prop)
{
    GLenum result = GL_INVALID_ENUM;

    switch (prop)
    {
    case TexturePixelStoreProperty::UNPACK_ROW_LENGTH:
        result = GL_UNPACK_ROW_LENGTH;
        break;
    }

    assert(result != GL_INVALID_ENUM);
    return result;
}

static constexpr uint64_t GeneralTextureProperty = 0x01;
static constexpr uint64_t PixelStoreTextureProperty = 0x02;

}

void mooned::render::Texture::set_property(TextureProperty prop)
{
    pod::multi_hash::insert(_properties, opengl::GeneralTextureProperty, {
        opengl::texture_property_name(prop),
        opengl::texture_property_value(prop)
    });
}

void mooned::render::Texture::set_pixel_store_property(TexturePixelStoreProperty prop, int value)
{
    pod::multi_hash::insert(_properties, opengl::PixelStoreTextureProperty, {
        opengl::pixel_store_property_name(prop),
        value
    });
}

mooned::render::Texture::Handle mooned::render::Texture::get_handle()
{
    assert(valid()); // Check we got all required values set.

    if (_handle == 0)
    {
        glGenTextures(1, std::addressof(_handle._value));

        auto type = _details.type;
        auto gl_type = opengl::texture_type(type);

        glBindTexture(gl_type, static_cast<uint32_t>(_handle));

        if (type == TextureDetails::Type::TEXTURE_2D)
        {
            glTexImage2D(gl_type,
                _details.mipmap_level,
                opengl::texture_color_format(_details.internal_color_format),
                _details.width,
                _details.height, 0,
                opengl::texture_color_format(_details.color_format),
                opengl::texure_data_type(_details.data_type),
                pod::buffer::data(_data)
            );
        }

        if (type == TextureDetails::Type::TEXTURE_2D_MULTISAMPLE)
        {
            glTexImage2DMultisample(gl_type,
                _details.sample_number,
                opengl::texture_color_format(_details.internal_color_format),
                _details.width,
                _details.height,
                GL_TRUE
            );
        }

        //! General properties
        auto* it = pod::multi_hash::find_first(_properties, opengl::GeneralTextureProperty);
        while (it)
        {
            glTexParameteri(gl_type, it->value.name, it->value.value);
            it = pod::multi_hash::find_next(_properties, it);
        }

        //! Pixel store properties
        it = pod::multi_hash::find_first(_properties, opengl::PixelStoreTextureProperty);
        while (it)
        {
            glPixelStorei(it->value.name, it->value.value);
            it = pod::multi_hash::find_next(_properties, it);
        }

        glBindTexture(gl_type, 0);
    }

    return _handle;
}

void mooned::render::Texture::release_handle()
{
    if (_handle != 0)
    {
        glDeleteTextures(1, std::addressof(_handle._value));
        _handle = 0;
    }
}
