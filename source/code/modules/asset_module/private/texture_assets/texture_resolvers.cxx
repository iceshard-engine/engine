#include "texture_resolvers.hxx"

namespace iceshard
{

    bool StbTextureResolver::resolve_asset_info(
        core::StringView extension,
        resource::ResourceMetaView const& meta,
        asset::AssetStatus& status_out,
        asset::AssetType& type_out
    ) noexcept
    {
        if (core::string::equals(extension, ".jpg")
            || core::string::equals(extension, ".jpeg")
            || core::string::equals(extension, ".png")
            || core::string::equals(extension, ".bmp"))
        {
            type_out = asset::AssetType::Texture;
            status_out = asset::AssetStatus::Available_Raw;
            return true;
        }
        return false;
    }

} // namespace iceshard
