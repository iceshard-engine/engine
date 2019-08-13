#pragma once
#include <memsys/allocator.h>
#include <collections/pod/hash.h>
#include <collections/data/buffer.h>
#include <kernel/utils/strong_typedef.h>

namespace mooned::render
{

//! Holds detailed information about a given texture.
struct TextureDetails
{
    //! All available texture types.
    enum class Type
    {
        INVALID,
        TEXTURE_1D,
        TEXTURE_2D,
        TEXTURE_2D_MULTISAMPLE,
        TEXTURE_3D,
    };

    Type type;

    //! The color format (internal and displayed)
    enum class ColorFormat
    {
        UNKNOWN,
        RGB,
        RGBA,
    };

    ColorFormat internal_color_format = ColorFormat::UNKNOWN;
    ColorFormat color_format = ColorFormat::UNKNOWN;

    //! The data type used to store color information
    enum class DataType
    {
        UNKNOWN,
        UNSIGNED_BYTE,
    };

    DataType data_type = DataType::UNKNOWN;

    unsigned mipmap_level = 0;

    unsigned sample_number = 0;

    int width = 0;
    int height = 0;
};

//! All available texture property values
enum class TextureProperty
{
    MIN_FILTER_NONE,
    MIN_FILTER_LINEAR,

    MAG_FILTER_NONE,
    MAG_FILTER_LINEAR,
};

//! Defines properties of the pixel format used to store the texture data.
enum class TexturePixelStoreProperty
{
    UNPACK_ROW_LENGTH,
};

//! A render-able texture object
class Texture
{
public:
    using Handle = mooned::strong_numeric_typedef<Texture, uint32_t>;

    //! Creates a texture of the given type
    //! \note The object requires an allocator to properly hold data for textures.
    Texture(mem::allocator& alloc, TextureDetails::Type texture_type);
    Texture(mem::allocator& alloc, TextureDetails texture_data);
    ~Texture();

    //! Returns the texture type
    inline TextureDetails::Type type() const { return _details.type; }

    //! Returns the texture details
    const TextureDetails& details() const { return _details; }

    //! Sets the texture internal and display formats.
    //! \note Changing this setting does only have an effect when a new handle is created!
    void set_format(TextureDetails::ColorFormat internal, TextureDetails::ColorFormat displayed);

    //! Sets the data format used to store color information for this texture
    //! \note Changing this setting does only have an effect when a new handle is created!
    void set_data_type(TextureDetails::DataType data_type);

    //! Sets the sample number for the given multisample texture.
    //! \note Changing this setting does only have an effect when a new handle is created!
    void set_sample_number(unsigned sample_numbver);

    //! Sets the texture width, height and mipmap level
    //! \note Changing this setting does only have an effect when a new handle is created!
    void set_size(int width, int height, unsigned mipmap_level = 0u);

    //! Sets the texture data.
    //! \note Changing this setting does only have an effect when a new handle is created!
    void set_data(const pod::Buffer& data);

    //! Sets a property on the texture
    //! \note Properties are specific to the used backend system, however similar ones are defines in a general enum.
    void set_property(TextureProperty prop);

    //! Sets a property on the texture with the given value
    //! \note Properties are specific to the used backend system, however similar ones are defines in a general enum.
    //! \note The given value should be valid for the given backend
    void set_pixel_store_property(TexturePixelStoreProperty prop, int value);

    //! Returns true if the texture definition is valid and will result in a valid handle.
    //! \note This is only a estimation, and may still result in an error on handle creation.
    bool valid() const;

    //! Returns a handle to be used in a render command buffer or as a render target.
    //! \note If a handle does not exist, it will create one.
    Handle get_handle();

    //! Releases the underlying handle, which makes it unusable in render command buffers or as a render target.
    //! \note If the handle does not exist, nothing happens.
    void release_handle();

private:
    pod::Buffer _data;
    TextureDetails _details;

    //! Defines a single texture property
    struct Property
    {
        uint32_t name;
        int32_t value;
    };

    pod::Hash<Property> _properties;

    Handle _handle;
};

}
