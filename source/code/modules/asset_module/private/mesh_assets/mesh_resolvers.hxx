#pragma once
#include <asset_system/asset_resolver.hxx>

namespace iceshard
{

    class AssimpMeshResolver final : public asset::AssetResolver
    {
    public:
        AssimpMeshResolver() noexcept = default;

        bool resolve_asset_info(
            core::StringView extension,
            resource::ResourceMetaView const& meta,
            asset::AssetStatus& status_out,
            asset::AssetType& type_out
        ) noexcept override;
    };

} // namespace iceshard
