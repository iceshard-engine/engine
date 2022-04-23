#pragma once
#include <ice/asset_storage.hxx>
#include <ice/asset_type_archive.hxx>
#include <ice/resource_tracker.hxx>
#include <ice/resource_meta.hxx>
#include <ice/resource.hxx>
#include <ice/pod/hash.hxx>
#include <ice/stack_string.hxx>
#include <ice/uri.hxx>

#include "asset_entry.hxx"
#include "asset_shelve.hxx"
#include "asset_request_awaitable.hxx"

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
            ice::Utf8String name
        ) noexcept -> ice::ResourceHandle*
        {
            ice::StackString<64, ice::c8utf> temp_name{ name };

            ice::ResourceHandle* resource = nullptr;

            ice::u32 ext_idx = 0;
            ice::u32 const ext_count = ice::size(definition.resource_extensions);
            ice::u32 const temp_name_len = ice::size(temp_name);
            while (resource == nullptr && ext_idx < ext_count)
            {
                ice::Utf8String const extension = definition.resource_extensions[ext_idx++];
                ice::string::resize(temp_name, temp_name_len);
                ice::string::push_back(temp_name, extension);

                ice::URI const uri{ scheme_urn, temp_name };
                resource = resource_tracker.find_resource(uri, ice::ResourceFlags::None);
            }

            return resource;
        }

        bool bake_asset(
            ice::Allocator& alloc,
            ice::AssetTypeDefinition const& definition,
            ice::ResourceTracker& resource_tracker,
            ice::AssetEntry const* asset_entry,
            ice::Memory& result
        ) noexcept
        {
            if (definition.fn_asset_oven.is_set())
            {
                return definition.fn_asset_oven(alloc, resource_tracker, *asset_entry->resource, asset_entry->data, result);
            }
            return false;
        }

        bool load_asset(
            ice::Allocator& alloc,
            ice::AssetTypeDefinition const& definition,
            ice::AssetStorage& asset_storage,
            ice::Metadata const& asset_metadata,
            ice::Data baked_data,
            ice::Memory& result
        ) noexcept
        {
            if (definition.fn_asset_loader.is_set())
            {
                return definition.fn_asset_loader(alloc, asset_storage, asset_metadata, baked_data, result);
            }
            return false;
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
            ice::Utf8String name,
            ice::AssetState requested_state
        ) noexcept -> ice::Task<ice::Asset> override;

        auto aquire_request(
            ice::AssetType type,
            ice::AssetState requested_state
        ) noexcept -> ice::AssetRequest*
        {
            ice::AssetRequest* result = nullptr;
            ice::AssetShelve* shelve = ice::pod::hash::get(_asset_shevles, type.identifier, nullptr);
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
        ice::pod::Hash<ice::AssetShelve*> _asset_shevles;
    };

    DefaultAssetStorage::DefaultAssetStorage(
        ice::Allocator& alloc,
        ice::ResourceTracker& resource_tracker,
        ice::UniquePtr<ice::AssetTypeArchive> asset_archive
    ) noexcept
        : _allocator{ alloc }
        , _resource_tracker{ resource_tracker }
        , _asset_archive{ ice::move(asset_archive) }
        , _asset_shevles{ _allocator }
    {
        ice::Span<ice::AssetType const> types = _asset_archive->asset_types();
        ice::pod::hash::reserve(_asset_shevles, static_cast<ice::u32>(ice::size(types) / ice::pod::detail::hash::Constant_MaxLoadFactor) + 1);

        for (ice::AssetType_Arg type : types)
        {
            ice::pod::hash::set(
                _asset_shevles,
                type.identifier,
                _allocator.make<AssetShelve>(
                    _allocator,
                    _asset_archive->find_definition(type)
                )
            );
        }
    }

    DefaultAssetStorage::~DefaultAssetStorage() noexcept
    {
        for (auto const& entry : _asset_shevles)
        {
            _allocator.destroy(entry.value);
        }
    }

    auto DefaultAssetStorage::request(
        ice::AssetType type,
        ice::Utf8String name,
        ice::AssetState requested_state
    ) noexcept -> ice::Task<ice::Asset>
    {
        ice::StringID const nameid = ice::stringid(name);
        ice::Asset result{ };

        ice::AssetShelve* shelve = ice::pod::hash::get(_asset_shevles, type.identifier, nullptr);
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

            ice::ResourceResult const load_result = co_await _resource_tracker.load_resource(resource);
            if (load_result.resource_status == ResourceStatus::Loaded)
            {
                ice::AssetState const state = shelve->definition.fn_asset_state(
                    shelve->definition,
                    load_result.resource->metadata(),
                    load_result.resource->uri()
                );

                if (asset_entry == nullptr)
                {
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

                    asset_entry->state = state;
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
                    result.data = asset_entry->data_baked;

                    // If it was not baked, then it's pre-baked.
                    if (result.data.location == nullptr)
                    {
                        result.data = asset_entry->data;
                    }
                    break;
                }
                case AssetState::Loaded:
                {
                    result.data = asset_entry->data_loaded;
                    break;
                }
                case AssetState::Runtime:
                {
                    result.data = asset_entry->data_runtime;
                    break;
                }
                }

                result.handle = result.data.location != nullptr ? asset_entry : nullptr;
            }
            else
            {
                ice::Data result_data;

                if (asset_entry->state == AssetState::Raw)
                {
                    ice::Memory baked_memory;
                    bool const bake_success = ice::detail::bake_asset(
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

                    asset_alloc.deallocate(asset_entry->data_baked.location);
                    asset_entry->data_baked = baked_memory;
                    asset_entry->state = AssetState::Baked;

                    result_data = asset_entry->data_baked;
                }

                if (requested_state != AssetState::Baked && asset_entry->state == AssetState::Baked)
                {
                    ice::Memory loaded_memory;
                    bool const load_success = ice::detail::load_asset(
                        asset_alloc,
                        shelve->definition,
                        *this,
                        asset_entry->resource->metadata(),
                        asset_entry->data_baked,
                        loaded_memory
                    );

                    if (load_success == false)
                    {
                        loaded_memory = co_await AssetRequestAwaitable{ nameid, *shelve, asset_entry, AssetState::Loaded };
                    }

                    asset_alloc.deallocate(asset_entry->data_loaded.location);
                    asset_entry->data_loaded = loaded_memory;
                    asset_entry->state = AssetState::Loaded;

                    result_data = asset_entry->data_loaded;
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
                            result_data = runtime_data;
                        }
                    }
                    else
                    {
                        result_data = asset_entry->data_runtime;
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
        return ice::make_unique<ice::AssetStorage, ice::DefaultAssetStorage>(
            alloc,
            alloc,
            resource_tracker,
            ice::move(asset_archive)
        );
    }

} // namespace ice
