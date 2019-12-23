#pragma once
#include <core/allocator.hxx>
#include <core/pointer.hxx>
#include <core/cexpr/stringid.hxx>
#include <core/pod/hash.hxx>

#include <asset_system/asset.hxx>
#include <resource/resource_system.hxx>

namespace asset
{

    class AssetLoader;

    //! \brief This class manages data and metadata associations across resources.
    class AssetSystem
    {
    public:
        AssetSystem(core::allocator& alloc, resource::ResourceSystem& resource_system) noexcept;
        ~AssetSystem() noexcept = default;

        auto request(Asset reference) noexcept -> AssetStatus;

        auto update(Asset reference, resource::URI content_location) noexcept -> AssetStatus;

        //! \brief Loads the given asset from the resource system.
        auto load(Asset reference, AssetData& data) noexcept -> AssetStatus;

        //! \brief Updates the asset system database.
        void update() noexcept;

    private:
        core::allocator& _allocator;

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
