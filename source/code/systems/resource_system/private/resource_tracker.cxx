/// Copyright 2022 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "resource_tracker.hxx"
#include "resource_internal.hxx"
#include <ice/resource_filter.hxx>

#include <ice/sync_manual_events.hxx>

namespace ice
{

    auto resource_uri(ice::ResourceHandle const& handle) noexcept -> ice::URI const&
    {
        return handle->uri();
    }

    auto resource_origin(ice::ResourceHandle const& handle) noexcept -> ice::String
    {
        return handle->origin();
    }

    auto resource_path(ice::ResourceHandle const& handle) noexcept -> ice::String
    {
        return handle->name();
    }

    auto resource_meta(ice::ResourceHandle const& handle, ice::Data& out_metadata) noexcept -> ice::Task<ice::Result>
    {
        out_metadata = co_await internal_provider(handle)->load_resource(handle, "meta");
        co_return out_metadata.location == nullptr;
    }

    // Might need to be moved somewhere else?
    auto get_loose_resource(ice::ResourceHandle const& handle) noexcept -> ice::LooseResource const*
    {
        return internal_provider(handle)->access_loose_resource(handle);
    }

    static_assert(std::atomic<std::coroutine_handle<>>::is_always_lock_free);

    ResourceTrackerImplementation::ResourceTrackerImplementation(
        ice::Allocator& alloc,
        ice::ResourceTrackerCreateInfo const& info
    ) noexcept
        : ResourceTracker{ }
        , _allocator{ alloc }
        , _allocator_data{ _allocator, "Data" }
        , _info{ info }
        , _resources{ _allocator }
        , _resource_providers{ _allocator }
        , _resource_writers{ _allocator }
        , _devui_widget{ }
    {
        ICE_ASSERT(
            _info.predicted_resource_count > 0,
            "Invalid value ({}) provided for 'predicted_resource_count'. Value needs to be a positive integer."
        );

        ice::hashmap::reserve(_resources, _info.predicted_resource_count);
        ice::hashmap::reserve(_resource_providers, 12);
        ice::hashmap::reserve(_resource_writers, 4);
    }

    ResourceTrackerImplementation::~ResourceTrackerImplementation() noexcept
    {
        ice::hashmap::clear(_resources);
    }

    auto ResourceTrackerImplementation::attach_provider(
        ice::UniquePtr<ice::ResourceProvider> provider
    ) noexcept -> ice::ResourceProvider*
    {
        ice::ResourceProvider* const result = provider.get();
        ice::multi_hashmap::insert(
            _resource_providers,
            ice::hash(provider->schemeid()),
            ice::move(provider)
        );
        return result;
    }

    auto ResourceTrackerImplementation::attach_writer(
        ice::UniquePtr<ice::ResourceWriter> writer
    ) noexcept -> ice::ResourceWriter*
    {
        ice::ResourceWriter* const result = writer.get();
        ice::ResourceProvider* const provider = attach_provider(ice::move(writer));
        ICE_ASSERT_CORE(provider != nullptr);

        ice::multi_hashmap::insert(
            _resource_writers,
            ice::hash(result->schemeid()),
            result
        );
        return result;
    }

    void ResourceTrackerImplementation::sync_resources() noexcept
    {
        IPT_ZONE_SCOPED;
        _devui_widget = ice::create_tracker_devui(_allocator, *this);

        ice::Array<ice::Resource*> temp_resources{ _allocator };
        for (auto const& provider : _resource_providers)
        {
            ice::array::clear(temp_resources);

            this->sync_provider(temp_resources, *provider);
        }
#if 0
        for (auto const& writer : _resource_writers)
        {
            ice::array::clear(temp_resources);

            this->sync_provider(temp_resources, *writer);
        }
#endif
    }

    auto ResourceTrackerImplementation::find_resource(
        ice::URI const& resource_uri,
        ice::ResourceFlags flags
    ) const noexcept -> ice::ResourceHandle
    {
        if (resource_uri.scheme() == ice::stringid_hash(ice::Scheme_URN))
        {
            return find_resource_by_urn(resource_uri, flags);
        }
        else
        {
            return find_resource_by_uri(resource_uri, flags);
        }
    }

