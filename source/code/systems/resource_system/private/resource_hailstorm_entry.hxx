/// Copyright 2023 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <hailstorm/hailstorm.hxx>
#include <ice/resource.hxx>
#include <ice/uri.hxx>

namespace ice
{

    class HailstormChunkLoader;

    class HailstormResource : public ice::Resource
    {
    public:
        HailstormResource(
            ice::URI const& uri,
            hailstorm::HailstormResource const& handle
        ) noexcept;

        virtual auto uri() const noexcept -> ice::URI const& override { return _uri; }
        virtual auto flags() const noexcept -> ice::ResourceFlags override { return {}; }

        virtual auto name() const noexcept -> ice::String override
        {
            return ice::string::substr(_uri.path, ice::string::find_first_of(_uri.path, '.') + 5);
        }
        virtual auto origin() const noexcept -> ice::String override { return _uri.path; }

        hailstorm::HailstormResource const& _handle;

    protected:
        ice::URI _uri;
    };

    class HailstormResourceMixed : public ice::HailstormResource
    {
    public:
        HailstormResourceMixed(
            ice::URI const& uri,
            hailstorm::HailstormResource const& handle,
            ice::HailstormChunkLoader& loader
        ) noexcept;

        auto load_metadata() const noexcept -> ice::Task<ice::Data> override;

    private:
        ice::HailstormChunkLoader& _loader;
    };

    class HailstormResourceSplit : public ice::HailstormResource
    {
    public:
        HailstormResourceSplit(
            ice::URI const& uri,
            hailstorm::HailstormResource const& handle,
            ice::HailstormChunkLoader& meta_loader,
            ice::HailstormChunkLoader& data_loader
        ) noexcept;

        auto load_metadata() const noexcept -> ice::Task<ice::Data> override;

    private:
        ice::HailstormChunkLoader& _meta_loader;
        ice::HailstormChunkLoader& _data_loader;
    };

} // namespace ice
