#pragma once
#include <core/pointer.hxx>
#include <asset_system/asset_loader.hxx>
#include <asset_system/asset_resolver.hxx>


namespace asset
{

    namespace v1
    {

        struct Mesh
        {
            uint32_t vertice_count = 0;
            uint32_t indice_count = 0;

            float* vertice_data = nullptr; // each vertice consists of 6 values (3 pos, 3 color)
            uint16_t* indice_data = nullptr;
        };

    } // namespace v1

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
