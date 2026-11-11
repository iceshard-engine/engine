/// Copyright 2022 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "asset_image.hxx"
#include <ice/config.hxx>
#include <ice/asset_storage.hxx>
#include <ice/resource.hxx>
#include <ice/resource_compiler_api.hxx>
#include <ice/render/render_image.hxx>
#include <ice/render/render_assets.hxx>

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

    static ice::String Constant_ImageExtensions[]{ ".jpg", ".png", ".jpeg", ".bmp" };

    static auto asset_image_compiler_supported_images(
        ice::Span<ice::Shard const> params
    ) noexcept -> ice::Span<ice::String const>
    {
        return Constant_ImageExtensions;
    }

    static auto asset_image_oven(
        ice::ResourceCompilerCtx& ctx,
        ice::ResourceHandle const& resource_handle,
        ice::ResourceTracker& resource_tracker,
        ice::Span<ice::ResourceHandle const> sources,
        ice::Span<ice::URI const> dependencies,
        ice::Allocator& result_alloc
    ) noexcept -> ice::Task<ice::ResourceCompilerResult>
    {
        IPT_ZONE_SCOPED;
        using ice::render::ImageInfo;

        ice::ResourceResult const res = co_await resource_tracker.load_resource(resource_handle);
        ICE_ASSERT_CORE(res.resource_status == ResourceStatus::Loaded);
        IPT_ZONE_TEXT_STR(res.resource->origin());

        ice::Data const data = res.data;
        ICE_ASSERT(data.size <= ice::usize{ice::i32_max}, "Image data larger than what can be baked! [size: {}]", data.size);

        stbi_uc const* image_buffer_raw = static_cast<stbi_uc const*>(data.location);

        ice::i32 width = 0;
        ice::i32 height = 0;
        ice::i32 channels = 0;

        stbi_uc* const image_buffer = stbi_load_from_memory(
            image_buffer_raw,
            static_cast<int>(data.size.value),
            &width, &height, &channels,
            STBI_rgb_alpha
        );

        ice::Memory out_memory{};
        if (image_buffer != nullptr)
        {
            ImageInfo* image_info;
            stbi_uc* image_data;

            ice::ChunkedAllocRequest alloc_request{};
            alloc_request.include(image_info, 1);
            alloc_request.include(image_data, width * height * channels);
            out_memory = result_alloc.allocate(alloc_request);

            image_info->type = ice::render::ImageType::Image2D;
            image_info->width = width;
            image_info->height = height;
            image_info->format = ice::render::ImageFormat::UNORM_RGBA;
            image_info->usage = ice::render::ImageUsageFlags::TransferDst
                | ice::render::ImageUsageFlags::Sampled;
            image_info->data.set_from_pointer(image_data);// ptr_offset_as_ptr(image_info, image_data);



            ice::memcpy(
                image_data,
                ice::Data{
                    .location = image_buffer,
                    .size = ice::size_of<stbi_uc> * width * height * 4,
                    .alignment = ice::align_of<stbi_uc>
                }
            );

            stbi_image_free(image_buffer);
        }

        co_return ResourceCompilerResult{ out_memory };
    }

    static auto asset_image_state(
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

    static auto asset_image_loader(
        void*,
        ice::Allocator& alloc,
        ice::AssetStorage&,
        ice::Config const&,
        ice::Data data,
        ice::Memory& out_memory
    ) noexcept -> ice::Task<bool>
    {
        // For Baked images the Loaded state is the same as the baked state.
        // Returning 'true' allows the AssetStorage to promote the baked data to Loaded state.
        co_return true;
    }

    void asset_category_image_definition(
        ice::AssetCategoryArchive& asset_category_archive,
        ice::ModuleQuery const& module_query
    ) noexcept
    {
        static constexpr ice::ResourceCompiler compiler{
            .fn_supported_resources = asset_image_compiler_supported_images,
            .fn_compile_source = asset_image_oven,
        };

        static ice::AssetCategoryDefinition const definition{
            .resource_extensions = Constant_ImageExtensions,
            .fn_asset_state = asset_image_state,
            .fn_asset_loader = asset_image_loader,
        };

        asset_category_archive.register_category(ice::render::AssetCategory_Texture2D, definition, &compiler);
    }

} // namespace ice
