#include <ice/asset.hxx>
#include <ice/resource.hxx>

#include "asset_entry.hxx"

namespace ice
{

    auto Asset::metadata() const noexcept -> ice::Metadata const&
    {
        if (handle == nullptr)
        {
            static ice::Metadata empty_metadata{ };
            return empty_metadata;
        }

        return reinterpret_cast<ice::AssetEntry const*>(handle)->resource->metadata();
    }

} // namespace ice
