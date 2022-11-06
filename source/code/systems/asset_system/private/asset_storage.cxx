/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/asset_storage.hxx>
#include <ice/asset_type_archive.hxx>
#include <ice/resource_tracker.hxx>
#include <ice/resource_meta.hxx>
#include <ice/resource.hxx>
#include <ice/container/hashmap.hxx>
#include <ice/string/static_string.hxx>
#include <ice/uri.hxx>

#include "asset_entry.hxx"
#include "asset_shelve.hxx"
#include "asset_request_awaitable.hxx"

#include <mutex>

namespace ice
{
    namespace detail
    {

        constexpr bool asset_state_covered(ice::AssetState current, ice::AssetState expected) noexcept
        {
            ice::u32 const current_val = static_cast<ice::u32>(current);
            ice::u32 const expected_val = static_cast<ice::u32>(expected);
            ice::u32 const baked_val = static_cast<ice::u32>(AssetState::Baked);

            return current_val == expected_val
                // If we are expecting at least "baked", we can be accessed no matter any higher state
                || (expected_val >= baked_val && current_val > expected_val);
        }

        auto find_resource(
            ice::AssetTypeDefinition const& definition,
            ice::ResourceTracker& resource_tracker,
            ice::String name
        ) noexcept -> ice::ResourceHandle*
        {
            ice::StaticString<64> temp_name{ name };

            ice::ResourceHandle* resource = nullptr;

            ice::u32 ext_idx = 0;
            ice::u32 const ext_count = ice::count(definition.resource_extensions);
            ice::u32 const temp_name_len = ice::size(temp_name);
            while (resource == nullptr && ext_idx < ext_count)
            {
                ice::String const extension = definition.resource_extensions[ext_idx++];
                ice::string::resize(temp_name, temp_name_len);
                ice::string::push_back(temp_name, extension);

                ice::URI const uri{ Scheme_URN, temp_name };
                resource = resource_tracker.find_resource(uri, ice::ResourceFlags::None);
            }

            return resource;
        }

        auto bake_asset(
            ice::Allocator& alloc,
            ice::AssetTypeDefinition const& definition,
            ice::ResourceTracker& resource_tracker,
            ice::AssetEntry const* asset_entry,
            ice::Memory& result
        ) noexcept -> ice::Task<bool>
        {
            if (definition.fn_asset_oven)
            {
                ice::LooseResource const* loose_resource = ice::get_loose_resource(asset_entry->resource_handle);
                ICE_ASSERT(loose_resource != nullptr, "Baking non-loose resources should never happen!");

                co_return co_await definition.fn_asset_oven(definition.ud_asset_oven, alloc, resource_tracker, *loose_resource, asset_entry->data, result);
            }
            co_return false;
        }

        auto load_asset(
            ice::Allocator& alloc,
            ice::AssetTypeDefinition const& definition,
            ice::AssetStorage& asset_storage,
            ice::Metadata const& asset_metadata,
            ice::Data baked_data,
            ice::Memory& result
        ) noexcept -> ice::Task<bool>
        {
            if (definition.fn_asset_loader)
            {
                co_return co_await definition.fn_asset_loader(definition.ud_asset_loader, alloc, asset_storage, asset_metadata, baked_data, result);
            }
            co_return false;
        }

    } // namespace detail


    class DefaultAssetStorage final : public ice::AssetStorage
    {
    public:
        DefaultAssetStorage(
            ice::Allocator& alloc,
            ice::ResourceTracker& resource_tracker,
            ice::UniquePtr<ice::AssetTypeArchive> asset_archive
        ) noexcept;

        ~DefaultAssetStorage() noexcept override;

        auto request(
            ice::AssetType type,
            ice::String name,
            ice::AssetState requested_state
        ) noexcept -> ice::Task<ice::Asset> override;

        auto aquire_request(
            ice::AssetType type,
            ice::AssetState requested_state
        ) noexcept -> ice::AssetRequest* override
        {
            ice::AssetRequest* result = nullptr;
            ice::AssetShelve* shelve = ice::hashmap::get(_asset_shelves, type.identifier, nullptr);
            if (shelve != nullptr)
            {
                result = shelve->aquire_request(requested_state);
            }
            return result;
        }

        auto release(
            ice::Asset asset
        ) noexcept -> ice::Task<> override;

