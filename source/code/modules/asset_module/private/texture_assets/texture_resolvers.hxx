#pragma once
#include <asset_system/asset_resolver.hxx>

namespace iceshard
{

    class StbTextureResolver final : public asset::AssetResolver
    {
    public:
        StbTextureResolver() noexcept = default;

        auto resolve_asset_type(
            core::StringView extension,
            resource::ResourceMetaView const& meta
        ) noexcept -> asset::AssetType override;
    };

} // namespace iceshard
