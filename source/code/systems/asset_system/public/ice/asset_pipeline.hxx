#pragma once
#include <ice/span.hxx>
#include <ice/string.hxx>
#include <ice/resource_meta.hxx>
#include <ice/asset.hxx>

namespace ice
{

    class AssetOven;
    class AssetLoader;

    class AssetPipeline
    {
    public:
        virtual ~AssetPipeline() noexcept = default;

        virtual auto supported_types() const noexcept -> ice::Span<AssetType> = 0;

        virtual bool supports_baking(ice::AssetType type) const noexcept = 0;

        virtual bool resolve(
            ice::String resource_extension,
            ice::Metadata resource_metadata,
            ice::AssetType& out_type,
            ice::AssetStatus& out_status
        ) noexcept = 0;

        virtual auto request_oven(
            ice::AssetType type
        ) noexcept -> ice::AssetOven* = 0;

        virtual auto request_loader(
            ice::AssetType type
        ) noexcept -> ice::AssetLoader* = 0;
    };

} // namespace ice
