/// Copyright 2022 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "asset_storage.hxx"
#include "asset_entry.hxx"
#include "asset_shelve.hxx"
#include "asset_shelve_devui.hxx"
#include "asset_request_awaitable.hxx"
#include "asset_transaction.hxx"

namespace ice
{
    namespace detail
    {

        template<ice::AssetState State>
        inline auto filter_states(ice::TaskAwaitableParams params, void*) noexcept
        {
            ICE_ASSERT(params.modifier == TaskAwaitableModifier::CustomValue, "Unexpected modifier type!");
            ice::AssetState const awaiting_state = static_cast<AssetState>(params.u32_value);
            return awaiting_state <= State;
        }

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
            ice::AssetCategoryDefinition const& definition,
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
            ice::ResourceCompiler const& compiler,
            ice::ResourceTracker& resource_tracker,
            ice::AssetStateTransaction& transaction
        ) noexcept -> ice::Task<bool>
        {
            ice::Array<ice::ResourceHandle*> sources{ alloc };
            ice::Array<ice::URI> dependencies{ alloc }; // Not used currently

            ice::ResourceHandle* const resource = transaction.asset.resource_handle;
            ice::ResourceCompilerCtx& ctx = transaction.asset._shelve->compiler_context;

            // Early return if sources or dependencies couldn't be collected
            if (compiler.fn_collect_sources(ctx, resource, resource_tracker, sources) == false
                || compiler.fn_collect_dependencies(ctx, resource, resource_tracker, dependencies) == false)
            {
                co_return false;
            }

            // If empty we add our own handle to the list
            if (ice::array::empty(sources))
            {
                ice::array::push_back(sources, resource);
            }

            ice::Array<ice::Task<>> tasks{ alloc };
            ice::array::reserve(tasks, ice::array::count(sources));

            static auto fn_validate = [&ctx](
                ice::ResourceCompiler const& compiler,
                ice::ResourceHandle* source,
                ice::ResourceTracker& tracker,
                std::atomic_bool& out_result
            ) noexcept -> ice::Task<>
            {
                // If failed update the result.
                if (co_await compiler.fn_validate_source(ctx, source, tracker) == false)
                {
                    out_result = false;
                }
            };

            std::atomic_bool all_sources_valid = true;
            for (ice::ResourceHandle* source : sources)
            {
                ice::array::push_back(tasks, fn_validate(compiler, source, resource_tracker, all_sources_valid));
            }

            co_await ice::await_tasks(tasks);

            ice::array::clear(tasks);

            // Validation failed
            if (all_sources_valid == false)
            {
                co_return false;
            }

            ice::Array<ice::ResourceCompilerResult> compiled_sources{ alloc };
            ice::array::resize(compiled_sources, ice::array::count(sources));

            static auto fn_compile = [&ctx](
                ice::ResourceCompiler const& compiler,
                ice::ResourceHandle* source,
                ice::ResourceTracker& tracker,
                ice::Span<ice::ResourceHandle* const> sources,
                ice::Span<ice::URI const> dependencies,
                ice::Allocator& result_alloc,
                ice::ResourceCompilerResult& out_result
            ) noexcept -> ice::Task<>
            {
                out_result = co_await compiler.fn_compile_source(
                    ctx,
                    source,
                    tracker,
                    sources,
                    dependencies,
                    result_alloc
                );
            };

            // Create all compilation tasks...
            ice::u32 source_idx = 0;
            for (ice::ResourceHandle* source : sources)
            {
                ice::array::push_back(
                    tasks,
                    fn_compile(
                        compiler,
                        source,
                        resource_tracker,
                        sources,
                        dependencies,
                        alloc,
                        compiled_sources[source_idx]
                    )
                );
                source_idx += 1;
            }

            // ... and wait for them to finish
            co_await ice::await_tasks(tasks);

            // Build the metadata
            ice::MutableMetadata meta{ alloc };
            if (ice::wait_for_result(compiler.fn_build_metadata(ctx, resource, resource_tracker, compiled_sources, dependencies, meta)))
            {
                // Finalize the asset
                transaction.set_result_data(
                    alloc,
                    meta,
                    compiler.fn_finalize(ctx, resource, compiled_sources, dependencies, alloc)
                );
            }

            // Deallocate all compiled sources
            for (ice::ResourceCompilerResult const& compiled : compiled_sources)
            {
                alloc.deallocate(compiled.result);
            }

            // We are good if we have data of positive size
            co_return transaction.asset._data != nullptr;
        }

