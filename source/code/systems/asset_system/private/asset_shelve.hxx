/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/mem_allocator.hxx>
#include <ice/asset_category_archive.hxx>
#include <ice/devui_widget.hxx>
#include <atomic>

#include "asset_entry.hxx"

namespace ice
{

    class AssetShelve final
    {
    public:
        AssetShelve(
            ice::Allocator& alloc,
            ice::DefaultAssetStorage& storage,
            ice::AssetCategoryDefinition const& definition,
            ice::ResourceCompiler const* compiler
        ) noexcept;

        ~AssetShelve() noexcept;

        auto asset_allocator() noexcept -> ice::Allocator&;

        auto select(
            ice::StringID_Arg name
        ) noexcept -> ice::AssetEntry*;

        auto select(
            ice::StringID_Arg name
        ) const noexcept -> ice::AssetEntry const*;

        auto store(
            ice::StringID_Arg name,
            ice::ResourceHandle const& resource_handle
        ) noexcept -> ice::AssetEntry*;

        void append_request(
            ice::AssetRequestAwaitable* request,
            ice::AssetState state
        ) noexcept;

        auto aquire_request(
            ice::AssetState state
        ) noexcept -> ice::AssetRequestAwaitable*;

        ice::DefaultAssetStorage& storage;
        ice::AssetCategoryDefinition const& definition;
        ice::ResourceCompiler const* compiler;

        class DevUI;

    private:
        ice::Allocator& _allocator;
        ice::HashMap<ice::AssetEntry*> _asset_resources;

        std::atomic<ice::AssetRequestAwaitable*> _new_requests[3];
        std::atomic<ice::AssetRequestAwaitable*> _reversed_requests[3];
    };

} // namespace ice
