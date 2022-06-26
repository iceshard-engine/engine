#include "asset_image.hxx"
#include <ice/render/render_image.hxx>
#include <ice/memory/pointer_arithmetic.hxx>

#define STB_IMAGE_IMPLEMENTATION
#if ISP_COMPILER_GCC
#   pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#   pragma GCC diagnostic ignored "-Wsign-compare"
#   include "asset_image_external/stb_image.hxx"
#   pragma GCC diagnostic pop
#else
#   include "asset_image_external/stb_image.hxx"
#endif
#undef assert

namespace ice
{

    auto asset_image_oven(
        void*,
        ice::Allocator& alloc,
        ice::ResourceTracker const&,
        ice::Resource_v2 const& resource,
        ice::Data data,
        ice::Memory& memory
    ) noexcept -> ice::Task<bool>
    {
        using ice::render::ImageInfo;

        stbi_uc const* image_buffer_raw = reinterpret_cast<stbi_uc const*>(data.location);

        ice::i32 width = 0;
        ice::i32 height = 0;
        ice::i32 channels = 0;

        stbi_uc* const image_buffer = stbi_load_from_memory(
            image_buffer_raw,
            data.size,
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

            ice::Buffer out_data{ alloc };
            ice::buffer::reserve(out_data, sizeof(ImageInfo) + image_data.size + 8);

            void* image_ptr = ice::buffer::append(
                out_data,
                ice::Data{ nullptr, sizeof(ImageInfo), alignof(ImageInfo) }
            );

            void* data_ptr = ice::buffer::append(out_data, image_data);

            auto* texture = reinterpret_cast<ImageInfo*>(image_ptr);
            texture->type = ice::render::ImageType::Image2D;
            texture->width = width;
            texture->height = height;
            texture->format = ice::render::ImageFormat::UNORM_RGBA;
            texture->usage = ice::render::ImageUsageFlags::TransferDst
                | ice::render::ImageUsageFlags::Sampled;
            texture->data = reinterpret_cast<void const*>(
                static_cast<ice::uptr>(
                    ice::memory::ptr_distance(image_ptr, data_ptr)
                )
            );

            memory = ice::buffer::extrude_memory(out_data);
            stbi_image_free(image_buffer);
        }

        co_return width != 0 && height != 0;
    }

    auto asset_image_loader(
        void*,
        ice::Allocator& alloc,
        ice::AssetStorage&,
        ice::Metadata const& meta,
        ice::Data data,
        ice::Memory& out_data
    ) noexcept -> ice::Task<bool>
    {
        using ice::render::ImageInfo;

        out_data.size = sizeof(ImageInfo);
        out_data.alignment = alignof(ImageInfo);
        out_data.location = alloc.allocate(out_data.size, out_data.alignment);

        ImageInfo const& image_data = *reinterpret_cast<ImageInfo const*>(data.location);
        ImageInfo* image = reinterpret_cast<ImageInfo*>(out_data.location);
        image->type = image_data.type;
        image->usage = image_data.usage;
        image->format = image_data.format;
        image->width = image_data.width;
        image->height = image_data.height;
        image->data = ice::memory::ptr_add(
            data.location,
            static_cast<ice::u32>(reinterpret_cast<ice::uptr>(image_data.data))
        );

        co_return true;
    }

    void asset_type_image_definition(ice::AssetTypeArchive& asset_type_archive) noexcept
    {
        static ice::Utf8String extensions[]{ u8".jpg", u8".png", u8".jpeg", u8".bmp" };

        static ice::AssetTypeDefinition type_definition{
            .resource_extensions = extensions,
            .fn_asset_oven = asset_image_oven,
            .fn_asset_loader = asset_image_loader
        };

        asset_type_archive.register_type(ice::render::AssetType_Texture2D, type_definition);
    }

} // namespace ice
