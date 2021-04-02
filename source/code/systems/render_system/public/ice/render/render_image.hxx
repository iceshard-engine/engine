#pragma once
#include <ice/data.hxx>

namespace ice::render
{

    enum class Image : ice::uptr
    {
        Invalid = 0x0
    };

    enum class ImageType : ice::u32
    {
        Image2D,
    };

    enum class ImageFormat : ice::u32
    {
        Invalid = 0,
        I32_RGBA,
        SRGB_RGBA,
        SRGB_BGRA,
        UNORM_RGBA,
        UNORM_BGRA,
        UNORM_D24_UINT_S8,
        SFLOAT_D32,
        SFLOAT_D32_UINT_S8,
    };

    enum class ImageUsageFlags : ice::u32
    {
        None = 0x0,
        Sampled = 0x0000'0001,
        ColorAttachment = 0x0001'0000,
        InputAttachment = 0x0002'0000,
        DepthStencilAttachment = 0x0004'0000,
    };

    enum class ImageLayout : ice::u32
    {
        Undefined,
        Color,
        Present,
        DepthStencil,
        ShaderReadOnly,
    };

    struct ImageInfo
    {
        ice::render::ImageType type;
        ice::render::ImageFormat format;
        ice::render::ImageUsageFlags usage;
        ice::u32 width;
        ice::u32 height;
        void const* data;
    };

    constexpr auto operator|(ImageUsageFlags left, ImageUsageFlags right) noexcept -> ImageUsageFlags
    {
        ice::u32 left_value = static_cast<ice::u32>(left);
        ice::u32 right_value = static_cast<ice::u32>(right);
        return static_cast<ImageUsageFlags>(left_value | right_value);
    }

    enum class Sampler : ice::uptr
    {
        Invalid
    };

    enum class SamplerFilter : ice::u32
    {
        Nearest,
        Linear,
        CubicImg,
        CubicExt,
    };

    enum class SamplerAddressMode : ice::u32
    {
        Repeat,
        RepeatMirrored,
        ClampToEdge,
        ClampToEdgeMirrored,
        ClampToBorder,
    };

    enum class SamplerMipMapMode : ice::u32
    {
        None,
        Nearest,
        Linear,
    };

    struct SamplerInfo
    {
        ice::render::SamplerFilter min_filter;
        ice::render::SamplerFilter mag_filter;

        struct AddressMode
        {
            ice::render::SamplerAddressMode u;
            ice::render::SamplerAddressMode v;
            ice::render::SamplerAddressMode w;
        } address_mode;

        ice::render::SamplerMipMapMode mip_map_mode;

        bool normalized_coordinates = true;
    };

} // namespace ice::render