    auto ResourceTrackerImplementation::find_resource_relative(
        ice::URI const& uri,
        ice::ResourceHandle const& handle
    ) const noexcept -> ice::ResourceHandle
    {
        ICE_ASSERT(handle != nullptr, "Trying to set resource from invalid handle!");

        ice::ResourceHandle result;
        ice::Resource const* resource = ice::internal_provider(handle)->resolve_relative_resource(uri, handle);
        if (resource != nullptr)
        {
            result = this->find_resource(resource->uri(), resource->flags());
        }
        return result;
    }

    auto ResourceTrackerImplementation::filter_resource_provider_uris(
        ice::ResourceProvider& provider,
        ice::ResourceFilter const& filter,
        ice::Array<ice::URI>& out_uris
    ) const noexcept -> ice::TaskExpected<ice::u32>
    {
        co_return co_await provider.filter_resource_uris(filter, out_uris);
    }

    auto ResourceTrackerImplementation::filter_resource_uris(
        ice::ResourceFilter const& filter,
        ice::Array<ice::URI>& out_uris
    ) const noexcept -> ice::TaskExpected<ice::u32>
    {
        ice::u32 collected = 0;
        for (ice::UniquePtr<ice::ResourceProvider> const& provider : _resource_providers)
        {
            static constexpr ice::String const keywords[]{ "blocked", "allowed" };
            bool const scheme_allowed = filter.allows_scheme(provider->schemeid());
            bool const hostname_allowed = filter.allows_hostname(provider->hostname());
            ICE_LOG_IF(
                !scheme_allowed || !hostname_allowed,
                LogSeverity::Debug, LogTag::Engine,
                "Provider was filtered out. [scheme: {} ({}), hostname: {} ({})]",
                provider->schemeid(), keywords[i32(scheme_allowed)],
                provider->hostname(), keywords[i32(hostname_allowed)]
            );
            if (scheme_allowed && hostname_allowed)
            {
                collected += co_await filter_resource_provider_uris(*provider, filter, out_uris);
            }
        }
        co_return collected;
    }

    auto ResourceTrackerImplementation::set_resource(
        ice::URI const& uri,
        ice::ResourceHandle const& resource_handle
    ) noexcept -> ice::Task<ice::ResourceResult>
    {
        co_return{ .resource_status = ResourceStatus::Invalid };
    }

    auto ResourceTrackerImplementation::load_resource(
        ice::ResourceHandle const& resource_handle
    ) noexcept -> ice::Task<ice::ResourceResult>
    {
        ice::TaskTransaction transaction;
        ice::ResourceLoadTransaction load_transaction{ resource_handle, transaction };
        if (co_await load_transaction == E_ResourceLoadNeeded)
        {
            ice::internal_set_status(resource_handle, ResourceStatus::Loading);

            ice::Expected<ice::Data, ice::ErrorCode> const result
                = co_await ice::internal_provider(resource_handle)->load_resource(resource_handle, {});

            ice::ResourceStatus new_status = ResourceStatus::Invalid;
            if (result.failed())
            {
                ICE_LOG(
                    LogSeverity::Error, LogTag::Engine,
                    "Failed to load resource {} with error: {}",
                    ice::resource_origin(resource_handle),
                    result.error()
                );
            }
            else
            {
                ice::internal_set_data(resource_handle, result.value());
                new_status = ResourceStatus::Loaded;
            }

            // Process all awaiting load contexts.
            // TODO: Schedule all of them instead?
            load_transaction.finalize(new_status);
        }
        else
        {
            ICE_ASSERT_CORE(ice::internal_status(resource_handle) == ResourceStatus::Loaded);
        }

        co_return ice::ResourceResult{
            .resource_status = ice::internal_status(resource_handle),
            .resource = resource_handle,
            .data = ice::internal_data(resource_handle),
        };
    }

