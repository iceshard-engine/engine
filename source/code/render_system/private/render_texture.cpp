#include <renderlib/render_texture.h>

namespace mooned::render
{

Texture::Texture(mem::allocator& alloc, TextureDetails::Type texture_type)
    : _data{ alloc }
    , _details{ texture_type }
    , _properties{ alloc }
    , _handle{ 0 }
{ }

Texture::Texture(mem::allocator& alloc, TextureDetails texture_data)
    : _data{ alloc }
    , _details{ std::move(texture_data) }
    , _properties{ alloc }
    , _handle{ 0 }
{ }

Texture::~Texture()
{
    pod::buffer::clear(_data);
    pod::hash::clear(_properties);

    release_handle();
}

void Texture::set_format(TextureDetails::ColorFormat internal, TextureDetails::ColorFormat displayed)
{
    _details.internal_color_format = internal;
    _details.color_format = displayed;
}

void Texture::set_data_type(TextureDetails::DataType data_type)
{
    _details.data_type = data_type;
}

void Texture::set_sample_number(unsigned sample_number)
{
    _details.sample_number = sample_number;
}

void Texture::set_size(int width, int height, unsigned mipmap_level /*= 0u*/)
{
    _details.width = width;
    _details.height = height;
    _details.mipmap_level = mipmap_level;
}

void Texture::set_data(const pod::Buffer& data)
{
    _data = data;
}

bool Texture::valid() const
{
    bool result = true;
    result &= _details.mipmap_level < 25;
    result &= _details.width > 0;
    result &= _details.height > 0;
    result &= _details.color_format != TextureDetails::ColorFormat::UNKNOWN;
    result &= _details.internal_color_format != TextureDetails::ColorFormat::UNKNOWN;
    if (_details.type != TextureDetails::Type::TEXTURE_2D_MULTISAMPLE)
    {
        result &= _details.data_type != TextureDetails::DataType::UNKNOWN;
    }
    else
    {
        result &= _details.sample_number > 0;
    }
    return result;
}

} // namespace mooned::render
