#pragma once
#include <core/allocator.hxx>
#include <core/pointer.hxx>
#include <core/cexpr/stringid.hxx>
#include <core/pod/hash.hxx>
#include <core/collections.hxx>

#include <asset_system/asset.hxx>
#include <asset_system/asset_loader.hxx>
#include <asset_system/asset_resolver.hxx>
#include <asset_system/asset_compiler.hxx>
#include <resource/resource_system.hxx>

#include <unordered_map>
#include <vector>

namespace asset
{

    enum class AssetResolverHandle : uint32_t { };

    enum class AssetLoaderHandle : uint32_t { };

    enum class AssetCompilerHandle : uint32_t { };

    //! \brief This class manages data and metadata associations across resources.
    class AssetSystem
    {
    public:
        AssetSystem(core::allocator& alloc, resource::ResourceSystem& resource_system) noexcept;
        ~AssetSystem() noexcept = default;

        auto add_resolver(
            core::memory::unique_pointer<asset::AssetResolver> resolver
        ) noexcept -> asset::AssetResolverHandle;

        void remove_resolver(
            asset::AssetResolverHandle resolver_handle
        ) noexcept;

        auto add_loader(
            asset::AssetType asset_type,
            core::memory::unique_pointer<asset::AssetLoader> loader
        ) noexcept -> asset::AssetLoaderHandle;

        void remove_loader(
            asset::AssetLoaderHandle loader_handle
        ) noexcept;

        auto request(Asset reference) noexcept -> AssetStatus;

        auto update(Asset reference, resource::URI content_location) noexcept -> AssetStatus;

        //! \brief Loads the given asset from the resource system.
        auto load(Asset reference, AssetData& data) noexcept -> AssetStatus;

        //! \brief Updates the asset system database.
        void update() noexcept;

    private:
        core::allocator& _allocator;

        uint32_t _next_resolver_handle = 0;
        uint32_t _next_loader_handle = 0;

        core::Map<AssetResolverHandle, core::memory::unique_pointer<AssetResolver>> _asset_resolvers;
        core::Map<AssetLoaderHandle, core::memory::unique_pointer<AssetLoader>> _asset_loaders;
        core::Map<AssetCompilerHandle, core::memory::unique_pointer<AssetCompiler>> _asset_compilers;

        core::Map<AssetType, std::vector<AssetCompiler*>> _asset_compiler_map;
        core::Map<AssetType, std::vector<AssetLoader*>> _asset_loader_map;

        struct AssetReference
        {
            resource::URI content_location = resource::uri_invalid;
            resource::Resource* resource_object;
            AssetStatus status;
            AssetCompilationResult* compiled_asset;
        };
        core::pod::Array<AssetReference> _resource_database;

        struct AssetObject
        {
            uint32_t resource_index;
            Asset reference;
        };
        core::pod::Hash<AssetObject> _asset_objects;

        resource::ResourceSystem& _resource_system;
    };

} // namespace asset
