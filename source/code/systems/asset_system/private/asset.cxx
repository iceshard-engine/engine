#include "asset_internal.hxx"
#include <ice/data.hxx>

namespace ice
{

    auto asset_status(ice::Asset asset) noexcept -> ice::AssetStatus
    {
        if (asset == Asset::Invalid)
        {
            return AssetStatus::Invalid;
        }

        detail::AssetObject* const object = reinterpret_cast<detail::AssetObject*>(asset);
        return object->status;
    }

    auto asset_data(ice::Asset asset, ice::Data& out_data) noexcept -> ice::AssetStatus
    {
        if (asset == Asset::Invalid)
        {
            return AssetStatus::Invalid;
        }

        detail::AssetObject const* const object = reinterpret_cast<detail::AssetObject*>(asset);
        if (object->status == AssetStatus::Loaded)
        {
            out_data = object->data;
        }
        return object->status;
    }

    auto asset_metadata(ice::Asset asset, ice::Data& out_data) noexcept -> ice::AssetStatus
    {
        if (asset == Asset::Invalid)
        {
            return AssetStatus::Invalid;
        }

        detail::AssetObject const* const object = reinterpret_cast<detail::AssetObject*>(asset);
        if (object->status != AssetStatus::Invalid)
        {
            out_data = object->metadata;
        }
        return object->status;
    }

} // namespace ice
