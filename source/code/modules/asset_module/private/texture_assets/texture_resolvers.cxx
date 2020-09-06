#include "texture_resolvers.hxx"

namespace iceshard
{

    auto StbTextureResolver::resolve_asset_type(
        core::StringView extension,
        resource::ResourceMetaView const& meta
    ) noexcept -> asset::AssetType
    {
        if (core::string::equals(extension, ".jpg")
            || core::string::equals(extension, ".jpeg")
            || core::string::equals(extension, ".png")
            || core::string::equals(extension, ".bmp"))
        {
            return asset::AssetType::Texture;
        }
        return asset::AssetType::Unresolved;
    }

} // namespace iceshard
