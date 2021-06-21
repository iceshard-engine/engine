#pragma once
#include <ice/asset_oven.hxx>

namespace ice
{

    class ResourceSystem;

    class IceshardMeshOven final : public ice::AssetOven
    {
    public:
        auto bake(
            ice::Data resource_data,
            ice::Metadata const& resource_meta,
            ice::ResourceSystem& resource_system,
            ice::Allocator& asset_alloc,
            ice::Memory& asset_data
        ) noexcept -> ice::BakeResult override;
    };

} // namespace iceshard
