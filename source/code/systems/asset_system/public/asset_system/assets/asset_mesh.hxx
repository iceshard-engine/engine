#pragma once
#include <core/pointer.hxx>
#include <asset_system/asset_loader.hxx>
#include <asset_system/asset_resolver.hxx>


namespace asset
{

    struct AssetMesh : public Asset
    {
        constexpr AssetMesh() noexcept = default;

        constexpr AssetMesh(core::StringView view) noexcept
            : Asset{ view, AssetType::Mesh }
        {
        }
    };

    auto default_resolver_mesh(core::allocator& alloc) noexcept -> core::memory::unique_pointer<asset::AssetResolver>;

    auto default_loader_mesh(core::allocator& alloc) noexcept -> core::memory::unique_pointer<asset::AssetLoader>;

} // namespace
