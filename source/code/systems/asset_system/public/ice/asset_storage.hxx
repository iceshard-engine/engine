#pragma once
#include <ice/uri.hxx>
#include <ice/task.hxx>
#include <ice/asset.hxx>
#include <ice/asset_type.hxx>
#include <ice/asset_type_archive.hxx>
#include <ice/resource_types.hxx>
#include <ice/unique_ptr.hxx>

namespace ice
{

    class AssetTypeArchive;

    class AssetRequest
    {
    public:
        enum class Result
        {
            Error,
            Success
        };

        virtual ~AssetRequest() noexcept = default;

        virtual auto state() const noexcept -> ice::AssetState = 0;
        virtual auto data() const noexcept -> ice::Data = 0;

        virtual auto resource() const noexcept -> ice::Resource_v2 const& = 0;
        virtual auto asset_definition() const noexcept -> ice::AssetTypeDefinition const& = 0;

        virtual auto allocate(ice::u32 size) const noexcept -> ice::Memory = 0;

        virtual void resolve(
            ice::AssetRequest::Result result,
            ice::Memory memory
        ) noexcept = 0;
    };

    class AssetStorage
    {
    public:
        virtual ~AssetStorage() = default;

        // Can return a raw, baked and loaded asset representation.
        virtual auto request(
            ice::AssetType_v2 type,
            ice::Utf8String name,
            ice::AssetState requested_state
        ) noexcept -> ice::Task<ice::Asset_v2> = 0;

        // Can ask the storage for work and pre-load assets into runtime objects.
        // ex.: Image Asset -> Vulkan Texture Handle
        virtual auto aquire_request(
            ice::AssetType_v2 type,
            ice::AssetState requested_state
        ) noexcept -> ice::AssetRequest* = 0;

        // Asset is no longer used
        virtual auto release(
            ice::Asset_v2 asset
        ) noexcept -> ice::Task<> = 0;
    };

    auto create_asset_storage(
        ice::Allocator& alloc,
        ice::ResourceTracker& resource_tracker,
        ice::UniquePtr<ice::AssetTypeArchive> asset_archive
    ) noexcept -> ice::UniquePtr<ice::AssetStorage>;

} // namespace ice
