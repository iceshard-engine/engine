#pragma once
#include <core/pointer.hxx>
#include <asset_system/asset_system.hxx>

namespace iceshard
{

    class AssetModule
    {
    public:
        virtual ~AssetModule() noexcept = default;
    };

    auto load_asset_module(
        core::allocator& alloc,
        core::StringView module_path,
        asset::AssetSystem& asset_system
    ) noexcept -> core::memory::unique_pointer<AssetModule>;

} // namespace iceshard
