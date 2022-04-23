#include <ice/asset.hxx>
#include <ice/resource.hxx>

#include "asset_entry.hxx"

namespace ice
{

    bool asset_check(ice::Asset const& asset, ice::AssetState expected_state) noexcept
    {
        return asset_state(asset) == expected_state;
    }

    auto asset_metadata(ice::Asset const& asset) noexcept -> ice::Metadata const&
    {
        return asset_metadata(asset.handle);
    }

    auto asset_metadata(ice::AssetHandle const* handle) noexcept -> ice::Metadata const&
    {
        if (handle == nullptr)
        {
            static ice::Metadata empty_metadata{ };
            return empty_metadata;
        }

        return reinterpret_cast<ice::AssetEntry const*>(handle)->resource->metadata();
    }

    auto asset_state(ice::Asset const& asset) noexcept -> ice::AssetState
    {
        return asset_state(asset.handle);
    }

    auto asset_state(ice::AssetHandle const* handle) noexcept -> ice::AssetState
    {
        if (handle == nullptr)
        {
            return AssetState::Invalid;
        }

        return reinterpret_cast<ice::AssetEntry const*>(handle)->state;
    }

} // namespace ice