    auto ResourceTrackerImplementation::release_resource(
        ice::ResourceHandle const& resource_handle
    ) noexcept -> ice::Task<ice::ResourceResult>
    {
        ICE_ASSERT(resource_handle != nullptr, "Trying to release resource from invalid handle!");

        // TODO: Maybe remove?
        // TODO: Notify the related provider?

        co_return ice::ResourceResult{
            .resource_status = internal_status(resource_handle),
            .resource = resource_handle,
            .data = { .size = internal_data(resource_handle).size },
        };
    }

    auto ResourceTrackerImplementation::unload_resource(
        ice::ResourceHandle const& resource_handle
    ) noexcept -> ice::Task<ice::ResourceResult>
    {
        ICE_ASSERT(resource_handle != nullptr, "Trying to unload resource from invalid handle!");

        // TODO: Maybe remove?
        // TODO: Notify the related provider?

        co_return ice::ResourceResult{
            .resource_status = internal_status(resource_handle),
            .resource = resource_handle,
            .data = { .size = internal_data(resource_handle).size },
        };
    }

    auto ResourceTrackerImplementation::create_resource(
        ice::URI const& resource_uri
    ) noexcept -> ice::TaskExpected<ice::ResourceHandle>
    {
        ice::u64 const hash_scheme = ice::hash(resource_uri.scheme());
        ice::ResourceWriter* writer = nullptr;

        auto it = ice::multi_hashmap::find_first(_resource_writers, hash_scheme);
        while (it != nullptr)
        {
            [[maybe_unused]]
            ice::ResourceWriter* candidate_writer = it.value();

            // We only allow to create resources for specific writers.
            if (candidate_writer->hostname() == resource_uri.host())
            {
                writer = candidate_writer;
            }
            it = ice::multi_hashmap::find_next(_resource_writers, it);
        }

        if (writer == nullptr)
        {
            co_return E_FailedToFindValidResourceWriter;
        }

        ice::Resource* resource = writer->find_resource(resource_uri);
        if (resource == nullptr)
        {
            resource = co_await writer->create_resource(
                resource_uri, ResourceCreationFlags::Overwrite
            );

            // TODO: Only save the new resource if it's not yet there.
            ice::multi_hashmap::insert(_resources, ice::hash(resource->name()), resource);
        }

        co_return ice::ResourceHandle{ resource };
    }

    auto ResourceTrackerImplementation::write_resource(
        ice::URI const& uri,
        ice::Data data,
        ice::usize write_offset
    ) noexcept -> ice::TaskExpected<bool>
    {
        ice::ResourceHandle resource = co_await create_resource(uri);
        ice::ResourceWriter* const writer = static_cast<ice::ResourceWriter*>(ice::internal_provider(resource));
        co_return co_await writer->write_resource(resource, data, write_offset);
    }

    auto ResourceTrackerImplementation::write_resource(
        ice::ResourceHandle const& resource,
        ice::Data data,
        ice::usize write_offset
    ) noexcept -> ice::TaskExpected<bool>
    {
        ice::ResourceWriter* const writer = static_cast<ice::ResourceWriter*>(ice::internal_provider(resource));
        co_return co_await writer->write_resource(resource, data, write_offset);
    }

    void ResourceTrackerImplementation::sync_provider(
        ice::Array<ice::Resource*>& out_resources,
        ice::ResourceProvider& provider
    ) noexcept
    {
        ice::ResourceProviderResult const refresh_result = provider.refresh(out_resources);
        if (refresh_result == ResourceProviderResult::Failure)
        {
            ICE_LOG(
                ice::LogSeverity::Warning, ice::LogTag::Engine,
                "Failed to refresh resource provider for scheme: {}",
                ice::stringid_hint(provider.schemeid())
            );
            return;
        }

        ice::u32 const new_count = ice::hashmap::count(_resources) + ice::array::count(out_resources);
        ICE_ASSERT(
            new_count <= _info.predicted_resource_count,
            "Maximum resource capacity of {} entiries reached!",
            _info.predicted_resource_count
        );

        ice::hashmap::reserve(_resources, new_count);

        // Store all resource handles
        IPT_ZONE_SCOPED_NAMED("create_hash_entries");
        for (ice::Resource* resource : out_resources)
        {
            ice::multi_hashmap::insert(
                _resources,
                ice::hash(resource->name()),
                resource
            );
        }
    }

