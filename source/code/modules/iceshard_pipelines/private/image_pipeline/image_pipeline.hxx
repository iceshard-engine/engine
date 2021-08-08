#pragma once
#include <ice/asset_pipeline.hxx>
#include "image_oven.hxx"
#include "image_loader.hxx"

namespace ice
{

    class IceshardImagePipeline final : public ice::AssetPipeline
    {
    public:
        auto supported_types() const noexcept -> ice::Span<AssetType const> override;

        bool supports_baking(
            ice::AssetType type
        ) const noexcept override;

        bool resolve(
            ice::String resource_extension,
            ice::Metadata resource_metadata,
            ice::AssetType& out_type,
            ice::AssetStatus& out_status
        ) noexcept override;

        auto request_oven(
            ice::AssetType type
        ) noexcept -> ice::AssetOven* override;

        auto request_loader(
            ice::AssetType type
        ) noexcept -> ice::AssetLoader* override;

    private:
        ice::IceshardImageOven _image_oven;
        ice::IceshardImageLoader _image_loader;
    };

} // namespace ice
