#include "mesh_resolvers.hxx"

namespace iceshard
{

    bool AssimpMeshResolver::resolve_asset_info(
        core::StringView extension,
        resource::ResourceMetaView const& meta,
        asset::AssetStatus& status_out,
        asset::AssetType& type_out
    ) noexcept
    {
        if (core::string::equals(extension, ".fbx")
            || core::string::equals(extension, ".x3d")
            || core::string::equals(extension, ".obj")
            || core::string::equals(extension, ".dae"))
        {
            type_out = asset::AssetType::Mesh;
            status_out = asset::AssetStatus::Available_Raw;
            return true;
        }
        return false;
    }

} // namespace iceshard
