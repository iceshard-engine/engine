#include "image_pipeline.hxx"

namespace ice
{

    auto IceshardImagePipeline::supported_types() const noexcept -> ice::Span<AssetType const>
    {
        static ice::AssetType supported_types[]{
            AssetType::Texture
        };
        return supported_types;
    }

    bool IceshardImagePipeline::supports_baking(
        ice::AssetType type
    ) const noexcept
    {
        return type == AssetType::Texture;
    }

    bool IceshardImagePipeline::resolve(
        ice::String resource_extension,
        ice::Metadata const& resource_metadata,
        ice::AssetType& out_type,
        ice::AssetStatus& out_status
    ) const noexcept
    {
        if (resource_extension == ".jpg"
            || resource_extension == ".jpeg"
            || resource_extension == ".png"
            || resource_extension == ".bmp")
        {
            out_type = AssetType::Texture;
            out_status = AssetStatus::Available_Raw;
            return true;
        }
        return false;
    }

    auto IceshardImagePipeline::request_oven(
        ice::AssetType type,
        ice::String extension,
        ice::Metadata const& metadata
    ) noexcept -> ice::AssetOven const*
    {
        return &_image_oven;
    }

    auto IceshardImagePipeline::request_loader(
        ice::AssetType type
    ) noexcept -> ice::AssetLoader const*
    {
        return &_image_loader;
    }

} // namespace ice
