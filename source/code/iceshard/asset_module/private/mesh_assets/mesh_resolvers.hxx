#pragma once
#include <asset_system/asset_resolver.hxx>

namespace iceshard
{

    class AssimpMeshResolver final : public asset::AssetResolver
    {
    public:
        AssimpMeshResolver() noexcept = default;

        auto resolve_asset_type(
            core::StringView extension,
            resource::ResourceMetaView const& meta
        ) noexcept -> asset::AssetType override;
    };

} // namespace iceshard
