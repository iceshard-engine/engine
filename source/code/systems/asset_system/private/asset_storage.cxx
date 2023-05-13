/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/asset_storage.hxx>
#include <ice/asset_type_archive.hxx>
#include <ice/resource_tracker.hxx>
#include <ice/resource_meta.hxx>
#include <ice/resource.hxx>
#include <ice/container/hashmap.hxx>
#include <ice/string/static_string.hxx>
#include <ice/profiler.hxx>
#include <ice/uri.hxx>

#include "asset_entry.hxx"
#include "asset_shelve.hxx"
#include "asset_request_awaitable.hxx"

#include <mutex>

namespace ice
{
    namespace detail
    {

        struct ProcessAwaitingTasks
        {
            ice::AssetState const current_state;
            ice::TaskQueue& awaiting_queue;
            std::atomic<ice::u8>& awaiting_count;

            void process() noexcept
            {
                ice::TaskAwaitableBase* last_remaining = nullptr;
                ice::AtomicLinkedQueue<ice::TaskAwaitableBase> remaining_tasks;

                // Process all awaiting tasks (load the raw_awaiting, every time, as it might got decreased)
                ice::u32 processed = 1; // We start with '1' which is "US"
                while (processed < awaiting_count.load(std::memory_order_relaxed))
                {
                    // Get all awaiting tasks
                    // TODO: maybe a pop here would be better?
                    for (ice::TaskAwaitableBase* awaitable : ice::linked_queue::consume(awaiting_queue._awaitables))
                    {
                        ICE_ASSERT(awaitable->_params.modifier == TaskAwaitableModifier_v3::CustomValue, "Unexpected modifier type!");
                        ice::AssetState const awaiting_state = static_cast<AssetState>(awaitable->_params.u32_value);
                        if (awaiting_state <= current_state)
                        {
                            awaitable->_coro.resume();
                            processed += 1;
                        }
                        else
                        {
                            last_remaining = awaitable;

                            // Pushing awaitiable onto remaining task does not change the 'next' pointer so we don't invalidate this range.
                            ice::linked_queue::push(remaining_tasks, awaitable);
                        }
                    }
                };

                ICE_ASSERT(
                    processed == awaiting_count.load(std::memory_order_relaxed),
                    "Processed too many tasks?!"
                );

                // Reset the 'next' ptr as it might contain old values, this also means we need to push remaining tasks back to the asset entry queue.
                // We are safe to do so here because another task loading a "next" asset representation will wait for all awaiting tasks to be pushed.
                if (last_remaining != nullptr)
                {
                    last_remaining->next = nullptr;

                    for (ice::TaskAwaitableBase* awaitable : ice::linked_queue::consume(remaining_tasks))
                    {
                        ice::linked_queue::push(awaiting_queue._awaitables, awaitable);
                    }

                    // We don't need to finx any 'next' pointer here fortunately
                }

                ice::u32 const expected_awaiting_count = awaiting_count.exchange(0, std::memory_order_relaxed);
                ICE_ASSERT(expected_awaiting_count == processed, "Got additional awaiting tasks!");
            }
        };

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
            ice::StaticString<128> temp_name{ name };
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
            ice::UniquePtr<ice::AssetTypeArchive> asset_archive,
            ice::AssetStorageCreateInfo const& create_info
        ) noexcept;

        ~DefaultAssetStorage() noexcept override;

        auto bind(
            ice::AssetType type,
            ice::String name
        ) noexcept -> ice::Asset
        {
            Asset result{ };
            // CHECK: Always called from the same thread.
            // OR TODO: Think of how to make it MT + lock-free

            ice::AssetShelve* shelve = nullptr;
            ice::AssetEntry* entry = nullptr;
            if (find_shelve_and_entry(type, name, shelve, entry))
            {
                result._handle = entry;

                // We need to ensure the previous count was higher else the asset might be released already
                ice::u32 const prev_count = entry->refcount.fetch_add(1, std::memory_order_relaxed);
                if (prev_count == 0)
                {
                    while (entry->current_state != AssetState::Unknown)
                    {
                        // TODO: Add an sleep statement?
                        //IPT_MESSAGE_C("Asset waiting for 'Release' operation to finish.", 0xEE8866);
                    }
                }
            }
            else if (shelve != nullptr)
            {
                ice::ResourceHandle* const resource_handle = ice::detail::find_resource(
                    shelve->definition,
                    _info.resource_tracker,
                    name
                );

                if (resource_handle)
                {
                    // Create a new asset entry if the handle exists
                    entry = shelve->store(
                        ice::stringid(name),
                        resource_handle
                    );

                    // TODO: Move to the 'store' method as argument.
                    entry->storage = this;

                    ice::u32 const prev_count = entry->refcount.fetch_add(1, std::memory_order_relaxed);
                    ICE_ASSERT(prev_count == 0, "Unexpected value!");

                    result._handle = entry;
                }
            }
            return result;
        }

