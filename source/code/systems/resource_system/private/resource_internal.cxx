/// Copyright 2025 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "resource_internal.hxx"

#include <ice/resource_status.hxx>
#include <ice/resource_provider.hxx>

namespace ice
{

    ResourceInternal::ResourceInternal(
        ice::ResourceProvider& provider
    ) noexcept
        : _provider{ provider }
        , _refcount{ 0 }
        , _reqcount{ 0 }
        , _status{ ResourceStatus::Available }
        , _last_data{ }
    {
    }

    ResourceInternal::~ResourceInternal() noexcept
    {
        ICE_ASSERT_CORE(_refcount.load(std::memory_order_relaxed) == 0);
    }

    auto ResourceInternal::aquire() noexcept -> ice::u32
    {
        return _refcount.fetch_add(1u, std::memory_order_relaxed);
    }

    auto ResourceInternal::release() noexcept -> ice::u32
    {
        return _refcount.fetch_sub(1u, std::memory_order_relaxed);
    }

    auto allocate_resource_object_memory(
        ice::Allocator& alloc,
        ice::ResourceProvider& provider,
        ice::usize size,
        ice::ualign align
    ) noexcept -> ice::Memory
    {
        ICE_ASSERT_CORE(ice::align_of<ResourceInternal> >= align);
        ice::AllocResult const result = alloc.allocate({ ice::size_of<ResourceInternal> + size, ice::align_of<ResourceInternal> });

        // Initialize the internal object.
        new (result.memory) ResourceInternal{ provider };

        return ice::ptr_add(result, ice::size_of<ResourceInternal>);
    }

    void deallocate_resource_object_memory(ice::Allocator& alloc, ice::Resource* pointer) noexcept
    {
        ice::ResourceInternal* internal = internal_ptr(pointer);
        ICE_ASSERT_CORE(internal->_status != ResourceStatus::Invalid);

        // Call the Internal object dtor since we created the object in that memory space
        internal->~ResourceInternal();

        alloc.deallocate(internal);
    }

    auto internal_ptr(ice::Resource* resource) noexcept -> ice::ResourceInternal*
    {
        return reinterpret_cast<ice::ResourceInternal*>(
            ice::ptr_sub(resource, ice::size_of<ice::ResourceInternal>)
        );
    }

    auto internal_aquire(ice::Resource* resource) noexcept -> ice::Resource*
    {
        if (resource != nullptr)
        {
            internal_ptr(resource)->aquire();
        }
        return resource;
    }

    void internal_release(ice::Resource* resource) noexcept
    {
        if (resource != nullptr)
        {
            ice::ResourceInternal* const internal = internal_ptr(resource);
            ice::u32 const prev_count = internal->release();
            ICE_ASSERT_CORE(prev_count > 0);

            if (prev_count == 1)
            {
                // TODO: When unloading properly reset the resource status.
                internal->_reqcount.store(0, std::memory_order_relaxed);
                internal->_status = ice::ResourceStatus::Available;
                internal->_last_data = {};
                internal->_provider.unload_resource(resource);
            }
        }
    }

    auto internal_provider(ice::Resource* resource) noexcept -> ice::ResourceProvider*
    {
        return resource != nullptr
            ? ice::addressof(internal_ptr(resource)->_provider)
            : nullptr;
    }

    auto internal_status(ice::Resource* resource) noexcept -> ice::ResourceStatus
    {
        return resource != nullptr
            ? internal_ptr(resource)->_status
            : ResourceStatus::Invalid;
    }

    void internal_set_status(ice::Resource* resource, ice::ResourceStatus status) noexcept
    {
        if (resource != nullptr)
        {
            internal_ptr(resource)->_status = status;
        }
    }

    auto internal_data(ice::Resource* resource) noexcept -> ice::Data
    {
        return resource != nullptr
            ? internal_ptr(resource)->_last_data
            : ice::Data{};
    }

    void internal_set_data(ice::Resource* resource, ice::Data data) noexcept
    {
        if (resource != nullptr)
        {
            internal_ptr(resource)->_last_data = data;
        }
    }

} // namespace ice
