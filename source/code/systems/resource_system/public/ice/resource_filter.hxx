/// Copyright 2025 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/config.hxx>
#include <ice/path_utils.hxx>
#include <ice/resource.hxx>
#include <ice/uri.hxx>

namespace ice
{

    class ResourceFilter
    {
    public:
        virtual ~ResourceFilter() noexcept = default;

        virtual auto filter_thread() const noexcept -> ice::TaskScheduler* { return nullptr; }

        virtual bool requires_metadata() const noexcept { return false; }

        virtual bool allows_scheme(ice::StringID_Arg scheme) const noexcept { return true; }

        virtual bool allows_hostname(ice::String hostname) const noexcept { return true; }

        virtual bool allows_resource(
            ice::Resource const* resource
        ) const noexcept = 0;

        virtual bool allows_metadata(
            ice::Config const& metadata
        ) const noexcept { return false; }
    };

    class FileResourceFilter : public ResourceFilter
    {
    public:
        FileResourceFilter(ice::String extension, ice::String hostname = {}) noexcept
            : _extension{ extension }
            , _hostname{ hostname }
        { }

        bool allows_hostname(ice::String hostname) const noexcept override
        {
            return _hostname.is_empty() || hostname == _hostname;
        }

        bool allows_resource(
            ice::Resource const* resource
        ) const noexcept override
        {
            return _extension.is_empty() || ice::path::extension(resource->origin()) == _extension;
        }

    private:
        ice::String _extension;
        ice::String _hostname;
    };

} // namespace ice