    auto ResourceTrackerImplementation::find_resource_by_urn(
        ice::URI const& resource_urn,
        ice::ResourceFlags flags
    ) const noexcept -> ice::ResourceHandle
    {
        ICE_ASSERT(
            resource_urn.scheme() == ice::stringid_hash(Scheme_URN),
            "Invalid URI object. Expected resource name got [{}]",
            resource_urn.scheme()
        );

        ice::ResourceHandle result;
        ice::u64 const hash_resouce = ice::hash(resource_urn.path());

        // Just grab the first for now
        auto it = ice::multi_hashmap::find_first(_resources, hash_resouce);
        if (it != nullptr)
        {
            result = ice::ResourceHandle{ it.value() };
        }
        return result;
    }

    auto ResourceTrackerImplementation::find_resource_by_uri(
        ice::URI const& resource_uri,
        ice::ResourceFlags flags
    ) const noexcept -> ice::ResourceHandle
    {
        ice::ResourceHandle handle;
        ice::Resource const* resource = nullptr;
        ice::ResourceProvider const* provider = nullptr;

        if (find_resource_and_provider(resource_uri, provider, resource) == false)
        {
            return {};
        }

        // TODO: Remove this abomination, we only need to remove the const'ness in the 'find' functions for the resource object.
        ice::u64 const hash_resouce = ice::hash(resource->name());
        auto it = ice::multi_hashmap::find_first(_resources, hash_resouce);
        while (it != nullptr && handle == nullptr)
        {
            if ((*it) == resource)
            {
                handle = ice::ResourceHandle{ *it };
            }
            else
            {
                it = ice::multi_hashmap::find_next(_resources, it);
            }
        }

        return handle;
    }

    bool ResourceTrackerImplementation::find_resource_and_provider(
        ice::URI const& resource_uri,
        ice::ResourceProvider const*& provider,
        ice::Resource const*& resource
    ) const noexcept
    {
        ice::u64 const hash_scheme = ice::hash(resource_uri.scheme());

        auto it = ice::multi_hashmap::find_first(_resource_providers, hash_scheme);
        while (it != nullptr && provider == nullptr)
        {
            if (resource = (*it)->find_resource(resource_uri); resource != nullptr)
            {
                provider = (*it).get();
            }
            else
            {
                it = ice::multi_hashmap::find_next(_resource_providers, it);
            }
        }

        return provider != nullptr;
    }

    auto create_resource_tracker(
        ice::Allocator& alloc,
        ice::ResourceTrackerCreateInfo const& create_info
    ) noexcept -> ice::UniquePtr<ice::ResourceTracker>
    {
        return ice::make_unique<ice::ResourceTrackerImplementation>(alloc, alloc, create_info);
    }

    auto resolve_dynlib_path(
        ice::ResourceTracker const& tracker,
        ice::Allocator& alloc,
        ice::String name
    ) noexcept -> ice::HeapString<>
    {
        // Try to reslove the name of the library with a dynlib URI.
        ice::URI const dynlib_uri{ ice::Scheme_Dynlib, name };
        if (ice::ResourceHandle res = tracker.find_resource(dynlib_uri); res != nullptr)
        {
            return { alloc, ice::resource_origin(res) };
        }

        // ... next with a URN with the expected library file name.
        ice::HeapString<> result{ alloc, name };
        if (ice::path::extension(name) == "") // Check that we have already a full file name.
        {
            if constexpr (ice::build::is_windows)
            {
                result.push_back(".dll");
            }
            else
            {
                // On unix we expect the library to be prepended with 'lib' and appended with '.so'
                result.clear();
                result.push_back("lib");
                result.push_back(name);
                result.push_back(".so");
            }
        }

        ice::URI const urn_uri{ ice::Scheme_URN, result };
        if (ice::ResourceHandle res = tracker.find_resource(urn_uri); res != nullptr)
        {
            return { alloc, ice::resource_origin(res) };
        }

        // ... and finally return the unresolved file name.
        return result;
    }

} // namespace ice
