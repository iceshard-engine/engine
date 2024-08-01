/// Copyright 2022 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/asset_types.hxx>
#include <ice/asset_storage.hxx>
#include <ice/asset_category_archive.hxx>
#include <ice/resource_tracker.hxx>
#include <ice/resource_meta.hxx>
#include <ice/resource.hxx>
#include <ice/container/hashmap.hxx>
#include <ice/string/static_string.hxx>
#include <ice/devui_widget.hxx>
#include <ice/task_expected.hxx>
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
            ice::UniquePtr<ice::AssetCategoryArchive> asset_archive,
            ice::AssetStorageCreateInfo const& create_info
        ) noexcept;

        ~DefaultAssetStorage() noexcept override;

        auto bind(
            ice::AssetCategory_Arg category,
            ice::String name
        ) noexcept -> ice::Asset override;

        auto request(
            ice::Asset const& asset,
            ice::AssetState requested_state
        ) noexcept -> ice::Task<ice::Data> override;

        auto request_asset_runtime(
            ice::AssetStateTransaction& transaction,
            ice::AssetShelve& shelve
        ) noexcept -> ice::TaskExpected<ice::Data, ice::ErrorCode>;

        auto request_asset_loaded(
            ice::AssetStateTransaction& transaction,
            ice::AssetShelve& shelve
        ) noexcept -> ice::TaskExpected<ice::Data, ice::ErrorCode>;

        auto request_asset_baked(
            ice::AssetStateTransaction& transaction,
            ice::AssetShelve& shelve
        ) noexcept -> ice::TaskExpected<ice::Data, ice::ErrorCode>;

        auto request_asset_raw(
            ice::AssetStateTransaction& transaction,
            ice::AssetShelve const& shelve
        ) noexcept -> ice::TaskExpected<ice::Data, ice::ErrorCode>;

        auto aquire_request(
            ice::AssetCategory_Arg category,
            ice::AssetState requested_state
        ) noexcept -> ice::AssetRequest* override;

        auto release(
            ice::Asset const& asset
        ) noexcept -> ice::Task<> override;

        class DevUI;

    protected:
        bool find_shelve_and_entry(
            ice::AssetCategory_Arg category,
            ice::String name,
            ice::AssetShelve*& shelve,
            ice::AssetEntry*& handle
        ) noexcept;

    private:
        ice::Allocator& _allocator;
        ice::AssetStorageCreateInfo _info;
        ice::UniquePtr<ice::AssetCategoryArchive> _asset_archive;
        ice::HashMap<ice::AssetShelve*> _asset_shelves;

        ice::UniquePtr<ice::DevUIWidget> _devui_widget;
    };

} // namespace ice
