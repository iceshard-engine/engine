/// Copyright 2025 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/resource.hxx>
#include <ice/sync_manual_events.hxx>

namespace ice
{

    class ResourceInternal final
    {
    public:
        ResourceInternal(
            ice::ResourceProvider& provider
        ) noexcept;
        ~ResourceInternal() noexcept;

        auto aquire() noexcept -> ice::u32;
        auto release() noexcept -> ice::u32;

    public:
        ice::ResourceProvider& _provider;
        std::atomic<ice::u16> _refcount;
        std::atomic<ice::i16> _reqcount;
        ice::ResourceStatus _status;
        ice::Data _last_data;
    };

    auto internal_ptr(ice::Resource* resource) noexcept -> ice::ResourceInternal*;
    auto internal_aquire(ice::Resource* resource) noexcept -> ice::Resource*;
    void internal_release(ice::Resource* resource) noexcept;
    auto internal_provider(ice::Resource* handle) noexcept -> ice::ResourceProvider*;
    auto internal_status(ice::Resource* resource) noexcept -> ice::ResourceStatus;
    void internal_set_status(ice::Resource* resource, ice::ResourceStatus status) noexcept;
    auto internal_data(ice::Resource* resource) noexcept -> ice::Data;
    void internal_set_data(ice::Resource* resource, ice::Data data) noexcept;

} // namespace ice
