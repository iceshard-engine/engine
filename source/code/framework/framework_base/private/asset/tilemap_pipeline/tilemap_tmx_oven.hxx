#pragma once
#include <ice/asset_oven.hxx>

namespace ice
{

    class IceTiledTmxAssetOven : public ice::AssetOven
    {
    public:
        auto bake(
            ice::ResourceHandle& resource,
            ice::ResourceTracker& resource_tracker,
            ice::AssetSystem& asset_system,
            ice::Allocator& asset_alloc,
            ice::Memory& asset_data
        ) const noexcept -> ice::BakeResult override;
    };

} // namespace ice
