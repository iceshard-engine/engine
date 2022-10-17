#include "asset_image.hxx"
#include <ice/render/render_image.hxx>

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
            int(data.size.value),
            &width, &height, &channels,
            STBI_rgb_alpha
        );

        if (image_buffer != nullptr)
        {
            ice::meminfo image_meminfo = ice::meminfo_of<ImageInfo>;
            ice::usize const offset_data = image_meminfo += ice::meminfo_of<stbi_uc> * width * height * 4;
            ice::Memory image_mem = alloc.allocate(image_meminfo);

            ice::render::ImageInfo* texture = reinterpret_cast<ImageInfo*>(image_mem.location);
            texture->type = ice::render::ImageType::Image2D;
            texture->width = width;
            texture->height = height;
            texture->format = ice::render::ImageFormat::UNORM_RGBA;
            texture->usage = ice::render::ImageUsageFlags::TransferDst
                | ice::render::ImageUsageFlags::Sampled;
            texture->data = std::bit_cast<void const*>(offset_data.value);

            ice::memcpy(
                ice::ptr_add(image_mem, offset_data),
                ice::Data{
                    .location = image_buffer,
                    .size = ice::size_of<stbi_uc> * width * height * 4,
                    .alignment = ice::align_of<stbi_uc>
                }
            );

            stbi_image_free(image_buffer);
            memory = image_mem;
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
        out_data = alloc.allocate(ice::meminfo_of<ImageInfo>);

        ImageInfo const& image_data = *reinterpret_cast<ImageInfo const*>(data.location);
        ImageInfo* image = reinterpret_cast<ImageInfo*>(out_data.location);
        image->type = image_data.type;
        image->usage = image_data.usage;
        image->format = image_data.format;
        image->width = image_data.width;
        image->height = image_data.height;
        image->data = ice::ptr_add(
            data.location,
            { std::bit_cast<ice::usize::base_type>(image_data.data) }
        );

        co_return true;
    }

    void asset_type_image_definition(ice::AssetTypeArchive& asset_type_archive) noexcept
    {
        static ice::String extensions[]{ ".jpg", ".png", ".jpeg", ".bmp" };

        static ice::AssetTypeDefinition type_definition{
            .resource_extensions = extensions,
            .fn_asset_oven = asset_image_oven,
            .fn_asset_loader = asset_image_loader
        };

        asset_type_archive.register_type(ice::render::AssetType_Texture2D, type_definition);
    }

} // namespace ice
