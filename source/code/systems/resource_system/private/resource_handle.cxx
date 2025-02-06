/// Copyright 2025 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/resource_handle.hxx>
#include "resource_internal.hxx"

namespace ice
{

    ResourceHandle::ResourceHandle(ice::Resource* resource) noexcept
        : _resource{ ice::internal_aquire(resource) }
    {
    }

    ResourceHandle::~ResourceHandle() noexcept
    {
        ice::internal_release(_resource);
    }

    ResourceHandle::ResourceHandle(ResourceHandle&& other) noexcept
        : _resource{ ice::exchange(other._resource, nullptr) }
    {
    }

    ResourceHandle::ResourceHandle(ResourceHandle const& other) noexcept
        : _resource{ ice::internal_aquire(other._resource) }
    {
    }

    auto ResourceHandle::operator=(ResourceHandle&& other) noexcept -> ResourceHandle&
    {
        if (this != ice::addressof(other))
        {
            if (_resource != nullptr)
            {
                ice::internal_release(_resource);
            }

            _resource = ice::exchange(other._resource, nullptr);
        }
        return *this;
    }

    auto ResourceHandle::operator=(ResourceHandle const& other) noexcept -> ResourceHandle&
    {
        if (this != ice::addressof(other))
        {
            if (_resource != nullptr)
            {
                ice::internal_release(_resource);
            }

            _resource = ice::internal_aquire(other._resource);
        }
        return *this;
    }

} // namespace ice