        auto request(
            ice::Asset const& asset,
            ice::AssetState requested_state
        ) noexcept -> ice::Task<ice::Data> override
        {
            ICE_ASSERT(asset._handle != nullptr, "Invalid asset object");

            ice::AssetEntry& entry = *static_cast<ice::AssetEntry*>(asset._handle);
            ICE_ASSERT(entry.storage == this, "Invalid asset provided for this storage!");

            ice::AssetShelve& shelve = *entry.shelve;

            ice::Data result{ };
            switch (requested_state)
            {
            case AssetState::Raw:
                result = co_await request_asset_raw(entry, shelve);
                break;
            case AssetState::Baked:
                result = co_await request_asset_baked(entry, shelve);
                break;
            case AssetState::Loaded:
                result = co_await request_asset_loaded(entry, shelve);
                break;
            case AssetState::Runtime:
                result = co_await request_asset_runtime(entry, shelve);
                break;
            }
            co_return result;
        }

        auto request_asset_runtime(
            ice::AssetEntry& entry,
            ice::AssetShelve& shelve
        ) noexcept -> ice::Task<ice::Data>
        {
            ice::AssetStateRequest request{ entry, AssetState::Runtime };
            ice::Data result = co_await request;

            // If the awaiting index was '0' we are responsible for loading and calling all awaiting tasks
            // - This also already passed the refcount check, meaning the data is not there or outdated.
            if (request.awaiting_index == 0)
            {
                // We request a baked resource first
                result = co_await request_asset_loaded(entry, shelve);
                ICE_ASSERT(result.location != nullptr, "We failed to access baked data!");

                ice::Memory runtime_data = co_await AssetRequestAwaitable{ StringID_Invalid, shelve, &entry, AssetState::Runtime };
                if (runtime_data.location != nullptr)
                {
                    shelve.asset_allocator().deallocate(entry.data_runtime);

                    entry.data_runtime = runtime_data;
                    entry.current_state = AssetState::Runtime;
                    result = ice::data_view(entry.data_runtime);
                }

                // Process all awaiting tasks
                // TODO: We keep the if once we fix the 'always success' assumption
                if (entry.current_state == AssetState::Runtime)
                {
                    // And the resource is already in baked format, set the result and call awaiting tasks
                    ice::detail::ProcessAwaitingTasks awaiting{ AssetState::Runtime, entry.queue, entry.runtime_awaiting };
                    awaiting.process();
                }
            }

            co_return result;
        }

        auto request_asset_loaded(
            ice::AssetEntry& entry,
            ice::AssetShelve& shelve
        ) noexcept -> ice::Task<ice::Data>
        {
            ice::AssetStateRequest request{ entry, AssetState::Loaded };
            ice::Data result = co_await request;

            // If the awaiting index was '0' we are responsible for loading and calling all awaiting tasks
            // - This also already passed the refcount check, meaning the data is not there or outdated.
            if (request.awaiting_index == 0)
            {
                // We request a baked resource first
                result = co_await request_asset_baked(entry, shelve);
                ICE_ASSERT(result.location != nullptr, "We failed to access baked data!");

                ice::Memory load_result;
                bool const load_success = co_await ice::detail::load_asset(
                    shelve.asset_allocator(),
                    shelve.definition,
                    *this,
                    entry.resource->metadata(),
                    result,
                    load_result
                );

                // If we have failed, we try to push a load request.
                // TODO: Needs to be reworked!
                if (load_success == false)
                {
                    // WE ASSUME SUCCESS HERE?!
                    load_result = co_await AssetRequestAwaitable{ ice::StringID_Invalid, shelve, &entry, AssetState::Loaded };
                }

                shelve.asset_allocator().deallocate(entry.data_loaded);
                entry.data_loaded = load_result;
                entry.current_state = AssetState::Loaded;
                result = ice::data_view(entry.data_loaded);

                // Process all awaiting tasks
                // TODO: We keep the if once we fix the 'always success' assumption
                if (entry.current_state == AssetState::Loaded)
                {
                    // And the resource is already in baked format, set the result and call awaiting tasks
                    ice::detail::ProcessAwaitingTasks awaiting{ AssetState::Loaded, entry.queue, entry.load_awaiting };
                    awaiting.process();
                }
            }

            co_return result;
        }

