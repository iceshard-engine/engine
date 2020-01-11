#pragma once
#include <core/string_view.hxx>
#include <resource/resource_meta.hxx>
#include <asset_system/asset.hxx>

namespace asset
{

    class AssetResolver
    {
    public:
        virtual ~AssetResolver() noexcept = default;

        virtual auto resolve_asset_type(core::StringView extension, resource::ResourceMetaView const& meta) noexcept -> AssetType = 0;
    };

} // namespace asset
