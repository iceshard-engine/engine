/// Copyright 2025 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/resource_handle.hxx>
#include "resource_internal.hxx"

namespace ice
{

    ResourceHandle::ResourceHandle(ice::Resource* resource) noexcept
        : resource{ ice::internal_aquire(resource) }
    {
    }

    ResourceHandle::~ResourceHandle() noexcept
    {
        ice::internal_release(resource);
    }

    ResourceHandle::ResourceHandle(ResourceHandle&& other) noexcept
        : resource{ ice::exchange(other.resource, nullptr) }
    {
    }

    ResourceHandle::ResourceHandle(ResourceHandle const& other) noexcept
        : resource{ ice::internal_aquire(other.resource) }
    {
    }

    auto ResourceHandle::operator=(ResourceHandle&& other) noexcept -> ResourceHandle&
    {
        if (this != ice::addressof(other))
        {
            if (resource != nullptr)
            {
                ice::internal_release(resource);
            }

            resource = ice::exchange(other.resource, nullptr);
        }
        return *this;
    }

    auto ResourceHandle::operator=(ResourceHandle const& other) noexcept -> ResourceHandle&
    {
        if (this != ice::addressof(other))
        {
            if (resource != nullptr)
            {
                ice::internal_release(resource);
            }

            resource = ice::internal_aquire(other.resource);
        }
        return *this;
    }

} // namespace ice