        auto request_asset_baked(
            ice::AssetEntry& entry,
            ice::AssetShelve& shelve
        ) noexcept -> ice::Task<ice::Data>
        {
            ice::AssetStateRequest request{ entry, AssetState::Baked };
            ice::Data result = co_await request;

            // If the awaiting index was '0' we are responsible for loading and calling all awaiting tasks
            // - This also already passed the refcount check, meaning the data is not there or outdated.
            if (request.awaiting_index == 0)
            {
                // We request a raw resource first
                result = co_await request_asset_raw(entry, shelve);

                // But we might already have it loaded as 'baked'...
                if (result.location == nullptr && entry.current_state == AssetState::Baked)
                {
                    // We are baked already so just grab the data
                    result = entry.data;
                }
                else
                {
                    ICE_ASSERT(result.location != nullptr, "We failed to load any data from resource handle!");

                    ice::Memory result_memory;
                    bool const bake_success = co_await ice::detail::bake_asset(
                        shelve.asset_allocator(),
                        shelve.definition,
                        _info.resource_tracker,
                        &entry,
                        result_memory
                    );

                    if (bake_success)
                    {
                        // Release previous memory if existing
                        shelve.asset_allocator().deallocate(entry.data_baked);

                        ICE_ASSERT(entry.current_state < AssetState::Baked, "Unexpected asset state!");
                        entry.data_baked = result_memory;
                        entry.current_state = AssetState::Baked;
                        result = ice::data_view(entry.data_baked);
                    }
                }

                // Process all awaiting tasks
                if (entry.current_state == AssetState::Baked)
                {
                    // And the resource is already in baked format, set the result and call awaiting tasks
                    ice::detail::ProcessAwaitingTasks awaiting{ AssetState::Baked, entry.queue, entry.bake_awaiting };
                    awaiting.process();
                }
            }

            co_return result;
        }

