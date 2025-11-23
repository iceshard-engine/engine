/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "asset_image.hxx"
#include <ice/config.hxx>
#include <ice/asset_storage.hxx>
#include <ice/resource_compiler_api.hxx>
#include <ice/render/render_image.hxx>

#define STB_IMAGE_IMPLEMENTATION
#if ISP_COMPILER_GCC
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#   pragma GCC diagnostic ignored "-Wsign-compare"
#   include "asset_image_external/stb_image.h"
#   pragma GCC diagnostic pop
#elif ISP_COMPILER_CLANG
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Wparentheses-equality"
#   pragma clang diagnostic ignored "-Wunused-but-set-variable"
#   include "asset_image_external/stb_image.h"
#   pragma clang diagnostic pop
#else
#   include "asset_image_external/stb_image.h"
#endif
#undef assert

namespace ice
{


    auto asset_image_state(
        void*,
        ice::AssetCategoryDefinition const&,
        ice::Config const& metadata,
        ice::URI const& uri
    ) noexcept -> ice::AssetState
    {
        bool baked = false;
        if (ice::config::get(metadata, "ice.shader.baked", baked) && baked)
        {
            return AssetState::Baked;
        }
        return AssetState::Raw;
    }

    auto asset_image_oven(
        ice::ResourceCompilerCtx& ctx,
        ice::ResourceHandle const& resource_handle,
        ice::ResourceTracker& resource_tracker,
        ice::Span<ice::ResourceHandle const> sources,
        ice::Span<ice::URI const> dependencies,
        ice::Allocator& result_alloc
    ) noexcept -> ice::Task<ice::ResourceCompilerResult>
    {
        using ice::render::ImageInfo;

        ice::ResourceResult const res = co_await resource_tracker.load_resource(resource_handle);
        ice::Data const data = res.data;
        ICE_ASSERT_CORE(res.resource_status == ResourceStatus::Loaded);

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

        ice::Memory image_mem{};
        if (image_buffer != nullptr)
        {
            ice::meminfo image_meminfo = ice::meminfo_of<ImageInfo>;
            ice::usize const offset_data = image_meminfo += ice::meminfo_of<stbi_uc> * width * height * 4;
            image_mem = result_alloc.allocate(image_meminfo);

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
        }

        co_return ResourceCompilerResult{ image_mem };
    }

    auto asset_image_loader(
        void*,
        ice::Allocator& alloc,
        ice::AssetStorage&,
        ice::Config const& meta,
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

    void asset_category_image_definition(
        ice::AssetCategoryArchive& asset_category_archive,
        ice::ModuleQuery const& module_query
    ) noexcept
    {
        static ice::String extensions[]{ ".jpg", ".png", ".jpeg", ".bmp" };

        static ice::ResourceCompiler const compiler{
            .fn_compile_source = asset_image_oven,
        };

        static ice::AssetCategoryDefinition const definition{
            .resource_extensions = extensions,
            .fn_asset_loader = asset_image_loader,
        };

        asset_category_archive.register_category(ice::render::AssetCategory_Texture2D, definition, &compiler);
    }

} // namespace ice
