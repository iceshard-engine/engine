#pragma once
#include <ice/memory.hxx>
#include <ice/allocator.hxx>

namespace ice
{

    class ResourceHandle;
    class ResourceTracker_v2;

    enum class BakeResult : ice::u32
    {
        Success = 0x0,
        Skipped = 0x1,
        Failure_InvalidData,
        Failure_MissingDependencies,
    };

    class AssetSystem;

    class AssetOven
    {
    public:
        virtual ~AssetOven() noexcept = default;

        virtual auto bake(
            ice::ResourceHandle& resource,
            ice::ResourceTracker_v2& resource_tracker,
            ice::Allocator& asset_alloc,
            ice::Memory& asset_data
        ) const noexcept -> ice::BakeResult
        {
            return BakeResult::Skipped;
        }

        virtual auto bake(
            ice::ResourceHandle& resource,
            ice::ResourceTracker_v2& resource_tracker,
            ice::AssetSystem& asset_system,
            ice::Allocator& asset_alloc,
            ice::Memory& asset_data
        ) const noexcept -> ice::BakeResult
        {
            return bake(resource, resource_tracker, asset_alloc, asset_data);
        }
    };

} // namespace ice