    private:
        ice::Allocator& _allocator;
        ice::ResourceTracker& _resource_tracker;
        ice::UniquePtr<ice::AssetTypeArchive> _asset_archive;
        ice::HashMap<ice::AssetShelve*> _asset_shelves;
    };

    DefaultAssetStorage::DefaultAssetStorage(
        ice::Allocator& alloc,
        ice::ResourceTracker& resource_tracker,
        ice::UniquePtr<ice::AssetTypeArchive> asset_archive
    ) noexcept
        : _allocator{ alloc }
        , _resource_tracker{ resource_tracker }
        , _asset_archive{ ice::move(asset_archive) }
        , _asset_shelves{ _allocator }
    {
        ice::Span<ice::AssetType const> types = _asset_archive->asset_types();
        ice::hashmap::reserve(_asset_shelves, ice::count(types));

        for (ice::AssetType_Arg type : types)
        {
            ice::hashmap::set(
                _asset_shelves,
                type.identifier,
                _allocator.create<AssetShelve>(
                    _allocator,
                    _asset_archive->find_definition(type)
                )
            );
        }
    }

    DefaultAssetStorage::~DefaultAssetStorage() noexcept
    {
        for (ice::AssetShelve* entry : _asset_shelves)
        {
            _allocator.destroy(entry);
        }
    }

