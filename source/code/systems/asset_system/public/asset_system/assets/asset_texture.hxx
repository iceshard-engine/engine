#pragma once
#include <core/pointer.hxx>
#include <asset_system/asset.hxx>
#include <asset_system/asset_loader.hxx>
#include <asset_system/asset_resolver.hxx>

namespace asset
{

    struct AssetTexture : public Asset
    {
        constexpr AssetTexture() noexcept = default;

        constexpr AssetTexture(core::StringView view) noexcept
            : Asset{ view, AssetType::Texture }
        {
        }
    };

} // namespace
