/// Copyright 2022 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/asset_storage.hxx>
#include <ice/asset_type_archive.hxx>
#include <ice/resource_tracker.hxx>
#include <ice/resource_meta.hxx>
#include <ice/resource.hxx>
#include <ice/container/hashmap.hxx>
#include <ice/string/static_string.hxx>
#include <ice/devui_widget.hxx>
#include <ice/task_utils.hxx>
#include <ice/profiler.hxx>
#include <ice/uri.hxx>

#include "asset_entry.hxx"

namespace ice
{

    class DefaultAssetStorage final : public ice::AssetStorage
    {
    public:
        DefaultAssetStorage(
            ice::Allocator& alloc,
            ice::UniquePtr<ice::AssetTypeArchive> asset_archive,
            ice::AssetStorageCreateInfo const& create_info
        ) noexcept;

        ~DefaultAssetStorage() noexcept override;

        auto bind(
            ice::AssetType type,
            ice::String name
        ) noexcept -> ice::Asset override;

        auto preload(
            ice::AssetType type,
            ice::String name,
            ice::AssetState state
        ) noexcept -> ice::Task<> override;

        auto request(
            ice::Asset const& asset,
            ice::AssetState requested_state
        ) noexcept -> ice::Task<ice::Data> override;

        auto request_asset_runtime(
            ice::AssetEntry& entry,
            ice::AssetShelve& shelve
        ) noexcept -> ice::Task<ice::Data>;

        auto request_asset_loaded(
            ice::AssetEntry& entry,
            ice::AssetShelve& shelve
        ) noexcept -> ice::Task<ice::Data>;

        auto request_asset_baked(
            ice::AssetEntry& entry,
            ice::AssetShelve& shelve
        ) noexcept -> ice::Task<ice::Data>;

        auto request_asset_raw(
            ice::AssetEntry& entry,
            ice::AssetShelve const& shelve
        ) noexcept -> ice::Task<ice::Data>;

        auto aquire_request(
            ice::AssetType type,
            ice::AssetState requested_state
        ) noexcept -> ice::AssetRequest* override;

        auto release(
            ice::Asset const& asset
        ) noexcept -> ice::Task<> override;

        class DevUI;

    protected:
        bool find_shelve_and_entry(
            ice::AssetType type,
            ice::String name,
            ice::AssetShelve*& shelve,
            ice::AssetEntry*& handle
        ) noexcept;

    private:
        ice::Allocator& _allocator;
        ice::AssetStorageCreateInfo _info;
        ice::UniquePtr<ice::AssetTypeArchive> _asset_archive;
        ice::HashMap<ice::AssetShelve*> _asset_shelves;

        ice::UniquePtr<ice::DevUIWidget> _devui_widget;
    };

} // namespace ice