    auto DefaultAssetStorage::request(
        ice::AssetType type,
        ice::String name,
        ice::AssetState requested_state
    ) noexcept -> ice::Task<ice::Asset>
    {
        ice::StringID const nameid = ice::stringid(name);
        ice::Asset result{ };

        ice::AssetShelve* shelve = ice::hashmap::get(_asset_shelves, type.identifier, nullptr);
        if (shelve != nullptr)
        {
            ice::Allocator& asset_alloc = shelve->asset_allocator();
            ice::AssetEntry* asset_entry = shelve->select(nameid);
            ice::ResourceHandle* resource = nullptr;

            if (asset_entry != nullptr)
            {
                resource = asset_entry->resource_handle;
            }
            else
            {
                resource = ice::detail::find_resource(
                    shelve->definition,
                    _resource_tracker,
                    name
                );
            }

            // TODO: Because of this call we needed to introduce a std::mutex for now, but we will redesign this in the next free sprint.
            //    We can end here on two different threads (the Resource thread or the calling thread) [gh#135]
            ice::ResourceResult const load_result = co_await _resource_tracker.load_resource(resource);
            if (load_result.resource_status == ResourceStatus::Loaded)
            {
                ice::AssetState const state = shelve->definition.fn_asset_state(
                    shelve->definition.ud_asset_state,
                    shelve->definition,
                    load_result.resource->metadata(),
                    load_result.resource->uri()
                );

                if (asset_entry == nullptr)
                {
                    // TODO: We needed to introduce a lock here so we properly select + store asset entry objects here. [gh#135]
                    static std::mutex mtx{ };
                    std::lock_guard lk{ mtx };

                    asset_entry = shelve->select(nameid);
                    if (asset_entry == nullptr)
                    {
                        asset_entry = shelve->store(
                            nameid,
                            resource,
                            load_result.resource,
                            state,
                            load_result.data
                        );
                    }

                    // asset_entry->state = state;
                }
                else if (asset_entry->state == AssetState::Unknown)
                {
                    asset_entry->data = load_result.data;
                    asset_entry->state = state;
                }
            }

            // Failed to acquire asset entry
            if (asset_entry == nullptr)
            {
                co_return result;
            }

            ICE_ASSERT(
                asset_entry != nullptr,
                "Invalid asset storage state! Missing asset entry for:",
                /*type.name, name*/
            );

            if (static_cast<ice::u32>(requested_state) <= static_cast<ice::u32>(asset_entry->state))
            {
                switch (requested_state)
                {
                case AssetState::Raw:
                {
                    // Either it's raw or we where required to bake it.
                    if (asset_entry->state == AssetState::Raw || asset_entry->data_baked.location == nullptr)
                    {
                        result.data = asset_entry->data;
                    }
                    break;
                }
                case AssetState::Baked:
                {
                    result.data = ice::data_view(asset_entry->data_baked);

                    // If it was not baked, then it's pre-baked.
                    if (result.data.location == nullptr)
                    {
                        result.data = asset_entry->data;
                    }
                    break;
                }
                case AssetState::Loaded:
                {
                    result.data = ice::data_view(asset_entry->data_loaded);
                    break;
                }
                case AssetState::Runtime:
                {
                    result.data = ice::data_view(asset_entry->data_runtime);
                    break;
                }
                }

                result.handle = result.data.location != nullptr ? asset_entry : nullptr;
            }
            else
            {
                ice::Data result_data{ };

                // TODO: The below objects where required to be introduced due to rare data races on various calling / resource threads.
                static std::recursive_mutex mtx_asset_logic{ }; // [gh#135]

                if (asset_entry->state == AssetState::Raw)
                {
                    while (!mtx_asset_logic.try_lock()) {} // [gh#135]

                    if (asset_entry->state == AssetState::Raw)
                    {
                        ice::Memory baked_memory;
                        bool const bake_success = co_await ice::detail::bake_asset(
                            asset_alloc,
                            shelve->definition,
                            _resource_tracker,
                            asset_entry,
                            baked_memory
                        );

                        if (bake_success == false)
                        {
                            baked_memory = co_await AssetRequestAwaitable{ nameid, *shelve, asset_entry, AssetState::Baked };
                        }

                        asset_alloc.deallocate(asset_entry->data_baked);
                        asset_entry->data_baked = baked_memory;
                        asset_entry->state = AssetState::Baked;

                        result_data = ice::data_view(asset_entry->data_baked);
                    }

                    mtx_asset_logic.unlock(); // [gh#135]
                }

                if (requested_state != AssetState::Baked && asset_entry->state == AssetState::Baked)
                {
                    while (!mtx_asset_logic.try_lock()) {} // [gh#135]
                    if (asset_entry->state == AssetState::Baked)
                    {
                        ice::Memory loaded_memory;
                        bool const load_success = co_await ice::detail::load_asset(
                            asset_alloc,
                            shelve->definition,
                            *this,
                            asset_entry->resource->metadata(),
                            ice::data_view(asset_entry->data_baked),
                            loaded_memory
                        );

                        if (load_success == false)
                        {
                            loaded_memory = co_await AssetRequestAwaitable{ nameid, *shelve, asset_entry, AssetState::Loaded };
                        }

                        asset_alloc.deallocate(asset_entry->data_loaded);
                        asset_entry->data_loaded = loaded_memory;
                        asset_entry->state = AssetState::Loaded;

                        result_data = ice::data_view(asset_entry->data_loaded);
                    }

                    mtx_asset_logic.unlock(); // [gh#135]
                }

                if (requested_state == AssetState::Runtime)
                {
                    if (asset_entry->state != AssetState::Runtime)
                    {
                        ice::Memory runtime_data = co_await AssetRequestAwaitable{ nameid, *shelve, asset_entry, AssetState::Runtime };
                        if (runtime_data.location != nullptr)
                        {
                            asset_entry->state = AssetState::Runtime;
                            asset_entry->data_runtime = runtime_data;
                            result_data = ice::data_view(runtime_data);
                        }
                    }
                    else
                    {
                        result_data = ice::data_view(asset_entry->data_runtime);
                    }
                }

                if (requested_state == asset_entry->state)
                {
                    result.data = result_data;
                    result.handle = result_data.location != nullptr ? asset_entry : nullptr;
                }
            }
        }

        co_return result;
    }

    auto DefaultAssetStorage::release(
        ice::Asset asset
    ) noexcept -> ice::Task<>
    {
        ICE_ASSERT(asset.handle != nullptr, "Invalid asset object! No valid handle found!");
        ice::AssetEntry* entry = static_cast<ice::AssetEntry*>(asset.handle);

        ice::ResourceResult const result = co_await _resource_tracker.unload_resource(entry->resource_handle);
        if ((result.resource_status & ice::ResourceStatus::Available) == ice::ResourceStatus::Available)
        {
            entry->data = result.data;
            entry->state = AssetState::Unknown;
        }
    }

    auto create_asset_storage(
        ice::Allocator& alloc,
        ice::ResourceTracker& resource_tracker,
        ice::UniquePtr<ice::AssetTypeArchive> asset_archive
    ) noexcept -> ice::UniquePtr<ice::AssetStorage>
    {
        return ice::make_unique<ice::DefaultAssetStorage>(
            alloc,
            alloc,
            resource_tracker,
            ice::move(asset_archive)
        );
    }

} // namespace ice
