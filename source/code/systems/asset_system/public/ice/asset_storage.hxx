/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/asset.hxx>
#include <ice/asset_type.hxx>
#include <ice/asset_types.hxx>
#include <ice/mem_unique_ptr.hxx>

namespace ice
{

    struct AssetStorageCreateInfo
    {
        //! \brief Resource tracker associated with this asset storage.
        ice::ResourceTracker& resource_tracker;

        //! \brief Task scheduler used to push work on other threads.
        ice::TaskScheduler& task_scheduler;

        //! \brief Flags used to push work on task threads.
        ice::TaskFlags task_flags;
    };

    class AssetStorage
    {
    public:
        virtual ~AssetStorage() = default;

        virtual auto bind(
            ice::AssetType type,
            ice::String name
        ) noexcept -> ice::Asset = 0;

        virtual auto request(
            ice::Asset const& entry,
            ice::AssetState requested_state
        ) noexcept -> ice::Task<ice::Data> = 0;

        virtual auto aquire_request(
            ice::AssetType type,
            ice::AssetState requested_state
        ) noexcept -> ice::AssetRequest* = 0;

        virtual auto release(
            ice::Asset const& asset
        ) noexcept -> ice::Task<> = 0;
    };

    auto create_asset_storage(
        ice::Allocator& alloc,
        ice::UniquePtr<ice::AssetTypeArchive> asset_type_archive,
        ice::AssetStorageCreateInfo const& create_info
    ) noexcept -> ice::UniquePtr<ice::AssetStorage>;

} // namespace ice
