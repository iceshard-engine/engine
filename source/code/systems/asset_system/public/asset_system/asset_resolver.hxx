#pragma once
#include <core/string_view.hxx>
#include <resource/resource_meta.hxx>
#include <asset_system/asset.hxx>

namespace asset
{

    class AssetCompiler;

    class AssetResolver
    {
    public:
        virtual ~AssetResolver() noexcept = default;

        virtual bool resolve_asset_info(
            core::StringView extension,
            resource::ResourceMetaView const& meta,
            asset::AssetStatus& status_out,
            asset::AssetType& type_out
        ) noexcept
        {
            type_out = resolve_asset_type(extension, meta);
            status_out = asset::AssetStatus::Available;
            return type_out != asset::AssetType::Unresolved;
        }

        [[deprecated]]
        virtual auto resolve_asset_type(
            core::StringView extension,
            resource::ResourceMetaView const& meta
        ) noexcept -> AssetType
        {
            IS_ASSERT(false, "Deprecated method called!");
            return AssetType::Unresolved;
        }
    };

} // namespace asset
