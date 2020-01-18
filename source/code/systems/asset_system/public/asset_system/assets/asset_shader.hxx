#pragma once
#include <core/pointer.hxx>
#include <asset_system/asset.hxx>
#include <asset_system/asset_loader.hxx>
#include <asset_system/asset_resolver.hxx>

namespace asset
{

    struct AssetShader : public Asset
    {
        constexpr AssetShader() noexcept = default;

        constexpr AssetShader(core::StringView view) noexcept
            : Asset{ view, AssetType::Shader }
        {
        }
    };

    auto default_resolver_shader(core::allocator& alloc) noexcept->core::memory::unique_pointer<asset::AssetResolver>;

    auto default_loader_shader(core::allocator& alloc) noexcept -> core::memory::unique_pointer<asset::AssetLoader>;

} // namespace
