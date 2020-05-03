#include "mesh_resolvers.hxx"

namespace iceshard
{

    auto AssimpMeshResolver::resolve_asset_type(
        core::StringView extension,
        resource::ResourceMetaView const& meta
    ) noexcept -> asset::AssetType
    {
        if (core::string::equals(extension, ".fbx")
            || core::string::equals(extension, ".x3d")
            || core::string::equals(extension, ".obj")
            || core::string::equals(extension, ".dae"))
        {
            return asset::AssetType::Mesh;
        }
        return asset::AssetType::Unresolved;
    }

} // namespace iceshard
