#pragma once
#include <core/allocator.hxx>
#include <core/pointer.hxx>
#include <core/cexpr/stringid.hxx>
#include <core/pod/hash.hxx>

#include <asset_system/asset.hxx>
#include <asset_system/asset_loader.hxx>
#include <asset_system/asset_resolver.hxx>
#include <resource/resource_system.hxx>

#include <unordered_map>
#include <vector>

namespace asset
{

    //! \brief This class manages data and metadata associations across resources.
    class AssetSystem
    {
    public:
        AssetSystem(core::allocator& alloc, resource::ResourceSystem& resource_system) noexcept;
        ~AssetSystem() noexcept = default;

        void add_resolver(core::memory::unique_pointer<asset::AssetResolver> resolver) noexcept;

        void add_loader(asset::AssetType asset_type, core::memory::unique_pointer<asset::AssetLoader> loader) noexcept;

        auto request(Asset reference) noexcept -> AssetStatus;

        auto update(Asset reference, resource::URI content_location) noexcept -> AssetStatus;

        //! \brief Loads the given asset from the resource system.
        auto load(Asset reference, AssetData& data) noexcept -> AssetStatus;

        //! \brief Updates the asset system database.
        void update() noexcept;

    private:
        core::allocator& _allocator;

        std::vector<core::memory::unique_pointer<AssetResolver>> _asset_resolver;
        std::unordered_map<AssetType, std::vector<core::memory::unique_pointer<AssetLoader>>> _asset_loader;

        struct AssetReference
        {
            resource::URI content_location{ resource::scheme_invalid, "" };
        };
        core::pod::Array<AssetReference> _resource_database;

        struct AssetObject
        {
            AssetStatus status;
            uint32_t resource;
            Asset reference;
        };
        core::pod::Hash<AssetObject> _asset_objects;

        resource::ResourceSystem& _resource_system;
    };

} // namespace asset
