#pragma once
#include <ice/asset_oven.hxx>

namespace ice
{

    class ResourceSystem;

    class IceshardMeshOven final : public ice::AssetOven
    {
    public:
        auto bake(
            ice::ResourceHandle& resource,
            ice::ResourceTracker_v2& resource_tracker,
            ice::Allocator& asset_alloc,
            ice::Memory& asset_data
        ) const noexcept -> ice::BakeResult override;
    };

} // namespace iceshard
