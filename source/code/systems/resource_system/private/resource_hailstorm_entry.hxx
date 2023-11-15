#pragma once
#include <ice/resource_hailstorm.hxx>
#include <ice/resource_meta.hxx>
#include <ice/resource.hxx>
#include <ice/uri.hxx>

//#include "resource_provider_hailstorm.hxx"
#include "native/native_fileio.hxx"

namespace ice
{

    class HailstormChunkLoader;

    class HailstormResource : public ice::Resource
    {
    public:
        HailstormResource(
            ice::URI const& uri,
            ice::hailstorm::HailstormResource const& handle
        ) noexcept;

        virtual auto uri() const noexcept -> ice::URI const& override { return _uri; }
        virtual auto flags() const noexcept -> ice::ResourceFlags override { return {}; }

        virtual auto name() const noexcept -> ice::String override
        {
            return ice::string::substr(_uri.path, ice::string::find_first_of(_uri.path, '.') + 5);
        }
        virtual auto origin() const noexcept -> ice::String override { return _uri.path; }

        ice::hailstorm::HailstormResource const& _handle;

    protected:
        ice::URI _uri;
    };

    class HailstormResourceMixed : public ice::HailstormResource
    {
    public:
        HailstormResourceMixed(
            ice::URI const& uri,
            ice::hailstorm::HailstormResource const& handle,
            ice::HailstormChunkLoader& loader
        ) noexcept;

        auto load_metadata(ice::Metadata& out_view) const noexcept -> ice::Task<bool>;

    private:
        ice::HailstormChunkLoader& _loader;
    };

    class HailstormResourceSplit : public ice::HailstormResource
    {
    public:
        HailstormResourceSplit(
            ice::URI const& uri,
            ice::hailstorm::HailstormResource const& handle,
            ice::HailstormChunkLoader& meta_loader,
            ice::HailstormChunkLoader& data_loader
        ) noexcept;

        auto load_metadata(ice::Metadata& out_view) const noexcept -> ice::Task<bool> override;

    private:
        ice::HailstormChunkLoader& _meta_loader;
        ice::HailstormChunkLoader& _data_loader;
    };

} // namespace ice
