#include "image_oven.hxx"
#include <ice/render/render_image.hxx>
#include <ice/buffer.hxx>

#define STB_IMAGE_IMPLEMENTATION
#include "external/stb_image.hxx"

namespace ice
{

    auto ice::IceshardImageOven::bake(
        ice::Data resource_data,
        ice::Metadata const& resource_meta,
        ice::ResourceSystem& resource_system,
        ice::Allocator& asset_alloc,
        ice::Memory& asset_data
    ) noexcept -> ice::BakeResult
    {
        using ice::render::ImageInfo;

        stbi_uc const* image_buffer_raw = reinterpret_cast<stbi_uc const*>(resource_data.location);

        ice::i32 width = 0;
        ice::i32 height = 0;
        ice::i32 channels = 0;

        stbi_uc* const image_buffer = stbi_load_from_memory(
            image_buffer_raw,
            resource_data.size,
            &width, &height, &channels,
            STBI_rgb_alpha
        );

        if (image_buffer != nullptr)
        {
            ice::Data image_data = ice::data_view(
                image_buffer,
                width * height * 4 * sizeof(stbi_uc),
                alignof(stbi_uc)
            );

            ice::Buffer out_data{ asset_alloc };
            ice::buffer::reserve(out_data, sizeof(ImageInfo) + image_data.size + 8);

            void* image_ptr = ice::buffer::append(
                out_data,
                ice::Data{ nullptr, sizeof(ImageInfo) }
            );

            void* data_ptr = ice::buffer::append(out_data, image_data);

            auto* texture = reinterpret_cast<ImageInfo*>(image_ptr);
            texture->width = width;
            texture->height = height;
            texture->format = ice::render::ImageFormat::UNORM_RGBA;
            texture->data = reinterpret_cast<void const*>(
                static_cast<ice::uptr>(
                    ice::memory::ptr_distance(image_ptr, data_ptr)
                )
            );

            return BakeResult::Success;
        }
        return BakeResult::Failure_InvalidData;
    }

} // namespace ice
