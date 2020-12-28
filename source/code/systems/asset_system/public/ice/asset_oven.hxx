#pragma once
#include <ice/data.hxx>
#include <ice/resource_meta.hxx>

namespace ice
{

    enum class BakeResult : ice::u32
    {
        Success = 0x0,
        Failure_InvalidData,
        Failure_MissingDependencies,
    };

    class AssetOven
    {
    public:
        virtual ~AssetOven() noexcept = 0;

        virtual auto bake(
            ice::Data resource_data,
            ice::Metadata const& resource_meta,
            ice::ResourceSystem& resource_system,
            ice::Allocator& asset_alloc,
            ice::Memory& asset_data
        ) noexcept -> ice::BakeResult = 0;
    };

} // namespace ice