        auto request_asset_raw(
            ice::AssetEntry& entry,
            ice::AssetShelve const& shelve
        ) noexcept -> ice::Task<ice::Data>
        {
            //IPT_MESSAGE(ice::resource_origin(entry.resource_handle));

            ice::AssetStateRequest request{ entry, AssetState::Raw };
            co_await request;

            // If the awaiting index was '0' we are responsible for loading and calling all awaiting tasks
            // - This also already passed the refcount check, meaning the data is not there or outdated.
            if (request.awaiting_index == 0)
            {
                //IPT_MESSAGE("Loading 'Raw' state");
                //ICE_ASSERT(entry.resource_state == AssetState::Unknown, "Unexpected asset state!");

                ice::ResourceResult const resource = co_await _info.resource_tracker.load_resource(entry.resource_handle);
                ICE_ASSERT(
                    resource.resource_status == ResourceStatus::Loaded,
                    "Failed to load resource!"
                );

                // Set the resource data
                entry.data = resource.data;

                if (entry.resource_state == AssetState::Unknown)
                {
                    //IPT_MESSAGE("Requesting resource");

                    entry.resource = resource.resource;
                    ice::AssetState const initial_state = shelve.definition.fn_asset_state(
                        shelve.definition.ud_asset_state,
                        shelve.definition,
                        entry.resource->metadata(),
                        entry.resource->uri()
                    );

                    // Save the data from the loaded resource tracker
                    entry.data = resource.data;
                    entry.resource_state = initial_state;
                    entry.current_state = initial_state;
                }
                else
                {
                    //IPT_MESSAGE("Re-using resource");

                    // Re-assing the 'current' state if the 'resource' state is known
                    entry.current_state = entry.resource_state;
                }

                ice::detail::ProcessAwaitingTasks awaiting{ AssetState::Raw, entry.queue, entry.raw_awaiting };
                awaiting.process();
            }

            // We call await resume manually as it will again properly select the value we want
            co_return request.await_resume();
        }

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
            ice::Asset const& asset
        ) noexcept -> ice::Task<> override;

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
    };

    DefaultAssetStorage::DefaultAssetStorage(
        ice::Allocator& alloc,
        ice::UniquePtr<ice::AssetTypeArchive> asset_archive,
        ice::AssetStorageCreateInfo const& create_info
    ) noexcept
        : _allocator{ alloc }
        , _info{ create_info }
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

    //auto DefaultAssetStorage::request(
    //    ice::AssetType type,
    //    ice::String name,
    //    ice::AssetState requested_state
    //) noexcept -> ice::Task<ice::Asset>
    //{
    //    ice::StringID const nameid = ice::stringid(name);
    //    ice::Asset result{ };

    //    ice::AssetShelve* shelve = ice::hashmap::get(_asset_shelves, type.identifier, nullptr);
    //    if (shelve != nullptr)
    //    {
    //        ice::Allocator& asset_alloc = shelve->asset_allocator();
    //        ice::AssetEntry* asset_entry = shelve->select(nameid);
    //        ice::ResourceHandle* resource = nullptr;

    //        if (asset_entry != nullptr)
    //        {
    //            resource = asset_entry->resource_handle;
    //        }
    //        else
    //        {
    //            resource = ice::detail::find_resource(
    //                shelve->definition,
    //                _info.resource_tracker,
    //                name
    //            );
    //        }

    //        // TODO: Because of this call we needed to introduce a std::mutex for now, but we will redesign this in the next free sprint.
    //        //    We can end here on two different threads (the Resource thread or the calling thread) [gh#135]
    //        ice::ResourceResult const load_result = co_await _info.resource_tracker.load_resource(resource);
    //        if (load_result.resource_status == ResourceStatus::Loaded)
    //        {
    //            ice::AssetState const state = shelve->definition.fn_asset_state(
    //                shelve->definition.ud_asset_state,
    //                shelve->definition,
    //                load_result.resource->metadata(),
    //                load_result.resource->uri()
    //            );

    //            if (asset_entry == nullptr)
    //            {
    //                // TODO: We needed to introduce a lock here so we properly select + store asset entry objects here. [gh#135]
    //                static std::mutex mtx{ };
    //                std::lock_guard lk{ mtx };

    //                asset_entry = shelve->select(nameid);
    //                if (asset_entry == nullptr)
    //                {
    //                    asset_entry = shelve->store(
    //                        nameid,
    //                        resource,
    //                        load_result.resource,
    //                        state,
    //                        load_result.data
    //                    );
    //                }

    //                // asset_entry->state = state;
    //            }
    //            else if (asset_entry->state == AssetState::Unknown)
    //            {
    //                asset_entry->data = load_result.data;
    //                asset_entry->state = state;
    //            }
    //        }

    //        // Failed to acquire asset entry
    //        if (asset_entry == nullptr)
    //        {
    //            co_return result;
    //        }

    //        ICE_ASSERT(
    //            asset_entry != nullptr,
    //            "Invalid asset storage state! Missing asset entry for:",
    //            /*type.name, name*/
    //        );

    //        if (static_cast<ice::u32>(requested_state) <= static_cast<ice::u32>(asset_entry->state))
    //        {
    //            switch (requested_state)
    //            {
    //            case AssetState::Raw:
    //            {
    //                // Either it's raw or we where required to bake it.
    //                if (asset_entry->state == AssetState::Raw || asset_entry->data_baked.location == nullptr)
    //                {
    //                    result.data = asset_entry->data;
    //                }
    //                break;
    //            }
    //            case AssetState::Baked:
    //            {
    //                result.data = ice::data_view(asset_entry->data_baked);

    //                // If it was not baked, then it's pre-baked.
    //                if (result.data.location == nullptr)
    //                {
    //                    result.data = asset_entry->data;
    //                }
    //                break;
    //            }
    //            case AssetState::Loaded:
    //            {
    //                result.data = ice::data_view(asset_entry->data_loaded);
    //                break;
    //            }
    //            case AssetState::Runtime:
    //            {
    //                result.data = ice::data_view(asset_entry->data_runtime);
    //                break;
    //            }
    //            }

    //            result.handle = result.data.location != nullptr ? asset_entry : nullptr;
    //        }
    //        else
    //        {
    //            ice::Data result_data{ };

    //            // TODO: The below objects where required to be introduced due to rare data races on various calling / resource threads.
    //            static std::recursive_mutex mtx_asset_logic{ }; // [gh#135]

    //            if (asset_entry->state == AssetState::Raw)
    //            {
    //                while (!mtx_asset_logic.try_lock()) {} // [gh#135]

    //                if (asset_entry->state == AssetState::Raw)
    //                {
    //                    ice::Memory baked_memory;
    //                    bool const bake_success = co_await ice::detail::bake_asset(
    //                        asset_alloc,
    //                        shelve->definition,
    //                        _info.resource_tracker,
    //                        asset_entry,
    //                        baked_memory
    //                    );

    //                    if (bake_success == false)
    //                    {
    //                        baked_memory = co_await AssetRequestAwaitable{ nameid, *shelve, asset_entry, AssetState::Baked };
    //                    }

    //                    asset_alloc.deallocate(asset_entry->data_baked);
    //                    asset_entry->data_baked = baked_memory;
    //                    asset_entry->state = AssetState::Baked;

    //                    result_data = ice::data_view(asset_entry->data_baked);
    //                }

    //                mtx_asset_logic.unlock(); // [gh#135]
    //            }

    //            if (requested_state != AssetState::Baked && asset_entry->state == AssetState::Baked)
    //            {
    //                while (!mtx_asset_logic.try_lock()) {} // [gh#135]
    //                if (asset_entry->state == AssetState::Baked)
    //                {
    //                    ice::Memory loaded_memory;
    //                    bool const load_success = co_await ice::detail::load_asset(
    //                        asset_alloc,
    //                        shelve->definition,
    //                        *this,
    //                        asset_entry->resource->metadata(),
    //                        ice::data_view(asset_entry->data_baked),
    //                        loaded_memory
    //                    );

    //                    if (load_success == false)
    //                    {
    //                        loaded_memory = co_await AssetRequestAwaitable{ nameid, *shelve, asset_entry, AssetState::Loaded };
    //                    }

    //                    asset_alloc.deallocate(asset_entry->data_loaded);
    //                    asset_entry->data_loaded = loaded_memory;
    //                    asset_entry->state = AssetState::Loaded;

    //                    result_data = ice::data_view(asset_entry->data_loaded);
    //                }

    //                mtx_asset_logic.unlock(); // [gh#135]
    //            }

    //            if (requested_state == AssetState::Runtime)
    //            {
    //                if (asset_entry->state != AssetState::Runtime)
    //                {
    //                    ice::Memory runtime_data = co_await AssetRequestAwaitable{ nameid, *shelve, asset_entry, AssetState::Runtime };
    //                    if (runtime_data.location != nullptr)
    //                    {
    //                        asset_entry->state = AssetState::Runtime;
    //                        asset_entry->data_runtime = runtime_data;
    //                        result_data = ice::data_view(runtime_data);
    //                    }
    //                }
    //                else
    //                {
    //                    result_data = ice::data_view(asset_entry->data_runtime);
    //                }
    //            }

    //            if (requested_state == asset_entry->state)
    //            {
    //                result.data = result_data;
    //                result.handle = result_data.location != nullptr ? asset_entry : nullptr;
    //            }
    //        }
    //    }

    //    co_return result;
    //}

    auto DefaultAssetStorage::release(
        ice::Asset const& asset
    ) noexcept -> ice::Task<>
    {
        ICE_ASSERT(asset._handle != nullptr, "Invalid asset object! No valid handle found!");
        ice::AssetEntry* entry = static_cast<ice::AssetEntry*>(asset._handle);
        ICE_ASSERT(entry->storage == this, "Invalid asset provided for this storage!");

        // We where the last ones to keep a reference.
        if (entry->refcount.fetch_sub(1, std::memory_order_relaxed) == 1)
        {
            entry->current_state = AssetState::Unknown;

            ice::ResourceResult const result = co_await _info.resource_tracker.unload_resource(entry->resource_handle);
            //if (ice::has_all(result.resource_status, ice::ResourceStatus::Available))
            //{
            //    entry->data = result.data;
            //}
        }
    }

    bool DefaultAssetStorage::find_shelve_and_entry(
        ice::AssetType type,
        ice::String name,
        ice::AssetShelve*& shelve,
        ice::AssetEntry*& entry
    ) noexcept
    {
        ice::StringID const nameid = ice::stringid(name);

        shelve = ice::hashmap::get(_asset_shelves, type.identifier, nullptr);
        if (shelve != nullptr)
        {
            entry = shelve->select(nameid);
        }
        return shelve != nullptr && entry != nullptr;
    }

    //auto create_asset_storage(
    //    ice::Allocator& alloc,
    //    ice::ResourceTracker& resource_tracker,
    //    ice::UniquePtr<ice::AssetTypeArchive> asset_archive
    //) noexcept -> ice::UniquePtr<ice::AssetStorage>
    //{
    //    return ice::make_unique<ice::DefaultAssetStorage>(
    //        alloc,
    //        alloc,
    //        resource_tracker,
    //        ice::move(asset_archive)
    //    );
    //}

    auto create_asset_storage(
        ice::Allocator& alloc,
        ice::UniquePtr<ice::AssetTypeArchive> asset_type_archive,
        ice::AssetStorageCreateInfo const& create_info
    ) noexcept -> ice::UniquePtr<ice::AssetStorage>
    {
        return ice::make_unique<ice::DefaultAssetStorage>(
            alloc,
            alloc,
            ice::move(asset_type_archive),
            create_info
        );
    }

} // namespace ice