        auto load_asset(
            ice::Allocator& alloc,
            ice::AssetCategoryDefinition const& definition,
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

    auto create_asset_storage_devui(
        ice::Allocator& alloc,
        ice::DefaultAssetStorage& storage,
        ice::Array<ice::UniquePtr<ice::AssetShelve::DevUI>> shelves
    ) noexcept -> ice::UniquePtr<ice::DevUIWidget>;

    DefaultAssetStorage::DefaultAssetStorage(
        ice::Allocator& alloc,
        ice::UniquePtr<ice::AssetCategoryArchive> asset_archive,
        ice::AssetStorageCreateInfo const& create_info
    ) noexcept
        : _allocator{ alloc }
        , _info{ create_info }
        , _asset_archive{ ice::move(asset_archive) }
        , _asset_shelves{ _allocator }
        , _devui_widget{ }
    {
        ice::Span<ice::AssetCategory const> categories = _asset_archive->categories();
        ice::hashmap::reserve(_asset_shelves, ice::count(categories));

        ice::Array<ice::UniquePtr<ice::AssetShelve::DevUI>> shelves{ _allocator };
        for (ice::AssetCategory_Arg category : categories)
        {
            ice::AssetShelve* const shelve = _allocator.create<AssetShelve>(
                _allocator,
                *this,
                _asset_archive->find_definition(category),
                _asset_archive->find_compiler(category)
            );
            ice::hashmap::set(_asset_shelves, category.identifier, shelve);

            if constexpr (ice::build::is_debug || ice::build::is_develop)
            {
                ice::array::push_back(shelves, ice::make_unique<AssetShelve::DevUI>(_allocator, *shelve));
            }
        }

        _devui_widget = create_asset_storage_devui(_allocator, *this, ice::move(shelves));
    }

    DefaultAssetStorage::~DefaultAssetStorage() noexcept
    {
        _devui_widget.reset();

        for (ice::AssetShelve* entry : _asset_shelves)
        {
            _allocator.destroy(entry);
        }
    }

    auto DefaultAssetStorage::bind(
        ice::AssetCategory_Arg category,
        ice::String name
    ) noexcept -> ice::Asset
    {
        Asset result{ };
        // CHECK: Always called from the same thread.
        // OR TODO: Think of how to make it MT + lock-free

        ice::AssetShelve* shelve = nullptr;
        ice::AssetEntry* entry = nullptr;
        if (find_shelve_and_entry(category, name, shelve, entry))
        {
            result._handle = entry;

            // We need to ensure the previous count was higher else the asset might be released already
            ice::u32 const prev_count = entry->refcount.fetch_add(1, std::memory_order_relaxed);
            if (prev_count == 0)
            {
                // Only keep metadata data
                while (entry->_data != nullptr)
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

                ice::u32 const prev_count = entry->refcount.fetch_add(1, std::memory_order_relaxed);
                ICE_ASSERT(prev_count == 0, "Unexpected value!");

                result._handle = entry;
            }
        }
        return result;
    }

    auto DefaultAssetStorage::request(
        ice::Asset const& asset,
        ice::AssetState requested_state
    ) noexcept -> ice::Task<ice::Data>
    {
        ICE_ASSERT(asset._handle != nullptr, "Invalid asset object");
        ice::AssetEntry& entry = *static_cast<ice::AssetEntry*>(asset._handle);

        // This object might be unused (it's only alive for the request duration)
        ice::AssetStateTrackers trackers{};
        ice::AssetStateTrackers* trackersptrs = nullptr;
        if (entry._request_trackers.compare_exchange_strong(trackersptrs, ice::addressof(trackers), std::memory_order_relaxed))
        {
            trackersptrs = ice::addressof(trackers);
        }

        ice::Expected<ice::Data, ice::ErrorCode> result{ };
        ice::AssetShelve& shelve = *entry._shelve;
        ice::AssetStateTransaction transaction{ requested_state, _allocator, entry, *trackersptrs };
        switch (transaction.target_state)
        {
        case AssetState::Raw:
            result = co_await this->request_asset_raw(transaction, shelve);
            break;
        case AssetState::Baked:
            result = co_await this->request_asset_baked(transaction, shelve);
            break;
        case AssetState::Loaded:
            result = co_await this->request_asset_loaded(transaction, shelve);
            break;
        case AssetState::Runtime:
            result = co_await this->request_asset_runtime(transaction, shelve);
            break;
        }

        // Release the trackers pointers
        if (entry._request_trackers.load(std::memory_order_relaxed) == trackersptrs)
        {
            entry._request_trackers.exchange(nullptr, std::memory_order_relaxed);
        }

        if (result.valid() == false)
        {
            ICE_LOG(LogSeverity::Error, LogTag::Asset, "Requesting asset data failed with {}", result.error());
            co_return {};
        }
        co_return result.value();
    }

    auto DefaultAssetStorage::request_asset_runtime(
        ice::AssetStateTransaction& transaction,
        ice::AssetShelve& shelve
    ) noexcept -> ice::TaskExpected<ice::Data, ice::ErrorCode>
    {
        ice::Expected<ice::Data, ice::ErrorCode> result =
            co_await ice::AssetStateRequest{ transaction, AssetState::Runtime };

        // If the awaiting index was '0' we are responsible for loading and calling all awaiting tasks
        // - This also already passed the refcount check, meaning the data is not there or outdated.
        if (result == E_RequestEvaluationNeeded)
        {
            // We request a baked resource first
            co_await request_asset_loaded(transaction, shelve);

            ice::Memory runtime_data = co_await AssetRequestAwaitable{
                StringID_Invalid, shelve, transaction
            };

            // ICE_ASSERT_CORE(runtime_data.location == transaction.result_data->readwrite);

            result = ice::data_view(runtime_data);
            // transaction.set_result_data(shelve.asset_allocator(), AssetState::Runtime, runtime_data);

            co_await ice::await_filtered_queue_on(transaction.trackers.tasks_queue, _info.task_scheduler, detail::filter_states<AssetState::Runtime>);
        }

        co_return result;
    }

    auto DefaultAssetStorage::request_asset_loaded(
        ice::AssetStateTransaction& transaction,
        ice::AssetShelve& shelve
    ) noexcept -> ice::TaskExpected<ice::Data, ice::ErrorCode>
    {
        ice::Expected<ice::Data, ice::ErrorCode> result =
            co_await ice::AssetStateRequest{ transaction, AssetState::Loaded };

        // If the awaiting index was '0' we are responsible for loading and calling all awaiting tasks
        // - This also already passed the refcount check, meaning the data is not there or outdated.
        if (result == E_RequestEvaluationNeeded)
        {
            // We request a baked resource first
            ice::Data baked_result = co_await request_asset_baked(transaction, shelve);

            ice::AssetState const current_state = transaction.asset.state();
            if (current_state == AssetState::Baked)
            {
                ice::Memory load_result{};
                ice::Metadata const metadata = ice::meta_load(
                    ice::asset_data_find(transaction.asset._metadata, AssetState::Baked)
                );
                bool const load_success = co_await ice::detail::load_asset(
                    shelve.asset_allocator(),
                    shelve.definition,
                    *this,
                    metadata,
                    baked_result,
                    load_result
                );

                if (load_success == false)
                {
                    co_return E_RequestEvaluationFailed;
                }

                result = ice::data_view(load_result);
                transaction.set_result_data(shelve.asset_allocator(), AssetState::Loaded, load_result);
            }

            co_await ice::await_filtered_queue_on(transaction.trackers.tasks_queue, _info.task_scheduler, detail::filter_states<AssetState::Loaded>);
        }

        co_return result;
    }

    auto DefaultAssetStorage::request_asset_baked(
        ice::AssetStateTransaction& transaction,
        ice::AssetShelve& shelve
    ) noexcept -> ice::TaskExpected<ice::Data, ice::ErrorCode>
    {
        ice::Expected<ice::Data, ice::ErrorCode> result =
            co_await ice::AssetStateRequest{ transaction, AssetState::Baked };

        // If the awaiting index was '0' we are responsible for loading and calling all awaiting tasks
        // - This also already passed the refcount check, meaning the data is not there or outdated.

        if (result == E_RequestEvaluationNeeded)
        {
            // We request a raw resource first
            ice::Data raw_data = co_await request_asset_raw(transaction, shelve);

            ice::AssetState const current_state = transaction.asset.state();
            if (current_state == AssetState::Raw)
            {
                ICE_ASSERT(raw_data.location != nullptr, "We failed to load any data from resource handle!");

                if (shelve.compiler != nullptr)
                {
                    bool const task_success = co_await ice::detail::bake_asset(
                        shelve.asset_allocator(),
                        *shelve.compiler,
                        _info.resource_tracker,
                        transaction
                    );
                    if (task_success == false)
                    {
                        co_return E_RequestEvaluationFailed;
                    }

                    result = ice::asset_data_find(transaction.asset._data, AssetState::Baked);
                }
            }

            co_await ice::await_filtered_queue_on(transaction.trackers.tasks_queue, _info.task_scheduler, detail::filter_states<AssetState::Baked>);
        }

        co_return result;
    }

    auto DefaultAssetStorage::request_asset_raw(
        ice::AssetStateTransaction& transaction,
        ice::AssetShelve const& shelve
    ) noexcept -> ice::TaskExpected<ice::Data, ice::ErrorCode>
    {
        ice::Expected<ice::Data, ice::ErrorCode> result =
            co_await ice::AssetStateRequest{ transaction, AssetState::Raw };

        // If the awaiting index was '0' we are responsible for loading and calling all awaiting tasks
        // - This also already passed the refcount check, meaning the data is not there or outdated.
        if (result == E_RequestEvaluationNeeded)
        {
            //IPT_MESSAGE("Loading 'Raw' state");
            //ICE_ASSERT(entry.resource_state == AssetState::Unknown, "Unexpected asset state!");

            ice::ResourceResult const resource = co_await _info.resource_tracker.load_resource(transaction.asset.resource_handle);
            ICE_ASSERT(
                resource.resource_status == ResourceStatus::Loaded,
                "Failed to load resource!"
            );

            ice::AssetState const current_state = transaction.asset.state();
            if (current_state == AssetState::Exists)
            {
                ice::Data metadata{};
                ice::MutableMetadata res_metadata{ _allocator };
                if (co_await ice::resource_meta(transaction.asset.resource_handle, metadata); metadata.location != nullptr)
                {
                    ice::Result const res = ice::meta_deserialize_from(res_metadata, metadata);
                    ICE_ASSERT_CORE(res == true);
                }

                result = resource.data;
                transaction.set_result_data(res_metadata, resource.data);
            }

            // TODO: Resource could be in 'Baked' so processing for 'Raw' should be avoided here.
            co_await ice::await_filtered_queue_on(transaction.trackers.tasks_queue, _info.task_scheduler, detail::filter_states<AssetState::Raw>);
        }

        // We call await resume manually as it will again properly select the value we want
        co_return result;
    }

    auto DefaultAssetStorage::aquire_request(
        ice::AssetCategory_Arg category,
        ice::AssetState requested_state
    ) noexcept -> ice::AssetRequest*
    {
        ice::AssetRequest* result = nullptr;
        ice::AssetShelve* shelve = ice::hashmap::get(_asset_shelves, category.identifier, nullptr);
        if (shelve != nullptr)
        {
            result = shelve->aquire_request(requested_state);
        }
        return result;
    }

    auto DefaultAssetStorage::release(
        ice::Asset const& asset
    ) noexcept -> ice::Task<>
    {
        ICE_ASSERT(asset._handle != nullptr, "Invalid asset object! No valid handle found!");
        ice::AssetEntry* entry = static_cast<ice::AssetEntry*>(asset._handle);

        // We where the last ones to keep a reference.
        if (entry->refcount.fetch_sub(1, std::memory_order_relaxed) == 1)
        {
            entry->_data.reset();

            [[maybe_unused]]
            ice::ResourceResult const result = co_await _info.resource_tracker.unload_resource(entry->resource_handle);
            //if (ice::has_all(result.resource_status, ice::ResourceStatus::Available))
            //{
            //    entry->data = result.data;
            //}
        }
    }

    bool DefaultAssetStorage::find_shelve_and_entry(
        ice::AssetCategory_Arg category,
        ice::String name,
        ice::AssetShelve*& shelve,
        ice::AssetEntry*& entry
    ) noexcept
    {
        ice::StringID const nameid = ice::stringid(name);

        shelve = ice::hashmap::get(_asset_shelves, category.identifier, nullptr);
        if (shelve != nullptr)
        {
            entry = shelve->select(nameid);
        }
        return shelve != nullptr && entry != nullptr;
    }

    auto create_asset_storage(
        ice::Allocator& alloc,
        ice::UniquePtr<ice::AssetCategoryArchive> asset_category_archive,
        ice::AssetStorageCreateInfo const& create_info
    ) noexcept -> ice::UniquePtr<ice::AssetStorage>
    {
        return ice::make_unique<ice::DefaultAssetStorage>(
            alloc,
            alloc,
            ice::move(asset_category_archive),
            create_info
        );
    }

} // namespace ice
